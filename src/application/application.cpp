#include "stay_awake/application.hpp"

#include <algorithm>
#include <utility>

namespace stay_awake
{
namespace
{
bool ValidDuration(const std::vector<DurationChoice>& choices, const std::optional<std::chrono::seconds> duration)
{
    return duration && std::any_of(choices.begin(), choices.end(),
                                   [duration](const auto& choice) { return choice.duration == *duration; });
}

std::string ModeName(const AwakeMode mode)
{
    return mode == AwakeMode::display ? "Require Display" : "Require System";
}
} // namespace

Application::Application(PlatformBinding& platform) : platform_(platform)
{
}

int Application::Run()
{
    const auto result = platform_.RunService([this](const ApplicationEvent& event) { Handle(event); });
    // RunService delivers quit before destroying native resources, including on a failed loop.
    if (!result.success)
    {
        platform_.ShowError(result.error);
    }
    return result.success ? 0 : 1;
}

const ViewState& Application::State() const
{
    return state_;
}

void Application::Handle(const ApplicationEvent& event)
{
    if (state_.stopped)
    {
        return;
    }
    if (dispatching_ && (event.kind == EventKind::quit || event.kind == EventKind::session_end_confirmed))
    {
        // Confirmed OS shutdown cannot wait for a modal dialog to return. Other reentrant events remain queued.
        Shutdown();
        Publish();
        return;
    }
    events_.push_back(event);
    if (dispatching_)
    {
        return;
    }
    dispatching_ = true;
    try
    {
        while (!events_.empty() && !state_.stopped)
        {
            const auto next = events_.front();
            events_.pop_front();
            Process(next);
            Publish();
            // Complete transitions and render BEFORE a modal dialog can deliver another event. Reentrant events are
            // queued until presentation returns, preventing duplicate expiry and stale UI writes.
            auto error = std::exchange(pending_error_, {});
            if (!error.empty() && !state_.stopped)
            {
                platform_.ShowError(error);
            }
        }
        dispatching_ = false;
    }
    catch (...)
    {
        dispatching_ = false;
        throw;
    }
}

void Application::Process(const ApplicationEvent& event)
{
    switch (event.kind)
    {
    case EventKind::initialized:
        initialized_ = true;
        break;
    case EventKind::show:
        if (!close_)
        {
            Refresh(false);
        }
        state_.visible = true;
        platform_.SetWindowVisible(true);
        break;
    case EventKind::hide:
        ClearHighlight();
        state_.visible = false;
        platform_.SetWindowVisible(false);
        break;
    case EventKind::quit:
    case EventKind::session_end_confirmed:
        Shutdown();
        break;
    case EventKind::session_end_canceled:
        break;
    case EventKind::tick:
        Tick();
        break;
    case EventKind::toggle_display:
        ToggleAwake(AwakeMode::display);
        break;
    case EventKind::toggle_system:
        ToggleAwake(AwakeMode::system);
        break;
    case EventKind::awake_duration_changed:
        if (!awake_)
        {
            state_.awake_duration = event.duration;
        }
        break;
    case EventKind::close_duration_changed:
        if (!close_)
        {
            state_.close_duration = event.duration;
        }
        break;
    case EventKind::select_window:
        if (!close_)
        {
            Select(event.index);
        }
        break;
    case EventKind::refresh:
        if (!close_)
        {
            Refresh(true);
        }
        break;
    case EventKind::toggle_close:
        ToggleClose();
        break;
    case EventKind::toggle_highlight:
        state_.highlight_active = !state_.highlight_active;
        UpdateSelection();
        break;
    }
}

void Application::Refresh(const bool user_initiated)
{
    ClearHighlight();
    state_.windows.clear();
    ++state_.catalog_revision;
    Select(std::nullopt);
    auto result = platform_.EnumerateWindows();
    state_.catalog_status = result.result.success ? "" : "Failed to refresh windows list: " + result.result.error;
    if (result.result.success)
    {
        state_.windows = std::move(result.windows);
    }
    else if (user_initiated)
    {
        pending_error_ = state_.catalog_status;
    }
}

void Application::Select(const std::optional<std::size_t> index)
{
    state_.selected_window = index && *index < state_.windows.size() ? index : std::nullopt;
    UpdateSelection();
}

void Application::UpdateSelection()
{
    state_.process_name.clear();
    state_.window_handle.clear();
    state_.window_position.clear();
    state_.overlay.reset();
    if (state_.selected_window)
    {
        const auto& window = state_.windows[*state_.selected_window];
        state_.process_name = window.process_name.empty() ? "Unknown" : window.process_name;
        state_.window_handle = FormatHandle(window.identity);
        const auto rect = platform_.WindowRectangle(window.identity);
        if (rect)
        {
            state_.window_position = "X: " + std::to_string(rect->x) + ", Y: " + std::to_string(rect->y) +
                                     ", Width: " + std::to_string(rect->width) +
                                     ", Height: " + std::to_string(rect->height);
            if (state_.highlight_active && rect->width > 0 && rect->height > 0)
            {
                state_.overlay = rect;
            }
        }
        else
        {
            state_.window_position = "Error getting position";
        }
    }
    UpdateOverlay();
}

void Application::UpdateOverlay()
{
    const auto result = platform_.SetOverlay(state_.overlay);
    if (!result.success)
    {
        state_.overlay.reset();
        state_.highlight_active = false;
        pending_error_ = "Failed to highlight window: " + result.error;
    }
}

void Application::ClearHighlight()
{
    state_.highlight_active = false;
    state_.overlay.reset();
    UpdateOverlay();
}

void Application::ToggleAwake(const AwakeMode mode)
{
    if (awake_)
    {
        if (awake_->mode == mode)
        {
            StopAwake(false);
        }
        return;
    }
    if (!ValidDuration(state_.awake_durations, state_.awake_duration))
    {
        pending_error_ = "Select a valid stay awake duration.";
        return;
    }
    const auto result = platform_.SetAwake(mode);
    if (!result.success)
    {
        state_.awake_caption = "Stay Awake - Start failed";
        state_.awake_status = ModeName(mode) + ": " + result.error;
        pending_error_ = "Failed to start stay awake: " + result.error;
        return;
    }
    awake_ = AwakeSession{mode, platform_.Now() + *state_.awake_duration};
    state_.awake_caption = "Stay Awake";
    state_.awake_status.clear();
}

void Application::StopAwake(const bool expired)
{
    const auto mode = awake_->mode;
    const bool retry = !awake_->deadline;
    awake_->deadline.reset(); // A failed release never retries on every tick.
    const auto result = platform_.SetAwake(std::nullopt);
    if (result.success)
    {
        awake_.reset();
        if (expired || retry)
        {
            state_.awake_caption = "Stay Awake - Ended";
            state_.awake_status = ModeName(mode) + " Ended At " + platform_.LocalTimestamp(true);
        }
    }
    else
    {
        state_.awake_caption = "Stay Awake - Release failed";
        state_.awake_status = "Error Ending " + ModeName(mode) + " At " + platform_.LocalTimestamp(true) + ": " +
                              result.error + ". Click the active mode to retry.";
        if (!expired)
        {
            pending_error_ = "Failed to stop stay awake: " + result.error;
        }
    }
}

void Application::ToggleClose()
{
    if (close_)
    {
        close_.reset();
        return;
    }
    if (!state_.selected_window)
    {
        pending_error_ = "No window selected.";
        return;
    }
    if (!ValidDuration(state_.close_durations, state_.close_duration))
    {
        pending_error_ = "Select a valid close duration.";
        return;
    }
    close_ = CloseSession{state_.windows[*state_.selected_window], platform_.Now() + *state_.close_duration};
    state_.close_group_caption = "Window Closer";
    state_.close_status.clear();
}

void Application::Tick()
{
    const auto now = platform_.Now();
    if (awake_ && awake_->deadline && now >= *awake_->deadline)
    {
        StopAwake(true);
    }
    if (close_ && now >= close_->deadline)
    {
        const auto target = close_->target;
        close_.reset(); // Consume before calling native code or delivering any reentrant events.
        const auto result = platform_.RequestClose(target.identity);
        const std::string outcome = result.success ? "Close requested" : "Close request failed";
        state_.close_group_caption = "Window Closer - " + outcome;
        state_.close_status = outcome + " " + FormatHandle(target.identity) + " At " + platform_.LocalTimestamp(false) +
                              " (" + (target.process_name.empty() ? "Unknown" : target.process_name) + ")";
        if (!result.success)
        {
            state_.close_status += ": " + result.error;
        }
        Refresh(false);
    }
}

void Application::Publish()
{
    bool needs_timer = (awake_ && awake_->deadline) || close_;
    const auto timer = platform_.SetTimerEnabled(needs_timer);
    if (!timer.success && needs_timer)
    {
        // Never leave a scheduled operation without expiry callbacks. Retain a failed power release for manual retry.
        if (awake_ && awake_->deadline)
        {
            StopAwake(false);
            if (!awake_)
            {
                state_.awake_caption = "Stay Awake - Timer failed";
                state_.awake_status = "Canceled: " + timer.error;
            }
        }
        if (close_)
        {
            state_.close_group_caption = "Window Closer - Timer failed";
            state_.close_status = "Schedule canceled: " + timer.error;
            close_.reset();
        }
        needs_timer = false;
        pending_error_ = "Unable to start countdown timer: " + timer.error;
    }
    state_.timer_needed = needs_timer;
    const auto now = platform_.Now();
    state_.awake_remaining =
        awake_ ? (awake_->deadline ? FormatRemaining(*awake_->deadline - now) : "Release failed") : "Not Enabled";
    state_.close_remaining = close_ ? FormatRemaining(close_->deadline - now) : "Not Enabled";
    state_.awake_duration_enabled = !awake_;
    state_.display_enabled = !awake_ || awake_->mode == AwakeMode::display;
    state_.system_enabled = !awake_ || awake_->mode == AwakeMode::system;
    state_.display_caption = awake_ && awake_->mode == AwakeMode::display ? "Stop Require Display" : "Require Display";
    state_.system_caption = awake_ && awake_->mode == AwakeMode::system ? "Stop Require System" : "Require System";
    state_.close_inputs_enabled = !close_;
    state_.close_caption = close_ ? "Stop" : "Schedule Close Window";
    state_.highlight_caption = state_.highlight_active ? "Stop Highlighting" : "Highlight Window";
    if (initialized_)
    {
        platform_.Present(state_);
    }
}

void Application::Shutdown()
{
    state_.stopped = true;
    events_.clear();
    close_.reset();
    if (awake_)
    {
        // The native power owner also retries on teardown if this best-effort release fails.
        (void)platform_.SetAwake(std::nullopt);
        awake_.reset();
    }
    ClearHighlight();
    state_.visible = false;
    (void)platform_.SetTimerEnabled(false);
    platform_.RequestExit();
}
} // namespace stay_awake
