# Portable application review

## Scope and basis

Reviewed `src/application/`, all public headers in `include/stay_awake/`, and `src/main.cpp` at commit
`b2cf743bec38653f9a0c47ea0cee585b01bd7791`, starting from a clean worktree. Review date: 2026-09-11 (UTC).
The basis was the current implementation, `AGENTS.md`, documented behavior in `README.md`, and the Windows adapter's
implementation of the platform contract.

The review traced event serialization, immediate shutdown, sleep-prevention start/stop and release retry, countdown
expiry, captured close targets, duration validation, selection refresh, highlighting, presentation snapshots, and
error reporting. Individual test implementations and coverage adequacy were excluded.

## Findings

No actionable findings were verified in this segment. This does not establish that the portable application is
defect-free.

Scheduled closes retain their captured target and consume the schedule before issuing a close request. Failed power
release retains the active mode for manual retry without repeated timer retries. Timer-start failure cancels schedules
and attempts to release power. Ordinary reentrant events are queued, while quit and confirmed session end take the
immediate cleanup path. Portable code contains no Win32 dependencies or platform gates.

## Checks

- Traced each application event through its feature controller, platform operation, and published view state.
- Checked cancellation, invalid duration/selection, expired deadlines, timer failure, catalog failure, and power-release
  failure paths in production code.
- Verified that duration calculations use injected elapsed time and local timestamps are used for displayed results.
- The existing native Linux build graph reported no work to do; all 46 existing tests passed when run directly with
  `build/linux-native-debug/tests/stay_awake_tests --gtest_brief=1`. No new tests or repository implementation changes
  were made.

## Unresolved questions

None requiring a change were established within this segment.

## Limits and residual risks

The platform contract permits modal reentrancy. As explicitly required by the repository, ordinary events, including
timer ticks, wait for the current modal error presentation to return; shutdown is the immediate exception. Native
message-delivery behavior and OS teardown cannot be established by the Linux test run.

Window identity is deliberately best effort: handle reuse within the same process and replacement between validation
and posting remain possible. The code comment and README accurately disclose that limitation, so it is not reported
as a defect. A successful close request is correctly described as queued, not as verified closure.

Actual Windows power release, suspend/resume, and shutdown checks remain pending on Windows and are also recorded in
the Windows services report.
