# IMPL-2026-06-001 — Camera → Transform Integration

## Source spec

`.specs/sprint-2026-06/camera-transform-integration/spec.md`

## Goal

Remove the `math::Camera` class and its duplicate position/orientation storage. Make `CameraComponent` projection-only (`fov_y`, `aspect`, `near`, `far`). Camera position and orientation come exclusively from the entity's `Transform`. Add a free function `math::view_matrix(Vec3, Quat)` and migrate every call site (13 app files, `render_system.cpp`, `free_camera_movement.cpp`, 2 test files) including migrating `cube_app` and `multi_material_app` from standalone `math::Camera` members to `CameraComponent` on an entity. Create a new ADR and update the wiki.

## Non-goals

1. Do NOT add position/orientation fields to `CameraComponent` — position/orientation must come from `entity().transform()`.
2. Do NOT change `World::active_camera()` / `register_camera()` / `unregister_camera()` — these accept `CameraComponent&` and remain unchanged.
3. Do NOT add view matrix caching or dirty flags — matrices are recomputed on each call.
4. Do NOT change `FreeCameraMovement` logic — only the API it uses to read/write position/orientation.
5. Do NOT add new features (orthographic, frustum culling, etc.) — pure refactor.
6. Do NOT change `triangle_app` or `run_app` — they do not use cameras.
7. Do NOT add new dependencies — only remove the existing `math::Camera` dependency.

## Relevant ADRs

- **ADR-002** (glm-wrapper-math): The math types (`Mat4`, `Vec3`, `Quat`) are thin wrappers around GLM with ABI compatibility. The `view_matrix()` free function must delegate to `glm::lookAt`.
- **ADR-009** (test-file-naming-convention): Test files follow `*_tests.cpp` naming.
- **ADR-019** (architecture-boundaries): Core math types live in `src/engine/math/`, scene components in `src/engine/scene/`. The free function belongs in `math/`.
- **ADR-005** (optional-ref-component-api): Components expose `entity()` after `on_attach()`.

## Files to inspect

The Code Agent MUST read (at minimum) the following files to understand current code before making any changes:

1. `src/engine/math/camera.h` — class to be deleted (full API)
2. `src/engine/math/camera.cpp` — implementation to be deleted
3. `src/engine/scene/camera_component.h` — component to rewrite
4. `src/engine/scene/camera_component.cpp` — component impl to rewrite
5. `src/engine/math/math.h` — must remove `#include "camera.h"` and add `view_matrix()` free function
6. `src/engine/scene/free_camera_movement.cpp` — call site to migrate
7. `src/engine/render/render_system.cpp` — call site to migrate
8. `src/cmd/apps/cube_app.h` and `cube_app.cpp` — standalone camera to migrate
9. `src/cmd/apps/multi_material_app.h` and `multi_material_app.cpp` — standalone camera to migrate
10. `src/cmd/apps/free_camera_app.cpp` — call site to migrate
11. `src/cmd/apps/phong_app.cpp` — call site to migrate
12. `src/cmd/apps/gltf_helmet_app.cpp` — call site to migrate
13. `src/cmd/apps/gltf_demo_app.cpp` — call site to migrate
14. `src/cmd/apps/hot_reload_app.cpp` — call site to migrate (uses `math::Camera` constructor + `camera()` accessor)
15. `src/cmd/apps/hot_reload_gltf_app.cpp` — call site to migrate (uses `math::Camera` constructor + `camera()` accessor + `world.active_camera()->camera()`)
16. `src/cmd/apps/asset_demo_app.cpp` — call site to migrate
17. `src/cmd/apps/cube_scene_app.cpp` — call site to migrate
18. `src/cmd/apps/textured_cube_app.cpp` — call site to migrate (uses `math::Camera` constructor)
19. `tests/scene_rendering_tests.cpp` — tests to update
20. `tests/lighting_tests.cpp` — tests to update
21. `src/engine/math/mat4.h` — to understand `Mat4::look_at` signature
22. `src/engine/math/quat.h` — to understand `Quat::from_euler`, `Quat::identity`
23. `src/engine/scene/component.h` — to understand `entity()` accessor
24. `src/engine/scene/transform.h` — to understand `Transform` struct (public `position`, `rotation` fields)

## Files allowed to change

### Deleted files
- `src/engine/math/camera.h`
- `src/engine/math/camera.cpp`

### Created files
- `docs/adr/ADR-024-camera-transform-integration.md`

### Modified files (engine core)
- `src/engine/scene/camera_component.h` — full rewrite
- `src/engine/scene/camera_component.cpp` — full rewrite
- `src/engine/math/math.h` — remove `#include "camera.h"` (line 12), add `view_matrix()` free function at end of namespace

### Modified files (engine modules)
- `src/engine/scene/free_camera_movement.cpp` — migrate from `cam.camera()` to `entity().transform()`
- `src/engine/render/render_system.cpp` — migrate from `cam_comp.camera().*` to `cam_comp.*` and `cam_comp.entity().transform()`

