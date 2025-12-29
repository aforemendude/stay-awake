# Stay Awake

![Stay Awake Icon](StayAwake/icon.png)

A lightweight utility for Windows that prevents the system from sleeping and provides a tool to automatically close specific windows after a scheduled duration.

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
- **Window Highlighting**: Optionally overlays a translucent red box on the selected window to visually verify the target before scheduling closure.
- Configurable duration: 15 minutes to 8 hours (in 15-minute increments).
- Logs closure details (Time, Handle, Process Name) in the interface upon completion.

### General
- **System Tray**: Closing the window minimizes the application to the system tray. Left-click the tray icon to show the window, or right-click for options (Show/Quit).
- **Single Instance**: Ensures only one instance of the application runs at a time. If a new instance is started, the existing one is brought to the foreground.

## Requirements

- **OS**: Windows 11
- **Runtime**: .NET 10.0 Desktop Runtime (Windows)
- **Framework**: .NET 10.0-windows

## Releases

Download release builds from the [Releases](https://github.com/aforemendude/stay-awake/releases) page.

Note: Due to file size, a self-contained release build will not be provided. You need to install the .NET 10.0 runtime separately.

## Build Instructions

### Prerequisites
- .NET 10.0 SDK

### Debug Build
To build the application for debugging (includes symbols, non-optimized):
```bash
dotnet build -c Debug
```
The output will be in `StayAwake/bin/Debug/net10.0-windows/`.

### Release Build
To build the application for production (optimized):
```bash
dotnet build -c Release
```
The output will be in `StayAwake/bin/Release/net10.0-windows/`.

## AI Disclosure

This project's code, documentation, and other assets were generated with the assistance of an AI coding agent.
