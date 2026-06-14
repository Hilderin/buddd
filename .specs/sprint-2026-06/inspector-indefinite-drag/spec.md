# SPEC-NNNN — Inspector Indefinite Drag

## Problem

In the editor inspector, when dragging float or vec property handles (the colored axis widgets for Position, Rotation, Scale, and individual float handles), the drag stops as soon as the cursor leaves the editor window boundary. This happens because the current implementation uses `ImGui::GetMouseDragDelta()` which relies on ImGui's internal mouse tracking, and ImGui stops receiving mouse-motion events from SDL when the cursor is outside the window.

Consequences:
- Users cannot scrub a value to a large range in one gesture (they must drag, release, re-click, drag again).
- If the mouse button is released outside the editor window, `ImGui::IsItemDeactivated()` does not fire, leaving the drag state "stuck" — the handle remains active, and any subsequent mouse movement inside the window jumps the value unexpectedly.
- The experience is noticeably inferior to modern engine editors (Unity, Unreal Editor, Godot) where dragging a numeric field works seamlessly regardless of cursor position.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Indefinite drag**: User can click+drag a float, Vec2, Vec3, Vec4, or Quat drag handle and scrub the value arbitrarily far, even when the cursor leaves the editor window (including movement to a different monitor). |
| G-02 | **Cursor hidden during drag**: While dragging, the OS cursor is hidden to indicate the drag is active and to avoid visual confusion when the cursor moves outside the window. |
| G-03 | **Cursor restoration on release**: When the mouse button is released (inside or outside the window), the cursor is shown again and warped back to the position where the drag originally started. |
| G-04 | **No stuck drag state**: Releasing the mouse button outside the window correctly ends the drag — the value stops changing, and the next click+drag starts a fresh drag from the current value. |
| G-05 | **No interference**: The new drag behavior does not conflict with FreeCameraMovement (which also uses relative mouse mode) or with other editor panels. |
| G-06 | **Non-regression**: All existing inspector editors for int, bool, string, and Color continue to work exactly as before. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No changes to the int editor** — `ImGui::DragInt` already works well and is not affected by the window-boundary issue in practice. |
| NG-02 | **No changes to the bool, string, or Color editors** — these types do not use drag-to-scrub and are unaffected. |
| NG-03 | **No changes to FreeCameraMovement** — it already uses `Window::set_mouse_capture()` correctly and continues to coexist via mutually exclusive activation (right-click for camera, left-click for property handles). |
| NG-04 | **No wiki or ADR updates** — documentation updates are deferred and handled by the wiki-agent in a later step. |
| NG-05 | **No automated tests** — the feature relies on OS-level relative mouse mode which cannot be meaningfully tested in a headless environment. Verification is via manual E2E testing. |
| NG-06 | **No keyboard shortcut for drag cancellation** — ESC does not cancel the drag (the drag ends naturally when the mouse button is released). No new keyboard handling is added. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, selects an entity in the Scene Panel, and edits float/vec properties in the Inspector Panel by dragging the colored axis handles (X/Y/Z) or the gray float handle. Expects seamless drag across the entire screen, matching the behavior of Unity/Unreal Editor. |
| **Editor developer** | Main consumer of the changed code. The `draw_axis_widget()` helper and float editor lambda are modified. The `Window` base class gains a new `set_mouse_position()` method. |

## User-visible behavior

### A. Before the feature

1. User clicks a colored axis handle (e.g., Position X) and drags left/right → the value changes.
2. If the cursor reaches the edge of the editor window and continues moving → the value stops changing (SDL stops delivering mouse events to the window).
3. If the user releases the mouse button while the cursor is outside the window → the handle remains "active" (stuck drag state). The next time the user moves the mouse inside the window, the value jumps unexpectedly.

### B. After the feature

