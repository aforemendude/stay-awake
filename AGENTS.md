# AGENTS.md

Stay Awake is a C++17 Windows 11 x64 tray utility for timed sleep prevention and scheduled window-close requests. CMake
exposes its portable controller as a library that can be built and tested natively on Linux and Windows.

## Repository Map

- `src/main.cpp` is the portable composition root: create the platform binding and run the controller.
- `include/stay_awake/application.hpp`, `application_state.hpp`, and `src/application/` own duration choices,
  independent deadlines, captured window identity, selection/highlight state, and user-visible results. All portable
  text is UTF-8; native handles cross this boundary only as opaque integer values.
- `include/stay_awake/platform_binding.hpp` defines native operations, injected clocks, presentation, and typed events;
  `platform_factory.hpp` declares the CMake-selected factory. The duration clock includes suspend/hibernate; the local
  clock is used only for completion timestamps.
- `src/platform/windows/` implements the binding through focused main-window, DPI, tray, single-instance, power,
  catalog, overlay, clock, and resource helpers. The main window owns control layout and notification routing. Expiry
  and scheduling decisions belong in the controller. No Win32 APIs, headers, types, or platform gates belong in the
  portable core or its tests.
- `stay_awake.rc` and `stay_awake.manifest` embed the preserved `assets/icon.ico`, version, common controls v6, and
  Per-Monitor V2 awareness. These resources attach directly to the GUI executable. Layout uses fixed logical units;
  target-window rectangles remain signed native screen pixels and must not be rescaled.
- `tests/application/` verifies controller behavior and durations using a fake binding, fake clocks, and fake native
  outcomes. The test executable links only `StayAwake::core` and GoogleTest.
- `CMakeLists.txt` defines the core, optional Windows adapter and GUI executable, pinned test dependency, warnings,
  install rule, and the maintained C++ formatting file list. Add new C++ files to that list.
- `CMakePresets.json` provides native Linux tests and Linux/Windows MinGW Debug/Release builds under `build/<preset>`.
- `cmake/toolchains/mingw-w64-x86_64.cmake` separates target library/include searches from host tools.
- `cmake/ClangFormat.cmake` provides `format` and `format-check` when clang-format is installed.
- `scripts/cmake.mjs` chooses host-appropriate build/test/format presets for the optional npm shortcuts.
- `README.md` is the user/contributor guide. `TODO.md` records migration choices, validation evidence, and pending
  Windows manual acceptance.
- `CODE_REVIEW_CORE.md`, `CODE_REVIEW_INTERFACE_TIMERS.md`, and `CODE_REVIEW_STARTUP_BUILD_DOCS.md` describe the former
  implementation. Preserve their contents and historical links; completing every finding is outside this port's scope.

## Testing Requirements

- Add only application unit tests that compile and run unchanged on both Linux and Windows. Test portable behavior
  through the binding fake; do not add Win32 unit tests, Windows headers, conditional skips, real sleeps, or desktop
  interaction to this suite.
- The dependency direction is executable → Windows adapter → portable core; tests → portable core only. Linux native
  configuration must not build the adapter. Cross-build tests use `DISCOVERY_MODE PRE_TEST` and must not execute during
  Linux builds. Run CTest only through the native-host presets.
- Verify platform behavior with the separate manual Windows matrix in `TODO.md`. A Linux cross-build does not prove UI,
  DPI, click-through, activation, or shutdown behavior. Leave unavailable checks pending.
- Controller events, presentation, power operations, and cleanup share one UI thread. `RunService` owns callback
  delivery; it sends Quit before teardown and detaches callbacks. Ordinary reentrant events are queued until the current
  transition/presentation finishes. Quit/confirmed session-end cleans up immediately, including in a modal loop. Never
  allow exceptions across a native callback or destructor.
- Preserve truthful operation outcomes: `Close requested` means a queued close message, not verified closure. Failed
  power release stays observable with a manual retry. A scheduled target must not be substituted by later list events.

## Common Commands

Run from the repository root; prefer npm shortcuts when Node is available:

```sh
npm ci
npm run build
npm run test
npm run format
npm run format:check
```

Direct Linux commands without Node:

```sh
cmake --preset linux-native-debug
cmake --build --preset linux-native-debug
ctest --preset linux-native-debug
cmake --preset linux-mingw-debug
cmake --build --preset linux-mingw-debug
cmake --preset linux-mingw-release
cmake --build --preset linux-mingw-release
cmake --build --preset linux-native-debug --target format-check
```

See [Build, test, and format](README.md#build-test-and-format) for requirements, Windows commands, and output paths.
Prettier formats maintained Markdown/JSON/JavaScript; historical review files and generated output are excluded.
