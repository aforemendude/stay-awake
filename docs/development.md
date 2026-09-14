# Development

See the [README](../README.md#build-test-and-format) for npm shortcuts and [AGENTS.md](../AGENTS.md) for development
constraints. Run commands from the repository root. Build hosts are Linux or Windows; the GUI runs only on Windows.

## Prerequisites

- CMake 3.28+, Ninja, and a C++17 compiler. Windows app builds require an x64 MinGW-w64 toolchain with Windows 10 APIs
  and `mincore` support; Linux tests use the host compiler.
- On Windows, use MSYS2 UCRT64 with matching `gcc`, `g++`, `windres`, `cmake`, and `ninja` on PATH.
- npm shortcuts require Node.js and npm (see the [Node version requirement](../package.json)). `npm ci` installs
  Prettier and the icon converter. Install clang-format for C++ formatting.

Debug/test configurations download pinned GoogleTest sources from GitHub on first configuration. Release presets disable
tests and do not fetch GoogleTest. Formatting ignores generated build output and dependencies.

## Direct CMake

Choose a preset from [CMakePresets.json](../CMakePresets.json):

| Host    | Preset                  | Builds                         |
| ------- | ----------------------- | ------------------------------ |
| Linux   | `linux-native-debug`    | Portable core and native tests |
| Linux   | `linux-mingw-debug`     | Windows app and tests          |
| Linux   | `linux-mingw-release`   | Windows app                    |
| Windows | `windows-mingw-debug`   | Windows app and native tests   |
| Windows | `windows-mingw-release` | Windows app                    |

Use the same preset for configuration and build. For a Release cross-build on Linux:

```sh
cmake --preset linux-mingw-release
cmake --build --preset linux-mingw-release
```

Executables are written to `build/<preset>/StayAwake.exe`. Distribute the Release executable; its resources and MinGW
runtime are embedded, so no adjacent assets or separately installed runtime are needed.

For native tests on Linux:

```sh
cmake --preset linux-native-debug
cmake --build --preset linux-native-debug
ctest --preset linux-native-debug
```

Use `windows-mingw-debug` for native Windows tests. Run CTest only with native-host presets; cross-built Windows tests
must run on Windows.

Portable unit tests mirror `src/` under `tests/` using `<source>.test.cpp` names. Entrypoint tests compile
`src/main.cpp` with a test-only function name and supply a fake platform factory; they link only the core and
GoogleTest. Register new test sources in `tests/CMakeLists.txt` and `STAY_AWAKE_FORMAT_FILES` in the root
`CMakeLists.txt`.

For C++ formatting, append `--target format` or `--target format-check` to the build command. Install clang-format
before configuration. Direct CMake builds do not require Node.js.

## Updating the icon

Edit [assets/icon.svg](../assets/icon.svg), then regenerate the Windows icon:

```sh
npm run icon:convert
```

The [converter](../scripts/convert-icon.mjs) uses [svg-to-ico](https://github.com/jtrauntvein/svg-to-ico) to create
`assets/icon.ico` with a single transparent, PNG-compressed 256 × 256 px image. Commit both SVG and ICO, then rebuild to
embed the icon. Direct CMake builds use the checked-in ICO.

See [release size and validation](release.md) for optimization settings and distribution checks.
