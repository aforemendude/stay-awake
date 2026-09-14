#include "stay_awake/awake_controller.hpp"

#include "fake_platform_binding.hpp"

#include <gtest/gtest.h>

namespace stay_awake
{
namespace
{
using namespace std::chrono_literals;

class AwakeControllerTest : public testing::Test
{
  protected:
    FakePlatformBinding platform;
    AwakeController awake{platform};

    void Start(AwakeMode mode = AwakeMode::display)
    {
        awake.SetDuration(10s);
        ASSERT_TRUE(awake.Toggle(mode).success);
    }
    AwakeViewState State() const
    {
        return awake.State(platform.now);
    }
};

TEST_F(AwakeControllerTest, DefaultsToAnIdleTwoHourSessionWithThirtyMinuteChoices)
{
    EXPECT_EQ(State().duration, 2h);
    ASSERT_EQ(State().durations.size(), 32U);
    EXPECT_EQ(State().durations.front().duration, 30min);
    EXPECT_EQ(State().durations.front().label, "00:30:00");
    EXPECT_EQ(State().status, "Ready");
    EXPECT_TRUE(State().duration_enabled);
    EXPECT_TRUE(State().display_enabled);
    EXPECT_TRUE(State().system_enabled);
    EXPECT_EQ(State().display_caption, "Require Display");
    EXPECT_EQ(State().system_caption, "Require System");
    EXPECT_FALSE(awake.NeedsTimer());
    EXPECT_EQ(awake.NextUpdate(platform.now), std::nullopt);
    EXPECT_TRUE(platform.power_calls.empty());
}

TEST_F(AwakeControllerTest, StartsEachModeAndStopsOnlyThroughItsActiveButton)
{
    for (const auto mode : {AwakeMode::display, AwakeMode::system})
    {
        SCOPED_TRACE(static_cast<int>(mode));
        platform.power_calls.clear();
        const bool display = mode == AwakeMode::display;
        Start(mode);
        EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{mode}));
        EXPECT_EQ(State().display_enabled, display);
        EXPECT_EQ(State().system_enabled, !display);
        EXPECT_FALSE(State().duration_enabled);
        EXPECT_EQ(display ? State().display_caption : State().system_caption,
                  display ? "Stop Require Display" : "Stop Require System");
        EXPECT_EQ(State().status,
                  display ? "Require Display - 00:00:10 remaining" : "Require System - 00:00:10 remaining");
        EXPECT_TRUE(awake.Toggle(display ? AwakeMode::system : AwakeMode::display).success);
        awake.SetDuration(1h);
        EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{mode}));
        EXPECT_EQ(State().duration, 10s);
        EXPECT_TRUE(awake.Toggle(mode).success);
        EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{mode, std::nullopt}));
        EXPECT_EQ(State().status, "Ready");
        EXPECT_TRUE(State().display_enabled);
        EXPECT_TRUE(State().system_enabled);
        EXPECT_TRUE(State().duration_enabled);
        EXPECT_EQ(State().display_caption, "Require Display");
        EXPECT_EQ(State().system_caption, "Require System");
        EXPECT_FALSE(awake.NeedsTimer());
    }
}

TEST_F(AwakeControllerTest, RejectsMissingOrInvalidDurationsWithoutStarting)
{
    for (const auto invalid : {std::optional<std::chrono::seconds>{}, std::optional(0s), std::optional(-1s),
                               std::optional(11s), std::optional<std::chrono::seconds>(9h)})
    {
        awake.SetDuration(invalid);
        const auto result = awake.Toggle(AwakeMode::display);
        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.error, "Select a valid stay awake duration.");
        EXPECT_EQ(State().status, result.error);
        EXPECT_FALSE(awake.NeedsTimer());
        EXPECT_TRUE(State().duration_enabled);
    }
    EXPECT_TRUE(platform.power_calls.empty());
}

