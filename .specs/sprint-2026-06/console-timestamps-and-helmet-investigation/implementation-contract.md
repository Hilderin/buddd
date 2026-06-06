# IMPL-023 — Console Timestamps, FreeCameraMovement Refactoring & Helmet Investigation

## Source spec

`.specs/sprint-2026-06/console-timestamps-and-helmet-investigation/spec.md`

## Goal

Implement five deliverables in one pass: (1) prepend `[HH:MM:SS.fff]` timestamps to every console log line; (2) create a reusable ECS `FreeCameraMovement` component; (3) refactor `free_camera_app` and `phong_app` to use the new component instead of duplicated inline camera movement code; (4) create a new `gltf-helmet` demo app with free-camera controls; (5) investigate and fix the DamagedHelmet geometry deformation and slow load (~10s → under 3s).

## Non-goals

- No CLI flag to disable console timestamps.
- FileSink timestamp format stays ISO 8601 (unchanged).
- No colour/ANSI codes in console output.
- No changes to PBR shaders, normal mapping V2, alpha modes, or `KHR_materials_pbrSpecularGlossiness`.
- No async model loading.
- No refactoring of any apps beyond `free_camera_app` and `phong_app`.
- No unit tests for `FreeCameraMovement` (verified via integration in the three apps).
- No new external dependencies.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-020 | ConsoleSink format changes from `[LEVEL] [Tag] message` to `[HH:MM:SS.fff] [LEVEL] [Tag] message`. ADR-020 section 5 must be updated. Override rationale: V1 simplicity deferral — timestamps added for real-time debugging of rendering/performance issues. |
| ADR-011 | Raw pointer `active_camera_` in World is a private implementation detail (non-owning observer). Not modified by this contract but must not be violated. |

## Files to inspect

These files must be read before editing to understand conventions and current state:

- `src/engine/log/console_sink.cpp` — current format string to modify (AC-001)
- `src/engine/log/console_sink.h` — header for ConsoleSink class
- `src/engine/log/file_sink.cpp` — reference for ISO 8601 timestamp pattern (AC-002)
- `src/engine/log/log.h` — LogMessage struct, Sink interface
- `src/engine/scene/component.h` — Component base class (FreeCameraMovement must inherit)
- `src/engine/scene/camera_component.h` / `.cpp` — reference pattern for ECS components
- `src/engine/scene/entity.h` — entity() accessor, get_component<T>() template
- `src/engine/scene/world.h` — World::each<T>() iteration pattern, add_component<T>()
- `src/engine/input/input_system.h` — InputSystem API (mouse_delta, is_down, is_mouse_down/up)
- `src/engine/input/key_code.h` — KeyCode enum values
- `src/engine/window/window.h` — set_mouse_capture, is_mouse_captured
- `src/engine/platform/platform.h` — delta_time(), input_system()
- `src/engine/math/quat.h` — Quat::from_euler(pitch, yaw, roll)
- `src/engine/math/math.h` — radians(), epsilon
- `src/engine/math/vec3.h` — Vec3::unit_y()
- `src/cmd/apps/free_camera_app.cpp` / `.h` — code to refactor (lines 90–146 of .cpp, members in .h)
- `src/cmd/apps/phong_app.cpp` / `.h` — code to refactor (lines 309–368 of .cpp, members in .h)
- `src/cmd/apps/gltf_demo_app.cpp` / `.h` — reference pattern for model loading with AssetManager
- `src/cmd/app.h` — App base class (running_, config(), setup(), render()); will add world() and set_running()
- `src/cmd/app.cpp` — run_app(), render loop to integrate Updatable update step
- `src/cmd/main.cpp` — scene dispatch table, help text
- `src/engine/asset/model_loader.cpp` — build_node() quaternion conversion (lines 792–800), build_model_from_mesh(), TRS application
- `src/engine/render/model_utils.h` — add_model_to_world() traversal pattern
- `docs/adr/ADR-020-custom-logging-system.md` — must be updated
- `docs/wiki/domain/logging.md` — must be updated
- `docs/wiki/architecture/module-map.md` — must be updated
- `src/engine/scene/world.h` — `add_component<T>()` template to add Updatable auto-registration; `remove_component<T>()` cleanup
- `src/engine/scene/world.cpp` — `flush_destroyed()` cleanup pattern for Updatable pointers

## Files allowed to change

Specific files that may be modified or created:

**Create:**
1. `src/engine/scene/free_camera_movement.h` — new FreeCameraMovement component header (inherits Component + Updatable)
2. `src/engine/scene/free_camera_movement.cpp` — new FreeCameraMovement component implementation
3. `src/cmd/apps/gltf_helmet_app.h` — new gltf-helmet app header with `world()` override
4. `src/cmd/apps/gltf_helmet_app.cpp` — new gltf-helmet app implementation; no manual camera iteration (auto-Updatable)
5. `src/engine/scene/updatable.h` — new `Updatable` pure abstract interface

**Modify:**
6. `src/engine/log/console_sink.cpp` — prepend `[HH:MM:SS.fff]` timestamp
7. `src/engine/log/console_sink.h` — update class-level comment documenting new format
8. `src/cmd/apps/free_camera_app.cpp` — remove inline camera movement code; add `world()` override; no manual `each<FreeCameraMovement>` call (auto-Updatable)
9. `src/cmd/apps/free_camera_app.h` — remove `yaw_`, `pitch_`, `prev_right_click_` members; add `world()` override
10. `src/cmd/apps/phong_app.cpp` — remove inline camera movement code; add `world()` override; keep orbiting lights code; add FreeCameraMovement in setup
11. `src/cmd/apps/phong_app.h` — remove `yaw_`, `pitch_`, `prev_right_click_` members; add `world()` override
12. `src/engine/scene/world.h` — add `updatables_` member, `update_updatables()` method; auto-register in `add_component<T>()`; cleanup in `remove_component<T>()`
13. `src/engine/scene/world.cpp` — implement `update_updatables()`; Updatable cleanup in `flush_destroyed()`
14. `src/cmd/app.h` — add `[[nodiscard]] virtual auto world() noexcept -> World*` and `void set_running(bool v)`
15. `src/cmd/app.cpp` — add Updatable update step before `app.render()` in render loop
16. `src/cmd/main.cpp` — add `#include "apps/gltf_helmet_app.h"`, add `"gltf-helmet"` to scene dispatch table, add help text for `gltf-helmet`
17. `docs/adr/ADR-020-custom-logging-system.md` — update section 5 ConsoleSink format description
18. `docs/wiki/domain/logging.md` — update console sink format example, add `GltfHelmet` source tag
19. `docs/wiki/architecture/module-map.md` — document FreeCameraMovement component and gltf-helmet app

