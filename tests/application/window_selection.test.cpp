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

    void ExpectOverlay(const Rectangle& expected) const
    {
        ASSERT_TRUE(State().overlay);
        ASSERT_TRUE(platform.overlay);
        for (const auto& actual : {*State().overlay, *platform.overlay})
        {
            EXPECT_EQ(actual.x, expected.x);
            EXPECT_EQ(actual.y, expected.y);
            EXPECT_EQ(actual.width, expected.width);
            EXPECT_EQ(actual.height, expected.height);
        }
    }
};

TEST_F(WindowSelectionTest, StartsWithEmptySelectionAndNoCatalogOrGeometryQueries)
{
    EXPECT_TRUE(State().windows.empty());
    EXPECT_EQ(State().catalog_revision, 0U);
    EXPECT_EQ(State().catalog_status, "Ready");
    EXPECT_EQ(State().selected_window, std::nullopt);
    EXPECT_EQ(selection.SelectedWindow(), std::nullopt);
    EXPECT_TRUE(State().process_name.empty());
    EXPECT_TRUE(State().window_position.empty());
    EXPECT_FALSE(State().highlight_active);
    EXPECT_EQ(State().highlight_caption, "Highlight Window");
    EXPECT_EQ(State().overlay, std::nullopt);
    EXPECT_EQ(platform.refreshes, 0);
    EXPECT_TRUE(platform.geometry_targets.empty());
}

TEST_F(WindowSelectionTest, PreservesSuppliedOrderingDuplicateTitlesAndUnicodeMetadata)
{
    platform.catalog.windows[0].title = u8"窗口 — Café 🌙";
    platform.catalog.windows[0].process_name = u8"编辑器";
    EXPECT_TRUE(selection.Refresh(false).success);
    ASSERT_EQ(State().windows.size(), 2U);
    EXPECT_EQ(State().windows[0].identity, (WindowIdentity{0xABC, 12, 0x100000001ULL}));
    EXPECT_EQ(State().windows[0].title, u8"窗口 — Café 🌙");
    EXPECT_EQ(State().windows[1].identity, (WindowIdentity{0xDEF, 34, 0x200000002ULL}));
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_EQ(State().process_name, u8"编辑器");
    EXPECT_EQ(State().window_position, "X: -900, Y: -200, Width: 800, Height: 600");
    const auto revision = State().catalog_revision;
    platform.catalog.windows[0].title = "Same title";
    EXPECT_TRUE(selection.Refresh(true).success);
    EXPECT_EQ(State().catalog_revision, revision + 1);
    ASSERT_EQ(State().windows.size(), 2U);
    EXPECT_EQ(State().windows[0].title, "Same title");
    EXPECT_EQ(State().windows[1].title, "Same title");
    EXPECT_EQ(State().windows[0].identity, (WindowIdentity{0xABC, 12, 0x100000001ULL}));
    EXPECT_EQ(State().windows[1].identity, (WindowIdentity{0xDEF, 34, 0x200000002ULL}));
    EXPECT_TRUE(selection.Select(1).success);
    ASSERT_TRUE(selection.SelectedWindow());
    EXPECT_EQ(selection.SelectedWindow()->identity, (WindowIdentity{0xDEF, 34, 0x200000002ULL}));
    EXPECT_EQ(selection.SelectedWindow()->title, "Same title");
    EXPECT_EQ(selection.SelectedWindow()->process_name, "beta");
    EXPECT_EQ(platform.geometry_targets,
              (std::vector<WindowIdentity>{{0xABC, 12, 0x100000001ULL}, {0xDEF, 34, 0x200000002ULL}}));
}

