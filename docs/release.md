# Release size and validation

See [development](development.md#direct-cmake) for build commands and [AGENTS.md](../AGENTS.md) for contribution
constraints.

## Optimization and distribution

Ship a single Windows 11 x64 executable with no adjacent assets or separately installed runtime.

MinGW `Release` and `MinSizeRel` builds optimize the core, Windows adapter, and executable with
[`-Os`](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html), function/data sections, and checked link-time
optimization (LTO). Match the compile and link optimization levels. CMake warns if LTO is unavailable. The linker
[removes unused sections and strips symbols](https://sourceware.org/binutils/docs/ld/Options.html), including debug data
from static runtime libraries. Keep `Debug` and `RelWithDebInfo` debugging information intact.

Preserve `-static -static-libgcc -static-libstdc++`, C++ exceptions, RTTI, embedded icon/manifest/version resources, and
executable security settings. Use integer conversions for countdowns and hexadecimal handles to avoid pulling stream and
locale machinery into the executable; preserve formatting behavior.

## Reference measurements

A Linux x64 cross-build with MinGW-w64 GCC 13-win32 produced these historical measurements. They are reference values,
not size limits; results vary with compiler and runtime libraries.

| Build                                       |     Bytes |
| ------------------------------------------- | --------: |
| Previous Release                            | 2,593,484 |
| Previous Release with symbols stripped      | 1,132,544 |
| Size flags, LTO, unused-code removal, strip |   997,376 |
| Above plus stream-free integer formatting   |   275,968 |

The final executable was 89.4% smaller and imported only Windows system DLLs.

## Validation

For changes likely to affect size (build/link settings, dependencies, formatting, or resources):

1. Measure the Release executable before and after with the same toolchain and configuration. Report byte counts and
   explain increases.
2. Inspect DLL imports. Only Windows-provided DLLs are acceptable; there must be no `libgcc`, `libstdc++`, or
   `libwinpthread` DLL dependency. OS-provided `msvcrt.dll` and UCRT API sets are expected. Static-link flags alone do
   not prove this.
3. Verify that icon, manifest, and version resources remain embedded and ASLR/NX remain enabled.

Inspect Linux cross-build PE headers and imports with:

```sh
x86_64-w64-mingw32-objdump -p build/linux-mingw-release/StayAwake.exe
```

On Windows, use the toolchain's `objdump` and the `windows-mingw-release` path.

Manually verify affected Windows behavior: tray/activation, DPI, overlay click-through, power, suspend/resume, and
shutdown. Report unavailable checks as pending; binary inspection and cross-compilation do not exercise these behaviors.
