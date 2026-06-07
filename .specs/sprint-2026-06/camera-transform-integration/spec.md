# SPEC-NNNN — Camera → Transform Integration

## Problem

The engine currently has a **position/orientation duplication** problem:

1. `math::Camera` stores its own `position_` and `orientation_` fields, duplicating state that already exists in `Entity::transform()` (position + rotation).
2. Every entity already has a `Transform` (position, rotation, scale) as an intrinsic member — but `CameraComponent` wraps `math::Camera` which independently stores position and orientation.
3. Lights correctly use `entity.transform().position` / `entity.transform().rotation` for their positioning, but cameras use the Camera's private fields. This inconsistency forces developers to learn two different patterns for camera vs. light positioning.
4. `CameraComponent` exposes a public `camera()` accessor that returns a `math::Camera&`, forcing verbose `->camera().setXxx()` chaining patterns at every call site.
5. When cameras move or rotate, developers must update BOTH `entity.transform()` (for the scene graph) AND `camera_.set_position()`/`camera_.set_orientation()` — a source of bugs.

## Goals

1. **Eliminate `math::Camera`** entirely — remove the duplicate position/orientation storage.
2. **CameraComponent becomes projection-only**: stores only `fov_y`, `aspect`, `near_plane`, `far_plane`. Position and orientation come from the entity's `Transform`.
3. **Provide a free function** `math::view_matrix(position, orientation)` so any code that needs a view matrix from a transform can compute it without Camera.
4. **Add convenient camera operations** to CameraComponent: `look_at()` variants, `projection_matrix()`, `view_projection_matrix()`.
5. **Remove the `camera()` accessor** — camera users call projection methods directly on CameraComponent.
6. **Migrate all call sites** to use the new API.
7. **Create an ADR** documenting the decision.
8. **Update the wiki** to reflect the new design.

## Non-goals

1. Do NOT add position/orientation to CameraComponent — these belong on Transform.
2. Do NOT change how `World::active_camera()` / `register_camera()` / `unregister_camera()` work — these remain unchanged.
3. Do NOT add view matrix caching or dirty flags — matrices are recomputed on each call (matching current behavior).
4. Do NOT change `FreeCameraMovement` logic — only the API it uses to read/write position/orientation.
5. Do NOT add new features (e.g., orthographic projection, frustum culling) — purely a refactor.
6. Do NOT break the `cube_app` and `multi_material_app` — they must continue to render identically after migration.
7. Do NOT change the `CubeSceneApp` and other existing ECS-based apps unless they use the old `camera()` accessor.

## Actors

| Actor | Description |
|---|---|
| **Developers** | Write engine code using cameras; maintain apps and scenes. |
| **ECS users** | Add CameraComponent to entities, position via Transform. |
| **FreeCameraMovement** | ECS component that reads/writes camera position/orientation. |
| **RenderSystem** | Consumes the active camera for view-projection computation. |
| **Test suite** | Scene rendering tests, lighting tests — validate camera behavior. |

## User-visible behavior

1. No API user can construct a `math::Camera` — the class is removed.
2. `CameraComponent` exposes:
   - `set_perspective(fov_y, aspect, near, far)` — set projection parameters.
   - `fov_y()`, `aspect()`, `near_plane()`, `far_plane()` — projection getters.
   - `projection_matrix()` → `Mat4` — perspective projection from local params.
   - `view_projection_matrix()` → `Mat4` — `projection_matrix() * view_matrix(entity().transform().position, entity().transform().rotation)`.
   - `look_at(target)` — orients entity's Transform rotation to look at target (keeps current position).
   - `look_at(eye, center, up)` — sets entity's Transform position + rotation to look from eye at center.
3. Free function `math::view_matrix(position, orientation)` → `Mat4` lives in `math/` (e.g., in `math.h` or a new `math/view_matrix.h`).
4. The `camera()` accessor is removed — users call projection methods directly on CameraComponent.
5. All position/orientation access goes through `entity().transform().position` / `entity().transform().rotation`.
6. All existing apps render identically after migration.

## User stories

### Story 1 — Camera setup via projection-only component (Priority: P1)

