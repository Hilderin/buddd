# IMPL-2026-06-INDEFINITE-DRAG — Inspector Indefinite Drag

## Source spec

`.specs/sprint-2026-06/inspector-indefinite-drag/spec.md`

## Goal

Enable indefinite drag-to-scrub on float and vec (Vec2, Vec3, Vec4, Quat) property handles in the editor inspector. On drag start, relative mouse mode is enabled and the cursor hidden; during the drag, value changes are computed from raw `InputSystem::mouse_delta().x` (accumulated per-frame) instead of `ImGui::GetMouseDragDelta()`; on release, relative mode is disabled, cursor is shown, and the cursor is warped back to the original drag-start position. This allows the user to drag arbitrarily far past window boundaries.

## Non-goals

- No changes to the int, bool, string, or Color editors.
- No changes to `src/editor/inspector_editors.h` (no interface change to `draw_axis_widget`).
- No changes to `src/engine/window/window.h`, `window_sdl3.h/.cpp`, or `window_headless.h/.cpp`.
- No changes to FreeCameraMovement (it already uses `Window::set_mouse_capture()` via right-click; property drag uses left-click — mutually exclusive).
- No CMakeLists.txt changes.
- No automated tests (verified via manual E2E testing).
- No wiki or ADR updates (deferred to the wiki-agent step).

## Relevant ADRs

- **ADR-012** (Navigable Object Graph / Engine Service): `Window::set_mouse_capture` already exists on the Window interface and is reachable via `ctx.engine.window`.
- **ADR-019** (Architecture Boundaries): SDL3 headers must not leak outside `src/engine/`. The new `set_mouse_position` on `InputSystem` and `set_sdl_window` on `InputSystemSDL3` keep SDL3 contained inside the engine layer. The editor (`src/editor/`) only calls abstract `InputSystem::set_mouse_position(int, int)`.
- **ADR-027** (Editor Architecture): The editor is a separate library that consumes engine APIs. `inspector_editors.cpp` lives in `src/editor/` and accesses engine services through `EngineContext`.
- **ADR-029** (Editor UX Decisions): No conflicts — indefinite drag is a UX improvement to existing property editors.

## Files to inspect

| File | Reason |
|---|---|
| `src/engine/input/input_system.h` | Existing abstract interface — add pure virtual `set_mouse_position` |
| `src/engine/input/input_system_sdl3.h` | Existing SDL3 implementation — add `sdl_window_` member and overrides |
| `src/engine/input/input_system_sdl3.cpp` | Existing SDL3 implementation — add `set_mouse_position` and `set_sdl_window` implementations |
| `src/engine/input/input_system_headless.h` | Headless implementation — add `set_mouse_position` override declaration |
| `src/engine/input/input_system_headless.cpp` | Headless implementation — add no-op `set_mouse_position` |
| `src/engine/platform/platform_sdl3.h` | Check `InputSystemSDL3` member and friend declaration |
| `src/engine/platform/platform_sdl3.cpp` | `create_window()` — set SDL window on input system after creation |
| `src/editor/inspector_editors.cpp` | `draw_axis_widget()` and float editor lambda — main behavioral changes |
| `src/editor/inspector_editors.h` | Verify no interface changes needed (confirm `draw_axis_widget` signature unchanged) |
| `src/engine/engine_context.h` | Understand `ctx.engine.services` / `ctx.engine.window` access pattern |
| `src/engine/engine_service.h` | Understand `platform()` return type for input system access |
| `src/engine/window/window.h` | Verify `set_mouse_capture` exists and signature (`bool captured`) |

## Files allowed to change

- `src/engine/input/input_system.h`
- `src/engine/input/input_system_sdl3.h`
- `src/engine/input/input_system_sdl3.cpp`
- `src/engine/input/input_system_headless.h`
- `src/engine/input/input_system_headless.cpp`
- `src/engine/platform/platform_sdl3.cpp`
- `src/editor/inspector_editors.cpp`

## Files forbidden to change