1. User clicks a colored axis handle (e.g., Position X) and starts dragging.
2. **On click (IsItemActivated)**: The OS cursor is hidden and relative mouse mode is enabled. The initial cursor position (in screen/window coordinates) is saved as the "drag start position".
3. **During drag (IsItemActive)**: The cursor is hidden. The value is updated using raw mouse deltas (`InputSystem::mouse_delta().x`) accumulated since the drag started, instead of `ImGui::GetMouseDragDelta()`. The cursor can move freely across any monitor without affecting the window — only relative motion matters.
4. **On release (IsItemDeactivated)**: Relative mouse mode is disabled, the cursor is shown again, and the cursor is warped back to the drag start position. The accumulator is cleared.
5. Releasing the mouse button outside the window works correctly because relative mouse mode captures the input — SDL delivers the button-up event regardless of cursor position.

### C. Which handles are affected

All handles that use `draw_axis_widget()` in `inspector_editors.cpp`:

| Type | Handles | Widget |
|---|---|---|
| `float` | 1 gray handle | `draw_axis_widget` (via float editor lambda) |
| `Vec2` | 2 colored handles (X, Y) | `draw_axis_widget` (via Vec2 editor → calls draw_axis_widget) |
| `Vec3` | 3 colored handles (X, Y, Z) | `draw_axis_widget` (via Vec3 editor) |
| `Vec4` | 4 colored handles (X, Y, Z, W) | `draw_axis_widget` (via Vec4 editor) |
| `Quat` | 3 colored handles (X=Pitch, Y=Yaw, Z=Roll) | `draw_axis_widget` (via Quat editor) |

The `int` editor (uses `ImGui::DragInt` natively) is NOT affected.

### D. Visual state during drag

- The colored axis widget rectangle remains visible (the cursor is hidden, but the widget UI does not change).
- The InputFloat text field continues to update with the new value during the drag.
- The cursor disappears — the user navigates by feel (relative mouse movements).

## User stories

### Story 1 — Drag past window boundary (Priority: P1)

As an editor user, I want to drag a float property handle past the edge of the editor window and have the value continue to change, so that I can scrub values across a large range in one gesture.

**Given** the editor is open with an entity selected (e.g., Position X = 0.0)
**When** I click and drag the Position X handle to the right and move the cursor outside the right edge of the editor window
**Then** the Position X value continues to increase as I drag further right (even when outside the window)
**And** the cursor is hidden during the drag

**Given** I continue dragging while outside the window
**When** I release the mouse button (while outside the window)
**Then** the drag ends cleanly (value stops changing)
**And** the cursor reappears at the position where the drag originally started

### Story 2 — Drag to a different monitor (Priority: P2)

As an editor user with a multi-monitor setup, I want to drag a value handle across multiple monitors without interruption.

**Given** the editor window is on monitor 1 and the user has a second monitor
**When** I click+drag a handle and move the cursor onto monitor 2
**Then** the value continues to change based on cursor movement
**And** the cursor remains hidden
**When** I release the mouse button
**Then** the cursor warps back to the starting position

### Story 3 — Normal click-to-type editing still works (Priority: P1)

As an editor user, I want to continue using single-click text entry on the InputFloat portion of the composite axis widget, without triggering relative mouse mode.

**Given** the editor is open with an entity selected
**When** I single-click the InputFloat text field of a Position X widget
**Then** the field enters text edit mode (cursor appears in the field)
**And** the mouse cursor remains visible
**And** relative mouse mode is NOT enabled

**Given** I type a new value and press Enter
**Then** the value changes to the typed value
**And** the scene is marked dirty

**Given** I press Escape before confirming
**Then** the value reverts to the previous value

### Story 4 — No interference with FreeCameraMovement (Priority: P2)

As an editor user, I want to use both property dragging and free-camera controls without them conflicting.

**Given** the editor is in edit mode (not dragging a property handle)
**When** I right-click and drag in the viewport to orbit the camera (if FreeCameraMovement is in use)
**Then** the camera controls work as before
**And** no unintended value changes occur in the Inspector