TEST_F(AwakeControllerTest, FailedPowerStartLeavesNoDeadlineAndCanBeRetried)
{
    platform.start_result = {false, "start denied"};
    awake.SetDuration(10s);
    const auto result = awake.Toggle(AwakeMode::display);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Failed to start stay awake: start denied");
    EXPECT_FALSE(awake.NeedsTimer());
    EXPECT_TRUE(State().duration_enabled);
    EXPECT_EQ(State().status, "Failed to start Require Display: start denied");
    platform.now += 1h;
    awake.Tick(platform.now);
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::display}));
    platform.start_result = {};
    EXPECT_TRUE(awake.Toggle(AwakeMode::display).success);
    EXPECT_TRUE(awake.NeedsTimer());
    EXPECT_EQ(State().status, "Require Display - 00:00:10 remaining");
    EXPECT_EQ(awake.NextUpdate(platform.now), 1h + 1s);
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::display, AwakeMode::display}));
}

TEST_F(AwakeControllerTest, ExpiryRecordsModeAndTimeOnceAtExactDeadline)
{
    Start(AwakeMode::system);
    platform.now = 9999ms;
    awake.Tick(platform.now);
    EXPECT_EQ(State().status, "Require System - 00:00:01 remaining");
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::system}));
    platform.now = 10s;
    awake.Tick(platform.now);
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::system, std::nullopt}));
    EXPECT_EQ(State().status, "Require System Ended At Friday, September 11, 2026 12:34:56 PM");
    EXPECT_FALSE(awake.NeedsTimer());
    platform.now += 1h;
    awake.Tick(platform.now);
    awake.Tick(platform.now);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    Start();
    EXPECT_EQ(State().status, "Require Display - 00:00:10 remaining");
}

TEST_F(AwakeControllerTest, FailedAutomaticReleaseIsVisibleWithoutTimerRetriesAndManualRetryWorks)
{
    Start();
    platform.release_result = {false, "release denied"};
    platform.now = 10s;
    awake.Tick(platform.now);
    const std::string failure = "Error Ending Require Display At Friday, September 11, 2026 12:34:56 PM: "
                                "release denied. Click the active mode to retry.";
    EXPECT_EQ(State().status, failure);
    EXPECT_FALSE(awake.NeedsTimer());
    EXPECT_FALSE(State().duration_enabled);
    EXPECT_TRUE(State().display_enabled);
    EXPECT_FALSE(State().system_enabled);
    EXPECT_TRUE(platform.errors.empty());
    awake.Tick(platform.now);
    awake.Tick(platform.now);
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::display, std::nullopt}));
    const auto retry = awake.Toggle(AwakeMode::display);
    EXPECT_FALSE(retry.success);
    EXPECT_EQ(retry.error, "Failed to stop stay awake: release denied");
    EXPECT_EQ(State().status, failure);
    EXPECT_EQ(awake.NextUpdate(platform.now), std::nullopt);
    platform.release_result = {};
    EXPECT_TRUE(awake.Toggle(AwakeMode::display).success);
    EXPECT_TRUE(State().duration_enabled);
    EXPECT_EQ(State().status, "Require Display Ended At Friday, September 11, 2026 12:34:56 PM");
    EXPECT_EQ(platform.power_calls,
              (std::vector<std::optional<AwakeMode>>{AwakeMode::display, std::nullopt, std::nullopt, std::nullopt}));
}

TEST_F(AwakeControllerTest, ManualReleaseFailureDoesNotClaimProtectionEnded)
{
    Start(AwakeMode::system);
    platform.release_result = {false, "retry needed"};
    const auto result = awake.Toggle(AwakeMode::system);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Failed to stop stay awake: retry needed");
    EXPECT_EQ(State().status, "Error Ending Require System At Friday, September 11, 2026 12:34:56 PM: retry needed. "
                              "Click the active mode to retry.");
    EXPECT_FALSE(awake.NeedsTimer());
    EXPECT_FALSE(State().display_enabled);
    EXPECT_TRUE(State().system_enabled);
    EXPECT_FALSE(State().duration_enabled);
    EXPECT_EQ(State().system_caption, "Stop Require System");
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::system, std::nullopt}));
}

