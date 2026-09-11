# Build and test infrastructure review

## Scope and basis

Reviewed the clean worktree at commit `b2cf743bec38653f9a0c47ea0cee585b01bd7791`: `CMakeLists.txt`,
`CMakePresets.json`, `cmake/`, `scripts/cmake.mjs`, npm manifests and lockfile, ignore/editor configuration,
`tests/CMakeLists.txt`, the portable fake binding as a test interface, and the build/setup instructions in `README.md`
and `AGENTS.md`. Ancillary license metadata and formatter configuration were inspected for setup context, without
reviewing formatting choices. Review date: 2026-09-11 (UTC).

The review covered target dependency direction, native versus cross-compilation, test discovery, dependency pinning,
tool requirements, npm command routing, and release runtime linkage. Individual test cases, fixture data, assertions,
formatter-owned concerns, third-party source, and generated implementation code were excluded.

## Findings

No actionable findings were verified in this segment. This is not a guarantee that the setup is defect-free.

The native preset disables the Windows adapter; tests link to the portable core and GoogleTest. Test discovery uses
`DISCOVERY_MODE PRE_TEST`, and only native-host test presets are exposed. Release configurations disable GoogleTest
fetching. The npm manifest, lockfile, and installed Prettier version agree.

## Checks

- Initial `git status --short`: clean.
- `ninja -C build/<preset> -n` for `linux-native-debug`, `linux-mingw-debug`, and `linux-mingw-release`: all reported no
  work to do, establishing that the existing outputs are current according to their build graphs.
- `build/linux-native-debug/tests/stay_awake_tests --gtest_brief=1`: all 46 tests passed. The existing native executable
  was invoked directly to avoid reconfiguration, downloads, and workspace build/log changes during this report-only
  review. CTest was not run, and no Windows test executable was executed on Linux.
- `node --check scripts/cmake.mjs`: passed. Installed CMake is 3.28.3, Node.js is v24.20.0, and Prettier is 3.9.6.
- Inspected the existing Release executable's PE metadata and imports with MinGW `objdump`: x86-64 Windows GUI binary,
  embedded resources, and Windows system imports; no separate MinGW runtime DLL dependency was present.

## Unresolved questions

None identified in the reviewed infrastructure.

## Limits and pending checks

A clean dependency fetch/install and a Windows-host UCRT64 build were not run. Dependencies were neither installed nor
updated. Existing build outputs and static configuration inspection do not establish reproducibility on a fresh host.
No formatting check was run because formatting is outside the requested skill's review scope. Windows GUI/runtime
validation is tracked in the Windows segment reports.
