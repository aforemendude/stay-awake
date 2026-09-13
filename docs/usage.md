# Usage details

See the [README](../README.md#using-stay-awake) for basic controls and [autostart setup](../README.md#run-at-startup).

## Tray and window

Launching again at the same elevation requests the existing window to show. Hiding the window stops highlighting but
leaves countdowns running. **Quit Stay Awake** in the window's system menu also works if the tray icon cannot be
created. The fixed-size window scales with display DPI and must fit the monitor's work area.

## Window selection and details

**Refresh List** clears selection and highlighting. Showing the main window refreshes the list unless a close is
scheduled.

**Show Details** displays the selected window's title, process name, process ID, process creation time, and window
handle from the last list refresh. **Copy** copies the details and closes the dialog; **OK** just closes it.

- **Process Creation Time (Unix):** seconds since January 1, 1970 UTC, with seven fractional digits (100-nanosecond
  precision).
- **Process Creation Time (Local):** a readable date and time using Windows regional and time-zone settings.

Unreadable creation times and failed local-time conversions appear as unavailable.

**Highlight Window** captures the selected window's position and size in a red, click-through overlay. It does not track
movement, resizing, or disappearance; toggle it to update the overlay. Windows with unreadable process identity cannot
be highlighted.

## Close requests

A close request fails if the target's identity cannot be verified or Windows denies access. Even a successful request
only queues a message; the target may remain open.

Targeting is best effort:
[reused window handles](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow) or a window
replaced during the request can cause a different window to receive it, even after checking process identity.

## Status and timing

**Status** shows **Ready** when idle with no previous result or issue, the remaining countdown while active, and the
last completion or error until the next run. The status below the list reports refresh errors and returns to **Ready**
after a successful refresh. Status timestamps use the same Windows regional long-date and time format as **Process
Creation Time (Local)**.

Countdowns include sleep and hibernation; overdue operations run after resume. Clock and time-zone changes affect result
timestamps, not durations.

Sleep prevention uses
[Windows power requests](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate)
to prevent idle sleep. They do not guarantee prevention of screen locking, screensavers, or explicitly requested sleep.