### Modified files (app files — all 13 listed in spec)
- `src/cmd/apps/cube_app.h`
- `src/cmd/apps/cube_app.cpp`
- `src/cmd/apps/multi_material_app.h`
- `src/cmd/apps/multi_material_app.cpp`
- `src/cmd/apps/cube_scene_app.cpp`
- `src/cmd/apps/textured_cube_app.cpp`
- `src/cmd/apps/free_camera_app.cpp`
- `src/cmd/apps/phong_app.cpp`
- `src/cmd/apps/gltf_helmet_app.cpp`
- `src/cmd/apps/gltf_demo_app.cpp`
- `src/cmd/apps/hot_reload_app.cpp`
- `src/cmd/apps/hot_reload_gltf_app.cpp`
- `src/cmd/apps/asset_demo_app.cpp`

### Modified files (tests)
- `tests/scene_rendering_tests.cpp`
- `tests/lighting_tests.cpp`

### Modified files (wiki — comprehensive list per spec-critic)
- `docs/wiki/domain/glossary.md` — update Camera and CameraComponent entries
- `docs/wiki/architecture/overview.md` — update camera_component.h entry
- `docs/wiki/domain/business-rules.md` — update light component accessor pattern section (lines 278-283)
- `docs/wiki/architecture/module-map.md` — update math/camera.h entries (lines 68-78) and camera_component.h entries (lines 108-132)

## Files forbidden to change

- Any file not listed in "Files allowed to change" MUST NOT be modified.
- `src/cmd/apps/triangle_app.cpp` and `src/cmd/apps/run_app.cpp` — no camera usage, must not be touched.
- `tests/render_device_tests.cpp` — no camera usage, must not be modified.
- `src/engine/CMakeLists.txt` — uses GLOB_RECURSE, no explicit camera.cpp reference; no change needed.
- Any ADR files other than the newly created `ADR-024-camera-transform-integration.md`.

## Existing conventions to follow

1. **Namespace nesting**: All engine code uses `namespace buddd::engine { ... }`. The `math` sub-namespace is `namespace buddd::engine::math`. App code uses `namespace buddd::cmd::app`.
2. **Inline functions in headers**: Small functions (getters, setters) are defined inline in the header with `auto func() noexcept -> T;` declaration + inline definition at bottom of file.
3. **`auto` return type**: Returns use trailing return type syntax: `auto func() noexcept -> Type`.
4. **`make_error` pattern**: Error returns use `return make_error(result);` where `result` is the error being propagated.
5. **GLM delegation**: Math wrapper types delegate to GLM via `reinterpret_cast` (assured by `static_assert` for size/std_layout).
6. **Component lifecycle**: Components override `on_attach()` (called after entity assigns `world_`/`entity_id_`). Constructor runs before `on_attach()` — guard for null `world_` in destructor.
7. **Entity Transform access**: `entity().transform().position` and `entity().transform().rotation` are public struct fields, not getter/setter methods.
8. **Logging pattern**: `BUDDD_LOG_TAG("TagName")` at top of .cpp file, `BUDDD_LOG_DEBUG/WARN/ERROR(...)` for log messages.
9. **Include style**: Includes use relative paths from `src/engine/` (e.g., `#include "math/math.h"`, `#include "scene/camera_component.h"`, `#include "log/log.h"`).
10. **`noexcept`**: All accessor and math functions are marked `noexcept`.
11. **`static_assert` for type traits**: `Mat4`, `Vec3`, `Quat` use `static_assert(std::is_standard_layout_v<...>)` and `static_assert(std::is_trivially_copyable_v<...>)`.

## Required implementation behavior

### 1. New `CameraComponent` header (`camera_component.h`)

The file must be rewritten from scratch. It must NOT include `"math/camera.h"`.

```cpp
#pragma once

#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "scene/component.h"

namespace buddd::engine {

class CameraComponent : public Component {
public:
    // -- Constructors --
    CameraComponent() = default;
    CameraComponent(float fov_y, float aspect, float near_plane, float far_plane);

    // -- Projection setters/getters --
    auto set_perspective(float fov_y, float aspect, float near_plane, float far_plane) -> void;
    auto fov_y() const noexcept -> float;
    auto aspect() const noexcept -> float;
    auto near_plane() const noexcept -> float;
    auto far_plane() const noexcept -> float;

    // -- Matrix computation (recomputed per call, no caching) --
    auto projection_matrix() const -> math::Mat4;
    auto view_matrix() const -> math::Mat4;                  // uses entity().transform()
    auto view_projection_matrix() const -> math::Mat4;       // projection * view

    // -- Look-at convenience (modifies entity Transform) --
    /// Orients the entity's rotation to look at `target` (keeps current position).
    auto look_at(math::Vec3 target) -> void;
    /// Sets entity's position to `eye`, orients to look at `center`.
    auto look_at(math::Vec3 eye, math::Vec3 center, math::Vec3 up) -> void;

    // -- Lifecycle (unchanged behavior) --
    auto on_attach() -> void override;
    ~CameraComponent() override;

private:
    float fov_y_ = math::radians(60.0f);
    float aspect_ = 16.0f / 9.0f;
    float near_ = 0.1f;
    float far_ = 100.0f;
};

} // namespace buddd::engine
```

### 2. New `CameraComponent` implementation (`camera_component.cpp`)

- Remove `#include "math/camera.h"`.
- Keep existing includes for `"scene/entity.h"`, `"scene/world.h"`, `"log/log.h"`.
- Add `#include "math/math.h"` for access to the new `view_matrix()` free function.

Implementation details:

```cpp
CameraComponent::CameraComponent(float fov_y, float aspect, float near_plane, float far_plane)
    : fov_y_(fov_y), aspect_(aspect), near_(near_plane), far_(far_plane) {}

auto CameraComponent::set_perspective(float fov_y, float aspect, float near_plane, float far_plane) -> void {
    fov_y_ = fov_y;
    aspect_ = aspect;
    near_ = near_plane;
    far_ = far_plane;
}

auto CameraComponent::fov_y() const noexcept -> float { return fov_y_; }
auto CameraComponent::aspect() const noexcept -> float { return aspect_; }
auto CameraComponent::near_plane() const noexcept -> float { return near_; }
auto CameraComponent::far_plane() const noexcept -> float { return far_; }

auto CameraComponent::projection_matrix() const -> math::Mat4 {
    return math::Mat4::perspective(fov_y_, aspect_, near_, far_);
}

auto CameraComponent::view_matrix() const -> math::Mat4 {
    auto& t = entity().transform();
    return math::view_matrix(t.position, t.rotation);
}

auto CameraComponent::view_projection_matrix() const -> math::Mat4 {
    return projection_matrix() * view_matrix();
}

void CameraComponent::look_at(math::Vec3 target) {
    auto& t = entity().transform();
    t.rotation = math::look_at_rotation(t.position, target, math::Vec3::unit_y());
}

void CameraComponent::look_at(math::Vec3 eye, math::Vec3 center, math::Vec3 up) {
    auto& t = entity().transform();
    t.position = eye;
    t.rotation = math::look_at_rotation(eye, center, up);
}
```

NOTE: Both `look_at` overloads delegate to `math::look_at_rotation()` which lives in `math/math.h` (the only file permitted to use GLM types per ADR-002/ADR-019). The single-arg `look_at(Vec3 target)` uses `Vec3::unit_y()` as up, replicating the old `Camera::look_at(Vec3 target)` behavior.

The `on_attach()` and destructor remain IDENTICAL to the current implementation (lines 22-34 of the current `camera_component.cpp`).

There must be NO `camera()` accessor method — neither public nor private.

There must be NO `math::Camera camera_` member.

### 3. Free functions in `math/math.h`

Add to `src/engine/math/math.h` at the end of `namespace buddd::engine::math`, after the existing functions but before the closing brace:

```cpp
// -- Look-at rotation (contains GLM-dependent math; must be in math/ per ADR-002/ADR-019) --
/// Computes a rotation quaternion that orients the forward direction
/// (0,0,-1) to look from `eye` toward `center` with the given `up` vector.
inline auto look_at_rotation(const Vec3& eye, const Vec3& center, const Vec3& up) noexcept -> Quat {
    Vec3 forward = (center - eye).normalized();
    Vec3 right = forward.cross(up).normalized();
    Vec3 ortho_up = right.cross(forward);
    glm::mat3 rot_mat(1.0f);
    rot_mat[0] = right.glm();
    rot_mat[1] = ortho_up.glm();
    rot_mat[2] = (-forward).glm();
    return Quat{glm::quat_cast(rot_mat)};
}

// -- View matrix --
inline auto view_matrix(const Vec3& position, const Quat& orientation) noexcept -> Mat4 {
    Vec3 forward = orientation * Vec3(0.0f, 0.0f, -1.0f);
    Vec3 up = orientation * Vec3(0.0f, 1.0f, 0.0f);
    return Mat4::look_at(position, position + forward, up);
}
```

Both are `inline` functions (like all other functions in math.h). `view_matrix()` delegates to `Mat4::look_at` exactly like the old `Camera::view_matrix()` did. `look_at_rotation()` contains the GLM types (`glm::mat3`, `glm::quat_cast`) and is the ONLY place in the codebase where these appear — this satisfies ADR-002/ADR-019.

**Must also remove** `#include "camera.h"` from `math/math.h` line 12.

### 4. Migration of `free_camera_movement.cpp`

Replace every occurrence:

| Before | After |
|---|---|
| Line 37-45: `auto cam_opt = entity().get_component<CameraComponent>();` + `auto& cam = cam_opt->camera();` | Keep first line as-is. Remove `auto& cam = cam_opt->camera();` line. All subsequent uses go through `cam_opt->` directly or through `entity().transform()` |
| Line 72: `cam.set_orientation(...)` | `entity().transform().rotation = ...` |
| Line 76: `cam.orientation()` | `entity().transform().rotation` |
| Line 82: `cam.orientation()` | `entity().transform().rotation` |
| Line 92: `cam.set_position(cam.position() + ...)` | `entity().transform().position = entity().transform().position + ...` |

Remove unused `#include "math/camera.h"` (not present in this file — not needed).

### 5. Migration of `render_system.cpp`

| Before | After |
|---|---|
| Line 38: `auto vp = cam_comp.camera().view_projection_matrix();` | `auto vp = cam_comp.view_projection_matrix();` |
| Line 39: `auto camera_pos = cam_comp.camera().position();` | `auto camera_pos = cam_comp.entity().transform().position;` |

### 6. Migration of `cube_app.h` and `cube_app.cpp`

**cube_app.h changes:**
- Remove `#include "math/camera.h"`
- Add `#include "scene/camera_component.h"`
- Add `#include "scene/entity.h"`
- Change member `buddd::engine::math::Camera camera_;` to `buddd::engine::Entity camera_entity_;`
- Add `#include "scene/world.h"` (for `add_entity`)