## Files forbidden to change

- `src/engine/log/file_sink.cpp` / `.h` — timestamp format must remain ISO 8601 (AC-002)
- `src/engine/asset/model_loader.cpp` — read-only investigation target; if a bug is found, changes are allowed only with explicit approval from the orchestrator. Logging-only additions (e.g., temporary debug prints) are permitted during investigation.
- `src/engine/render/pbr_shaders.h` — PBR shader pipeline is out of scope per spec non-goals
- Any file in `src/engine/render/` beyond render already referenced — PBR/material pipeline changes excluded
- Any other app files beyond those listed above
- `src/engine/platform/` — no changes to platform layer
- `src/engine/window/` — no changes to window layer
- `src/engine/input/` — no changes to input layer

## Existing conventions to follow

1. **Code style**: `snake_case` for functions/variables, `PascalCase` for classes, `UPPER_CASE` for constants. Opening braces on same line. `auto` return type with trailing return type. `constexpr` for compile-time constants. `static constexpr std::string_view` for shader source strings.
2. **Namespace**: `buddd::engine` for engine-level types (`Component`, `InputSystem`, etc.), `buddd::cmd::app` for app classes.
3. **Include style**: `#include "path/from/src/root/file.h"` — paths are relative to `src/`.
4. **Log tag declaration**: Every `.cpp` using log macros has `BUDDD_LOG_TAG("Module:Sub")` at file scope. New files: `free_camera_movement.cpp` → `BUDDD_LOG_TAG("Scene:FreeCamera")`, `gltf_helmet_app.cpp` → `BUDDD_LOG_TAG("GltfHelmet")`.
5. **Component pattern**: Inherit from `Component`, define constructor/destructor in `.cpp`, access `entity()` to get owning Entity handle. Use `entity().get_component<T>()` to find sibling components. Protected members `world_` and `entity_id_` from base are inherited. See `CameraComponent` for reference.
6. **App pattern**: Inherit from `App`, implement `config()`, `setup()`, `render()`. Store `World` as `std::unique_ptr<World>`, `RenderSystem` as `std::unique_ptr<RenderSystem>`. Camera entity is stored as `Entity` or `std::unique_ptr<Entity>`.
7. **World::each usage**: `world_.each<T>([&](Entity e, T& component) -> bool { ...; return true; })`. Return `false` to stop iteration. The callback receives `(Entity, T&)`.
8. **Mouse capture convention**: Right-click toggles capture via edge detection (`prev_right_click_`). Mouse look uses `mouse_delta()` (dx, dy). Forward is `orientation * Vec3{0,0,-1}` projected onto XZ plane and normalized. Movement uses `W/A/S/D/Space/CtrlLeft/CtrlRight`.
9. **Camera::from_euler**(pitch, yaw, roll): applies pitch (X), yaw (Y), roll (Z) in XYZ order, all angles in radians. Yaw around Y follows right-hand rule. `negative dx` → yaw decreases (turns right in RH system with Y-up).
10. **Updatable pattern**: `Updatable` is a pure abstract class NOT inheriting from `Component`. A component can inherit from both via `class Foo : public Component, public Updatable`. `dynamic_cast<Updatable*>` works on any component pointer (requires RTTI, which is enabled).
11. **World auto-registration**: When adding a component via `add_component<T>()`, use `if constexpr (std::is_base_of_v<Updatable, T>)` to conditionally push `static_cast<Updatable*>(ptr)` into `updatables_`. When removing (in `remove_component<T>()` or during `flush_destroyed()`), use `dynamic_cast<Updatable*>` + `std::erase` to remove stale pointers before the component destructor runs.
12. **Header guard style**: `#pragma once` (no `#ifndef` guards).

## Required implementation behavior

### 1. Console timestamps (AC-001, AC-002, AC-003, AC-028, EC-005)

**File**: `src/engine/log/console_sink.cpp`

Replace the `std::fprintf` call in `ConsoleSink::write()` with one that prepends a wall-clock timestamp `[HH:MM:SS.fff]`:

```cpp
void ConsoleSink::write(const LogMessage& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;

    struct tm local_tm;
    localtime_r(&time_t_now, &local_tm);

    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &local_tm);

    std::fprintf(stderr, "[%s.%03d] [%s] [%.*s] %s\n",
                 timestamp, static_cast<int>(ms),
                 level_name(message.level),
                 static_cast<int>(message.tag.length()), message.tag.data(),
                 message.message.c_str());
}
```

- **Use `std::chrono::system_clock::now()`** for wall-clock time (AC-028).
- **Format**: `[HH:MM:SS.fff] [LEVEL] [Tag] message` where HH=00-23, MM=00-59, SS=00-59, fff=000-999.
- **Time zone**: local time via `localtime_r()`.
- **No `noexcept`** — match `FileSink::write()` which is not `noexcept` (EC-005).
- **No try/catch** around the chrono call — let exceptions propagate naturally (EC-005).
- **No `--no-timestamp` flag** (AC-003).
- **Include**: add `#include <chrono>`, `#include <ctime>` if not already present (may be transitively included; add explicitly).

**Header**: `src/engine/log/console_sink.h` — update the class-level comment from `"No timestamp, no color."` to `"Format: [HH:MM:SS.fff] [LEVEL] [Tag] message\n"`.

### 2. FreeCameraMovement component (AC-004 through AC-012, EC-001 through EC-004, ER-004)

**New file**: `src/engine/scene/free_camera_movement.h`

```cpp
#pragma once

#include "input/key_code.h"
#include "scene/component.h"
#include "scene/updatable.h"

#include <cstdint>

namespace buddd::engine {
class InputSystem;
class Window;
} // namespace buddd::engine

namespace buddd::engine {

class FreeCameraMovement : public Component, public Updatable {
public:
    /// @param initial_yaw  Initial yaw angle in radians (default 0).
    /// @param initial_pitch Initial pitch angle in radians (default 0).
    explicit FreeCameraMovement(float initial_yaw = 0.0f, float initial_pitch = 0.0f);

    // -- Configurable parameters (public per AC-005) --
    float move_speed = 5.0f;
    float mouse_sensitivity = 0.002f;
    float pitch_clamp_degrees = 89.0f;
    bool invert_yaw = false;
    bool invert_pitch = false;

    /// Called once per frame via Updatable dispatch. Returns false if ESC was
    /// pressed (request exit), true otherwise.
    /// @param input  The engine input system for this frame.
    /// @param window The application window (for mouse capture).
    /// @param dt     Delta time in seconds since last frame.
    [[nodiscard]] auto update(const InputSystem& input, Window& window, float dt) -> bool override;

private:
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    bool prev_right_click_ = false;
    bool missing_camera_warned_ = false; // one-shot warning flag
};

} // namespace buddd::engine
```

