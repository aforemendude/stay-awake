# Stay Awake

![Stay Awake Icon](assets/icon.png)

A Windows 11 x64 tray utility that keeps the system awake for a chosen duration and requests a selected window to close
after an independent countdown.

Published builds are available on the [Releases](https://github.com/aforemendude/stay-awake/releases) page and may
differ from the current source described below. To build this version, see
[Build, test, and format](#build-test-and-format).

## Using Stay Awake

Launch `StayAwake.exe`; it starts in the tray with both features inactive. Left-click the tray icon or choose **Show**
from its right-click menu to open the window. Launching again at the same elevation requests the existing window to
show.

**X** and **Alt-F4** hide the window and stop highlighting; both countdowns continue. To exit, choose tray **Quit** or
**Quit Stay Awake** in the window's system menu. The latter also works if the tray icon cannot be created.

### Sleep prevention

Choose a duration, then start one mode:

- **Require Display** keeps the system and display awake.
- **Require System** keeps the system awake while allowing the display to turn off.

Click the active mode's **Stop** button to end it early. If releasing sleep prevention fails, the result shows the error
and the button remains available to retry.

These
[Windows power requests](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate)
prevent idle sleep; they do not guarantee prevention of screen locking, screensavers, or sleep explicitly requested by
the user.

### Window Closer

Select a window, choose a duration, and click **Schedule Close Window**. **Stop** cancels the schedule. **Refresh List**
clears selection and highlighting; showing the main window refreshes the list automatically unless a close is scheduled.

**Highlight Window** marks the selected window with a red overlay that lets clicks pass through. It captures the
window's current position and size without following later movement, resizing, or disappearance; toggle it to update the
overlay.

**Close requested** means a close message was queued. The target may show a save prompt, ignore the request, or remain
unresponsive; Stay Awake does not force termination or verify closure. If the target's identity cannot be verified or
Windows denies access, the request fails. Windows whose process identity cannot be read cannot be highlighted either.

Targeting is best effort:
[reused window handles](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow) or a window
replaced during the request can cause a different window to receive it, even after checking process identity.

Both countdowns include time spent asleep or hibernating; overdue operations run after resume. Clock and time-zone
changes affect result timestamps, not durations. The fixed-size window scales with display DPI and must fit the
monitor's work area.

## Build, test, and format

Run commands from the repository root. Build hosts are Linux or Windows; the GUI runs only on Windows.

- **Build tools:** CMake 3.28+, Ninja, and a C++17 compiler. Windows app builds need an x64 MinGW-w64 toolchain with
  Windows 10 APIs and `mincore` support. Linux tests use the host compiler.
- **Windows setup:** use an MSYS2 UCRT64 environment with matching `gcc`, `g++`, `windres`, `cmake`, and `ninja` on
  PATH.
- **Optional tools:** Node.js and npm for shortcuts (see the [Node version requirement](package.json)); clang-format for
  C++ formatting. Prettier is installed by `npm ci`.

Debug/test configurations download pinned GoogleTest sources from GitHub on first configuration. Release presets disable
tests and do not fetch GoogleTest.

### npm shortcuts

```sh
npm ci
npm run build
npm run test
npm run format
npm run format:check
```

`build` produces Debug and Release Windows executables, cross-compiling on Linux. `test` builds and runs the portable
tests on the current host. `format` and `format:check` run Prettier for documentation/configuration and clang-format for
C++; generated build output and dependencies are ignored.

### Direct CMake

Choose a preset from [CMakePresets.json](CMakePresets.json):

| Host    | Preset                  | Builds                         |
| ------- | ----------------------- | ------------------------------ |
| Linux   | `linux-native-debug`    | Portable core and native tests |
| Linux   | `linux-mingw-debug`     | Windows app and tests          |
| Linux   | `linux-mingw-release`   | Windows app                    |
| Windows | `windows-mingw-debug`   | Windows app and native tests   |
| Windows | `windows-mingw-release` | Windows app                    |

Use that preset for both configuration and build. For example, to cross-compile the Release app on Linux:

```sh
cmake --preset linux-mingw-release
cmake --build --preset linux-mingw-release
```

After building a native test preset, run `ctest --preset linux-native-debug` on Linux or
`ctest --preset windows-mingw-debug` on Windows. Run cross-built Windows tests on Windows. For C++ formatting, append
`--target format` or `--target format-check` to the build command; clang-format must be installed before configuration.

Executables are written to `build/<preset>/StayAwake.exe`. Copy the Release executable to a Windows 11 x64 machine and
launch it; icons and the MinGW runtime are embedded, so no adjacent assets or separately installed runtime are needed.

See [AGENTS.md](AGENTS.md) for development constraints and Windows validation guidance.

Licensed under the [MIT License](LICENSE).
