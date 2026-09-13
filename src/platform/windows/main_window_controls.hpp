#pragma once

#include "platform/windows/list_scrolling.hpp"
#include "platform/windows/native.hpp"
#include "stay_awake/platform_binding.hpp"

#include <exception>
#include <vector>

namespace stay_awake::windows
{
// Owns child-control presentation; application decisions stay behind the event callback.
class MainWindowControls
{
  public:
    MainWindowControls(EventHandler emit, std::exception_ptr& failure);
    void Create(HWND window);
    bool HasControls() const noexcept;
    void Layout(UINT dpi);
    void UpdateListExtent();
    void Present(const ViewState& state);
    void Command(int id, int notification);

  private:
    struct Control
    {
        HWND window;
        int x;
        int y;
        int width;
        int height;
    };

    HWND Add(int id, const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int width, int height,
             DWORD ex_style = 0);
    ListScrolling& Scrolling(int id) noexcept;
    void Text(int id, std::string_view text);
    void Enable(int id, bool enabled);
    void SelectDuration(int id, const std::vector<DurationChoice>& choices,
                        std::optional<std::chrono::seconds> duration);

    EventHandler emit_;
    HWND window_ = nullptr; // Borrowed parent; destroyed before this component.
    std::vector<Control> controls_;
    std::vector<DurationChoice> awake_durations_;
    std::vector<DurationChoice> close_durations_;
    std::vector<std::wstring> titles_;
    std::uint64_t catalog_revision_ = ~std::uint64_t{0};
    bool durations_loaded_ = false;
    bool rendering_ = false;
    UINT dpi_ = 96;
    UniqueFont font_;
    ListScrolling window_list_scroll_;
    ListScrolling awake_duration_scroll_;
    ListScrolling close_duration_scroll_;
};
} // namespace stay_awake::windows
