# Application core review

## Scope and basis

Reviewed `src/main.cpp`, `src/application/`, and `include/stay_awake/` at commit
`5f80b0ca3120d25f5304f9a2dc110cbf71806617`. The worktree was clean before report creation.
The review covers application dispatch, duration calculations, power-session state, captured close targets,
selection/highlighting state, and the platform contract, with `AGENTS.md` and usage documentation as context.

## Findings

No verified findings in this segment. This does not establish that the code is defect-free.

## Review evidence

- Traced event queuing, immediate reentrant shutdown, deferred presentation dialogs, and service teardown.
- Checked duration validation, rounded countdown scheduling, independent deadlines, cancellation on timer failure,
  and retention of failed power release for manual retry.
- Checked that active close schedules retain captured window metadata and ignore list refresh/selection events.
- Checked the portable boundary for Windows headers, APIs, and platform-dependent types.

## Unresolved questions

None identified in the core logic. Native callback and presentation behavior is covered by the Windows reports.

## Checks and limitations

Static tracing and a fresh Linux Debug build completed. The newly built native test executable passed all
85 tests across seven suites. The existing native CTest preset also ran all 85 registered tests successfully.
The fresh build used `/tmp/stay-awake-review-native-20260913` and the already available GoogleTest sources.
Individual test cases, assertions, fixture data, and coverage adequacy are excluded by the review skill.
The intentional queuing of ordinary events during modal presentation also defers timer processing until that
presentation returns; immediate quit and confirmed session-end use the separate shutdown path.
Windows runtime and UI checks belong to the corresponding segment reports.
