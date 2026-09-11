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
    WindowInfo target{{0xABC, 12}, "Same title", "alpha"};

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

TEST_F(CloseControllerTest, NoSelectionReportsExactErrorAndDoesNotSchedule)
{
    const auto result = close.Toggle(std::nullopt);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "No window selected.");
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
    EXPECT_EQ(State().remaining, "Not Enabled");
    EXPECT_TRUE(close.Toggle(target).success);
    platform.now = 10s;
    EXPECT_FALSE(close.Tick(platform.now));
    EXPECT_TRUE(platform.close_requests.empty());
    EXPECT_EQ(State().remaining, "00:00:09");
    platform.now = 19s;
    EXPECT_TRUE(close.Tick(platform.now));
    EXPECT_EQ(platform.close_requests.size(), 1U);
}

TEST_F(CloseControllerTest, CapturesTargetAndConsumesScheduleOnceAtExactDeadline)
{
    Start();
    EXPECT_FALSE(State().inputs_enabled);
    EXPECT_EQ(State().caption, "Stop");
    target = {{0xDEF, 34}, "Same title", "beta"};
    close.SetDuration(1h);
    EXPECT_EQ(State().duration, 10s);
    platform.now = 9999ms;
    EXPECT_FALSE(close.Tick(platform.now));
    EXPECT_EQ(State().remaining, "00:00:01");
    EXPECT_TRUE(platform.close_requests.empty());
    platform.now = 10s;
    EXPECT_TRUE(close.Tick(platform.now));
    ASSERT_EQ(platform.close_requests.size(), 1U);
    EXPECT_EQ(platform.close_requests[0], (WindowIdentity{0xABC, 12}));
    EXPECT_EQ(State().status, "Close requested ABC At 09/11 12:34 (alpha)");
    EXPECT_EQ(State().status.find("Closed"), std::string::npos);
    EXPECT_FALSE(close.Active());
    EXPECT_FALSE(close.Tick(platform.now));
    EXPECT_EQ(platform.close_requests.size(), 1U);
    Start();
    EXPECT_TRUE(State().status.empty());
    EXPECT_EQ(State().group_caption, "Window Closer");
}

TEST_F(CloseControllerTest, FailedRequestUsesCapturedMetadataAndDoesNotRetry)
{
    for (const auto* failure : {"Target window no longer exists", "Target window owner changed", "access denied"})
    {
        Start();
        platform.close_result = {false, failure};
        platform.now += 10s;
        EXPECT_TRUE(close.Tick(platform.now));
        EXPECT_NE(State().status.find("Close request failed ABC At 09/11 12:34 (alpha)"), std::string::npos);
        EXPECT_NE(State().status.find(failure), std::string::npos);
        EXPECT_TRUE(State().inputs_enabled);
        EXPECT_FALSE(close.Active());
        EXPECT_FALSE(close.Tick(platform.now));
        EXPECT_TRUE(platform.errors.empty());
    }
    EXPECT_EQ(platform.close_requests.size(), 3U);
}

TEST_F(CloseControllerTest, MissingProcessNameUsesFallback)
{
    target.process_name.clear();
    Start();
    platform.now = 10s;
    EXPECT_TRUE(close.Tick(platform.now));
    EXPECT_EQ(State().status, "Close requested ABC At 09/11 12:34 (Unknown)");
}
} // namespace
} // namespace stay_awake
