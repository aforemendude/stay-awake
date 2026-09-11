# Stay Awake

![Stay Awake Icon](assets/icon.png)

A lightweight Windows 11 x64 tray utility that keeps the system awake for a chosen duration and requests a selected
window to close after an independent countdown. The native executable includes its icon and needs no separately
installed application runtime or adjacent assets.

## Using Stay Awake

Launch `StayAwake.exe`; it starts hidden with both features inactive. Left-click the tray icon or choose **Show** from
its right-click menu to open the window. A second launch requests Show on the existing instance in the same interactive
session. Activation requires the same elevation and remains subject to Windows foreground restrictions. A launch during
initialization retries for up to two seconds; an error explains when the existing instance is unavailable.

Closing the window with **X** or **Alt-F4** hides it to the tray and stops highlighting. Both countdowns continue.
Ordinary minimize remains minimize. Use tray **Quit**, or **Quit Stay Awake** in the window's system menu, to exit and
release resources. The system-menu action also provides an exit if tray creation fails; in that case the window stays
available. Explorer restart restores the tray icon. Confirmed sign-out/shutdown releases resources; a canceled shutdown
leaves the app running.

### Sleep prevention

- **Require Display** keeps the system and display awake.
- **Require System** keeps the system awake while allowing the display to turn off.
- Choices run from 30 minutes through 8 hours in 15-minute increments, plus `00:00:10` in all builds. The default is 2
  hours. Only one awake mode can run at a time.
- The active button becomes **Stop Require Display** or **Stop Require System**. Click it to release protection early.
  Timed completion records the mode, outcome, and local time. If release fails, the active button remains available for
  manual retry and the result describes the error.

The app uses Windows execution-state requests to prevent idle sleep. These requests do not guarantee prevention of
screen locking, screensavers, or explicit user sleep. See Microsoft's
[SetThreadExecutionState contract](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate).

### Window Closer

Select a window, choose a duration, and click **Schedule Close Window**. Choices run from 15 minutes through 8 hours in
15-minute increments, plus `00:00:10` in all builds. The default is 1 hour. **Stop** cancels the schedule without
requesting a close. Sleep prevention and window-close scheduling run independently.

The list contains visible top-level windows with nonblank titles, ordered using Windows locale collation. The shell,
exact title `Program Manager`, and Stay Awake's own windows are excluded; ordinary Explorer windows remain eligible.
Duplicate titles are distinct selectable windows. **Refresh List** clears selection and highlighting. Showing the main
window refreshes automatically unless a close is scheduled.

Selected details show the process basename, uppercase hexadecimal window handle, and signed screen coordinates/size.
Unavailable metadata appears as `Unknown` or `Error getting position`. Read-only detail and result fields can be
scrolled and copied; the window list has horizontal scrolling for long titles. Tab, Shift-Tab, Enter/Space on buttons,
and native list/combo navigation support keyboard use; Ctrl+A and Ctrl+C select/copy read-only field contents.

At expiry the captured handle and owning process ID are revalidated, then one asynchronous `WM_CLOSE` is posted. **Close
requested** records the handle, process, and local time. The target can show a save prompt, ignore the request, or
remain unresponsive: this app does not kill processes or verify that the window closed. Missing targets, changed owners,
and denied requests produce a result describing the failure. Handle/PID checks cannot eliminate same-process handle
reuse or the race between validation and posting. See
[PostMessageW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew).

**Highlight Window** displays a red, approximately 25%-opaque, non-activating overlay that passes mouse clicks through.
It captures geometry on selection/toggle; it does not track later movement, resizing, or disappearance. Refresh,
deselection, invalid geometry, hiding, and exit remove the overlay. Highlight intent can be enabled without a selected
window and remains independent of scheduling.

### Time and display scaling

