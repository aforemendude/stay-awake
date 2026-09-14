# Windows runtime review

## Scope and basis

Reviewed service lifecycle and adapter methods in `src/platform/windows/windows_platform_binding.*`,
`single_instance.*`, `power_manager.*`, `clock.*`, `window_catalog.*`, `native.*`, and
`platform_factory.cpp`, together with the main-window timer/callback paths and the portable platform contract,
at commit `5f80b0ca3120d25f5304f9a2dc110cbf71806617`.
The worktree was clean before report creation. UI presentation and build configuration have separate reports.

## Findings

No verified findings in this segment. This does not establish that the code is defect-free.

## Review evidence

- Traced single-instance mutex ownership, bounded startup handoff, activation-event delivery, and ownership
  retention through native teardown.
- Checked the UI-thread message pump, bounded queue drain, one-shot consumption of native timers,
  callback exception capture, quit delivery, callback detachment, and immediate session-end cleanup.
- Checked execution-state acquisition/release and best-effort release retries on teardown.
- Checked biased interrupt-time selection against Microsoft's distinction between elapsed time and
  unbiased time that excludes sleep/hibernation. [Interrupt time documentation](https://learn.microsoft.com/en-us/windows/win32/sysinfo/interrupt-time)
- Traced window enumeration/filtering, UTF-8 conversion, process metadata, identity validation, geometry,
  and asynchronous `WM_CLOSE` delivery. The documented same-process HWND-reuse and validation/posting
  races remain explicit limitations; they are not represented as guaranteed targeting.
- Checked owned and borrowed native-resource lifetimes and timestamp conversion/error paths.

## Unresolved questions

None identified from static tracing. OS integration remains unverified without a Windows session.

## Checks and limitations

Static review and a fresh Windows Release cross-build completed successfully with GCC 13-win32.
PE inspection confirmed x64/Windows GUI, Windows-only DLL/API-set imports, embedded resources,
and ASLR/NX flags. Build details and the incremental-resource finding are in
`CODE_REVIEW_BUILD_AND_TOOLING.md`.
Manual Windows checks remain pending for same-elevation second-launch activation, initialization failures,
power acquisition/release, suspend and hibernate resume, shutdown cancellation/confirmation, and shutdown
inside modal loops. Cross-compilation does not validate these behaviors.
No Windows binaries were executed on this Linux host. Dependency source, individual test cases,
fixture data, and coverage adequacy are excluded.
