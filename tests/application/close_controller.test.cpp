#include "stay_awake/close_controller.hpp"

#include "fake_platform_binding.hpp"

#include <gtest/gtest.h>

namespace stay_awake
{
namespace
{
using namespace std::chrono_literals;

class CloseControllerTest : public testing::Test
{
  protected:
    FakePlatformBinding platform;
    CloseController close{platform};
    WindowInfo target{{0xABC, 12, 0x100000001ULL}, "Same title", "alpha"};

    void Start()
    {
        close.SetDuration(10s);
        ASSERT_TRUE(close.Toggle(target).success);
    }
    CloseViewState State() const
    {
        return close.State(platform.now);
    }
};

TEST_F(CloseControllerTest, DefaultsToAnIdleOneHourScheduleWithFifteenMinuteChoices)
{
    EXPECT_EQ(State().duration, 1h);
    ASSERT_EQ(State().durations.size(), 33U);
    EXPECT_EQ(State().durations.front().duration, 15min);
    EXPECT_EQ(State().durations.front().label, "00:15:00");
    EXPECT_EQ(State().status, "Ready");
    EXPECT_EQ(State().caption, "Schedule Close Window");
    EXPECT_TRUE(State().inputs_enabled);
    EXPECT_FALSE(close.Active());
    EXPECT_EQ(close.NextUpdate(platform.now), std::nullopt);
    EXPECT_TRUE(platform.close_requests.empty());
}

TEST_F(CloseControllerTest, NoSelectionReportsExactErrorAndDoesNotSchedule)
{
    const auto result = close.Toggle(std::nullopt);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "No window selected.");
    EXPECT_EQ(State().status, result.error);
    EXPECT_TRUE(State().inputs_enabled);
    EXPECT_FALSE(close.Active());
}

TEST_F(CloseControllerTest, RejectsMissingOrInvalidDurationsWithoutStarting)
{
    for (const auto invalid : {std::optional<std::chrono::seconds>{}, std::optional(0s), std::optional(-1s),
                               std::optional(11s), std::optional<std::chrono::seconds>(9h)})
    {
        close.SetDuration(invalid);
        const auto result = close.Toggle(target);
        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.error, "Select a valid close duration.");
        EXPECT_EQ(State().status, result.error);
        EXPECT_FALSE(close.Active());
        EXPECT_TRUE(State().inputs_enabled);
    }
    EXPECT_TRUE(platform.close_requests.empty());
}

TEST_F(CloseControllerTest, RestartAfterCancellationUsesNewDeadline)
{
    Start();
    platform.now = 9s;
    EXPECT_TRUE(close.Toggle(std::nullopt).success); // Cancellation does not need a target.
    EXPECT_FALSE(close.Active());
    EXPECT_TRUE(State().inputs_enabled);
    EXPECT_EQ(State().caption, "Schedule Close Window");
    EXPECT_EQ(State().status, "Ready");
    EXPECT_TRUE(close.Toggle(target).success);
    platform.now = 10s;
    EXPECT_FALSE(close.Tick(platform.now));
    EXPECT_TRUE(platform.close_requests.empty());
    EXPECT_EQ(State().status, "Close scheduled - 00:00:09 remaining");
    platform.now = 19s;
    EXPECT_TRUE(close.Tick(platform.now));
    EXPECT_EQ(platform.close_requests, (std::vector<WindowIdentity>{target.identity}));
}

TEST_F(CloseControllerTest, CapturesTargetAndConsumesScheduleOnceAtExactDeadline)
{
    Start();
    EXPECT_FALSE(State().inputs_enabled);
    EXPECT_EQ(State().caption, "Stop");
    target = {{0xDEF, 34, 0x200000002ULL}, "Same title", "beta"};
    close.SetDuration(1h);
    EXPECT_EQ(State().duration, 10s);
    platform.now = 9999ms;
    EXPECT_FALSE(close.Tick(platform.now));
    EXPECT_EQ(State().status, "Close scheduled - 00:00:01 remaining");
    EXPECT_TRUE(platform.close_requests.empty());
    platform.now = 10s;
    EXPECT_TRUE(close.Tick(platform.now));
    ASSERT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.close_requests[0], (WindowIdentity{0xABC, 12, 0x100000001ULL}));
    EXPECT_EQ(State().status, "Close requested ABC at Friday, September 11, 2026 12:34:56 PM (alpha)");
    EXPECT_FALSE(close.Active());
    EXPECT_FALSE(close.Tick(platform.now));
    EXPECT_EQ(platform.close_requests.size(), 1U);
    Start();
    EXPECT_EQ(State().status, "Close scheduled - 00:00:10 remaining");
}

TEST_F(CloseControllerTest, CapturesProcessLifetimeWhenHandleAndPidAreReused)
{
    Start();
    target.identity.process_creation_time = 0x200000001ULL;
    platform.now = 10s;
    EXPECT_TRUE(close.Tick(platform.now));
    ASSERT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.close_requests[0], (WindowIdentity{0xABC, 12, 0x100000001ULL}));
    EXPECT_FALSE(platform.close_requests[0] == target.identity);
    target.identity.process_creation_time.reset();
    EXPECT_FALSE(platform.close_requests[0] == target.identity);
}

