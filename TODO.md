# TODO: Port Stay Awake to C++ and Win32

This is the execution plan for replacing the .NET application with a Windows C++ application that can be cross-compiled
from Linux. Creating this plan does not perform the migration.

Use `/home/sudoer/workspace-example/DoubleClickHotkey` as the reference project throughout implementation. That
directory will remain available. Match its CMake workflows, portable application/platform boundary, formatting, and
application-only unit testing approach; adapt its console application structure to this project's Win32 GUI.

## Scope and completion criteria

- [ ] Replace the C# application, solution, and .NET build/format commands with C++17, CMake, and MinGW-w64 workflows.
- [ ] Preserve existing functionality, layout, displayed information, tray behavior, and independent timers, subject to
      the small corrections explicitly identified below.
- [ ] Build a Windows x64 `StayAwake.exe` from Linux and Windows. Build and run portable application tests natively on
      both hosts. Linux does not need an application UI or a power-management implementation.
- [ ] Implement the UI through direct Win32 APIs. Keep the window fixed in logical size, with proper DPI scaling at
      startup and when DPI changes. No WinForms, WPF, Qt, or other UI framework is needed.
- [ ] Add an `AGENTS.md` adapted from the reference, documenting the architecture, commands, and testing boundaries.
- [ ] Keep `CODE_REVIEW_CORE.md`, `CODE_REVIEW_INTERFACE_TIMERS.md`, and `CODE_REVIEW_STARTUP_BUILD_DOCS.md` intact as
      historical documents. Resolving every review finding is not a prerequisite for this port.
- [ ] Preserve the license, project identity, and existing icon assets. Remove obsolete runtime/build documentation.

Do not expand this work into settings persistence, new command-line features, a Linux GUI, forced process termination,
an installer, a new release pipeline, or a comprehensive review-remediation project. Straightforward lifecycle and
correctness fixes belong in the port; more involved alternatives below can remain deferred with their limits recorded.

## 1. Establish the behavior baseline before deleting C# sources

- [ ] Use the actual source as the behavior specification. The README omits some existing behavior and includes a
      screen-lock guarantee that the implementation does not establish.
- [ ] Preserve a usable reference to the pre-migration revision in the commit history. Once deleted, the C# paths below
      can be inspected through Git; do not retain a second .NET implementation in the tree.
- [ ] Capture a Windows baseline of the layout and interactions if Windows is available. If it is unavailable, proceed
      from the source and leave visual verification pending rather than claiming it passed.

| Area               | Source                                            | Behavior to carry forward                                                                                                                                                                                                                                                   |
| ------------------ | ------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Startup            | `StayAwake/Program.cs`, `MainForm_Shown`          | One instance per interactive session; start hidden with a tray icon and both features inactive. A second launch requests Show on the existing instance, then exits.                                                                                                         |
| Main window        | `StayAwake/Forms/MainForm.Designer.cs`            | Title `Stay Awake`; fixed border; maximize disabled; minimize available. Designer client size is 784 × 546, with an upper Stay Awake group and a larger Window Closer group below.                                                                                          |
| Awake durations    | `MainForm.LoadDurations`                          | 30 minutes through 8 hours inclusive, every 15 minutes, followed by `00:00:10`. Default: 2 hours. Format choices as `hh:mm:ss`.                                                                                                                                             |
| Close durations    | `MainForm.LoadDurations`                          | 15 minutes through 8 hours inclusive, every 15 minutes, followed by `00:00:10`. Default: 1 hour. The 10-second choices currently appear in all builds.                                                                                                                      |
| Awake modes        | `MainForm.StartStayAwake`, `Core/PowerManager.cs` | Require Display keeps system and display awake; Require System keeps the system awake while allowing the display to turn off. Only one awake mode can be active.                                                                                                            |
| Awake controls     | `StartStayAwake`, `StopStayAwake`                 | Active button becomes `Stop Require Display` or `Stop Require System`; the other mode and duration selector are disabled. Stop restores idle controls.                                                                                                                      |
| Countdown          | `MainTimer_Tick`, designer timer                  | Both features show Remaining Time in `hh:mm:ss`, with `Not Enabled` when inactive. A one-second UI timer runs when either feature is active. Both can run concurrently.                                                                                                     |
| Awake completion   | `MainTimer_Tick`                                  | Record the mode, success/error, and local end time in the Stay Awake group status; existing time format is `MM/dd HH:mm:ss`. Starting again resets the previous completion caption.                                                                                         |
| Window list        | `Core/WindowCloser.cs`                            | Enumerate visible top-level windows with nonblank titles. Exclude the shell window and exact title `Program Manager`; sort by title. This does not exclude every `explorer.exe` window.                                                                                     |
| Window details     | `LstWindows_SelectedIndexChanged`                 | Show process name, uppercase hexadecimal native handle, and `X`, `Y`, `Width`, `Height` from the window rectangle. Process lookup failure uses `Unknown`; rectangle failure shows `Error getting position`. Details are read-only and copyable.                             |
| Refresh            | `RefreshWindows`, `ShowForm`                      | Refresh clears selection/details and stops highlighting. Showing the main window refreshes unless a close is scheduled. Explicit refresh failure shows an error dialog; automatic refresh failure does not interrupt with a dialog.                                         |
| Close scheduling   | `BtnCloseWindow_Click`                            | Require a selected window, otherwise report `No window selected.`. Schedule one target; change button to `Stop`; disable list, close duration, and refresh while scheduled. Awake controls and highlight remain independent.                                                |
| Close cancellation | `StopCloseWindow`                                 | Cancel without sending a close request; restore controls and `Not Enabled`. The initial caption is `Schedule Close Window`, but the existing reset caption is `Close Window`; choose a consistent caption below.                                                            |
| Close completion   | `MainTimer_Tick`, `WindowCloser.CloseWindow`      | Post `WM_CLOSE` once at expiry; record time, handle, process name, and outcome, then stop the schedule and refresh. Existing timestamp format is `MM/dd HH:mm`. This requests a window close, not a process kill.                                                           |
| Highlight          | `OverlayForm.cs`, selection/highlight handlers    | Toggle `Highlight Window` / `Stop Highlighting`; show a red overlay at 25% opacity over the selected rectangle. It is borderless, topmost, absent from the taskbar, and intended to pass clicks through. Current positioning updates on selection/toggle, not continuously. |
| Hide and show      | `MainForm_FormClosing`, `ShowForm`                | User close hides to tray and stops highlighting, while both scheduled operations continue. Tray left-click, tray Show, and second launch restore the normal window and request activation. Ordinary minimize is distinct from close-to-tray.                                |
| Exit               | tray Quit handler                                 | Quit exits the process. The native version must explicitly clean up power requests, timers, tray icon, overlay, and instance resources. OS shutdown/sign-out must also be allowed.                                                                                          |