**cube_app.cpp changes:**
- Remove `#include "math/camera.h"` (if present)
- In `setup()`:
  - Replace `camera_.look_at(eye, center, up); camera_.set_perspective(...);` with:
    ```cpp
    camera_entity_ = ctx.world.add_entity();
    camera_entity_.transform().position = be::math::Vec3{3.0f, 2.0f, 3.0f};
    auto& cam_comp = camera_entity_.add_component<be::CameraComponent>();
    cam_comp.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(config().width) / static_cast<float>(config().height),
        0.1f, 100.0f
    );
    camera_entity_.transform().rotation = ... /* compute look-at rotation */
    ```
  - The look-at orientation must be computed explicitly: use `cam_comp.look_at(eye, center, up)` which sets both position and rotation.
- In `on_render()`:
  - Replace `camera_.projection_matrix() * camera_.view_matrix()` with:
    ```cpp
    auto& cam_comp = *camera_entity_.get_component<be::CameraComponent>();
    be::math::Mat4 mvp = cam_comp.projection_matrix() * math::view_matrix(
        camera_entity_.transform().position, camera_entity_.transform().rotation
    ) * model_matrix;
    ```
    OR equivalently:
    ```cpp
    auto& cam_comp = *camera_entity_.get_component<be::CameraComponent>();
    be::math::Mat4 mvp = cam_comp.view_projection_matrix() * model_matrix;
    ```
    (The second form is preferred — it matches the old behavior: `projection * view * model`.)

### 7. Migration of `multi_material_app.h` and `multi_material_app.cpp`

Same pattern as cube_app.

**multi_material_app.h changes:**
- Remove `#include "math/camera.h"`
- Add `#include "scene/camera_component.h"`
- Add `#include "scene/entity.h"`
- Change member `engine::math::Camera camera_;` to `engine::Entity camera_entity_;`

**multi_material_app.cpp changes:**
- Same migration pattern as cube_app.cpp:
  - Remove standalone camera setup, replace with entity + CameraComponent
  - In `setup()`: create entity, set Transform position, add CameraComponent, call `set_perspective()`, call `look_at(eye, center, up)`.
  - In `on_render()`: use `cam_comp.view_projection_matrix()` or equivalent.

### 8. Migration of ECS-based app files

These app files use the pattern: `be::math::Camera camera; entity.add_component<be::CameraComponent>(camera);` followed by `->camera().set_position(...)`, `->camera().set_orientation(...)`, `->camera().set_perspective(...)`.

The transformation rules for ALL of these are:

#### Pattern A: `math::Camera camera` constructed standalone, then passed to `add_component`

When constructing a `math::Camera` standalone and passing to `add_component<CameraComponent>(camera)`:

1. Remove `#include "math/camera.h"` from the `.cpp` file.
2. Remove the standalone `math::Camera camera;` variable.
3. Create the entity via `ctx.world.add_entity()` if not already done.
4. Add the CameraComponent via `entity.add_component<CameraComponent>()` (default constructor).
5. Set entity Transform position/rotation directly, e.g., `entity.transform().position = Vec3{...}; entity.transform().rotation = Quat{...};`
6. Set projection via `cam_comp.set_perspective(...)`.
7. For `look_at()` usage, call `cam_comp.look_at(eye, center, up)` which sets both position and rotation.

#### Pattern B: `->camera()` accessor

| Old code | New code |
|---|---|
| `cam_opt->camera().set_position(Vec3(...))` | `entity.transform().position = Vec3(...)` |
| `cam_opt->camera().set_orientation(Quat(...))` | `entity.transform().rotation = Quat(...)` |
| `cam_opt->camera().set_perspective(f,a,n,f2)` | `cam_opt->set_perspective(f,a,n,f2)` |
| `cam_opt->camera().look_at(target)` | `cam_opt->look_at(target)` |
| `cam_opt->camera().fov_y()` | `cam_opt->fov_y()` |
| `cam_opt->camera().position()` | `entity.transform().position` |
| `cam_opt->camera().orientation()` | `entity.transform().rotation` |
| `cam_opt->camera().view_projection_matrix()` | `cam_opt->view_projection_matrix()` |
| `cam_opt->camera().projection_matrix()` | `cam_opt->projection_matrix()` |
| `cam_opt->camera().view_matrix()` | `cam_opt->view_matrix()` |
| `world.active_camera()->camera()` | `*world.active_camera()` (returns `CameraComponent&` directly) |
| `cam_opt->camera().set_position(...)` on a `get_component()` result | `get_component<CameraComponent>()->entity().transform().position = ...` |

#### Pattern C: `CameraComponent(camera)` constructor with `math::Camera` argument

Replace `entity.add_component<CameraComponent>(camera)` where `camera` is a `math::Camera`:
- Remove the `math::Camera camera;` construction.
- Set entity transform position/rotation manually (or use `cam_comp.look_at()`).
- Add CameraComponent with default constructor then call `set_perspective()`.

#### Specific file-by-file migration notes

**`cube_scene_app.cpp`** (lines 31-43):
- Remove `#include "math/camera.h"`
- Remove `be::math::Camera camera;` construction (line 31)
- Replace with: `entity.transform().position = be::math::Vec3{3.0f, 2.0f, 3.0f};` then `auto& cc = entity.add_component<be::CameraComponent>();` then `cc.set_perspective(...)` and `cc.look_at(eye, center, up)`.

**`textured_cube_app.cpp`** (lines 55-67): Same pattern as cube_scene_app.