Both countdowns use monotonic elapsed time that includes suspend/hibernate. After resume, overdue operations run once on
the next timer callback. Manual clock, time-zone, and daylight-saving changes affect local result timestamps, not the
durations. Countdown text appears immediately and rounds positive fractional seconds up. The adapter uses biased
[Windows interrupt time](https://learn.microsoft.com/en-us/windows/win32/sysinfo/interrupt-time).

The window has a fixed 784 × 606 logical client area with Per-Monitor V2 DPI awareness; controls, fonts, and icons scale
at startup and when moving between monitors. The usable monitor work area must fit that scaled client area plus the
window frame. Small work areas are not handled by shrinking fonts or adding scrolling. Window metadata and overlay
bounds remain native screen pixels, including monitors left of or above the primary display.

## Releases

Existing published builds are available on the [Releases](https://github.com/aforemendude/stay-awake/releases) page.
This source migration does not publish a new release. Build the native executable below; Windows UI/manual acceptance
for this migration remains pending as recorded in [TODO.md](TODO.md#10-validate-builds-and-native-behavior).

## Build, test, and format

### Requirements

- Runtime platform: Windows 11 x64.
- Build hosts: Linux or Windows, CMake 3.28+, Ninja, and an x64 MinGW-w64 C++ toolchain with Windows 10 API declarations
  and `mincore` import-library support. Linux native tests additionally need a host C++17 compiler.
- Tests fetch GoogleTest at commit `063de7e9578f82b369302001269680b4b1553359` with a pinned SHA-256. The first
  Debug/test configuration needs network access to GitHub. Release builds disable tests and do not fetch GoogleTest.
- Formatting C++ requires clang-format (validated with 18). Markdown/JSON/JavaScript use the pinned Prettier dependency.
- Optional convenience scripts: Node.js 24.19.0+ and npm. Node is not needed for direct CMake build/test/install
  commands.

Linux package names commonly include `cmake`, `ninja-build`, `g++`, `g++-mingw-w64-x86-64`, `binutils-mingw-w64-x86-64`,
and `clang-format`. Ensure the packaged CMake meets the minimum version. Windows builds use a MinGW environment such as
the MSYS2 UCRT64 shell, with `gcc`, `g++`, `windres`, `cmake`, and `ninja` on PATH; install the matching x64 UCRT64
toolchain/CMake/Ninja packages. Keep tools from the same environment together. MSVC, ARM64, and macOS workflows are not
part of the supported matrix.

### npm shortcuts

```sh
npm ci
npm run build
npm run test
npm run format
npm run format:check
```

`build` builds Debug and Release Windows executables: Linux cross-compiles with MinGW-w64; Windows uses native MinGW.
`test` builds and runs the portable application tests on the current host (the Windows Debug preset also builds the
app). `build:debug` and `build:release` remain available for individual configurations. The dispatcher propagates
configuration/build/test failures.

`format` runs Prettier for maintained documentation/configuration and clang-format for C++. `format:check` verifies
both. Generated output and the three historical review documents are excluded.

### Direct CMake: Linux

```sh
# Portable core and native Linux tests; no Windows code is compiled.
cmake --preset linux-native-debug
cmake --build --preset linux-native-debug
ctest --preset linux-native-debug

# Windows Debug app and test executable, cross-compiled without running Windows binaries.
cmake --preset linux-mingw-debug
cmake --build --preset linux-mingw-debug

# Windows Release app, without downloading/building test dependencies.
cmake --preset linux-mingw-release
cmake --build --preset linux-mingw-release

cmake --build --preset linux-native-debug --target format
cmake --build --preset linux-native-debug --target format-check
```

### Direct CMake: Windows

Run these in the configured x64 MinGW environment:

```sh
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug

cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release

cmake --build --preset windows-mingw-debug --target format
cmake --build --preset windows-mingw-debug --target format-check
```

CTest presets exist only for native execution. Tests link the portable core and GoogleTest, with no Windows adapter,
desktop interaction, platform skips, or real sleeps. Cross-built test discovery is deferred until CTest runs on Windows.
To build only the portable core in a custom configuration, set `STAY_AWAKE_BUILD_APP=OFF`; requesting the app on a
non-Windows target produces a configure error. Linux has no GUI or power-management implementation.

### Output and installation

| Build host | Debug executable                          | Release executable                          |
| ---------- | ----------------------------------------- | ------------------------------------------- |
| Linux      | `build/linux-mingw-debug/StayAwake.exe`   | `build/linux-mingw-release/StayAwake.exe`   |
| Windows    | `build/windows-mingw-debug/StayAwake.exe` | `build/windows-mingw-release/StayAwake.exe` |

Copy the Release `StayAwake.exe` into any folder on a Windows x64 machine and launch it. Icons, manifest, and the MinGW
runtime are linked into the application; only Windows system DLLs are required. There is no console window or installer.
An optional CMake install rule copies the executable to `<prefix>/bin/StayAwake.exe`:

```sh
cmake --install build/linux-mingw-release --prefix ./build/install
# On Windows:
cmake --install build/windows-mingw-release --prefix ./build/install
```

## Contributor notes

The portable `Application` coordinates event delivery, lifecycle, the shared countdown timer, and presentation.
`AwakeController` owns sleep prevention and release retries; `CloseController` owns close scheduling and captured
targets; `WindowSelection` owns the catalog, selection details, and highlighting. Each class owns its feature state, and
`Application` composes their grouped `ViewState` snapshots for the Windows view. Matching test files exercise each
feature through a shared fake platform binding; application tests cover coordination, reentrant events, and shutdown.

See [AGENTS.md](AGENTS.md) for the architecture and application-only testing boundary, and [TODO.md](TODO.md) for the
migration decisions, recorded validation, deferred alternatives, and Windows manual acceptance matrix. The
`CODE_REVIEW_*.md` documents retain their original contents and source links as historical context for the prior
implementation.

Licensed under the [MIT License](LICENSE).
