#include "platform/windows/main_window.hpp"

#include "platform/windows/dpi.hpp"
#include "platform/windows/resource.h"

#include <algorithm>
#include <commctrl.h>
#include <utility>

namespace stay_awake::windows
{
namespace
{
enum ControlId
{
    awake_heading = 1000,
    display,
    system,
    awake_duration,
    awake_status,
    close_heading,
    close_button,
    close_duration,
    refresh,
    show_details,
    highlight,
    process_name,
    window_position,
    close_status,
    catalog_status,
    window_list,
};
constexpr UINT_PTR countdown_timer = 1;

bool CanScrollList(const UINT message, const WPARAM wparam) noexcept
{
    switch (message)
    {
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_CHAR: // Native type-to-select can bring an off-screen item into view.
        return true;
    case WM_KEYDOWN:
        return wparam == VK_UP || wparam == VK_DOWN || wparam == VK_LEFT || wparam == VK_RIGHT || wparam == VK_PRIOR ||
               wparam == VK_NEXT || wparam == VK_HOME || wparam == VK_END;
    case WM_VSCROLL:
    case WM_HSCROLL:
        // Thumb dragging already follows the pointer directly; only suppress incremental scrolling.
        return LOWORD(wparam) == SB_LINEUP || LOWORD(wparam) == SB_LINEDOWN || LOWORD(wparam) == SB_PAGEUP ||
               LOWORD(wparam) == SB_PAGEDOWN || LOWORD(wparam) == SB_TOP || LOWORD(wparam) == SB_BOTTOM;
    default:
        return false;
    }
}

struct ListPaintState
{
    explicit ListPaintState(HWND window) noexcept
        : top(SendMessageW(window, LB_GETTOPINDEX, 0, 0)), horizontal(GetScrollPos(window, SB_HORZ)),
          selection(SendMessageW(window, LB_GETCURSEL, 0, 0)), caret(SendMessageW(window, LB_GETCARETINDEX, 0, 0)),
          focused(GetFocus() == window), ui_state(SendMessageW(window, WM_QUERYUISTATE, 0, 0))
    {
    }

    LRESULT top;
    int horizontal;
    LRESULT selection;
    LRESULT caret;
    bool focused;
    LRESULT ui_state;
};

void InvalidateListItem(HWND window, const LRESULT index) noexcept
{
    if (index == LB_ERR)
    {
        return;
    }
    RECT item{};
    RECT client{};
    if (SendMessageW(window, LB_GETITEMRECT, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&item)) == LB_ERR ||
        !GetClientRect(window, &client))
    {
        InvalidateRect(window, nullptr, TRUE);
        return;
    }
    // Selection and focus decoration can span the whole row, including past the text's horizontal extent.
    item.left = client.left;
    item.right = client.right;
    RECT visible{};
    if (IntersectRect(&visible, &item, &client))
    {
        InvalidateRect(window, &visible, TRUE);
    }
}

class ListRedrawScope
{
  public:
    ListRedrawScope(HWND window, HWND combo, bool& active) noexcept
        : window_(window), combo_(combo), active_(active), before_(window)
    {
        active_ = true;
        SendMessageW(window_, WM_SETREDRAW, FALSE, 0);
    }
    ~ListRedrawScope() noexcept
    {
        if (IsWindow(window_))
        {
            const bool keep_visible =
                !combo_ || (IsWindowVisible(combo_) && SendMessageW(combo_, CB_GETDROPPEDSTATE, 0, 0) != FALSE);
            SendMessageW(window_, WM_SETREDRAW, TRUE, 0);
            if (keep_visible)
            {
                const ListPaintState after(window_);
                if (before_.top != after.top || before_.horizontal != after.horizontal)
                {
                    // Scrolling changes the viewport and scrollbar thumbs. Queue one repaint so successive
                    // inputs can coalesce; keep erasure for empty space and partially visible rows.
                    RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_NOCHILDREN);
                }
                else
                {
                    // Navigation within the viewport only changes selection/focus rows. Input at a scroll
                    // boundary (or a partial wheel delta) need not invalidate the whole control again.
                    if (before_.selection != after.selection)
                    {
                        InvalidateListItem(window_, before_.selection);
                        InvalidateListItem(window_, after.selection);
                    }
                    if (before_.caret != after.caret || before_.focused != after.focused ||
                        before_.ui_state != after.ui_state)
                    {
                        InvalidateListItem(window_, before_.caret);
                        InvalidateListItem(window_, after.caret);
                    }
                }
            }
            else
            {
                // WM_SETREDRAW(TRUE) can restore WS_VISIBLE after native input closed the dropdown.
                ShowWindow(window_, SW_HIDE);
            }
        }
        active_ = false;
    }
    ListRedrawScope(const ListRedrawScope&) = delete;
    ListRedrawScope& operator=(const ListRedrawScope&) = delete;

