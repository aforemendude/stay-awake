# Windows services review

## Scope and basis

Reviewed `src/platform/windows/windows_platform_binding.*`, `platform_factory.cpp`, `native.*`, `clock.*`,
`power_manager.*`, `single_instance.*`, and `window_catalog.*`, with related application and UI call sites, at commit
`b2cf743bec38653f9a0c47ea0cee585b01bd7791`. The initial worktree was clean. Review date: 2026-09-11 (UTC).

The review traced startup and secondary launches, the message/event loop, callback failures, quit and session-end
cleanup, handle lifetime, power requests, clocks, UTF-8 conversion, window enumeration, process identity validation,
and asynchronous close requests. It used current source, repository contracts, and Microsoft API documentation.

## Findings

No actionable findings were verified in this segment. This is not a guarantee of correct behavior on every Windows
configuration.

## Checks and observations

- Traced startup success, secondary-instance return, initialization failure, callback failure, normal quit, and
  confirmed/canceled session-end paths. `RunService` delivers quit before normal teardown; cleanup detaches the
  handler and tray pointer before destroying the main window. Member destruction order retains instance ownership
  through resource and power cleanup.
- Checked that confirmed session end releases important resources inside the callback rather than depending on a
  later message-loop iteration. This is consistent with the OS termination timing described in
  [WM_ENDSESSION documentation](https://learn.microsoft.com/en-us/windows/win32/shutdown/wm-endsession).
- Checked power flag combinations and cleanup retry. The adapter uses `ES_CONTINUOUS` with the applicable system/display
  requirements and clears them on the same thread. Its generic failure message avoids treating an undocumented
  extended error code as authoritative.
  [SetThreadExecutionState documentation](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate)
- Verified interrupt-time units and the choice of biased elapsed time, separate from local result timestamps.
  [Interrupt time documentation](https://learn.microsoft.com/en-us/windows/win32/sysinfo/interrupt-time)
- Traced enumeration filtering, title conversion and sorting, process-name fallback, PID/creation-time validation,
  and the final single `PostMessageW(WM_CLOSE)` call. Posting is asynchronous and subject to Windows integrity-level
  restrictions; failure is returned to the application.
  [PostMessageW documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew)
- Reviewed native callback exception containment and nonthrowing teardown paths. Window enumeration captures exceptions
  inside the callback and propagates them only after the native enumeration returns.
- Existing MinGW Debug and Release build graphs reported no work to do. No Windows executable was executed on Linux.

## Unresolved questions

None established as a separate correctness concern beyond the pending native checks.

## Limits and residual risks

Windows 11 runtime validation was unavailable. Pending checks include concurrent secondary launches, elevated and
unelevated launches, crash/shutdown handoff, real power acquisition/release, suspend and hibernate resume, denied close
requests, Explorer recovery, and session end while a native menu or error dialog is active. Source inspection and
existing cross-build outputs do not replace those checks.

Target validation cannot eliminate races with window replacement or handle reuse. The implementation and README
accurately acknowledge this; it is not reported as a new finding.
[IsWindow documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow)

No exhaustive resource-exhaustion fault injection, fresh toolchain build, or third-party dependency-source audit was
performed. The UI-specific focus finding and desktop validation details are in `CODE_REVIEW_WINDOWS_UI.md`.
