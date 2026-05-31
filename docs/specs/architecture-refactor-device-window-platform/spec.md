# SPEC-016 — Architecture Refactor: Navigable Object Graph (RenderDevice → Window → Platform → InputSystem)

## Status

`Draft`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|
| Approved by | |
| Date | |
| Time | |

## Problem

The current architecture requires demos and other engine-layer consumers to receive both `Platform&` and `RenderDevice&` as separate parameters. This creates several pain points:

1. **Redundant parameter passing**: Every demo function signature (`run_triangle_demo`, `run_cube_demo`, `run_cube_scene_demo`, `run_free_camera_demo`) must accept `Platform&` plus `RenderDevice&`, even when the demo only needs `Platform` for event polling.
2. **No back-link from Window to Platform**: `Window` is created by `Platform` but has no reference back to its creator. There is no way to reach `Platform` (or `InputSystem`) from a `Window` reference.
3. **No back-link from RenderDevice to Window**: `RenderDevice` is created from a `Window` but has no reference back to it. There is no way to reach `Window` (or its `set_mouse_capture` or other window-level features) from a `RenderDevice` reference.
4. **Mouse-capture blocker**: Adding mouse capture (`SDL_SetWindowRelativeMouseMode`) requires access to `Window`, but `Window` is not reachable from code paths that currently hold only `RenderDevice&` (e.g., demos, future gameplay systems).

Without a navigable object graph, every feature that touches `Window`, `Platform`, or `InputSystem` requires threading additional references through the call chain, making the API surface grow and the coupling tighter with each addition.

## Goals

- **G-01**: Establish a navigable object graph: `RenderDevice → Window → Platform → InputSystem`. From any `RenderDevice&`, the entire upstream object graph must be reachable.
- **G-02**: Add `set_mouse_capture(bool)` and `is_mouse_captured() -> bool` to the `Window` abstract interface, with SDL3 and headless backend implementations.
- **G-03**: Remove the redundant `Platform&` parameter from all four demo function signatures. Demos access `Platform` via the navigable graph.
- **G-04**: Keep all existing behavior, output, and test expectations unchanged — pure refactoring.
- **G-05**: Add mouse-capture camera control to the free camera demo: right-click to capture, camera movement/rotation only while captured (Godot editor pattern).

## Non-goals

- No changes to the input system (`InputSystem` stays on `Platform`, no ownership or interface changes).
- No new features beyond `set_mouse_capture` / `is_mouse_captured`.
- No changes to the render system (`RenderSystem`, `MeshRenderer`, `Model`, etc.).
- No multi-window implementation — only preparation for the future (the navigable graph supports it, but only one active window remains).
- No backend selection changes.
- No changes to `demo_helpers.h/.cpp` (their functions already accept only `RenderDevice&`).
- No changes to `CaptureCommand` or capture scenarios — they already receive `RenderDevice&` only.
- No behavioral changes in demos other than the free camera demo (triangle, cube, cube-scene remain identical).

## Actors

| Actor | Description |
|---|---|
| Core engine developer | Maintains `Window`, `RenderDevice`, `Platform`, and `InputSystem` abstractions and their concrete backends. |
| Demo author | Writes demo functions that currently accept `Platform&` and `RenderDevice&`. After the refactor, they receive only `RenderDevice&`. |
| Test author | Writes unit tests that construct `RenderDeviceHeadless` directly. Must update constructions to use `EngineService::create(Backend::Headless, config)` instead. |
| SDK consumer | Code outside `src/engine/` that uses the engine API. Gains navigable access to `Window`, `Platform`, and `InputSystem` from a single `RenderDevice&` entry point. |

## User-visible behavior

The architectural refactoring (navigable object graph, `Window` back-link from `RenderDevice`, `Platform&` parameter removal) has no behavioral effect — the `triangle`, `cube`, and `cube-scene` demos produce exactly the same output, frame count, and window title as before.

The free camera demo (`buddd demo free-camera`) gains a new interaction mode on top of the refactoring: holding the right mouse button captures the mouse (relative mouse mode, cursor hidden), enabling camera movement (WASD) and rotation (mouse look). Releasing the right button releases the mouse capture (cursor visible, normal mouse mode). Camera controls are disabled while the mouse is not captured. This matches the Godot editor camera control pattern.

## User stories

### Story 1 — Demo author can access Platform from RenderDevice (Priority: P1)

As a demo author, I want to receive only `RenderDevice&` in my demo function and still access `Platform` (for event polling, delta time) so that the API surface is simpler and I do not need to thread extra references through the call chain.