- `src/editor/inspector_editors.h` — no interface changes
- `src/engine/window/window.h` — no changes needed
- `src/engine/window/window_sdl3.h` — no changes needed
- `src/engine/window/window_sdl3.cpp` — no changes needed
- `src/engine/window/window_headless.h` — no changes needed
- `src/engine/window/window_headless.cpp` — no changes needed
- `src/engine/platform/platform_sdl3.h` — no changes needed (friend declaration already exists)
- `src/engine/platform/platform.h` — no changes needed
- `src/engine/engine_service.h` — no changes needed
- `src/engine/engine_context.h` — no changes needed
- Any `CMakeLists.txt` file
- Any test file

## Existing conventions to follow

- **Code style**: `auto func_name() -> void` return-type trailing syntax used throughout. Pure virtuals use `= 0`. Overrides marked `override`. Member variables use trailing underscore (`sdl_window_`).
- **InputSystemSDL3 constructor**: private, called only via friend `PlatformSDL3` or `InputSystem::create()`. Default constructor `InputSystemSDL3() = default;`.
- **Log macros**: Use `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "...")` and `BUDDD_LOG_TAGGED_WARN("Editor:Inspector", "...")` (already used in `inspector_editors.cpp`).
- **Namespace**: All engine code in `buddd::engine`, all editor code in `buddd::editor`.
- **Include guards**: `#pragma once` style used throughout.
- **Static state in editors**: `draw_axis_widget()` and the float editor lambda already use `static std::unordered_map<const void*, float>` for per-handle state. The contract extends this to a struct-based map — this pattern is established.
- **Accessing input system from editor**: Use `ctx.engine.services.platform().input_system()` which returns `InputSystem&`.
- **Accessing window from editor**: Use `ctx.engine.window.set_mouse_capture(...)` which calls the abstract `Window` interface.

## Required implementation behavior

### 1. Add `InputSystem::set_mouse_position` pure virtual

**File**: `src/engine/input/input_system.h`

Insert after `is_mouse_released()` declaration (line 52) and before the deleted copy/move operators (line 54):

```cpp
/// Set the OS cursor position to the given window client coordinates (top-left origin).
/// SDL3 backend: calls SDL_WarpMouseInWindow(). Headless: no-op.
/// Used to restore the cursor position after a relative-mouse-mode drag ends.
/// @param x  Window client X coordinate (screen pixels, top-left origin).
/// @param y  Window client Y coordinate (screen pixels, top-left origin).
virtual auto set_mouse_position(int x, int y) -> void = 0;
```

### 2. Extend `InputSystemSDL3` with `sdl_window_` and overrides

**File**: `src/engine/input/input_system_sdl3.h`

a) Add a public setter after the `~InputSystemSDL3()` override (line 14):

```cpp
/// Store the SDL_Window pointer for use by set_mouse_position().
/// Called by PlatformSDL3::create_window() after window creation.
void set_sdl_window(SDL_Window* window);
```

b) Add `set_mouse_position` override declaration after `is_mouse_released` (line 29):

```cpp
auto set_mouse_position(int x, int y) -> void override;
```

c) Add private member before the `// ── State (double-buffered) ──` section (after line 36):

```cpp
SDL_Window* sdl_window_{nullptr};
```

**File**: `src/engine/input/input_system_sdl3.cpp`

Add two new method implementations at the end of the `// ── Mouse state ──` section (after line 75, before `// ── SDL event processing ──`):

```cpp
auto InputSystemSDL3::set_mouse_position(int x, int y) -> void {
    if (sdl_window_) {
        SDL_WarpMouseInWindow(sdl_window_, static_cast<float>(x), static_cast<float>(y));
    }
}
```

Add the `set_sdl_window` implementation anywhere in the file (suggested location: before `on_sdl_event`):

```cpp
void InputSystemSDL3::set_sdl_window(SDL_Window* window) {
    sdl_window_ = window;
}
```

### 3. Extend `InputSystemHeadless` with no-op override

**File**: `src/engine/input/input_system_headless.h`

Add override declaration after `is_mouse_released` (line 23):

```cpp
auto set_mouse_position(int x, int y) -> void override;
```

**File**: `src/engine/input/input_system_headless.cpp`

Add implementation after `is_mouse_released` (after line 43):

```cpp
auto InputSystemHeadless::set_mouse_position(int /*x*/, int /*y*/) -> void {
    // No-op: headless mode has no cursor.
}
```

### 4. Wire `set_sdl_window` in `PlatformSDL3::create_window()`

**File**: `src/engine/platform/platform_sdl3.cpp`