  private:
    HWND window_;
    HWND combo_;
    bool& active_;
    ListPaintState before_;
};
} // namespace

MainWindow::MainWindow(EventHandler handler) : handler_(std::move(handler))
{
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_STANDARD_CLASSES};
    Require(InitCommonControlsEx(&common) != FALSE, "Initialize native controls");
    WNDCLASSEXW cls{};
    cls.cbSize = sizeof(cls);
    cls.lpfnWndProc = WindowProc;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    cls.lpszClassName = L"StayAwake.MainWindow";
    Require(RegisterClassExW(&cls) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Register main window");
    const auto size = OuterSize(96);
    // No WS_VISIBLE: hidden startup never flashes a window, and pending activation is consumed only after setup.
    window_.Reset(CreateWindowExW(main_ex_style, cls.lpszClassName, L"Stay Awake", main_style, CW_USEDEFAULT,
                                  CW_USEDEFAULT, size.cx, size.cy, nullptr, nullptr, cls.hInstance, this));
    Require(window_.Get() != nullptr, "Create main window");
    const auto system_menu = GetSystemMenu(window_.Get(), FALSE);
    Require(AppendMenuW(system_menu, MF_SEPARATOR, 0, nullptr) != FALSE, "Extend system menu");
    Require(AppendMenuW(system_menu, MF_STRING, 0x1000, L"Quit Stay Awake") != FALSE, "Add system Quit action");
    CreateControls();
    Layout(GetDpiForWindow(window_.Get()));
    KeepOnWorkArea(window_.Get());
}

MainWindow::~MainWindow()
{
    handler_ = {};
    tray_ = nullptr;
    if (window_.Get())
    {
        KillTimer(window_.Get(), countdown_timer);
    }
    window_.Reset();
}

HWND MainWindow::Get() const
{
    return window_.Get();
}

HICON MainWindow::SmallIcon() const
{
    return small_icon_.Get();
}

void MainWindow::SetTray(TrayIcon* tray)
{
    tray_ = tray;
}

void MainWindow::ClearHandler()
{
    handler_ = {};
}

std::exception_ptr MainWindow::Failure() const
{
    return failure_;
}