## 2. Decisions and alternatives

Record the chosen option before implementing each affected component. Recommended defaults are sufficient to proceed
unless later instructions say otherwise; these are implementation choices, not a requirement to stop for approval.

### D1. Supported build matrix and language level

- **Recommended: C++17, CMake 3.28+, Ninja, Windows 11 x64, Linux and Windows MinGW-w64 workflows**, matching the
  reference. Pros: proven repository pattern, small support matrix, portable native tests. Cons: the current README's
  macOS build and optional Windows ARM64 examples will no longer describe supported workflows.
- **Additional macOS cross-build, Windows ARM64, or MSVC presets.** Pros: broader build/architecture support. Cons:
  extra toolchains, entry-point/resource/linking differences, and validation environments. Defer unless required; do not
  infer these capabilities from a configurable MinGW target prefix.

### D2. Direct Win32 UI construction

- **Recommended: standard native controls created with `CreateWindowExW`, with layout in C++.** Use BUTTON/group box,
  STATIC, read-only EDIT, COMBOBOX, and LISTBOX controls. Pros: direct Win32, native keyboard/accessibility behavior,
  straightforward mapping from the existing UI. Cons: explicit control IDs, notification routing, font and DPI layout.
- **Win32 dialog resources using `DIALOGEX` and native controls.** Pros: declarative layout and dialog navigation. Cons:
  dialog-unit conversion and automatic dialog DPI behavior require care to avoid double scaling; a cross-platform
  resource-editor workflow is less convenient. This still satisfies direct Win32.
- **Owner-drawn controls/custom GDI painting.** Pros: full appearance control. Cons: much more painting, hit testing,
  focus, accessibility, and theme work with no functional benefit for this layout. Use custom drawing only where needed,
  such as the highlight overlay.

### D3. Portable application interface

- **Recommended: one abstract `PlatformBinding`, a portable controller, and a portable view state**, following the
  reference. Divide the Windows implementation into focused components behind that binding. Pros: simple composition
  root, one fake for controller tests, clear ownership. Cons: the binding is larger than the reference's small hotkey
  contract; keep its methods about application operations rather than individual HWNDs.
- **Separate clock, power, window-catalog, view, and lifecycle interfaces.** Pros: narrower responsibilities and smaller
  fakes per feature. Cons: more wiring and types for a small application. Split only if the single contract becomes
  difficult to understand; the portable/native boundary must remain the same.

### D4. Duration clock and suspend behavior

- **Recommended: monotonic elapsed time that includes suspend/hibernate, plus a separate local clock for status
  timestamps.** This preserves the existing practical treatment of suspended time while removing clock-change bugs.
  Pros: DST/manual clock changes do not alter durations; an overdue close is requested on the first tick after resume.
  Cons: requires an explicit clock contract and Windows clock adapter; a selected window can receive its scheduled close
  soon after resume. Use a verified suspend-inclusive source such as biased interrupt time.
- **Monotonic awake-time only.** Pros: countdown pauses during suspend, which some users may prefer. Cons: changes the
  current behavior and extends real elapsed durations. Windows unbiased interrupt time excludes sleep and hibernation.
- **Local or UTC wall-clock deadlines.** Pros: superficially closest to the current implementation and easy to display.
  Cons: local time has DST/time-zone issues; even UTC is affected by clock corrections. Do not choose this merely for
  line-by-line similarity.

Microsoft distinguishes biased and unbiased interrupt time in the
[QueryUnbiasedInterruptTime documentation](https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-queryunbiasedinterrupttime).
Keep that OS choice inside the adapter. `std::chrono` duration types are portable, but an unspecified
`std::chrono::steady_clock` suspend policy is not a substitute for this contract.

### D5. Instance activation and message-loop integration

- **Recommended: session-local named mutex plus an auto-reset Show event**, with bounded endpoint-readiness retry by the
  second process and event consumption on the main thread after UI initialization. Integrate the event with the message
  queue using `MsgWaitForMultipleObjectsEx`. Pros: close to existing behavior, no worker-thread UI lifetime race,
  pending Show can survive initialization. Cons: requires careful wait/message pumping and startup/shutdown ordering. A
  modal dialog can delay consumption until the outer loop resumes.
- **Mutex plus a private window message to a discoverable receiver.** Pros: fits a normal GUI message pump, including
  modal loops. Cons: needs receiver discovery/readiness retry and attention to foreground activation and integrity
  boundaries; `FindWindow` alone does not establish single-instance ownership.
