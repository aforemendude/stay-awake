# Code review: native power and window operations

## Scope and review basis

- Reviewed `StayAwake/Core/NativeMethods.cs`, `PowerManager.cs`, and `WindowCloser.cs`, with their callers in
  `StayAwake/Forms/MainForm.cs`.
- Basis: the clean worktree at commit `b8c723401dabd4ffb442f9ba0c56ab0cfa7ea890`, reviewed on September 9, 2026. No
  applicable `AGENTS.md` files were found.
- Checked execution-state flags and thread ownership, native signatures and return handling, window enumeration, process
  metadata, and scheduled close targeting. Review complete for this segment.

## Findings

### CORE-1: A delayed close can target a different window after handle reuse

**Severity:** Medium

**Locations:** [StayAwake/Core/WindowCloser.cs:71–76](StayAwake/Core/WindowCloser.cs#L71),
[StayAwake/Core/WindowCloser.cs:86–90](StayAwake/Core/WindowCloser.cs#L86). Caller:
[StayAwake/Forms/MainForm.cs:357–362](StayAwake/Forms/MainForm.cs#L357).

**Problem:** Enumeration stores the window handle, title, and process name, but discards the process ID. The scheduler
retains that snapshot for up to eight hours and later sends `WM_CLOSE` to its handle without checking whether it still
belongs to the selected window. Windows can recycle a destroyed window's handle for a different window; this behavior is
explicitly documented in Microsoft's
[IsWindow reference](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow).

**Impact:** If the selected window closes before the deadline and its handle is reused, the timer can close an unrelated
window and report the original process name. This can interrupt unrelated work; it is more serious than merely failing
to close a stale selection. This consequence follows from the current call path and documented handle reuse; it was not
reproduced on Windows here.

**Recommendation:** Retain the owning process ID and process lifetime identity when selecting a target, revalidate
ownership immediately before posting, and invalidate the schedule when the selected window is destroyed. Account for
reuse within the same process as well. A bare `IsWindow` check or comparison of process names alone does not establish
that the original window is still present.

## Unresolved questions

- None needed to establish the finding. The frequency of handle reuse in the user's typical applications was not
  measured.

## Checks and limitations

- Traced all native declarations and their production call sites. Start and stop of the execution-state request both
  occur on the WinForms UI thread; the flags match the documented
  [SetThreadExecutionState contract](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate).
- Verified that posting failures are surfaced by `WindowCloser.CloseWindow`. Whether successful posting means a
  completed closure is addressed in `CODE_REVIEW_INTERFACE_TIMERS.md`.
- This environment is Linux, and `dotnet`, `csc`, and `mcs` are unavailable. No compilation, native execution,
  power-policy checks, or Windows window-lifecycle experiments were possible. Dependencies were not installed or
  changed.
- No additional verified findings were identified in the power-management wrapper or native declarations. That does not
  establish that they are defect-free.
