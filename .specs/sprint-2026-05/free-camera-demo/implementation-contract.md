# IMPL-015 — Free Camera Interactive Demo

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|
| Approved by | |
| Date | |
| Time | |

## Source spec

`.specs/sprint-2026-05/free-camera-demo/spec.md` — SPEC-015: Free Camera Interactive Demo

## Goal

Implement the `buddd demo free-camera` command: an interactive, fly-through camera demo that opens an 800×600 window, renders a coloured cube at the origin using ECS (World + CameraComponent + MeshRenderer + RenderSystem), and lets the user move/look around using WASD + mouse look + Space/Control for vertical movement. Movement is delta-time scaled for framerate independence. Pressing Escape exits the loop. The dispatch is wired into `demo_command.cpp` as a new `else if` branch.

## Non-goals

- No changes to existing demos (`triangle`, `cube`, `cube-scene`) beyond the structural `else` → `else if` refactor in `demo_command.cpp` and the addition of `Platform::delta_time()` to the engine.
- No other engine files beyond the 5 platform files listed in `Files allowed to change`.
- No configurable speed, sensitivity, or key bindings.
- No mouse-look toggle — always active.
- No collision, physics, smoothing, acceleration, damping, or camera roll.
- No UI, HUD, or debug overlay.
- No controller/gamepad/touch input.
- No headless or CI-compatible mode — requires a display.
- No new test files — the demo code is application-level, not engine-level, and is not unit-testable in the headless test suite.
- No changes to CMakeLists.txt — the existing `GLOB_RECURSE` picks up new `*.cpp` files in `src/cmd/demo/`.
- No new dependencies — only engine abstractions and C++ stdlib.

## Relevant constitution rules

| Rule | Relevance |
|---|---|
| **CONST-001** (Architecture Boundaries) | The demo code must not include `<SDL3/`, `<GL/`, `<glad/>`, or `<glm/>` directly. All rendering and input goes through engine abstractions (`Platform`, `InputSystem`, `RenderDevice`, `World`, `CameraComponent`, `MeshRenderer`, `RenderSystem`, `Camera`, `Quat`, `Vec3`). The public header `free_camera_demo.h` must contain only forward declarations for `Platform` and `RenderDevice`. |
| **CONST-002** (Testing Policy) | The free-camera demo is interactive and requires a display. No test file is required for this demo. Acceptance criteria are verified via code review and manual visual inspection. |
| **CONST-003** (Documentation Policy) | The wiki module map (`docs/wiki/architecture/module-map.md`) will need a minor update to list the new demo files. |
| **CONST-004** (Security Policy) | No elevated privileges, network access, or filesystem I/O. No impact. |

## Relevant ADRs

| ADR | Relevance |
|---|---|
| **ADR-004** (Demo System Architecture) | The new demo follows the mandated pattern: `.h`/`.cpp` pair in `src/cmd/demo/`, single free function in `buddd::cmd::demo`, if/else-if dispatch in `DemoCommand::run()`. |
| **ADR-005** (std::optional<T&> for Component Lookup) | `entity.get_component<CameraComponent>()` returns `std::optional<CameraComponent&>`. The demo must obtain a reference to the camera via `->camera()`, not a copy. |
| **ADR-001** (Result&lt;T&gt; / Error Pattern) | The demo uses `setup_cube(device)` which calls `std::exit(EXIT_FAILURE)` on failure (same pattern as existing demos). No new `Result<T>` handling is needed. |

## Files to inspect

The Code Agent MUST read these files before making any edits:

1. `src/cmd/demo/cube_scene_demo.h` — Template for the new header (namespace, forward declarations, `[[nodiscard]]` signature).
2. `src/cmd/demo/cube_scene_demo.cpp` — Template for the new `.cpp` (ECS setup pattern, `setup_cube` usage, `RenderSystem` usage, frame-rate limiting with `steady_clock`).
3. `src/cmd/commands/demo_command.cpp` — Dispatch file that must be modified (include, usage text, validation, `else if` branch).
4. `src/cmd/demo/demo_helpers.h` — `setup_cube(device)` signature and `CubeResources` struct.
5. `src/engine/input/input_system.h` — `InputSystem` API (`mouse_delta()`, `is_down(KeyCode)`, etc.).
6. `src/engine/input/key_code.h` — `KeyCode` enum (W, A, S, D, Space, Escape, ControlLeft, ControlRight values).
7. `src/engine/scene/camera_component.h` — `CameraComponent` API (constructor from `Camera`, `camera()` accessor).
8. `src/engine/math/camera.h` — `Camera` class API (`set_position`, `set_orientation`, `set_perspective`, `position()`, `orientation()`).
9. `src/engine/math/quat.h` — `Quat::from_euler(pitch, yaw, roll)` signature and `Quat * Vec3` operator.
10. `src/engine/math/vec3.h` — `Vec3` API (`.normalize()`, `.normalized()`, `.y`, arithmetic operators).
11. `src/engine/math/math.h` — `radians()` function, `pi`, `epsilon` constants.
12. `src/engine/render/render_system.h` — `RenderSystem` constructor `(RenderDevice&, World&)` and `render()` method.
13. `src/engine/render/mesh_renderer.h` — `MeshRenderer` constructor from `std::shared_ptr<Model>`.
14. `src/engine/scene/entity.h` — `Entity::create(world)`, `add_component<T>(args...)`, `get_component<T>()`.
15. `src/engine/platform/platform.h` — `Platform::input_system() -> InputSystem&`, `poll_events() -> bool`, and the new `delta_time()` that must be added.
16. `src/engine/platform/platform_sdl3.h` — `PlatformSDL3` member declarations and existing `poll_events()` override.
17. `src/engine/platform/platform_sdl3.cpp` — Existing `poll_events()` implementation where delta computation must be injected.
18. `src/engine/platform/platform_headless.h` — `PlatformHeadless` class structure.
19. `src/engine/platform/platform_headless.cpp` — Existing `poll_events()` implementation.

## Files allowed to change

1. `src/cmd/demo/free_camera_demo.h` — **Create**. Public header for the free-camera demo.
2. `src/cmd/demo/free_camera_demo.cpp` — **Create**. Implementation of the free-camera demo.
3. `src/cmd/commands/demo_command.cpp` — **Modify**. Add include, usage entry, validation, and dispatch branch.
4. `src/engine/platform/platform.h` — **Modify**. Add `virtual auto delta_time() const noexcept -> float = 0;` to the `Platform` abstract class.
5. `src/engine/platform/platform_sdl3.h` — **Modify**. Add `float delta_time_{1.0f / 60.0f};` and `uint64_t last_frame_ticks_{0};` member variables, add `auto delta_time() const noexcept -> float override;` declaration, add `#include <cstdint>`.
6. `src/engine/platform/platform_sdl3.cpp` — **Modify**. In `poll_events()`, compute delta using `SDL_GetTicks()`. Implement `delta_time()`.
7. `src/engine/platform/platform_headless.h` — **Modify**. Add `auto delta_time() const noexcept -> float override;` declaration.
8. `src/engine/platform/platform_headless.cpp` — **Modify**. Implement `delta_time()` returning `1.0f / 60.0f`.

## Files forbidden to change

- Any file under `src/engine/` except the 5 platform files listed in `Files allowed to change` (items 4–8).
- `src/cmd/demo/cube_demo.h` / `src/cmd/demo/cube_demo.cpp`
- `src/cmd/demo/cube_scene_demo.h` / `src/cmd/demo/cube_scene_demo.cpp`
- `src/cmd/demo/triangle_demo.h` / `src/cmd/demo/triangle_demo.cpp`
- `src/cmd/demo/demo_helpers.h` / `src/cmd/demo/demo_helpers.cpp`
- `src/cmd/commands/demo_command.h`
- Any file in `tests/`
- Any `CMakeLists.txt`
- Any file under `docs/`
- Any file under `src/editor/`

