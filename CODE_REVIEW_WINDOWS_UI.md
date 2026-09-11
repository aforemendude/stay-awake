# Windows UI review

## Scope and basis

Reviewed `src/platform/windows/main_window.*`, `tray_icon.*`, `overlay_window.*`, `dpi.*`, the manifest and resource
files, and their application/binding call sites at commit `b2cf743bec38653f9a0c47ea0cee585b01bd7791`. The worktree was
clean before review reports were created. Review date: 2026-09-11 (UTC).

The review covers native controls, event routing, focus and keyboard access, tray recovery, DPI layout, overlay
geometry and input transparency, native resource ownership, and the corresponding README descriptions. Findings are
based on source tracing and Microsoft API documentation, not a Windows desktop execution.

## Findings

### 1. Keyboard focus is restored from the last hide instead of the last activation

- **Severity:** Low
- **Location:** `src/platform/windows/main_window.cpp:548-550`; related state update at
  `src/platform/windows/main_window.cpp:355-362`.
- **Problem:** `WM_SETFOCUS` restores `last_focus_`, but the only assignment to that field occurs when explicitly hiding
  the window. Ordinary deactivation, including Alt-Tab, never records the currently focused child. On reactivation,
  default `WM_ACTIVATE` handling focuses the top-level window, which runs this handler. Consequently, selecting a
  duration or focusing the window list, switching to another application, and returning moves focus to the first
  control or to a stale control from the last hide. Microsoft's documented default activation behavior establishes
  the message path. [WM_ACTIVATE documentation](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-activate)
- **Impact:** Keyboard users lose their place whenever they switch applications. A subsequent Space or Enter can
  operate an unrelated button instead of continuing interaction with the duration selector or window list.
- **Recommendation:** Preserve the focused descendant on normal deactivation as well as explicit hide, and restore
  that enabled descendant on activation. Keep the existing fallback for a saved control that is no longer available.
- **Verification:** Searched every read/write of `last_focus_` and traced the unhandled `WM_ACTIVATE` through
  `DefWindowProcW` to `WM_SETFOCUS`. A Windows reproduction remains pending.

## Unresolved questions

None beyond the native validation limits below.

## Checks and observations

- Reviewed control construction, notification IDs, duration/list population, copyable result fields, and keyboard
  preprocessing. `IsDialogMessageW` is supported for ordinary windows containing controls, so using it with this
  custom window class is not itself a defect.
  [IsDialogMessageW documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-isdialogmessagew)
- Traced font/icon ownership across layout changes and window destruction, and tray removal versus object destruction
  during nested menu loops.
- Checked logical-unit layout separately from signed native target rectangles. The overlay passes target rectangles
  directly to `SetWindowPos` without applying layout scaling.
- The layered overlay also has `WS_EX_TRANSPARENT`; Microsoft documents that this combination passes mouse input to
  underlying windows. Its comment correctly distinguishes this behavior from `HTTRANSPARENT` alone.
  [Layered window behavior](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features#layered-windows)
- The existing MinGW Debug and Release build graphs reported no work to do. This establishes build-graph freshness,
  not interactive behavior.
- Checked resource references, manifest architecture/DPI settings, and the icon container. `assets/icon.ico` has a valid
  single-entry ICO directory with a 256-by-256 PNG payload; the README image is a separate PNG. Artwork aesthetics were
  not reviewed.

## Limits and pending checks

Windows 11 desktop validation was unavailable. Pending checks include tray mouse/keyboard activation and Explorer
restart, focus restoration, DPI transitions between monitors, overlay painting and click-through, screen-reader
control labeling, and shutdown while menus or error dialogs are open. No application binary was run on Windows.

The fixed window can exceed a small monitor's work area. `dpi.cpp:30` explicitly explains the choice to preserve
readable scale and keep the caption reachable, and the README states the fit requirement. This disclosed limitation
is not reported as a defect. The overlay intentionally captures a static rectangle and does not follow later window
movement; that documented behavior is also not a finding.