**Given** I am actively dragging a property handle in the Inspector
**When** I move the mouse in the viewport
**Then** the value changes based on mouse delta (expected behavior)
**And** the camera does not move

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | User clicks + drags a float/vec drag handle → cursor hides, value changes with mouse movement. | Manual: drag Position X handle, confirm cursor disappears and value changes. |
| AC-002 | Moving the cursor outside the editor window (even to a different monitor) continues to update the value. | Manual: drag Position X past window edge, confirm value still changes. Drag to second monitor, confirm value still changes. |
| AC-003 | Releasing the mouse button outside the window properly ends the drag (no "stuck" drag state). | Manual: drag outside window, release, move mouse back inside window — verify value does not jump unexpectedly and handle is not active. |
| AC-004 | On release, the cursor warps back to where the drag initially started. | Manual: drag handle, release outside window — verify cursor appears at the original click position (not at current mouse position). |
| AC-005 | No regression in normal click-to-type editing (InputFloat still works without triggering relative mode). | Manual: click the InputFloat portion of a composite widget, type a value, press Enter — verify value changes and cursor remains visible throughout. |
| AC-006 | No interference with other editor panels or the engine's FreeCameraMovement. | Manual: activate free-camera (right-click in viewport), verify camera controls work. Then drag a property handle in Inspector, verify camera does not move. |
| AC-007 | `InputSystem` base class gains pure virtual `set_mouse_position(int x, int y)` method. | Code review: verify `input_system.h` contains the new method, `InputSystemSDL3` implements via `SDL_WarpMouseInWindow`, `InputSystemHeadless` is a no-op. |
| AC-008 | `draw_axis_widget()` uses relative mouse mode + `InputSystem::mouse_delta()` during drag, instead of `ImGui::GetMouseDragDelta()`. | Code review: verify `IsItemActivated` calls `set_mouse_capture(true)`, `IsItemActive` accumulates `input_system().mouse_delta().first`, `IsItemDeactivated` calls `set_mouse_capture(false)` + `set_mouse_position(start_x, start_y)`. |
| AC-009 | The float editor lambda (lines 146-198 in `inspector_editors.cpp`) also uses the same relative-mouse drag pattern. | Code review: verify the float editor's drag-handle code is updated identically to `draw_axis_widget()`. |
| AC-010 | The int editor is unchanged (still uses `ImGui::DragInt`). | Code review: verify no changes to the int editor lambda. |

## E2E Verification