## Existing conventions to follow

1. **Header guard**: `#pragma once` (consistent with all existing headers).
2. **Namespace**: `buddd::cmd::demo` for the free function.
3. **Forward declarations in header**: Forward-declare `buddd::engine::Platform` and `buddd::engine::RenderDevice` in the `.h` (no includes in `.h` except standard headers — but in practice no includes at all; just forward declarations).
4. **Function signature** (from ADR-004):
   ```cpp
   [[nodiscard]] auto run_free_camera_demo(buddd::engine::Platform& platform,
                                           buddd::engine::RenderDevice& device,
                                           int argc, const char* const* argv) -> int;
   ```
5. **Include path in `.cpp`**: `#include "demo/free_camera_demo.h"` (relative to `src/`).
6. **Namespace alias**: `namespace be = buddd::engine;` at the top of the `.cpp`.
7. **Output to `std::cerr`**: Demo start message, end message, abort message.
8. **Error handling for `setup_cube`**: `setup_cube(device)` already calls `std::exit(EXIT_FAILURE)` on failure — no additional error handling needed.
9. **Include style**: Engine includes use the full path from `src/engine/`, e.g., `#include "platform/platform.h"`, `#include "render/render_system.h"`, `#include "scene/world.h"`.
10. **Frame-rate limiting pattern**: Sleep for the remainder of a ~16ms frame duration using `std::this_thread::sleep_for`, same as `cube_scene_demo.cpp`.
11. **No GLM/SDL3/OpenGL includes**: Verify via grep that no such includes exist in the demo files.
12. **`[[maybe_unused]]` on `argc`/`argv`**: The demo does not use per-demo arguments; annotate parameters with `[[maybe_unused]]`.

## Required implementation behavior

### 1. Header: `src/cmd/demo/free_camera_demo.h`

- Include guard: `#pragma once`.
- Forward-declare `class buddd::engine::Platform` and `class buddd::engine::RenderDevice` in namespace `buddd::engine`.
- Declare `auto run_free_camera_demo(...) -> int` in namespace `buddd::cmd::demo` with the exact signature from "Existing conventions to follow" above.
- Use `[[nodiscard]]`.
- Include Doxygen-style doc comment describing the demo, controls, and exit behavior (matching the spec §Key entities).
- No other includes. No inline implementations.

Exact content (modulo formatting):

