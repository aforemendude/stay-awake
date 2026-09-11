#include "stay_awake/close_controller.hpp"

namespace stay_awake
{
CloseController::CloseController(PlatformBinding& platform) : platform_(platform)
{
}

void CloseController::SetDuration(const std::optional<std::chrono::seconds> duration)
{
    if (!session_)
    {
        state_.duration = duration;
    }
}

OperationResult CloseController::Toggle(const std::optional<WindowInfo>& target)
{
    if (session_)
    {
        Cancel();
        return {};
    }
    if (!target)
    {
        return {false, "No window selected."};
    }
    if (!ValidDuration(state_.durations, state_.duration))
    {
        return {false, "Select a valid close duration."};
    }
    session_ = Session{*target, platform_.Now() + *state_.duration};
    state_.group_caption = "Window Closer";
    state_.status.clear();
    return {};
}

bool CloseController::Tick(const ElapsedTime now)
{
    if (!session_ || now < session_->deadline)
    {
        return false;
    }
    const auto target = session_->target;
    Cancel(); // Consume before calling native code or delivering any reentrant events.
    const auto result = platform_.RequestClose(target.identity);
    const std::string outcome = result.success ? "Close requested" : "Close request failed";
    state_.group_caption = "Window Closer - " + outcome;
    state_.status = outcome + " " + FormatHandle(target.identity) + " At " + platform_.LocalTimestamp(false) + " (" +
                    (target.process_name.empty() ? "Unknown" : target.process_name) + ")";
    if (!result.success)
    {
        state_.status += ": " + result.error;
    }
    return true;
}

bool CloseController::Active() const
{
    return session_.has_value();
}

void CloseController::Cancel()
{
    session_.reset();
}

void CloseController::CancelForTimerFailure(const std::string& error)
{
    if (session_)
    {
        state_.group_caption = "Window Closer - Timer failed";
        state_.status = "Schedule canceled: " + error;
        Cancel();
    }
}

CloseViewState CloseController::State(const ElapsedTime now) const
{
    auto state = state_;
    state.remaining = session_ ? FormatRemaining(session_->deadline - now) : "Not Enabled";
    state.inputs_enabled = !session_;
    state.caption = session_ ? "Stop" : "Schedule Close Window";
    return state;
}
} // namespace stay_awake