- **Adapt the reference's event/shared-memory command channel.** Pros: reusable pattern for command transfer and
  mixed-elevation access. Cons: Show is the only command needed here, and the reference explicitly permits commands to
  fail while the receiver initializes. Add a readiness strategy; copying that limitation would retain an obvious bug.

For the default protocol, prefer retaining the existing StayAwake mutex/event identities where compatible, so an old and
a new executable recognize each other. Do not copy DoubleClickHotkey's object names. Preserve session-local scope;
machine-wide singleton behavior is a different product choice. For mixed-elevation launches, either adapt the
reference's limited Show-channel permissions or document a same-elevation limitation; broader access helps activation
but adds security-descriptor complexity. Never allow the activation channel to schedule closes or issue power commands.

### D6. Scheduled target identity

- **Recommended minimum: capture the selected handle and owning process ID, then revalidate immediately before
  posting.** Pros: small change that rejects missing targets and many handle-reuse cases. Cons: it cannot eliminate
  reuse within the same process or the race between validation and posting. Do not claim a complete identity guarantee.
- **Also retain process creation identity and track target destruction.** Pros: stronger protection against process and
  handle reuse. Cons: more access failures, hooks/lifetimes, and platform work; a title comparison is not a valid
  replacement because ordinary applications change titles. Defer if it stops being a straightforward addition.

Do not retain the current bare list selection as the scheduled target. Capture a value at schedule time regardless of
the identity option, and never silently substitute a different row or a newly reused target.

### D7. Highlight tracking

- **Recommended: selection/toggle snapshot, matching current behavior.** Pros: smallest faithful port and no continuous
  monitoring. Cons: an overlay can become stale if its target moves, resizes, minimizes, or disappears. Hide it on
  refresh, deselection, invalid/zero-size geometry, main-window hide, and exit; document the snapshot behavior.
- **Poll geometry while highlighting.** Pros: simple live tracking and detection of disappeared targets. Cons: adds a
  timer even when no scheduled feature is active, plus interval and minimized-window policies.
- **Use WinEvent hooks for movement/destruction.** Pros: responsive updates without polling. Cons: event filtering,
  lifetime, threading, and reentrancy complexity. Defer unless live tracking is specifically selected.

### D8. Text encoding and title ordering

- **Recommended: UTF-8 in the portable model, explicit UTF-16 conversion at Win32 calls.** Pros: matches the reference's
  portable string approach and makes Linux tests predictable. Cons: conversion helpers and a defined fallback for
  malformed native text are required.
- **UTF-16 portable strings (`std::u16string`).** Pros: closer to native window text and fewer conceptual conversions.
  Cons: more friction in formatting, test messages, and stream output. Avoid assuming `wchar_t` has the same width on
  Windows and Linux.
- **Recommended ordering: obtain locale-aware title order from the Windows adapter.** Pros: closer to C#'s
  culture-sensitive `OrderBy(w => w.Title)`; no locale library in the portable core. Cons: ordering varies by user
  locale, and native collation remains a manual check. Filtering/selection policy can still be portable.
- **Portable ordinal ordering instead.** Pros: deterministic and easy to test. Cons: visibly different order for case,
  accents, and non-ASCII titles. Record this as a deliberate difference if chosen.

### D9. Small UI behavior choices

- **Recommended: retain both 10-second choices in all builds.** Pros: preserves existing functionality and provides a
  quick manual test path. Cons: exposes an option originally described in source as debugging. Making it Debug-only
  cleans up the Release menu but changes the current feature set.
- **Recommended: use `Schedule Close Window` consistently when idle and `Stop` when scheduled.** Pros: removes an
  inconsistent caption without changing an action. Cons: differs from the old post-cancellation text.
- **Recommended: render remaining time immediately on start and round positive fractional seconds up.** Pros: avoids
  temporarily displaying `Not Enabled` or `00:00:00` for an active timer. Cons: differs by up to a second from the
  current truncating display. Retaining truncation is also viable; expiry must always use the real deadline.
- **Recommended: keep short status text in group captions, with an adjacent read-only detail line if needed.** Pros:
  maintains the layout while keeping long process names/errors accessible at all DPIs. Cons: may require a modest
  fixed-size adjustment. Captions alone are closer to the old layout but can clip completion information; a new log
  panel is more complexity than the current single-result display needs.
- **Own-window filtering: preserve current eligibility, or (recommended) exclude StayAwake's own windows.** Preserving
  it is closest to the source but can allow scheduling the main window to hide itself. Excluding the application's own
  PID is a small, sensible default that avoids self-targeting; it deliberately narrows the list. Keep other
  applications' ordinary Explorer windows eligible.

### D10. Native runtime packaging and GUI entry point

- **Recommended: statically link the MinGW runtime for the application**, as the reference does. Pros: a standalone
  executable requiring only Windows system DLLs, with no .NET/runtime installation. Cons: larger executable and runtime
  fixes require rebuilding. Inspect actual imports rather than assuming flags prove self-containment.
- **Ship MinGW runtime DLLs alongside the executable.** Pros: smaller executable and explicit shared runtime files.
  Cons: multi-file distribution and additional packaging/version matching. This departs from the reference's workflow.
- **Entry option A (recommended): keep portable `main()` and use MinGW's supported GUI-subsystem startup.** Pros:
  closest to the reference and minimal glue. Cons: confirm linking with the chosen toolchains; this is not an automatic
  MSVC promise.
