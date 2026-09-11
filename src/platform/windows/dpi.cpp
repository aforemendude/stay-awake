#include "platform/windows/dpi.hpp"

#include <algorithm>

namespace stay_awake::windows
{
int Scale(const int value, const UINT dpi)
{
    return MulDiv(value, static_cast<int>(dpi), 96);
}

SIZE OuterSize(const UINT dpi)
{
    RECT rect{0, 0, Scale(client_width, dpi), Scale(client_height, dpi)};
    Require(AdjustWindowRectExForDpi(&rect, main_style, FALSE, main_ex_style, dpi) != FALSE, "Size window for DPI");
    return {rect.right - rect.left, rect.bottom - rect.top};
}

void KeepOnWorkArea(HWND window)
{
    if (IsIconic(window))
    {
        return;
    }
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    RECT rect{};
    if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor) && GetWindowRect(window, &rect))
    {
        // Fixed logical size: retain readable scale even on an undersized display, but keep the caption reachable.
        const LONG x = std::clamp(rect.left, monitor.rcWork.left,
                                  std::max(monitor.rcWork.left, monitor.rcWork.right - (rect.right - rect.left)));
        const LONG y = std::clamp(rect.top, monitor.rcWork.top,
                                  std::max(monitor.rcWork.top, monitor.rcWork.bottom - (rect.bottom - rect.top)));
        SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
} // namespace stay_awake::windows
