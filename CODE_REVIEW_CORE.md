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
- Added process creation-time capture and validation on September 11, 2026. Targeting remains explicitly best effort.

## Findings

### CORE-1: A delayed close can target a different window after handle reuse

**Severity:** Medium

**Status:** Partially mitigated with process creation-time validation; remaining window-reuse risk documented.

**Locations:** [WindowIdentity](include/stay_awake/application_state.hpp#L25),
[target validation and close posting](src/platform/windows/window_catalog.cpp#L110), and
[captured close schedule](src/application/close_controller.cpp#L32).

**Original locations:** [StayAwake/Core/WindowCloser.cs:71–76](StayAwake/Core/WindowCloser.cs#L71),
[StayAwake/Core/WindowCloser.cs:86–90](StayAwake/Core/WindowCloser.cs#L86). Caller:
[StayAwake/Forms/MainForm.cs:357–362](StayAwake/Forms/MainForm.cs#L357).

**Mitigation:** The C++ implementation captures the window handle, process ID, and full 64-bit process creation time,
preserves that scheduled identity, and validates it before posting `WM_CLOSE` or querying highlight geometry. Process
creation time and name are read through the same process handle during enumeration. The creation time comes from
[`GetProcessTimes`](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes)
using `PROCESS_QUERY_LIMITED_INFORMATION`. A changed creation time rejects a recycled PID belonging to a different
process lifetime. Missing captured creation time, failure to reopen/query the process, a missing window, or a changed
owner also rejects the operation. Windows with unavailable creation times remain listed; validation never falls back to
handle/PID alone.

**Remaining problem:** These checks do not distinguish a replacement window within the same process lifetime. No
window-destruction tracking is retained, and validation and posting are separate operations. Windows can recycle a
destroyed window's handle for a different window; this behavior is explicitly documented in Microsoft's
[IsWindow reference](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindow).

**Impact:** Window targeting is best effort and is not guaranteed to be correct. If the selected window closes before
the deadline and its handle is reused within the same process lifetime, the timer can request closure of a different
window while displaying the original target metadata. A replacement between validation and posting also remains
possible. These consequences follow from the current call path; they were not reproduced on Windows here.

**Documented behavior:** The [user guide](README.md#window-closer) describes the creation-time checks and explicitly
warns that they cannot guarantee the original window receives the close request. Window-destruction tracking and
stronger coordination with the target application are outside this mitigation's scope; the remaining finding is retained
because process creation time identifies a process lifetime, not an individual window lifetime.

## Unresolved questions

- None needed to establish the finding. The frequency of handle reuse in the user's typical applications was not
  measured.

## Checks and limitations

- Traced enumeration, captured schedules, ownership/creation-time validation, and posting. The native close helper
  explicitly documents the remaining same-process handle-reuse and validation/posting race.
- Portable tests cover preservation of the full creation-time value through selection and scheduling, identities with
  matching handles/PIDs but different creation times, and failed requests reported without retry. They use the fake
  binding and do not exercise native handle reuse or validate Win32 ownership checks.
- `npm run test` passed all 46 portable tests on Linux. `npm run build` cross-compiled the Windows Debug application and
  tests and the Release application successfully; Windows binaries were not run on Linux.
- Pending manual Windows checks: an unchanged target still receives a close request; missing/changed creation times and
  denied process queries prevent posting and highlighting; same-process window reuse remains a documented limitation.
  Windows window-lifecycle experiments and power-policy checks remain unperformed. This reassessment checked the
  existing finding, rather than conducting a new full review of the adapter.