- **Entry option B: a tiny Windows `wWinMain` wrapper calling a portable `RunApplication()`.** Pros: explicit GUI entry
  point and easier future MSVC support. Cons: one extra platform source. In either case, put no Win32 code in the
  portable composition root and make Explorer launch free of a console window.

### D11. Native operation failures

- **Recommended: explicit operation results, failed starts leave the feature idle, failed power release remains visible
  with a manual retry path.** Pros: UI does not claim power protection was released when it was not; avoids repeated
  timer-driven error dialogs. Cons: needs a small additional error state and control-state definition.
- **Reset immediately to idle on a release error, as the old code does.** Pros: simpler state machine. Cons: can display
  `Not Enabled` while the request remains outstanding. Prefer correcting this inexpensive state inconsistency.
- **Automatic bounded release retry.** Pros: may recover transient errors. Cons: extra policy/timer states; optional
  rather than necessary for the first port. Cleanup must still make a best-effort release on exit.

### D12. High DPI on a small monitor work area

- **Recommended: Keep one fixed logical client size at every scale.** Pros: simplest interpretation of fixed size and
  consistent layout. Cons: the scaled window may exceed a small screen's work area. Document the usable work-area
  requirement; moving the window alone does not guarantee every control can be reached.
- **Keep the normal fixed layout, but reduce the list viewport when needed to fit the monitor.** Pros: preserves
  control/font scaling, layout order, and access to actions without enabling user resizing. Cons: fewer visible list
  rows and extra work-area calculations; extremely small work areas may still need a documented limit.
- **Scroll the entire fixed logical layout inside a bounded viewport.** Pros: all content remains reachable even when
  the monitor cannot fit the normal window. Cons: extra navigation and scrolling behavior unlike the current UI.

Do not fit a high-DPI screen by silently shrinking fonts or disabling DPI awareness.

## 3. Target repository structure and ownership

Names below are a proposed map, not a requirement for a separate file for every small helper. Preserve the reference's
dependency direction: executable → Windows adapter → portable core; tests → portable core only.

```text
AGENTS.md
CMakeLists.txt
CMakePresets.json
cmake/
  ClangFormat.cmake
  toolchains/mingw-w64-x86_64.cmake
include/stay_awake/
  application.hpp
  application_state.hpp       # durations, selections, feature/view state, result values
  platform_binding.hpp
  platform_factory.hpp
src/
  main.cpp                    # portable composition root
  application/
    application.cpp
    durations.cpp             # split formatting/model helpers only when useful
  platform/windows/
    platform_factory.cpp
    windows_platform_binding.hpp/.cpp
    main_window.hpp/.cpp      # controls, event routing, rendering, fixed layout
    dpi.hpp/.cpp
    tray_icon.hpp/.cpp
    single_instance.hpp/.cpp
    power_manager.hpp/.cpp
    window_catalog.hpp/.cpp   # enumeration, native metadata, guarded close request
    overlay_window.hpp/.cpp
    clock.hpp/.cpp
    resource.h
    stay_awake.rc
    stay_awake.manifest
    entry_point.cpp           # only if the selected GUI startup strategy needs it
assets/
  icon.ico
  icon.png
tests/
  CMakeLists.txt
  application/
    application.test.cpp
    durations.test.cpp
scripts/cmake.mjs
.clang-format
.prettierignore
README.md
TODO.md
CODE_REVIEW_*.md              # preserve the three existing documents
```

- [ ] Keep `windows.h`, HWND/HANDLE, window procedures, resource IDs, OS flags, and `#ifdef _WIN32` out of public
      application headers, `src/application/`, and unit tests. Windows-only helpers belong under the adapter directory.
- [ ] Define portable types for duration choices, awake mode, window identity, signed rectangle coordinates, selected
      metadata, countdown/status text, operation results, and control availability. Native handles may cross the
      boundary only as opaque values such as a strongly typed `std::uintptr_t`, never as a dereferenceable native API.
- [ ] The controller owns selection, captured schedule target, durations, deadlines, active/error states, highlight
      intent, show/refresh policy, and user-visible results. It decides which native operations to request.
- [ ] The binding owns native resource lifetimes, event delivery, clocks, enumeration/geometry queries, locale-specific
      services, power requests, close posting, overlay display, and presentation of controller state.
- [ ] Route typed application events from control notifications, tray commands, timer ticks, activation, and shutdown.
      Keep layout/control code free of timer-expiry and scheduling decisions.
- [ ] Keep all controller calls and execution-state changes on the UI thread. Document callback lifetime and reentrancy:
      modal dialogs can pump messages, so finish state transitions before presenting an error.
- [ ] Use RAII for owned Win32 handles, GDI objects, icons, menus, and event registrations. Distinguish borrowed/shared
      handles from owned resources. No exception may escape a Win32 callback or a destructor.

## 4. Implement the CMake and tooling foundation

Reference files: `CMakeLists.txt`, `CMakePresets.json`, `cmake/toolchains/mingw-w64-x86_64.cmake`,
`cmake/ClangFormat.cmake`, `tests/CMakeLists.txt`, `scripts/cmake.mjs`, and formatting files in DoubleClickHotkey.

- [ ] Create project `StayAwake` and namespaced targets such as `StayAwake::core` and `StayAwake::platform`; produce
      `StayAwake.exe`. Preserve existing project version/license metadata unless there is a separate reason to change
      it.
- [ ] Define `STAY_AWAKE_BUILD_APP`, defaulting on for Windows targets. With it off, compile only portable code/tests;
      explicitly requesting an app on an unsupported target should fail with an understandable configure error.
