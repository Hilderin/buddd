# Workflow Coordination: Inspector Indefinite Drag

## Orchestrator

**Feature**: `inspector-indefinite-drag`
**Status**: completed
**Current step**: done
**Loop history**: 1 — implementation-contract-critic rejected contract due to incorrect include paths in section 5a. Looping back to implementation-contract-author for fix.
**Naming correction**: Method renamed from `warp_mouse` to `set_mouse_position` per human feedback. Moved from `Window` to `InputSystem` for API symmetry with `InputSystem::mouse_position()`.
**Initial instructions**: In the editor inspector, when dragging floats or vec props, when the mouse goes outside the editor, the drag is stopped — we should be able to drag indefinitely (Unity/Unreal style).
**Notes**:

### Decision Log (Grill Me)

**Definition of Ready walkthrough (2026-06-14):**

**Clarity & Completeness:**
- [x] Scope clearly defined: Enable indefinite drag-to-scrub on float/vec property handles in the editor inspector using relative mouse mode (cursor hidden, raw mouse deltas). Int, bool, string, Color property types excluded.
- [x] Dependencies identified: `Window::set_mouse_capture()` (already exists, uses SDL_SetWindowRelativeMouseMode), `InputSystem::mouse_delta()` (already exists, accumulates raw xrel/yrel). New: `InputSystem::set_mouse_position()` to restore cursor position on drag end.
- [x] Edge cases described: Mouse release outside window (relative mode captures mouse, button up delivered), ESC during drag (not handled specially — drag naturally ends when mouse button released).
- [x] Expected behavior unambiguous: User clicks drag handle → cursor hidden, relative mode enabled, value changes based on raw mouse delta → user releases → cursor warps back to starting position, relative mode disabled.

**Verification:**
- [x] Verification method: Manual E2E testing (drag past window boundary, verify value continues changing, verify cursor returns to start on release).
- [x] Acceptance criteria specific and measurable: (1) Drag past window edge continues updating value, (2) Drag ends properly when button released outside window, (3) Cursor returns to starting position on release, (4) No interference with other editor functionality.
- [x] Success/failure states described.

**Documentation:**
- [x] Interface changes: `InputSystem` base class gets new `set_mouse_position(int x, int y)` pure virtual method.
- [x] Existing documentation to update: `docs/wiki/editor/editor-panels.md` (drag handle behavior description).

**Technical:**
- [x] Technical constraints: SDL3's `SDL_SetWindowRelativeMouseMode` used via existing `Window::set_mouse_capture()`. New `SDL_WarpMouseInWindow` via `InputSystem::set_mouse_position()`. No build changes needed.
- [x] Risks/unknowns: Relative mouse mode + ImGui interaction verified — ImGui correctly tracks active item state via mouse button events (unaffected by relative mode). `GetMouseDragDelta()` not used during relative mode drag; custom accumulator using `InputSystem::mouse_delta().x` instead.
- [x] Performance: Minimal — one extra `mouse_delta()` call per frame during drag.

**Key decisions:**
- Approach: Use `SDL_SetWindowRelativeMouseMode` (cursor lock) + `InputSystem::mouse_delta()` for raw deltas (Option B — infinite drag, Unity/Unreal style).
- Cursor behavior: Hidden during drag, restored + warped to starting position on release.
- Scope: Float, Vec2, Vec3, Vec4, Quat drag handles only (not int).
- Testing: Skip automated tests for now (manual E2E only).

## spec-author