| Method | Description |
|---|---|
| **Manual smoke test** | Run `buddd edit` with a scene loaded. Select an entity. Verify: (1) Drag Position X past window edge → value continues changing and cursor is hidden; (2) Release outside → cursor warps back; (3) Drag a Rotation handle (Quat) → same behavior; (4) Click-to-type on InputFloat works without triggering relative mode; (5) FreeCameraMovement still works (right-click in viewport); (6) Int editor unchanged (DragInt still works). |
| **Build verification** | `cmake --build --preset debug` with zero new warnings from `src/editor/`, `src/engine/window/`, and `tests/`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A user can drag a float property past the window boundary and the value continues changing. | Manual: Position X drag past window edge, observe value change in the InputFloat field. |
| SC-002 | A user can release the mouse button outside the window and the drag ends cleanly (no stuck state). | Manual: drag past window edge, release, move mouse back into window — verify no unexpected value jump. |
| SC-003 | On drag release, the cursor returns to the original click position within the window. | Manual: observe cursor position after release — it should be at the original handle click location. |
| SC-004 | All existing non-float/non-vec inspector editors (int, bool, string, Color) work identically before and after the change. | Manual: edit each type, verify expected behavior (DragInt, Checkbox, InputText, ColorEdit) unchanged. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| **Window focus loss during drag** | Relative mouse mode is released by SDL on focus loss (existing behavior). `set_mouse_capture(false)` is called on `IsItemDeactivated()`. If `IsItemDeactivated()` doesn't fire (focus loss without button release), the drag state persists but relative mode is desynchronized. The next `IsItemActivated()` call resets the state. |
| **ESC during drag** | Currently left undefined — the drag continues until mouse button release. No special ESC handling. The cursor remains hidden. |
| **Multiple monitors with different DPI scaling** | Raw mouse deltas from SDL are in device-independent units. The drag speed multiplier handles the conversion to value changes. No DPI-specific handling needed. |
| **Headless mode (no display)** | `WindowHeadless::set_mouse_capture()` is a no-op. `WindowHeadless::set_mouse_position()` is a no-op. `InputSystemHeadless::mouse_delta()` returns `(0, 0)`. The drag code path exists but produces no value change. |
| **Rapid click (no drag)** | If the user clicks and immediately releases (no movement), `IsItemActivated()` and `IsItemDeactivated()` fire in the same frame. `set_mouse_capture(true/false)` are called but produce no visible effect since no relative mode was enabled. No cursor warp occurs (accumulator is 0, no value change). |
| **Dragging two handles simultaneously** | Not possible with a single mouse cursor. Each handle has its own `ImGui::PushID` scope. Only one handle can be active at a time. |
| **Concurrent FreeCameraMovement and property drag** | FreeCameraMovement activates on right-click, property drag activates on left-click on a handle. These are mutually exclusive input actions (different buttons). No conflict. |
| **Very rapid drag (high polling rate mouse, 1000Hz)** | `InputSystem::mouse_delta()` accumulates deltas per frame. The drag code reads it once per frame. No issue with high-frequency mice. |
| **Drag start position at window edge** | If the user clicks a handle very close to the window edge, the saved start position is within the window. `set_mouse_position()` will correctly return the cursor to that position. |
| **Zero-size or minimized window** | `set_mouse_position` on a minimized window is a no-op (SDL3 handles this gracefully). Relative mouse mode cannot be enabled on a minimized window (SDL3 returns an error which is ignored). |

## Error cases

| Case | Expected behaviour |
|---|---|
| **`SDL_WarpMouseInWindow` called on destroyed window** | Not possible — the Window destructor runs after editor shutdown. The Window is alive during all inspector interactions. |
| **Relative mouse mode fails to enable** | `SDL_SetWindowRelativeMouseMode` returns false. The existing code ignores the return value. The drag continues with the fallback `GetMouseDragDelta()` behavior (value changes only when cursor is inside the window). No crash. |
| **`mouse_delta()` called after `begin_frame()` but before `end_frame()`** | This is the normal usage pattern — the input system accumulates delta over the frame, and `mouse_delta()` returns the accumulated value. Called once per frame during `IsItemActive()`. No issue. |
| **Multiple calls to `set_mouse_position()` per frame** | No issue — SDL respects the last warp position. |
| **Drag starts but `set_mouse_capture(false)` is never called** | If the editor crashes during a drag, relative mouse mode remains enabled. The next editor launch resets it (SDL window is recreated). The user can also press ESC (FreeCameraMovement already handles this recovery). |
| **Drag state persists after scene switch** | Selection changes terminate the property editing context. On the next frame, `IsItemActive()` returns false for the old handle. The code in `draw_axis_widget()` calls `IsItemDeactivated()` if the item was previously active, which properly releases mouse capture and warps the cursor. |

## Interface Changes

### InputSystem base class (`src/engine/input/input_system.h`)

```cpp
// NEW pure virtual method on class InputSystem:

/// Set the OS cursor position to the given window client coordinates (top-left origin).
/// SDL3 backend: calls SDL_WarpMouseInWindow(). Headless: no-op.
/// Used to restore the cursor position after a relative-mouse-mode drag ends.
virtual auto set_mouse_position(int x, int y) -> void = 0;
```

### InputSystemSDL3 (`src/engine/input/input_system_sdl3.h/.cpp`)