HWND MainWindow::Add(const int id, const wchar_t* cls, const wchar_t* text, const DWORD style, const int x, const int y,
                     const int width, const int height, const DWORD ex_style)
{
    const auto child =
        CreateWindowExW(ex_style, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, window_.Get(),
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    Require(child != nullptr, "Create native control");
    controls_.push_back({child, x, y, width, height});
    return child;
}

void MainWindow::CreateControls()
{
    constexpr DWORD button = BS_PUSHBUTTON | WS_TABSTOP;
    constexpr DWORD static_text = SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS;
    constexpr DWORD combo = CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP;
    // Coordinates are logical 96-DPI units.
    Add(-1, L"STATIC", L"", SS_ETCHEDHORZ, 18, 22, 24, 2);
    Add(awake_heading, L"STATIC", L"Stay Awake", SS_CENTER | SS_CENTERIMAGE, 42, 11, 124, 20);
    Add(-1, L"STATIC", L"", SS_ETCHEDHORZ, 166, 22, 600, 2);
    Add(display, L"BUTTON", L"Require Display", button | WS_GROUP, 18, 37, 196, 30);
    Add(system, L"BUTTON", L"Require System", button, 18, 74, 196, 30);
    Add(-1, L"STATIC", L"Duration:", 0, 222, 42, 72, 24);
    Add(awake_duration, L"COMBOBOX", L"", combo, 300, 37, 264, 300);
    Add(-1, L"STATIC", L"Status:", 0, 20, 111, 62, 24);
    Add(awake_status, L"STATIC", L"", static_text, 84, 111, 682, 24);

    Add(-1, L"STATIC", L"", SS_ETCHEDHORZ, 18, 156, 24, 2);
    Add(close_heading, L"STATIC", L"Window Closer", SS_CENTER | SS_CENTERIMAGE, 42, 145, 124, 20);
    Add(-1, L"STATIC", L"", SS_ETCHEDHORZ, 166, 156, 600, 2);
    Add(close_button, L"BUTTON", L"Schedule Close Window", button | WS_GROUP, 18, 171, 196, 30);
    Add(-1, L"STATIC", L"After:", 0, 222, 176, 72, 24);
    Add(close_duration, L"COMBOBOX", L"", combo, 300, 171, 264, 300);
    Add(show_details, L"BUTTON", L"Show Details", button, 570, 171, 196, 30);
    Add(refresh, L"BUTTON", L"Refresh List", button, 18, 208, 196, 30);
    Add(-1, L"STATIC", L"Process Name:", 0, 222, 213, 130, 24);
    Add(process_name, L"STATIC", L"", static_text, 356, 213, 410, 24);
    Add(highlight, L"BUTTON", L"Highlight Window", button, 18, 245, 196, 30);
    Add(-1, L"STATIC", L"Window Position:", 0, 222, 250, 130, 24);
    Add(window_position, L"STATIC", L"", static_text, 356, 250, 410, 24);
    Add(-1, L"STATIC", L"Status:", 0, 20, 286, 62, 24);
    Add(close_status, L"STATIC", L"", static_text, 84, 286, 682, 24);
    Add(window_list, L"LISTBOX", L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP, 18, 315,
        748, 240, WS_EX_CLIENTEDGE);
    Add(-1, L"STATIC", L"Status:", 0, 20, 565, 62, 24);
    Add(catalog_status, L"STATIC", L"", static_text, 84, 565, 682, 24);
    for (const int id : {window_list, awake_duration, close_duration})
    {
        ConfigureListScrolling(id);
    }
}

void MainWindow::ConfigureListScrolling(const int id)
{
    const auto control = GetDlgItem(window_.Get(), id);
    Require(SetWindowSubclass(control, ListScrollProc, id, reinterpret_cast<DWORD_PTR>(this)) != FALSE,
            "Configure immediate list scrolling");
    if (id != window_list)
    {
        COMBOBOXINFO info{};
        info.cbSize = sizeof(info);
        Require(GetComboBoxInfo(control, &info) != FALSE, "Get duration dropdown list");
        // Mouse input reaches the popup directly; keyboard input can arrive through the combo box.
        // Both callbacks share one redraw scope for this dropdown when native processing forwards input.
        Require(SetWindowSubclass(info.hwndList, ListScrollProc, id, reinterpret_cast<DWORD_PTR>(this)) != FALSE,
                "Configure immediate dropdown scrolling");
    }
}

MainWindow::ListScrollState& MainWindow::ScrollState(const int id) noexcept
{
    return id == awake_duration ? awake_duration_scroll_
                                : (id == close_duration ? close_duration_scroll_ : window_list_scroll_);
}

void MainWindow::Layout(const UINT dpi)
{
    if (layout_active_)
    {
        return;
    }
    layout_active_ = true;
    try
    {
        dpi_ = dpi;
        UniqueFont next_font(CreateFontW(-MulDiv(12, static_cast<int>(dpi), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                                         FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI"));
        Require(next_font.Get() != nullptr, "Create DPI-scaled font");
        for (const auto& control : controls_)
        {
            SendMessageW(control.window, WM_SETFONT, reinterpret_cast<WPARAM>(next_font.Get()), TRUE);
        }
        // Switch ownership before any fallible layout work, keeping every child's font alive on failure too.
        font_.Reset(next_font.Release());
        for (const auto& control : controls_)
        {
            Require(MoveWindow(control.window, Scale(control.x, dpi), Scale(control.y, dpi), Scale(control.width, dpi),
                               Scale(control.height, dpi), TRUE) != FALSE,
                    "Lay out native controls");
        }
        SendDlgItemMessageW(window_.Get(), window_list, LB_SETITEMHEIGHT, 0, Scale(24, dpi));
        for (const auto id : {awake_duration, close_duration})
        {
            SendDlgItemMessageW(window_.Get(), id, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), Scale(23, dpi));
            SendDlgItemMessageW(window_.Get(), id, CB_SETITEMHEIGHT, 0, Scale(24, dpi));
        }
        const auto instance = GetModuleHandleW(nullptr);
        UniqueIcon small(static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_STAY_AWAKE), IMAGE_ICON,
                                                       GetSystemMetricsForDpi(SM_CXSMICON, dpi),
                                                       GetSystemMetricsForDpi(SM_CYSMICON, dpi), 0)));
        UniqueIcon large(static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_STAY_AWAKE), IMAGE_ICON,
                                                       GetSystemMetricsForDpi(SM_CXICON, dpi),
                                                       GetSystemMetricsForDpi(SM_CYICON, dpi), 0)));
        Require(small.Get() && large.Get(), "Load embedded icons");
        SendMessageW(window_.Get(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small.Get()));
        SendMessageW(window_.Get(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(large.Get()));
        if (tray_)
        {
            tray_->UpdateIcon(small.Get());
        }
        small_icon_.Reset(small.Release());
        large_icon_.Reset(large.Release());
        const auto size = OuterSize(dpi);
        Require(SetWindowPos(window_.Get(), nullptr, 0, 0, size.cx, size.cy,
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE,
                "Resize window for DPI");
        UpdateListExtent();
        layout_active_ = false;
    }
    catch (...)
    {
        layout_active_ = false;
        throw;
    }
}

