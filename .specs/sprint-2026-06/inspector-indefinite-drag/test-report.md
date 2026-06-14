# Test Report: Inspector Indefinite Drag

## Test Summary

**Total tests**: 833
**Passed**: 833
**Failed**: 0
**Skipped**: 0

**Build**: clean (zero errors, zero warnings in `src/` and `tests/`)

---

## Unit Tests

All 833 existing unit tests pass with no regressions. No new unit tests were added (the feature relies on OS-level relative mouse mode which cannot be meaningfully tested in a headless environment — see NG-05 in the spec).

---

## Integration / E2E Tests

| Scenario | Method | Result | Evidence |
|---|---|---|---|
| N/A — No automated E2E possible | N/A | N/A | See Manual Tests Required below |

### Visual analysis notes

This feature produces no visual output that can be captured in a headless/offscreen environment. The behavior is entirely interactive (mouse cursor state, relative movement, window edge crossing). Verification requires a human tester running the editor on a display.

---

## Regression Checks

| App / Module | Check performed | Result | Evidence |
|---|---|---|---|
| Full test suite | All 833 tests pass | PASS | `ctest --preset debug --output-on-failure` — 100% passed, 0 failed |
| Build warnings | `cmake --build --preset debug --clean-first` | PASS | Zero new warnings in `src/editor/`, `src/engine/input/`, `src/engine/platform/` |
| Forbidden files unchanged | `git diff --name-only HEAD` | PASS | Only 7 allowed files modified. All forbidden files (window/*, engine_service.h, engine_context.h, inspector_editors.h, CMakeLists.txt, etc.) are unchanged. |
| Int editor | Code inspection | PASS | Still uses `ImGui::DragInt` (lines 273–285 of `inspector_editors.cpp`) — unchanged. |
| Bool editor | Code inspection | PASS | Still uses `ImGui::Checkbox` (lines 288–298) — unchanged. |
| String editor | Code inspection | PASS | Still uses `ImGui::InputText` (lines 301–317) — unchanged. |
| Color editor | Code inspection | PASS | Still uses `ImGui::ColorEdit3/ColorEdit4` (lines 469–505) — unchanged. |
| Project build | `cmake --build --preset debug` | PASS | Full build succeeds, `buddd` and `buddd_tests` binaries produced. |

No regressions detected.

---

## Done Criteria Verification (Implementation Contract)

| ID | Description | Status | Evidence |
|---|---|---|---|
| DC-01 | `input_system.h` contains pure virtual `set_mouse_position(int, int) -> void = 0` | ✅ PASS | Lines 54–59 |
| DC-02 | `input_system_sdl3.h` declares `set_sdl_window`, `set_mouse_position` override, `sdl_window_` member | ✅ PASS | Lines 18, 35, 52 |
| DC-03 | `input_system_sdl3.cpp` implements `set_mouse_position` via `SDL_WarpMouseInWindow` with null check, `set_sdl_window` stores pointer | ✅ PASS | Lines 95–103 |
| DC-04 | `input_system_headless.h` declares `set_mouse_position(int, int) override` | ✅ PASS | Line 25 |
| DC-05 | `input_system_headless.cpp` implements `set_mouse_position` as no-op | ✅ PASS | Lines 45–47 |
| DC-06 | `platform_sdl3.cpp` calls `input_system_.set_sdl_window(sdl_window)` after `SDL_CreateWindow` | ✅ PASS | Line 176 |
| DC-07 | `draw_axis_widget()` no longer has `(void)ctx;` | ✅ PASS | Line `(void)ctx;` removed |
| DC-08 | `draw_axis_widget()` uses `static std::unordered_map<const void*, DragState>` | ✅ PASS | Line 95 |
| DC-09 | `draw_axis_widget()` calls `set_mouse_capture(true)` on activate, `set_mouse_capture(false)` + `set_mouse_position()` on deactivate | ✅ PASS | Lines 109, 131–135 |
| DC-10 | `draw_axis_widget()` accumulates `input.mouse_delta().first` into `drag_accumulator` during `IsItemActive()` | ✅ PASS | Lines 116–118 |
| DC-11 | Float editor lambda uses same `DragState` + relative-mouse pattern | ✅ PASS | Lines 214–260 |
| DC-12 | Int editor lambda unchanged (still uses `ImGui::DragInt`) | ✅ PASS | Lines 273–285 |
| DC-13 | Build succeeds with zero new warnings in modified source paths | ✅ PASS | Build clean |
| DC-14 | All three log calls present (drag start, drag end, warp mouse) | ✅ PASS | `draw_axis_widget`: lines 111–112, 137–140; float editor: lines 228–229, 253–256 |
| DC-15 | Rapid click (no drag) — `IsItemActivated` + `IsItemDeactivated` in same frame, state balanced, no value change | ✅ PASS | Code pattern — set_mouse_capture(true/false) balanced, entry created/erased |

---

## Manual Tests Required

The feature relies on OS-level relative mouse mode and cross-window-boundary interaction, which cannot be meaningfully tested in a headless/CI environment. The following manual smoke test must be performed by a human on a system with a display and a mouse.

### Manual Test Procedure

**Prerequisites:**
- A display with a window manager running (Linux/X11 or Wayland, Windows, or macOS).
- A mouse with at least one button.
- A scene file loaded (create a new scene or use an existing one).

**Test 1: Drag past window boundary (AC-001, AC-002, SC-001)**

1. Run `buddd edit` with a scene loaded. Select an entity that has a Position component.
2. Locate the Position X handle in the Inspector panel (the red colored axis widget).
3. Click and hold the left mouse button on the Position X colored handle.
4. **Verify**: The cursor disappears (hidden by relative mouse mode).
5. **Verify**: The Position X value changes in the InputFloat field as you drag left/right.
6. Continue dragging past the right edge of the editor window.
7. **Verify**: The Position X value continues to change even when the cursor is outside the window.
8. Move the cursor further right (multiple window-widths away if possible).
9. **Verify**: The value continues to increase proportionally to mouse movement.

**Test 2: Release outside window (AC-003, AC-004, SC-002, SC-003)**

1. Repeat steps 1–3 of Test 1.
2. Drag past the window edge.
3. While the cursor is outside the window, release the mouse button.
4. **Verify**: The value stops changing (the drag ends cleanly).
5. **Verify**: The cursor reappears at the position where the original drag started (the handle click location inside the window), not at the current mouse position outside the window.
6. Move the mouse back inside the window.
7. **Verify**: The value does not jump unexpectedly (no stuck drag state).

**Test 3: Rotation handle (Quat) — AC-001/002 on Quat**

1. Select an entity with a Rotation component (or switch to rotation display).
2. Click and drag a Rotation handle (X=Pitch, Y=Yaw, Z=Roll).
3. **Verify**: The same indefinite drag behavior works — cursor hidden, value changes past window boundary, cursor warps back on release.

**Test 4: Click-to-type still works (AC-005, SC-004)**

1. Select an entity with a Position component.
2. Single-click the InputFloat text field of the Position X widget (not the colored handle).
3. **Verify**: The field enters text edit mode (cursor appears blinking in the field).
4. **Verify**: The mouse cursor remains visible (relative mouse mode is NOT enabled).
5. Type a new value (e.g., `42.5`) and press Enter.
6. **Verify**: The value changes to the typed value.
7. Click the InputFloat field again, type a value, then press Escape.
8. **Verify**: The value reverts to the previous value.

**Test 5: FreeCameraMovement no interference (AC-006, SC-004)**

1. Ensure the viewport is visible.
2. Right-click and drag in the viewport to orbit the camera.
3. **Verify**: Camera controls work as before (no unintended behavior).
4. While not dragging a property handle, verify no unintended value changes appear in the Inspector.
5. Now click and drag a property handle in the Inspector (left-click on a colored handle).
6. **Verify**: The property value changes based on mouse movement.
7. **Verify**: The camera does NOT move while dragging the property handle.

**Test 6: Int editor unchanged (SC-004, AC-010)**

1. Select an entity or component that has an integer property (e.g., a counter, index, or similar).
2. **Verify**: The int property uses `ImGui::DragInt` — drag left/right changes the value but only while the cursor is inside the window (standard behavior, unchanged).

**Test 7: Bool, string, Color editors unchanged (SC-004)**

1. Verify that bool properties still display as a checkbox and toggle correctly.
2. Verify that string properties still display as an InputText field and accept text input.
3. Verify that Color properties still display as a ColorEdit3/ColorEdit4 widget and open a color picker on click.

---

## Issues Found

### Blocking

- None.

### Non-blocking

- The spec's aspirational failure log `BUDDD_LOG_TAGGED_WARN("Editor:Inspector", "set_mouse_capture(true) failed")` (spec line 311) cannot be implemented because `Window::set_mouse_capture` currently returns `void`. This was documented in the implementation contract and is not a regression.
- Automated testing is not possible for this feature due to reliance on OS-level relative mouse mode. The feature depends on SDL's `SDL_SetWindowRelativeMouseMode` and `SDL_WarpMouseInWindow` which require a real window and display. Headless mode cannot meaningfully verify the behavior.