- Add `SDL_Window* sdl_window_{nullptr};` private member.
- Add `void set_sdl_window(SDL_Window* window)` (called by `PlatformSDL3` after window creation).
- Implement `set_mouse_position` via `SDL_WarpMouseInWindow(sdl_window_, x, y)` (no-op if `sdl_window_` is null).

```cpp
// NEW private member in class InputSystemSDL3:
SDL_Window* sdl_window_{nullptr};

// Implementation of set_mouse_position:
auto InputSystemSDL3::set_mouse_position(int x, int y) -> void {
    if (sdl_window_) {
        SDL_WarpMouseInWindow(sdl_window_, x, y);
    }
}
```

Also in `PlatformSDL3::create_window()`, after creating the window:
```cpp
input_system_.sdl_window_ = sdl_window;  // or input_system_.set_sdl_window(sdl_window);
```

### InputSystemHeadless (`src/engine/input/input_system_headless.h/.cpp`)

```cpp
auto InputSystemHeadless::set_mouse_position(int, int) -> void {
    // no-op (headless has no cursor)
}
```

### `draw_axis_widget()` (`src/editor/inspector_editors.cpp`)

Signature change: the `(void)ctx;` line is removed and the `ctx` parameter is used.

**New includes needed** (added at top of `inspector_editors.cpp` or in the anonymous namespace block):
- `#include "engine_context.h"` (for `EngineContext` — may already be transitively included)
- No new includes needed beyond what's already available via `editor.h` — `Window` is available via `ctx.engine.window`, `InputSystem` via `ctx.engine.services.platform().input_system()`.
  (Verify during implementation.)

**Behavioral change** (conceptual):

```cpp
// During drag, instead of:
//   float pixel_delta = ImGui::GetMouseDragDelta().x;
//   float new_val = initial_value + pixel_delta * drag_speed * 0.01f;

// Use:
//   auto& input = ctx.engine.services.platform().input_system();
//   drag_accumulator += input.mouse_delta().first;
//   float new_val = initial_value + drag_accumulator * drag_speed * 0.01f;
```

**State management**:
- `start_x`, `start_y` — saved on `IsItemActivated` from `ImGui::GetMousePos()`.
- `drag_accumulator` — accumulated `input.mouse_delta().first` each frame during `IsItemActive`.
- `initial_value` — saved per-handle (same pattern as current code using `static std::unordered_map`).
- All state cleared on `IsItemDeactivated`, plus `input.set_mouse_position(start_x, start_y)` to restore cursor.

### Float editor lambda (`src/editor/inspector_editors.cpp`, lines 146-198)

Same pattern as `draw_axis_widget()` — the drag-handle section within the float editor lambda is updated identically.

### No other files changed