**Given** a demo function that receives `buddd::engine::RenderDevice& device`
**When** I call `device.window().platform()`
**Then** I obtain a `buddd::engine::Platform&` referencing the Platform that created the Window.

### Story 2 — Demo author can access InputSystem from RenderDevice (Priority: P1)

As a demo author, I want to access the `InputSystem` from a `RenderDevice&` without needing a separate `Platform&` parameter.

**Given** a demo function that receives `buddd::engine::RenderDevice& device`
**When** I call `device.window().platform().input_system()`
**Then** I obtain a `buddd::engine::InputSystem&` referencing the input system owned by the Platform.

### Story 3 — Developer can capture mouse from Window (Priority: P1)

As a developer, I want to call `window.set_mouse_capture(true)` to enable relative mouse mode (hide cursor, capture mouse) and `window.is_mouse_captured()` to query the current state.

**Given** a `Window&` reference
**When** I call `window.set_mouse_capture(true)`
**Then** the cursor is hidden and relative mouse motion is active (SDL3 backend) or nothing happens (headless backend).

**Given** a `Window&` reference with mouse capture enabled
**When** I call `window.is_mouse_captured()`
**Then** it returns `true` (SDL3 backend) or `false` (headless backend).

### Story 4 — Demo functions no longer accept Platform& (Priority: P1)

As a CLI maintainer, I want `demo_command.cpp` to pass only `RenderDevice&` (and argc/argv) to demo functions so that the dispatch code is simpler and does not need to hold a separate `Platform&` after device creation.

**Given** `demo_command.cpp` dispatches to a demo function
**When** the dispatch call is invoked
**Then** it passes `(**device, argc-2, argv+2)` instead of `(**platform, **device, argc-2, argv+2)`.

### Story 5 — Existing demos navigate through RenderDevice (Priority: P2)

As a demo author, I want the four existing demos to compile and run after the refactor with no behavioral change, accessing `Platform` through the object graph where needed.

**Given** `run_cube_demo` (and `triangle`, `cube-scene`, `free-camera`)
**When** they call `device.window().platform().poll_events()` in place of `platform.poll_events()`
**Then** the demo runs identically to the pre-refactor version.

### Story 6 — Free camera demo captures mouse on right-click (Priority: P1)

As a user, I want to hold the right mouse button in the free camera demo to capture the mouse (relative mode, cursor hidden), move and rotate the camera while captured, and release to return to normal mouse mode, following the Godot editor pattern.

**Given** the free camera demo is running
**When** I press and hold the right mouse button
**Then** the mouse cursor is hidden and relative mouse mode is activated (mouse captured).

**Given** the free camera demo is running with mouse captured
**When** I move the mouse while right-click is held
**Then** the camera rotates (yaw/pitch) following mouse movement.

**Given** the free camera demo is running with mouse captured
**When** I press WASD keys while right-click is held
**Then** the camera moves forward/backward/left/right.

**Given** the free camera demo is running with mouse captured
**When** I release the right mouse button
**Then** the mouse cursor is shown again and relative mouse mode is deactivated.

