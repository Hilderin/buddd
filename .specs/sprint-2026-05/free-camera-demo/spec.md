# SPEC-015 — Free Camera Interactive Demo

## Problem

The Buddd Engine has demos that render a cube (SPEC-009, `buddd demo cube`) and a scene-graph variant (SPEC-011, `buddd demo cube-scene`), but both use a **fixed camera** — the user cannot move or look around. The engine also has all the ingredients for an interactive fly-through camera (InputSystem, CameraComponent, RenderSystem, World), but no demo wires them together.

Without this feature:

- There is no way to validate that camera position and orientation updates at runtime produce correct rendered output.
- There is no interactive demo to exercise the full real-time input→camera→render pipeline.
- Developers cannot manually inspect the scene from arbitrary angles during development.
- The InputSystem has no end-to-end integration test in an interactive context.

## Goals

- **Interactive free-camera demo**: Provide `buddd demo free-camera` that opens an 800×600 window, renders a cube in the scene, and lets the user fly around using WASD + mouse look + Space/Control for vertical movement.
- **Framerate-independent movement**: All camera motion uses delta-time scaling so movement feels consistent regardless of frame rate.
- **ECS-based rendering**: The demo uses `World` + `CameraComponent` + `MeshRenderer` + `RenderSystem` — no manual MVP computation or draw calls.
- **Input-driven camera**: The `InputSystem` (via `Platform::input_system()`) drives camera yaw/pitch/position every frame.
- **Demo pattern compliance**: New `.h`/`.cpp` pair in `src/cmd/demo/`, dispatch added to `demo_command.cpp`.
- **Architecture boundary**: No GLM, SDL3, or OpenGL headers outside `src/engine/` (CONST-001).
- **Constant speed**: One `constexpr` speed value, no configuration mechanism.
- **Exit on Escape**: Pressing Escape exits the demo by breaking the render loop (explicit `input.is_down(KeyCode::Escape)` check).

## Non-goals

- No configurable movement speed or sensitivity — single `constexpr` value for each.
- No mouse-look toggle — mouse look is always active (no button hold required).
- No gamepad, controller, or touch input.
- No collision detection or physics — the camera clips through geometry freely.
- No smoothing, acceleration, or damping on camera movement.
- No UI or HUD overlay.
- No multiple objects in the scene — a single cube is sufficient.
- No networking, multiplayer, or shared camera state.
- No camera-relative vertical movement — Space/Control move along world Y axis only.
- No headless or CI-friendly mode for this demo — it requires a display (it is interactive by nature).
- No roll control — the camera has no roll; only yaw and pitch are user-controlled.
- No frame-count limit — the demo runs until the user presses Escape or closes the window.
- No support for multiple cubes or dynamic object spawning.


## Actors

| Actor | Description |
|---|---|
| End user | Runs `buddd demo free-camera` and flies around a 3D scene using keyboard + mouse. |
| Engine developer | Reuses the free-camera demo as a reference pattern for building interactive features. |
| Code reviewer | Verifies that the demo follows existing patterns, uses ECS correctly, and maintains CONST-001. |

## User-visible behavior

### Starting the demo

When the user runs `buddd demo free-camera`:

1. A window opens with title `"Buddd Engine — Demo: free-camera"` and dimensions 800×600.
2. A solid coloured cube (unit cube, centred at origin, same cube as existing `setup_cube()`) is rendered in the centre of the scene.
3. The camera starts at position `(0.0f, 2.0f, 5.0f)` with identity orientation (looking along the −Z axis toward the origin).
4. The demo runs an interactive loop until the user presses Escape or closes the window.
5. On exit, the process returns `EXIT_SUCCESS`.

### Controls