TEST_F(WindowSelectionTest, MissingMetadataUsesFallbackAndDeselectClearsDetails)
{
    platform.catalog.windows[0].process_name.clear();
    platform.catalog.windows[0].identity.process_creation_time.reset();
    platform.rectangle.reset();
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_EQ(State().process_name, "Unknown");
    ASSERT_TRUE(selection.SelectedWindow());
    EXPECT_FALSE(selection.SelectedWindow()->identity.process_creation_time);
    EXPECT_EQ(State().window_position, "Error getting position");
    EXPECT_TRUE(selection.Select(State().windows.size()).success);
    EXPECT_FALSE(State().selected_window);
    EXPECT_FALSE(selection.SelectedWindow());
    EXPECT_TRUE(State().process_name.empty());
    EXPECT_TRUE(State().window_position.empty());
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_TRUE(selection.Select(std::nullopt).success);
    EXPECT_EQ(selection.SelectedWindow(), std::nullopt);
    EXPECT_TRUE(State().process_name.empty());
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
    EXPECT_EQ(State().catalog_status, result.error);
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_EQ(State().catalog_status, result.error);
    platform.catalog.result = {};
    EXPECT_TRUE(selection.Refresh(true).success);
    EXPECT_EQ(State().catalog_status, "Ready");
    EXPECT_EQ(platform.refreshes, 4);
    EXPECT_EQ(State().catalog_revision, 4U);
    ASSERT_EQ(State().windows.size(), 2U);
    EXPECT_EQ(State().windows[0].identity, (WindowIdentity{0xABC, 12, 0x100000001ULL}));
    EXPECT_EQ(State().windows[1].identity, (WindowIdentity{0xDEF, 34, 0x200000002ULL}));
}

TEST_F(WindowSelectionTest, HighlightIntentWorksWithoutSelectionAndFollowsSelectionSnapshots)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    EXPECT_TRUE(State().highlight_active);
    EXPECT_EQ(State().highlight_caption, "Stop Highlighting");
    EXPECT_FALSE(State().overlay);
    EXPECT_TRUE(selection.Select(0).success);
    ExpectOverlay({-900, -200, 800, 600});
    platform.rectangle = Rectangle{500, 300, 100, 200};
    ExpectOverlay({-900, -200, 800, 600}); // No live tracking selected.
    EXPECT_TRUE(selection.Select(1).success);
    ExpectOverlay({500, 300, 100, 200});
    EXPECT_EQ(platform.geometry_targets,
              (std::vector<WindowIdentity>{{0xABC, 12, 0x100000001ULL}, {0xDEF, 34, 0x200000002ULL}}));
    EXPECT_TRUE(selection.Select(std::nullopt).success);
    EXPECT_FALSE(State().overlay);
    EXPECT_FALSE(platform.overlay);
    EXPECT_TRUE(State().highlight_active);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_EQ(State().highlight_caption, "Highlight Window");
}

TEST_F(WindowSelectionTest, InvalidGeometrySuppressesPreviouslyVisibleOverlay)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    for (const auto rectangle :
         {std::optional<Rectangle>{}, std::optional(Rectangle{1, 2, 0, 20}), std::optional(Rectangle{1, 2, -1, 20}),
          std::optional(Rectangle{1, 2, 20, 0}), std::optional(Rectangle{1, 2, 20, -1})})
    {
        platform.rectangle = Rectangle{-900, -200, 800, 600};
        EXPECT_TRUE(selection.Select(0).success);
        ExpectOverlay({-900, -200, 800, 600});
        platform.rectangle = rectangle;
        EXPECT_TRUE(selection.Select(1).success);
        EXPECT_FALSE(State().overlay);
        EXPECT_FALSE(platform.overlay);
        EXPECT_TRUE(State().highlight_active);
        EXPECT_EQ(State().highlight_caption, "Stop Highlighting");
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
    EXPECT_FALSE(platform.overlay);
    platform.overlay_result = {};
    EXPECT_TRUE(selection.ToggleHighlight().success);
    EXPECT_TRUE(State().highlight_active);
    ExpectOverlay({-900, -200, 800, 600});
}