**Given** the free camera demo is running without mouse capture
**When** I move the mouse or press WASD keys
**Then** the camera does NOT move or rotate.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `Window` base class declares `auto platform() noexcept -> Platform&` returning a non-owning reference to the creating `Platform`. | Inspect `window.h`. |
| AC-002 | `Window` base class declares `virtual auto set_mouse_capture(bool captured) -> void = 0`. | Inspect `window.h`. |
| AC-003 | `Window` base class declares `virtual auto is_mouse_captured() const noexcept -> bool = 0`. | Inspect `window.h`. |
| AC-004 | `WindowSDL3` passes `Platform&` to the `Window` base constructor and stores it. | Unit test: create a `WindowSDL3` via `PlatformSDL3`, call `window->platform()`, verify it returns the same platform reference. |
| AC-005 | `WindowSDL3::set_mouse_capture(true)` calls `SDL_SetWindowRelativeMouseMode(window_, true)`. | Unit test: verify `window->is_mouse_captured()` returns `true` after `set_mouse_capture(true)`. |
| AC-006 | `WindowSDL3::set_mouse_capture(false)` calls `SDL_SetWindowRelativeMouseMode(window_, false)`. | Unit test: verify `window->is_mouse_captured()` returns `false` after `set_mouse_capture(false)`. |
| AC-007 | `WindowSDL3::is_mouse_captured()` calls `SDL_GetWindowRelativeMouseMode(window_)` or returns stored state. | Unit test: verify consistency with `set_mouse_capture`. |
| AC-008 | `WindowHeadless` passes `Platform&` to the `Window` base constructor and stores it. | Unit test: create a `WindowHeadless` via `PlatformHeadless`, call `window->platform()`, verify it returns the same platform reference. |
| AC-009 | `WindowHeadless::set_mouse_capture(bool)` is a no-op. | Unit test: call `set_mouse_capture(true)` on a headless window, verify `is_mouse_captured()` returns `false`. |
| AC-010 | `WindowHeadless::is_mouse_captured()` returns `false`. | Unit test: verify returns `false` unconditionally. |
| AC-011 | `RenderDevice` base class declares `virtual auto window() noexcept -> Window& = 0`. | Inspect `render_device.h`. |
| AC-012 | `RenderDeviceOpenGL` stores a `Window&` (in addition to `SDL_Window*`) and implements `window()` to return it. | Unit test: create a `RenderDeviceOpenGL` via the factory path, call `device->window()`, verify the reference is valid and matches the input `Window`. |
| AC-013 | `RenderDeviceHeadless` stores a `Window&` (replacing `int width_, height_`) and implements `window()` to return it. | Unit test: create a `RenderDeviceHeadless(Window&)`, call `device.window()`, verify it returns the same reference. |
| AC-014 | `RenderDevice::create(Window&)` passes the `Window&` to both backend constructors (`RenderDeviceHeadless(Window&)` and `RenderDeviceOpenGL(Window&, ...)`). | Inspect `render_device.cpp`. |
| AC-015 | `RenderDeviceHeadless::size()` delegates to `window_.width()` and `window_.height()`. | Unit test: create a `WindowHeadless(800, 600)`, create a `RenderDeviceHeadless` from it, verify `device.size()` returns `{800, 600}`. |
| AC-016 | `PlatformSDL3::create_window()` passes `*this` to `WindowSDL3` constructor. | Inspect `platform_sdl3.cpp`. |
| AC-017 | `PlatformHeadless::create_window()` passes `*this` to `WindowHeadless` constructor. | Inspect `platform_headless.cpp`. |
| AC-018 | Demo function `run_triangle_demo` signature changes from `(Platform&, RenderDevice&, int, const char* const*)` to `(RenderDevice&, int, const char* const*)`. | Inspect `triangle_demo.h`. |
| AC-019 | Demo function `run_cube_demo` signature changes from `(Platform&, RenderDevice&, int, const char* const*)` to `(RenderDevice&, int, const char* const*)`. | Inspect `cube_demo.h`. |
| AC-020 | Demo function `run_cube_scene_demo` signature changes from `(Platform&, RenderDevice&, int, const char* const*)` to `(RenderDevice&, int, const char* const*)`. | Inspect `cube_scene_demo.h`. |
| AC-021 | Demo function `run_free_camera_demo` signature changes from `(Platform&, RenderDevice&, int, const char* const*)` to `(RenderDevice&, int, const char* const*)`. | Inspect `free_camera_demo.h`. |
| AC-022 | `demo_command.cpp` dispatch calls pass only `(**device, argc-2, argv+2)` — no `**platform` argument. | Inspect `demo_command.cpp`. |
| AC-023 | All four demo functions compile with `(RenderDevice&, int, const char* const*)` signatures (no `Platform&` parameter). | Inspect each demo header for the updated signature. |
| AC-024 | All existing unit tests pass. | Run `ctest --preset debug` — all tests pass. |
| AC-025 | `RenderDeviceHeadless(Window&)` constructor replaces `RenderDeviceHeadless(int, int)` — `width_` and `height_` members are removed. | Inspect `render_device_headless.h`. |
| AC-026 | `WindowSDL3` stores a `Platform&` (protected member in `Window` base class). | Inspect `window.h` for `Platform& platform_`. |
| AC-027 | `WindowHeadless()` constructor accepts `(int, int, Platform&)` — third parameter added. | Inspect `window_headless.h`. |
| AC-028 | `WindowSDL3()` constructor accepts `(SDL_Window*, int, int, Platform&)` — fourth parameter added. | Inspect `window_sdl3.h`. |
| AC-029 | `RenderDeviceOpenGL()` constructor accepts `(Window&, SDL_Window*, SDL_GLContext)` — first parameter added. | Inspect `render_device_opengl.h`. |
| AC-030 | Test files using `RenderDeviceHeadless(800, 600)` are updated to use `EngineService::create(Backend::Headless, config)` instead. | Inspect `tests/render_device_tests.cpp`, `tests/scene_rendering_tests.cpp`, `tests/model_tests.cpp`. |
| AC-031 | Forward declaration for `Platform` is added to `window.h`. | Inspect `window.h`. |
| AC-032 | Free camera demo calls `window.set_mouse_capture(true)` when right mouse button transitions from released to held. | Inspect `free_camera_demo.cpp` for call on right-click press. |
| AC-033 | Free camera demo calls `window.set_mouse_capture(false)` when right mouse button transitions from held to released. | Inspect `free_camera_demo.cpp` for call on right-click release. |
| AC-034 | Camera position (WASD) only updates while mouse is captured (right-click held). | Manual — run `buddd demo free-camera`, press WASD without right-click — camera stays still. Press WASD while holding right-click — camera moves. |
| AC-035 | Camera rotation (mouse look) only updates yaw/pitch while mouse is captured (right-click held). | Manual — run `buddd demo free-camera`, move mouse without right-click — camera does not rotate. Move mouse while holding right-click — camera rotates. |
| AC-036 | Free camera demo does not crash when right-click is rapidly pressed and released. | Manual — run `buddd demo free-camera`, rapidly press and release right-click for 5 seconds — no crash, no stuck capture state. |
| AC-037 | `EngineService::create(Backend::Headless, valid_config)` succeeds and returns a non-null `unique_ptr<EngineService>`. | Unit test: create with `{.width=800, .height=600}`, verify result is valid. |
| AC-038 | `EngineService::create(Backend::Headless, invalid_config)` returns an error (not a valid `EngineService`). | Unit test: create with negative dimensions or other invalid config, verify error returned. |
| AC-039 | `engine->platform()` returns a valid `Platform&` reference. | Unit test: call `.platform()` on the created EngineService, verify it returns a reference to a `Platform` (call a method on it). |
| AC-040 | `engine->window()` returns a valid `Window&` reference. | Unit test: call `.window()`, verify `.width()` and `.height()` match the config used at creation. |
| AC-041 | `engine->device()` returns a valid `RenderDevice&` reference. | Unit test: call `.device()`, verify `.size()` returns dimensions matching the window config. |
| AC-042 | `engine->device().window().platform()` returns the same `Platform&` as `engine->platform()`. | Unit test: compare addresses — they must be identical. |
| AC-043 | `engine->device().window().platform().input_system()` compiles and returns a valid `InputSystem&`. | Unit test: call the full chain, ensure it compiles and returns a non-null reference (access a method or property on it). |
| AC-044 | `tests/demo_tests.cpp` no longer runs `buddd demo ...` as subprocesses. | Inspect `tests/demo_tests.cpp` — any `[cli][demo]` test that spawns a subprocess is removed. |

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All four demos produce identical stdout/stderr output compared to pre-refactor baseline. |
| SC-002 | All existing unit tests pass with zero failures. |
| SC-003 | `set_mouse_capture(true)` on an SDL3 window results in `SDL_GetWindowRelativeMouseMode()` returning `true`. |
| SC-004 | Demo functions compile without a `Platform&` parameter. |

