#pragma once

#include "stay_awake/application_state.hpp"

#include <functional>
#include <string_view>

namespace stay_awake
{
using EventHandler = std::function<void(const ApplicationEvent&)>;

class PlatformBinding
{
  public:
    virtual ~PlatformBinding() = default;

    // RunService owns setup and event delivery; the binding retains instance ownership through final teardown.
    // Deliver initialized after creating the hidden window/tray, before consuming pending Show requests, and quit
    // before tearing services down. A successful secondary launch sends Show and returns without initializing UI.
    // All calls, including power changes and cleanup, run on this same UI thread. Operations return expected errors;
    // no exception may escape a native callback. Modal presentation may reenter the event handler.
    virtual OperationResult RunService(EventHandler handler) = 0;
    // Monotonic time since an arbitrary origin, INCLUDING suspend/hibernate; unaffected by wall-clock adjustments.
    virtual ElapsedTime Now() = 0;
    // Local time formatted MM/dd HH:mm:ss (true) or MM/dd HH:mm (false), independent of Now().
    virtual std::string LocalTimestamp(bool with_seconds) = 0;
    virtual OperationResult SetAwake(std::optional<AwakeMode> mode) = 0;
    // The adapter filters visible/nonblank/non-shell/non-Program-Manager/non-own-PID windows and sorts by locale.
    virtual WindowListResult EnumerateWindows() = 0;
    virtual std::optional<Rectangle> WindowRectangle(WindowIdentity target) = 0;
    // Revalidate existence and owning PID immediately before posting ONE asynchronous close request.
    virtual OperationResult RequestClose(WindowIdentity target) = 0;
    virtual OperationResult SetOverlay(std::optional<Rectangle> rectangle) = 0;
    virtual OperationResult SetTimerEnabled(bool enabled) = 0;
    virtual void Present(const ViewState& state) = 0;
    virtual void SetWindowVisible(bool visible) = 0;
    virtual void ShowError(std::string_view message) = 0;
    virtual void RequestExit() = 0;
};
} // namespace stay_awake
