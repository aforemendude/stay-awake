# Code review: interface, timers, and application lifecycle

## Scope and review basis

- Reviewed `StayAwake/Forms/MainForm.cs` and `StayAwake/Forms/OverlayForm.cs`. Read the designer wiring and resource
  metadata to establish control defaults, event subscriptions, timer configuration, and disposal behavior; generated
  layout/resource content was not independently reviewed as production logic.
- Basis: the clean worktree at commit `b8c723401dabd4ffb442f9ba0c56ab0cfa7ea890`, reviewed on September 9, 2026.
- Traced duration selection, start/stop transitions, both timer branches, window selection and refresh, highlighting,
  tray restoration, and exit handling. Review complete for this segment. The startup event listener is reported
  separately in `CODE_REVIEW_STARTUP_BUILD_DOCS.md`.

## Findings

### UI-1: Local clock changes shorten or extend both duration timers

**Severity:** Medium

**Locations:** [StayAwake/Forms/MainForm.cs:199](StayAwake/Forms/MainForm.cs#L199),
[StayAwake/Forms/MainForm.cs:278](StayAwake/Forms/MainForm.cs#L278),
[StayAwake/Forms/MainForm.cs:326–333](StayAwake/Forms/MainForm.cs#L326), and
[StayAwake/Forms/MainForm.cs:353–355](StayAwake/Forms/MainForm.cs#L353).

**Problem:** Both features construct their deadline from `DateTime.Now.Add(duration)` and compare it with subsequent
local wall-clock readings. Daylight-saving transitions, time-zone changes, and manual/system clock adjustments therefore
change the effective duration. `DateTime` subtraction does not normalize offsets; see Microsoft's
[DateTime subtraction documentation](https://learn.microsoft.com/en-us/dotnet/api/system.datetime.op_subtraction?view=net-10.0).

**Impact and verification:** A focused Python calculation modeled the same local-time arithmetic using the installed
`America/New_York` time-zone data. A 30-minute timer started at 01:50 on March 8, 2026 has a stored deadline of 02:20;
after only ten actual minutes, the clock reads 03:00 and the code treats it as expired. At the November 1 fallback, a
30-minute timer started at the first 01:50 still has 60 displayed minutes left after 30 actual minutes. Consequently, a
target window can close early, wake protection can end early, or either feature can continue substantially longer than
selected. This was an arithmetic reproduction, not execution of the C# application.

**Recommendation:** Measure the selected duration with a monotonic elapsed-time source and derive the countdown from
elapsed time. Keep local wall time for completion timestamps. Decide explicitly how elapsed time should include system
suspend; switching only to UTC avoids DST changes but does not protect against system-clock corrections.

### UI-2: The tray-close handler also cancels operating-system shutdown

**Severity:** Medium

**Location:** [StayAwake/Forms/MainForm.cs:393–398](StayAwake/Forms/MainForm.cs#L393). Related explicit-exit path:
[StayAwake/Forms/MainForm.cs:422–425](StayAwake/Forms/MainForm.cs#L422).

**Problem:** Every `FormClosing` event is canceled unless the tray's Quit command previously set `_isExplicitClose`. The
handler never examines `e.CloseReason`, so a Windows shutdown or sign-out follows the same cancel-and-hide branch as
clicking the title-bar X. WinForms distinguishes operating-system closure with
[`CloseReason.WindowsShutDown`](https://learn.microsoft.com/en-us/dotnet/api/system.windows.forms.closereason?view=windowsdesktop-10.0),
and setting `Cancel` prevents normal form closure under the
[FormClosing contract](https://learn.microsoft.com/en-us/dotnet/api/system.windows.forms.form.formclosing?view=windowsdesktop-10.0).

**Impact:** A resident tray utility objects to ordinary session termination even when no timer is active. Depending on
Windows shutdown handling, it can delay or interrupt shutdown/sign-out or require the user to force termination. The
exact Windows 11 prompt/timeout behavior was not exercised here; the unconditional cancellation is verified in the
source. Microsoft's
[session-ending message documentation](https://learn.microsoft.com/en-us/windows/win32/shutdown/wm-queryendsession)
describes the consequences of an application rejecting session termination.

**Recommendation:** Apply cancel-and-hide only to the intended user-close reason. Allow operating-system shutdown and
actual application exit to proceed, with the usual timer, overlay, and listener cleanup.

### UI-3: A queued close request is reported as a completed closure

**Severity:** Medium

**Location:** [StayAwake/Forms/MainForm.cs:359–370](StayAwake/Forms/MainForm.cs#L359). Native wrapper:
[StayAwake/Core/WindowCloser.cs:86–90](StayAwake/Core/WindowCloser.cs#L86).

**Problem:** As soon as `CloseWindow` returns, the interface records `Closed`, stops the schedule, and refreshes the
list. The wrapper only checks whether `PostMessage(WM_CLOSE)` succeeded.
[PostMessage returns before the target processes the request](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagew);
a target can also display a confirmation dialog and retain its window under the documented
[WM_CLOSE behavior](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-close).

**Impact:** An application awaiting confirmation, declining closure, or failing to process messages can remain open
indefinitely while Stay Awake reports a successful closure and has stopped monitoring it. This makes the scheduled-close
result unreliable for unattended use. The distinction is established by the API contract and the immediate success
assignment, without assuming that every target refuses closure.

**Recommendation:** Report `Close requested` after successful posting. If the product promises confirmation of closure,
observe disappearance of the original window with a bounded wait and distinguish completed, timed-out, and failed
outcomes while preserving the target application's confirmation behavior. Apply the identity safeguards in CORE-1 when
checking the target.

## Unresolved questions

- Should a duration include time spent suspended? The current wall-clock design counts it, but the repository does not
  state that contract. This affects the choice of replacement clock in UI-1.
- Is highlighting intended to be a snapshot or to follow the target? Bounds are updated only when the selection handler
  runs (`MainForm.cs:122–173`), so moving or resizing a target leaves the overlay at its old bounds. Without a
  documented tracking promise or a Windows interaction check, this is recorded as a behavior question rather than a
  verified requirement violation.

## Checks and limitations

- Completed source tracing of all interface handlers and timer transitions. Verified event subscriptions and the
  one-second WinForms timer configuration against the designer.
- Ran the focused DST arithmetic reproduction described in UI-1; both forward and backward clock transitions demonstrate
  incorrect duration accounting.
- Parsed the `.resx` XML and inspected its metadata and resource reference. No malformed XML was found.
- Linux and the absence of `dotnet`, `csc`, and `mcs` prevent building or running WinForms here. Shutdown interaction,
  actual window-close confirmations, DPI scaling, screen-reader behavior, keyboard navigation, overlay hit testing, and
  multiple-monitor rendering remain untested.
- No dependencies, tests, snapshots, or generated artifacts were installed or changed. Review writing is the only
  repository modification.