## Edge cases

| ID | Edge case | Expected behavior |
|---|---|---|
| EC-001 | `Window` destructor runs before `RenderDevice` destructor | Undefined behavior at the abstract level (existing lifecycle rule). The graph references are non-owning and do not change ownership semantics. |
| EC-002 | `Platform` destructor runs before `Window` destructor | Undefined behavior (existing lifecycle rule). Same as before. |
| EC-003 | `set_mouse_capture(true)` called on a headless window | No-op. `is_mouse_captured()` still returns `false`. |
| EC-004 | `set_mouse_capture(true)` called on an SDL3 window when mouse capture is already enabled | Redundant call — SDL handles gracefully. `is_mouse_captured()` still returns `true`. |
| EC-005 | `set_mouse_capture(false)` called on an SDL3 window when mouse capture is already disabled | Redundant call — SDL handles gracefully. `is_mouse_captured()` still returns `false`. |
| EC-006 | `window()` called on a `RenderDevice` whose `Window` has been destroyed | Undefined behavior (existing lifecycle rule — `Window` must outlive `RenderDevice`). |
| EC-007 | `platform()` called on a `Window` whose `Platform` has been destroyed | Undefined behavior (existing lifecycle rule — `Platform` must outlive `Window`). |
| EC-008 | `RenderDeviceHeadless` constructed from a `WindowHeadless` whose dimensions are zero or negative | `WindowHeadless` already rejects invalid dimensions at construction time (via `WindowConfig` validation in `Platform::create_window`). Direct construction bypasses that validation; stored dimensions reflect what was passed. `size()` returns whatever dimensions the `Window` holds. |
| EC-009 | Multiple `Window` instances from the same `Platform` (future multi-window) | The `Window` base stores a `Platform&` pointing to the creating `Platform`. Each `Window` correctly identifies its creator. Not activated yet — still single-window in practice. |
| EC-010 | Right-click held while mouse capture fails (SDL3 error) | `set_mouse_capture` returns `void` — error is silently ignored. Mouse motion may not be captured; camera will not rotate. No crash. |
| EC-011 | Right-click rapidly pressed and released | Each press/release pair calls `set_mouse_capture(true/false)` sequentially. SDL3 handles redundant calls gracefully. No crash or stuck capture state. |
| EC-012 | Right-click held, then window loses focus | SDL3 automatically releases relative mouse mode when window loses focus. The demo's next poll loop sees no right-click held and does not attempt to re-capture. When focus returns, user must release and re-press right-click to re-capture. |

