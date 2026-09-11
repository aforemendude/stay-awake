#pragma once

#include "platform/windows/native.hpp"

namespace stay_awake::windows
{
class OverlayWindow
{
  public:
    OperationResult Set(std::optional<Rectangle> rectangle);

  private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept;
    UniqueWindow window_;
};
} // namespace stay_awake::windows