As a developer, I want to create a camera by setting projection parameters directly on CameraComponent, so that position/orientation are naturally handled via Transform.

**Given** a fresh World and entity
**When** I add a CameraComponent to the entity and call `set_perspective(60°, 16:9, 0.1, 100)`
**Then** the component's `fov_y()` returns 60° in radians, `aspect()` returns 16/9, `near_plane()` returns 0.1, `far_plane()` returns 100
**And** `projection_matrix()` returns a valid perspective projection matrix

### Story 2 — Camera positioning via Transform (Priority: P1)

As a developer, I want to position a camera by setting the entity's transform, so that camera positioning follows the same pattern as lights.

**Given** a camera entity with CameraComponent at position (0, 0, 0) and identity rotation
**When** I set `entity.transform().position = (3, 2, 3)` and `entity.transform().rotation = some_rotation`
**Then** `view_projection_matrix()` returns a correct view-projection computed from the entity's transform position and rotation

### Story 3 — FreeCameraMovement uses Transform (Priority: P2)

As a developer, I want FreeCameraMovement to read/write camera position/orientation through the entity's Transform, so that movement works without a Camera accessor.

**Given** an entity with CameraComponent and FreeCameraMovement
**When** FreeCameraMovement runs its update
**Then** it reads rotation from `entity().transform().rotation` and writes position to `entity().transform().position`

### Story 4 — RenderSystem consumes new API (Priority: P1)

As a developer, I want RenderSystem to use the new CameraComponent API so it continues to render correctly.

**Given** a World with an active camera entity that has CameraComponent
**When** RenderSystem::render_scene() runs
**Then** `view_projection_matrix()` is called on the CameraComponent (not on a nested Camera)
**And** camera position is read from `cam_comp.entity().transform().position`

### Story 5 — Look-at convenience (Priority: P2)

As a developer, I want to orient a camera to look at a target using the component's `look_at()` method.

**Given** a camera entity at position (3, 2, 3)
**When** I call `cam_comp.look_at(Vec3(0, 0, 0))`
**Then** the entity's transform rotation is updated so the camera points at the origin
**And** the view matrix computed from `view_projection_matrix()` looks correct

### Story 6 — Legacy apps continue to work (Priority: P1)

As a user, CubeApp and MultiMaterialApp render identically after migration.

