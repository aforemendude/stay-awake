#include "stay_awake/application_state.hpp"

#include <algorithm>
#include <charconv>
#include <iterator>

namespace stay_awake
{
bool ValidDuration(const std::vector<DurationChoice>& choices, const std::optional<std::chrono::seconds> duration)
{
    return duration && std::any_of(choices.begin(), choices.end(),
                                   [duration](const auto& choice) { return choice.duration == *duration; });
}

std::string FormatRemaining(const ElapsedTime remaining)
{
    const auto seconds = std::chrono::ceil<std::chrono::seconds>(std::max(remaining, ElapsedTime::zero())).count();
    auto text = std::to_string(seconds / 3600);
    if (text.size() < 2)
    {
        text.insert(text.begin(), '0');
    }
    for (const auto component : {seconds / 60 % 60, seconds % 60})
    {
        text += ':';
        text += static_cast<char>('0' + component / 10);
        text += static_cast<char>('0' + component % 10);
    }
    return text;
}

ElapsedTime NextCountdownUpdate(const ElapsedTime deadline, const ElapsedTime now)
{
    if (deadline <= now)
    {
        return now;
    }
    const auto seconds = std::chrono::ceil<std::chrono::seconds>(deadline - now);
    return deadline - (seconds - std::chrono::seconds(1));
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
    // Two hexadecimal digits per byte also fit the largest opaque handle.
    char buffer[sizeof(identity.handle) * 2];
    const auto result = std::to_chars(std::begin(buffer), std::end(buffer), identity.handle, 16);
    std::string text(buffer, result.ptr);
    for (auto& digit : text)
    {
        if (digit >= 'a' && digit <= 'f')
        {
            digit = static_cast<char>(digit - 'a' + 'A');
        }
    }
    return text;
}
} // namespace stay_awake