void MainWindow::UpdateListExtent()
{
    const auto list = GetDlgItem(window_.Get(), window_list);
    const auto dc = GetDC(list);
    if (!dc)
    {
        return;
    }
    const auto previous = SelectObject(dc, font_.Get());
    int extent = 0;
    for (const auto& title : titles_)
    {
        SIZE size{};
        if (GetTextExtentPoint32W(dc, title.data(), static_cast<int>(title.size()), &size))
        {
            extent = std::max(extent, static_cast<int>(size.cx));
        }
    }
    SelectObject(dc, previous);
    ReleaseDC(list, dc);
    SendMessageW(list, LB_SETHORIZONTALEXTENT, extent + Scale(12, dpi_), 0);
}

void MainWindow::Text(const int id, const std::string_view text)
{
    const auto value = ToWide(text);
    const auto control = GetDlgItem(window_.Get(), id);
    const int length = GetWindowTextLengthW(control);
    std::wstring previous(static_cast<std::size_t>(length) + 1, L'\0');
    previous.resize(GetWindowTextW(control, previous.data(), static_cast<int>(previous.size())));
    if (previous != value)
    {
        SetWindowTextW(control, value.c_str()); // Avoid repainting unchanged text on unrelated ticks.
    }
}

void MainWindow::Enable(const int id, const bool enabled)
{
    EnableWindow(GetDlgItem(window_.Get(), id), enabled);
}