**`free_camera_app.cpp`** (lines 31-43):
- Remove `#include "math/camera.h"`
- Remove `be::math::Camera camera;` (line 32)
- Remove `auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();` (line 35)
- Replace with:
  ```cpp
  auto& cam_comp = *camera_entity_.get_component<be::CameraComponent>();
  camera_entity_.transform().position = be::math::Vec3{0.0f, 2.0f, 5.0f};
  camera_entity_.transform().rotation = be::math::Quat::from_euler(0.0f, 0.0f, 0.0f);
  cam_comp.set_perspective(be::math::radians(60.0f), ..., 0.1f, 100.0f);
  ```

**`phong_app.cpp`** (lines 207-217):
- Remove `#include "math/camera.h"`
- Remove `be::math::Camera camera;` (line 208)
- Remove `auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();` (line 211)
- Replace with:
  ```cpp
  auto& cam_comp = *camera_entity_.get_component<be::CameraComponent>();
  camera_entity_.transform().position = be::math::Vec3{6.0f, 3.5f, 8.0f};
  camera_entity_.transform().rotation = be::math::Quat::from_euler(be::math::radians(-18.0f), be::math::radians(35.0f), 0.0f);
  cam_comp.set_perspective(be::math::radians(55.0f), ..., 0.1f, 100.0f);
  ```

**`gltf_helmet_app.cpp`** (lines 36-48):
- Remove `#include "math/camera.h"`
- Remove `be::math::Camera camera;` (line 37)
- Remove `auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();` (line 40)
- Replace with:
  ```cpp
  auto& cam_comp = *camera_entity_.get_component<be::CameraComponent>();
  camera_entity_.transform().position = be::math::Vec3{0.0f, 1.5f, 3.0f};
  camera_entity_.transform().rotation = be::math::Quat::from_euler(-0.4636f, 0.0f, 0.0f);
  cam_comp.set_perspective(be::math::radians(55.0f), ..., 0.1f, 100.0f);
  ```

**`gltf_demo_app.cpp`** (lines 42-51):
- Remove `#include "math/camera.h"`
- Remove `be::math::Camera camera;` (line 43)
- Remove `auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();` (line 46)
- Replace with:
  ```cpp
  auto& cam_comp = *camera_entity_.get_component<be::CameraComponent>();
  camera_entity_.transform().position = be::math::Vec3{0.0f, 1.0f, 3.0f};
  camera_entity_.transform().rotation = be::math::Quat::from_euler(0.0f, be::math::radians(180.0f), 0.0f);
  cam_comp.set_perspective(be::math::radians(55.0f), ..., 0.1f, 100.0f);
  ```
- Also lines 82-89 in `on_frame_begin()`:
  ```cpp
  // Old:
  auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();
  float angle = ...;
  cam.set_position(...);
  cam.look_at(...);
  // New:
  auto& cam_comp = *camera_entity_.get_component<be::CameraComponent>();
  float angle = ...;
  camera_entity_.transform().position = be::math::Vec3{...};
  cam_comp.look_at(be::math::Vec3{0.0f, 0.0f, 0.0f});
  ```

**`hot_reload_app.cpp`** (lines 62-73):
- Remove `#include "math/camera.h"`
- Remove `be::math::Camera camera;` construction (line 62)
- Replace with: set `entity.transform().position`, use `look_at()` and `set_perspective()` on the component.

**`hot_reload_gltf_app.cpp`** (lines 60-67):
- Remove `#include "math/camera.h"`
- `cam_entity.add_component<be::CameraComponent>(be::math::Camera{});` → `cam_entity.add_component<be::CameraComponent>();` (default constructor, no argument needed)
- Remove `auto& cam = cam_entity.get_component<be::CameraComponent>()->camera();` (line 62)
- Replace with:
  ```cpp
  auto& cam_comp = *cam_entity.get_component<be::CameraComponent>();
  cam_entity.transform().position = {0.0f, 1.0f, 3.0f};
  cam_comp.look_at({0.0f, 0.0f, 0.0f});
  cam_comp.set_perspective(...);
  ```
- Also lines 132-137 in `on_frame_begin()`:
  ```cpp
  // Old:
  auto& cam = cam_opt->camera();
  cam.set_position(...);
  cam.look_at(...);
  // New:
  cam_opt->look_at(...);  // This sets both position and rotation in one call
  // Actually: need entity from the component. Since active_camera() returns CameraComponent&,
  // we can set its entity's transform position first, then call look_at
  auto& cc = *cam_opt;
  cc.entity().transform().position = {3.0f * std::sin(a), 1.0f, 3.0f * std::cos(a)};
  cc.look_at({0.0f, 0.0f, 0.0f});
  ```

**`asset_demo_app.cpp`** (lines 55-67): Same pattern as cube_scene_app and textured_cube_app.

### 9. Migration of test files

**`tests/scene_rendering_tests.cpp`**:

- Remove `#include "math/camera.h"` (not currently included — verify).
- In the "CameraComponent auto-registers" test (lines 209-232):
  ```cpp
  // Remove:
  math::Camera cam;
  cam.set_perspective(math::radians(90.0f), 1.0f, 0.1f, 50.0f);
  auto& cc = entity.add_component<CameraComponent>(cam);
  // Replace with:
  auto& cc = entity.add_component<CameraComponent>();
  cc.set_perspective(math::radians(90.0f), 1.0f, 0.1f, 50.0f);
  
  // Then replace:
  REQUIRE(cc.camera().fov_y() == Approx(math::radians(90.0f)).margin(TOL));
  cc.camera().set_perspective(math::radians(45.0f), 2.0f, 0.5f, 200.0f);
  REQUIRE(cc.camera().fov_y() == Approx(math::radians(45.0f)).margin(TOL));
  REQUIRE(cc.camera().aspect() == Approx(2.0f).margin(TOL));
  // With:
  REQUIRE(cc.fov_y() == Approx(math::radians(90.0f)).margin(TOL));
  cc.set_perspective(math::radians(45.0f), 2.0f, 0.5f, 200.0f);
  REQUIRE(cc.fov_y() == Approx(math::radians(45.0f)).margin(TOL));
  REQUIRE(cc.aspect() == Approx(2.0f).margin(TOL));
  
  // Remove const accessor test:
  // const auto& ccc = cc;
  // REQUIRE(ccc.camera().fov_y() == ...);
  ```
- In the "CameraComponent destructor unregisters" test (lines 237-248):
  ```cpp
  // Remove: math::Camera cam;
  // Replace: entity.add_component<CameraComponent>(cam);
  // With:    entity.add_component<CameraComponent>();
  ```
- In the "RenderSystem draw call count" test (lines 356-417):
  ```cpp
  // Remove: math::Camera cam; and cam.set_perspective(...);
  // Replace with:
  auto& cam_comp = cam_entity.add_component<CameraComponent>();
  cam_comp.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
  ```
- In the "RenderSystem MVP computation" test (lines 422-519):
  - Same pattern: remove `math::Camera cam;`, replace with `add_component<CameraComponent>()` + `set_perspective()`.
- In the "RenderSystem set_uniform failure skip" test (lines 552-649):
  - Same pattern.
- In the "Multiple camera components on different entities" test (lines 752-773):
  - Already uses default `CameraComponent()` constructor — no change needed.
- In the "Const-correctness of accessors" test (lines 778-791):
  ```cpp
  // Remove:
  // const auto& const_cc = *opt;
  // (void)const_cc.camera(); // const overload
  // This test was checking the const camera() accessor — remove entirely since there is no camera() accessor.
  // Replace with a simpler const-correctness check:
  const auto& const_world = world;
  auto opt = const_world.active_camera();
  REQUIRE(opt.has_value());
  // CameraComponent& obtained from active_camera() is non-const (method is const, but returns non-const ref)
  ```
- **Add new tests** for:
  - `CameraComponent::projection_matrix()` returns correct value.
  - `CameraComponent::view_projection_matrix()` equals `projection * view`.
  - `CameraComponent::look_at(Vec3)` updates entity transform rotation.
  - `CameraComponent::look_at(Vec3, Vec3, Vec3)` updates entity transform position and rotation.
  - `math::view_matrix(position, orientation)` returns correct Mat4.

**`tests/lighting_tests.cpp`**:

Every occurrence of:
```cpp
math::Camera cam;
cam.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
cam_entity.add_component<CameraComponent>(cam);
```
Must become:
```cpp
auto& cam_comp = cam_entity.add_component<CameraComponent>();
cam_comp.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
```

Affected tests (13 occurrences):
- "RenderSystem collects directional lights" (lines 376-378)
- "RenderSystem collects point lights" (lines 426-428)
- "RenderSystem collects spot lights" (lines 461-463)
- "RenderSystem caps at 8 lights" (lines 515-517)
- "Light colour * intensity premultiplied" (lines 556-558)
- "Normal matrix computation" (lines 591-593)
- "Backward compat: unlit material" (lines 647-649)
- "RenderSystem sets u_camera_pos" (lines 685-688) — also replace `cam.set_position(...)` with `cam_entity.transform().position = ...`
- "RenderSystem does not overwrite material properties" (lines 715-717)
- "Light component entity destruction" (lines 763-765)
- "Zero lights renders with ambient only" (lines 806-808)
- "RenderSystem sets u_model" (lines 851-853)
- "Spot light cone uniforms" (lines 1049-1051)

For the "RenderSystem sets u_camera_pos" test specifically (lines 684-688):
```cpp
// Old:
math::Camera cam;
cam.set_position(math::Vec3(10.0f, 5.0f, -3.0f));
cam.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
cam_entity.add_component<CameraComponent>(cam);
// New:
cam_entity.transform().position = math::Vec3(10.0f, 5.0f, -3.0f);
auto& cam_comp = cam_entity.add_component<CameraComponent>();
cam_comp.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
```

### 10. ADR creation

Create `docs/adr/ADR-024-camera-transform-integration.md` (after latest ADR-023). 
Follow the existing ADR template style. Content must document:
- The decision to remove `math::Camera` class entirely
- Rationale: eliminate duplicate position/orientation storage, align with entity Transform pattern (matching how lights work)
- New API: CameraComponent is projection-only, camera position/rotation from Transform
- Free function `math::view_matrix(Vec3, Quat)` for computing view matrices without Camera class
- Deprecation of the old `camera()` accessor pattern
- Migration approach: all call sites moved to new API

### 11. Implementation order

To prevent build breaks, implement in this exact order:

1. **Add free functions + remove camera.h include** in `math/math.h` — add `view_matrix()` and `look_at_rotation()`, AND remove `#include "camera.h"`.
2. **Rewrite `camera_component.h`** — new projection-only header (no `#include "math/camera.h"`).
3. **Rewrite `camera_component.cpp`** — new implementation (no `#include "math/camera.h"`).
4. **Migrate `render_system.cpp`** — simple 2-line change. **Also remove `#include "math/camera.h"` if present.**
5. **Migrate `free_camera_movement.cpp`** — moderate changes. **Also remove `#include "math/camera.h"` if present.**
6. **Migrate app files** — each file must also remove `#include "math/camera.h"` from its includes. In any order:
   - `cube_app.h/.cpp`
   - `multi_material_app.h/.cpp`
   - `cube_scene_app.cpp`
   - `textured_cube_app.cpp`
   - `free_camera_app.cpp`
   - `phong_app.cpp`
   - `gltf_helmet_app.cpp`
   - `gltf_demo_app.cpp`
   - `hot_reload_app.cpp`
   - `hot_reload_gltf_app.cpp`
   - `asset_demo_app.cpp`
7. **Migrate test files** — `scene_rendering_tests.cpp`, `lighting_tests.cpp`. **Each must also remove `#include "math/camera.h"` if present.**
8. **Delete `camera.h` and `camera.cpp`** — only safe after NO remaining `#include "math/camera.h"` exists in any file (verified by `grep -r "math/camera\.h" src/ tests/`).
9. **Create ADR** — `ADR-024-camera-transform-integration.md`
10. **Update wiki pages** — glossary.md, overview.md, business-rules.md, module-map.md

## Required tests

### Unit tests

All existing tests must continue to pass after migration, except:
- Tests explicitly testing `camera()` accessor (e.g., lines 223-231 in scene_rendering_tests.cpp, the const accessor test at lines 778-791) must be updated to use the new API.
- Tests constructing `math::Camera` objects must be updated to construct `CameraComponent` directly.

New unit tests to add in `scene_rendering_tests.cpp`:

| Test name | What it verifies | Related AC |
|---|---|---|
| `CameraComponent projection matrix` | `projection_matrix()` returns correct perspective matrix matching `Mat4::perspective(fov, aspect, near, far)` | AC-007 |
| `CameraComponent view matrix` | `view_matrix()` returns correct matrix using entity transform position/rotation | AC-008 |
| `CameraComponent view_projection matrix` | `view_projection_matrix()` == `projection_matrix() * view_matrix()` | AC-008 |
| `CameraComponent look_at(Vec3)` | `look_at(target)` modifies entity transform rotation only, position unchanged | AC-009 |
| `CameraComponent look_at(eye,center,up)` | `look_at(e,c,u)` modifies both position and rotation | AC-010 |
| `math::view_matrix free function` | `view_matrix(pos, orient)` returns correct lookAt matrix | AC-011 |
| `math::look_at_rotation free function` | `look_at_rotation(eye, center, up)` returns correct quaternion matching GLM `lookAt` orientation | AC-010/ADR-002 |
| `CameraComponent two-arg constructor` | `CameraComponent(fov, aspect, near, far)` stores correct values | AC-012 |

### E2E / Integration verification

1. `make test` or equivalent test runner reports all tests passing.
2. `buddd run cube` renders rotating cube correctly (identical visual output).
3. `buddd run multi-material` renders multi-material cube correctly.
4. `buddd run free-camera` allows WASD + mouse camera control.
5. `buddd run phong` renders 5 cubes with lighting correctly.
6. Compilation verification: all 13 app files, 2 test files, and engine core compile without errors.
7. Static analysis: `git diff` confirms no remaining references to `math::Camera` or `camera()` accessor calls.

## Edge cases

| Case | Expected behavior |
|---|---|
| `CameraComponent` on stack, never attached to entity | Destructor guards null `world_`. `entity()` accessor returns undefined handle before `on_attach()`. Unchanged from current. |
| `look_at()` called before entity attached | Undefined behavior (entity() may return invalid handle). Precondition documented but not enforced. |
| `view_matrix()` / `view_projection_matrix()` called before entity attached | Undefined behavior. No new guards added. |
| `FreeCameraMovement` on entity without CameraComponent | Already handled: logs one-shot warning, becomes no-op. No change. |
| `FreeCameraMovement` on entity with CameraComponent but no Transform | Impossible — every Entity has Transform as intrinsic member. |
| Multiple cameras registered in World | Last-registered wins (unchanged behavior). |
| Camera entity destroyed | CameraComponent destructor unregisters from World (unchanged behavior). |
| `projection_matrix()` called with extreme near/far values | No validation. Delegates to GLM `perspective()` — same as current. |
| `aspect` set to 0 or negative | GLM division by zero / undefined matrix. Not guarded — same as current. |
| `cube_app` without active camera registered to World | `cube_app` does not use `RenderSystem` — computes MVP manually. No dependency on World camera registration. |
| `look_at(Vec3 target)` with target == camera position | Forward direction is zero vector, division by zero in normalization. Same behavior as old `Camera::look_at()`. No validation added. |

## Security impact

None. Camera components are owned by entities within a World. No external input validation changes. No access control changes.

## Data and migration impact

None. No persistent data, schema, or seed data changes. All changes are in-memory API refactoring.

## API compatibility impact

