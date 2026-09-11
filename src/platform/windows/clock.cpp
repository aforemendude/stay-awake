#include "platform/windows/clock.hpp"

#include <windows.h>

#include <cstdio>

namespace stay_awake::windows
{
ElapsedTime InterruptTime()
{
    ULONGLONG ticks = 0;
    QueryInterruptTime(&ticks);
    return ElapsedTime(ticks / 10000); // Biased interrupt time includes the sleep/hibernate bias on resume.
}

std::string LocalTimestamp(const bool with_seconds)
{
    SYSTEMTIME time{};
    GetLocalTime(&time);
    char text[32]{};
    if (with_seconds)
    {
        std::snprintf(text, sizeof(text), "%02u/%02u %02u:%02u:%02u", time.wMonth, time.wDay, time.wHour, time.wMinute,
                      time.wSecond);
    }
    else
    {
        std::snprintf(text, sizeof(text), "%02u/%02u %02u:%02u", time.wMonth, time.wDay, time.wHour, time.wMinute);
    }
    return text;
}
} // namespace stay_awake::windows
