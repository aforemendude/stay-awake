#include "platform/windows/windows_platform_binding.hpp"

#include "platform/windows/clock.hpp"
#include "platform/windows/window_catalog.hpp"

#include <commctrl.h>
#include <cstring>

namespace stay_awake::windows
{
namespace
{
OperationResult CopyTextToClipboard(HWND owner, const std::wstring& text)
{
    const auto bytes = (text.size() + 1) * sizeof(wchar_t);
    UniqueResource<HGLOBAL, GlobalFree> memory(GlobalAlloc(GMEM_MOVEABLE, bytes));
    if (!memory.Get())
    {
        return {false, NativeError("Allocate clipboard text")};
    }
    auto* destination = GlobalLock(memory.Get());
    if (!destination)
    {
        return {false, NativeError("Lock clipboard text")};
    }
    std::memcpy(destination, text.c_str(), bytes);
    GlobalUnlock(memory.Get());

    if (!OpenClipboard(owner))
    {
        return {false, NativeError("Open clipboard")};
    }
    const bool copied = EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, memory.Get()) != nullptr;
    const auto error = GetLastError();
    if (copied)
    {
        // Windows owns the allocation only after SetClipboardData succeeds.
        (void)memory.Release();
    }
    CloseClipboard();
    return copied ? OperationResult{} : OperationResult{false, NativeError("Copy text to clipboard", error)};
}

std::wstring FormatUnixProcessCreationTime(const std::optional<std::uint64_t> creation_time)
{
    if (!creation_time)
    {
        return L"Unavailable";
    }
    // FILETIME counts 100-nanosecond ticks since 1601; Unix time counts seconds since 1970.
    constexpr std::uint64_t ticks_per_second = 10'000'000;
    constexpr std::uint64_t epoch_offset_ticks = 116'444'736'000'000'000;
    const bool before_unix_epoch = *creation_time < epoch_offset_ticks;
    const auto unix_ticks =
        before_unix_epoch ? epoch_offset_ticks - *creation_time : *creation_time - epoch_offset_ticks;
    const auto fraction = std::to_wstring(unix_ticks % ticks_per_second);
    return (before_unix_epoch ? L"-" : L"") + std::to_wstring(unix_ticks / ticks_per_second) + L"." +
           std::wstring(7 - fraction.size(), L'0') + fraction;
}

std::wstring FormatLocalTimestamp(const SYSTEMTIME& local_time)
{
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
    return FormatLocalTimestamp(local_time);
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
        (void)window_->ScheduleTick(std::nullopt);
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

std::string WindowsPlatformBinding::LocalTimestamp()
{
    SYSTEMTIME time{};
    GetLocalTime(&time);
    return ToUtf8(FormatLocalTimestamp(time));
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

OperationResult WindowsPlatformBinding::ScheduleTick(const std::optional<ElapsedTime> deadline)
{
    return window_ ? window_->ScheduleTick(deadline) : OperationResult{};
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
    ShowCopyableMessage(L"Window Details", text);
}

void WindowsPlatformBinding::ShowStatus(const std::string_view title, const std::string_view message)
{
    ShowCopyableMessage(ToWide(title).c_str(), ToWide(message));
}

void WindowsPlatformBinding::ShowCopyableMessage(const wchar_t* title, const std::wstring& text)
{
    if (exiting_)
    {
        return;
    }
    constexpr int copy_button = 100;
    const TASKDIALOG_BUTTON buttons[] = {{copy_button, L"&Copy"}, {IDOK, L"OK"}};
    TASKDIALOGCONFIG dialog{};
    dialog.cbSize = sizeof(dialog);
    dialog.hwndParent = window_ ? window_->Get() : nullptr;
    dialog.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT;
    dialog.pszWindowTitle = title;
    dialog.pszMainIcon = TD_INFORMATION_ICON;
    dialog.pszContent = text.c_str();
    dialog.cButtons = static_cast<UINT>(std::size(buttons));
    dialog.pButtons = buttons;
    dialog.nDefaultButton = IDOK;
    int pressed_button = 0;
    const auto result = TaskDialogIndirect(&dialog, &pressed_button, nullptr, nullptr);
    if (exiting_)
    {
        return;
    }
    if (FAILED(result))
    {
        ShowError(NativeError("Show message dialog", static_cast<DWORD>(result)));
    }
    else if (pressed_button == copy_button)
    {
        const auto copied = CopyTextToClipboard(dialog.hwndParent, text);
        if (!copied.success)
        {
            ShowError(copied.error);
        }
    }
}

void WindowsPlatformBinding::RequestExit()
{
    exiting_ = true;
    // WM_ENDSESSION(TRUE) may be followed immediately by OS process teardown. Release important resources before
    // returning from that callback, even inside a modal loop. The main HWND survives until RunService unwinds.
    if (window_)
    {
        (void)window_->ScheduleTick(std::nullopt);
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
