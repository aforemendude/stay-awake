# AGENTS.md

Stay Awake is a C++17 Windows 11 x64 tray utility with a portable core and Linux/Windows tests.

## Development

- Keep scheduling and feature state in `src/application/`; native operations and UI belong in `src/platform/windows/`
  behind `include/stay_awake/platform_binding.hpp`. Core and tests must have no Win32 APIs, headers, types, or platform
  gates. Use UTF-8 text and opaque integer handles across the boundary.
- Dependencies: executable → Windows adapter → core; tests → core and GoogleTest only. Exclude the adapter from native
  Linux builds.
- Keep events, presentation, power, and cleanup on one UI thread. Queue ordinary reentrant events until transitions and
  presentation finish; quit and confirmed session-end clean up immediately, including in modal loops. `RunService`
  delivers quit before teardown and detaches callbacks. Never let exceptions escape native callbacks or destructors.
- Use the injected monotonic clock, including suspend/hibernate, for durations; local time only for result timestamps.
  Preserve captured scheduled targets across list events. `Close requested` means queued, not verified. Keep failed
  power release visible with manual retry.
- Scale layout in logical units; never rescale signed native screen-pixel target rectangles.
- Ship one executable with only Windows-provided DLL dependencies. Preserve static linking, size optimizations, debug
  information in debugging builds, and integer-based numeric formatting as described in
  [release guidance](docs/release.md).
- Add new C++ files to `STAY_AWAKE_FORMAT_FILES` in `CMakeLists.txt`.
- Update documentation with behavior, workflow, and build changes. Keep README focused on common tasks; put useful
  reference details in `docs/`. Keep prose concise, avoid duplication, and update affected links.

## Validation

- Use `tests/application/fake_platform_binding.hpp` for portable application tests that run unchanged on Linux and
  Windows: no Win32 code, conditional skips, real sleeps, or desktop interaction.
- Run CTest only with native-host presets; retain cross-build `DISCOVERY_MODE PRE_TEST`.
- For size-affecting changes, follow [release validation](docs/release.md#validation): compare same-toolchain Release
  byte counts and inspect DLL imports, embedded resources, and ASLR/NX.
- Manually check affected Windows behavior: tray/activation, DPI, overlay click-through, power, suspend/resume, and
  shutdown. Report unavailable checks as pending; cross-compilation does not validate them.
- Prefer `npm run build`, `npm run test`, and `npm run format:check`; apply formatting with `npm run format`. See
  [development](docs/development.md) for prerequisites and direct CMake commands.
