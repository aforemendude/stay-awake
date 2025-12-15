# Stay Awake

A lightweight utility for Windows that prevents the system from sleeping and provides a tool to automatically close specific windows after a scheduled duration.

## Features

### Sleep Prevention
- Keeps the system awake by resetting the system idle timer.
- Configurable duration: 30 minutes to 8 hours (in 15-minute increments).
- Real-time countdown timer.
- Prevents the computer from going to sleep or locking the screen due to inactivity.

### Window Closer
- Automatically closes a selected window after a specified duration.
- Scans and lists all currently open windows.
- Displays detailed process information (Process Name, Window Handle).
- Configurable duration: 15 minutes to 8 hours (in 15-minute increments).
- Logs closure details (Time, Handle, Process Name) in the interface upon completion.

## Requirements

- **OS**: Windows
- **Runtime**: .NET 10.0 Desktop Runtime (Windows)
- **Framework**: .NET 10.0-windows

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

### Publish (Single File)
To publish as a self-contained single-file executable (no external .NET runtime required):
```bash
dotnet publish -c Release -r win-x64 --self-contained -p:PublishSingleFile=true
```
The executable will be located in `StayAwake/bin/Release/net10.0-windows/win-x64/publish/`.