TEST_F(WindowSelectionTest, ClearHighlightPreservesSelectionAndDetails)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    ExpectOverlay({-900, -200, 800, 600});
    const auto geometry_targets = platform.geometry_targets;
    EXPECT_TRUE(selection.ClearHighlight().success);
    EXPECT_TRUE(selection.ClearHighlight().success);
    EXPECT_FALSE(State().overlay);
    EXPECT_FALSE(platform.overlay);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_EQ(State().highlight_caption, "Highlight Window");
    EXPECT_EQ(State().selected_window, 0U);
    ASSERT_TRUE(selection.SelectedWindow());
    EXPECT_EQ(selection.SelectedWindow()->identity, (WindowIdentity{0xABC, 12, 0x100000001ULL}));
    EXPECT_EQ(State().process_name, "alpha");
    EXPECT_EQ(State().window_position, "X: -900, Y: -200, Width: 800, Height: 600");
    EXPECT_EQ(platform.geometry_targets, geometry_targets);
}

TEST_F(WindowSelectionTest, EmptySuccessfulRefreshClearsThePreviousCatalogAndSelection)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    platform.catalog.windows.clear();
    EXPECT_TRUE(selection.Refresh(true).success);
    EXPECT_EQ(State().catalog_status, "Ready");
    EXPECT_EQ(State().catalog_revision, 2U);
    EXPECT_TRUE(State().windows.empty());
    EXPECT_EQ(selection.SelectedWindow(), std::nullopt);
    EXPECT_TRUE(State().process_name.empty());
    EXPECT_TRUE(State().window_position.empty());
    EXPECT_FALSE(State().overlay);
    EXPECT_FALSE(platform.overlay);
    EXPECT_FALSE(State().highlight_active);
}

TEST_F(WindowSelectionTest, ReportsOverlayRemovalFailureAndAllowsRetry)
{
    EXPECT_TRUE(selection.Refresh(false).success);
    EXPECT_TRUE(selection.Select(0).success);
    EXPECT_TRUE(selection.ToggleHighlight().success);
    platform.overlay_clear_result = {false, "overlay removal failed"};
    const auto result = selection.ClearHighlight();
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Failed to highlight window: overlay removal failed");
    EXPECT_FALSE(State().overlay);
    EXPECT_FALSE(State().highlight_active);
    EXPECT_EQ(State().highlight_caption, "Highlight Window");
    platform.overlay_clear_result = {};
    EXPECT_TRUE(selection.ClearHighlight().success);
    EXPECT_FALSE(platform.overlay);
}

TEST_F(WindowSelectionTest, RefreshReportsOverlayFailureEvenWhenTheCatalogSucceeds)
{
    platform.overlay_clear_result = {false, "overlay removal failed"};
    const auto result = selection.Refresh(false);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Failed to highlight window: overlay removal failed");
    EXPECT_EQ(State().catalog_status, "Ready");
    EXPECT_EQ(State().catalog_revision, 1U);
    ASSERT_EQ(State().windows.size(), 2U);
    EXPECT_EQ(State().windows[0].identity, (WindowIdentity{0xABC, 12, 0x100000001ULL}));
    EXPECT_EQ(State().windows[1].identity, (WindowIdentity{0xDEF, 34, 0x200000002ULL}));
}

TEST_F(WindowSelectionTest, UserRefreshPrioritizesCatalogFailureWhileAutomaticRefreshReportsOverlayFailure)
{
    platform.overlay_clear_result = {false, "overlay removal failed"};
    platform.catalog.result = {false, "enumeration failed"};
    const auto automatic = selection.Refresh(false);
    EXPECT_FALSE(automatic.success);
    EXPECT_EQ(automatic.error, "Failed to highlight window: overlay removal failed");
    EXPECT_EQ(State().catalog_status, "Failed to refresh windows list: enumeration failed");
    const auto user = selection.Refresh(true);
    EXPECT_FALSE(user.success);
    EXPECT_EQ(user.error, "Failed to refresh windows list: enumeration failed");
    EXPECT_EQ(State().catalog_status, user.error);
    EXPECT_TRUE(State().windows.empty());
    EXPECT_EQ(State().catalog_revision, 2U);
}
} // namespace
} // namespace stay_awake
