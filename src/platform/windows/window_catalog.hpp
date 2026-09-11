#pragma once

#include "stay_awake/application_state.hpp"

namespace stay_awake::windows
{
WindowListResult EnumerateWindows();
std::optional<Rectangle> WindowRectangle(WindowIdentity identity);
OperationResult RequestClose(WindowIdentity identity);
} // namespace stay_awake::windows