**Design rationale**:
- Inherits from both `Component` (for ECS integration) and `Updatable` (for auto-dispatch via `World::update_updatables()`).
- Constructor takes `initial_yaw`/`initial_pitch` so that apps with non-zero initial camera orientation (e.g., `phong_app`) can initialize the internal state correctly. Defaults to 0 for apps where identity orientation is correct (free_camera_app, gltf-helmet).
- `move_speed`, `mouse_sensitivity`, `pitch_clamp_degrees`, `invert_yaw`, `invert_pitch` are public per AC-005.
- `yaw_`, `pitch_`, `prev_right_click_` are private per AC-012.
- `missing_camera_warned_` prevents spam-logging when no CameraComponent is found (EC-002, ER-004).
- The `update()` method signature (`const InputSystem&`, `Window&`, `float dt` → `bool`) matches the `Updatable` interface exactly (`override`).

**New file**: `src/engine/scene/free_camera_movement.cpp`

```cpp
#include "scene/free_camera_movement.h"
#include "log/log.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "scene/camera_component.h"
#include "scene/entity.h"
#include "input/input_system.h"
#include "window/window.h"

#include <algorithm> // std::clamp

BUDDD_LOG_TAG("Scene:FreeCamera");

namespace buddd::engine {

FreeCameraMovement::FreeCameraMovement(float initial_yaw, float initial_pitch)
    : yaw_(initial_yaw), pitch_(initial_pitch)
{
}

auto FreeCameraMovement::update(const InputSystem& input, Window& window, float dt) -> bool {
    // ── ESC to exit (checked first, before mouse capture) ──
    if (input.is_down(KeyCode::Escape)) {
        return false;
    }

    // ── Get CameraComponent on the same entity ──
    auto cam_opt = entity().get_component<CameraComponent>();
    if (!cam_opt) {
        if (!missing_camera_warned_) {
            BUDDD_LOG_WARN("FreeCameraMovement: entity has no CameraComponent — movement is a no-op");
            missing_camera_warned_ = true;
        }
        return true; // no-op, don't crash
    }
    auto& cam = cam_opt->camera();

    // ── Mouse capture (right-click toggle, edge-detected) ──
    bool curr_right_click = input.is_mouse_down(MouseButton::Right);
    if (curr_right_click && !prev_right_click_) {
        window.set_mouse_capture(true);
    }
    if (!curr_right_click && prev_right_click_) {
        window.set_mouse_capture(false);
    }
    prev_right_click_ = curr_right_click;

    bool mouse_captured = window.is_mouse_captured();
    if (!mouse_captured) {
        return true; // no movement when mouse not captured
    }

    // ── Mouse look ──
    auto [dx, dy] = input.mouse_delta();
    float yaw_sign = invert_yaw ? 1.0f : -1.0f;
    float pitch_sign = invert_pitch ? -1.0f : 1.0f;
    yaw_ += dx * mouse_sensitivity * yaw_sign;
    pitch_ += -dy * mouse_sensitivity * pitch_sign;

    float pitch_clamp_rad = math::radians(pitch_clamp_degrees);
    pitch_ = std::clamp(pitch_, -pitch_clamp_rad, pitch_clamp_rad);

    cam.set_orientation(math::Quat::from_euler(pitch_, yaw_, 0.0f));

    // ── Keyboard movement ──
    constexpr float k_epsilon = 1.0e-6f;
    math::Vec3 forward = cam.orientation() * math::Vec3{0.0f, 0.0f, -1.0f};
    forward.y = 0.0f;
    if (forward.length_squared() > k_epsilon) {
        forward.normalize();
    }

    math::Vec3 right = cam.orientation() * math::Vec3{1.0f, 0.0f, 0.0f};
    math::Vec3 movement{0.0f, 0.0f, 0.0f};

    if (input.is_down(KeyCode::W))           { movement += forward; }
    if (input.is_down(KeyCode::S))           { movement -= forward; }
    if (input.is_down(KeyCode::D))           { movement += right; }
    if (input.is_down(KeyCode::A))           { movement -= right; }
    if (input.is_down(KeyCode::Space))       { movement += math::Vec3::unit_y(); }
    if (input.is_down(KeyCode::ControlLeft))  { movement -= math::Vec3::unit_y(); }
    if (input.is_down(KeyCode::ControlRight)) { movement -= math::Vec3::unit_y(); }

    cam.set_position(cam.position() + movement * move_speed * dt);

    return true;
}

} // namespace buddd::engine
```

**Edge cases** (from spec):
- **dt = 0** (EC-001): `movement * move_speed * 0` = no movement. `mouse_delta()` returns (0,0). No division by zero.
- **No CameraComponent on entity** (EC-002, ER-004): Logs `[WARN]` once, returns `true` (no-op). No assert, no crash.
- **Window not focused / no input** (EC-003): `mouse_delta()` returns (0,0), no mouse look. Movement unchanged. No drift.
- **Rapid right-click toggle** (EC-004): Edge detection via `prev_right_click_` prevents repeated toggle. State machine tracks `false→true` edge (capture) and `true→false` edge (release).
- **Component not yet attached** (ER-004): `entity()` already requires attachment; if called before, behaviour is undefined (same as other components). The `get_component<CameraComponent>()` guards against this by returning `nullopt`, triggering the warning path.

### 3. Refactor free_camera_app (AC-013, AC-014, AC-015, AC-018)

**Changes with Updatable system**: The camera movement is now handled automatically by `run_app()` calling `World::update_updatables()` before each `render()` call. `FreeCameraMovement` (which inherits from both `Component` and `Updatable`) is auto-registered in `updatables_` when added to an entity. No manual iteration is needed.

**File**: `src/cmd/apps/free_camera_app.cpp`

