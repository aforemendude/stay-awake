#pragma once

#include "platform/windows/native.hpp"
#include "platform/windows/tray_icon.hpp"
#include "stay_awake/platform_binding.hpp"

#include <exception>
#include <vector>

namespace stay_awake::windows
{
class MainWindow
{
  public:
    explicit MainWindow(EventHandler handler);
    ~MainWindow();
    HWND Get() const;
    HICON SmallIcon() const;
    void SetTray(TrayIcon* tray);
    void ClearHandler();
    void Present(const ViewState& state);
    void SetVisible(bool visible);
    OperationResult SetTimerEnabled(bool enabled);
    std::exception_ptr Failure() const;
    bool PreTranslate(MSG& message);

  private:
    struct Control
    {
        HWND window;
        int x;
        int y;
        int width;
        int height;
    };

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept;
    LRESULT Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void Emit(ApplicationEvent event);
    void CreateControls();
    HWND Add(int id, const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int width, int height,
             DWORD ex_style = 0);
    void Layout(UINT dpi);
    void UpdateListExtent();
    void Command(int id, int notification);
    void Text(int id, std::string_view text);
    void Enable(int id, bool enabled);
    void SelectDuration(int id, const std::vector<DurationChoice>& choices,
                        std::optional<std::chrono::seconds> duration);

    EventHandler handler_;
    TrayIcon* tray_ = nullptr; // Borrowed; detached before the tray is destroyed.
    std::vector<Control> controls_;
    std::vector<DurationChoice> awake_durations_;
    std::vector<DurationChoice> close_durations_;
    std::vector<std::wstring> titles_;
    std::uint64_t catalog_revision_ = ~std::uint64_t{0};
    bool durations_loaded_ = false;
    bool rendering_ = false;
    bool timer_enabled_ = false;
    bool layout_active_ = false;
    UINT dpi_ = 96;
    HWND last_focus_ = nullptr; // Borrowed child HWND.
    std::exception_ptr failure_;
    UniqueFont font_;
    UniqueIcon small_icon_;
    UniqueIcon large_icon_;
    UniqueWindow window_; // Destroy children before their font/icons.
};
} // namespace stay_awake::windows