TEST_F(CloseControllerTest, PreservesAnUnavailableProcessLifetimeInTheCapturedTarget)
{
    target.identity.process_creation_time.reset();
    Start();
    target.identity.process_creation_time = 0x200000002ULL;
    EXPECT_TRUE(close.Tick(10s));
    EXPECT_EQ(platform.close_requests, (std::vector<WindowIdentity>{{0xABC, 12, std::nullopt}}));
}

TEST_F(CloseControllerTest, FailedRequestUsesCapturedMetadataAndDoesNotRetry)
{
    for (const auto* failure :
         {"Target window no longer exists", "Target window owner changed", "Target window process changed",
          "Target window process creation time unavailable", "access denied"})
    {
        SCOPED_TRACE(failure);
        platform.close_requests.clear();
        Start();
        platform.close_result = {false, failure};
        platform.now += 10s;
        EXPECT_TRUE(close.Tick(platform.now));
        EXPECT_EQ(State().status,
                  std::string("Close request failed ABC at Friday, September 11, 2026 12:34:56 PM (alpha): ") +
                      failure);
        EXPECT_TRUE(State().inputs_enabled);
        EXPECT_FALSE(close.Active());
        EXPECT_FALSE(close.Tick(platform.now));
        EXPECT_EQ(platform.close_requests, (std::vector<WindowIdentity>{target.identity}));
        EXPECT_TRUE(platform.errors.empty());
    }
}

TEST_F(CloseControllerTest, MissingProcessNameUsesFallback)
{
    target.process_name.clear();
    Start();
    platform.now = 10s;
    EXPECT_TRUE(close.Tick(platform.now));
    EXPECT_EQ(State().status, "Close requested ABC at Friday, September 11, 2026 12:34:56 PM (Unknown)");
    EXPECT_EQ(platform.close_requests, (std::vector<WindowIdentity>{target.identity}));
}

TEST_F(CloseControllerTest, NextUpdateUsesTheCapturedDeadlineAndDisappearsAfterExpiry)
{
    platform.now = 350ms;
    Start();
    EXPECT_EQ(close.NextUpdate(350ms), 1350ms);
    EXPECT_EQ(close.NextUpdate(1350ms), 2350ms);
    EXPECT_EQ(close.NextUpdate(10349ms), 10350ms);
    EXPECT_EQ(close.NextUpdate(10350ms), 10350ms);
    EXPECT_EQ(close.NextUpdate(11s), 11s);
    EXPECT_TRUE(close.Tick(11s));
    EXPECT_EQ(close.NextUpdate(11s), std::nullopt);
    EXPECT_EQ(platform.close_requests, (std::vector<WindowIdentity>{target.identity}));
}

TEST_F(CloseControllerTest, CancelDropsTheScheduleWithoutRequestingClose)
{
    Start();
    close.Cancel();
    close.Cancel();
    EXPECT_FALSE(close.Active());
    EXPECT_EQ(close.NextUpdate(platform.now), std::nullopt);
    EXPECT_TRUE(State().inputs_enabled);
    EXPECT_EQ(State().caption, "Schedule Close Window");
    EXPECT_EQ(State().status, "Ready");
    EXPECT_FALSE(close.Tick(1h));
    EXPECT_TRUE(platform.close_requests.empty());
}

TEST_F(CloseControllerTest, TimerFailureCancelsOnlyAnActiveScheduleAndAllowsRestart)
{
    close.CancelForTimerFailure("unused");
    EXPECT_EQ(State().status, "Ready");
    Start();
    close.CancelForTimerFailure("timer unavailable");
    EXPECT_EQ(State().status, "Schedule canceled: timer unavailable");
    EXPECT_TRUE(State().inputs_enabled);
    EXPECT_EQ(State().caption, "Schedule Close Window");
    EXPECT_FALSE(close.Active());
    EXPECT_EQ(close.NextUpdate(platform.now), std::nullopt);
    close.CancelForTimerFailure("another failure");
    EXPECT_FALSE(close.Tick(1h));
    EXPECT_EQ(State().status, "Schedule canceled: timer unavailable");
    EXPECT_TRUE(platform.close_requests.empty());
    platform.now = 20s;
    Start();
    EXPECT_EQ(State().status, "Close scheduled - 00:00:10 remaining");
    EXPECT_EQ(close.NextUpdate(platform.now), 21s);
    EXPECT_FALSE(close.Tick(29999ms));
    EXPECT_TRUE(close.Tick(30s));
    EXPECT_EQ(platform.close_requests, (std::vector<WindowIdentity>{target.identity}));
}

TEST_F(CloseControllerTest, ConsumesTheScheduleBeforeRequestCloseCanReenter)
{
    Start();
    bool reentered = false;
    platform.on_close = [&] {
        // Guard the callback so a regression reports a duplicate request rather than recursing indefinitely.
        if (reentered)
        {
            return;
        }
        reentered = true;
        EXPECT_FALSE(close.Active());
        EXPECT_EQ(close.NextUpdate(10s), std::nullopt);
        EXPECT_TRUE(close.State(10s).inputs_enabled);
        EXPECT_FALSE(close.Tick(10s));
    };
    EXPECT_TRUE(close.Tick(10s));
    EXPECT_TRUE(reentered);
    EXPECT_EQ(platform.close_requests, (std::vector<WindowIdentity>{target.identity}));
}
} // namespace
} // namespace stay_awake