```cpp
#pragma once

namespace buddd::engine {
class Platform;
class RenderDevice;
} // namespace buddd::engine

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

### 2. Implementation: `src/cmd/demo/free_camera_demo.cpp`

**Includes** (in this order):
1. `"demo/free_camera_demo.h"`
2. `"demo/demo_helpers.h"`
3. Engine headers needed:
   - `"platform/platform.h"` — for `Platform::input_system()`
   - `"render/render_device.h"` — for `RenderDevice`
   - `"render/render_system.h"` — for `RenderSystem`
   - `"render/mesh_renderer.h"` — for `MeshRenderer`
   - `"scene/world.h"` — for `World`
   - `"scene/camera_component.h"` — for `CameraComponent`
   - `"math/camera.h"` — for `Camera`
   - `"math/math.h"` — for `radians()`
   - `"math/vec3.h"` — for `Vec3`
   - `"math/quat.h"` — for `Quat`
4. Standard headers:
   - `<algorithm>` — for `std::clamp`
   - `<chrono>` — for `std::chrono::steady_clock` (frame-rate limiting, NOT delta-time computation)
   - `<cstdlib>` — for `EXIT_SUCCESS`, `EXIT_FAILURE`
   - `<iostream>` — for `std::cerr`
   - `<memory>` — for `std::make_shared`
   - `<thread>` — for `std::this_thread::sleep_for`

**Namespace alias**: `namespace be = buddd::engine;` at file scope.

**Constants** (constexpr at namespace scope or inside function — use inside function to keep local):

```cpp
constexpr float k_move_speed = 5.0f;          // units/second
constexpr float k_mouse_sensitivity = 0.002f; // radians/pixel
constexpr float k_pitch_clamp = 89.0f;        // degrees — clamped before conversion
```

**Function implementation** — follow this exact algorithm:

1. **Print start message** to `std::cerr`: `"Demo started: free-camera (interactive)\n"`.

2. **Set up ECS world**:
   - Create `be::World world;`
   - Create camera entity: `auto camera_entity = be::Entity::create(world);`
   - Construct a default `be::math::Camera` (default constructor gives identity orientation, (0,0,0) position)
   - Add `CameraComponent`: `camera_entity.add_component<be::CameraComponent>(camera);`
   - **Obtain a REFERENCE to the camera** (MUST NOT use a copy):
     ```cpp
     auto& cam = camera_entity.get_component<be::CameraComponent>()->camera();
     ```
   - Configure camera:
     ```cpp
     cam.set_position(be::math::Vec3{0.0f, 2.0f, 5.0f});
     cam.set_orientation(be::math::Quat::from_euler(0.0f, 0.0f, 0.0f));
     cam.set_perspective(be::math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
     ```

3. **Create cube entity**:
   - `auto cube = setup_cube(device);`
   - `auto cube_entity = be::Entity::create(world);`
   - `cube_entity.add_component<be::MeshRenderer>(std::make_shared<be::Model>(std::move(cube.model)));`

4. **Create RenderSystem**: `be::RenderSystem render_system(device, world);`

5. **Initialize yaw/pitch and timing**:
   ```cpp
   float yaw = 0.0f;
   float pitch = 0.0f;
   constexpr auto frame_duration = std::chrono::milliseconds(16);
   ```

6. **Obtain input system reference BEFORE loop** (fixes the spec-critic pseudocode ordering warning):
   ```cpp
   auto& input = platform.input_system();
   ```

7. **Interactive loop** (while true, with break conditions):
   ```cpp
   while (true) {
       auto frame_start = std::chrono::steady_clock::now();

       // Event polling — returns false when window closed
       if (!platform.poll_events()) {
           std::cerr << "Demo aborted by user\n";
           return EXIT_SUCCESS;
       }

       // Escape key check — explicit, NOT via poll_events()
       if (input.is_down(be::KeyCode::Escape)) {
           break;
       }

        // Delta time computation — via Platform (SDL_GetTicks in poll_events)
        float dt = platform.delta_time();

       // ── Mouse look (NOT scaled by dt) ──
       auto [dx, dy] = input.mouse_delta();
       yaw += dx * k_mouse_sensitivity;
       pitch += -dy * k_mouse_sensitivity;

       // Pitch clamping to ±89°
       pitch = std::clamp(pitch, be::math::radians(-k_pitch_clamp),
                                   be::math::radians(k_pitch_clamp));

       cam.set_orientation(be::math::Quat::from_euler(pitch, yaw, 0.0f));

       // ── Keyboard movement ──
       // Compute forward (XZ-projected, normalized) and right vectors
       be::math::Vec3 forward = cam.orientation() * be::math::Vec3{0.0f, 0.0f, -1.0f};
       forward.y = 0.0f;                        // Project onto XZ plane
       if (forward.length_squared() > be::math::epsilon) {
           forward.normalize();                  // Prevent division by zero at extreme pitch
       }

       be::math::Vec3 right = cam.orientation() * be::math::Vec3{1.0f, 0.0f, 0.0f};

       be::math::Vec3 movement{0.0f, 0.0f, 0.0f};

       if (input.is_down(be::KeyCode::W))      { movement += forward; }
       if (input.is_down(be::KeyCode::S))      { movement -= forward; }
       if (input.is_down(be::KeyCode::D))      { movement += right; }
       if (input.is_down(be::KeyCode::A))      { movement -= right; }
       if (input.is_down(be::KeyCode::Space))  { movement += be::math::Vec3::unit_y(); }
       if (input.is_down(be::KeyCode::ControlLeft))  { movement -= be::math::Vec3::unit_y(); }
       if (input.is_down(be::KeyCode::ControlRight)) { movement -= be::math::Vec3::unit_y(); }

       // Apply delta-time-scaled movement
       cam.set_position(cam.position() + movement * k_move_speed * dt);

       // ── Render ──
       render_system.render();

       // ── Frame-rate limiting (sleep remainder of ~16ms) ──
       auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
       if (frame_elapsed < frame_duration) {
           std::this_thread::sleep_for(frame_duration - frame_elapsed);
       }
   }
   ```

8. **After loop break** (Escape pressed), print end message and return:
   ```cpp
   std::cerr << "Demo complete: free-camera (interactive)\n";
   return EXIT_SUCCESS;
   ```

**Key constraints:**
- `cam` MUST be a reference (`auto&`), not a copy. The spec-critic specifically flagged this.
- Mouse look MUST use `mouse_delta()`, NOT `mouse_position()`.
- Mouse look MUST NOT be multiplied by `dt`.
- `forward.y = 0` MUST be set before normalization, and `normalize()` MUST only be called if `length_squared()` is non-negligible (guard against degenerate vector at extreme pitch).
- Both `ControlLeft` and `ControlRight` MUST be checked.
- `set_perspective()` MUST be called explicitly (default aspect is 16:9, not 4:3).
- The `poll_events()` call MUST come BEFORE the Escape `is_down()` check, because `poll_events()` internally calls `begin_frame()` which resets the mouse delta.

### 3. Registration: `src/cmd/commands/demo_command.cpp`

**Changes required (exact):**

a) **Add include** after line 3 (the last existing demo include):
```cpp
#include "demo/free_camera_demo.h"
```

b) **Add usage text** — insert into `k_demo_usage` after the `cube-scene` line:
```
"  free-camera  Run the free camera demo (interactive, WASD + mouse look)\n"
```

The full `k_demo_usage` after modification should read:
```cpp
inline constexpr std::string_view k_demo_usage =
    "Usage: buddd demo <demo>\n"
    "\n"
    "Available demos:\n"
    "  triangle     Run the triangle demo (120 frames)\n"
    "  cube         Run the cube demo (120 frames, rotating coloured cube)\n"
    "  cube-scene   Run the cube demo via scene graph (World + RenderSystem)\n"
    "  free-camera  Run the free camera demo (interactive, WASD + mouse look)\n"
    "\n"
    "Demo names are case-sensitive.\n";
```

c) **Add to validation condition** (line 55) — add `&& demo_name != "free-camera" inside the existing if:
```cpp
if (demo_name != "triangle" && demo_name != "cube" && demo_name != "cube-scene" && demo_name != "free-camera") {
```

d) **Add dispatch branch** — insert an `else if` BEFORE the final `else` (which dispatches to `run_cube_demo`):
```cpp
} else if (demo_name == "free-camera") {
    return buddd::cmd::demo::run_free_camera_demo(**platform, **device, argc - 2, argv + 2);
} else {
    // demo_name == "cube" (validated above)
    return buddd::cmd::demo::run_cube_demo(**platform, **device, argc - 2, argv + 2);
}
```