Changes:
1. **Remove** the inline camera movement code (entire body from `auto& input = device.window().platform().input_system()` down to just before `// ── Render ──`).
2. **Do NOT add** any `each<FreeCameraMovement>` replacement — the Updatable system handles it via `World::update_updatables()` in the `run_app()` render loop.
3. **Remove** `#include "input/input_system.h"`, `#include "math/quat.h"`, `#include "math/vec3.h"`, `#include "window/window.h"`, `#include "platform/platform.h"` if no longer needed.
4. **Add** `#include "scene/free_camera_movement.h"` (needed for `add_component<FreeCameraMovement>()` in setup).
5. In the **setup** function, after the `CameraComponent` is added, add `FreeCameraMovement` to the camera entity:
```cpp
camera_entity_.add_component<be::FreeCameraMovement>();
```
   The default yaw=0, pitch=0 matches the identity orientation set for the camera (position (0,2,5), identity orientation).
6. The `render()` method now only contains:
```cpp
auto buddd::cmd::app::FreeCameraApp::render(be::RenderDevice&, int) -> void {
    render_system_->render_scene();
}
```

**File**: `src/cmd/apps/free_camera_app.h`

1. **Remove** private members: `float yaw_ = 0.0f;`, `float pitch_ = 0.0f;`, `bool prev_right_click_ = false;`.
2. **Add** public override:
```cpp
auto world() noexcept -> be::World* override { return world_.get(); }
```
3. Add `#include "scene/free_camera_movement.h"` is needed in the `.cpp`, not the header.

**Behaviour preservation** (AC-015): The refactored app must behave identically:
- Right-click capture toggle, mouse look, WASD+Space/Ctrl movement, ESC to exit.
- Initial camera position (0, 2, 5), identity orientation, 60° FOV.
- `move_speed` = 5.0f, `mouse_sensitivity` = 0.002f, `pitch_clamp_degrees` = 89.0f (component defaults match the hardcoded values).
- Camera update is now performed by `World::update_updatables()` (called by `run_app()` before each `render()`), not by manual `each<FreeCameraMovement>()` inside `render()`.

### 4. Refactor phong_app (AC-021, AC-022, AC-023)

**Changes with Updatable system**: Camera movement is handled automatically by `World::update_updatables()`. The app only needs to override `world()` and add `FreeCameraMovement` in setup.

**File**: `src/cmd/apps/phong_app.cpp`

Changes:
1. **Remove** the inline camera movement code (lines 309–368, from `auto& input = ...` down to the orbiting lights update).
2. **Do NOT add** any `each<FreeCameraMovement>` replacement — the Updatable system handles it automatically.
3. **Keep** the orbiting lights code (lines 370–385 in original) — this is NOT camera movement code.
4. **Remove** `auto& input = device.window().platform().input_system();` (no longer needed inline).
5. **Remove** `float dt = device.window().platform().delta_time();`.
6. **Remove** `#include "input/input_system.h"`, `#include "math/quat.h"`, `#include "math/vec3.h"`, `#include "window/window.h"`, `#include "platform/platform.h"` if not used elsewhere.
7. **Add** `#include "scene/free_camera_movement.h"`.

8. In the **setup** function, after the `CameraComponent` is added, add `FreeCameraMovement` with initial yaw/pitch matching the existing camera orientation:
```cpp
// Remove the yaw_/pitch_ assignments (lines 301-302 in original).
// Add FreeCameraMovement to the camera entity:
camera_entity_->add_component<be::FreeCameraMovement>(
    be::math::radians(35.0f),   // initial yaw — matches existing yaw_ initialisation
    be::math::radians(-18.0f)   // initial pitch — matches existing pitch_ initialisation
);
```

9. The `render()` method now only keeps the orbiting lights update and render call:
```cpp
auto buddd::cmd::app::PhongApp::render(be::RenderDevice& device, int) -> void {
    float dt = device.window().platform().delta_time();
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - start_time_).count();

    // ── Update orbiting lights ──
    float t = elapsed;
    float orbit_r = 6.0f;
    float orbit_y = 2.5f;
    pointA_entity_->transform().position = be::math::Vec3{...};
    pointB_entity_->transform().position = be::math::Vec3{...};

    // ── Render ──
    render_system_->render_scene();
}
```

**Important**: The camera position and orientation set via `cam.set_position()` and `cam.set_orientation()` in `setup()` (lines 215-220) must remain — these set the initial camera pose. The FreeCameraMovement constructor just ensures the internal `yaw_`/`pitch_` state matches for continuity.

**File**: `src/cmd/apps/phong_app.h`

1. **Remove** private members: `float yaw_ = 0.0f;`, `float pitch_ = 0.0f;`, `bool prev_right_click_ = false;`.
2. **Add** public override:
```cpp
auto world() noexcept -> be::World* override { return world_.get(); }
```
3. **Check** `#include <chrono>` — must keep (for `start_time_`), `<algorithm>` may be removed if `std::clamp` no longer used.

**Behaviour preservation** (AC-023): The refactored app must behave identically:
- Initial camera position (6.0, 3.5, 8.0), orientation pitch=-18°, yaw=35°.
- Same camera controls. Orbiting lights unaffected.
- Same window title, resolution, cubes, and lighting setup.
- Camera update is now performed by `World::update_updatables()` (called by `run_app()` before each `render()`).

### 5. New gltf-helmet app (AC-019, AC-020, AC-021, AC-022)

**New file**: `src/cmd/apps/gltf_helmet_app.h`

```cpp
#pragma once

#include "app.h"

#include <memory>

namespace buddd::engine {
class RenderDevice;
class World;
class RenderSystem;
class Entity;
class AssetManager;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// glTF Helmet demo: loads the DamagedHelmet model with free-camera controls.
/// Camera is auto-updated via the Updatable system (FreeCameraMovement).
class GltfHelmetApp final : public App {
public:
    GltfHelmetApp();
    ~GltfHelmetApp() override;

    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 glTF Helmet", 1280, 720};
    }

    [[nodiscard]] auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

    auto world() noexcept -> buddd::engine::World* override { return world_.get(); }

private:
    std::unique_ptr<buddd::engine::AssetManager> asset_manager_;
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::Entity> camera_entity_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
};

} // namespace buddd::cmd::app
```

**Design rationale**: Follows the exact pattern of `GltfDemoApp` (header has forward declarations, camera entity stored as `unique_ptr<Entity>`, constructor/destructor defined in `.cpp`).

**New file**: `src/cmd/apps/gltf_helmet_app.cpp`

