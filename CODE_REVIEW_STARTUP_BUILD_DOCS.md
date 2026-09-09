# Code review: startup, build configuration, and documentation

## Scope and review basis

- Reviewed `StayAwake/Program.cs`, `StayAwake/StayAwake.csproj`, `StayAwake.slnx`, `README.md`, `.gitignore`, and
  `.vscode/settings.json`. Traced the startup listener in `StayAwake/Forms/MainForm.cs` as part of the single-instance
  contract.
- Checked the license file and image/resource references for repository context. Image artwork and generated resource
  payloads were outside the code-review scope; resource structure and referenced-file integrity were checked.
- Basis: the clean worktree at commit `b8c723401dabd4ffb442f9ba0c56ab0cfa7ea890`, reviewed on September 9, 2026. No
  applicable repository instructions were found.
- Review complete for this segment, including dependency and test-infrastructure inventory.

## Findings

### STARTUP-1: An early second launch can permanently stop the activation listener

**Severity:** Medium

**Location:** [StayAwake/Forms/MainForm.cs:21–33](StayAwake/Forms/MainForm.cs#L21). Startup order:
[StayAwake/Program.cs:32–37](StayAwake/Program.cs#L32); secondary-instance signal:
[StayAwake/Program.cs:19–20](StayAwake/Program.cs#L19).

**Problem:** The named event is available before the main form is constructed, and the constructor starts its background
waiter before `Application.Run` has created/shown the form. If a second launch signals the event during this interval,
the waiter immediately calls `Invoke` on a form whose native handle may not yet exist. Microsoft explicitly identifies
this constructor/`Application.Run(new MainForm())` ordering in the
[InvokeRequired guidance](https://learn.microsoft.com/en-us/dotnet/api/system.windows.forms.control.invokerequired?view=windowsdesktop-10.0);
[Invoke throws when no suitable handle exists](https://learn.microsoft.com/en-us/dotnet/api/system.windows.forms.control.invoke?view=windowsdesktop-10.0).

**Impact:** The exception exits the waiter task. Because its task is neither retained nor observed and the loop has no
exception recovery, later launches still signal the named event and exit, but no listener remains to show the first
instance. The existing app remains usable through its tray icon, so this is a persistent failure of the advertised
secondary-launch activation workflow rather than a claimed process crash. The interleaving is established from
initialization order and documented API behavior; its occurrence rate was not measured on Windows.

**Recommendation:** Start consuming activation requests after UI handle creation and initial show/hide processing have
completed, preserving any already-signaled event until then. Retain and observe the listener task and give it a
cancellation/disposal path so lifecycle exceptions cannot silently disable activation. Replacing `Invoke` with
`BeginInvoke` alone does not remove the handle-readiness requirement.

### DOCS-1: The README promises screen-lock prevention that the implementation does not provide

**Severity:** Low

**Location:** [README.md:17](README.md#L17). Implementation:
[StayAwake/Core/PowerManager.cs:13–17](StayAwake/Core/PowerManager.cs#L13) and
[StayAwake/Core/PowerManager.cs:27](StayAwake/Core/PowerManager.cs#L27).

**Problem:** The feature list says the utility prevents screen locking due to inactivity. Its implementation requests
system/display availability through `SetThreadExecutionState`; Microsoft documents that this API
[does not stop the screen saver](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate).
Windows also supports inactivity policies that lock the session through the screen saver, as described in the
[machine-inactivity policy reference](https://learn.microsoft.com/cs-cz/windows/client-management/mdm/policy-csp-LocalPoliciesSecurityOptions?WT.mc_id=Portal-fx#interactivelogon_machineinactivitylimit).
No separate screen-lock prevention mechanism exists in the reviewed source.

**Impact:** A user relying on the advertised feature can still encounter a locked session during an active stay-awake
period. Keeping the display powered and keeping a session unlocked are distinct behaviors, so the unconditional claim
misstates a material capability.

**Recommendation:** Describe the actual idle-sleep and optional display-power behavior, and remove the guarantee about
preventing screen locking. Document that screen-saver and session-lock policies can still apply.

## Unresolved questions

- The README links externally distributed release builds, but their contents and build provenance are not present in
  this repository. This review does not establish whether those binaries match the reviewed commit or contain all
  framework-dependent output files.

## Checks and limitations

- Parsed the project, solution, and `.resx` XML successfully; verified the solution project path, icon path, README
  image path, and editor-settings JSON.
- Validated the standalone and embedded ICO directory headers and image byte ranges without executing/deserializing
  embedded objects. Both checks passed.
- The project has no `PackageReference` or `ProjectReference` entries. Its `net10.0-windows`, Windows Forms, executable
  output, and unsafe-code settings are internally consistent with the source and documented runtime requirement. No
  verified dependency/configuration defect was identified in those declarations; a successful build was not established.
- No test projects, test-runner configuration, CI workflows, repository build scripts, or shared MSBuild/SDK override
  files were found. Their absence is recorded as inventory, not as a test-coverage or missing-test finding. Individual
  test cases and coverage adequacy were outside this review.
- `dotnet`, `csc`, and `mcs` are unavailable, and the host is Linux. Debug/Release builds, source-generated interop
  compilation, framework-dependent deployment, Windows single-instance races, foreground activation, and screen-lock
  behavior were not executed. No dependencies were installed, updated, or repaired.
- Source/configuration files were not changed. Only the three review reports were added.
