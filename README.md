# Stay Awake

![Stay Awake Icon](assets/icon.svg)

A Windows 11 x64 tray utility that keeps the system awake and requests a selected window to close, each on its own
countdown.

Download `StayAwake.exe` from [Releases](https://github.com/aforemendude/stay-awake/releases). The app needs no
installer, adjacent assets, or separately installed runtime.

## Using Stay Awake

Launch the app, then left-click the tray icon or choose **Show** from its menu. Both features start inactive.

- **Stay Awake:** choose a duration and click **Require Display** to keep the system and display awake, or **Require
  System** to allow the display to turn off. Click **Stop** to end it early. If release fails, the error stays visible
  and **Stop** lets you retry. This prevents idle sleep; screen locking, screensavers, and manually requested sleep may
  still occur.
- **Window Closer:** select a window, choose a duration, and click **Schedule Close Window**. **Stop** cancels the
  schedule. **Show Details** identifies the selection; **Highlight Window** marks its position with a click-through
  overlay. **Close requested** means a message was queued: the target may prompt to save, ignore it, or remain
  unresponsive. Stay Awake does not force termination or verify closure.

**X** and **Alt-F4** hide the window; countdowns continue. Exit with tray **Quit** or **Quit Stay Awake** in the
window's system menu. Countdowns include sleep and hibernation; overdue operations run after resume.

See [usage details](docs/usage.md) for window targeting, highlighting, and status behavior.

## Run at startup

Keep `StayAwake.exe` in a permanent location. Press **Win+R**, enter `shell:startup`, and place a shortcut to the
executable in the folder that opens (`%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup`).

The app will start in the tray when you sign in; start countdowns manually. Remove the shortcut to disable autostart.

## Build, test, and format

Install the [prerequisites](docs/development.md#prerequisites), then run from the repository root:

```sh
npm ci
npm run build
npm run test
npm run format
npm run format:check
```

`build` creates Debug and Release Windows executables at `build/<preset>/StayAwake.exe`, cross-compiling on Linux.
`test` runs portable tests on the current host. Formatting uses Prettier and clang-format.

See [development](docs/development.md) for direct CMake commands and icon updates,
[release size and validation](docs/release.md) for binary checks, and [AGENTS.md](AGENTS.md) for contribution
constraints.

Licensed under the [MIT License](LICENSE).
