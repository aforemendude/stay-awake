#include "stay_awake/application_state.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace stay_awake
{
std::string FormatRemaining(const ElapsedTime remaining)
{
    const auto seconds = std::chrono::ceil<std::chrono::seconds>(std::max(remaining, ElapsedTime::zero())).count();
    std::ostringstream text;
    text << std::setfill('0') << std::setw(2) << seconds / 3600 << ':' << std::setw(2) << seconds / 60 % 60 << ':'
         << std::setw(2) << seconds % 60;
    return text.str();
}

std::vector<DurationChoice> MakeDurations(const std::chrono::minutes first)
{
    std::vector<DurationChoice> choices;
    for (auto duration = first; duration <= std::chrono::hours(8); duration += std::chrono::minutes(15))
    {
        choices.push_back({duration, FormatRemaining(duration)});
    }
    choices.push_back({std::chrono::seconds(10), "00:00:10"});
    return choices;
}

std::string FormatHandle(const WindowIdentity identity)
{
    std::ostringstream text;
    text << std::uppercase << std::hex << identity.handle;
    return text.str();
}
} // namespace stay_awake
