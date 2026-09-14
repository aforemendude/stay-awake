#include "platform/windows/main_window_controls.hpp"

#include "platform/windows/dpi.hpp"

#include <algorithm>
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
} // namespace

MainWindowControls::MainWindowControls(EventHandler emit, std::exception_ptr& failure)
    : emit_(std::move(emit)), window_list_scroll_(failure), awake_duration_scroll_(failure),
      close_duration_scroll_(failure)
{
}

bool MainWindowControls::HasControls() const noexcept
{
    return !controls_.empty();
}

HWND MainWindowControls::Add(const int id, const wchar_t* cls, const wchar_t* text, const DWORD style, const int x,
                             const int y, const int width, const int height, const DWORD ex_style)
{
    const auto child =
        CreateWindowExW(ex_style, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, window_,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    Require(child != nullptr, "Create native control");
    controls_.push_back({child, x, y, width, height});
    return child;
}

void MainWindowControls::Create(HWND window)
{
    window_ = window;
    constexpr DWORD button = BS_PUSHBUTTON | WS_TABSTOP;
    constexpr DWORD static_text = SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS;
    constexpr DWORD status_text = SS_LEFTNOWORDWRAP | SS_NOPREFIX | SS_NOTIFY;
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
    Add(awake_status, L"STATIC", L"", status_text, 84, 111, 682, 24);

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
    Add(close_status, L"STATIC", L"", status_text, 84, 286, 682, 24);
    Add(window_list, L"LISTBOX", L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP, 18, 323,
        748, 232, WS_EX_CLIENTEDGE);
    Add(-1, L"STATIC", L"Status:", 0, 20, 565, 62, 24);
    Add(catalog_status, L"STATIC", L"", status_text, 84, 565, 682, 24);
    for (const int id : {window_list, awake_duration, close_duration})
    {
        Scrolling(id).Attach(GetDlgItem(window_, id), id != window_list,
                             [this, id] { Command(id, id == window_list ? LBN_SELCHANGE : CBN_SELCHANGE); });
    }
}

ListScrolling& MainWindowControls::Scrolling(const int id) noexcept
{
    return id == awake_duration ? awake_duration_scroll_
                                : (id == close_duration ? close_duration_scroll_ : window_list_scroll_);
}

void MainWindowControls::Layout(const UINT dpi)
{
    dpi_ = dpi;
    UniqueFont next_font(CreateFontW(-MulDiv(12, static_cast<int>(dpi), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH, L"Segoe UI"));
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
    SendDlgItemMessageW(window_, window_list, LB_SETITEMHEIGHT, 0, Scale(24, dpi));
    for (const auto id : {awake_duration, close_duration})
    {
        SendDlgItemMessageW(window_, id, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), Scale(23, dpi));
        SendDlgItemMessageW(window_, id, CB_SETITEMHEIGHT, 0, Scale(24, dpi));
    }
}

void MainWindowControls::UpdateListExtent()
{
    const auto list = GetDlgItem(window_, window_list);
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

void MainWindowControls::Text(const int id, const std::string_view text)
{
    const auto value = ToWide(text);
    const auto control = GetDlgItem(window_, id);
    const int length = GetWindowTextLengthW(control);
    std::wstring previous(static_cast<std::size_t>(length) + 1, L'\0');
    previous.resize(GetWindowTextW(control, previous.data(), static_cast<int>(previous.size())));
    if (previous != value)
    {
        SetWindowTextW(control, value.c_str()); // Avoid repainting unchanged text on unrelated ticks.
    }
}

void MainWindowControls::Enable(const int id, const bool enabled)
{
    EnableWindow(GetDlgItem(window_, id), enabled);
}

void MainWindowControls::SelectDuration(const int id, const std::vector<DurationChoice>& choices,
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
    if (SendDlgItemMessageW(window_, id, CB_GETCURSEL, 0, 0) != index)
    {
        SendDlgItemMessageW(window_, id, CB_SETCURSEL, index, 0);
    }
}

void MainWindowControls::Present(const ViewState& state)
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
                    SendDlgItemMessageW(window_, id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
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
        SendDlgItemMessageW(window_, window_list, LB_RESETCONTENT, 0, 0);
        for (const auto& window : state.selection.windows)
        {
            titles_.push_back(ToWide(window.title));
            const auto result = SendDlgItemMessageW(window_, window_list, LB_ADDSTRING, 0,
                                                    reinterpret_cast<LPARAM>(titles_.back().c_str()));
            Require(result != LB_ERR && result != LB_ERRSPACE, "Populate window list");
        }
        catalog_revision_ = state.selection.catalog_revision;
        UpdateListExtent();
    }
    const auto selection =
        state.selection.selected_window ? static_cast<LRESULT>(*state.selection.selected_window) : LB_ERR;
    if (SendDlgItemMessageW(window_, window_list, LB_GETCURSEL, 0, 0) != selection)
    {
        SendDlgItemMessageW(window_, window_list, LB_SETCURSEL, selection, 0);
    }
    rendering_ = false;
}

void MainWindowControls::Command(const int id, const int notification)
{
    if (rendering_)
    {
        return;
    }
    if (notification == STN_CLICKED)
    {
        switch (id)
        {
        case awake_status:
            emit_({EventKind::show_awake_status});
            return;
        case close_status:
            emit_({EventKind::show_close_status});
            return;
        case catalog_status:
            emit_({EventKind::show_catalog_status});
            return;
        }
    }
    const bool selection_changed = (id == window_list && notification == LBN_SELCHANGE) ||
                                   ((id == awake_duration || id == close_duration) && notification == CBN_SELCHANGE);
    if (selection_changed && Scrolling(id).DeferSelection())
    {
        // Restore drawing before selection handling can enter a modal error dialog or reenter presentation.
        return;
    }
    if (notification == BN_CLICKED)
    {
        switch (id)
        {
        case display:
            emit_({EventKind::toggle_display});
            break;
        case system:
            emit_({EventKind::toggle_system});
            break;
        case close_button:
            emit_({EventKind::toggle_close});
            break;
        case refresh:
            emit_({EventKind::refresh});
            break;
        case highlight:
            emit_({EventKind::toggle_highlight});
            break;
        case show_details:
            emit_({EventKind::show_details});
            break;
        }
    }
    if ((id == awake_duration || id == close_duration) && notification == CBN_SELCHANGE)
    {
        const auto index = SendDlgItemMessageW(window_, id, CB_GETCURSEL, 0, 0);
        const auto& choices = id == awake_duration ? awake_durations_ : close_durations_;
        const auto duration = index >= 0 && static_cast<std::size_t>(index) < choices.size()
                                  ? std::optional(choices[index].duration)
                                  : std::nullopt;
        emit_({id == awake_duration ? EventKind::awake_duration_changed : EventKind::close_duration_changed,
               std::nullopt, duration});
    }
    if (id == window_list && notification == LBN_SELCHANGE)
    {
        const auto index = SendDlgItemMessageW(window_, id, LB_GETCURSEL, 0, 0);
        emit_({EventKind::select_window, index == LB_ERR ? std::nullopt : std::optional<std::size_t>(index)});
    }
}

} // namespace stay_awake::windows
