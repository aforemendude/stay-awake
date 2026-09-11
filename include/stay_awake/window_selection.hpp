#pragma once

#include "stay_awake/platform_binding.hpp"

namespace stay_awake
{
class WindowSelection
{
  public:
    explicit WindowSelection(PlatformBinding& platform);
    // Returns errors requiring a dialog; automatic catalog failures remain in State().catalog_status.
    OperationResult Refresh(bool user_initiated);
    OperationResult Select(std::optional<std::size_t> index);
    OperationResult ToggleHighlight();
    OperationResult ClearHighlight();
    std::optional<WindowInfo> SelectedWindow() const;
    const WindowSelectionViewState& State() const;

  private:
    OperationResult UpdateSelection();
    OperationResult UpdateOverlay();

    PlatformBinding& platform_;
    WindowSelectionViewState state_;
};
} // namespace stay_awake
