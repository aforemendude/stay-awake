#include "platform/windows/power_manager.hpp"

#include "platform/windows/native.hpp"

namespace stay_awake::windows
{
PowerManager::~PowerManager()
{
    Reset();
}

void PowerManager::Reset() noexcept
{
    if (active_ && SetThreadExecutionState(ES_CONTINUOUS))
    {
        active_ = false;
    }
}

OperationResult PowerManager::Set(const std::optional<AwakeMode> mode)
{
    EXECUTION_STATE flags = ES_CONTINUOUS;
    if (mode)
    {
        flags |= ES_SYSTEM_REQUIRED;
        if (*mode == AwakeMode::display)
        {
            flags |= ES_DISPLAY_REQUIRED;
        }
    }
    if (!SetThreadExecutionState(flags))
    {
        // This API does not document an extended GetLastError result.
        return {false, "SetThreadExecutionState rejected the power request"};
    }
    active_ = mode.has_value();
    return {};
}
} // namespace stay_awake::windows