## Error cases

| ID | Error case | Expected behavior |
|---|---|---|
| ER-001 | SDL3 `SDL_SetWindowRelativeMouseMode` fails | `set_mouse_capture` currently returns `void`. If SDL returns `false`, the error is silently ignored (consistent with existing SDL usage patterns in the codebase). A future improvement could add error reporting. |
| ER-002 | `RenderDevice::create(Window&)` receives a window whose `native_handle()` is unexpectedly `nullptr` for the SDL3 path | Existing behavior unchanged: the null-check dispatches to Headless backend. No change. |
| ER-003 | `RenderDevice::create(Window&)` receives a window whose `native_handle()` is non-null but `SDL_GL_CreateContext` fails | Existing behavior unchanged: returns `Error::Category::RenderDeviceCreationFailed`. No change. |

## Permissions and security

- No authentication, authorization, or access control concerns.
- `set_mouse_capture` controls cursor visibility and relative mouse motion. On the SDL3 backend, it calls standard SDL3 API that operates within the application window — no system-level privilege required.
- No new data is persisted or transmitted.
- The object graph uses non-owning references (`Platform&`, `Window&`). No ownership transfer occurs. The existing lifecycle rules (`Platform` outlives `Window` outlives `RenderDevice`) must be maintained to prevent dangling references.

## Observability

| ID | Observation point | Detail |
|---|---|---|
| OBS-001 | `Window::set_mouse_capture` call | A debug log line could be added (future improvement), but is not required for the initial refactoring. |
| OBS-002 | `Window::is_mouse_captured` call | Trivial getter — no observable side effect. |
| OBS-003 | Object graph wiring errors | Any wiring mistake (e.g., missing `*this` in `create_window`) will result in a compile error or a null-reference crash at runtime (since `Platform&` is a reference, not a pointer, it cannot be null). |

## Out of scope

- Changes to `InputSystem` interface or ownership.
- Changes to `Platform` interface (besides passing `*this` to `Window` constructors).
- Changes to `RenderSystem`, `MeshRenderer`, `Model`, or any scene-graph render types.
- Changes to `demo_helpers.h/.cpp` (they already accept only `RenderDevice&`).
- Changes to capture command or capture scenarios.
- Multi-window support (the graph supports it structurally, but only one active window remains).
- Adding new tests beyond verifying the new accessors — the existing test suite is sufficient to validate no regressions.
- Error logging for `SDL_SetWindowRelativeMouseMode` failures.

## Assumptions

1. **Lifecycle invariants hold**: `Platform` outlives `Window` outlives `RenderDevice`. This is an existing invariant documented in the wiki and enforced by convention and construction order. The refactoring adds non-owning references that assume this invariant.
2. **`SDL_SetWindowRelativeMouseMode` availability**: In SDL3, `SDL_SetWindowRelativeMouseMode` is the standard API for mouse capture (relative mouse mode). It is part of the SDL3 API surface already linked.
3. **`EngineService` ownership of the graph**: A new class `EngineService` (in `src/engine/engine_service.h`) owns the entire `Platform` → `Window` → `RenderDevice` chain via `unique_ptr`. It is the single entry point for engine lifecycle in both tests and production. Tests use `EngineService::create(Backend::Headless, config)` instead of manual construction chains. (Confirmed by human 2026-05-31.)
4. **Demo header forward declarations**: Demo headers currently forward-declare `class Platform` and `class RenderDevice`. After removing the `Platform&` parameter, the `Platform` forward declaration can be removed from each demo header (unless it is still needed for internal includes — likely not, since `Platform` is no longer in the function signature).
5. **`demo_command.cpp` `platform` variable kept as-is**: The `auto platform = ...` variable remains for `Platform` lifetime management. Only the dispatch calls change to remove `**platform` from the argument list. No stylistic cleanup. (Confirmed by human 2026-05-31.)
6. **`Window` constructor visibility**: `Window` base class currently has a `protected` default constructor. A new protected constructor `Window(Platform& platform)` will be added. Concrete subclasses call it from their initialization lists.
7. **`WindowSDL3` stores `Platform&` through the base class**: The base class `Window` stores `Platform& platform_` as a protected member, and the subclasses pass the `Platform&` up to the base constructor.
8. **Free camera demo right-click detection**: The demo already polls mouse button state via `InputSystem`. Right-click press/release is detected by comparing current and previous frame button states (edge detection: `is_down && !was_down` for press, `!is_down && was_down` for release). Mouse capture is toggled on press (`true`) and release (`false`).

