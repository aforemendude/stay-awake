#pragma once

#include "stay_awake/platform_binding.hpp"

namespace stay_awake
{
class AwakeController
{
  public:
    explicit AwakeController(PlatformBinding& platform);
    void SetDuration(std::optional<std::chrono::seconds> duration);
    OperationResult Toggle(AwakeMode mode);
    void Tick(ElapsedTime now);
    bool NeedsTimer() const;
    void CancelForTimerFailure(const std::string& error);
    void Shutdown();
    AwakeViewState State(ElapsedTime now) const;

  private:
    struct Session
    {
        AwakeMode mode;
        // No deadline means release failed and only a manual retry is allowed.
        std::optional<ElapsedTime> deadline;
    };

    OperationResult Stop(bool expired);

    PlatformBinding& platform_;
    AwakeViewState state_;
    std::optional<Session> session_;
};
} // namespace stay_awake
