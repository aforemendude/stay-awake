#pragma once

#include "platform/windows/native.hpp"

namespace stay_awake::windows
{
constexpr int client_width = 784;
constexpr int client_height = 606;
constexpr DWORD main_style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
constexpr DWORD main_ex_style = WS_EX_CONTROLPARENT;

int Scale(int value, UINT dpi);
SIZE OuterSize(UINT dpi);
void KeepOnWorkArea(HWND window);
} // namespace stay_awake::windows