**Important**: The existing `else` for `"cube"` must remain as an `else` (not changed to `else if`). The `free-camera` branch is inserted between `cube-scene` and `cube`. The `cube` case must remain the final fallback because the validation above guarantees it matches.

### 4. Engine changes — `Platform::delta_time()`

#### 4a. `src/engine/platform/platform.h`

Add the following pure virtual method after `input_system()` (before the deleted copy constructors):

```cpp
/// Returns the time elapsed since the last poll_events() call, in seconds.
/// Under normal operation, always > 0. Useful for framerate-independent movement.
[[nodiscard]] virtual auto delta_time() const noexcept -> float = 0;
```

The resulting `Platform` class should have methods in this order:
1. `create()` (static)
2. `~Platform()` (virtual destructor)
3. `create_window()`
4. `poll_events()`
5. `input_system()`
6. `delta_time()` ← **new**
7. Deleted copy/move constructors and operators
8. `protected` default constructor

#### 4b. `src/engine/platform/platform_sdl3.h`

Add `#include <cstdint>` at the top of the file (alongside existing includes):

```cpp
#include <cstdint>
```

Add two private member variables after `input_system_`:

```cpp
float delta_time_{1.0f / 60.0f};  // Default: ~16.67ms
uint64_t last_frame_ticks_{0};     // SDL_GetTicks() value from last poll
```

