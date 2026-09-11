#include "stay_awake/application.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <utility>

namespace stay_awake
{
namespace
{
using namespace std::chrono_literals;

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
    WindowListResult catalog{{}, {{{0xABC, 12}, "Same title", "alpha"}, {{0xDEF, 34}, "Same title", "beta"}}};
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
    ViewState view;
    bool timer_enabled = false;
    int refreshes = 0;
    int exits = 0;
    int presentations = 0;
};

class ApplicationTest : public testing::Test
{
  protected:
    FakePlatformBinding platform;
    Application application{platform};

    void SetUp() override
    {
        Send(EventKind::initialized);
    }
    void Send(EventKind kind)
    {
        application.Handle({kind});
    }
    void Select(std::optional<std::size_t> index)
    {
        application.Handle({EventKind::select_window, index});
    }
    void Duration(EventKind kind, std::optional<std::chrono::seconds> value)
    {
        application.Handle({kind, std::nullopt, value});
    }
    void StartAwake(EventKind mode = EventKind::toggle_display, std::chrono::seconds duration = 10s)
    {
        Duration(EventKind::awake_duration_changed, duration);
        Send(mode);
    }
    void StartClose(std::chrono::seconds duration = 10s)
    {
        Send(EventKind::show);
        Select(0);
        Duration(EventKind::close_duration_changed, duration);
        Send(EventKind::toggle_close);
    }
    const ViewState& State() const
    {
        return application.State();
    }
};

TEST_F(ApplicationTest, InitializesHiddenIdleWithoutUnnecessaryTimer)
{
    EXPECT_FALSE(State().visible);
    EXPECT_FALSE(State().stopped);
    EXPECT_FALSE(State().timer_needed);
    EXPECT_FALSE(State().selected_window);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_FALSE(State().overlay);
    EXPECT_TRUE(State().display_enabled);
    EXPECT_TRUE(State().system_enabled);
    EXPECT_TRUE(State().awake_duration_enabled);
    EXPECT_TRUE(State().close_inputs_enabled);
    EXPECT_EQ(State().display_caption, "Require Display");
    EXPECT_EQ(State().system_caption, "Require System");
    EXPECT_EQ(State().close_caption, "Schedule Close Window");
    EXPECT_EQ(State().awake_remaining, "Not Enabled");
    EXPECT_EQ(State().close_remaining, "Not Enabled");
    EXPECT_EQ(State().awake_duration, 2h);
    EXPECT_EQ(State().close_duration, 1h);
    EXPECT_TRUE(platform.visibility.empty());
    EXPECT_TRUE(platform.power_calls.empty());
    EXPECT_FALSE(platform.timer_enabled);
}

TEST_F(ApplicationTest, StartsEachModeAndStopsOnlyThroughItsActiveButton)
{
    for (const auto event : {EventKind::toggle_display, EventKind::toggle_system})
    {
        const bool display = event == EventKind::toggle_display;
        StartAwake(event);
        ASSERT_EQ(platform.power_calls.back(), display ? AwakeMode::display : AwakeMode::system);
        EXPECT_EQ(State().display_enabled, display);
        EXPECT_EQ(State().system_enabled, !display);
        EXPECT_FALSE(State().awake_duration_enabled);
        EXPECT_EQ(display ? State().display_caption : State().system_caption,
                  display ? "Stop Require Display" : "Stop Require System");
        EXPECT_EQ(State().awake_remaining, "00:00:10");
        const auto count = platform.power_calls.size();
        Send(display ? EventKind::toggle_system : EventKind::toggle_display);
        Duration(EventKind::awake_duration_changed, 1h);
        EXPECT_EQ(platform.power_calls.size(), count);
        EXPECT_EQ(State().awake_duration, 10s);
        Send(event);
        EXPECT_EQ(platform.power_calls.back(), std::nullopt);
        EXPECT_EQ(State().awake_remaining, "Not Enabled");
        EXPECT_TRUE(State().display_enabled);
        EXPECT_TRUE(State().system_enabled);
        EXPECT_TRUE(State().awake_duration_enabled);
        EXPECT_FALSE(State().timer_needed);
    }
}

TEST_F(ApplicationTest, RejectsMissingOrInvalidDurationsWithoutStarting)
{
    Send(EventKind::show);
    Select(0);
    for (const auto invalid : {std::optional<std::chrono::seconds>{}, std::optional(0s), std::optional(-1s),
                               std::optional(11s), std::optional<std::chrono::seconds>(9h)})
    {
        Duration(EventKind::awake_duration_changed, invalid);
        Duration(EventKind::close_duration_changed, invalid);
        Send(EventKind::toggle_display);
        Send(EventKind::toggle_close);
        EXPECT_FALSE(State().timer_needed);
        EXPECT_TRUE(State().awake_duration_enabled);
        EXPECT_TRUE(State().close_inputs_enabled);
    }
    EXPECT_TRUE(platform.power_calls.empty());
    EXPECT_TRUE(platform.close_requests.empty());
    EXPECT_EQ(platform.errors.size(), 10U);
}

TEST_F(ApplicationTest, FailedPowerStartLeavesNoDeadlineAndCanBeRetried)
{
    platform.start_result = {false, "start denied"};
    StartAwake();
    EXPECT_FALSE(State().timer_needed);
    EXPECT_EQ(State().awake_remaining, "Not Enabled");
    EXPECT_TRUE(State().awake_duration_enabled);
    EXPECT_NE(State().awake_status.find("start denied"), std::string::npos);
    EXPECT_EQ(platform.errors.size(), 1U);
    platform.now += 1h;
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 1U);
    platform.start_result = {};
    Send(EventKind::toggle_display);
    EXPECT_TRUE(State().timer_needed);
    EXPECT_TRUE(State().awake_status.empty());
}

