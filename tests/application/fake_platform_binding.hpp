#pragma once

#include "stay_awake/platform_binding.hpp"

#include <functional>
#include <string>
#include <vector>

namespace stay_awake
{
class FakePlatformBinding final : public PlatformBinding
{
  public:
    OperationResult RunService(EventHandler handler) override
    {
        for (const auto& event : service_events)
        {
            handler(event);
        }
        return service_result;
    }
    ElapsedTime Now() override
    {
        return now;
    }
    std::string LocalTimestamp(bool with_seconds) override
    {
        return with_seconds ? timestamp : timestamp.substr(0, 11);
    }
    OperationResult SetAwake(std::optional<AwakeMode> mode) override
    {
        power_calls.push_back(mode);
        return mode ? start_result : release_result;
    }
    WindowListResult EnumerateWindows() override
    {
        ++refreshes;
        return catalog;
    }
    std::optional<Rectangle> WindowRectangle(WindowIdentity target) override
    {
        geometry_targets.push_back(target);
        return rectangle;
    }
    OperationResult RequestClose(WindowIdentity target) override
    {
        close_requests.push_back(target);
        if (on_close)
        {
            on_close();
        }
        return close_result;
    }
    OperationResult SetOverlay(std::optional<Rectangle> value) override
    {
        overlay = value;
        return value ? overlay_result : OperationResult{};
    }
    OperationResult SetTimerEnabled(bool enabled) override
    {
        timer_enabled = enabled && timer_result.success;
        return enabled ? timer_result : OperationResult{};
    }
    void Present(const ViewState& state) override
    {
        view = state;
        ++presentations;
        if (on_present)
        {
            on_present();
        }
    }
    void SetWindowVisible(bool visible) override
    {
        visibility.push_back(visible);
    }
    void ShowError(std::string_view message) override
    {
        errors.emplace_back(message);
        if (on_error)
        {
            on_error();
        }
    }
    void RequestExit() override
    {
        ++exits;
    }

    ElapsedTime now{0};
    std::string timestamp = "09/11 12:34:56";
    WindowListResult catalog{
        {},
        {{{0xABC, 12, 0x100000001ULL}, "Same title", "alpha"}, {{0xDEF, 34, 0x200000002ULL}, "Same title", "beta"}}};
    std::optional<Rectangle> rectangle = Rectangle{-900, -200, 800, 600};
    std::optional<Rectangle> overlay;
    OperationResult service_result;
    OperationResult start_result;
    OperationResult release_result;
    OperationResult close_result;
    OperationResult overlay_result;
    OperationResult timer_result;
    std::vector<ApplicationEvent> service_events;
    std::vector<std::optional<AwakeMode>> power_calls;
    std::vector<WindowIdentity> close_requests;
    std::vector<WindowIdentity> geometry_targets;
    std::vector<bool> visibility;
    std::vector<std::string> errors;
    std::function<void()> on_close;
    std::function<void()> on_error;
    std::function<void()> on_present;
    ViewState view;
    bool timer_enabled = false;
    int refreshes = 0;
    int exits = 0;
    int presentations = 0;
};
} // namespace stay_awake