**Given** CubeApp uses a camera entity with CameraComponent instead of `math::Camera camera_` member
**When** the app runs
**Then** the 120-frame rotating cube renders identically to before (same MVP computation)

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `math::Camera` class is removed (header and source deleted or made empty stub) | No `#include "math/camera.h"` compiles; `math::Camera` identifier not defined |
| AC-002 | `CameraComponent` no longer includes `math/camera.h` | `camera_component.h` has no `#include "math/camera.h"` |
| AC-003 | `CameraComponent` has no `camera()` accessor | `cc.camera()` fails to compile |
| AC-004 | `CameraComponent` stores only `fov_y_`, `aspect_`, `near_`, `far_` as private members | Header inspection; no `position_` or `orientation_` members |
| AC-005 | `CameraComponent` provides `set_perspective(fov_y, aspect, near, far)` | Test calls method and verifies getters |
| AC-006 | `CameraComponent` provides getters: `fov_y()`, `aspect()`, `near_plane()`, `far_plane()` | Test calls each getter and verifies stored values |
| AC-007 | `CameraComponent::projection_matrix()` returns correct Mat4 | Test computes expected perspective and compares with result |
| AC-008 | `CameraComponent::view_projection_matrix()` returns correct Mat4 | Test verifies result equals `projection_matrix() * view_matrix(position, rotation)` |
| AC-009 | `CameraComponent::look_at(target)` updates entity transform rotation | Test creates entity at position P, calls `look_at(target)`, verifies rotation |
| AC-010 | `CameraComponent::look_at(eye, center, up)` updates entity transform position and rotation | Test calls and verifies both position and rotation are updated |
| AC-011 | Free function `math::view_matrix(position, orientation)` exists and returns correct Mat4 | Unit test computes lookAt matrix and compares with expected values |
| AC-012 | `CameraComponent` constructor accepts `(fov_y, aspect, near, far)` or default | Test instantiates with both forms |
| AC-013 | `CameraComponent` auto-registers with World via `on_attach()` (unchanged behavior) | Existing test passes unchanged |
| AC-014 | `CameraComponent` destructor unregisters from World (guard for null world) | Existing test passes unchanged |
| AC-015 | `FreeCameraMovement` reads/writes position and orientation via `entity().transform()` | Inspect code: no reference to `camera()` accessor; all position/orientation access via entity transform |
| AC-016 | `RenderSystem` uses `cam_comp.view_projection_matrix()` instead of `cam_comp.camera().view_projection_matrix()` | Inspect code: the `camera()` call is gone |
| AC-017 | `RenderSystem` reads camera position from `cam_comp.entity().transform().position` | Inspect code |
| AC-018 | `cube_app` compiles and renders identically (uses CameraComponent with entity instead of `math::Camera camera_` member) | App runs without crash, renders rotating cube |
| AC-019 | `multi_material_app` compiles and renders identically (uses CameraComponent with entity instead of `math::Camera camera_` member) | App runs without crash, renders multi-material cube |
| AC-020 | All app files that used `->camera().set_position()` now use `entity().transform().position = ...` | Compile check — no `camera()` accessor calls remain |
| AC-021 | All app files that used `->camera().set_orientation()` now use `entity().transform().rotation = ...` | Compile check — no `set_orientation` calls remain |
| AC-022 | All app files that used `->camera().set_perspective()` now use `component.set_perspective()` | Compile check — no `cam.set_perspective()` on a nested Camera object (direct call on component is fine) |
| AC-023 | All app files that used `->camera().look_at()` now use `component.look_at()` | Compile check |
| AC-024 | All app files that used `->camera().position()` now use `entity().transform().position` | Compile check |
| AC-025 | All app files that used `->camera().orientation()` now use `entity().transform().rotation` | Compile check |
| AC-026 | CubeApp's `camera_` member type changes from `math::Camera` to a camera entity handle | Header inspection |
| AC-027 | MultiMaterialApp's `camera_` member type changes from `math::Camera` to a camera entity handle | Header inspection |
| AC-028 | `math/camera.h` and `math/camera.cpp` are removed from build (or marked stale) | CMake build succeeds; no link errors |
| AC-029 | The free function `math::view_matrix()` does not require a `Camera` object | Function signature uses `Vec3` + `Quat`, compiles standalone |
| AC-030 | All existing tests pass after migration | `make test` or equivalent test runner reports success |

## E2E Verification

- **Method**: Automated test suite (`buddd_tests`) + manual verification that `buddd run cube`, `buddd run multi-material`, `buddd run free-camera`, and `buddd run phong` still render correctly (e.g., via `--capture` and compare with known-good screenshots, or visual inspection).
- **Compilation verification**: All 13 app files and both test files compile without errors.
- **Static analysis**: `git diff` confirms no remaining references to `math::Camera` or `camera()` accessor.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | `math::Camera` class removed: no `#include "math/camera.h"` remains anywhere in the codebase. |
| SC-002 | Zero remaining `camera()` accessor calls across all source and test files. |
| SC-003 | Zero remaining `set_position()`/`set_orientation()` calls on Camera objects. |
| SC-004 | All 13 app files, 2 test translation units, engine core, and build system compile cleanly. |
| SC-005 | All existing tests pass without modification (except those explicitly testing the old Camera API). |

## Edge cases