TEST_F(ApplicationTest, ExpiryRecordsModeAndTimeOnceAtExactDeadline)
{
    StartAwake(EventKind::toggle_system);
    platform.now = 9999ms;
    Send(EventKind::tick);
    EXPECT_EQ(State().awake_remaining, "00:00:01");
    EXPECT_EQ(platform.power_calls.size(), 1U);
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_EQ(State().awake_status, "Require System Ended At 09/11 12:34:56");
    EXPECT_EQ(State().awake_remaining, "Not Enabled");
    EXPECT_FALSE(State().timer_needed);
    platform.now += 1h;
    Send(EventKind::tick);
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    StartAwake();
    EXPECT_TRUE(State().awake_status.empty());
    EXPECT_EQ(State().awake_caption, "Stay Awake");
}

TEST_F(ApplicationTest, FailedAutomaticReleaseIsVisibleWithoutTimerRetriesAndManualRetryWorks)
{
    StartAwake();
    platform.release_result = {false, "release denied"};
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(State().awake_remaining, "Release failed");
    EXPECT_FALSE(State().timer_needed);
    EXPECT_FALSE(State().awake_duration_enabled);
    EXPECT_TRUE(State().display_enabled);
    EXPECT_FALSE(State().system_enabled);
    EXPECT_NE(State().awake_status.find("Require Display At 09/11 12:34:56"), std::string::npos);
    EXPECT_NE(State().awake_status.find("release denied"), std::string::npos);
    EXPECT_TRUE(platform.errors.empty());
    Send(EventKind::tick);
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    Send(EventKind::toggle_display);
    EXPECT_EQ(platform.errors.size(), 1U);
    EXPECT_EQ(State().awake_remaining, "Release failed");
    platform.release_result = {};
    Send(EventKind::toggle_display);
    EXPECT_EQ(State().awake_remaining, "Not Enabled");
    EXPECT_TRUE(State().awake_duration_enabled);
    EXPECT_NE(State().awake_status.find("Ended At"), std::string::npos);
}

TEST_F(ApplicationTest, ManualReleaseFailureDoesNotClaimProtectionEnded)
{
    StartAwake(EventKind::toggle_system);
    platform.release_result = {false, "retry needed"};
    Send(EventKind::toggle_system);
    EXPECT_EQ(State().awake_remaining, "Release failed");
    EXPECT_EQ(platform.errors.size(), 1U);
    EXPECT_FALSE(State().timer_needed);
    EXPECT_FALSE(State().display_enabled);
}

TEST_F(ApplicationTest, CancelingAwakeLeavesCloseTimerActive)
{
    StartAwake();
    StartClose();
    Send(EventKind::toggle_display);
    EXPECT_TRUE(State().timer_needed);
    EXPECT_FALSE(State().close_inputs_enabled);
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_FALSE(State().timer_needed);
}