- [ ] Apply C++17, disabled compiler extensions, and the reference's target-scoped warning settings. Define `UNICODE`,
      `_UNICODE`, `NOMINMAX`, `WIN32_LEAN_AND_MEAN`, and the chosen Windows API baseline only on native targets.
- [ ] Select Windows sources and libraries in CMake. Link required libraries for the implementation, normally `user32`,
      `gdi32`, and `shell32`, plus `comctl32`, `advapi32`, or others only when the selected APIs require them.
- [ ] Configure the GUI subsystem, selected startup strategy, and static runtime flags on the app target. Keep the
      GoogleTest executable a normal console test executable.
- [ ] Enable the RC language only when building the Windows app. Use the cross toolchain's target `windres`, and attach
      the `.rc` directly to the executable so its icon/manifest cannot disappear through static-library extraction.
- [ ] Embed the existing icon and an application manifest declaring Per-Monitor V2 awareness, normal `asInvoker`
      execution, and common-controls v6 if used. No external icon/manifest file should be required at runtime.
- [ ] Use target-only include/library/package searches and host program searches in the MinGW toolchain, following the
      reference. Keep output under `build/<preset>` and enable compile command export.
- [ ] Add matching configure/build presets:

| Preset                  | App | Tests | Execution                                                                     |
| ----------------------- | --- | ----- | ----------------------------------------------------------------------------- |
| `linux-native-debug`    | Off | On    | Run portable tests on Linux.                                                  |
| `linux-mingw-debug`     | On  | On    | Cross-compile app and Windows tests; do not run PE files during Linux builds. |
| `linux-mingw-release`   | On  | Off   | Cross-compile release app.                                                    |
| `windows-mingw-debug`   | On  | On    | Build app and run portable tests natively on Windows.                         |
| `windows-mingw-release` | On  | Off   | Build release app on Windows.                                                 |

- [ ] Add CTest presets only for native test environments. Adopt the reference's pinned GoogleTest FetchContent URL and
      hash, and `gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)` to avoid executing Windows tests while building on
      Linux. Explain the first-configuration network requirement; release configuration should not fetch GoogleTest.
- [ ] Adapt the Node preset dispatcher so `npm run build` builds Debug and Release Windows applications for the host,
      `npm run test` builds/runs native tests, and formatting invokes the appropriate CMake target. Propagate errors.
- [ ] Retain useful existing `build:debug` and `build:release` shortcuts by extending the dispatcher, or explicitly
      document their replacement. Keeping them has little cost and avoids breaking established commands.
- [ ] Copy/adapt `.clang-format` and CMake format targets. Keep the maintained source list complete, including new
      headers/tests; retain Prettier for Markdown/JSON/JavaScript. Exclude `build/` and dependency/generated output.
- [ ] Keep Node/npm optional for direct CMake use. Update `package-lock.json` only when package metadata/dependencies
      require it; no new JavaScript framework or runtime dependency is necessary.

## 5. Port portable application behavior

- [ ] Generate duration values rather than hard-coding selected indexes. Select defaults by duration value. Awake list:
      31 quarter-hour choices plus 10 seconds; close list: 32 quarter-hour choices plus 10 seconds.
- [ ] Implement independent awake and scheduled-close state machines. Starting/stopping one must not reset the other.
      Derive button captions, enablement, and timer-needed state from the model, not from current control text.
- [ ] Start awake protection only after a valid selection and successful native request. Save the requested mode and
      deadline, update the display immediately, and prevent mode switching while active.
- [ ] Implement manual and timed awake stop, including the chosen release-error state. Preserve mode/time/outcome
      information at expiry; manual stop does not need to invent an expiry record.
- [ ] Use an injected duration clock and separate timestamp source. Compute remaining time from a deadline on every tick
      rather than subtracting one second per callback. Clamp display to zero; process late ticks and resume correctly
      without repeated completion actions.
- [ ] Implement refresh/selection state, metadata display, and highlight intent. Preserve the old filtering rules except
      any explicitly chosen own-window exclusion. Preserve duplicate titles as distinct selectable targets.
- [ ] Require a selected window to schedule a close; capture its identity and display metadata. Ignore stale UI events
      that attempt to replace the target, refresh, or alter the close duration while a schedule is active.
- [ ] Cancel without closing; at expiry consume the schedule once, request a guarded close, store its result, restore
      controls, and refresh automatically. Preserve the result even if that refresh fails.
- [ ] Report successful posting as `Close requested` rather than `Closed`, keeping time, handle, and process name. A
      queued message does not establish that the target handled it or closed; see
      [PostMessageW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew).
- [ ] Clear highlight state consistently on refresh/hide even when enumeration fails. Suppress stale overlays after
      invalid selection or geometry. Highlight with no selection may remain enabled as an intent, as in the current
      code, but must display no overlay until there is a valid selection.
- [ ] Show/second-launch events refresh only when no close is scheduled, then restore/activate the main window. Hide
      events stop highlight and preserve schedules; Quit/confirmed session-end events cancel schedules and request
      cleanup.
- [ ] Keep user-initiated errors visible and automatic completion errors in status. Do not generate recurring modal
      dialogs every second or erase a useful result just because automatic refresh failed.

## 6. Implement the native window and DPI layout

- [ ] Start from the designer's 784 × 546 client area as a reference, defining a fixed logical layout at 96 DPI. Do not
      assume the WinForms font-scaled dimensions are a pixel-perfect DPI specification; modest changes are acceptable.
- [ ] Preserve this control arrangement:

```text
Stay Awake
  Require Display       Duration: [hh:mm:ss]
  Require System        Remaining Time: [value]

Window Closer
  Schedule Close Window After: [hh:mm:ss]          Process: [read-only value]
  Refresh List          Remaining Time: [value]   Handle:  [read-only value]
  Highlight Window      Window Position: [X, Y, Width, Height]
  [large, single-selection list of window titles                              ]
```

- [ ] Use a caption/system menu/minimize style without a resizable frame or maximize box. Enforce the chosen logical
      client size for normal sizing, accounting for the non-client frame at each DPI.
- [ ] Apply the selected D12 work-area fallback on small monitors, preserving the same control order and readable fonts.
      Keep the window reachable after monitor removal or a work-area change.
- [ ] Establish DPI awareness before creating any HWND. Use current window DPI, scaled layout constants, DPI-sized fonts
      and icons, and `AdjustWindowRectExForDpi` for the outer rectangle. Apply the font to all controls.
- [ ] Reuse one layout routine at creation and DPI changes. Handle `WM_DPICHANGED`, applying the suggested rectangle and
      rebuilding font/icon/layout resources. Consider `WM_GETDPISCALEDSIZE` if custom fixed-size rounding needs to
      influence that rectangle; avoid cumulative scaling and recursive DPI changes. Follow Microsoft's
      [Win32 DPI guidance](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows).
- [ ] Scale control spacing, rows, button widths, combo dropdown height, list item height, and status areas as well as
      the outer window. Measure long captions/details so all information remains available without clipping.
- [ ] Support keyboard traversal, visible focus, button activation, combo/list navigation, and copying read-only
      details. With a plain top-level window, explicitly provide dialog-style navigation such as `IsDialogMessageW`.
- [ ] Preserve single selection, a useful visible scrollbar, full Unicode titles, and accessible long list entries using
      horizontal extent or an equivalent native presentation. Do not use truncated display text as identity.
- [ ] Keep native rectangle coordinates in screen pixels for metadata and overlay positioning, including negative
      multi-monitor coordinates. Do not apply the main window's UI scale to a target rectangle a second time.
- [ ] Make error/status text readable in the fixed layout; preserve full details through a read-only field or tooltip if
      captions cannot fit. Avoid introducing a persistent history feature just to preserve one completion result.

## 7. Implement native services and lifecycle fixes

### Instance ownership, tray, and shutdown

- [ ] Acquire single-instance ownership atomically before starting application services; distinguish already-running
      from actual creation/access errors. Keep the ownership resource until teardown finishes.
- [ ] Create the hidden main window, controls, and tray before consuming Show requests. Do not briefly show then hide at
      startup. A request received during initialization must not be overwritten by a later initial-hide action.
- [ ] Handle the mutex-created/event-not-created interval with bounded retry or explicit readiness. Retain an already
      signaled event until the receiver is ready. Handle primary exit during activation without starting two owners,
      indefinite waiting, or silently disabling all future Show requests.
- [ ] If a worker listener is selected instead of the recommended combined loop, provide cancellation, join it before
      destroying the UI, and marshal through a valid native recipient. Do not recreate the old unobserved waiter.
- [ ] Add the `Stay Awake` tray icon, left-click Show, and right-click Show/Quit menu using `Shell_NotifyIconW` and
      native menus. Restore a minimized window and request foreground activation; handle Windows foreground restrictions
      without claiming activation is guaranteed under every policy.
- [ ] Re-add the tray icon after Explorer's `TaskbarCreated` message. If initial tray creation fails, show a usable
      window/error rather than leaving an unreachable hidden process.
- [ ] Handle user `WM_CLOSE` as hide-to-tray; keep scheduled operations alive and hide/disable highlight. Keep ordinary
      minimize behavior. Only actual shutdown destroys the window and exits the message loop.