**Status**: completed
**Summary**:
Spec for indefinite drag-to-scrub on float/vec property handles in the editor inspector. Uses relative mouse mode (SDL_SetWindowRelativeMouseMode) + InputSystem::mouse_delta() to enable infinite drag past window boundaries. Adds InputSystem::set_mouse_position() to restore cursor position on release. Covers draw_axis_widget() and the float editor lambda in inspector_editors.cpp. Excludes int/bool/string/Color editors. 10 acceptance criteria covering behavior, interface changes, and non-regression.
**Artifacts**:
- `.specs/sprint-2026-06/inspector-indefinite-drag/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Spec Validation

**Status**: approved
**Summary**:
Spec approved by human. Key changes: `InputSystem::set_mouse_position()` added to InputSystem interface (not Window), relative mouse mode + `InputSystem::mouse_delta()` for indefinite drag.
**Date**: 2026-06-14 18:57
**Feedback**: Approved after naming correction (warp_mouse → set_mouse_position, moved from Window to InputSystem).

## implementation-contract-author

**Status**: completed
**Summary**:
Wrote implementation contract at `.specs/sprint-2026-06/inspector-indefinite-drag/implementation-contract.md`. Contract covers: (1) adding `InputSystem::set_mouse_position()` pure virtual to input_system.h; (2) extending InputSystemSDL3 with `sdl_window_` member, `set_sdl_window()`, and `set_mouse_position()` via SDL_WarpMouseInWindow; (3) no-op override in InputSystemHeadless; (4) wiring `set_sdl_window()` in PlatformSDL3::create_window(); (5) replacing drag logic in `draw_axis_widget()` and the float editor lambda to use relative mouse mode + `mouse_delta().x` instead of `ImGui::GetMouseDragDelta()`, with a `DragState` struct; (6) logging on drag start/end/warp.

15 verifiable Done Criteria (DC-01 through DC-15). Edge cases from spec spec carried forward. Notes one discrepancy: the spec's "set_mouse_capture(true) failed" warning log cannot be implemented because `Window::set_mouse_capture` currently returns `void` — documented in contract for implementer to handle.

**Loopback fix (iteration 2)**: Fixed include paths in section 5a — removed erroneous `engine/` prefix from `#include "engine_service.h"`, `#include "input/input_system.h"`, `#include "platform/platform.h"` to match the engine's public include directory layout (`src/engine/`).
**Artifacts**:
- `.specs/sprint-2026-06/inspector-indefinite-drag/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- The spec's failure log `BUDDD_LOG_TAGGED_WARN("Editor:Inspector", "set_mouse_capture(true) failed")` (spec line 311) cannot be implemented because `Window::set_mouse_capture` currently returns `void`. Documented in the contract's Required implementation behavior section 8 for the implementer to handle (either leave the log out or extend the Window interface — the latter would expand scope).
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review after loopback fix: Include paths in section 5a corrected — `"engine_service.h"`, `"input/input_system.h"`, `"platform/platform.h"` no longer have the erroneous `engine/` prefix. All 10 ACs still covered by 15 DCs. No new issues introduced. Contract is sound and ready for implementation. Verdict: accepted.
**Artifacts**:
- `.specs/sprint-2026-06/inspector-indefinite-drag/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- Fragile line number references throughout the contract — absolute line numbers may not match actual files if edited since contract was written. Descriptive markers would be more robust.
- `<unordered_map>` not explicitly included in `inspector_editors.cpp` (pre-existing — works via transitive include).
- `SDL_WarpMouseInWindow` return value (bool) ignored; `set_mouse_position` is `-> void` per spec — no way to detect warp failures.
- `platform_sdl3.cpp` insertion point line number reference may be off.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-14 18:57
**Warnings**:
- Spec's failure log for `set_mouse_capture(true)` cannot be implemented due to `void` return type (discrepancy noted, non-blocking).
**Notes**: Approved to proceed with implementation.

## code-implementer