```cpp
#include "apps/gltf_helmet_app.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "asset/model_asset.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/model_utils.h"
#include "scene/free_camera_movement.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "platform/platform.h"
#include "window/window.h"
#include "input/input_system.h"

#include <cstdlib>
#include <memory>

BUDDD_LOG_TAG("GltfHelmet");

namespace be = buddd::engine;

buddd::cmd::app::GltfHelmetApp::GltfHelmetApp() = default;
buddd::cmd::app::GltfHelmetApp::~GltfHelmetApp() = default;

auto buddd::cmd::app::GltfHelmetApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    // AssetManager
    std::string base_path = "assets";
    auto am_result = be::AssetManager::create(device, base_path);
    if (!am_result) {
        BUDDD_LOG_ERROR("Failed to create AssetManager: {}",
                        be::to_string(am_result.error()));
        return std::unexpected(am_result.error());
    }
    asset_manager_ = std::move(*am_result);

    world_ = std::make_unique<be::World>();

    // ── Camera ──
    camera_entity_ = std::make_unique<be::Entity>(be::Entity::create(*world_));
    be::math::Camera camera;
    camera_entity_->add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity_->get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{0.0f, 1.5f, 3.0f});
    // Pitch to look at origin from (0, 1.5, 3):
    //   direction = (0, 0, 0) - (0, 1.5, 3) = (0, -1.5, -3)
    //   pitch = asin(-1.5 / sqrt(1.5^2 + 3^2)) ≈ -0.4636 rad
    cam.set_orientation(be::math::Quat::from_euler(-0.4636f, 0.0f, 0.0f));
    cam.set_perspective(be::math::radians(55.0f),
                        static_cast<float>(config().width) / static_cast<float>(config().height),
                        0.1f, 100.0f);

    // FreeCameraMovement on the camera entity
    // Initial yaw=0, pitch matches the look-at direction above
    camera_entity_->add_component<be::FreeCameraMovement>(0.0f, -0.4636f);

    // ── Directional light (white, intensity 1.5, pitch=-45°, yaw=45°) ──
    {
        auto light_entity = be::Entity::create(*world_);
        light_entity.add_component<be::DirectionalLightComponent>(
            be::math::Vec3{1.0f, 1.0f, 1.0f},  // white
            1.5f                                 // intensity
        );
        light_entity.transform().rotation =
            be::math::Quat::from_euler(be::math::radians(-45.0f),
                                        be::math::radians(45.0f), 0.0f);
    }

    // ── Load DamagedHelmet ──
    auto model_asset = asset_manager_->create<be::ModelAsset>(
        "models/damaged-helmet/DamagedHelmet");
    if (!model_asset) {
        BUDDD_LOG_ERROR("Failed to load DamagedHelmet model: {}",
                        be::to_string(model_asset.error()));
        return std::unexpected(model_asset.error());
    }

    auto& root = (*model_asset)->root_node();
    be::add_model_to_world(*world_, root);

    // ── Render system ──
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);

    return {};
}

auto buddd::cmd::app::GltfHelmetApp::render(be::RenderDevice&, int) -> void {
    // Camera is auto-updated via World::update_updatables() in run_app()
    render_system_->render_scene();
}

**Updatable auto-dispatch**: The `render()` method no longer manually iterates `FreeCameraMovement`. The `run_app()` function calls `app.world()->update_updatables(input, window, dt)` before each `app.render()` call, which triggers `FreeCameraMovement::update()` on the camera entity automatically. ESC exit is handled by `update_updatables()` returning `false`, which calls `app.set_running(false)`.
```

### 6. Scene dispatch in main.cpp (AC-020)

**File**: `src/cmd/main.cpp`

1. **Add include**: `#include "apps/gltf_helmet_app.h"` between the existing app includes (after `#include "apps/phong_app.h"`).
2. **Add scene dispatch** after the `"phong"` case:
```cpp
else if (scene == "gltf-helmet")
    app = std::make_unique<bc::app::GltfHelmetApp>();
```
3. **Add help text** in the `Unknown scene` branch, after the `gltf` line:
```
                "  gltf-helmet  Interactive DamagedHelmet inspection with free camera\n"
```

### 7. Helmet investigation (AC-023 through AC-027)

The Code Agent must investigate and fix the DamagedHelmet deformation and slow load. The following areas must be examined:

**Deformation (AC-024a–e, AC-025, AC-026, AC-027):**

1. **Quaternion conversion** (AC-025, AC-026): Review `build_node()` lines 792–800 in `model_loader.cpp`. The glTF quaternion `(x, y, z, w)` → engine `Quat(w, x, y, z)` conversion is correct per the comment and the Quat constructor signature `Quat(float w_, float x_, float y_, float z_)`. For the helmet node with rotation `[0.7071, 0, -0, 0.7071]`, the engine Quat becomes `(0.7071, 0.7071, 0, -0)` which evaluates to `(0.7071, 0.7071, 0, 0)` (since -0.0f == 0.0f). This represents a 90° rotation around X. **Verify by inspection** that this produces the correct orientation.

2. **TRS application** (AC-026): Review `add_model_to_world_impl()` in `model_utils.h` (lines 40–43). TRS from node is copied to entity transform exactly once per node. No double application.

