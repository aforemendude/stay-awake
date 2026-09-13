#include "platform/windows/main_window.hpp"

#include "platform/windows/dpi.hpp"
#include "platform/windows/resource.h"

#include <commctrl.h>
#include <utility>

namespace stay_awake::windows
{
namespace
{
constexpr UINT_PTR countdown_timer = 1;
} // namespace

MainWindow::MainWindow(EventHandler handler)
    : handler_(std::move(handler)), controls_([this](ApplicationEvent event) { Emit(event); }, failure_)
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
    controls_.Create(window_.Get());
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

void MainWindow::Present(const ViewState& state)
{
    controls_.Present(state);
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
        controls_.Layout(dpi);
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
        controls_.UpdateListExtent();
        layout_active_ = false;
    }
    catch (...)
    {
        layout_active_ = false;
        throw;
    }
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
        controls_.Command(LOWORD(wparam), HIWORD(wparam));
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
        if (window_.Get() && controls_.HasControls())
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
