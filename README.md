# Stay Awake

![Stay Awake Icon](StayAwake/icon.png)

A lightweight utility for Windows that prevents the system from sleeping and provides a tool to automatically close
specific windows after a scheduled duration.

## Features

### Sleep Prevention

- **Two Modes**:
  - **Require Display**: Keeps the system awake and the display turned on.
  - **Require System**: Keeps the system awake (background tasks running) but allows the display to turn off.
- Keeps the system awake by resetting the system idle timer.
- Configurable duration: 30 minutes to 8 hours (in 15-minute increments).
- Real-time countdown timer.
- Prevents the computer from going to sleep or locking the screen due to inactivity.

### Window Closer

- Automatically closes a selected window after a specified duration.
- Scans and lists all currently open windows.
- Displays detailed process information (Process Name, Window Handle) and Window Position (X, Y, Width, Height).
- **Window Highlighting**: Optionally overlays a translucent red box on the selected window to visually verify the
  target before scheduling closure.
- Configurable duration: 15 minutes to 8 hours (in 15-minute increments).
- Logs closure details (Time, Handle, Process Name) in the interface upon completion.

### General

- **System Tray**: Closing the window minimizes the application to the system tray. Left-click the tray icon to show the
  window, or right-click for options (Show/Quit).
- **Single Instance**: Ensures only one instance of the application runs at a time. If a new instance is started, the
  existing one is brought to the foreground.

## Requirements

- **OS**: Windows 11
- **Runtime**: .NET 10.0 Desktop Runtime (Windows)
- **Framework**: .NET 10.0-windows

## Releases

Download release builds from the [Releases](https://github.com/aforemendude/stay-awake/releases) page.

Each build is a single framework-dependent `StayAwake.exe`. Install the .NET 10.0 Desktop Runtime (Windows) matching the
build's architecture separately; the runtime is not bundled in the executable.

## Build Instructions

### Prerequisites

- .NET 10.0 SDK on Windows, Linux, or macOS
- Node.js 24.19.0 or newer and npm for the convenience scripts and Prettier

The project cross-compiles to Windows x64 (`win-x64`) by default. Running the application requires Windows. The first
build or C# formatting run restores the required Windows targeting packs from NuGet.

### Build Both Configurations

```bash
npm run build
```

This publishes both Debug and Release builds. Each `publish/` directory contains only `StayAwake.exe`, with debug
symbols embedded. The .NET settings use
[framework-dependent single-file publishing](https://learn.microsoft.com/en-us/dotnet/core/deploying/single-file/overview).

### Debug Build

To build the application for debugging (includes symbols, non-optimized):

```bash
npm run build:debug
# Or, using only the .NET SDK:
dotnet publish StayAwake/StayAwake.csproj -c Debug
```

The executable will be `StayAwake/bin/Debug/net10.0-windows/win-x64/publish/StayAwake.exe`.

### Release Build

To build the application for production (optimized):

```bash
npm run build:release
# Or, using only the .NET SDK:
dotnet publish StayAwake/StayAwake.csproj -c Release
```

The executable will be `StayAwake/bin/Release/net10.0-windows/win-x64/publish/StayAwake.exe`.

To target another Windows architecture, pass a runtime identifier, for example:

```bash
npm run build:release -- --runtime win-arm64
# Or:
dotnet publish StayAwake/StayAwake.csproj -c Release --runtime win-arm64
```

The output path uses the selected runtime identifier in place of `win-x64`. Use the executable from `publish/` for
distribution; `dotnet build` produces intermediate files without creating the single-file bundle.

### Code Formatting

The format commands run Prettier for documentation and configuration files and `dotnet format` for C# sources:

```bash
npm install
npm run format
npm run format:check
```

Prettier wraps prose at 120 columns. Generated build output is excluded from formatting.

To format or check only C# sources with the .NET SDK:

```bash
dotnet format StayAwake.slnx
dotnet format StayAwake.slnx --verify-no-changes
```