Add the override declaration in the `public` section, after `input_system()`:

```cpp
[[nodiscard]] auto delta_time() const noexcept -> float override;
```

The `private` section should look like:
```cpp
private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformSDL3() = default;

    InputSystemSDL3 input_system_;
    float delta_time_{1.0f / 60.0f};
    uint64_t last_frame_ticks_{0};
};
```

#### 4c. `src/engine/platform/platform_sdl3.cpp`

In `poll_events()`, after `input_system_.begin_frame();` and before the event loop, add:

```cpp
// Compute delta time from SDL_GetTicks
Uint64 now = SDL_GetTicks();
if (last_frame_ticks_ != 0) {
    delta_time_ = static_cast<float>(now - last_frame_ticks_) / 1000.0f;
} else {
    delta_time_ = 1.0f / 60.0f;  // First frame: assume 60 FPS
}
last_frame_ticks_ = now;
```

Add the `delta_time()` implementation at file scope (e.g., after `input_system()`):

```cpp
auto PlatformSDL3::delta_time() const noexcept -> float {
    return delta_time_;
}
```

The resulting `poll_events()` should be:

```cpp
auto PlatformSDL3::poll_events() -> bool {
    // 1. Compute delta time from SDL_GetTicks
    Uint64 now = SDL_GetTicks();
    if (last_frame_ticks_ != 0) {
        delta_time_ = static_cast<float>(now - last_frame_ticks_) / 1000.0f;
    } else {
        delta_time_ = 1.0f / 60.0f;  // First frame: assume 60 FPS
    }
    last_frame_ticks_ = now;

    // 2. Begin the input frame (copies current→previous, resets delta/wheel)
    input_system_.begin_frame();

    // 3. Process all pending SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
        // Route non-quit events to the input system
        input_system_.on_sdl_event(event);
    }
    return true;
}
```

**IMPORTANT**: The `SDL_GetTicks()` call and delta computation MUST happen BEFORE `input_system_.begin_frame()`. This ensures that the delta time value is available and stable for any systems that might want to use it during frame processing. The order is: compute delta → begin frame (resets input state for new frame) → process events.

#### 4d. `src/engine/platform/platform_headless.h`

Add the override declaration in the `public` section, after `input_system()`:

```cpp
[[nodiscard]] auto delta_time() const noexcept -> float override;
```

#### 4e. `src/engine/platform/platform_headless.cpp`

Add the `delta_time()` implementation:

```cpp
auto PlatformHeadless::delta_time() const noexcept -> float {
    return 1.0f / 60.0f;  // Fixed 60 FPS for headless
}
```

## Required tests

No new test files are required. The free-camera demo requires a display and is interactive — it cannot run in the headless test suite per CONST-002 and the spec's explicit non-goal.

**Verification of acceptance criteria** is done via:
- Code review (AC-001 through AC-023, except AC-007 which is reviewed).
- Manual build and run: `cmake --build --preset debug && ./build/debug/buddd demo free-camera`.
- Manual visual inspection: cube is visible, mouse look works, WASD moves correctly, Space/Control move up/down, Escape exits.
- Regression verification: all existing demos still run (`triangle`, `cube`, `cube-scene`).
- Grep verification for CONST-001 compliance.

