#include "stay_awake/application_state.hpp"

#include <gtest/gtest.h>

#include <cstdio>

namespace stay_awake
{
namespace
{
using namespace std::chrono_literals;

TEST(DurationsTest, PreservesEveryChoiceAndDefaultInBothLists)
{
    const ViewState state;
    for (const auto first : {15min, 30min})
    {
        const auto choices = MakeDurations(first);
        ASSERT_EQ(choices.size(), first == 15min ? 33U : 32U);
        EXPECT_EQ(choices.front().duration, first);
        EXPECT_EQ(choices[choices.size() - 2].duration, 8h);
        for (std::size_t index = 0; index + 1 < choices.size(); ++index)
        {
            EXPECT_EQ(choices[index].duration, first + 15min * index);
            const auto seconds = choices[index].duration.count();
            char label[20]{};
            std::snprintf(label, sizeof(label), "%02d:%02d:00", static_cast<int>(seconds / 3600),
                          static_cast<int>(seconds / 60 % 60));
            EXPECT_EQ(choices[index].label, label);
        }
        EXPECT_EQ(choices.back().duration, 10s);
        EXPECT_EQ(choices.back().label, "00:00:10");
    }
    EXPECT_EQ(state.awake_duration, 2h);
    EXPECT_EQ(state.close_duration, 1h);
    EXPECT_EQ(state.awake_durations.front().label, "00:30:00");
    EXPECT_EQ(state.close_durations.front().label, "00:15:00");
}

TEST(DurationsTest, RoundsPositiveFractionsUpAndClampsExpiredCountdowns)
{
    EXPECT_EQ(FormatRemaining(1ms), "00:00:01");
    EXPECT_EQ(FormatRemaining(999ms), "00:00:01");
    EXPECT_EQ(FormatRemaining(1000ms), "00:00:01");
    EXPECT_EQ(FormatRemaining(1001ms), "00:00:02");
    EXPECT_EQ(FormatRemaining(3600001ms), "01:00:01");
    EXPECT_EQ(FormatRemaining(8h), "08:00:00");
    EXPECT_EQ(FormatRemaining(0ms), "00:00:00");
    EXPECT_EQ(FormatRemaining(-10ms), "00:00:00");
}

TEST(DurationsTest, FormatsOpaqueHandlesAsUppercaseHex)
{
    EXPECT_EQ(FormatHandle({0xABCDEF, 1}), "ABCDEF");
    EXPECT_EQ(FormatHandle({0, 1}), "0");
}
} // namespace
} // namespace stay_awake