TEST_F(AwakeControllerTest, NextUpdateUsesTheCapturedDeadlineAndDisappearsAfterManualStop)
{
    platform.now = 350ms;
    Start();
    EXPECT_EQ(awake.NextUpdate(350ms), 1350ms);
    EXPECT_EQ(awake.NextUpdate(1350ms), 2350ms);
    EXPECT_EQ(awake.NextUpdate(10349ms), 10350ms);
    EXPECT_EQ(awake.NextUpdate(10350ms), 10350ms);
    EXPECT_EQ(awake.NextUpdate(11s), 11s);
    EXPECT_TRUE(awake.Toggle(AwakeMode::display).success);
    EXPECT_EQ(awake.NextUpdate(11s), std::nullopt);
    platform.now = 20s;
    Start();
    awake.Tick(29999ms);
    EXPECT_EQ(platform.power_calls,
              (std::vector<std::optional<AwakeMode>>{AwakeMode::display, std::nullopt, AwakeMode::display}));
    awake.Tick(30s);
    EXPECT_EQ(awake.NextUpdate(30s), std::nullopt);
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::display, std::nullopt,
                                                                           AwakeMode::display, std::nullopt}));
}

TEST_F(AwakeControllerTest, TimerFailureCancelsActivePowerOnceAndLeavesIdleStateAlone)
{
    awake.CancelForTimerFailure("unused");
    EXPECT_EQ(State().status, "Ready");
    EXPECT_TRUE(platform.power_calls.empty());
    Start();
    awake.CancelForTimerFailure("timer unavailable");
    EXPECT_EQ(State().status, "Canceled: timer unavailable");
    EXPECT_TRUE(State().duration_enabled);
    EXPECT_EQ(awake.NextUpdate(platform.now), std::nullopt);
    awake.CancelForTimerFailure("another failure");
    awake.Tick(1h);
    EXPECT_EQ(State().status, "Canceled: timer unavailable");
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::display, std::nullopt}));
}

TEST_F(AwakeControllerTest, TimerFailurePreservesFailedReleaseUntilManualRetry)
{
    Start(AwakeMode::system);
    platform.release_result = {false, "release denied"};
    awake.CancelForTimerFailure("timer unavailable");
    EXPECT_EQ(State().status, "Error Ending Require System At Friday, September 11, 2026 12:34:56 PM: release denied. "
                              "Click the active mode to retry.");
    EXPECT_FALSE(State().duration_enabled);
    EXPECT_EQ(awake.NextUpdate(platform.now), std::nullopt);
    awake.CancelForTimerFailure("another failure");
    awake.Tick(1h);
    EXPECT_TRUE(awake.Toggle(AwakeMode::display).success);
    awake.SetDuration(1h);
    EXPECT_EQ(State().duration, 10s);
    EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::system, std::nullopt}));
    platform.release_result = {};
    EXPECT_TRUE(awake.Toggle(AwakeMode::system).success);
    EXPECT_EQ(State().status, "Require System Ended At Friday, September 11, 2026 12:34:56 PM");
    EXPECT_TRUE(State().duration_enabled);
    EXPECT_EQ(platform.power_calls,
              (std::vector<std::optional<AwakeMode>>{AwakeMode::system, std::nullopt, std::nullopt}));
}

TEST_F(AwakeControllerTest, ShutdownReleasesOnceEvenOnFailureAndDropsTheDeadline)
{
    for (const auto& release : {OperationResult{}, OperationResult{false, "release denied"}})
    {
        platform.release_result = release;
        platform.power_calls.clear();
        awake.Shutdown();
        EXPECT_TRUE(platform.power_calls.empty());
        Start();
        awake.Shutdown();
        awake.Shutdown();
        awake.Tick(1h);
        EXPECT_EQ(platform.power_calls, (std::vector<std::optional<AwakeMode>>{AwakeMode::display, std::nullopt}));
        EXPECT_EQ(awake.NextUpdate(platform.now), std::nullopt);
        EXPECT_TRUE(State().duration_enabled);
        EXPECT_TRUE(State().display_enabled);
        EXPECT_TRUE(State().system_enabled);
        EXPECT_TRUE(platform.errors.empty());
    }
}
} // namespace
} // namespace stay_awake