TEST_F(ApplicationTest, CancelingCloseLeavesAwakeActive)
{
    StartAwake();
    StartClose();
    Send(EventKind::toggle_close);
    EXPECT_TRUE(State().timer_needed);
    EXPECT_TRUE(State().close_inputs_enabled);
    EXPECT_EQ(State().close_caption, "Schedule Close Window");
    EXPECT_EQ(State().close_remaining, "Not Enabled");
    EXPECT_TRUE(platform.close_requests.empty());
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_TRUE(platform.close_requests.empty());
}

TEST_F(ApplicationTest, EarlierCloseExpiryLeavesAwakeDeadlineUnaffected)
{
    StartAwake(EventKind::toggle_system, 30min);
    StartClose();
    platform.now = 11s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.power_calls.size(), 1U);
    EXPECT_EQ(State().awake_remaining, "00:29:49");
    EXPECT_TRUE(State().timer_needed);
}

TEST_F(ApplicationTest, EarlierAwakeExpiryLeavesCloseDeadlineUnaffected)
{
    StartAwake();
    StartClose(15min);
    platform.now = 11s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_TRUE(platform.close_requests.empty());
    EXPECT_EQ(State().close_remaining, "00:14:49");
    EXPECT_TRUE(State().timer_needed);
}

TEST_F(ApplicationTest, BothDeadlinesExpireOnceOnSameDelayedCallback)
{
    StartAwake();
    StartClose();
    platform.now = 100s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_FALSE(State().timer_needed);
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_EQ(platform.close_requests.size(), 1U);
}

TEST_F(ApplicationTest, FailedReleaseDoesNotPreventSimultaneousCloseCompletion)
{
    StartAwake();
    StartClose();
    platform.release_result = {false, "release failed"};
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(State().awake_remaining, "Release failed");
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_FALSE(State().timer_needed);
    EXPECT_TRUE(platform.errors.empty());
}

TEST_F(ApplicationTest, WallClockJumpsDoNotChangeDurationAndResumeConsumesOverdueWork)
{
    StartAwake();
    StartClose();
    platform.timestamp = "03/01 01:02:03";
    platform.now = 1s;
    Send(EventKind::tick);
    EXPECT_EQ(State().awake_remaining, "00:00:09");
    EXPECT_EQ(State().close_remaining, "00:00:09");
    // Fake elapsed time includes suspend: no actual OS clocks or sleeps in application tests.
    platform.now += 8h;
    platform.timestamp = "03/01 09:02:03";
    Send(EventKind::tick);
    EXPECT_NE(State().awake_status.find("03/01 09:02:03"), std::string::npos);
    EXPECT_NE(State().close_status.find("03/01 09:02"), std::string::npos);
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.power_calls.size(), 2U);
}

TEST_F(ApplicationTest, RestartAfterCancellationUsesNewDeadline)
{
    StartClose();
    platform.now = 9s;
    Send(EventKind::toggle_close);
    Send(EventKind::toggle_close);
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_TRUE(platform.close_requests.empty());
    EXPECT_EQ(State().close_remaining, "00:00:09");
    platform.now = 19s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.close_requests.size(), 1U);
}

TEST_F(ApplicationTest, PreservesSuppliedOrderingDuplicateTitlesAndUnicodeMetadata)
{
    platform.catalog.windows[0].title = u8"窗口 — Café 🌙";
    platform.catalog.windows[0].process_name = u8"编辑器";
    Send(EventKind::show);
    ASSERT_EQ(State().windows.size(), 2U);
    EXPECT_EQ(State().windows[0].title, u8"窗口 — Café 🌙");
    EXPECT_EQ(State().windows[1].identity, (WindowIdentity{0xDEF, 34}));
    Select(0);
    EXPECT_EQ(State().process_name, u8"编辑器");
    EXPECT_EQ(State().window_handle, "ABC");
    EXPECT_EQ(State().window_position, "X: -900, Y: -200, Width: 800, Height: 600");
    platform.catalog.windows[0].title = "Same title";
    Send(EventKind::refresh);
    Select(1);
    Send(EventKind::toggle_close);
    platform.now = 1h;
    Send(EventKind::tick);
    ASSERT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.close_requests[0], (WindowIdentity{0xDEF, 34}));
}

TEST_F(ApplicationTest, MissingMetadataUsesFallbackAndDeselectClearsDetails)
{
    platform.catalog.windows[0].process_name.clear();
    platform.rectangle.reset();
    Send(EventKind::show);
    Select(0);
    EXPECT_EQ(State().process_name, "Unknown");
    EXPECT_EQ(State().window_position, "Error getting position");
    Select(100);
    EXPECT_FALSE(State().selected_window);
    EXPECT_TRUE(State().process_name.empty());
    EXPECT_TRUE(State().window_handle.empty());
    EXPECT_TRUE(State().window_position.empty());
}

