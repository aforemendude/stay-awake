#pragma once

#include <windows.h>

#include <exception>
#include <functional>

namespace stay_awake::windows
{
// Keeps a native list and, for dropdowns, its combo box in one redraw/selection scope.
class ListScrolling
{
  public:
    explicit ListScrolling(std::exception_ptr& failure) noexcept;
    ~ListScrolling() noexcept;
    ListScrolling(const ListScrolling&) = delete;
    ListScrolling& operator=(const ListScrolling&) = delete;
    ListScrolling(ListScrolling&&) = delete;
    ListScrolling& operator=(ListScrolling&&) = delete;

    void Attach(HWND control, bool combo, std::function<void()> on_selection);
    void Detach() noexcept;
    bool DeferSelection() noexcept;

  private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR subclass_id,
                                       DWORD_PTR reference) noexcept;

    std::exception_ptr& failure_;
    std::function<void()> on_selection_;
    HWND control_ = nullptr; // Borrowed; cleared by WM_NCDESTROY or Detach.
    HWND list_ = nullptr;    // The control itself or its borrowed combo popup.
    bool combo_ = false;
    bool active_ = false;
    bool selection_pending_ = false;
};
} // namespace stay_awake::windows
