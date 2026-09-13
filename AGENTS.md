# AGENTS.md

Stay Awake is a C++17 Windows 11 x64 tray utility with a portable core and tests for Linux and Windows.

## Development constraints

- Keep scheduling and feature state in `src/application/`; native operations and UI rendering belong in
  `src/platform/windows/`, behind `include/stay_awake/platform_binding.hpp`. No Win32 APIs, headers, types, or platform
  gates belong in the core or its tests. Portable text is UTF-8; native handles cross the boundary as opaque integers.
- Preserve the dependency direction: executable → Windows adapter → portable core; tests → core and GoogleTest only.
  Linux native builds must not compile the adapter.
- Events, presentation, power operations, and cleanup share one UI thread. `RunService` delivers quit before teardown
  and detaches callbacks. Queue ordinary reentrant events until the current transition/presentation finishes; quit and
  confirmed session-end clean up immediately, including in modal loops. No exceptions may escape native callbacks or
  destructors.
- Use the injected monotonic clock, including suspend/hibernate, for durations; local time is only for result
  timestamps. Keep captured scheduled targets stable across list events. `Close requested` means a queued message, not
  verified closure. Failed power release must remain visible with a manual retry.
- Scale UI layout in logical units; target rectangles remain signed native screen pixels and must not be rescaled.
- Keep distribution as a single executable that runs on Windows 11 x64 without separately installed runtimes or adjacent
  assets. Preserve MinGW static linking (`-static -static-libgcc -static-libstdc++`); only Windows-provided DLL
  dependencies are acceptable.
- Preserve size optimization for `Release` and `MinSizeRel` across the core, Windows adapter, and executable: `-Os`,
  function/data sections, checked LTO support, unused-section removal, and symbol stripping. Match the LTO optimization
  level at compile and link time. Keep `Debug` and `RelWithDebInfo` debugging information intact.
- Use integer conversions for simple numeric formatting; avoid pulling stream/locale machinery into the application for
  countdowns or hexadecimal handles. Preserve formatting behavior, exceptions, RTTI, embedded resources, and executable
  security settings when reducing size.
- Add new C++ files to `STAY_AWAKE_FORMAT_FILES` in `CMakeLists.txt`.

## Validation

- Add portable application tests through `tests/application/fake_platform_binding.hpp`. They must run unchanged on Linux
  and Windows, without Win32 code, conditional skips, real sleeps, or desktop interaction.
- Run CTest only with native-host presets. Keep cross-build test discovery at `DISCOVERY_MODE PRE_TEST` so Linux builds
  never execute Windows binaries.
- For changes likely to affect binary size (build/link settings, dependencies, formatting, or resources), measure the
  Release executable before and after with the same toolchain and configuration. Report byte counts and explain
  increases. The [README measurements](README.md#release-size-and-runtime-dependencies) are reference values, not a
  portable size limit.
- For those changes, inspect the resulting Release executable's DLL imports and embedded resources. Verify that no
  separately installed runtime is required (including `libgcc`, `libstdc++`, or `libwinpthread` DLLs), that the icon,
  manifest, and version resources remain embedded, and that ASLR/NX settings remain enabled. Windows system DLLs such as
  `msvcrt.dll` and OS-provided UCRT API sets are expected; static-link flags alone are not sufficient verification.
- Manually verify affected Windows behavior: tray/activation, DPI, overlay click-through, power, suspend/resume, and
  shutdown. Cross-compilation does not validate these; report unavailable checks as pending.
- Prefer `npm run build`, `npm run test`, and `npm run format:check`; use `npm run format` to apply formatting. See
  [README.md](README.md#build-test-and-format) for prerequisites and direct CMake commands.
