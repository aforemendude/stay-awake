#include "platform/windows/windows_platform_binding.hpp"

#include "platform/windows/clock.hpp"
#include "platform/windows/window_catalog.hpp"

namespace stay_awake::windows
{
OperationResult WindowsPlatformBinding::RunService(EventHandler handler)
{
    OperationResult result;
    bool initialized = false;
    try
    {
        // The embedded manifest establishes Per-Monitor V2 before any HWND is created.
        if (!instance_.AcquireOrShow())
        {
            return {};
        }
        window_ = std::make_unique<MainWindow>(handler);
        tray_ = std::make_unique<TrayIcon>(window_->Get(), window_->SmallIcon());
        window_->SetTray(tray_.get());
        const bool tray_ready = tray_->Add();
        initialized = true;
        handler({EventKind::initialized});
        if (!tray_ready)
        {
            handler({EventKind::show});
            ShowError("Unable to create the tray icon. Use Quit Stay Awake in the window's system menu to exit.");
        }
        const HANDLE event = instance_.ShowEvent();
        while (!exiting_)
        {
            const auto wait = MsgWaitForMultipleObjectsEx(1, &event, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            Require(wait != WAIT_FAILED, "Wait for application events");
            if (wait == WAIT_OBJECT_0)
            {
                handler({EventKind::show});
            }
            MSG message{};
            // Bound each drain so a busy queue cannot indefinitely starve a pending Show event.
            for (int count = 0; count < 128 && !exiting_ && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++count)
            {
                if (message.message == WM_QUIT)
                {
                    exiting_ = true;
                    break;
                }
                if (!window_->PreTranslate(message))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                if (window_->Failure())
                {
                    std::rethrow_exception(window_->Failure());
                }
            }
        }
        if (window_->Failure())
        {
            std::rethrow_exception(window_->Failure());
        }
    }
    catch (const std::exception& error)
    {
        result = {false, error.what()};
    }
    catch (...)
    {
        result = {false, "Unexpected native application error"};
    }
    if (initialized)
    {
        try
        {
            handler({EventKind::quit});
        }
        catch (...)
        {
            // Last-resort native teardown still runs, even if controller/presentation allocation failed.
        }
    }
    Cleanup();
    return result;
}

void WindowsPlatformBinding::Cleanup() noexcept
{
    exiting_ = true;
    if (window_)
    {
        window_->ClearHandler();
        window_->SetTray(nullptr);
        (void)window_->SetTimerEnabled(false);
    }
    power_.Reset();
    (void)overlay_.Set(std::nullopt);
    tray_.reset();
    window_.reset();
    // The power owner retries failed release during binding destruction, before instance ownership is released.
}

ElapsedTime WindowsPlatformBinding::Now()
{
    return InterruptTime();
}

std::string WindowsPlatformBinding::LocalTimestamp(const bool with_seconds)
{
    return windows::LocalTimestamp(with_seconds);
}

OperationResult WindowsPlatformBinding::SetAwake(const std::optional<AwakeMode> mode)
{
    return power_.Set(mode);
}

WindowListResult WindowsPlatformBinding::EnumerateWindows()
{
    return windows::EnumerateWindows();
}

std::optional<Rectangle> WindowsPlatformBinding::WindowRectangle(const WindowIdentity target)
{
    return windows::WindowRectangle(target);
}

OperationResult WindowsPlatformBinding::RequestClose(const WindowIdentity target)
{
    return windows::RequestClose(target);
}

OperationResult WindowsPlatformBinding::SetOverlay(const std::optional<Rectangle> rectangle)
{
    return overlay_.Set(rectangle);
}

OperationResult WindowsPlatformBinding::SetTimerEnabled(const bool enabled)
{
    return window_ ? window_->SetTimerEnabled(enabled) : OperationResult{};
}

void WindowsPlatformBinding::Present(const ViewState& state)
{
    if (window_)
    {
        window_->Present(state);
    }
}

void WindowsPlatformBinding::SetWindowVisible(const bool visible)
{
    if (window_ && !exiting_)
    {
        window_->SetVisible(visible);
    }
}

void WindowsPlatformBinding::ShowError(const std::string_view message)
{
    const auto text = ToWide(message);
    MessageBoxW(window_ ? window_->Get() : nullptr, text.c_str(), L"Stay Awake - Error", MB_OK | MB_ICONERROR);
}

void WindowsPlatformBinding::RequestExit()
{
    exiting_ = true;
    // WM_ENDSESSION(TRUE) may be followed immediately by OS process teardown. Release important resources before
    // returning from that callback, even inside a modal loop. The main HWND survives until RunService unwinds.
    if (window_)
    {
        (void)window_->SetTimerEnabled(false);
        window_->SetTray(nullptr);
    }
    power_.Reset();
    (void)overlay_.Set(std::nullopt);
    if (tray_)
    {
        tray_->Remove();
    }
    PostQuitMessage(0);
}
} // namespace stay_awake::windows