TEST_F(ApplicationTest, RefreshFailureClearsSelectionAndOverlayWithUserOnlyDialog)
{
    Send(EventKind::show);
    Select(0);
    Send(EventKind::toggle_highlight);
    ASSERT_TRUE(State().overlay);
    platform.catalog.result = {false, "enumeration failed"};
    Send(EventKind::refresh);
    EXPECT_FALSE(State().selected_window);
    EXPECT_FALSE(State().overlay);
    EXPECT_FALSE(platform.overlay);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_TRUE(State().windows.empty());
    EXPECT_TRUE(State().window_handle.empty());
    EXPECT_NE(State().catalog_status.find("enumeration failed"), std::string::npos);
    ASSERT_EQ(platform.errors.size(), 1U);
    Send(EventKind::show);
    EXPECT_EQ(platform.errors.size(), 1U);
}

TEST_F(ApplicationTest, NoSelectionReportsExactErrorAndDoesNotSchedule)
{
    Send(EventKind::toggle_close);
    ASSERT_EQ(platform.errors.size(), 1U);
    EXPECT_EQ(platform.errors[0], "No window selected.");
    EXPECT_TRUE(State().close_inputs_enabled);
    EXPECT_FALSE(State().timer_needed);
}

TEST_F(ApplicationTest, ScheduleCapturesIdentityAndIgnoresStaleListAndDurationEvents)
{
    StartClose();
    const int refreshes = platform.refreshes;
    Select(1);
    Select(std::nullopt);
    Duration(EventKind::close_duration_changed, 1h);
    Send(EventKind::refresh);
    Send(EventKind::show);
    EXPECT_EQ(platform.refreshes, refreshes);
    EXPECT_EQ(State().selected_window, 0U);
    EXPECT_EQ(State().close_duration, 10s);
    platform.catalog.windows[0] = {{0xAAA, 99}, "replacement", "other"};
    platform.now = 10s;
    Send(EventKind::tick);
    ASSERT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.close_requests[0], (WindowIdentity{0xABC, 12}));
    EXPECT_EQ(State().close_status, "Close requested ABC At 09/11 12:34 (alpha)");
    EXPECT_EQ(State().close_status.find("Closed"), std::string::npos);
    EXPECT_FALSE(State().selected_window);
    EXPECT_TRUE(State().close_inputs_enabled);
    EXPECT_EQ(State().close_caption, "Schedule Close Window");
}

TEST_F(ApplicationTest, CloseFailurePreservesCapturedMetadataEvenWhenAutomaticRefreshFails)
{
    for (const auto* failure : {"Target window no longer exists", "Target window owner changed", "access denied"})
    {
        platform.catalog.result = {};
        StartClose();
        platform.close_result = {false, failure};
        platform.catalog.result = {false, "refresh failed"};
        platform.now += 10s;
        Send(EventKind::tick);
        EXPECT_NE(State().close_status.find("Close request failed ABC At 09/11 12:34 (alpha)"), std::string::npos);
        EXPECT_NE(State().close_status.find(failure), std::string::npos);
        EXPECT_NE(State().catalog_status.find("refresh failed"), std::string::npos);
        EXPECT_TRUE(State().close_inputs_enabled);
        EXPECT_FALSE(State().timer_needed);
        EXPECT_TRUE(platform.errors.empty());
    }
    EXPECT_EQ(platform.close_requests.size(), 3U);
}

TEST_F(ApplicationTest, SuccessfulCloseResultSurvivesFailedAutomaticRefresh)
{
    StartClose();
    platform.catalog.result = {false, "refresh failed"};
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(State().close_status, "Close requested ABC At 09/11 12:34 (alpha)");
    EXPECT_TRUE(platform.errors.empty());
    EXPECT_FALSE(State().timer_needed);
}