## Open questions

All seven open questions have been resolved. See below for the final decisions.

### Q1 (resolved) — `demo_command.cpp` `platform` variable disposal

**Resolution**: Keep as-is. The `platform` variable stays for `Platform` lifetime management, but is no longer forwarded to demo functions. No stylistic cleanup needed. (Confirmed by human 2026-05-31.)

### Q2 (resolved) — Model tests helper function / test helper strategy

**Resolution**: Replace the proposed `make_headless_device(w, h)` helper with a proper `EngineService` class in `src/engine/engine_service.h`. `EngineService` owns the entire `Platform` → `Window` → `RenderDevice` chain and is created via `EngineService::create(Backend, WindowConfig)`. Tests create an `EngineService` instead of using a free-standing helper, avoiding any dangling-reference lifetime bug. (Confirmed by human 2026-05-31.)

### Q3 (resolved) — Forward declaration of `Platform` in `window.h`

**Resolution**: Accepted. `window.h` adds a forward declaration `class Platform;` in the `buddd::engine` namespace. This is a minimal dependency (no header inclusion, just a declaration). `Window` is conceptually a child of `Platform`, making this coupling appropriate.

### Q4 (resolved) — `WindowHeadless` constructor signature change

**Resolution**: `WindowHeadless(int width, int height)` becomes `WindowHeadless(int width, int height, Platform& platform)`. The `Platform&` is passed to the `Window` base class. Only `PlatformHeadless::create_window` constructs `WindowHeadless`, and it is updated as part of this refactoring.

### Q5 (resolved) — `RenderDeviceOpenGL` constructor change

**Resolution**: `RenderDeviceOpenGL(SDL_Window*, SDL_GLContext)` becomes `RenderDeviceOpenGL(Window&, SDL_Window*, SDL_GLContext)`. Both `Window&` and `SDL_Window*` are stored. The `Window&` is used for the `window()` accessor; `SDL_Window*` continues to be used for internal SDL calls.

### Q6 (resolved) — WindowSDL3 stored state for `is_mouse_captured`

**Resolution**: Cache `bool captured_{false}` in `WindowSDL3`, updated on every `set_mouse_capture` call. This avoids an SDL call per query and is consistent with the headless no-op pattern. (Confirmed by human 2026-05-31.)

### Q7 (resolved) — Spec number

**Resolution**: SPEC-016 (not SPEC-014). Updated throughout the document. (Confirmed by human 2026-05-31.)

## Key entities

```
EngineService  ──>  Platform  ──>  Window  ──>  RenderDevice
                      │             │
                      │       set_mouse_capture(bool)
                      │       is_mouse_captured() -> bool
                      │
                 InputSystem
```

| Entity | File | Role |
|---|---|---|
| `EngineService` | `src/engine/engine_service.h/.cpp` | Owns the entire Platform→Window→RenderDevice chain. Created via `EngineService::create(Backend, WindowConfig)`. Provides `platform()`, `window()`, `device()` accessors. |
| `Window` | `src/engine/window/window.h` | Abstract base. Stores `Platform& platform_` (protected). Declares `platform()`, `set_mouse_capture(bool)`, `is_mouse_captured()`. |
| `WindowSDL3` | `src/engine/window/window_sdl3.h/.cpp` | SDL3 backend. Implements mouse capture via `SDL_SetWindowRelativeMouseMode` / `SDL_GetWindowRelativeMouseMode`. |
| `WindowHeadless` | `src/engine/window/window_headless.h/.cpp` | Headless backend. `set_mouse_capture` is no-op, `is_mouse_captured()` returns `false`. |
| `RenderDevice` | `src/engine/render/render_device.h` | Abstract base. New pure virtual `window() -> Window&`. |
| `RenderDeviceOpenGL` | `src/engine/render/render_device_opengl.h/.cpp` | OpenGL backend. Stores `Window& window_`. Implements `window()`. |
| `RenderDeviceHeadless` | `src/engine/render/render_device_headless.h/.cpp` | Headless backend. Stores `Window& window_` (replaces `int width_, height_`). `size()` delegates to `window_`. |
| `Platform` | `src/engine/platform/platform.h` | Abstract base. No interface changes. `create_window()` now passes `*this` to Window constructors. |
| `InputSystem` | `src/engine/input/input_system.h` | No changes. Accessed via `device.window().platform().input_system()`. |

