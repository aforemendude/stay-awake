# Code review: native power and window operations

## Scope and review basis

- Originally reviewed `StayAwake/Core/NativeMethods.cs`, `PowerManager.cs`, and `WindowCloser.cs`, with their callers in
  `StayAwake/Forms/MainForm.cs`.
- Basis: the clean worktree at commit `b8c723401dabd4ffb442f9ba0c56ab0cfa7ea890`, reviewed on September 9, 2026. No
  applicable `AGENTS.md` files were found.
- Checked execution-state flags and thread ownership, native signatures and return handling, window enumeration, process
  metadata, and scheduled close targeting. Review complete for this segment.
- Rechecked the finding against the C++ implementation at commit `30dd6c5` on September 11, 2026. The remaining issue
  and current source locations are described below; original source links are retained for historical context.

## Findings

### CORE-1: A delayed close can target a different window after handle reuse

**Severity:** Medium

**Status:** Partially mitigated; still open.

**Locations:** [WindowIdentity](include/stay_awake/application_state.hpp#L25),
[target validation and close posting](src/platform/windows/window_catalog.cpp#L98), and
[captured close schedule](src/application/close_controller.cpp#L32).

**Original locations:** [StayAwake/Core/WindowCloser.cs:71–76](StayAwake/Core/WindowCloser.cs#L71),
[StayAwake/Core/WindowCloser.cs:86–90](StayAwake/Core/WindowCloser.cs#L86). Caller:
[StayAwake/Forms/MainForm.cs:357–362](StayAwake/Forms/MainForm.cs#L357).

**Problem:** The C++ implementation captures the window handle and process ID, preserves the scheduled target, and
checks `IsWindow` and `GetWindowThreadProcessId` before posting `WM_CLOSE`. These checks reject missing windows and
different-PID owners, but do not distinguish a replacement window within the same process or a recycled process ID. No
process lifetime identity or window-destruction tracking is retained, and validation and posting are separate
operations. Windows can recycle a destroyed window's handle for a different window; this behavior is explicitly
documented in Microsoft's
[IsWindow reference](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow).

**Impact:** If the selected window closes before the deadline and its handle is reused with the same PID, the timer can
request closure of a different window while displaying the original target metadata. A replacement between validation
and posting also remains possible. These consequences follow from the current call path; they were not reproduced on
Windows here.

**Recommendation:** Extend target identity with process lifetime information and invalidate the schedule when the
selected window is destroyed. Account for reuse within the same process and the race between validation and posting. The
existing handle/PID checks alone do not establish that the original window is still present.

## Unresolved questions

- None needed to establish the finding. The frequency of handle reuse in the user's typical applications was not
  measured.

## Checks and limitations

- Traced the current enumeration, captured schedule, ownership validation, and posting paths. The native close helper
  explicitly documents the remaining same-process handle-reuse and validation/posting race.
- `npm run test` passed all 45 portable tests on Linux. Captured-target and failed-request tests exercise controller
  behavior through the fake binding; they do not exercise native handle reuse or validate Win32 ownership checks.
- Windows window-lifecycle experiments and power-policy checks remain unperformed. This reassessment checked the
  existing finding, rather than conducting a new full review of the adapter.