TEST_F(ApplicationTest, HighlightIntentWorksWithoutSelectionAndFollowsSelectionSnapshots)
{
    Send(EventKind::show);
    Send(EventKind::toggle_highlight);
    EXPECT_TRUE(State().highlight_active);
    EXPECT_EQ(State().highlight_caption, "Stop Highlighting");
    EXPECT_FALSE(State().overlay);
    EXPECT_FALSE(State().timer_needed);
    Select(0);
    ASSERT_TRUE(State().overlay);
    EXPECT_EQ(State().overlay->x, -900);
    platform.rectangle = Rectangle{500, 300, 100, 200};
    Send(EventKind::tick);
    EXPECT_EQ(State().overlay->x, -900); // No live tracking selected.
    Select(1);
    EXPECT_EQ(State().overlay->x, 500);
    EXPECT_EQ(platform.geometry_targets.back(), (WindowIdentity{0xDEF, 34}));
    Select(std::nullopt);
    EXPECT_FALSE(State().overlay);
    EXPECT_TRUE(State().highlight_active);
    Send(EventKind::toggle_highlight);
    EXPECT_FALSE(State().highlight_active);
}

TEST_F(ApplicationTest, InvalidGeometrySuppressesPreviouslyVisibleOverlay)
{
    Send(EventKind::show);
    Select(0);
    Send(EventKind::toggle_highlight);
    ASSERT_TRUE(platform.overlay);
    for (const auto rectangle :
         {std::optional<Rectangle>{}, std::optional(Rectangle{1, 2, 0, 20}), std::optional(Rectangle{1, 2, 20, -1})})
    {
        platform.rectangle = rectangle;
        Select(1);
        EXPECT_FALSE(State().overlay);
        EXPECT_FALSE(platform.overlay);
    }
}

TEST_F(ApplicationTest, OverlayFailureIsReportedAndResetsIntent)
{
    Send(EventKind::show);
    Select(0);
    platform.overlay_result = {false, "overlay unavailable"};
    Send(EventKind::toggle_highlight);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_FALSE(State().overlay);
    ASSERT_EQ(platform.errors.size(), 1U);
    EXPECT_NE(platform.errors[0].find("overlay unavailable"), std::string::npos);
}

TEST_F(ApplicationTest, CloseScheduleDoesNotDisableHighlightAndHideKeepsBothTimers)
{
    StartAwake();
    StartClose();
    Send(EventKind::toggle_highlight);
    EXPECT_TRUE(State().highlight_active);
    EXPECT_TRUE(State().overlay);
    Send(EventKind::hide);
    EXPECT_FALSE(State().visible);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_FALSE(State().overlay);
    EXPECT_TRUE(State().timer_needed);
    EXPECT_FALSE(State().close_inputs_enabled);
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_FALSE(State().visible);
}

TEST_F(ApplicationTest, QuitCleansUpAndIgnoresAllLaterCallbacks)
{
    StartAwake();
    StartClose();
    Send(EventKind::toggle_highlight);
    Send(EventKind::quit);
    EXPECT_TRUE(State().stopped);
    EXPECT_FALSE(State().timer_needed);
    EXPECT_FALSE(State().overlay);
    EXPECT_EQ(platform.exits, 1);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_TRUE(platform.close_requests.empty());
    const int presentations = platform.presentations;
    const int refreshes = platform.refreshes;
    platform.now = 20s;
    Send(EventKind::tick);
    Send(EventKind::show);
    Send(EventKind::toggle_display);
    Send(EventKind::quit);
    EXPECT_EQ(platform.exits, 1);
    EXPECT_EQ(platform.presentations, presentations);
    EXPECT_EQ(platform.refreshes, refreshes);
    EXPECT_TRUE(platform.close_requests.empty());
}

TEST_F(ApplicationTest, CanceledSessionEndKeepsRunningAndConfirmedEndCleansUpSilently)
{
    StartAwake();
    StartClose();
    Send(EventKind::session_end_canceled);
    EXPECT_FALSE(State().stopped);
    EXPECT_TRUE(State().timer_needed);
    EXPECT_TRUE(platform.power_calls.back().has_value());
    platform.release_result = {false, "release failed"};
    Send(EventKind::session_end_confirmed);
    EXPECT_TRUE(State().stopped);
    EXPECT_FALSE(State().timer_needed);
    EXPECT_TRUE(platform.errors.empty());
    EXPECT_EQ(platform.exits, 1);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_TRUE(platform.close_requests.empty());
}