**Status**: completed
**Summary**:
Implemented indefinite drag-to-scrub for float/vec property handles in the editor inspector. Added `InputSystem::set_mouse_position()` pure virtual to the input system interface, with SDL3 implementation via `SDL_WarpMouseInWindow` (with null check) and headless no-op. Extended `InputSystemSDL3` with `sdl_window_` member and `set_sdl_window()` setter. Wired `set_sdl_window()` in `PlatformSDL3::create_window()` after `SDL_CreateWindow`. Replaced `draw_axis_widget()` and float editor lambda drag handling to use relative mouse mode (`Window::set_mouse_capture()`) + `InputSystem::mouse_delta().first` accumulation instead of `ImGui::GetMouseDragDelta()`, using a new `DragState` struct. Added logging on drag start/end/warp. Added necessary includes (`engine_service.h`, `input/input_system.h`, `platform/platform.h`, `window/window.h`).
**Artifacts**:
- `src/engine/input/input_system.h` — added `set_mouse_position` pure virtual
- `src/engine/input/input_system_sdl3.h` — added `set_sdl_window`, `set_mouse_position` override, `sdl_window_` member
- `src/engine/input/input_system_sdl3.cpp` — implemented `set_mouse_position` and `set_sdl_window`
- `src/engine/input/input_system_headless.h` — added `set_mouse_position` override
- `src/engine/input/input_system_headless.cpp` — implemented no-op `set_mouse_position`
- `src/engine/platform/platform_sdl3.cpp` — wired `input_system_.set_sdl_window(sdl_window)`
- `src/editor/inspector_editors.cpp` — replaced drag logic with relative mouse mode, added includes, added `DragState` struct
**Questions for human**:
none
**Warnings**:
- `Window::set_mouse_capture()` returns `void`, so the spec's aspirational failure log (`"set_mouse_capture(true) failed"`) could not be implemented — this was documented in the contract and is a non-issue.
- Added `#include "window/window.h"` beyond what the contract listed, because `Window` is only forward-declared in `engine_context.h` and the code needs a complete type to call `set_mouse_capture()`.
**Blocking issues**:
none

## tester

**Status**: completed
**Summary**:
Completed build verification and full test suite run. All 833 tests pass with zero failures and zero new warnings. Verified all 15 Done Criteria against the implementation. Confirmed that all forbidden files remain unchanged. The feature cannot be automated (requires OS-level relative mouse mode). A detailed manual test procedure is provided in the test report.
**Artifacts**:
- `.specs/sprint-2026-06/inspector-indefinite-drag/test-report.md`
**Questions for human**:
none
**Warnings**:
- The spec's "set_mouse_capture(true) failed" warning log cannot be implemented because `Window::set_mouse_capture()` returns `void`. This pre-existing limitation is documented and non-blocking.
**Blocking issues**:
none
**Manual tests required**:
See `.specs/sprint-2026-06/inspector-indefinite-drag/test-report.md` — 7-step manual test procedure covering: drag past window boundary, release outside window, rotation handles, click-to-type, FreeCameraMovement, int editor, and bool/string/Color editors.

## Manual Test Validation

**Status**: passed
**Instructions**:
7 manual tests covering: drag past window boundary, release outside window, rotation handles, click-to-type, FreeCameraMovement, int editor, bool/string/Color editors.
**Human feedback**:
All 7 tests passed successfully.
**Date**: 2026-06-14 18:57
**Notes**: Feature behaves as expected — cursor hidden during drag, value changes outside window, cursor warps back on release. No interference with other editors or FreeCameraMovement.

## wiki-agent

**Status**: completed
**Summary**:
Updated the operational wiki to document the new indefinite-drag behavior for float/vec inspector property handles. Added the new `InputSystem::set_mouse_position()` method to the Input submodule module-map. Updated the Inspector Property Editors section to describe the relative-mouse-mode drag pattern (cursor hidden, raw deltas from `InputSystem::mouse_delta()`, warp on release).
**Artifacts**:
- `docs/wiki/editor/editor-panels.md`
- `docs/wiki/architecture/module-map.md`
**Changes made**:
- `docs/wiki/editor/editor-panels.md`: Added SPEC-NNNN subsection to the Inspector Property Editors blockquote describing the indefinite-drag behavior (relative mouse mode, cursor hiding, InputSystem::mouse_delta accumulation, warp on release via InputSystem::set_mouse_position).
- `docs/wiki/architecture/module-map.md`: Added `set_mouse_position(int x, int y)` to the `input_system.h` row in the Input submodule table.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above (spec-author → Human Spec Validation → implementation-contract-author → implementation-contract-critic → Human Validation → code-implementer → tester → Manual Test Validation → wiki-agent).
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
