#pragma once

#include "stay_awake/application_state.hpp"

namespace stay_awake::windows
{
ElapsedTime InterruptTime();
std::string LocalTimestamp(bool with_seconds);
} // namespace stay_awake::windows
