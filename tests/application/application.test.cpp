#include "stay_awake/application.hpp"

#include "fake_platform_binding.hpp"

#include <gtest/gtest.h>

namespace stay_awake
{
namespace
{
using namespace std::chrono_literals;

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
    EXPECT_FALSE(State().selection.selected_window);
    EXPECT_FALSE(State().selection.highlight_active);
    EXPECT_FALSE(State().selection.overlay);
    EXPECT_TRUE(State().awake.display_enabled);
    EXPECT_TRUE(State().awake.system_enabled);
    EXPECT_TRUE(State().awake.duration_enabled);
    EXPECT_TRUE(State().close.inputs_enabled);
    EXPECT_EQ(State().awake.display_caption, "Require Display");
    EXPECT_EQ(State().awake.system_caption, "Require System");
    EXPECT_EQ(State().close.caption, "Schedule Close Window");
    EXPECT_EQ(State().awake.remaining, "Not Enabled");
    EXPECT_EQ(State().close.remaining, "Not Enabled");
    EXPECT_EQ(State().awake.duration, 2h);
    EXPECT_EQ(State().close.duration, 1h);
    EXPECT_TRUE(platform.visibility.empty());
    EXPECT_TRUE(platform.power_calls.empty());
    EXPECT_FALSE(platform.timer_enabled);
}

TEST_F(ApplicationTest, CancelingAwakeLeavesCloseTimerActive)
{
    StartAwake();
    StartClose();
    Send(EventKind::toggle_display);
    EXPECT_TRUE(State().timer_needed);
    EXPECT_FALSE(State().close.inputs_enabled);
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
    EXPECT_TRUE(State().close.inputs_enabled);
    EXPECT_EQ(State().close.caption, "Schedule Close Window");
    EXPECT_EQ(State().close.remaining, "Not Enabled");
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
    EXPECT_EQ(State().awake.remaining, "00:29:49");
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
    EXPECT_EQ(State().close.remaining, "00:14:49");
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
    EXPECT_EQ(State().awake.remaining, "Release failed");
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
    EXPECT_EQ(State().awake.remaining, "00:00:09");
    EXPECT_EQ(State().close.remaining, "00:00:09");
    // Fake elapsed time includes suspend: no actual OS clocks or sleeps in application tests.
    platform.now += 8h;
    platform.timestamp = "03/01 09:02:03";
    Send(EventKind::tick);
    EXPECT_NE(State().awake.status.find("03/01 09:02:03"), std::string::npos);
    EXPECT_NE(State().close.status.find("03/01 09:02"), std::string::npos);
    EXPECT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.power_calls.size(), 2U);
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
    EXPECT_EQ(State().selection.selected_window, 0U);
    EXPECT_EQ(State().close.duration, 10s);
    platform.catalog.windows[0] = {{0xAAA, 99}, "replacement", "other"};
    platform.now = 10s;
    Send(EventKind::tick);
    ASSERT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.close_requests[0], (WindowIdentity{0xABC, 12}));
    EXPECT_EQ(State().close.status, "Close requested ABC At 09/11 12:34 (alpha)");
    EXPECT_EQ(State().close.status.find("Closed"), std::string::npos);
    EXPECT_FALSE(State().selection.selected_window);
    EXPECT_TRUE(State().close.inputs_enabled);
    EXPECT_EQ(State().close.caption, "Schedule Close Window");
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
        EXPECT_NE(State().close.status.find("Close request failed ABC At 09/11 12:34 (alpha)"), std::string::npos);
        EXPECT_NE(State().close.status.find(failure), std::string::npos);
        EXPECT_NE(State().selection.catalog_status.find("refresh failed"), std::string::npos);
        EXPECT_TRUE(State().close.inputs_enabled);
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
    EXPECT_EQ(State().close.status, "Close requested ABC At 09/11 12:34 (alpha)");
    EXPECT_TRUE(platform.errors.empty());
    EXPECT_FALSE(State().timer_needed);
}

TEST_F(ApplicationTest, CloseScheduleDoesNotDisableHighlightAndHideKeepsBothTimers)
{
    StartAwake();
    StartClose();
    Send(EventKind::toggle_highlight);
    EXPECT_TRUE(State().selection.highlight_active);
    EXPECT_TRUE(State().selection.overlay);
    Send(EventKind::hide);
    EXPECT_FALSE(State().visible);
    EXPECT_FALSE(State().selection.highlight_active);
    EXPECT_FALSE(State().selection.overlay);
    EXPECT_TRUE(State().timer_needed);
    EXPECT_FALSE(State().close.inputs_enabled);
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
    EXPECT_FALSE(State().selection.overlay);
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
    EXPECT_EQ(State().awake.remaining, "Not Enabled");
    EXPECT_FALSE(State().timer_needed);
    EXPECT_NE(State().awake.status.find("timer unavailable"), std::string::npos);
    StartClose();
    EXPECT_TRUE(State().close.inputs_enabled);
    EXPECT_FALSE(State().timer_needed);
    EXPECT_NE(State().close.status.find("timer unavailable"), std::string::npos);
    platform.now = 1h;
    Send(EventKind::tick);
    EXPECT_TRUE(platform.close_requests.empty());
}