## Detailed design

### `src/engine/window/window.h`

- Add forward declaration `class Platform;` in the `buddd::engine` namespace.
- Add a new protected constructor: `explicit Window(Platform& platform)`.
- Add protected member: `Platform& platform_`.
- Add: `auto platform() noexcept -> Platform& { return platform_; }`
- Add pure virtual: `virtual auto set_mouse_capture(bool captured) -> void = 0;`
- Add pure virtual: `virtual auto is_mouse_captured() const noexcept -> bool = 0;`

### `src/engine/window/window_sdl3.h`

- Change constructor: `WindowSDL3(SDL_Window* window, int width, int height, Platform& platform)`.
- Forward `platform` to `Window(platform)` base constructor.
- Override: `set_mouse_capture(bool captured) -> void` — calls `SDL_SetWindowRelativeMouseMode(window_, captured)` and stores `captured_`.
- Override: `is_mouse_captured() const noexcept -> bool` — returns stored `captured_`.
- Add private member: `bool captured_{false}`.

### `src/engine/window/window_headless.h`

- Change constructor: `WindowHeadless(int width, int height, Platform& platform)`.
- Forward `platform` to `Window(platform)` base constructor.
- Override: `set_mouse_capture(bool) -> void` — no-op.
- Override: `is_mouse_captured() const noexcept -> bool` — returns `false`.

### `src/engine/render/render_device.h`

- Add pure virtual: `virtual auto window() noexcept -> Window& = 0;`

### `src/engine/render/render_device_opengl.h`

- Change constructor: `RenderDeviceOpenGL(Window& window, SDL_Window* sdl_window, SDL_GLContext context)`.
- Store `Window& window_` as a private member.
- Implement `window() -> Window&` returning `window_`.
- Keep `SDL_Window* window_` for internal SDL calls (rename to `sdl_window_` to avoid ambiguity, or leave as `window_`).

### `src/engine/render/render_device_headless.h`

- Change constructor: `RenderDeviceHeadless(Window& window)` — replaces `(int width, int height)`.
- Store `Window& window_` (replaces `int width_, height_`).
- Implement `window() -> Window&` returning `window_`.
- `size()` returns `{window_.width(), window_.height()}`.
- Remove `int width_` and `int height_` private members.

### `src/engine/render/render_device.cpp` (factory)

- Update: `new RenderDeviceHeadless(window)` — pass the `Window&` directly instead of `(window.width(), window.height())`.
- Update: `new RenderDeviceOpenGL(window, sdl_window, gl_context)` — pass the `Window&` as the first argument.

### `src/engine/platform/platform_sdl3.cpp`

- Update `create_window()`: pass `*this` to `WindowSDL3`:
  ```cpp
  return std::unique_ptr<Window>(new WindowSDL3(sdl_window, config.width, config.height, *this));
  ```

### `src/engine/platform/platform_headless.cpp`

- Update `create_window()`: pass `*this` to `WindowHeadless`:
  ```cpp
  return std::unique_ptr<Window>(new WindowHeadless(config.width, config.height, *this));
  ```

### `src/cmd/demo/*.h` (all four demo headers)

- Remove `Platform&` parameter from function signature.
- Remove forward declaration for `class Platform` (no longer needed).
- Update doc comments to reflect new access pattern.

### `src/cmd/demo/*.cpp` (all four demo implementations)

- Replace `platform.poll_events()` with `device.window().platform().poll_events()`.
- Replace `platform.input_system()` with `device.window().platform().input_system()`.
- Replace `platform.delta_time()` with `device.window().platform().delta_time()`.
- Remove `#include "platform/platform.h"` if no longer needed (check if `Platform` type is used elsewhere in the file).
- Remove `Platform& platform` from function signature and local uses.

### `src/cmd/commands/demo_command.cpp`

- Update dispatch calls: remove `**platform, ` from each call.
  - Before: `return buddd::cmd::demo::run_triangle_demo(**platform, **device, argc - 2, argv + 2);`
  - After: `return buddd::cmd::demo::run_triangle_demo(**device, argc - 2, argv + 2);`