TEST_F(ApplicationTest, TimerSetupFailureCancelsCloseAndReleasesAwake)
{
    platform.timer_result = {false, "timer unavailable"};
    StartAwake();
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_EQ(State().awake_remaining, "Not Enabled");
    EXPECT_FALSE(State().timer_needed);
    EXPECT_NE(State().awake_status.find("timer unavailable"), std::string::npos);
    StartClose();
    EXPECT_TRUE(State().close_inputs_enabled);
    EXPECT_FALSE(State().timer_needed);
    EXPECT_NE(State().close_status.find("timer unavailable"), std::string::npos);
    platform.now = 1h;
    Send(EventKind::tick);
    EXPECT_TRUE(platform.close_requests.empty());
}

TEST_F(ApplicationTest, TimerFailureWithReleaseFailureKeepsManualRetryAvailable)
{
    platform.timer_result = {false, "timer unavailable"};
    platform.release_result = {false, "release failed"};
    StartAwake();
    EXPECT_EQ(State().awake_remaining, "Release failed");
    EXPECT_FALSE(State().timer_needed);
    EXPECT_TRUE(State().display_enabled);
    EXPECT_FALSE(State().system_enabled);
    EXPECT_EQ(platform.errors.size(), 1U);
    platform.release_result = {};
    Send(EventKind::toggle_display);
    EXPECT_EQ(State().awake_remaining, "Not Enabled");
}

TEST_F(ApplicationTest, ReentrantTickDuringCloseCannotPostTwice)
{
    StartAwake();
    StartClose();
    platform.on_close = [this] { Send(EventKind::tick); };
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_FALSE(State().timer_needed);
    EXPECT_EQ(State().close_status, "Close requested ABC At 09/11 12:34 (alpha)");
}

TEST_F(ApplicationTest, ModalErrorSeesCompletedStateAndQuitImmediatelyDisablesLaterEvents)
{
    StartAwake();
    platform.release_result = {false, "release failed"};
    platform.on_error = [this] {
        EXPECT_EQ(platform.view.awake_remaining, "Release failed");
        EXPECT_FALSE(platform.view.timer_needed);
        Send(EventKind::quit);
        Send(EventKind::show);
        Send(EventKind::toggle_system);
    };
    Send(EventKind::toggle_display);
    EXPECT_TRUE(State().stopped);
    EXPECT_EQ(platform.exits, 1);
    EXPECT_EQ(platform.power_calls.size(), 3U); // Start, failed stop, best-effort exit release.
    EXPECT_EQ(platform.errors.size(), 1U);
    EXPECT_TRUE(platform.visibility.empty());
}

TEST_F(ApplicationTest, ConfirmedSessionEndDuringModalErrorCleansUpBeforeReturningToTheDialog)
{
    StartAwake();
    StartClose();
    platform.release_result = {false, "release failed"};
    platform.on_error = [this] {
        Send(EventKind::session_end_confirmed);
        EXPECT_TRUE(State().stopped);
        EXPECT_EQ(platform.exits, 1);
        EXPECT_FALSE(platform.timer_enabled);
        EXPECT_FALSE(platform.overlay);
        EXPECT_TRUE(platform.close_requests.empty());
    };
    Send(EventKind::toggle_display);
    EXPECT_TRUE(State().stopped);
    EXPECT_EQ(platform.errors.size(), 1U);
}

TEST_F(ApplicationTest, CanceledSessionEndDuringModalErrorRetainsCloseSchedule)
{
    StartAwake();
    StartClose();
    platform.release_result = {false, "release failed"};
    platform.on_error = [this] { Send(EventKind::session_end_canceled); };
    Send(EventKind::toggle_display);
    EXPECT_FALSE(State().stopped);
    EXPECT_TRUE(State().timer_needed);
    platform.now = 10s;
    Send(EventKind::tick);
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(State().awake_remaining, "Release failed");
}

TEST(ApplicationRunTest, ServiceActivationAndFailureUsePortableBoundary)
{
    FakePlatformBinding platform;
    platform.service_events = {{EventKind::initialized}, {EventKind::show}, {EventKind::quit}};
    Application application(platform);
    EXPECT_EQ(application.Run(), 0);
    EXPECT_EQ(platform.visibility, (std::vector<bool>{true}));
    EXPECT_EQ(platform.exits, 1);
    EXPECT_TRUE(application.State().stopped);
    FakePlatformBinding failed;
    failed.service_result = {false, "startup failed"};
    Application failing(failed);
    EXPECT_EQ(failing.Run(), 1);
    EXPECT_EQ(failed.errors, (std::vector<std::string>{"startup failed"}));
}
} // namespace
} // namespace stay_awake