In `PlatformSDL3::create_window()`, after the null-check guard (`if (sdl_window == nullptr) { ... }`) and before the `SDL_SetWindowMinimumSize` call, add:

```cpp
input_system_.set_sdl_window(sdl_window);
```

This must appear after `SDL_CreateWindow` succeeds and before `SDL_SetWindowMinimumSize`. The exact insertion point is between line 174 and line 176.

### 5. Implement indefinite drag in `draw_axis_widget()`

**File**: `src/editor/inspector_editors.cpp`

#### 5a. Add includes at top of file

After the existing `#include <limits>` (line 17), add:

```cpp
#include "engine_service.h"
#include "input/input_system.h"
#include "platform/platform.h"
```

Note: The implementer must verify whether these are already transitively included. If any is already available via the existing include chain (`editor.h` → `scene/world.h` → `engine_context.h`), it may be skipped. At minimum, `input_system.h` and `platform.h` must be included since `InputSystem` and `Platform` are only forward-declared elsewhere in the include chain.

#### 5b. Define the DragState struct

At the top of the anonymous namespace (after line 22 `namespace {`), add:

```cpp
/// Per-handle state for indefinite drag-to-scrub using relative mouse mode.
struct DragState {
    float initial_value;      ///< Value when the drag started.
    float drag_accumulator;   ///< Accumulated raw mouse delta since drag start.
    float start_x;            ///< ImGui window X coordinate at drag start.
    float start_y;            ///< ImGui window Y coordinate at drag start.
};
```

#### 5c. Rewrite drag handling in `draw_axis_widget()`

Replace the body of `draw_axis_widget()` starting at line 47 (the `(void)ctx;` line) with the following:

- **Remove** line 47: `(void)ctx;  // reserved for future use` — ctx is now used.
- **Replace** the `static` map type from `std::unordered_map<const void*, float>` to `std::unordered_map<const void*, DragState>`.
- **Replace** the drag-handling block (lines 83–102) as follows:

```cpp
    // Indefinite drag-to-scrub using relative mouse mode
    static std::unordered_map<const void*, DragState> drag_states;
    bool drag_changed = false;

    if (ImGui::IsItemActive()) {
        if (ImGui::IsItemActivated()) {
            // Save initial state and enable relative mouse mode
            DragState ds;
            ds.initial_value = *value;
            ds.drag_accumulator = 0.0f;
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ds.start_x = mouse_pos.x;
            ds.start_y = mouse_pos.y;
            drag_states[static_cast<const void*>(value)] = ds;

            ctx.engine.window.set_mouse_capture(true);

            BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                "drag start: handle={} initial_value={}", id, ds.initial_value);
        }

        auto& ds = drag_states[static_cast<const void*>(value)];
        auto& input = ctx.engine.services.platform().input_system();
        ds.drag_accumulator += input.mouse_delta().first;
        float new_val = ds.initial_value + ds.drag_accumulator * drag_speed * 0.01f;

        if (new_val != *value) {
            *value = new_val;
            drag_changed = true;
        }
    }

    if (ImGui::IsItemDeactivated()) {
        auto it = drag_states.find(static_cast<const void*>(value));
        if (it != drag_states.end()) {
            const auto& ds = it->second;

            ctx.engine.window.set_mouse_capture(false);

            auto& input = ctx.engine.services.platform().input_system();
            input.set_mouse_position(static_cast<int>(ds.start_x),
                                     static_cast<int>(ds.start_y));

            BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                "drag end: handle={} final_value={}", id, *value);
            BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                "warp mouse to ({}, {})", ds.start_x, ds.start_y);

            drag_states.erase(it);
        }
    }
```

### 6. Implement indefinite drag in the float editor lambda

**File**: `src/editor/inspector_editors.cpp`

In the float editor lambda (lines 146–198), apply the same transformation:

- **Replace** `static std::unordered_map<const void*, float> initial_values;` (line 173) with `static std::unordered_map<const void*, DragState> drag_states;`
- **Replace** the drag-handling body (lines 174–188) with the same pattern as step 5c, with these adaptations:
  - The variable `speed` is used instead of `drag_speed`.
  - The value pointer is `&value` (a `float&`) not `value` (a `float*`).
  - The `ImGui::IsItemDeactivated` block uses `&value` as the key.