TEST_F(ApplicationTest, TimerFailureWithReleaseFailureKeepsManualRetryAvailable)
{
    platform.timer_result = {false, "timer unavailable"};
    platform.release_result = {false, "release failed"};
    StartAwake();
    EXPECT_EQ(State().awake.remaining, "Release failed");
    EXPECT_FALSE(State().timer_needed);
    EXPECT_TRUE(State().awake.display_enabled);
    EXPECT_FALSE(State().awake.system_enabled);
    EXPECT_EQ(platform.errors.size(), 1U);
    platform.release_result = {};
    Send(EventKind::toggle_display);
    EXPECT_EQ(State().awake.remaining, "Not Enabled");
}

TEST_F(ApplicationTest, PresentsFeatureErrorsAndKeepsAutomaticCatalogFailuresSilent)
{
    Send(EventKind::toggle_close);
    ASSERT_EQ(platform.errors.size(), 1U);
    EXPECT_EQ(platform.errors.back(), "No window selected.");
    EXPECT_TRUE(platform.view.close.inputs_enabled);
    Send(EventKind::show);
    Select(0);
    platform.overlay_result = {false, "overlay unavailable"};
    Send(EventKind::toggle_highlight);
    ASSERT_EQ(platform.errors.size(), 2U);
    EXPECT_EQ(platform.errors.back(), "Failed to highlight window: overlay unavailable");
    EXPECT_FALSE(platform.view.selection.highlight_active);
    EXPECT_FALSE(platform.view.selection.overlay);
    platform.catalog.result = {false, "enumeration failed"};
    Send(EventKind::refresh);
    ASSERT_EQ(platform.errors.size(), 3U);
    EXPECT_EQ(platform.errors.back(), "Failed to refresh windows list: enumeration failed");
    EXPECT_EQ(platform.view.selection.catalog_status, platform.errors.back());
    Send(EventKind::show);
    EXPECT_EQ(platform.errors.size(), 3U);
}

TEST_F(ApplicationTest, HighlightAloneNeedsNoTimerAndTicksKeepTheSelectionSnapshot)
{
    Send(EventKind::show);
    Select(0);
    Send(EventKind::toggle_highlight);
    ASSERT_TRUE(State().selection.overlay);
    EXPECT_FALSE(State().timer_needed);
    const auto queries = platform.geometry_targets.size();
    platform.rectangle = Rectangle{500, 300, 100, 200};
    Send(EventKind::tick);
    EXPECT_EQ(State().selection.overlay->x, -900);
    EXPECT_EQ(platform.geometry_targets.size(), queries);
    EXPECT_FALSE(platform.timer_enabled);
}

TEST_F(ApplicationTest, ReentrantFeatureEventsWaitUntilPresentationCompletes)
{
    Duration(EventKind::awake_duration_changed, 10s);
    bool delivered = false;
    platform.on_present = [this, &delivered] {
        if (delivered)
        {
            return;
        }
        delivered = true;
        Send(EventKind::toggle_system);
        Duration(EventKind::awake_duration_changed, 1h);
        Send(EventKind::toggle_display);
        EXPECT_EQ(platform.power_calls.size(), 1U);
        EXPECT_EQ(State().awake.remaining, "00:00:10");
        EXPECT_FALSE(platform.view.awake.duration_enabled);
    };
    Send(EventKind::toggle_display);
    EXPECT_TRUE(delivered);
    EXPECT_EQ(State().awake.duration, 10s);
    EXPECT_EQ(State().awake.remaining, "Not Enabled");
    EXPECT_TRUE(State().awake.duration_enabled);
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::display, std::nullopt}));
    EXPECT_FALSE(State().timer_needed);
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
    EXPECT_EQ(State().close.status, "Close requested ABC At 09/11 12:34 (alpha)");
}

TEST_F(ApplicationTest, ModalErrorSeesCompletedStateAndQuitImmediatelyDisablesLaterEvents)
{
    StartAwake();
    platform.release_result = {false, "release failed"};
    platform.on_error = [this] {
        EXPECT_EQ(platform.view.awake.remaining, "Release failed");
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
    EXPECT_EQ(State().awake.remaining, "Release failed");
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
