# Build and tooling review

## Scope and basis

Reviewed the root CMake configuration and presets, `cmake/`, `scripts/`, npm manifests and lockfile,
test registration and shared platform/factory setup, checked-in resources, repository configuration,
and build/release documentation at commit `5f80b0ca3120d25f5304f9a2dc110cbf71806617`.
The worktree was clean before report creation. Dependency implementations and generated output are excluded
as review targets; existing generated build metadata is used only to verify build behavior.

## Findings

### Incremental builds do not track the manifest or resource-ID header

- **Severity:** Medium
- **Location:** `CMakeLists.txt:84-87`; resource inputs at `src/platform/windows/stay_awake.rc:2-5`.
- **Problem:** The resource object's explicit dependency covers `assets/icon.ico` only. On the documented
  Linux MinGW/Ninja toolchain, the generated resource compiler command does not emit dependency information,
  and Ninja records zero discovered dependencies for `stay_awake.rc.res`. Neither `stay_awake.manifest`
  nor `resource.h` is a dependency of that object. Changing only the manifest therefore leaves the old
  resource object and executable in place during an incremental build.
- **Impact:** A developer can rebuild successfully yet distribute the previous DPI-awareness, privilege,
  compatibility, or common-controls declaration. Changing `IDI_STAY_AWAKE` in `resource.h` is worse:
  C++ dependency tracking recompiles `main_window.cpp` with the new ID, while the embedded icon retains
  the old ID, causing the startup icon loads at `src/platform/windows/main_window.cpp:97-103` to fail.
- **Recommendation:** Add the manifest and local `resource.h` include to the resource source's
  `OBJECT_DEPENDS` alongside the icon. This property explicitly triggers recompilation with Ninja.
  [CMake 3.28 OBJECT_DEPENDS documentation](https://cmake.org/cmake/help/v3.28/prop_sf/OBJECT_DEPENDS.html)
- **Verification:** With the existing GCC 13-win32/CMake 3.28.3 build,
  `ninja -C build/linux-mingw-release -t deps CMakeFiles/stay_awake.dir/src/platform/windows/stay_awake.rc.res`
  reports `#deps 0`. Querying the absolute manifest path with `ninja -t query` reports an unknown target.
  The generated RC rule invokes `windres -O coff` without dependency-generation options. A fresh Release
  build under `/tmp/stay-awake-review-release-20260913` reproduced the zero-dependency resource object;
  its C++ object dependency list separately confirmed that `main_window.cpp` tracks `resource.h`.

## Unresolved questions

None identified beyond the finding above.

## Checks and limitations

- Both JavaScript scripts pass `node --check` with Node.js 24.20.0.
- `npm ls --depth=0` confirms that the installed direct dependencies match the manifest.
- Reviewed native-host test presets, `DISCOVERY_MODE PRE_TEST`, core-only test linkage, and the test-only
  entrypoint/factory arrangement. Individual test cases, assertions, fixture data, and coverage are excluded.
- Fresh Linux Debug configuration/build succeeded with CMake 3.28.3, Ninja, and GCC 13.3.0. The resulting
  test executable passed all 85 tests across seven suites. Native CTest registration also ran 85/85 tests
  successfully against the existing native preset build.
- Fresh Windows Release configuration/cross-build succeeded with the `linux-mingw-release` preset and
  GCC 13-win32. The executable is **275,968 bytes**. PE inspection confirmed x64/Windows GUI,
  `DYNAMIC_BASE`, `HIGH_ENTROPY_VA`, `NX_COMPAT`, and icon/group-icon, version, and manifest resources.
  Imports are Windows system DLLs/API sets, including `msvcrt.dll`; no `libgcc`, `libstdc++`, or
  `libwinpthread` DLL is imported.
- Fresh builds used `/tmp/stay-awake-review-native-20260913` and
  `/tmp/stay-awake-review-release-20260913`, with existing GoogleTest sources supplied through
  `FETCHCONTENT_SOURCE_DIR_GOOGLETEST` and downloads disabled for the native configuration.
  No dependencies were installed, updated, or repaired. No source, test, or configuration files were changed.
- No before/after size comparison is applicable to this report-only review. Windows-host builds,
  Debug/RelWithDebInfo Windows binaries, and manual desktop behavior were not validated in this session.
- Formatter-owned concerns and external release publication are outside this review.
