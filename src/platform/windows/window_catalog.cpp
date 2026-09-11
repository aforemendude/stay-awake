#include "platform/windows/window_catalog.hpp"

#include "platform/windows/native.hpp"

#include <algorithm>
#include <exception>
#include <vector>

namespace stay_awake::windows
{
namespace
{
std::optional<std::uint64_t> ProcessCreationTime(const HANDLE process)
{
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!process || !GetProcessTimes(process, &creation, &exit, &kernel, &user))
    {
        return std::nullopt;
    }
    return (static_cast<std::uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime;
}

std::string ProcessName(const HANDLE process)
{
    if (!process)
    {
        return "Unknown";
    }
    std::wstring path(32768, L'\0');
    DWORD count = static_cast<DWORD>(path.size());
    if (!QueryFullProcessImageNameW(process, 0, path.data(), &count))
    {
        return "Unknown";
    }
    path.resize(count);
    const auto slash = path.find_last_of(L"\\/");
    auto name = path.substr(slash == std::wstring::npos ? 0 : slash + 1);
    const auto dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos)
    {
        name.resize(dot);
    }
    return name.empty() ? "Unknown" : ToUtf8(name);
}

struct NativeEntry
{
    std::wstring title;
    WindowInfo window;
};

struct Enumeration
{
    HWND shell = GetShellWindow();
    DWORD own_pid = GetCurrentProcessId();
    std::vector<NativeEntry> entries;
    std::exception_ptr exception;
};

BOOL CALLBACK CollectWindow(HWND window, LPARAM parameter) noexcept
{
    auto& enumeration = *reinterpret_cast<Enumeration*>(parameter);
    try
    {
        if (window == enumeration.shell || !IsWindowVisible(window))
        {
            return TRUE;
        }
        DWORD pid = 0;
        if (!GetWindowThreadProcessId(window, &pid) || !pid || pid == enumeration.own_pid)
        {
            return TRUE;
        }
        const int length = GetWindowTextLengthW(window);
        if (length <= 0)
        {
            return TRUE;
        }
        std::wstring title(static_cast<std::size_t>(length) + 1, L'\0');
        const int count = GetWindowTextW(window, title.data(), static_cast<int>(title.size()));
        if (count <= 0)
        {
            return TRUE;
        }
        title.resize(count);
        if (title == L"Program Manager")
        {
            return TRUE;
        }
        std::vector<WORD> types(title.size());
        const bool classified = GetStringTypeW(CT_CTYPE1, title.data(), count, types.data()) != FALSE;
        if (classified && std::all_of(types.begin(), types.end(), [](WORD type) { return (type & C1_SPACE) != 0; }))
        {
            return TRUE;
        }
        UniqueHandle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        WindowInfo info{{reinterpret_cast<std::uintptr_t>(window), pid, ProcessCreationTime(process.Get())},
                        ToUtf8(title),
                        ProcessName(process.Get())};
        enumeration.entries.push_back({std::move(title), std::move(info)});
        return TRUE;
    }
    catch (...)
    {
        enumeration.exception = std::current_exception();
        return FALSE;
    }
}

OperationResult Validate(const WindowIdentity identity)
{
    const auto window = NativeWindow(identity);
    DWORD pid = 0;
    if (!IsWindow(window) || !GetWindowThreadProcessId(window, &pid))
    {
        return {false, "Target window no longer exists"};
    }
    if (!identity.process_id || pid != identity.process_id)
    {
        return {false, "Target window owner changed"};
    }
    if (!identity.process_creation_time)
    {
        return {false, "Target window process creation time unavailable"};
    }
    UniqueHandle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
    if (!process.Get())
    {
        return {false, NativeError("OpenProcess")};
    }
    const auto creation_time = ProcessCreationTime(process.Get());
    if (!creation_time)
    {
        return {false, NativeError("GetProcessTimes")};
    }
    if (creation_time != identity.process_creation_time)
    {
        return {false, "Target window process changed"};
    }
    return {};
}
} // namespace

WindowListResult EnumerateWindows()
{
    Enumeration enumeration;
    SetLastError(ERROR_SUCCESS);
    const bool succeeded = EnumWindows(CollectWindow, reinterpret_cast<LPARAM>(&enumeration)) != FALSE;
    const DWORD error = GetLastError();
    if (enumeration.exception)
    {
        std::rethrow_exception(enumeration.exception);
    }
    if (!succeeded)
    {
        return {{false, NativeError("EnumWindows", error)}, {}};
    }
    std::stable_sort(enumeration.entries.begin(), enumeration.entries.end(), [](const auto& left, const auto& right) {
        const int comparison =
            CompareStringEx(LOCALE_NAME_USER_DEFAULT, 0, left.title.data(), static_cast<int>(left.title.size()),
                            right.title.data(), static_cast<int>(right.title.size()), nullptr, nullptr, 0);
        return comparison ? comparison == CSTR_LESS_THAN : left.title < right.title;
    });
    WindowListResult result;
    for (auto& entry : enumeration.entries)
    {
        result.windows.push_back(std::move(entry.window));
    }
    return result;
}

std::optional<Rectangle> WindowRectangle(const WindowIdentity identity)
{
    if (!Validate(identity).success)
    {
        return std::nullopt;
    }
    RECT rectangle{};
    if (!GetWindowRect(NativeWindow(identity), &rectangle))
    {
        return std::nullopt;
    }
    return Rectangle{static_cast<int>(rectangle.left), static_cast<int>(rectangle.top),
                     static_cast<int>(rectangle.right - rectangle.left),
                     static_cast<int>(rectangle.bottom - rectangle.top)};
}

OperationResult RequestClose(const WindowIdentity identity)
{
    const auto result = Validate(identity);
    if (!result.success)
    {
        return result;
    }
    // Handle, PID, and process creation time provide best-effort targeting, not a correctness guarantee:
    // same-process HWND reuse and replacement between validation and posting remain possible.
    if (!PostMessageW(NativeWindow(identity), WM_CLOSE, 0, 0))
    {
        return {false, NativeError("PostMessage(WM_CLOSE)")};
    }
    return {};
}
} // namespace stay_awake::windows