- [ ] Accept `WM_QUERYENDSESSION`; complete cleanup on `WM_ENDSESSION(TRUE)` without blocking dialogs. If end-session is
      canceled (`FALSE`), continue running. Do not conflate these messages with close-to-tray. See Microsoft's
      [session query](https://learn.microsoft.com/en-us/windows/win32/shutdown/wm-queryendsession) and
      [session completion](https://learn.microsoft.com/en-us/windows/win32/shutdown/wm-endsession) contracts.
- [ ] Make Quit/confirmed session-end/startup-failure cleanup idempotent: prevent new callbacks, stop timers and pending
      schedules, release awake protection on its owning thread, remove overlay/tray, destroy native resources, then
      release instance ownership. Pending activation must not resurrect a closing window.

### Power, timer, window catalog, and overlay

- [ ] Implement `SetThreadExecutionState` using `ES_CONTINUOUS | ES_SYSTEM_REQUIRED`, plus `ES_DISPLAY_REQUIRED` for
      display mode; clear with `ES_CONTINUOUS` on the same thread. Check failure and return a useful operation result.
      Do not introduce away mode or simulated input. The API does not block explicit user sleep or screensavers; see
      [SetThreadExecutionState](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate).
- [ ] Drive controller ticks with a roughly one-second Win32 timer, running only while required by active features (and
      optional live highlighting if selected). Hidden windows must continue receiving ticks. Handle timer setup failure
      so an operation cannot remain active indefinitely without expiry processing.
- [ ] Implement the chosen duration clock and local timestamp formatting without adding Windows types to the core. If
      using biased interrupt time, verify declarations/import-library support in both MinGW environments.
- [ ] Enumerate with `EnumWindows` and retrieve Unicode title, visibility, shell identity, owning process, process name,
      and rectangle as needed. Use limited process-query rights; an inaccessible process name must not abort the entire
      list. Return the executable basename without extension to match `Process.ProcessName`.
- [ ] Tolerate windows disappearing or titles changing during enumeration. Distinguish whole-enumeration failure from
      unavailable metadata on a single window. Keep app eligibility rules and native querying responsibilities clear.
- [ ] Revalidate the captured target according to D6, then use asynchronous `PostMessageW(..., WM_CLOSE, ...)`. Return
      missing/changed target or posting failure honestly, including access-denied cases. Do not block on the foreign
      window, kill its process, dismiss its save prompts, or repeatedly resend after a successful request.
- [ ] Create the overlay as a red layered, borderless, topmost tool window at approximately 25% alpha. Use no-activate
      display/positioning and appropriate transparent hit behavior; verify actual click-through on Windows rather than
      relying on the old `WS_EX_TRANSPARENT` comment as proof.
- [ ] Apply target screen bounds directly; suppress invalid/zero-size rectangles and keep the overlay off the taskbar
      and out of Alt-Tab. Stop/destroy it on the appropriate application events. Reuse it safely across toggles.

## 8. Add application-only unit tests

Use the reference's `tests/application/application.test.cpp` fake-binding pattern. Tests must compile and run unchanged
on Linux and Windows, link only the portable core plus GoogleTest, and use fake clocks/results/events. Do not add Win32
unit tests, Windows headers, conditional test skips, real desktop interaction, or real sleeps to this suite. Platform
behavior is verified by builds and the separate Windows manual checks below.

- [ ] Duration generation: exact endpoints, 15-minute increments, appended 10-second choice, counts, formatted labels,
      and 2-hour/1-hour defaults; reject absent or invalid choices.
- [ ] Initial state: both features idle, default selections, correct captions, no selected window/overlay, hidden
      startup intent, and no unnecessary repeating timer.
- [ ] Awake starts in either mode; competing mode disabled; stop/expiry restores expected state; failed start does not
      leave an active deadline; failed release follows D11 and remains observable/retryable.
- [ ] Both timers active at once; stopping/expiring one leaves the other unaffected; timer-needed output becomes false
      only when nothing needs ticks. Include both deadlines expiring on one callback.
- [ ] Deadline edges: before/exactly at/after expiry, delayed/multiple ticks, restart after cancellation, immediate and
      rounded/truncated countdown presentation, and no duplicate power-release or close requests.
- [ ] Wall-clock jumps change completion timestamps but not remaining duration. Simulate the selected suspend policy
      through fake elapsed time; do not test actual OS clocks.
- [ ] List filtering policy if implemented in the core; distinct targets with identical titles; supplied ordering;
      selection/details clearing; process/rectangle fallback results; refresh failure; Unicode data round trips.
- [ ] Close without selection reports an error; scheduling captures identity; later list events cannot replace it;
      cancellation sends nothing; success/failure/missing/changed-target results preserve metadata and restore controls.
- [ ] A successful close request produces request wording, not verified-closure wording. Completion runs once and
      automatic refresh cannot erase the result or accidentally restart the schedule.
- [ ] Highlight toggles with/without selection, changes target on selection, and is cleared on refresh/hide/exit,
      including refresh failure. Unavailable geometry suppresses the overlay; no selection may retain highlight intent
      as specified above. Scheduled close does not disable the highlight action.
- [ ] Show/activation refreshes only when permitted, user close hides without canceling deadlines, Quit/confirmed
      session-end requests cleanup, and canceled session-end leaves the application running.
- [ ] User versus automatic error presentation and callback-after-shutdown handling. Exercise reentrant events where the
      fake can expose a meaningful duplicate-completion or lifetime regression.
- [ ] Test a portable composition root only if it contains meaningful behavior; do not mechanically copy the reference's
      renamed-`main` test trick when this project's entry point merely constructs and runs the controller.

## 9. Finish repository migration and contributor documentation

- [ ] Move `StayAwake/icon.ico` and `StayAwake/icon.png` into `assets/` without changing their content. Update the
      README image path and resource references before removing the original directory.
- [ ] Once the native implementation and portable tests are present, delete `StayAwake.slnx`,
      `StayAwake/StayAwake.csproj`, every C# source/designer file, and `MainForm.resx`. Extract/preserve any needed
      embedded resources first. Remove the now-obsolete `StayAwake/` directory; do not merely disable the old build.
- [ ] Replace the large .NET-focused `.gitignore` with relevant CMake, Node, editor, and local-file exclusions,
      following the reference. Preserve useful existing protections such as `.env`; ignore `build/`, local CMake
      presets, and root compile-command links. Review `.vscode/settings.json` for obsolete C# vocabulary/settings.
- [ ] Add `AGENTS.md` using the reference's Repository Map, Testing Requirements, and Common Commands pattern. Describe
      StayAwake's portable state/controller, platform factory/binding, Win32 services, resources, and DPI
      responsibilities.
- [ ] Explicitly require application-only portable unit tests and keep native APIs out of the core. Document npm/direct
      CMake workflows and link to the updated README. Explain that `CODE_REVIEW_*.md` describes the prior implementation
      and remains historical context, rather than an obligation to complete all findings in this port.
- [ ] Rewrite README requirements and build/test/format/output/install instructions for Windows 11 x64, CMake, Ninja,
      MinGW-w64, optional Node/npm, clang-format, and pinned GoogleTest fetching. Include both Linux and Windows
      presets.
- [ ] Remove .NET SDK/runtime, NuGet, `dotnet publish`, `dotnet format`, and old `bin/.../publish` instructions from
      active documentation/scripts. Document new output `build/<windows-release-preset>/StayAwake.exe`, with the actual
      preset names, and direct commands for users who do not use npm.
- [ ] Describe hidden startup, tray Show/Quit, both timer ranges/defaults and 10-second choices, chosen suspend policy,
      highlighting behavior, selected-window close requests, and any intentionally changed wording/filtering.
- [ ] Describe sleep prevention accurately; remove the unsupported promise that this application prevents screen
      locking. Do not add simulated input or security-policy changes to make that old sentence true.
- [ ] Preserve all three review documents without rewriting their historical source links. Note intentional differences
      and deferred alternatives in this TODO or implementation notes instead.
- [ ] Audit tracked files for stale .NET build/runtime references. Expected historical mentions in `CODE_REVIEW_*.md`
      and this migration plan are not a reason to delete those documents.

## 10. Validate builds and native behavior

### Build and repository checks

- [ ] Configure/build/test `linux-native-debug` successfully without Windows headers/libraries or a .NET SDK.
- [ ] Build `linux-mingw-debug` and `linux-mingw-release`, including resources and the GUI executable. Confirm cross
      configuration/build does not try to execute the generated Windows test binary.
- [ ] On Windows, build Debug/Release with the documented MinGW environment and run `windows-mingw-debug` CTest.
- [ ] Run `npm run build`, `npm run test`, `npm run format:check`, and any retained individual build shortcuts. Verify
      direct CMake commands too where not already exercised by those scripts.
- [ ] Inspect the PE architecture, GUI subsystem, imports, embedded icon, and DPI manifest using suitable toolchain
      inspection tools. Confirm no CLR dependency and no unshipped MinGW runtime DLL dependencies.
- [ ] Launch the Release executable from an otherwise empty folder on Windows without .NET installed. Confirm no console
      flash and no missing adjacent assets/runtime files. Exercise `cmake --install` if an install rule is added.
- [ ] Review dependency direction and includes: the test target must not depend on or link the Windows adapter. The
      Linux native preset must not compile it; Windows presets can build it separately for the app. No Win32 types or
      platform gates should have slipped into portable application tests.
- [ ] Check formatting, `git diff --check`, icon preservation, removal of the .NET project, updated documentation, and
      preservation of all three review documents. Do not run a blanket formatter that rewrites those historical files.

### Windows manual acceptance matrix

Use the existing 10-second choices and harmless disposable target windows for fast verification. These are manual
platform checks, not a second unit-test suite.

| Scenario                                                                           | Required result                                                                                                                                                           |
| ---------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| First launch, tray click, Show, minimize, X, Alt-F4, Quit                          | Hidden startup with one tray icon; Show restores; minimize remains ordinary minimize; close hides and stops highlight; Quit exits and removes the icon.                   |
| Several near-simultaneous launches; launches during startup and quit               | One owner, no permanently lost activation listener, no crash/deadlock, and successful Show when a receiver is available; bounded failures are understandable.             |
| Normal/elevated launches and separate interactive sessions                         | Match the chosen activation scope/permissions; separate sessions do not incorrectly block each other.                                                                     |
| Explorer restart                                                                   | Tray icon returns and remains functional.                                                                                                                                 |
| Require Display and Require System                                                 | Correct requests, captions, disabled controls, live countdown, manual stop, timed expiry, and retained completion mode/time. Inspect Windows power requests where useful. |
| Both features concurrently, including while hidden                                 | Each expires/cancels independently; hiding does not stop either timer.                                                                                                    |
| Refresh, selection, duplicate/non-ASCII/long titles, inaccessible process metadata | Stable target selection, sorted eligible list, correct process/hex handle/rectangle display, usable long text, and appropriate fallback/errors.                           |
| Close cancellation, expiry, already-gone target, changed owner, denied posting     | No request on cancel; one guarded request on expiry; truthful result and restored controls; no substituted target.                                                        |
| Target with an unsaved-document prompt or unresponsive UI                          | StayAwake remains responsive and reports a request rather than claiming closure; no forced termination.                                                                   |
| Highlight toggle, selection, missing/invalid geometry, main-window hide            | Red translucent overlay at correct bounds, real click-through, no focus theft/taskbar entry, and appropriate cleanup; tracking matches D7.                                |
| OS sign-out/shutdown and canceled end-session                                      | No close-to-tray veto or modal shutdown error; cleanup on confirmed exit; canceled session termination leaves the app usable.                                             |
| Manual clock/time-zone changes and suspend/resume                                  | Durations follow D4, timestamps remain local, and overdue actions run once after resume.                                                                                  |
| Launch at 100%, 125%, 150%, 200%, and an available higher scale                    | Fixed logical layout, scaled frame/fonts/controls/icons, readable labels/status, copyable details, usable lists/dropdowns.                                                |
| Drag across monitors with different DPI; change scale while running/hidden         | Proper resize/re-layout without clipping, drift, or DPI loops; restore from tray at the correct scale.                                                                    |
| Mixed-DPI target windows and monitors left/above the primary display               | Metadata and overlay use the correct screen coordinates, including negative X/Y, without double scaling.                                                                  |
| Keyboard-only use and long repeated show/hide/highlight cycles                     | Usable tab/focus/navigation behavior; no growing collection of tray icons, overlays, GDI objects, or stale callbacks.                                                     |

- [ ] Record commands/environments actually used, passing checks, any deliberate differences from the baseline, and
      remaining limitations. A successful Linux cross-build is not evidence that Windows UI/DPI behavior passed.
- [ ] Mark the migration complete only after the native build, portable tests, required native acceptance checks,
      documentation, cleanup, and preservation requirements above are satisfied. Leave unavailable checks explicitly
      pending for execution on the appropriate Windows environment.
