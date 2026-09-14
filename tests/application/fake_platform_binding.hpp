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
        ++service_runs;
        if (on_service)
        {
            on_service();
        }
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
    std::string LocalTimestamp() override
    {
        return timestamp;
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
        const auto result = value ? overlay_result : overlay_clear_result;
        if (result.success)
        {
            overlay = value;
        }
        return result;
    }
    OperationResult ScheduleTick(std::optional<ElapsedTime> deadline) override
    {
        if (deadline == timer_deadline)
        {
            return {};
        }
        timer_requests.push_back(deadline);
        timer_deadline = timer_result.success ? deadline : std::nullopt;
        return deadline ? timer_result : OperationResult{};
    }
    bool DeliverTick(const EventHandler& handler)
    {
        if (!timer_deadline)
        {
            return false;
        }
        timer_deadline.reset();
        handler({EventKind::tick});
        return true;
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
    void ShowWindowDetails(const WindowInfo& window) override
    {
        window_details.push_back(window);
        if (on_details)
        {
            on_details();
        }
    }
    void ShowStatus(std::string_view title, std::string_view message) override
    {
        status_titles.emplace_back(title);
        statuses.emplace_back(message);
        if (on_status)
        {
            on_status();
        }
    }

    ElapsedTime now{0};
    std::string timestamp = "Friday, September 11, 2026 12:34:56 PM";
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
    OperationResult overlay_clear_result;
    OperationResult timer_result;
    std::optional<ElapsedTime> timer_deadline;
    std::vector<ApplicationEvent> service_events;
    std::vector<std::optional<AwakeMode>> power_calls;
    std::vector<std::optional<ElapsedTime>> timer_requests;
    std::vector<WindowIdentity> close_requests;
    std::vector<WindowIdentity> geometry_targets;
    std::vector<bool> visibility;
    std::vector<std::string> errors;
    std::vector<WindowInfo> window_details;
    std::vector<std::string> status_titles;
    std::vector<std::string> statuses;
    std::function<void()> on_close;
    std::function<void()> on_error;
    std::function<void()> on_present;
    std::function<void()> on_details;
    std::function<void()> on_status;
    std::function<void()> on_service;
    ViewState view;
    int refreshes = 0;
    int exits = 0;
    int presentations = 0;
    int service_runs = 0;
};
} // namespace stay_awake
