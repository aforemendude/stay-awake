#include "stay_awake/window_selection.hpp"

#include <utility>

namespace stay_awake
{
WindowSelection::WindowSelection(PlatformBinding& platform) : platform_(platform)
{
}

OperationResult WindowSelection::Refresh(const bool user_initiated)
{
    auto result = ClearHighlight();
    state_.windows.clear();
    ++state_.catalog_revision;
    const auto selection = Select(std::nullopt);
    if (!selection.success)
    {
        result = selection;
    }
    auto catalog = platform_.EnumerateWindows();
    state_.catalog_status = catalog.result.success ? "" : "Failed to refresh windows list: " + catalog.result.error;
    if (catalog.result.success)
    {
        state_.windows = std::move(catalog.windows);
    }
    else if (user_initiated)
    {
        return {false, state_.catalog_status};
    }
    return result;
}

OperationResult WindowSelection::Select(const std::optional<std::size_t> index)
{
    state_.selected_window = index && *index < state_.windows.size() ? index : std::nullopt;
    return UpdateSelection();
}

OperationResult WindowSelection::ToggleHighlight()
{
    state_.highlight_active = !state_.highlight_active;
    return UpdateSelection();
}

OperationResult WindowSelection::ClearHighlight()
{
    state_.highlight_active = false;
    state_.overlay.reset();
    return UpdateOverlay();
}

std::optional<WindowInfo> WindowSelection::SelectedWindow() const
{
    return state_.selected_window ? std::optional(state_.windows[*state_.selected_window]) : std::nullopt;
}

const WindowSelectionViewState& WindowSelection::State() const
{
    return state_;
}

OperationResult WindowSelection::UpdateSelection()
{
    state_.process_name.clear();
    state_.window_handle.clear();
    state_.window_position.clear();
    state_.overlay.reset();
    if (state_.selected_window)
    {
        const auto& window = state_.windows[*state_.selected_window];
        state_.process_name = window.process_name.empty() ? "Unknown" : window.process_name;
        state_.window_handle = FormatHandle(window.identity);
        const auto rect = platform_.WindowRectangle(window.identity);
        if (rect)
        {
            state_.window_position = "X: " + std::to_string(rect->x) + ", Y: " + std::to_string(rect->y) +
                                     ", Width: " + std::to_string(rect->width) +
                                     ", Height: " + std::to_string(rect->height);
            if (state_.highlight_active && rect->width > 0 && rect->height > 0)
            {
                state_.overlay = rect;
            }
        }
        else
        {
            state_.window_position = "Error getting position";
        }
    }
    return UpdateOverlay();
}

OperationResult WindowSelection::UpdateOverlay()
{
    const auto result = platform_.SetOverlay(state_.overlay);
    if (!result.success)
    {
        state_.overlay.reset();
        state_.highlight_active = false;
    }
    state_.highlight_caption = state_.highlight_active ? "Stop Highlighting" : "Highlight Window";
    return result.success ? OperationResult{} : OperationResult{false, "Failed to highlight window: " + result.error};
}
} // namespace stay_awake
