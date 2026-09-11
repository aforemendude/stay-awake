#include "stay_awake/awake_controller.hpp"

namespace stay_awake
{
namespace
{
std::string ModeName(const AwakeMode mode)
{
    return mode == AwakeMode::display ? "Require Display" : "Require System";
}
} // namespace

AwakeController::AwakeController(PlatformBinding& platform) : platform_(platform)
{
}

void AwakeController::SetDuration(const std::optional<std::chrono::seconds> duration)
{
    if (!session_)
    {
        state_.duration = duration;
    }
}

OperationResult AwakeController::Toggle(const AwakeMode mode)
{
    if (session_)
    {
        return session_->mode == mode ? Stop(false) : OperationResult{};
    }
    if (!ValidDuration(state_.durations, state_.duration))
    {
        return {false, "Select a valid stay awake duration."};
    }
    const auto result = platform_.SetAwake(mode);
    if (!result.success)
    {
        state_.caption = "Stay Awake - Start failed";
        state_.status = ModeName(mode) + ": " + result.error;
        return {false, "Failed to start stay awake: " + result.error};
    }
    session_ = Session{mode, platform_.Now() + *state_.duration};
    state_.caption = "Stay Awake";
    state_.status.clear();
    return {};
}

OperationResult AwakeController::Stop(const bool expired)
{
    const auto mode = session_->mode;
    const bool retry = !session_->deadline;
    session_->deadline.reset(); // A failed release never retries on every tick.
    const auto result = platform_.SetAwake(std::nullopt);
    if (result.success)
    {
        session_.reset();
        if (expired || retry)
        {
            state_.caption = "Stay Awake - Ended";
            state_.status = ModeName(mode) + " Ended At " + platform_.LocalTimestamp(true);
        }
    }
    else
    {
        state_.caption = "Stay Awake - Release failed";
        state_.status = "Error Ending " + ModeName(mode) + " At " + platform_.LocalTimestamp(true) + ": " +
                        result.error + ". Click the active mode to retry.";
        if (!expired)
        {
            return {false, "Failed to stop stay awake: " + result.error};
        }
    }
    return {};
}

void AwakeController::Tick(const ElapsedTime now)
{
    if (NeedsTimer() && now >= *session_->deadline)
    {
        (void)Stop(true);
    }
}

bool AwakeController::NeedsTimer() const
{
    return session_ && session_->deadline;
}

void AwakeController::CancelForTimerFailure(const std::string& error)
{
    if (NeedsTimer())
    {
        (void)Stop(false);
        if (!session_)
        {
            state_.caption = "Stay Awake - Timer failed";
            state_.status = "Canceled: " + error;
        }
    }
}

void AwakeController::Shutdown()
{
    if (session_)
    {
        // The native power owner also retries on teardown if this best-effort release fails.
        (void)platform_.SetAwake(std::nullopt);
        session_.reset();
    }
}

AwakeViewState AwakeController::State(const ElapsedTime now) const
{
    auto state = state_;
    state.remaining =
        session_ ? (session_->deadline ? FormatRemaining(*session_->deadline - now) : "Release failed") : "Not Enabled";
    state.duration_enabled = !session_;
    state.display_enabled = !session_ || session_->mode == AwakeMode::display;
    state.system_enabled = !session_ || session_->mode == AwakeMode::system;
    state.display_caption =
        session_ && session_->mode == AwakeMode::display ? "Stop Require Display" : "Require Display";
    state.system_caption = session_ && session_->mode == AwakeMode::system ? "Stop Require System" : "Require System";
    return state;
}
} // namespace stay_awake