**AC-007 specific note**: The pitch clamp logic lives in application-level demo code and cannot be unit-tested via the existing engine test suite. Verification is via code review: confirm `std::clamp` (or equivalent) is applied to `pitch` with bounds `radians(-89.0f)` and `radians(89.0f)`.

## Edge cases

All edge cases from SPEC-015 §Edge cases are carried forward:

| Case | Required behavior |
|---|---|
| Mouse delta is zero | No yaw/pitch change. Camera orientation unchanged. |
| Mouse delta is very large (alt-tab, focus change) | Yaw/pitch jump by the full delta. Acceptable — no smoothing. |
| All movement keys pressed simultaneously | Vectors add together. The camera moves diagonally (combined directions). |
| W + D pressed simultaneously | Forward-right diagonal movement at full speed. |
| Space + Control pressed simultaneously | Net vertical movement cancels out (zero vertical). |
| Window closed via X button | `poll_events()` returns `false`. Exit with EXIT_SUCCESS and "Demo aborted by user" message. |
| Pitch reaches exactly ±90° | Clamped to ±89° — no gimbal lock. |
| Yaw accumulates beyond 2π | Yaw is unbounded. `Quat::from_euler` handles any value correctly. |
| `platform.delta_time()` is very large (debugger breakpoint between frames) | Movement jumps by a large amount. This is acceptable for a demo; no cap is applied. Under normal operation `delta_time` is always > 0. |
| Mouse moved while holding Escape | Escape check (is_down) takes priority — demo exits. |
| Keyboard key held when window loses focus | Per InputSystem design, key-up events fire on focus loss. Next frame does not see the key as down. |
| Forward vector at extreme pitch is near-zero | Guard with `length_squared() > epsilon` check before `normalize()`. If too small, skip forward/backward movement for that frame (the vector components are zero or negligible). |

## Security impact

None. No elevated privileges, no network access, no filesystem I/O, no secrets. Architecture boundary (CONST-001) is maintained — no SDL3, OpenGL, or GLM includes in demo code. Verified via:
```
grep -E '#include <(GL/|SDL3/|glm/)' src/cmd/demo/free_camera_demo.h src/cmd/demo/free_camera_demo.cpp
```
Expected output: zero matches.

## Data and migration impact

None. No schema changes, data migrations, seed data, or persistent state.

## API compatibility impact

None. The demo adds a new entry point function and extends `demo_command.cpp` — no existing API is modified. The new function signature follows the established pattern (ADR-004).

## Documentation impact

- `docs/wiki/architecture/module-map.md` (lines 179-187) — add an entry for the new demo files in the "Demo files" table. This is handled by the wiki-agent, not the Code Agent.
- No README or other documentation changes needed.

## ADR impact

None. This implementation follows ADR-004 (demo system architecture) and ADR-005 (component lookup API) without modifying or superseding them.

## Constitution impact

None. CONST-001 is fully respected (no backend headers in demo code). No amendment needed.

## Done criteria

The Code Agent MUST satisfy ALL of the following for the implementation to be considered complete:

- [ ] **DC-001**: `src/cmd/demo/free_camera_demo.h` exists with `#pragma once`, forward declarations of `Platform` and `RenderDevice`, and the `run_free_camera_demo` function declaration in namespace `buddd::cmd::demo` with `[[nodiscard]]`.
- [ ] **DC-002**: `src/cmd/demo/free_camera_demo.cpp` exists with correct includes, `namespace be = buddd::engine;`, `constexpr` constants `k_move_speed` (5.0f) and `k_mouse_sensitivity` (0.002f), and the `run_free_camera_demo` implementation.
- [ ] **DC-003**: The demo creates a `World`, at least one entity with `CameraComponent`, and at least one entity with `MeshRenderer` (using `setup_cube(device)`).
- [ ] **DC-004**: Camera starts at position `(0.0f, 2.0f, 5.0f)` with identity orientation (looking along −Z).
- [ ] **DC-005**: `set_perspective(radians(60.0f), 800.0f/600.0f, 0.1f, 100.0f)` is called on the camera.
- [ ] **DC-006**: The camera reference is obtained via `entity.get_component<CameraComponent>()->camera()` — NOT via a copy. Verified by code review: the variable holding the camera is `auto&` (or `auto const&`), not `auto`.
- [ ] **DC-007**: Mouse look reads `mouse_delta()` (not `mouse_position()`) and is NOT scaled by delta time.
- [ ] **DC-008**: Yaw accumulates unbounded; pitch is clamped to `±89°` (≈ ±1.5533 rad) via `std::clamp` or equivalent.
- [ ] **DC-009**: Camera orientation is rebuilt each frame via `Quat::from_euler(pitch, yaw, 0.0f)`.
- [ ] **DC-010**: WASD movement uses the camera's forward direction projected onto XZ (normalized, with near-zero guard) and the camera's right direction. Space/Control move along world Y.
- [ ] **DC-011**: Both `ControlLeft` and `ControlRight` are accepted for downward movement.
- [ ] **DC-012**: Movement is scaled by `dt * k_move_speed` where `dt` is obtained from `platform.delta_time()`, NOT from manual `std::chrono::steady_clock` computation.
- [ ] **DC-013**: The demo loop exits when `poll_events()` returns `false` (window closed) OR `input.is_down(KeyCode::Escape)` is true.
- [ ] **DC-014**: The demo uses `RenderSystem::render()` for all rendering — no manual `begin_frame()`/`end_frame()`/MVP/draw calls in demo code.
- [ ] **DC-015**: Frame-rate limiting sleeps for remainder of ~16ms, matching the existing demo pattern.
- [ ] **DC-016**: `demo_command.cpp` includes `"demo/free_camera_demo.h"`, adds `free-camera` to usage text, adds `"free-camera"` to the validation condition, and adds `else if (demo_name == "free-camera")` before the final `else` (cube).
- [ ] **DC-017**: No SDL3, OpenGL, or GLM headers are included in `free_camera_demo.h` or `free_camera_demo.cpp` — verified by grep returning zero matches.
- [ ] **DC-018**: The demo compiles and links with zero warnings (`-Wall -Wextra`).
- [ ] **DC-019**: All existing demos (`triangle`, `cube`, `cube-scene`) compile and run after the changes.
- [ ] **DC-020**: The output messages match the spec: `"Demo started: free-camera (interactive)\n"`, `"Demo complete: free-camera (interactive)\n"`, `"Demo aborted by user\n"` — all via `std::cerr`.
- [ ] **DC-021**: The `.cpp` file is ≤150 lines (relaxed from 120 per spec-critic warning SC-004; the 120 target was identified as potentially tight).
- [ ] **DC-022**: `Platform` abstract class in `platform.h` declares `[[nodiscard]] virtual auto delta_time() const noexcept -> float = 0;` after `input_system()` and before the deleted copy constructors.
- [ ] **DC-023**: `PlatformSDL3` in `platform_sdl3.h` has `float delta_time_{1.0f / 60.0f};` and `uint64_t last_frame_ticks_{0};` private members, `#include <cstdint>`, and `[[nodiscard]] auto delta_time() const noexcept -> float override;` declaration.
- [ ] **DC-024**: `PlatformSDL3::poll_events()` in `platform_sdl3.cpp` computes delta via `SDL_GetTicks()` BEFORE `input_system_.begin_frame()`, stores result in `delta_time_`, and `PlatformSDL3::delta_time()` returns `delta_time_`.
- [ ] **DC-025**: `PlatformHeadless` has `[[nodiscard]] auto delta_time() const noexcept -> float override;` declaration in `platform_headless.h` and the implementation in `platform_headless.cpp` returns `1.0f / 60.0f`.
- [ ] **DC-026**: The demo (`free_camera_demo.cpp`) uses `platform.delta_time()` for delta-time computation — no manual `std::chrono::steady_clock` prev/now/dt computation. Verified by grep: no `prev_time`, `steady_clock::now()` used only for frame-rate limiting, not for delta-time.
