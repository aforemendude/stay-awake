#include "stay_awake/window_selection.hpp"

#include "fake_platform_binding.hpp"

#include <gtest/gtest.h>

namespace stay_awake
{
namespace
{
class WindowSelectionTest : public testing::Test
{
  protected:
    FakePlatformBinding platform;
    WindowSelection selection{platform};

    const WindowSelectionViewState& State() const
    {
        return selection.State();
    }
};

TEST_F(WindowSelectionTest, PreservesSuppliedOrderingDuplicateTitlesAndUnicodeMetadata)
{
    platform.catalog.windows[0].title = u8"窗口 — Café 🌙";
    platform.catalog.windows[0].process_name = u8"编辑器";
    EXPECT_TRUE(selection.Refresh(false).success);
    ASSERT_EQ(State().windows.size(), 2U);
    EXPECT_EQ(State().windows[0].title, u8"窗口 — Café 🌙");
    EXPECT_EQ(State().windows[1].identity, (WindowIdentity{0xDEF, 34}));
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_EQ(State().process_name, u8"编辑器");
    EXPECT_EQ(State().window_handle, "ABC");
    EXPECT_EQ(State().window_position, "X: -900, Y: -200, Width: 800, Height: 600");
    const auto revision = State().catalog_revision;
    platform.catalog.windows[0].title = "Same title";
    EXPECT_TRUE(selection.Refresh(true).success);
    EXPECT_EQ(State().catalog_revision, revision + 1);
    EXPECT_TRUE(selection.Select(1).success);
    ASSERT_TRUE(selection.SelectedWindow());
    EXPECT_EQ(selection.SelectedWindow()->identity, (WindowIdentity{0xDEF, 34}));
}

TEST_F(WindowSelectionTest, MissingMetadataUsesFallbackAndDeselectClearsDetails)
{
    platform.catalog.windows[0].process_name.clear();
    platform.rectangle.reset();
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_EQ(State().process_name, "Unknown");
    EXPECT_EQ(State().window_position, "Error getting position");
    EXPECT_TRUE(selection.Select(100).success);
    EXPECT_FALSE(State().selected_window);
    EXPECT_FALSE(selection.SelectedWindow());
    EXPECT_TRUE(State().process_name.empty());
    EXPECT_TRUE(State().window_handle.empty());
    EXPECT_TRUE(State().window_position.empty());
}

TEST_F(WindowSelectionTest, RefreshFailureClearsSelectionAndOverlayWithUserOnlyError)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    ASSERT_TRUE(State().overlay);
    platform.catalog.result = {false, "enumeration failed"};
    const auto result = selection.Refresh(true);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Failed to refresh windows list: enumeration failed");
    EXPECT_FALSE(State().selected_window);
    EXPECT_FALSE(State().overlay);
    EXPECT_FALSE(platform.overlay);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_TRUE(State().windows.empty());
    EXPECT_TRUE(State().window_handle.empty());
    EXPECT_EQ(State().catalog_status, result.error);
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_EQ(State().catalog_status, result.error);
}

TEST_F(WindowSelectionTest, HighlightIntentWorksWithoutSelectionAndFollowsSelectionSnapshots)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    EXPECT_TRUE(State().highlight_active);
    EXPECT_EQ(State().highlight_caption, "Stop Highlighting");
    EXPECT_FALSE(State().overlay);
    EXPECT_TRUE(selection.Select(0).success);
    ASSERT_TRUE(State().overlay);
    EXPECT_EQ(State().overlay->x, -900);
    platform.rectangle = Rectangle{500, 300, 100, 200};
    EXPECT_EQ(State().overlay->x, -900); // No live tracking selected.
    EXPECT_TRUE(selection.Select(1).success);
    EXPECT_EQ(State().overlay->x, 500);
    EXPECT_EQ(platform.geometry_targets.back(), (WindowIdentity{0xDEF, 34}));
    EXPECT_TRUE(selection.Select(std::nullopt).success);
    EXPECT_FALSE(State().overlay);
    EXPECT_TRUE(State().highlight_active);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_EQ(State().highlight_caption, "Highlight Window");
}

TEST_F(WindowSelectionTest, InvalidGeometrySuppressesPreviouslyVisibleOverlay)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    ASSERT_TRUE(platform.overlay);
    for (const auto rectangle :
         {std::optional<Rectangle>{}, std::optional(Rectangle{1, 2, 0, 20}), std::optional(Rectangle{1, 2, 20, -1})})
    {
        platform.rectangle = rectangle;
        EXPECT_TRUE(selection.Select(1).success);
        EXPECT_FALSE(State().overlay);
        EXPECT_FALSE(platform.overlay);
    }
}

TEST_F(WindowSelectionTest, OverlayFailureIsReportedAndResetsIntent)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.Select(0).success);
    platform.overlay_result = {false, "overlay unavailable"};
    const auto result = selection.ToggleHighlight();
    EXPECT_FALSE(State().highlight_active);
    EXPECT_FALSE(State().overlay);
    EXPECT_EQ(State().highlight_caption, "Highlight Window");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Failed to highlight window: overlay unavailable");
}
} // namespace
} // namespace stay_awake
