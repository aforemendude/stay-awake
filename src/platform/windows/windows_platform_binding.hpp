#pragma once

#include "platform/windows/main_window.hpp"
#include "platform/windows/overlay_window.hpp"
#include "platform/windows/power_manager.hpp"
#include "platform/windows/single_instance.hpp"
#include "stay_awake/platform_binding.hpp"

#include <memory>

namespace stay_awake::windows
{
class WindowsPlatformBinding final : public PlatformBinding
{
  public:
    OperationResult RunService(EventHandler handler) override;
    ElapsedTime Now() override;
    std::string LocalTimestamp(bool with_seconds) override;
    OperationResult SetAwake(std::optional<AwakeMode> mode) override;
    WindowListResult EnumerateWindows() override;
    std::optional<Rectangle> WindowRectangle(WindowIdentity target) override;
    OperationResult RequestClose(WindowIdentity target) override;
    OperationResult SetOverlay(std::optional<Rectangle> rectangle) override;
    OperationResult SetTimerEnabled(bool enabled) override;
    void Present(const ViewState& state) override;
    void SetWindowVisible(bool visible) override;
    void ShowError(std::string_view message) override;
    void RequestExit() override;

  private:
    void Cleanup() noexcept;

    // Reverse destruction order retains instance ownership until every native resource and power request is gone.
    SingleInstance instance_;
    PowerManager power_;
    std::unique_ptr<MainWindow> window_;
    OverlayWindow overlay_;
    std::unique_ptr<TrayIcon> tray_;
    bool exiting_ = false;
};
} // namespace stay_awake::windows