| Case | Expected behavior |
|---|---|
| CameraComponent on stack, never attached to entity | Destructor guards null `world_` (unchanged from current behavior). `entity()` accessor returns undefined Entity handle — caller must not use it before attachment. |
| `look_at()` called before entity is attached | Behavior is undefined (entity() may return invalid handle). Documented as precondition. |
| `view_projection_matrix()` called before entity attached | Behavior is undefined because `entity().transform()` requires a valid entity. No new guards added. |
| `FreeCameraMovement` on entity without CameraComponent | Already handled: logs a one-shot warning and becomes no-op. No change. |
| `FreeCameraMovement` on entity with CameraComponent but no Transform | Impossible — every Entity has a Transform as an intrinsic member. |
| Multiple cameras registered in World | Last-registered wins (unchanged behavior). |
| Camera entity destroyed | CameraComponent destructor unregisters from World (unchanged behavior). |
| `projection_matrix()` called with extreme values (very large/small near/far) | No validation. Same as current behavior — delegates to GLM `perspective()`. |
| `aspect` set to 0 or negative | GLM behavior — division by zero or undefined matrix. Not guarded (unchanged from current). |

## Error cases

| Case | Response |
|---|---|
| `CameraComponent` used without entity attached | Undefined behavior (same as current). No error checking added. |
| `math::view_matrix()` called with zero-length forward direction | Returns an undefined matrix (delegates to Mat4::look_at internal behavior). No validation. |

## Permissions and security

No changes. CameraComponents are owned by entities within a World. No access control changes.

## Observability

- `CameraComponent::on_attach()` logs `"CameraComponent: registered entity N as active camera"` (unchanged).
- `CameraComponent::~CameraComponent()` logs `"CameraComponent: unregistered entity N"` (unchanged).
- `FreeCameraMovement` warns once if entity has no `CameraComponent` (unchanged).
- `RenderSystem` warns if no active camera (unchanged).

## Out of scope

| Item | Rationale |
|---|---|
| Orthographic projection | Not requested; only perspective is needed. |
| View matrix caching / dirty flags | Would add complexity. Matrices recomputed per call (matching current behavior). |
| Frustum culling | A separate feature. |
| Changing `World::register_camera()` / `unregister_camera()` | These work correctly with `CameraComponent&`. No change needed. |
| Migrating `triangle_app` | Triangle app does not use a camera at all. |
| Migrating `run_app` | Empty window, no camera. |
| Adding new apps or scenes | Pure refactor of existing functionality. |

## Assumptions

1. `math::view_matrix()` can be implemented as either a standalone function in `math/view_matrix.h` or added to `math/math.h`. Either location is acceptable.
2. The `look_at` implementation in CameraComponent will delegate internally to the same math as the old `Camera::look_at()`, but write to `entity().transform().position` and `entity().transform().rotation` instead of private members.
3. The `cube_app` and `multi_material_app` will gain an `Entity` member to hold the camera entity, replacing the `math::Camera camera_` member. They will create this entity during `setup()` via `ctx.world.add_entity()`.
4. ADR will be created in `docs/adr/` with a sequential number after the latest existing ADR.
5. The `sqrt` in Camera::look_at.cpp's `normalized()` call is acceptable performance-wise — no optimization needed.
6. `CubeApp` and `MultiMaterialApp` compute their own MVP manually (they do not use `RenderSystem`) — after migration they will use `CameraComponent` via an entity but still compute MVP manually.

## Open questions

None. All decisions were clarified during the grill-me session.

## Key entities

| Entity | Description |
|---|---|
| `Transform` | Intrinsic member of every Entity — stores `position`, `rotation`, `scale`. |
| `CameraComponent` | ECS component — stores projection parameters (`fov_y`, `aspect`, `near`, `far`). Provides `look_at()`, `projection_matrix()`, `view_projection_matrix()`. Reads position/rotation from owning entity's Transform. |
| `math::view_matrix()` | Free function — computes a lookAt view matrix from a `Vec3` position and `Quat` orientation. Replacement for `Camera::view_matrix()`. |

## Impact analysis

### Files to delete
| File | Reason |
|---|---|
| `src/engine/math/camera.h` | Entire class removed |
| `src/engine/math/camera.cpp` | Entire implementation removed |

### Files to create
| File | Content |
|---|---|
| `docs/adr/ADR-NNNN-camera-transform-integration.md` | Architectural decision record |

### Files to modify

