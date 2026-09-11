#include "platform/windows/overlay_window.hpp"

namespace stay_awake::windows
{
LRESULT CALLBACK OverlayWindow::WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
    if (message == WM_NCHITTEST)
    {
        return HTTRANSPARENT;
    }
    if (message == WM_MOUSEACTIVATE)
    {
        return MA_NOACTIVATE;
    }
    if (message == WM_ERASEBKGND)
    {
        RECT rectangle{};
        GetClientRect(window, &rectangle);
        const auto dc = reinterpret_cast<HDC>(wparam);
        const auto previous = SetDCBrushColor(dc, RGB(255, 0, 0));
        FillRect(dc, &rectangle, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        SetDCBrushColor(dc, previous);
        return 1;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

OperationResult OverlayWindow::Set(const std::optional<Rectangle> rectangle)
{
    if (!rectangle || rectangle->width <= 0 || rectangle->height <= 0)
    {
        if (window_.Get())
        {
            ShowWindow(window_.Get(), SW_HIDE);
        }
        return {};
    }
    if (!window_.Get())
    {
        WNDCLASSEXW cls{};
        cls.cbSize = sizeof(cls);
        cls.lpfnWndProc = WindowProc;
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.lpszClassName = L"StayAwake.Highlight";
        if (!RegisterClassExW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            return {false, NativeError("Register highlight window")};
        }
        // Layered + transparent bypasses hit testing even across process/thread boundaries; HTTRANSPARENT alone only
        // forwards within the same thread. NOACTIVATE prevents focus theft, TOOLWINDOW excludes taskbar/Alt-Tab.
        window_.Reset(
            CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                            cls.lpszClassName, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, cls.hInstance, nullptr));
        if (!window_.Get())
        {
            return {false, NativeError("Create highlight window")};
        }
        if (!SetLayeredWindowAttributes(window_.Get(), 0, 64, LWA_ALPHA))
        {
            const auto error = NativeError("Set highlight opacity");
            window_.Reset();
            return {false, error};
        }
    }
    if (!SetWindowPos(window_.Get(), HWND_TOPMOST, rectangle->x, rectangle->y, rectangle->width, rectangle->height,
                      SWP_NOACTIVATE | SWP_SHOWWINDOW))
    {
        const auto error = NativeError("Position highlight window");
        ShowWindow(window_.Get(), SW_HIDE);
        return {false, error};
    }
    InvalidateRect(window_.Get(), nullptr, TRUE);
    return {};
}
} // namespace stay_awake::windows
