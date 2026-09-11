#include "stay_awake/application.hpp"

#include <utility>

namespace stay_awake
{
Application::Application(PlatformBinding& platform)
    : platform_(platform), awake_(platform), close_(platform), selection_(platform)
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
        if (!close_.Active())
        {
            ReportError(selection_.Refresh(false));
        }
        state_.visible = true;
        platform_.SetWindowVisible(true);
        break;
    case EventKind::hide:
        ReportError(selection_.ClearHighlight());
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
        ReportError(awake_.Toggle(AwakeMode::display));
        break;
    case EventKind::toggle_system:
        ReportError(awake_.Toggle(AwakeMode::system));
        break;
    case EventKind::awake_duration_changed:
        awake_.SetDuration(event.duration);
        break;
    case EventKind::close_duration_changed:
        close_.SetDuration(event.duration);
        break;
    case EventKind::select_window:
        if (!close_.Active())
        {
            ReportError(selection_.Select(event.index));
        }
        break;
    case EventKind::refresh:
        if (!close_.Active())
        {
            ReportError(selection_.Refresh(true));
        }
        break;
    case EventKind::toggle_close:
        ReportError(close_.Toggle(selection_.SelectedWindow()));
        break;
    case EventKind::toggle_highlight:
        ReportError(selection_.ToggleHighlight());
        break;
    }
}

void Application::Tick()
{
    const auto now = platform_.Now();
    awake_.Tick(now);
    if (close_.Tick(now))
    {
        ReportError(selection_.Refresh(false));
    }
}

void Application::Publish()
{
    bool needs_timer = awake_.NeedsTimer() || close_.Active();
    const auto timer = platform_.SetTimerEnabled(needs_timer);
    if (!timer.success && needs_timer)
    {
        // Never leave a scheduled operation without expiry callbacks. Retain a failed power release for manual retry.
        awake_.CancelForTimerFailure(timer.error);
        close_.CancelForTimerFailure(timer.error);
        needs_timer = false;
        pending_error_ = "Unable to start countdown timer: " + timer.error;
    }
    state_.timer_needed = needs_timer;
    const auto now = platform_.Now();
    state_.awake = awake_.State(now);
    state_.close = close_.State(now);
    state_.selection = selection_.State();
    if (initialized_)
    {
        platform_.Present(state_);
    }
}

void Application::Shutdown()
{
    state_.stopped = true;
    events_.clear();
    close_.Cancel();
    awake_.Shutdown();
    (void)selection_.ClearHighlight();
    state_.visible = false;
    (void)platform_.SetTimerEnabled(false);
    platform_.RequestExit();
}

void Application::ReportError(const OperationResult& result)
{
    if (!result.success)
    {
        pending_error_ = result.error;
    }
}
} // namespace stay_awake