The complete replacement for lines 173–188:

```cpp
            // Indefinite drag-to-scrub using relative mouse mode
            static std::unordered_map<const void*, DragState> drag_states;
            if (ImGui::IsItemActive()) {
                if (ImGui::IsItemActivated()) {
                    DragState ds;
                    ds.initial_value = value;
                    ds.drag_accumulator = 0.0f;
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    ds.start_x = mouse_pos.x;
                    ds.start_y = mouse_pos.y;
                    drag_states[static_cast<const void*>(&value)] = ds;

                    ctx.engine.window.set_mouse_capture(true);

                    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                        "drag start: handle={} initial_value={}", label.c_str(), ds.initial_value);
                }

                auto& ds = drag_states[static_cast<const void*>(&value)];
                auto& input = ctx.engine.services.platform().input_system();
                ds.drag_accumulator += input.mouse_delta().first;
                float new_val = ds.initial_value + ds.drag_accumulator * speed * 0.01f;

                if (new_val != value) {
                    value = new_val;
                    changed = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                auto it = drag_states.find(static_cast<const void*>(&value));
                if (it != drag_states.end()) {
                    const auto& ds = it->second;

                    ctx.engine.window.set_mouse_capture(false);

                    auto& input = ctx.engine.services.platform().input_system();
                    input.set_mouse_position(static_cast<int>(ds.start_x),
                                             static_cast<int>(ds.start_y));

                    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                        "drag end: handle={} final_value={}", label.c_str(), value);
                    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                        "warp mouse to ({}, {})", ds.start_x, ds.start_y);

                    drag_states.erase(it);
                }
            }
```

### 7. Verify includes compile

The implementer must verify that `inspector_editors.cpp` compiles with the new includes. The required complete types for the call chain `ctx.engine.services.platform().input_system()` are:
- `EngineService` (complete via `engine_service.h`)
- `Platform` (complete via `platform/platform.h`)
- `InputSystem` (complete via `input/input_system.h`)

If any of these are already transitively included through existing headers, the corresponding `#include` may be omitted.

### 8. Logging behavior (spec reference: spec lines 306-311)

All logging uses the existing `log/log.h` include already present in `inspector_editors.cpp`. No additional logging includes are needed.

| Event | Log call | Condition |
|---|---|---|
| Relative mode activation | `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "drag start: handle={} initial_value={}", id, ds.initial_value)` | On `IsItemActivated` |
| Relative mode deactivation | `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "drag end: handle={} final_value={}", id, *value)` | On `IsItemDeactivated` |
| Warp mouse | `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "warp mouse to ({}, {})", ds.start_x, ds.start_y)` | On `IsItemDeactivated` when `set_mouse_position` called |
| Relative mode failure | `BUDDD_LOG_TAGGED_WARN("Editor:Inspector", "set_mouse_capture(true) failed")` | On `IsItemActivated` after `set_mouse_capture(true)` returns false |