| Input | Action |
|---|---|
| `W` | Move the camera forward in the horizontal plane (along the camera's forward direction projected onto XZ). |
| `S` | Move the camera backward in the horizontal plane. |
| `A` | Move the camera left (strafe) in the horizontal plane. |
| `D` | Move the camera right (strafe) in the horizontal plane. |
| `Space` | Move the camera **up** along the world +Y axis. |
| `Left Control` | Move the camera **down** along the world −Y axis. |
| **Mouse move** | Control camera look direction: horizontal mouse delta → yaw (left/right), vertical mouse delta → pitch (up/down). Always active. |
| `Escape` | Exit the demo (check `input.is_down(KeyCode::Escape)` and break the loop). |

### Camera movement mechanics

**Yaw and pitch (mouse look):**

- Each frame, the accumulated mouse delta is read from `InputSystem::mouse_delta()`.
- `delta.x` (horizontal) adjusts yaw: `yaw += delta.x * k_mouse_sensitivity`.
- `delta.y` (vertical) adjusts pitch: `pitch += -delta.y * k_mouse_sensitivity` (negated for natural feel — moving mouse forward tilts camera down).
- Mouse look is **not** multiplied by delta time. The mouse delta is already proportional to the frame duration (it accumulates from raw mouse events between `begin_frame()` calls), so the perceived rotation speed per unit of physical mouse movement is naturally framerate-independent without additional scaling.
- Yaw is **unbounded** — the user can spin freely.
- Pitch is **clamped** to the interval `[-89°, +89°]` in radians (approximately `[-1.5533, +1.5533]`) to prevent gimbal lock.
- The camera orientation is rebuilt each frame from the accumulated yaw and pitch: `Quat::from_euler(pitch, yaw, 0.0f)`.
- The updated orientation is applied to the `CameraComponent`'s camera via `camera.set_orientation(...)`.

**Positional movement (keyboard):**

- Each frame, a movement vector is computed from the pressed keys.
- Movement is scaled by `delta_time * k_move_speed`.
- **Forward/backward (W/S):** The camera's forward direction (orientation rotates `Vec3{0.0f, 0.0f, -1.0f}`) is projected onto the XZ plane (`forward.y = 0`), normalized, and used as the movement direction.
- **Left/right (A/D):** The camera's right direction (orientation rotates `Vec3{1.0f, 0.0f, 0.0f}`) is used directly. No projection is needed because the right vector is already perpendicular to the forward Z direction.
- **Up/down (Space/Left Control):** Movement is along the world +Y axis (Space) or −Y axis (Control), regardless of camera orientation.
- All directional components are added into a single `Vec3` movement vector, scaled by `delta_time * k_move_speed`, and added to the camera's position via `camera.set_position(camera.position() + movement)`.

**Framerate independence:**

- `delta_time` is obtained from `platform.delta_time()` each frame — the engine Platform computes the time elapsed since the last `poll_events()` call, in seconds.
- **Keyboard movement** is scaled by `delta_time` so that the camera moves at `k_move_speed` units per second regardless of frame rate.
- **Mouse look** is **not** scaled by `delta_time`. The accumulated mouse delta from `InputSystem::mouse_delta()` is already proportional to the frame duration (it accumulates relative mouse motion events between `begin_frame()` calls). At higher frame rates, each frame accumulates less mouse delta, so the rotation per second of physical mouse movement is naturally framerate-independent without additional delta-time scaling.

**Constants:**

| Constant | Value | Description |
|---|---|---|
| `k_move_speed` | `5.0f` units/second | Forward/backward/strafe/vertical movement speed. |
| `k_mouse_sensitivity` | `0.002f` radians/pixel | Mouse-look sensitivity factor. |

These constants are `constexpr` values defined in the demo source file, not configurable at runtime.

### Demo loop pseudocode

```
setup:
  world ← World()
  camera_entity ← Entity::create(world)
  camera_entity.add_component<CameraComponent>(Camera{})
  cube_entity ← Entity::create(world)
  cube_entity.add_component<MeshRenderer>(shared_ptr{setup_cube(device).model})
  render_system ← RenderSystem(device, world)
  
  yaw ← 0.0f
  pitch ← 0.0f
  cam ← camera_entity.component<CameraComponent>().camera()
  cam.set_position(0.0f, 2.0f, 5.0f)
  cam.set_orientation(Quat::from_euler(0, 0, 0))
  cam.set_perspective(60.0f, 800.0f/600.0f, 0.1f, 100.0f)

loop:
  if not platform.poll_events() → break (window closed)
  if input.is_down(KeyCode::Escape) → break (Escape pressed)
  dt ← platform.delta_time()
  
  input ← platform.input_system()
  
  // Mouse look (NOT scaled by dt — delta is naturally per-frame)
  mouse_delta ← input.mouse_delta()
  yaw += mouse_delta.x * k_mouse_sensitivity
  pitch += -mouse_delta.y * k_mouse_sensitivity
  pitch ← clamp(pitch, -89° in rad, +89° in rad)
  cam.set_orientation(Quat::from_euler(pitch, yaw, 0.0f))
  
  // Keyboard movement
  forward ← cam.orientation() * Vec3(0, 0, -1); forward.y ← 0; forward.normalize()
  right ← cam.orientation() * Vec3(1, 0, 0)
  movement ← Vec3(0, 0, 0)
  if input.is_down(KeyCode::W) → movement += forward
  if input.is_down(KeyCode::S) → movement -= forward
  if input.is_down(KeyCode::D) → movement += right
  if input.is_down(KeyCode::A) → movement -= right
  if input.is_down(KeyCode::Space) → movement += Vec3(0, 1, 0)
  if input.is_down(KeyCode::ControlLeft) → movement -= Vec3(0, 1, 0)
  // Also check ControlRight as a fallback
  if input.is_down(KeyCode::ControlRight) → movement -= Vec3(0, 1, 0)
  
  cam.set_position(cam.position() + movement * k_move_speed * dt)
  
  // Render
  render_system.render()
  
  // Optional: frame-rate limiting (sleep remainder of ~16ms)
```

### Demo registration

In `demo_command.cpp`:

1. Add `#include "demo/free_camera_demo.h"`.
2. Add `"  free-camera  Run the free camera demo (interactive, WASD + mouse look)\n"` to the `k_demo_usage` constant.
3. Add `demo_name != "free-camera"` to the validation condition (line 55) alongside the existing demo names.
4. Add an `else if` branch before the final `else` (which handles `"cube"`) to dispatch to `run_free_camera_demo` when `demo_name == "free-camera"`.

The existing demos (`triangle`, `cube`, `cube-scene`) remain fully functional and unchanged.

### Output

| Event | Output | Target |
|---|---|---|
| Demo started | `"Demo started: free-camera (interactive)\n"` | `std::cerr` |
| Demo ended (normal, Escape pressed) | `"Demo complete: free-camera (interactive)\n"` | `std::cerr` |
| Demo ended (window closed) | `"Demo aborted by user\n"` | `std::cerr` |

## Key entities

### `free_camera_demo.h` / `free_camera_demo.cpp`

New files under `src/cmd/demo/`, namespace `buddd::cmd::demo`.

Entry point:

```cpp
namespace buddd::cmd::demo {

/// Runs the free camera demo: interactive fly-through camera using WASD +
/// mouse look + Space/Control vertical movement. Press Escape to exit.
///
/// @param platform  The engine platform (for event polling and input).
/// @param device    The render device (for rendering).
/// @param argc      Argument count (argv[0] is the demo name).
/// @param argv      Argument vector (argv[0] is the demo name).
/// @return 0 on success, non-zero on error.
[[nodiscard]] auto run_free_camera_demo(buddd::engine::Platform& platform,
                                        buddd::engine::RenderDevice& device,
                                        int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo
```

The header exposes no engine-internal or backend types. The demo entry point receives the already-created `Platform&` and `RenderDevice&` (consistent with all other demos).

## User stories

### Story 1 — Fly through the scene with WASD + mouse (Priority: P1)

As an end user, I want to run the free-camera demo and fly around a 3D scene using first-person-style controls, so that I can inspect the scene from any angle.

**Given** a built `buddd` executable on a system with a display

**When** I run `./buddd demo free-camera`

**Then** an 800×600 window opens with title `"Buddd Engine — Demo: free-camera"`, a coloured cube is visible at the centre of the scene, and I can move the camera with WASD (forward/backward/left/right), look around with the mouse, and fly up/down with Space/Left Control. The scene re-renders every frame from the updated camera position and orientation.

### Story 2 — Mouse look is always active (Priority: P1)

As an end user, I want the camera direction to follow my mouse movements at all times without holding any button, so that the controls feel responsive and natural.

**Given** the free-camera demo is running

**When** I move the mouse

**Then** the camera yaw and pitch update immediately based on mouse delta, and the rendered scene reflects the new view direction.

### Story 3 — Escape exits the demo (Priority: P1)

As an end user, I want to press Escape to exit the demo cleanly, so that I can return to the terminal without using the window close button.

**Given** the free-camera demo is running

**When** I press the Escape key

**Then** the demo loop detects the Escape key via `input.is_down(KeyCode::Escape)`, breaks the render loop, the window closes, and the process exits with code 0.

### Story 4 — Movement is framerate-independent (Priority: P2)

As a developer, I want the camera movement speed and mouse sensitivity to feel consistent regardless of the frame rate, so that the demo behaves predictably on different hardware.

**Given** the free-camera demo running at any frame rate

**When** I hold the W key for 1 second of wall-clock time

**Then** the camera moves forward by approximately `k_move_speed` units (within floating-point precision and frame-timing variance).

### Story 5 — Pitch is clamped to prevent gimbal lock (Priority: P2)

As a developer, I want the camera pitch to be clamped to ±89°, so that the camera cannot flip upside down and the view remains stable.

**Given** the free-camera demo is running

**When** I move the mouse vertically for an extended period

**Then** the camera pitch never exceeds approximately +89° (looking straight up) or −89° (looking straight down), even with continued mouse input in the same direction.

### Story 6 — Demo is registered in `buddd demo` usage (Priority: P1)

As an end user, I want the free-camera demo to appear in the list of available demos, so that I can discover it.

**Given** a built `buddd` executable

**When** I run `./buddd demo` (without a demo name)

**Then** the usage text includes `"  free-camera  Run the free camera demo (interactive, WASD + mouse look)"`.

**When** I run `./buddd demo free-camera`

**Then** the free-camera demo starts.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | Files `src/cmd/demo/free_camera_demo.h` and `src/cmd/demo/free_camera_demo.cpp` exist in namespace `buddd::cmd::demo`. | File compilation succeeds; `buddd::cmd::demo::run_free_camera_demo` is a valid function symbol. |
| AC-002 | `run_free_camera_demo` has signature `auto run_free_camera_demo(buddd::engine::Platform&, buddd::engine::RenderDevice&, int, const char* const*) -> int`. | File compiles; signature matches the existing demo pattern. |
| AC-003 | The demo creates a `World`, at least one entity with a `CameraComponent`, and at least one entity with a `MeshRenderer` (the cube). | Code review confirms the demo uses ECS: `World`, `CameraComponent`, `MeshRenderer`, `RenderSystem`. |
| AC-004 | The camera starts at position `(0.0f, 2.0f, 5.0f)` with identity orientation (looking along −Z). | Code review confirms initial camera setup. |
| AC-005 | The cube is created via `setup_cube(device)` and attached as a `MeshRenderer` on an entity at the origin (identity transform). | Code review confirms cube setup. |
| AC-006 | Each frame, the demo reads `InputSystem::mouse_delta()` and adjusts yaw by `delta.x * k_mouse_sensitivity` and pitch by `-delta.y * k_mouse_sensitivity`. Mouse look is not scaled by delta time. | Code review confirms mouse look computation. |
| AC-007 | Pitch is clamped to ±89° (approximately ±1.5533 radians). | Code review confirms `clamp` or equivalent check. Unit test: with any delta above the threshold, pitch never exceeds the bound. |
| AC-008 | Camera orientation is rebuilt each frame via `Quat::from_euler(pitch, yaw, 0.0f)` and applied to the camera via `set_orientation()`. | Code review confirms orientation update. |
| AC-009 | Each frame, the demo reads `InputSystem::is_down()` for W, S, A, D, Space, ControlLeft, ControlRight and computes a movement vector. | Code review confirms keyboard query logic. |
| AC-010 | WASD movement uses the camera's forward direction projected onto XZ (normalized) and the camera's right direction. Space/Control move along world Y. | Code review confirms movement direction computation. |
| AC-011 | Movement vector is scaled by `dt * k_move_speed` where `dt` is the frame delta time in seconds, and added to camera position. | Code review confirms delta-time scaling. |
| AC-012 | `k_move_speed` is `5.0f` units/second and `k_mouse_sensitivity` is `0.002f` radians/pixel, both `constexpr`. | Code review confirms constant values. |
| AC-013 | The demo loop runs until `platform.poll_events()` returns `false` (window close) OR `input.is_down(KeyCode::Escape)` is `true`. | Code review confirms loop condition. |
| AC-014 | The demo uses `RenderSystem::render()` for all rendering (no manual `begin_frame()`/`end_frame()`/MVP compute/draw calls). | Code review confirms `RenderSystem` usage. |
| AC-015 | Free camera demo is registered in `demo_command.cpp` usage text. | Running `./buddd demo` shows `free-camera` in the demo list. |
| AC-016 | Free camera demo is dispatched correctly from `demo_command.cpp`. | Running `./buddd demo free-camera` starts the demo. |
| AC-017 | All existing demos (`triangle`, `cube`, `cube-scene`) remain functional after the addition. | Running each existing demo produces the expected output. |
| AC-018 | No SDL3, OpenGL, or GLM headers are included in `src/cmd/demo/free_camera_demo.h` (public header). | Code review confirms no backend headers in the `.h` file. |
| AC-019 | Mouse look reads `mouse_delta()` (not `mouse_position()`). | Code review confirms the correct InputSystem method is called. |
| AC-020 | Both `ControlLeft` and `ControlRight` are accepted for downward movement. | Code review confirms both key codes are queried. |
| AC-021 | The demo does not include or use GLM, SDL3, or OpenGL directly — only engine abstraction headers (Platform, RenderDevice, World, CameraComponent, MeshRenderer, RenderSystem, InputSystem, math types). | `grep -E '#include <(GL/|SDL3/|glm/)' src/cmd/demo/free_camera_demo.h src/cmd/demo/free_camera_demo.cpp` returns no matches (except via engine headers). |
| AC-022 | Frame-rate independence: keyboard movement uses delta-time scaling; the camera travels `k_move_speed` units per second of wall-clock time regardless of frame rate. Mouse look is framerate-independent because the accumulated mouse delta is naturally proportional to frame duration. | Code review confirms `dt` is obtained from `platform.delta_time()` and applied to movement but NOT to mouse look. |
| AC-023 | The camera position is updated by `Vec3` addition each frame. No manual view matrix or MVP computation is done in the demo code. | Code review confirms no manual matrix math outside of the camera/ECS abstractions. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An end user can explore the scene from any angle by combining WASD, mouse look, and Space/Control — the cube is visible and its perceived position changes correctly with camera movement. | Manual visual inspection: the cube appears to move in the expected direction relative to camera motion (parallax is correct). |
| SC-002 | The free-camera demo compiles and links without warnings, and runs on the OpenGL 4.5 backend at a sustained interactivity level (≥30 FPS). | Build with `-Wall -Wextra` produces zero warnings. Demo runs at ≥30 FPS on a typical development machine. |
| SC-003 | All existing demos and tests continue to pass with no regressions. | `cmake --build --preset debug && ctest --preset debug` — all tests pass. |
| SC-004 | The free-camera demo code spans at most 120 lines in the `.cpp` file (excluding blank lines and comments), consistent with the size of the existing `cube_demo.cpp` (87 lines) and `cube_scene_demo.cpp` (91 lines). | `wc -l src/cmd/demo/free_camera_demo.cpp` ≤ 120. |
| SC-005 | Constexpr constants (`k_move_speed`, `k_mouse_sensitivity`) are defined exactly once each, with a comment describing the unit. | Code review. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| Mouse delta is zero (no mouse movement this frame) | No yaw/pitch change. The camera orientation remains as it was. |
| Mouse delta is very large (e.g., during window focus change or alt-tab) | Yaw/pitch jump by the full delta amount. This is acceptable for a demo; no smoothing is applied. |
| All movement keys pressed simultaneously | Movement vectors add together. The camera moves diagonally (combined directions). |
| W + D pressed simultaneously | The camera moves forward-right (diagonal) at full speed in that direction (vectors add, then scaled by speed × dt). |
| Space + Control pressed simultaneously | Net vertical movement cancels out (zero vertical movement). |
| Window is closed via the window manager (X button) | `poll_events()` returns `false`. The demo exits normally with `EXIT_SUCCESS`. |
| Pitch reaches exactly ±90° | Clamped to ±89° — the camera stops rotating at the limit. |
| Yaw accumulates beyond 2π | Yaw is unbounded. `Quat::from_euler(pitch, yaw, 0.0f)` handles any yaw value correctly (wrapping internally via trigonometric functions). |
| `mouse_delta()` is not reset between frames | The InputSystem contract (SPEC-013) resets delta to zero in `begin_frame()`, which is called inside `poll_events()`. This is handled correctly. No special handling needed. |
| `platform.delta_time()` returns a very large value (e.g., debugger breakpoint between frames) | Movement jumps by a large amount. This is acceptable for a demo; no cap is applied. Under normal operation `delta_time` is always > 0. |
| Mouse is moved while holding Escape | The explicit Escape key check takes priority — the demo exits regardless of mouse state. |
| Keyboard key is held when window loses focus | SDL sends key-up events for all held keys on focus loss (per existing InputSystem design). The next frame does not detect the key as down. |

## Error cases

| Case | Expected behaviour |
|---|---|
| `setup_cube(device)` fails | Calls `std::exit(EXIT_FAILURE)` after printing to `stderr` (same as all other demos). |
| `RenderSystem::render()` finds no active camera | RenderSystem prints a warning to `std::cerr` (existing behaviour) and continues. The demo loop continues running. |
| Unknown demo name typed (e.g., `buddd demo free-camera-typo`) | The unknown-demo handler in `demo_command.cpp` prints an error to `stderr` with usage text and returns `EXIT_FAILURE`. |
| Extra arguments passed (`buddd demo free-camera --extra`) | A warning is printed to `stderr` (existing pattern in `demo_command.cpp`) and the demo runs normally (extra args are ignored). |

## Permissions and security

- No elevated privileges required.
- No network access, secrets, or credentials involved.
- The demo operates entirely in user space with no I/O beyond the existing window creation and rendering pipeline.
- No data is read from or written to disk.
- The architecture boundary (CONST-001) is maintained: no code in `src/cmd/demo/free_camera_demo.h` or `.cpp` includes SDL3, OpenGL, or GLM headers. All access to engine functionality goes through the engine's public abstractions (`Platform`, `InputSystem`, `RenderDevice`, `World`, `CameraComponent`, `MeshRenderer`, `RenderSystem`, math types).

## Observability

All observability uses `std::cerr`, consistent with the project pattern.

| Signal | Source |
|---|---|
| Demo start | `std::cerr << "Demo started: free-camera (interactive)\n"` |
| Demo end (normal — Escape pressed) | `std::cerr << "Demo complete: free-camera (interactive)\n"` |
| Demo end (window closed) | `std::cerr << "Demo aborted by user\n"` |

## Out of scope

- Configurable speed, sensitivity, or key bindings — all controls are hardcoded.
- Mouse-look toggle or click-to-capture — mouse look is always active.
- Collision detection, physics, or wall clipping.
- Camera smoothing, acceleration, or damping.
- Multiple demo objects beyond the single cube.
- Editor-style orbit or arcball camera — this is a first-person free-fly camera.
- HUD, crosshair, debug overlay, or on-screen text.
- Frame-count limit or timed auto-exit — the demo runs until the user exits.
- Controller, gamepad, or touch input.
- Camera roll — only yaw and pitch are controlled.
- Per-axis speed configuration — a single `k_move_speed` applies to all axes.
- Headless mode or automated testing of the interactive demo — it requires a display.
- Changes to existing demos (`triangle`, `cube`, `cube-scene`).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `Quat::from_euler(pitch, yaw, roll)` follows the XYZ convention as documented in `quat.h`: pitch around X, yaw around Y, roll around Z, applied in XYZ order. |
| A-02 | Multiplying a `Quat` by a `Vec3` (via `operator*`) performs the rotation `q * v * q^-1`, which rotates the vector by the quaternion. |
| A-03 | `Camera::set_orientation(Quat)` and `Camera::set_position(Vec3)` exist and set the camera's orientation and position respectively. The `view_matrix()` uses these values. |
| A-04 | `Camera::orientation() const` returns the current orientation quaternion. |
| A-05 | `InputSystem::mouse_delta()` returns accumulated mouse motion since the last `begin_frame()` call, and `begin_frame()` is called inside `Platform::poll_events()` before event processing (per SPEC-013). |
| A-06 | `InputSystem::is_down(KeyCode)` returns `true` if the key is currently held (per SPEC-013). Both `ControlLeft` and `ControlRight` are queried for downward movement. |
| A-07 | The `InputSystem` is accessible via `platform.input_system()` which returns a non-const `InputSystem&` (per SPEC-013). |
| A-08 | The camera's forward direction, when orientation is identity, is along −Z (`Vec3{0.0f, 0.0f, -1.0f}`). This is the OpenGL convention that the engine uses. |
| A-09 | The right direction when orientation is identity is along +X (`Vec3{1.0f, 0.0f, 0.0f}`). |
| A-10 | `RenderSystem` is in namespace `buddd::engine` and is constructed with `(RenderDevice&, World&)`. Its `render()` method calls `device.begin_frame()`/`end_frame()` internally. |
| A-11 | `Entity::create(world)` returns an `Entity` handle that can call `add_component<T>(args...)`. |
| A-12 | `MeshRenderer` takes a `std::shared_ptr<Model>` in its constructor. The cube's model is obtained from `setup_cube(device).model`. |
| A-13 | The free-camera demo files are automatically picked up by the CMake build via the existing `GLOB_RECURSE` pattern (no CMakeLists.txt changes needed). |
| A-14 | The demo does not need to call `World::flush_destroyed()` because no entities are destroyed during the loop. |
| A-15 | `Platform::delta_time()` returns a positive float representing seconds since the last `poll_events()` call. Under normal operation, this is always > 0. |
| A-16 | The `#pragma once` header guard convention is used for `free_camera_demo.h`, consistent with the existing codebase. |
| A-17 | The camera's initial position `(0.0f, 2.0f, 5.0f)` places the camera slightly above and in front of the origin, looking at the cube at the origin. The identity orientation (−Z forward) points toward the origin. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | Should `ControlRight` be supported as an alternative downward key in addition to `ControlLeft`? The spec assumes yes, for convenience on different keyboard layouts and for parity with standard game controls. | **Key mapping.** Affects which `KeyCode` values are queried. Resolved: both `ControlLeft` and `ControlRight` are accepted. |
