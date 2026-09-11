#pragma once

#include "stay_awake/awake_controller.hpp"
#include "stay_awake/close_controller.hpp"
#include "stay_awake/window_selection.hpp"

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
    void Process(const ApplicationEvent& event);
    void Tick();
    void Publish();
    void Shutdown();
    void ReportError(const OperationResult& result);

    PlatformBinding& platform_;
    AwakeController awake_;
    CloseController close_;
    WindowSelection selection_;
    ViewState state_;
    std::deque<ApplicationEvent> events_;
    bool dispatching_ = false;
    bool initialized_ = false;
    std::string pending_error_;
};
} // namespace stay_awake