**Important**: `Window::set_mouse_capture` currently returns `void` (see `window.h` line 42). The spec mentions logging when it fails, but the current `Window` interface returns `void`. The implementer must:
1. Check if `WindowSDL3::set_mouse_capture` has a meaningful return value (currently `void` at window.h line 42 and window_sdl3.cpp line 39-47 — it is `void`).
2. If the interface remains `void`, remove the failure log since there is no return value to check. (The spec's failure log is aspirational; the current interface does not support it.)
3. Document this discrepancy in the implementation.

### 9. No-change verification

The implementer must confirm that even after editing `inspector_editors.cpp`, the int editor (lines 200-213), bool editor (lines 216-226), string editor (lines 229-244), Vec2 editor (lines 248-271), Vec3 editor (lines 274-301), Vec4 editor (lines 304-335), Quat editor (lines 338-394), and Color editor (lines 397-433) are unchanged. Only `draw_axis_widget()` and the float editor lambda are modified.

## Required tests

### Unit tests

None. The spec explicitly excludes automated tests (NG-05: "the feature relies on OS-level relative mouse mode which cannot be meaningfully tested in a headless environment").

### E2E / Integration verification

Manual smoke test procedure (from spec):

1. Run `buddd edit` with a scene loaded. Select an entity.
2. **Drag past window boundary**: Click+drag the Position X handle past the window edge → verify the value continues changing and the cursor is hidden.
3. **Release outside window**: Drag past window edge, release the mouse button while outside the window → verify the value stops changing and the cursor reappears at the original click position.
4. **Rotation handle**: Drag a Rotation (Quat) handle → same indefinite drag behavior.
5. **Click-to-type**: Single-click the InputFloat portion of a composite axis widget → verify text entry works without triggering relative mouse mode (cursor remains visible, no capture).
6. **FreeCameraMovement**: Right-click+drag in the viewport → verify camera controls work. Then drag a property handle in the Inspector → verify camera does not move.
7. **Int editor**: Verify `ImGui::DragInt` still works unchanged for int properties.
8. **Build verification**: `cmake --build --preset debug` with zero new warnings from `src/editor/` and `src/engine/input/`.

## Edge cases

All edge cases from the spec must be handled (spec lines 168-179). The implementation must pass the following checks:

| Edge case | Expected behavior | How contract enforces |
|---|---|---|
| **Window focus loss during drag** | Relative mode released by SDL; `set_mouse_capture(false)` called on `IsItemDeactivated()`. If focus loss without button release, the map entry stays but is cleaned up on next activation. | The `IsItemDeactivated` block calls `set_mouse_capture(false)` — this is a no-op if already released by SDL. The `drag_states` map entry persists until the handle is deactivated or re-activated, but the stale entry causes no harm (overwritten on next activation). |
| **ESC during drag** | Drag continues until mouse button release. No special ESC handling. Cursor remains hidden. | No ESC handling is added. The drag state is only cleaned up on `IsItemDeactivated`. |
| **Multiple monitors, different DPI** | Raw mouse deltas from SDL are device-independent. `drag_accumulator * drag_speed * 0.01f` handles the conversion. | Implementation uses `InputSystem::mouse_delta().first` which returns raw SDL xrel values. No DPI-aware math is applied. |
| **Headless mode** | `WindowHeadless::set_mouse_capture()` is a no-op. `InputSystemHeadless::set_mouse_position()` is a no-op. `InputSystemHeadless::mouse_delta()` returns `(0,0)`. Drag code path exists but produces no value change. | The `InputSystemHeadless` overrides are specified as no-ops. The drag code in `inspector_editors.cpp` is unconditionally compiled — in headless mode, `mouse_delta()` returns 0, so no value change occurs. |
| **Rapid click (no drag)** | `IsItemActivated` + `IsItemDeactivated` fire in same frame. `set_mouse_capture(true/false)` called but no relative mode transition. Cursor not warped (accumulator is 0). | The code calls `set_mouse_capture(true)` on activated and `set_mouse_capture(false)` on deactivated. If both fire in the same frame, the net effect is a no-op. |
| **Dragging two handles simultaneously** | Not possible — single mouse cursor, unique `ImGui::PushID` scope per handle. Only one active at a time. | The `drag_states` map is keyed by `value` pointer — each handle has a distinct value pointer, so state is per-handle. |
| **Concurrent FreeCameraMovement** | FreeCamera uses right-click, property drag uses left-click. Mutually exclusive input actions. | No cross-system interaction. Both independently call `set_mouse_capture()` but on different mouse buttons. |
| **Very rapid drag (1000Hz mouse)** | `InputSystem::mouse_delta()` accumulates per-frame. Code reads it once per frame during `IsItemActive()`. | The accumulator is read once per frame from `mouse_delta().first`. Multiple SDL motion events between frames are already summed by `InputSystemSDL3::on_sdl_event()`. |
| **Drag start position at window edge** | Saved `start_x`/`start_y` from `ImGui::GetMousePos()` are valid window coordinates. `set_mouse_position(static_cast<int>(start_x), static_cast<int>(start_y))` correctly returns cursor there. | No special handling needed — `ImGui::GetMousePos()` returns valid window coordinates even near the edge. |
| **Zero-size or minimized window** | `set_mouse_position` on minimized window is a no-op (SDL3 handles gracefully). Relative mouse mode cannot be enabled on minimized window (SDL3 returns error, ignored by set_mouse_capture). | `InputSystemSDL3::set_mouse_position` checks `sdl_window_` before calling `SDL_WarpMouseInWindow`. SDL3 handles the minimized case internally. |

## Security impact

None. The feature uses OS-level relative mouse mode — a standard desktop input mechanism. No authentication, authorization, file I/O, or network access is involved. No new input validation surfaces are introduced (mouse deltas are already consumed by the existing `InputSystem`).

## Data and migration impact

None. No schema changes, no data migrations, no seed data changes, and no data loss risks.

## API compatibility impact

**Backward-compatible addition** to the `InputSystem` interface:
- New pure virtual `set_mouse_position(int x, int y) -> void` is added to the `InputSystem` abstract base class. Any existing subclass not updated will fail to compile (linker error on the pure virtual). Since only `InputSystemSDL3` and `InputSystemHeadless` exist, and both are updated in this contract, no external compatibility break occurs.
- No existing methods are removed or changed.
- No CMakeLists.txt changes needed.

## Documentation impact

- README: none
- Wiki pages: `docs/wiki/editor/editor-panels.md` — the Inspector Property Editors section should be updated after implementation to describe the indefinite-drag behavior (deferred to wiki-agent step per NG-04).
- Other specs: none

## ADR impact

No new ADR is required. The implementation follows the existing patterns established by ADR-012 (Window::set_mouse_capture), ADR-026 (ImGui integration), and ADR-027 (editor architecture). The addition of `InputSystem::set_mouse_position()` is a natural extension of the existing `InputSystem` interface and does not warrant a new ADR.

## Done criteria

All of the following must be verifiable after implementation:

- [ ] **DC-01**: `src/engine/input/input_system.h` contains pure virtual `set_mouse_position(int x, int y) -> void = 0`.
- [ ] **DC-02**: `src/engine/input/input_system_sdl3.h` declares `set_sdl_window(SDL_Window*)`, `set_mouse_position(int, int) override`, and private member `SDL_Window* sdl_window_{nullptr}`.
- [ ] **DC-03**: `src/engine/input/input_system_sdl3.cpp` implements `set_mouse_position` via `SDL_WarpMouseInWindow` (with null check) and `set_sdl_window` stores the pointer.
- [ ] **DC-04**: `src/engine/input/input_system_headless.h` declares `set_mouse_position(int, int) override`.
- [ ] **DC-05**: `src/engine/input/input_system_headless.cpp` implements `set_mouse_position` as a no-op.
- [ ] **DC-06**: `src/engine/platform/platform_sdl3.cpp` calls `input_system_.set_sdl_window(sdl_window)` after `SDL_CreateWindow` succeeds in `create_window()`.
- [ ] **DC-07**: `src/editor/inspector_editors.cpp` no longer has `(void)ctx;` in `draw_axis_widget()`.
- [ ] **DC-08**: `draw_axis_widget()` uses a `static std::unordered_map<const void*, DragState>` (with struct containing `initial_value`, `drag_accumulator`, `start_x`, `start_y`) instead of the old `std::unordered_map<const void*, float>` for `initial_values`.
- [ ] **DC-09**: `draw_axis_widget()` calls `ctx.engine.window.set_mouse_capture(true)` on `IsItemActivated` and `ctx.engine.window.set_mouse_capture(false)` + `input.set_mouse_position(start_x, start_y)` on `IsItemDeactivated`.
- [ ] **DC-10**: `draw_axis_widget()` accumulates `input.mouse_delta().first` into `drag_accumulator` during `IsItemActive()` instead of using `ImGui::GetMouseDragDelta()`.
- [ ] **DC-11**: The float editor lambda (in `register_editor<float>(...)`) uses the same `DragState` struct and relative-mouse drag pattern as `draw_axis_widget()`.
- [ ] **DC-12**: The int editor lambda is unchanged (still uses `ImGui::DragInt`).
- [ ] **DC-13**: Build succeeds with `cmake --build --preset debug` with zero new warnings in `src/editor/`, `src/engine/input/`, and `src/engine/platform/`.
- [ ] **DC-14**: All three logging calls are present: "drag start" on activate, "drag end" on deactivate, "warp mouse" on deactivate.
- [ ] **DC-15**: Reset behavior: rapid click (no drag) does not leave stale state — `IsItemActivated` + `IsItemDeactivated` fire in the same frame, `set_mouse_capture(true/false)` balanced, map entry created and erased, no value change.
