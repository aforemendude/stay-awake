#include "platform/windows/windows_platform_binding.hpp"

#include "platform/windows/clock.hpp"
#include "platform/windows/window_catalog.hpp"

namespace stay_awake::windows
{
namespace
{
std::wstring FormatUnixProcessCreationTime(const std::optional<std::uint64_t> creation_time)
{
    if (!creation_time)
    {
        return L"Unavailable";
    }
    // FILETIME counts 100-nanosecond ticks since 1601; Unix time counts seconds since 1970.
    constexpr std::uint64_t ticks_per_second = 10'000'000;
    constexpr std::int64_t epoch_offset_seconds = 11'644'473'600;
    const auto unix_seconds = static_cast<std::int64_t>(*creation_time / ticks_per_second) - epoch_offset_seconds;
    return std::to_wstring(unix_seconds);
}

std::wstring FormatLocalProcessCreationTime(const std::optional<std::uint64_t> creation_time)
{
    if (!creation_time)
    {
        return L"Unavailable";
    }
    const FILETIME file_time{static_cast<DWORD>(*creation_time), static_cast<DWORD>(*creation_time >> 32)};
    SYSTEMTIME utc_time{}, local_time{};
    DYNAMIC_TIME_ZONE_INFORMATION time_zone{};
    if (!FileTimeToSystemTime(&file_time, &utc_time) ||
        GetDynamicTimeZoneInformation(&time_zone) == TIME_ZONE_ID_INVALID ||
        !SystemTimeToTzSpecificLocalTimeEx(&time_zone, &utc_time, &local_time))
    {
        return L"Unavailable";
    }
    const int date_size =
        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_LONGDATE, &local_time, nullptr, nullptr, 0, nullptr);
    const int time_size = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &local_time, nullptr, nullptr, 0);
    if (!date_size || !time_size)
    {
        return L"Unavailable";
    }
    std::wstring date(static_cast<std::size_t>(date_size), L'\0');
    std::wstring time(static_cast<std::size_t>(time_size), L'\0');
    const int date_count =
        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_LONGDATE, &local_time, nullptr, date.data(), date_size, nullptr);
    const int time_count = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &local_time, nullptr, time.data(), time_size);
    if (!date_count || !time_count)
    {
        return L"Unavailable";
    }
    date.resize(static_cast<std::size_t>(date_count - 1));
    time.resize(static_cast<std::size_t>(time_count - 1));
    return date + L" " + time;
}
} // namespace

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

void WindowsPlatformBinding::ShowWindowDetails(const WindowInfo& window)
{
    const auto text = L"Window Title: " + ToWide(window.title) + L"\r\nProcess Name: " +
                      ToWide(window.process_name.empty() ? "Unknown" : window.process_name) + L"\r\nProcess ID: " +
                      std::to_wstring(window.identity.process_id) + L"\r\nProcess Creation Time (Unix): " +
                      FormatUnixProcessCreationTime(window.identity.process_creation_time) +
                      L"\r\nProcess Creation Time (Local): " +
                      FormatLocalProcessCreationTime(window.identity.process_creation_time) + L"\r\nWindow Handle: 0x" +
                      ToWide(FormatHandle(window.identity));
    MessageBoxW(window_ ? window_->Get() : nullptr, text.c_str(), L"Window Details", MB_OK | MB_ICONINFORMATION);
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
