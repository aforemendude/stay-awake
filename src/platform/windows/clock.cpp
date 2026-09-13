#include "platform/windows/clock.hpp"

#include <windows.h>

namespace stay_awake::windows
{
ElapsedTime InterruptTime()
{
    ULONGLONG ticks = 0;
    QueryInterruptTime(&ticks);
    return ElapsedTime(ticks / 10000); // Biased interrupt time includes the sleep/hibernate bias on resume.
}
} // namespace stay_awake::windows