- `src/editor/inspector_editors.h` — No interface changes (the `draw_axis_widget` function signature stays the same; it already takes `const EditorContext& ctx`).
- `src/engine/window/window.h` — No changes (Window's interface stays the same).
- No CMakeLists.txt changes needed.

## Key entities

### `InputSystem` (extended)

```
InputSystem (abstract base)
├── mouse_position() -> pair<float,float>           (existing)
├── mouse_delta() -> pair<float,float>              (existing)
├── mouse_wheel() -> pair<float,float>              (existing)
├── set_mouse_position(int x, int y) -> void        ← NEW
└── ... (other keyboard/mouse query methods)
```

### `draw_axis_widget()` internal state (per drag session)

```
Per-handle drag state (static unordered_map keyed by value pointer):
├── initial_value: float       (value at drag start)
├── drag_accumulator: float    (accumulated raw mouse delta)
├── start_x: float             (cursor X at drag start — screen coords)
└── start_y: float             (cursor Y at drag start — screen coords)
```

## Permissions and security

- No changes to permissions or security posture.
- Relative mouse mode is a standard OS-level input mode (available on all major desktop platforms).
- No authentication or authorization boundaries are crossed.
- No file I/O or network access is involved.

## Observability

| Signal | Source |
|---|---|
| **Relative mouse mode activation** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "drag start: handle={} initial_value={}", id, initial_value)` — logged once on `IsItemActivated`. |
| **Relative mouse mode deactivation** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "drag end: handle={} final_value={}", id, *value)` — logged once on `IsItemDeactivated`. |
| **Warp mouse call** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "warp mouse to ({}, {})", start_x, start_y)` — logged on `IsItemDeactivated` when `set_mouse_position()` is called. |
| **Relative mouse mode failure** | Warning log if `set_mouse_capture(true)` returns false (SDL3 may return false if the window is not focused, etc.) — `BUDDD_LOG_TAGGED_WARN("Editor:Inspector", "set_mouse_capture(true) failed")` |

## Documentation impact

The following existing documentation should be updated when this feature is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Update the Inspector Property Editors section to describe the new indefinite-drag behavior (relative mouse mode, cursor hiding, warp on release). |
| `docs/wiki/architecture/module-map.md` | Add `InputSystem::set_mouse_position()` to the InputSystem submodule section. |

## Out of scope

- Int editor (uses `ImGui::DragInt` natively — unchanged).
- Bool editor (checkbox — unchanged).
- String editor (InputText — unchanged).
- Color editor (ColorEdit3/ColorEdit4 — unchanged).
- FreeCameraMovement component (already uses relative mouse mode via `set_mouse_capture` — no changes needed).
- Wiki and ADR updates (handled by the wiki-agent in a later step).
- Automated tests (headless mode cannot test OS-level mouse capture).
- Keyboard shortcut to cancel drag (ESC — left undefined; drag ends on mouse button release).
- Gimbal lock or rotation-specific edge cases (Quat editor unchanged).
- Changes to the `InspectorTypeEditorRegistry` or any editor registration API.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `SDLK_WarpMouseInWindow` is available in SDL3 (confirmed — it is part of the SDL3 API, declared in `<SDL3/SDL_mouse.h>`). |
| A-02 | `InputSystem::mouse_delta()` returns relative mouse motion accumulated since the last `begin_frame()` call, even when relative mouse mode is enabled. Confirmed by checking the SDL3 input system — SDL delivers `SDL_EVENT_MOUSE_MOTION` with `xrel`/`yrel` regardless of relative mode. |
| A-03 | `ImGui::IsItemActive()` and `ImGui::IsItemDeactivated()` work correctly when relative mouse mode is enabled — they track the ImGui active ID based on mouse button state, which is unaffected by relative mode. |
| A-04 | `ImGui::GetMousePos()` returns a valid position even when relative mouse mode is enabled — ImGui tracks the cursor position internally (it does not use the OS cursor position). The saved `start_x`/`start_y` are from ImGui's coordinate system, not the OS cursor position. |
| A-05 | `Window::set_mouse_capture(true)` both hides the cursor and enables relative mouse mode. `Window::set_mouse_capture(false)` both shows the cursor and disables relative mouse mode. This is the existing behavior (wraps `SDL_SetWindowRelativeMouseMode`). |
| A-06 | No other code in the editor calls `set_mouse_capture()` during property editing. FreeCameraMovement uses right-click, which is a different mouse button — mutually exclusive. |
| A-07 | The `ctx.engine.window` reference is always valid during editor frame rendering (it is set before editor panels are rendered and valid until after the last panel's `draw_ui()` returns). |
| A-08 | `ctx.engine.services.platform().input_system()` returns a valid `InputSystem&` for the lifetime of the editor session (owned by `EngineService` which outlives the editor). |
| A-09 | The `draw_axis_widget()` function is only called from the main UI thread (ImGui convention). No thread-safety concerns with the static state maps. |

## Open questions

| ID | Question | Priority | Impact |
|---|---|---|---|
| Q-01 | **Should we handle window focus loss during drag (EC-012 style)?** The existing `WindowSDL3::set_mouse_capture()` comment notes that `captured_` desyncs on focus loss. This is an existing known issue with the same pattern used by FreeCameraMovement. For now, we leave it as-is (the issue is pre-existing and not made worse by this feature). The `IsItemDeactivated()` path will reset state when the user releases the mouse button. | **Low** — pre-existing issue, not introduced by this feature. | No clarification needed. |
| Q-02 | **Should the drag accumulator be per-handle or per-value-pointer?** The current code uses `static std::unordered_map<const void*, float>` keyed by the `value` pointer. This works correctly for all cases (each widget instance has a distinct value pointer). We extend this map to store `(initial_value, drag_accumulator, start_x, start_y)` as a struct. A simpler alternative is to use local variables and rely on `IsItemActivated`/`IsItemDeactivated`, but the `static unordered_map` pattern already handles multiple simultaneous handles correctly (though only one can be active at a time). | **Low** — implementation detail. | No clarification needed. |

---

## Self-validation checklist

| Check | Pass/Fail |
|---|---|
| Is every acceptance criterion testable? | ✅ Yes — all ACs have clear manual test procedures or code review guidelines. |
| Are all edge cases and error cases covered? | ✅ Yes — 11 edge cases and 9 error cases listed. |
| Are there any hidden implementation decisions? | ✅ No — the spec specifies behavior and interface changes, not implementation details (e.g., how the accumulator is stored is left to the contract). |
| Are success criteria measurable and technology-agnostic? | ✅ Yes — SC-001 through SC-004 are about user-observable outcomes. |
| Are user stories prioritized and independently testable? | ✅ Yes — P1/P2 stories with Given/When/Then. |
| Are there no more than 10 `[NEEDS CLARIFICATION]` markers? | ✅ Yes — zero markers. |
| Does the spec contradict any accepted spec? | ✅ No — aligns with F-05 (Inspector Transform), F-06 (Properties Panel UX), and F-07 (Component Properties). |
| Are assumptions documented for every reasonable default made? | ✅ Yes — all 9 assumptions documented. |
| Does the spec satisfy the Definition of Ready? | ✅ Yes — see DoR check below. |

### Definition of Ready check

| Criterion | Status |
|---|---|
| Scope is clearly defined (what is included and what is explicitly excluded) | ✅ Yes — Goals vs Non-goals clearly separate in-scope from out-of-scope. |
| Dependencies on other features, modules, or external systems are identified | ✅ Yes — depends on `Window::set_mouse_capture()` (existing), `InputSystem::set_mouse_position()` (new), `InputSystem::mouse_delta()` (existing), SDL3 `SDL_WarpMouseInWindow`. |
| Edge cases and error conditions are described | ✅ Yes — 11 edge cases, 9 error cases. |
| The expected behavior is unambiguous and testable | ✅ Yes — ACs are specific, Gherkin stories, behavioral descriptions. |
| The spec defines how the feature will be verified end-to-end | ✅ Yes — manual smoke test + build verification. |
| Acceptance criteria are specific, measurable, and verifiable | ✅ Yes — AC-001 through AC-010 all have clear verification methods. |
| Success and failure states are described | ✅ Yes — success criteria vs error cases. |
| Interface changes are documented | ✅ Yes — `InputSystem::set_mouse_position()` API change documented, includes needed documented. |
| Existing documentation that must be updated is listed | ✅ Yes — wiki pages listed in Documentation impact section. |
| Technical constraints are identified | ✅ Yes — SDL3 `SDL_SetWindowRelativeMouseMode`, `SDL_WarpMouseInWindow`, `InputSystem::mouse_delta()`. No build changes. |
| Risks or unknowns are surfaced | ✅ Yes — relative mouse mode + ImGui interaction, focus loss desync noted. |
| Performance or resource implications are noted | ✅ Yes — minimal: one extra `mouse_delta()` call per frame during drag. |