void MainWindow::SelectDuration(const int id, const std::vector<DurationChoice>& choices,
                                const std::optional<std::chrono::seconds> duration)
{
    int index = -1;
    for (std::size_t i = 0; i < choices.size(); ++i)
    {
        if (duration && choices[i].duration == *duration)
        {
            index = static_cast<int>(i);
            break;
        }
    }
    if (SendDlgItemMessageW(window_.Get(), id, CB_GETCURSEL, 0, 0) != index)
    {
        SendDlgItemMessageW(window_.Get(), id, CB_SETCURSEL, index, 0);
    }
}

void MainWindow::Present(const ViewState& state)
{
    rendering_ = true;
    if (!durations_loaded_)
    {
        awake_durations_ = state.awake.durations;
        close_durations_ = state.close.durations;
        for (const auto& [id, choices] :
             {std::pair{awake_duration, &awake_durations_}, std::pair{close_duration, &close_durations_}})
        {
            for (const auto& choice : *choices)
            {
                const auto label = ToWide(choice.label);
                const auto result =
                    SendDlgItemMessageW(window_.Get(), id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
                Require(result != CB_ERR && result != CB_ERRSPACE, "Populate duration choices");
            }
        }
        durations_loaded_ = true;
    }
    SelectDuration(awake_duration, awake_durations_, state.awake.duration);
    SelectDuration(close_duration, close_durations_, state.close.duration);
    Text(display, state.awake.display_caption);
    Text(system, state.awake.system_caption);
    Text(close_button, state.close.caption);
    Text(highlight, state.selection.highlight_caption);
    Text(awake_status, state.awake.status);
    Text(close_status, state.close.status);
    Text(catalog_status, state.selection.catalog_status);
    Text(process_name, state.selection.process_name);
    Text(window_position, state.selection.window_position);
    Enable(display, state.awake.display_enabled);
    Enable(system, state.awake.system_enabled);
    Enable(awake_duration, state.awake.duration_enabled);
    Enable(show_details, state.selection.selected_window.has_value());
    for (const int id : {window_list, refresh, close_duration})
    {
        Enable(id, state.close.inputs_enabled);
    }
    if (catalog_revision_ != state.selection.catalog_revision)
    {
        titles_.clear();
        SendDlgItemMessageW(window_.Get(), window_list, LB_RESETCONTENT, 0, 0);
        for (const auto& window : state.selection.windows)
        {
            titles_.push_back(ToWide(window.title));
            const auto result = SendDlgItemMessageW(window_.Get(), window_list, LB_ADDSTRING, 0,
                                                    reinterpret_cast<LPARAM>(titles_.back().c_str()));
            Require(result != LB_ERR && result != LB_ERRSPACE, "Populate window list");
        }
        catalog_revision_ = state.selection.catalog_revision;
        UpdateListExtent();
    }
    const auto selection =
        state.selection.selected_window ? static_cast<LRESULT>(*state.selection.selected_window) : LB_ERR;
    if (SendDlgItemMessageW(window_.Get(), window_list, LB_GETCURSEL, 0, 0) != selection)
    {
        SendDlgItemMessageW(window_.Get(), window_list, LB_SETCURSEL, selection, 0);
    }
    rendering_ = false;
}

void MainWindow::SaveFocus(HWND window)
{
    const auto focus = GetFocus();
    if (IsChild(window, focus))
    {
        last_focus_ = focus;
    }
}

void MainWindow::RestoreFocus(HWND window)
{
    SetFocus(IsChild(window, last_focus_) && IsWindowEnabled(last_focus_) ? last_focus_
                                                                          : GetNextDlgTabItem(window, nullptr, FALSE));
}

void MainWindow::SetVisible(const bool visible)
{
    if (visible)
    {
        ShowWindow(window_.Get(), SW_RESTORE);
        KeepOnWorkArea(window_.Get());
        SetForegroundWindow(window_.Get());
        RestoreFocus(window_.Get());
    }
    else
    {
        SaveFocus(window_.Get());
        ShowWindow(window_.Get(), SW_HIDE);
    }
}

OperationResult MainWindow::SetTimerEnabled(const bool enabled)
{
    if (enabled == timer_enabled_)
    {
        return {};
    }
    if (enabled)
    {
        if (!SetTimer(window_.Get(), countdown_timer, 1000, nullptr))
        {
            return {false, NativeError("SetTimer")};
        }
    }
    else
    {
        KillTimer(window_.Get(), countdown_timer);
    }
    timer_enabled_ = enabled;
    return {};
}

void MainWindow::Emit(const ApplicationEvent event)
{
    if (handler_)
    {
        handler_(event);
    }
}

bool MainWindow::PreTranslate(MSG& message)
{
    if (message.message == WM_KEYDOWN && IsChild(window_.Get(), message.hwnd))
    {
        wchar_t cls[32]{};
        GetClassNameW(message.hwnd, cls, static_cast<int>(std::size(cls)));
        if (message.wParam == VK_RETURN && lstrcmpiW(cls, L"Button") == 0 && IsWindowEnabled(message.hwnd))
        {
            SendMessageW(message.hwnd, BM_CLICK, 0, 0);
            return true;
        }
    }
    return IsDialogMessageW(window_.Get(), &message) != FALSE;
}

void MainWindow::Command(const int id, const int notification)
{
    if (rendering_)
    {
        return;
    }
    const bool selection_changed = (id == window_list && notification == LBN_SELCHANGE) ||
                                   ((id == awake_duration || id == close_duration) && notification == CBN_SELCHANGE);
    if (selection_changed && ScrollState(id).active)
    {
        // Restore drawing before selection handling can enter a modal error dialog or reenter presentation.
        ScrollState(id).selection_pending = true;
        return;
    }
    if (notification == BN_CLICKED)
    {
        switch (id)
        {
        case display:
            Emit({EventKind::toggle_display});
            break;
        case system:
            Emit({EventKind::toggle_system});
            break;
        case close_button:
            Emit({EventKind::toggle_close});
            break;
        case refresh:
            Emit({EventKind::refresh});
            break;
        case highlight:
            Emit({EventKind::toggle_highlight});
            break;
        case show_details:
            Emit({EventKind::show_details});
            break;
        }
    }
    if ((id == awake_duration || id == close_duration) && notification == CBN_SELCHANGE)
    {
        const auto index = SendDlgItemMessageW(window_.Get(), id, CB_GETCURSEL, 0, 0);
        const auto& choices = id == awake_duration ? awake_durations_ : close_durations_;
        const auto duration = index >= 0 && static_cast<std::size_t>(index) < choices.size()
                                  ? std::optional(choices[index].duration)
                                  : std::nullopt;
        Emit({id == awake_duration ? EventKind::awake_duration_changed : EventKind::close_duration_changed,
              std::nullopt, duration});
    }
    if (id == window_list && notification == LBN_SELCHANGE)
    {
        const auto index = SendDlgItemMessageW(window_.Get(), id, LB_GETCURSEL, 0, 0);
        Emit({EventKind::select_window, index == LB_ERR ? std::nullopt : std::optional<std::size_t>(index)});
    }
}

LRESULT CALLBACK MainWindow::ListScrollProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam,
                                            UINT_PTR subclass_id, DWORD_PTR reference) noexcept
{
    auto* self = reinterpret_cast<MainWindow*>(reference);
    if (message == WM_NCDESTROY)
    {
        RemoveWindowSubclass(window, ListScrollProc, subclass_id);
    }
    try
    {
        const auto id = static_cast<int>(subclass_id);
        auto& scroll = self->ScrollState(id);
        if (!CanScrollList(message, wparam) || scroll.active || !IsWindowVisible(window) || !IsWindowEnabled(window))
        {
            return DefSubclassProc(window, message, wparam, lparam);
        }
        HWND list = window;
        HWND combo = nullptr;
        if (id != window_list)
        {
            combo = GetDlgItem(self->window_.Get(), id);
            COMBOBOXINFO info{};
            info.cbSize = sizeof(info);
            Require(GetComboBoxInfo(combo, &info) != FALSE, "Get duration dropdown list");
            list = info.hwndList;
            // A collapsed combo still handles selection normally; never enable drawing on its hidden popup.
            if (!IsWindowVisible(list))
            {
                return DefSubclassProc(window, message, wparam, lparam);
            }
        }
        LRESULT result;
        {
            // Let the native control handle wheel deltas, selection and navigation, but paint only the final
            // position. This bypasses its smooth-scroll effect without changing the user's system preference.
            ListRedrawScope redraw(list, combo, scroll.active);
            result = DefSubclassProc(window, message, wparam, lparam);
        }
        if (std::exchange(scroll.selection_pending, false))
        {
            self->Command(id, id == window_list ? LBN_SELCHANGE : CBN_SELCHANGE);
        }
        return result;
    }
    catch (...)
    {
        self->failure_ = std::current_exception();
        PostQuitMessage(1);
        return 0;
    }
}