**Breaking changes:**
- `math::Camera` class removed entirely — any external code using `math::Camera` will not compile.
- `CameraComponent::camera()` accessor removed — consumers must call projection/view methods directly on CameraComponent.
- `math/camera.h` header removed — any `#include "math/camera.h"` will not compile.
- `CameraComponent(const math::Camera& camera)` constructor removed — replaced by default constructor + `set_perspective()` or `(fov_y, aspect, near, far)` constructor.

**New API:**
- `CameraComponent(fov_y, aspect, near, far)` constructor.
- `CameraComponent::set_perspective(fov_y, aspect, near, far)`.
- `CameraComponent::fov_y()`, `aspect()`, `near_plane()`, `far_plane()` getters.
- `CameraComponent::projection_matrix()`, `view_matrix()`, `view_projection_matrix()`.
- `CameraComponent::look_at(Vec3)` and `look_at(Vec3, Vec3, Vec3)`.
- `math::view_matrix(Vec3, Quat)` free function.

**Unchanged:**
- `World::active_camera()` returns `std::optional<CameraComponent&>` (unchanged behavior).
- `World::register_camera(CameraComponent&)` / `unregister_camera(CameraComponent&)` — unchanged.
- `CameraComponent::on_attach()` / destructor — unchanged behavior.
- `FreeCameraMovement` update logic — only the API calls changed.

## Documentation impact

- **README**: None.
- **Wiki pages** (all must be updated by the wiki-agent after code changes):
  - `docs/wiki/domain/glossary.md`: Update `Camera` entry (remove `math::Camera` class definition), update `CameraComponent` entry (no longer wraps `math::Camera`, projection-only).
  - `docs/wiki/architecture/overview.md`: Update `camera_component.h` description — no longer wraps `math::Camera`. Remove or update `camera.h`/`camera.cpp` entries.
  - `docs/wiki/domain/business-rules.md`: Update light component accessor pattern section (lines 278-283) to reference new CameraComponent API.
  - `docs/wiki/architecture/module-map.md`: Update math/camera.h entries (lines 68-78) and scene camera_component.h entries (lines 108-132).
- **Other specs**: None.

## ADR impact

A new ADR is required: `docs/adr/ADR-024-camera-transform-integration.md`. This records the architectural decision to remove `math::Camera`, make CameraComponent projection-only, and use entity Transform for camera position/orientation. The ADR should be created after code changes are complete (last step).

No existing ADRs are deprecated.

## Done criteria

- [ ] `src/engine/math/camera.h` deleted — `git diff` shows the file removed
- [ ] `src/engine/math/camera.cpp` deleted — `git diff` shows the file removed
- [ ] `src/engine/math/math.h` no longer includes `"camera.h"` — verified by code inspection
- [ ] `math::view_matrix(Vec3, Quat)` free function exists in `math/math.h` — verified by inspection
- [ ] `math::look_at_rotation(Vec3, Vec3, Vec3)` free function exists in `math/math.h` — verified by inspection
- [ ] `src/engine/scene/camera_component.h` rewritten — no `#include "math/camera.h"`, no `camera()` accessor, no `math::Camera camera_` member, only projection fields
- [ ] `src/engine/scene/camera_component.h` has exact public API: constructors, `set_perspective`, `fov_y()`, `aspect()`, `near_plane()`, `far_plane()`, `projection_matrix()`, `view_matrix()`, `view_projection_matrix()`, `look_at` (2 overloads), `on_attach()`, destructor
- [ ] `src/engine/scene/camera_component.cpp` implements all methods — `on_attach()` and destructor unchanged from original
- [ ] `src/engine/render/render_system.cpp` — lines 38-39 migrated: `cam_comp.view_projection_matrix()` and `cam_comp.entity().transform().position`
- [ ] `src/engine/scene/free_camera_movement.cpp` — all `cam.camera()`, `cam.position()`, `cam.orientation()`, `cam.set_position()`, `cam.set_orientation()` replaced with `entity().transform()`
- [ ] `src/cmd/apps/cube_app.h` — `math::Camera camera_` replaced with `Entity camera_entity_`
- [ ] `src/cmd/apps/cube_app.cpp` — camera created via entity + `CameraComponent`, MVP uses `view_projection_matrix()`
- [ ] `src/cmd/apps/multi_material_app.h` — `math::Camera camera_` replaced with `Entity camera_entity_`
- [ ] `src/cmd/apps/multi_material_app.cpp` — camera created via entity + `CameraComponent`
- [ ] All 9 remaining ECS-based app files migrated — no `camera()` accessor calls remain, no `math::Camera` references
- [ ] `tests/scene_rendering_tests.cpp` — updated: old `camera()` accessor tests removed/replaced, new tests added for new API
- [ ] `tests/lighting_tests.cpp` — all `math::Camera cam;` patterns replaced with `add_component<CameraComponent>()` + `set_perspective()`
- [ ] `docs/adr/ADR-024-camera-transform-integration.md` created
- [ ] No `#include "math/camera.h"` remains in any file — verified by `grep -r "math/camera\.h" src/ tests/`
- [ ] No `camera()` accessor call remains — verified by `grep -rn "\.camera()" src/ tests/` (should return no results for the old pattern; beware false positives for string literals)
- [ ] No `math::Camera` identifier remains — verified by `grep -rn "math::Camera" src/ tests/`
- [ ] All tests pass — `make test` or equivalent reports 100% success
- [ ] All apps compile — `buddd run cube`, `buddd run multi-material`, etc., compile and run without crashes