### `src/cmd/demo/free_camera_demo.cpp` (mouse capture additions)

- Add right-click edge detection in the frame loop: track `bool prev_right_click_` and `bool curr_right_click_`.
- On right-click press transition (`!prev && curr`):
  - Call `device.window().set_mouse_capture(true)` to enable relative mouse mode and hide cursor.
- On right-click release transition (`prev && !curr`):
  - Call `device.window().set_mouse_capture(false)` to disable relative mouse mode and show cursor.
- Guard camera WASD movement behind `device.window().is_mouse_captured()` check — skip position deltas when not captured.
- Guard camera mouse-look rotation (yaw/pitch adjustment from mouse delta) behind `device.window().is_mouse_captured()` check — skip rotation deltas when not captured.
- Poll mouse motion deltas from `InputSystem` in every frame as before, but only apply them to yaw/pitch when mouse is captured.

### `src/engine/engine_service.h/.cpp` (new)

A new class `EngineService` that owns the entire `Platform` → `Window` → `RenderDevice` chain:

```cpp
class EngineService {
public:
    // Factory — creates Platform, then Window, then RenderDevice.
    // Returns an error (Error::Category::...) if any step fails.
    static auto create(Backend backend, const WindowConfig& config)
        -> tl::expected<std::unique_ptr<EngineService>, Error>;

    // Accessors — all references are valid for the lifetime of the EngineService.
    auto platform() noexcept -> Platform&;
    auto window() noexcept -> Window&;
    auto device() noexcept -> RenderDevice&;

private:
    EngineService(std::unique_ptr<Platform> platform,
                  std::unique_ptr<Window> window,
                  std::unique_ptr<RenderDevice> device);

    std::unique_ptr<Platform> platform_;
    std::unique_ptr<Window> window_;
    std::unique_ptr<RenderDevice> device_;
};
```

Construction order (in `EngineService::create`):
1. Create `Platform` via `Platform::create(backend)`.
2. Create `Window` via `platform->create_window(config)` (passes `*this` as `Platform&`).
3. Create `RenderDevice` via `RenderDevice::create(*window)`.

Destruction order (automatic from member declaration order): `RenderDevice` destroyed first, then `Window`, then `Platform`. This matches the required lifecycle invariants.

`EngineService` is usable both in tests and in `demo_command.cpp` (but demo_command changes are optional — the primary use case is tests).

### Test files

#### `tests/render_device_tests.cpp`
- Replace `buddd::engine::RenderDeviceHeadless device(800, 600);` with `auto engine = EngineService::create(Backend::Headless, {.width=800, .height=600});` and use `engine->device()`.

#### `tests/scene_rendering_tests.cpp`
- Replace all `RenderDeviceHeadless device(800, 600);` constructions with `EngineService::create(Backend::Headless, ...)` and use `engine->device()` and `engine->platform()` where needed.

#### `tests/model_tests.cpp`
- Replace `create_headless_device()` helper (or remove it) with `EngineService::create(Backend::Headless, config)` usage.

#### `tests/demo_tests.cpp`
- Remove the `[cli][demo]` subprocess tests that run `buddd demo <name>`. These tests pop up windows and are unsuitable for automated unit testing. The file may be removed entirely or retained with only non-subprocess tests. No replacement is needed — demo correctness is verified via compilation of demo functions and the EngineService creation tests.

#### `tests/platform_abstraction_tests.cpp`
- These tests use `RenderDevice::create(*window.value())` which already passes `Window&` — no change needed unless constructors change signature in a way that affects `create()`.

## Constraints

1. **No functional behavior change for triangle, cube, cube-scene demos**: These three demos remain pure refactoring with identical outputs, frame counts, window titles, and interactive behavior.
2. **Free camera demo gains mouse-capture behavior**: The free camera demo is intentionally modified with the right-click mouse capture feature (G-05). Its previous behavior is superseded.
3. **All existing tests must pass**: The full test suite (`ctest --preset debug`) must pass with zero failures.
4. **All three unchanged demos must work**: `triangle`, `cube`, `cube-scene` must compile and run identically.
5. **No new #includes of SDL3/OpenGL headers outside `src/engine/`**: The architecture boundary (CONST-001) must be preserved.
6. **Backend selection unchanged**: The compile-time `BUDDD_HAS_DISPLAY` flag and runtime `Backend::SDL3`/`Backend::Headless` selection are unchanged.
7. **Lifecycle ownership unchanged**: `RenderDevice` does not own `Window`, `Window` does not own `Platform`. All references are non-owning.
