#pragma once

#include "platform/windows/main_window_controls.hpp"
#include "platform/windows/native.hpp"
#include "platform/windows/tray_icon.hpp"
#include "stay_awake/platform_binding.hpp"

#include <exception>

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
    OperationResult ScheduleTick(std::optional<ElapsedTime> deadline);
    std::exception_ptr Failure() const;
    bool PreTranslate(MSG& message);

  private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept;
    LRESULT Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void Emit(ApplicationEvent event);
    void Layout(UINT dpi);
    void SaveFocus(HWND window);
    void RestoreFocus(HWND window);

    EventHandler handler_;
    TrayIcon* tray_ = nullptr; // Borrowed; detached before the tray is destroyed.
    std::optional<ElapsedTime> timer_deadline_;
    bool layout_active_ = false;
    HWND last_focus_ = nullptr; // Borrowed child HWND.
    std::exception_ptr failure_;
    MainWindowControls controls_;
    UniqueIcon small_icon_;
    UniqueIcon large_icon_;
    UniqueWindow window_; // Destroy children before callbacks, controls, fonts, icons, and failure storage.
};
} // namespace stay_awake::windows