LRESULT CALLBACK MainWindow::WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        self = static_cast<MainWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self)
    {
        try
        {
            return self->Message(window, message, wparam, lparam);
        }
        catch (...)
        {
            self->failure_ = std::current_exception();
            PostQuitMessage(1);
            return 0;
        }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT MainWindow::Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (tray_ && message == tray_->TaskbarMessage())
    {
        if (!tray_->Add())
        {
            Emit({EventKind::show});
            MessageBoxW(window,
                        L"The tray icon could not be restored. The window will remain available. "
                        L"Use Quit in the window's system menu to exit.",
                        L"Stay Awake", MB_OK | MB_ICONERROR);
        }
        return 0;
    }
    switch (message)
    {
    case WM_COMMAND:
        Command(LOWORD(wparam), HIWORD(wparam));
        return 0;
    case WM_TIMER:
        if (wparam == countdown_timer && timer_enabled_)
        {
            Emit({EventKind::tick});
        }
        return 0;
    case WM_CLOSE:
        if (tray_ && !tray_->Available())
        {
            Emit({EventKind::show}); // A failed tray must not strand an invisible process.
        }
        else
        {
            Emit({EventKind::hide});
        }
        return 0;
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        Emit({wparam ? EventKind::session_end_confirmed : EventKind::session_end_canceled});
        return 0;
    case WM_SYSCOMMAND:
        if ((wparam & 0xFFF0) == 0x1000)
        {
            Emit({EventKind::quit});
            return 0;
        }
        break;
    case WM_DPICHANGED:
        if (window_.Get() && !controls_.empty())
        {
            const auto* suggested = reinterpret_cast<RECT*>(lparam);
            const auto size = OuterSize(HIWORD(wparam));
            SetWindowPos(window, nullptr, suggested->left, suggested->top, size.cx, size.cy,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            Layout(HIWORD(wparam));
            KeepOnWorkArea(window);
        }
        return 0;
    case WM_GETDPISCALEDSIZE:
        *reinterpret_cast<SIZE*>(lparam) = OuterSize(static_cast<UINT>(wparam));
        return TRUE;
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        KeepOnWorkArea(window);
        break;
    case WM_ACTIVATE:
        if (LOWORD(wparam) == WA_INACTIVE)
        {
            SaveFocus(window);
        }
        // Keep default activation handling so WM_SETFOCUS restores the saved child.
        break;
    case WM_SETFOCUS:
        RestoreFocus(window);
        return 0;
    case tray_message:
        if (tray_)
        {
            const auto event = tray_->Handle(lparam);
            if (event)
            {
                Emit({*event});
            }
        }
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
} // namespace stay_awake::windows
