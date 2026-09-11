#include "platform/windows/single_instance.hpp"

namespace stay_awake::windows
{
namespace
{
// Unprefixed kernel object names are session-local.
constexpr auto mutex_name = L"StayAwake-Mutex-1927b19d-4cad-4589-9ed1-17ae32b96c1b";
constexpr auto event_name = L"StayAwake-Event-1927b19d-4cad-4589-9ed1-17ae32b96c1b";
} // namespace

SingleInstance::~SingleInstance()
{
    event_.Reset();
    if (owns_mutex_)
    {
        ReleaseMutex(mutex_.Get());
    }
}

bool SingleInstance::AcquireOrShow()
{
    mutex_.Reset(CreateMutexW(nullptr, TRUE, mutex_name));
    const auto error = GetLastError();
    if (!mutex_.Get())
    {
        throw std::runtime_error(
            NativeError("Create instance mutex; launch at the same elevation as Stay Awake", error));
    }
    owns_mutex_ = error != ERROR_ALREADY_EXISTS;
    const auto deadline = GetTickCount64() + 2000;
    while (!owns_mutex_)
    {
        const auto wait = WaitForSingleObject(mutex_.Get(), 0);
        if (wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED)
        {
            owns_mutex_ = true; // The former owner finished shutdown or crashed; initialize as the new owner.
            break;
        }
        Require(wait != WAIT_FAILED, "Wait for instance mutex");
        UniqueHandle receiver(OpenEventW(EVENT_MODIFY_STATE, FALSE, event_name));
        if (receiver.Get())
        {
            // Request foreground permission only for the existing application's window. Windows may still deny it.
            const auto window = FindWindowW(nullptr, L"Stay Awake");
            DWORD pid = 0;
            if (window && GetWindowThreadProcessId(window, &pid))
            {
                AllowSetForegroundWindow(pid);
            }
            Require(SetEvent(receiver.Get()) != FALSE, "Send Show to Stay Awake");
            return false;
        }
        const auto event_error = GetLastError();
        if (event_error != ERROR_FILE_NOT_FOUND)
        {
            throw std::runtime_error(NativeError("Open Show event; use the same elevation as Stay Awake", event_error));
        }
        if (GetTickCount64() >= deadline)
        {
            throw std::runtime_error("Stay Awake did not become ready within two seconds. Try launching again.");
        }
        Sleep(40);
    }
    event_.Reset(CreateEventW(nullptr, FALSE, FALSE, event_name));
    Require(event_.Get() != nullptr, "Create Show event");
    return true;
}

HANDLE SingleInstance::ShowEvent() const
{
    return event_.Get();
}
} // namespace stay_awake::windows
