# Windows UI review

## Scope and basis

Reviewed `main_window.*`, `main_window_controls.*`, `list_scrolling.*`, `dpi.*`, `overlay_window.*`, and `tray_icon.*`
under `src/platform/windows/`, plus the details/clipboard presentation in `windows_platform_binding.cpp` and related
usage documentation, at commit `5f80b0ca3120d25f5304f9a2dc110cbf71806617`. The worktree was clean before report
creation.

## Findings

### Long failure statuses hide the diagnostic with no way to expand it

- **Resolution:** All three status content areas now open their complete text on a left-click anywhere in the control,
  even for short messages. The dialog shares the **Copy** and **OK** implementation with **Show Details**. Overflowing
  status text clips at the right edge without an ellipsis. See [status usage](docs/usage.md#status-and-timing). Native
  click, clipping, clipboard, DPI, and modal shutdown checks remain pending on Windows.
- **Severity:** Low
- **Location:** `src/platform/windows/main_window_controls.cpp:59`, `:70`, and `:86`; status construction at
  `src/application/awake_controller.cpp:63-67` and `src/application/close_controller.cpp:49-53`.
- **Problem:** Both feature statuses use a fixed 682-by-24 logical-unit static control with `SS_ENDELLIPSIS`. The close
  result places its error after the handle, regional long-date timestamp, and process name. Power-release errors
  likewise place the diagnostic and retry instruction after a long prefix and timestamp. Long results are truncated, and
  the UI provides no status tooltip, expansion, or copy action. Microsoft documents that this style forces a single line
  and truncates overflowing text.
  [Static control styles](https://learn.microsoft.com/en-us/windows/win32/controls/static-control-styles)
- **Impact:** A scheduled close failure or automatic power-release failure can display its failure prefix while hiding
  the reason needed to understand or resolve it. These expiry paths do not open an error dialog. **Show Details**
  displays window metadata, not the operation result, and cannot recover the truncated diagnostic.
- **Recommendation:** Provide access to the complete status through an expandable/selectable result, a details action,
  or a tooltip with an accessible equivalent. Keep the compact countdown if desired.
- **Verification:** Traced status construction through `MainWindowControls::Present` and its static controls, and
  checked the available dialog/copy paths. Exact truncation positions depend on the Windows font, locale, and process
  name; those positions were not measured on a Windows desktop.

## Review evidence

- Traced tray activation, tray restoration and failure fallback, hiding, system-menu exit, and focus restoration.
- Checked DPI-scaled control layout, icon ownership, work-area positioning, and preservation of signed native screen
  coordinates for overlays.
- Inspected subclass lifetime, redraw restoration, deferred selection delivery, and exception containment.
- Checked details timestamp formatting, clipboard allocation ownership, and modal exit handling.

## Unresolved questions

None requiring a finding beyond the diagnostic presentation issue. Native rendering and input behavior remain subject to
the manual checks below.

## Checks and limitations

Static review and the fresh Windows Release cross-build completed successfully. PE inspection confirmed embedded
icon/group-icon, version, and manifest resources. A Windows desktop is unavailable. Manual checks remain pending for
tray mouse/keyboard activation and Explorer restart, focus and tab navigation, list/combo scrolling and repaint,
mixed-monitor DPI changes, overlay click-through, clipboard behavior, high-contrast/accessibility behavior, and modal
shutdown. The fixed logical window size on undersized displays is an explicit documented limitation and is not reported
as a defect. Formatting, individual test cases, and coverage adequacy are excluded.