#### Engine core
| File | Changes |
|---|---|
| `src/engine/scene/camera_component.h` | Remove `#include "math/camera.h"`. Remove `camera()` accessors. Remove `math::Camera camera_` member. Add private `fov_y_`, `aspect_`, `near_`, `far_` members. Add `set_perspective()`, getters, `look_at()` (2 overloads), `projection_matrix()`, `view_projection_matrix()`. Add default constructor + `(fov_y, aspect, near, far)` constructor. |
| `src/engine/scene/camera_component.cpp` | Remove `Camera(camera)` constructor and `camera()` accessors. Implement projection/look-at methods. Keep `on_attach()` and destructor unchanged. |
| `src/engine/math/math.h` or new `math/view_matrix.h` | Add `auto view_matrix(Vec3 position, Quat orientation) -> Mat4` free function. |

#### Scene module
| File | Changes |
|---|---|
| `src/engine/scene/free_camera_movement.cpp` | Line 45: `auto& cam = cam_opt->camera()` → remove. Line 72: `cam.set_orientation(...)` → `entity().transform().rotation = ...`. Line 76: `cam.orientation()` → `entity().transform().rotation`. Line 82: `cam.orientation()` → `entity().transform().rotation`. Line 92: `cam.set_position(cam.position() + ...)` → `entity().transform().position = entity().transform().position + ...`. Remove unused `#include "math/camera.h"`. |

#### Render module
| File | Changes |
|---|---|
| `src/engine/render/render_system.cpp` | Line 38: `cam_comp.camera().view_projection_matrix()` → `cam_comp.view_projection_matrix()`. Line 39: `cam_comp.camera().position()` → `cam_comp.entity().transform().position`. |

#### App files
| File | Changes |
|---|---|
| `src/cmd/apps/cube_app.h` | Replace `math::Camera camera_` with a camera entity handle. |
| `src/cmd/apps/cube_app.cpp` | Create camera entity in `setup()`, set its Transform position/rotation and CameraComponent perspective. Compute MVP from `entity.transform()` + `component.projection_matrix()`. |
| `src/cmd/apps/multi_material_app.h` | Replace `math::Camera camera_` with a camera entity handle. |
| `src/cmd/apps/multi_material_app.cpp` | Same pattern as cube_app. |
| `src/cmd/apps/cube_scene_app.cpp` | Remove `#include "math/camera.h"`. Construct CameraComponent inline with `set_perspective()` and `look_at()` on the component instead of constructing `math::Camera`. |
| `src/cmd/apps/textured_cube_app.cpp` | Same as cube_scene_app. |
| `src/cmd/apps/free_camera_app.cpp` | Replace `camera()` accessor chain with direct CameraComponent calls and entity Transform access. |
| `src/cmd/apps/phong_app.cpp` | Same pattern. |
| `src/cmd/apps/gltf_helmet_app.cpp` | Same pattern. |
| `src/cmd/apps/gltf_demo_app.cpp` | Same pattern. |
| `src/cmd/apps/hot_reload_app.cpp` | Same pattern. |
| `src/cmd/apps/hot_reload_gltf_app.cpp` | Same pattern. |
| `src/cmd/apps/asset_demo_app.cpp` | Same pattern. |

#### Test files
| File | Changes |
|---|---|
| `tests/scene_rendering_tests.cpp` | Update tests that use `CameraComponent(camera)` constructor → use default or new constructor. Replace `cc.camera().fov_y()` → `cc.fov_y()`. Replace `cc.camera().set_perspective()` → `cc.set_perspective()`. Remove `ccc.camera()` const accessor test. |
| `tests/lighting_tests.cpp` | Update `CameraComponent(cam)` construction → use `CameraComponent()` with explicit `set_perspective()`. Remove all `camera()` accessor calls. |

#### Wiki files
| File | Changes |
|---|---|
| `docs/wiki/domain/business-rules.md` | Section "Light component accessor pattern" (line 278-283) references CameraComponent pattern — update to reflect new API. |
| `docs/wiki/architecture/module-map.md` | Update `math/camera.h` entry (lines 68-78). Update scene submodule `camera_component.h` entries (lines 108-132). Remove or update `Camera` references. |
| `docs/adr/ADR-NNNN-camera-transform-integration.md` | New ADR documenting the decision to remove `math::Camera` and integrate camera position with Transform. |
