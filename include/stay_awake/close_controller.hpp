#pragma once

#include "stay_awake/platform_binding.hpp"

namespace stay_awake
{
class CloseController
{
  public:
    explicit CloseController(PlatformBinding& platform);
    void SetDuration(std::optional<std::chrono::seconds> duration);
    OperationResult Toggle(const std::optional<WindowInfo>& target);
    // Returns true when a schedule is consumed, so Application can refresh the window catalog.
    bool Tick(ElapsedTime now);
    bool Active() const;
    void Cancel();
    void CancelForTimerFailure(const std::string& error);
    CloseViewState State(ElapsedTime now) const;

  private:
    struct Session
    {
        WindowInfo target;
        ElapsedTime deadline;
    };

    PlatformBinding& platform_;
    CloseViewState state_;
    std::optional<Session> session_;
};
} // namespace stay_awake
