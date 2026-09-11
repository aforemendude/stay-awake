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

    bool operator==(const WindowIdentity& other) const
    {
        return handle == other.handle && process_id == other.process_id;
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
std::string FormatRemaining(ElapsedTime remaining);
std::string FormatHandle(WindowIdentity identity);

struct ViewState
{
    std::vector<DurationChoice> awake_durations = MakeDurations(std::chrono::minutes(30));
    std::vector<DurationChoice> close_durations = MakeDurations(std::chrono::minutes(15));
    std::optional<std::chrono::seconds> awake_duration = std::chrono::hours(2);
    std::optional<std::chrono::seconds> close_duration = std::chrono::hours(1);
    bool visible = false;
    bool stopped = false;
    bool timer_needed = false;
    bool awake_duration_enabled = true;
    bool display_enabled = true;
    bool system_enabled = true;
    bool close_inputs_enabled = true;
    bool highlight_active = false;
    std::string display_caption = "Require Display";
    std::string system_caption = "Require System";
    std::string close_caption = "Schedule Close Window";
    std::string highlight_caption = "Highlight Window";
    std::string awake_remaining = "Not Enabled";
    std::string close_remaining = "Not Enabled";
    std::string awake_caption = "Stay Awake";
    std::string close_group_caption = "Window Closer";
    std::string awake_status;
    std::string close_status;
    std::string catalog_status;
    std::vector<WindowInfo> windows;
    std::uint64_t catalog_revision = 0;
    std::optional<std::size_t> selected_window;
    std::string process_name;
    std::string window_handle;
    std::string window_position;
    std::optional<Rectangle> overlay;
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
