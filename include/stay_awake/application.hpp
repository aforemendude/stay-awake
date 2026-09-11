#pragma once

#include "stay_awake/platform_binding.hpp"

#include <deque>

namespace stay_awake
{
class Application
{
  public:
    explicit Application(PlatformBinding& platform);
    int Run();
    void Handle(const ApplicationEvent& event);
    const ViewState& State() const;

  private:
    struct AwakeSession
    {
        AwakeMode mode;
        std::optional<ElapsedTime> deadline;
    };
    struct CloseSession
    {
        WindowInfo target;
        ElapsedTime deadline;
    };

    void Process(const ApplicationEvent& event);
    void Refresh(bool user_initiated);
    void Select(std::optional<std::size_t> index);
    void UpdateSelection();
    void UpdateOverlay();
    void ClearHighlight();
    void ToggleAwake(AwakeMode mode);
    void StopAwake(bool expired);
    void ToggleClose();
    void Tick();
    void Publish();
    void Shutdown();

    PlatformBinding& platform_;
    ViewState state_;
    std::optional<AwakeSession> awake_;
    std::optional<CloseSession> close_;
    std::deque<ApplicationEvent> events_;
    bool dispatching_ = false;
    bool initialized_ = false;
    std::string pending_error_;
};
} // namespace stay_awake
