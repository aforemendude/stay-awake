#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace stay_awake
{
using ElapsedTime = std::chrono::milliseconds;

enum class AwakeMode
{
    display,
    system,
};

struct OperationResult
{
    bool success = true;
    std::string error;
};

struct WindowIdentity
{
    std::uintptr_t handle = 0;
    std::uint32_t process_id = 0;
    // Opaque process-lifetime timestamp; absent when the platform cannot read it.
    std::optional<std::uint64_t> process_creation_time = std::nullopt;

    bool operator==(const WindowIdentity& other) const
    {
        return handle == other.handle && process_id == other.process_id &&
               process_creation_time == other.process_creation_time;
    }
};

struct WindowInfo
{
    WindowIdentity identity;
    std::string title;
    std::string process_name = "Unknown";
};

struct Rectangle
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct WindowListResult
{
    OperationResult result;
    std::vector<WindowInfo> windows;
};

struct DurationChoice
{
    std::chrono::seconds duration;
    std::string label;
};

std::vector<DurationChoice> MakeDurations(std::chrono::minutes first);
bool ValidDuration(const std::vector<DurationChoice>& choices, std::optional<std::chrono::seconds> duration);
std::string FormatRemaining(ElapsedTime remaining);
std::string FormatHandle(WindowIdentity identity);

struct AwakeViewState
{
    std::vector<DurationChoice> durations = MakeDurations(std::chrono::minutes(30));
    std::optional<std::chrono::seconds> duration = std::chrono::hours(2);
    bool duration_enabled = true;
    bool display_enabled = true;
    bool system_enabled = true;
    std::string display_caption = "Require Display";
    std::string system_caption = "Require System";
    std::string remaining = "Not Enabled";
    std::string caption = "Stay Awake";
    std::string status;
};

struct CloseViewState
{
    std::vector<DurationChoice> durations = MakeDurations(std::chrono::minutes(15));
    std::optional<std::chrono::seconds> duration = std::chrono::hours(1);
    bool inputs_enabled = true;
    std::string caption = "Schedule Close Window";
    std::string remaining = "Not Enabled";
    std::string group_caption = "Window Closer";
    std::string status;
};

struct WindowSelectionViewState
{
    bool highlight_active = false;
    std::string highlight_caption = "Highlight Window";
    std::string catalog_status;
    std::vector<WindowInfo> windows;
    std::uint64_t catalog_revision = 0;
    std::optional<std::size_t> selected_window;
    std::string process_name;
    std::string window_handle;
    std::string window_position;
    std::optional<Rectangle> overlay;
};

// Presentation snapshot composed from independently owned feature state.
struct ViewState
{
    AwakeViewState awake;
    CloseViewState close;
    WindowSelectionViewState selection;
    bool visible = false;
    bool stopped = false;
    bool timer_needed = false;
};

enum class EventKind
{
    initialized,
    show,
    hide,
    quit,
    session_end_confirmed,
    session_end_canceled,
    tick,
    toggle_display,
    toggle_system,
    awake_duration_changed,
    close_duration_changed,
    select_window,
    refresh,
    toggle_close,
    toggle_highlight,
};

struct ApplicationEvent
{
    EventKind kind;
    std::optional<std::size_t> index = std::nullopt;
    std::optional<std::chrono::seconds> duration = std::nullopt;
};
} // namespace stay_awake
