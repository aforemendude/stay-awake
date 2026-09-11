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

TEST_F(AwakeControllerTest, StartsEachModeAndStopsOnlyThroughItsActiveButton)
{
    for (const auto mode : {AwakeMode::display, AwakeMode::system})
    {
        const bool display = mode == AwakeMode::display;
        Start(mode);
        ASSERT_EQ(platform.power_calls.back(), mode);
        EXPECT_EQ(State().display_enabled, display);
        EXPECT_EQ(State().system_enabled, !display);
        EXPECT_FALSE(State().duration_enabled);
        EXPECT_EQ(display ? State().display_caption : State().system_caption,
                  display ? "Stop Require Display" : "Stop Require System");
        EXPECT_EQ(State().remaining, "00:00:10");
        const auto count = platform.power_calls.size();
        EXPECT_TRUE(awake.Toggle(display ? AwakeMode::system : AwakeMode::display).success);
        awake.SetDuration(1h);
        EXPECT_EQ(platform.power_calls.size(), count);
        EXPECT_EQ(State().duration, 10s);
        EXPECT_TRUE(awake.Toggle(mode).success);
        EXPECT_EQ(platform.power_calls.back(), std::nullopt);
        EXPECT_EQ(State().remaining, "Not Enabled");
        EXPECT_TRUE(State().display_enabled);
        EXPECT_TRUE(State().system_enabled);
        EXPECT_TRUE(State().duration_enabled);
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
    EXPECT_EQ(State().remaining, "Not Enabled");
    EXPECT_TRUE(State().duration_enabled);
    EXPECT_NE(State().status.find("start denied"), std::string::npos);
    platform.now += 1h;
    awake.Tick(platform.now);
    EXPECT_EQ(platform.power_calls.size(), 1U);
    platform.start_result = {};
    EXPECT_TRUE(awake.Toggle(AwakeMode::display).success);
    EXPECT_TRUE(awake.NeedsTimer());
    EXPECT_TRUE(State().status.empty());
}

TEST_F(AwakeControllerTest, ExpiryRecordsModeAndTimeOnceAtExactDeadline)
{
    Start(AwakeMode::system);
    platform.now = 9999ms;
    awake.Tick(platform.now);
    EXPECT_EQ(State().remaining, "00:00:01");
    EXPECT_EQ(platform.power_calls.size(), 1U);
    platform.now = 10s;
    awake.Tick(platform.now);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    EXPECT_EQ(State().status, "Require System Ended At 09/11 12:34:56");
    EXPECT_EQ(State().remaining, "Not Enabled");
    EXPECT_FALSE(awake.NeedsTimer());
    platform.now += 1h;
    awake.Tick(platform.now);
    awake.Tick(platform.now);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    Start();
    EXPECT_TRUE(State().status.empty());
    EXPECT_EQ(State().caption, "Stay Awake");
}

TEST_F(AwakeControllerTest, FailedAutomaticReleaseIsVisibleWithoutTimerRetriesAndManualRetryWorks)
{
    Start();
    platform.release_result = {false, "release denied"};
    platform.now = 10s;
    awake.Tick(platform.now);
    EXPECT_EQ(State().remaining, "Release failed");
    EXPECT_FALSE(awake.NeedsTimer());
    EXPECT_FALSE(State().duration_enabled);
    EXPECT_TRUE(State().display_enabled);
    EXPECT_FALSE(State().system_enabled);
    EXPECT_NE(State().status.find("Require Display At 09/11 12:34:56"), std::string::npos);
    EXPECT_NE(State().status.find("release denied"), std::string::npos);
    EXPECT_TRUE(platform.errors.empty());
    awake.Tick(platform.now);
    awake.Tick(platform.now);
    EXPECT_EQ(platform.power_calls.size(), 2U);
    const auto retry = awake.Toggle(AwakeMode::display);
    EXPECT_FALSE(retry.success);
    EXPECT_EQ(retry.error, "Failed to stop stay awake: release denied");
    EXPECT_EQ(State().remaining, "Release failed");
    platform.release_result = {};
    EXPECT_TRUE(awake.Toggle(AwakeMode::display).success);
    EXPECT_EQ(State().remaining, "Not Enabled");
    EXPECT_TRUE(State().duration_enabled);
    EXPECT_NE(State().status.find("Ended At"), std::string::npos);
}

TEST_F(AwakeControllerTest, ManualReleaseFailureDoesNotClaimProtectionEnded)
{
    Start(AwakeMode::system);
    platform.release_result = {false, "retry needed"};
    const auto result = awake.Toggle(AwakeMode::system);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Failed to stop stay awake: retry needed");
    EXPECT_EQ(State().remaining, "Release failed");
    EXPECT_FALSE(awake.NeedsTimer());
    EXPECT_FALSE(State().display_enabled);
}
} // namespace
} // namespace stay_awake