3. **Single traversal of hierarchy** (AC-027): `build_node()` in `model_loader.cpp` recurses through children to construct the `ModelNode` tree. `add_model_to_world()` traverses the same hierarchy to create ECS entities. These are two separate traversals (loading → tree construction → ECS instantiation), which is correct by design. Each traversal does its job exactly once. If a bug exists (e.g., a node's model is moved out during `add_model_to_world` and then accessed again), this should be caught.

4. **Vertex/index buffer integrity**: `build_model_from_mesh()` (line 571) reads glTF accessor data and allocates vertex/index buffers. Verify that vertex count matches accessor (14556) and index count matches (46356) (AC-024a, AC-024b). Verify vertex position min/max bounds within tolerance (AC-024c).

5. **Visual inspection** (AC-024e): Launch `buddd run gltf-helmet` from default camera position and verify no missing triangles, inverted faces, or clearly wrong geometry.

**Performance (AC-023):**

The DamagedHelmet takes ~10s to load. The target is < 3s. Investigate the following areas:

1. **Texture loading**: The helmet has 5 external textures. Check `load_gltf_texture()` for redundant loads — if the same texture file is loaded for multiple primitives or materials, it should be cached. `AssetManager` may already provide caching; verify.
2. **Material creation**: Check `create_pbr_material()` — is a new PBR material created for each primitive even when the material index is the same?
3. **Data copies**: In `build_model_from_mesh()`, vertex data for each primitive is appended to `all_vertices`, which may cause repeated reallocations. Consider `reserve()` calls.
4. **AssetManager YAML metadata**: Ensure `assets/models/damaged-helmet/DamagedHelmet.y[a]ml` exists and is properly configured. If missing, the model may not load through the standard path.

**Diagnostic approach**: Add temporary `BUDDD_LOG_INFO` markers before/after expensive operations (model load, texture load, material creation) to measure elapsed time using the new console timestamps.

**Fix scope**: The fix must stay within the constraints of the spec's Non-goals — no async loading, no PBR shader changes. If the root cause requires changes outside this scope (e.g., PBR shader pipeline), report to orchestrator per the investigation risk note in the spec.

### 8. Update ADR-020 logging section (spec Documentation Impact)

**File**: `docs/adr/ADR-020-custom-logging-system.md`

In section 5 (Sink interface), update the ConsoleSink bullet point:
- **Before**: "**ConsoleSink** — Always present, writes `[LEVEL] [Tag] message\n` to stderr. No timestamp, no color."
- **After**: "**ConsoleSink** — Always present, writes `[HH:MM:SS.fff] [LEVEL] [Tag] message\n` to stderr. Wall-clock timestamp with millisecond precision, no color."

### 9. Update wiki logging.md (spec Documentation Impact)

**File**: `docs/wiki/domain/logging.md`

1. Update the **Console sink** section to show the new format:
   - Format: `[HH:MM:SS.fff] [LEVEL] [Tag] message\n`
   - Update example output lines.
2. Add new source tag to the source tag registry:
   - `**`GltfHelmet`** — glTF Helmet demo app`
3. Keep the File sink format unchanged.

### 10. Update wiki module-map.md (spec Documentation Impact)

**File**: `docs/wiki/architecture/module-map.md`

Add entries for:
- `src/engine/scene/free_camera_movement.h` / `.cpp` — FreeCameraMovement component (ECS)
- `src/cmd/apps/gltf_helmet_app.h` / `.cpp` — glTF Helmet demo app

### 11. Updatable system (AC-004, AC-005, AC-006, AC-007, AC-008, AC-034, EC-010)

The Updatable system eliminates manual component iteration in app render loops. Apps with a `World` simply override `world()` to return their `World*`, and `run_app()` automatically calls `World::update_updatables()` before each `app.render()`.

#### 11.1 New file: `src/engine/scene/updatable.h`

Create a pure abstract class with `#pragma once` and namespace `buddd::engine`:

```cpp
#pragma once

namespace buddd::engine {

class InputSystem;
class Window;

/// Pure abstract interface for per-frame update logic.
/// Components can inherit from both Component and Updatable via multiple
/// inheritance. Updatable is orthogonal to Component — it does not depend on or
/// inherit from Component.
class Updatable {
public:
    virtual ~Updatable() = default;

    /// Called once per frame before app.render().
    /// @param input  Engine input system for current frame.
    /// @param window Application window (for mouse capture, etc.).
    /// @param dt     Delta time in seconds since last frame.
    /// @return false to request early loop exit (e.g., ESC pressed),
    ///         true to continue normal loop execution.
    [[nodiscard]] virtual auto update(const InputSystem& input, Window& window, float dt) -> bool = 0;
};

} // namespace buddd::engine
```

#### 11.2 Modify `src/engine/scene/world.h`

**Add private member**:
```cpp
std::vector<Updatable*> updatables_;
```

**Add public method declaration**:
```cpp
/// Iterates all registered Updatable components, calling update() on each.
/// Returns false on the first false return (short-circuit), true if all pass.
auto update_updatables(const InputSystem& input, Window& window, float dt) -> bool;
```

**Modify `add_component<T>()` template** (after `ptr->on_attach()`, before the return):
```cpp
if constexpr (std::is_base_of_v<Updatable, T>) {
    updatables_.push_back(static_cast<Updatable*>(ptr));
}
```

**Modify `remove_component<T>()` template** — clean up `updatables_` BEFORE the component is erased:
```cpp
template<typename T>
inline auto World::remove_component(EntityId id) -> bool {
    auto* node = lookup_node(id);
    // UB if node is null or pending_destroy_.
    for (auto it = node->components_.begin(); it != node->components_.end(); ++it) {
        if (dynamic_cast<T*>(it->get())) {
            // If this component derives from Updatable, remove its raw pointer
            // from updatables_ before destroying the component.
            if (auto* upd = dynamic_cast<Updatable*>(it->get())) {
                std::erase(updatables_, upd);
            }
            node->components_.erase(it);
            return true;
        }
    }
    return false;
}
```

**Add include**: `#include "scene/updatable.h"` (or forward-declare `Updatable` at namespace scope; prefer include since the vector stores `Updatable*` and the templates need the complete type for `std::is_base_of_v`).

**Add `#include <algorithm>`** for `std::erase` if not already included.

#### 11.3 Modify `src/engine/scene/world.cpp`

**Implement `update_updatables()`**:
```cpp
auto World::update_updatables(const InputSystem& input, Window& window, float dt) -> bool {
    for (auto* upd : updatables_) {
        if (!upd->update(input, window, dt)) {
            return false; // short-circuit
        }
    }
    return true;
}
```

**Modify `flush_destroyed()`** — clean up Updatable raw pointers before the node's unique_ptr goes out of scope:

In the **parent-linked branch** (after `auto owned = std::move(*sit);`), before `owned` goes out of scope at `break`:
```cpp
// Clean up Updatable pointers before the component unique_ptrs are destroyed
for (auto& c : owned->components_) {
    if (auto* upd = dynamic_cast<Updatable*>(c.get())) {
        std::erase(updatables_, upd);
    }
}
```

In the **root-entity branch** (after `auto owned = std::move(*rit);`), same cleanup:
```cpp
for (auto& c : owned->components_) {
    if (auto* upd = dynamic_cast<Updatable*>(c.get())) {
        std::erase(updatables_, upd);
    }
}
```

**Include**: Add `#include "scene/updatable.h"` and `#include <algorithm>` in `world.cpp`.

#### 11.4 Modify `src/cmd/app.h`

**Add forward declaration** at top:
```cpp
namespace buddd::engine { class World; }
```

**Add public methods** after the existing `is_running()`:
```cpp
/// Returns a pointer to the app's World if one exists, nullptr otherwise.
/// Override in apps that have a World (for Updatable auto-dispatch).
[[nodiscard]] virtual auto world() noexcept -> buddd::engine::World* { return nullptr; }

/// Public setter to allow run_app() to stop the render loop.
void set_running(bool v) { running_ = v; }
```

#### 11.5 Modify `src/cmd/app.cpp` — render loop integration

**Add includes**:
```cpp
#include "scene/world.h"
#include "scene/updatable.h"
#include "input/input_system.h"
#include "platform/platform.h"
```

**Before `app.render(**device, frame)` in the render loop** (insert at line 112, before the render call):
```cpp
// ── Updatable auto-dispatch ──
if (auto* app_world = app.world()) {
    auto& input = (*device)->window().platform().input_system();
    auto& window = (*device)->window();
    float dt = (*device)->window().platform().delta_time();
    if (!app_world->update_updatables(input, window, dt)) {
        app.set_running(false);
    }
}
```

This must be placed after `app.on_frame_begin()` and before `app.render()`. The complete render loop segment becomes:

```cpp
// Frame start hook (hot-reload polling, etc.)
app.on_frame_begin();

// ── Updatable auto-dispatch ──
if (auto* app_world = app.world()) {
    auto& input = (*device)->window().platform().input_system();
    auto& window = (*device)->window();
    float dt = (*device)->window().platform().delta_time();
    if (!app_world->update_updatables(input, window, dt)) {
        app.set_running(false);
    }
}

// Render
app.render(**device, frame);
```

**Design rationale**: The Updatable auto-dispatch runs BEFORE `app.render()` so that apps never see stale input state. The `world()` virtual method is the integration point — apps without a world simply don't override it (default returns `nullptr`). This keeps the `run_app` function unaware of which specific apps have worlds and which don't.

#### 11.6 Dangling pointer safety

The `updatables_` vector stores raw `Updatable*` pointers. Pointers become dangling when:
- **Entity destroyed** via `destroy_entity()` + `flush_destroyed()`: Cleanup happens in `flush_destroyed()` before the `owned` unique_ptr goes out of scope (both parent-linked and root-entity branches).
- **Component removed** via `remove_component<T>()`: Cleanup happens in the template before `node->components_.erase(it)`.
- **World destroyed**: The `updatables_` vector is not explicitly cleared — all Component/Updatable objects are destroyed via the `EntityNode` tree (roots_ / children_). Since the World is being destroyed, no subsequent `update_updatables()` call is possible.

All cleanup operations use `dynamic_cast<Updatable*>` + `std::erase`, which is O(n) per removal. This is acceptable because the number of updatable components per frame is expected to be small (typically ≤ 10).

## Required tests

### Unit tests

- **ConsoleSink timestamp format**: Add a test in the existing logging test file (e.g., `tests/logging_tests.cpp` or create a small test) that creates a `ConsoleSink`, writes a `LogMessage`, and verifies the output matches `[HH:MM:SS.fff] [INFO] [TestTag] hello`. Use a `MemorySink` or capture stderr via pipe to verify the format. **Test must not assert wall-clock values** — only the format pattern and that the timestamp is present and non-empty.
- **FreeCameraMovement**: No unit tests per spec non-goals. Verified via integration in the three apps.

### E2E / Integration verification

| Step | Command | What to verify | AC |
|---|---|---|---|
| Console timestamps | `buddd run free-camera` | Every log line begins with `[HH:MM:SS.fff]` (observe stderr output) | AC-001 |
| File sink unchanged | `buddd run free-camera --log-file=/tmp/test.log` | File has ISO 8601 format `2026-06-06T14:32:05 [LEVEL] [Tag] message` | AC-002 |
| Updatable auto-dispatch | `buddd run free-camera` | Camera updates automatically; no `each<FreeCameraMovement>` in app render code | AC-006, AC-008 |
| FreeCameraApp behaviour | `buddd run free-camera` | Same camera controls, ESC to exit, position (0, 2, 5) | AC-015, AC-020 |
| PhongApp behaviour | `buddd run phong` | Same camera start (6, 3.5, 8), orbiting lights, controls | AC-018, AC-023 |
| GltfHelmet loads | `buddd run gltf-helmet` | Window title "Buddd Engine — glTF Helmet", 1280×720, DamagedHelmet visible with correct geometry | AC-024–027, AC-029e |
| Helmet load time | `buddd run gltf-helmet --log-level=debug` | Load time under 3 seconds (observe timestamp difference in log output) | AC-028 |
| ESC exit in gltf-helmet | `buddd run gltf-helmet`, press ESC | Window closes, process exits | AC-012 |
| Updatable cleanup safety | `buddd run free-camera` (or any): destroy entity with Updatable, verify no crash | No crash on next frame after entity destruction | AC-034 |

## Edge cases

All edge cases from the spec are carried forward:

| EC ID | Condition | Expected behaviour | Where enforced |
|---|---|---|---|
| EC-001 | `dt = 0` in FreeCameraMovement::update | No movement, no division by zero | `movement * move_speed * 0 = 0`, `mouse_delta()` returns (0,0) |
| EC-002 | No CameraComponent on entity | Warning logged once, returns true (no-op) | `missing_camera_warned_` flag + `get_component` guard |
| EC-003 | Window not focused / no input for long time | mouse_delta=(0,0), no drift | `mouse_delta()` returns accumulated frame value |
| EC-004 | Rapid right-click toggle | Edge detection works correctly via `prev_right_click_` | State machine tracks `false→true` and `true→false` transitions |
| EC-005 | `system_clock::now()` throws | Exception propagates, program terminates. NOT guarded. | No try/catch, not noexcept |
| EC-006 | Helmet texture load failure | Magenta fallback, warning logged | Handled by existing texture loading code |
| EC-007 | Helmet quaternion `[0.7071, 0, -0, 0.7071]` → engine | `Quat(0.7071, 0.7071, 0, 0)` = 90° X rotation | `build_node()` conversion |
| EC-008 | Index type Uint16 with 46356 indices | Correctly converted with vertex offset | `build_model_from_mesh()` index handling |
| EC-009 | Multiple root nodes in glTF scene | All root nodes traversed and added to world | `load_gltf_model()` loop over root_nodes |
| ER-001 | glTF parse failure | Error logged, setup returns error, engine exits | Existing error handling |
| ER-002 | Texture load failure | Warning, magenta fallback | Existing fallback code |
| ER-003 | Window creation fails at 1280×720 | Falls back to default (likely 1024×768) | Existing fallback in run_app |
| EC-010 | Entity with Updatable component destroyed (via destroy_entity + flush_destroyed) or component removed (via remove_component) | Updatable* pointer removed from updatables_ before component destructor runs. No dangling pointer, no crash on next update_updatables() call. | flush_destroyed() and remove_component<T>() cleanup in world.cpp/world.h |
| ER-004 | FreeCameraMovement::update before component attached | `entity().get_component` returns nullopt, warning logged once, returns true | `missing_camera_warned_` guard |
| ER-005 | Non-existent textures referenced | Warning logged, magenta fallback, no crash | Existing fallback code |

## Security impact

None. Console log output is plain text; no sensitive data expected. `system_clock::now()` has no security implications. No new input parsing or network access.

## Data and migration impact

None. No schema changes, no new persistent data, no migrations.

## API compatibility impact

- `ConsoleSink::write()` output format changes. Any code parsing console output (e.g., log aggregators, tests) must be updated. **No internal code parses console output** — the MemorySink is used for tests.
- `FreeCameraMovement` is a new class; no existing API is broken.
- `free_camera_app` and `phong_app` public APIs unchanged (`config()`, `setup()`, `render()` signatures identical).

## Documentation impact

| Document | Change required |
|---|---|
| `docs/adr/ADR-020-custom-logging-system.md` | ConsoleSink section: format changed from `[LEVEL] [Tag] message` to `[HH:MM:SS.fff] [LEVEL] [Tag] message` |
| `docs/wiki/domain/logging.md` | Update console sink format examples; add `GltfHelmet` source tag |
| `docs/wiki/architecture/module-map.md` | Add `Updatable` interface, `FreeCameraMovement` component, `gltf-helmet` app; document auto-registration and dispatch flow |

## ADR impact

- **ADR-020**: Section 5 must be updated to reflect the new ConsoleSink format. The ADR status remains "Accepted" — the update is a minor amendment to an implementation detail.
- **Updatable architecture**: The new `Updatable` interface, auto-registration in `World`, and auto-dispatch in `run_app()` constitute a meaningful architectural addition. The spec-critic and spec-author both noted this may warrant a **new ADR** to document the design rationale and orthogonality with `Component`. The ADR agent should evaluate whether ADR-024 or similar is needed.

## Done criteria

- [ ] **Console timestamps**: `console_sink.cpp` modified to prepend `[HH:MM:SS.fff]` using `std::chrono::system_clock::now()`. File sink unchanged. No `--no-timestamp` flag. No `noexcept`, no try/catch around chrono call.
- [ ] **Updatable interface**: `src/engine/scene/updatable.h` exists as pure abstract class with `virtual auto update(const InputSystem& input, Window& window, float dt) -> bool = 0;` and `virtual ~Updatable() = default;`. Namespace `buddd::engine`. No dependency on `Component`.
- [ ] **World auto-registration**: `world.h` has `std::vector<Updatable*> updatables_` member and `update_updatables()` method declaration. `add_component<T>()` uses `if constexpr (std::is_base_of_v<Updatable, T>)` to auto-register. `remove_component<T>()` cleans up via `dynamic_cast<Updatable*>` + `std::erase` before erasing.
- [ ] **World flush_destroyed cleanup**: `world.cpp` `flush_destroyed()` removes destroyed Updatable pointers from `updatables_` in both parent-linked and root-entity branches before the `owned` unique_ptr destructors run.
- [ ] **App changes**: `app.h` has `virtual auto world() noexcept -> World*` (default returns `nullptr`) and `void set_running(bool)`. `app.cpp` render loop calls `app.world()->update_updatables(input, window, dt)` before `app.render()`, with `set_running(false)` on `false` return.
- [ ] **FreeCameraMovement component**: Files `free_camera_movement.h` and `.cpp` exist in `src/engine/scene/`, compile without errors. Header shows: inherits `Component` AND `Updatable`, public fields (move_speed=5, mouse_sensitivity=0.002f, pitch_clamp_degrees=89.0f, invert_yaw=false, invert_pitch=false), `update(const InputSystem&, Window&, float dt) -> bool override`, private members (yaw_, pitch_, prev_right_click_, missing_camera_warned_).
- [ ] **free_camera_app refactored**: `free_camera_app.cpp` has NO inline mouse/keyboard code and NO `each<FreeCameraMovement>` call. Render() only calls `render_system_->render_scene()`. `free_camera_app.h` adds `world()` override returning `world_.get()`; removes `yaw_`, `pitch_`, `prev_right_click_` members. Camera entity has `FreeCameraMovement` added in setup.
- [ ] **phong_app refactored**: `phong_app.cpp` has NO inline mouse/keyboard code and NO `each<FreeCameraMovement>` call. Render() keeps only orbiting lights update + `render_scene()`. `phong_app.h` adds `world()` override returning `world_.get()`; removes `yaw_`, `pitch_`, `prev_right_click_` members. FreeCameraMovement added with initial yaw=radians(35), pitch=radians(-18).
- [ ] **gltf-helmet app**: `gltf_helmet_app.h` and `.cpp` exist in `src/cmd/apps/`. Config returns title "Buddd Engine — glTF Helmet", 1280×720. Setup creates camera at (0, 1.5, 3), looking at origin, 55° FOV, 0.1/100 near/far. Has FreeCameraMovement with yaw=0, pitch≈-0.4636. Directional light (white, intensity 1.5, pitch=-45°, yaw=45°). Loads DamagedHelmet via AssetManager. `world()` override returns `world_.get()`. Render loop has no manual `each<FreeCameraMovement>` — auto-updated via Updatable system.
- [ ] **Scene dispatch**: `main.cpp` has `"gltf-helmet"` → `GltfHelmetApp` dispatch and help text.
- [ ] **Helmet investigation complete**: Quaternion conversion verified correct. TRS application verified single-pass. Vertex count 14556 / index count 46356 verified. Performance bottleneck identified and fixed — load time < 3s.
- [ ] **ADR-020 updated**: ConsoleSink format documented as `[HH:MM:SS.fff] [LEVEL] [Tag] message`.
- [ ] **Wiki updated**: `logging.md` has new console format and `GltfHelmet` source tag. `module-map.md` has Updatable interface, FreeCameraMovement component, and gltf-helmet app entries.
- [ ] **Build succeeds**: `cmake --build .` completes without errors for all targets.
- [ ] **E2E verification**: `buddd run free-camera`, `buddd run phong`, and `buddd run gltf-helmet` all launch and behave correctly per the E2E verification table above.
