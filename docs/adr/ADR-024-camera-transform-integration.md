# ADR-024-camera-transform-integration — Camera → Transform Integration

## Status

Accepted

## Context

The `math::Camera` class (`src/engine/math/camera.h/.cpp`) stored duplicate position and orientation data that was already available in the entity's `Transform` component (`Transform::position`, `Transform::rotation`). Every `CameraComponent` (an ECS component inheriting `Component`) wrapped a `math::Camera` instance, exposing a `camera()` accessor that provided direct access to `math::Camera`'s position/orientation/projection methods.

This design had several problems:

1. **Duplicate storage** — Camera position/orientation existed in two places: the entity's `Transform` and the `math::Camera` inside `CameraComponent`. Keeping them in sync required manual coordination.
2. **Misaligned with entity pattern** — Light components (`DirectionalLightComponent`, `PointLightComponent`, `SpotLightComponent`) already used entity Transform for position/orientation. The camera was the only component that maintained its own copy, creating an inconsistency.
3. **Redundant indirection** — Most call sites accessed camera position/orientation through `cam_comp.camera().position()` instead of `entity().transform().position`, adding unnecessary indirection.
4. **Unnecessary math module type** — `math::Camera` was the only type in the math module with a `.cpp` file, and it combined pure math functions (matrix computation) with mutable state (position, orientation, projection parameters).

## Decision

1. **Remove `math::Camera` class entirely** — Delete `src/engine/math/camera.h` and `src/engine/math/camera.cpp`.

2. **Make `CameraComponent` projection-only** — The component now stores only the four projection parameters (`fov_y`, `aspect`, `near_plane`, `far_plane`). Camera position and orientation come exclusively from `entity().transform()`.

3. **Add free functions in `math/math.h`**:
   - `math::view_matrix(Vec3 position, Quat orientation) -> Mat4` — computes a view matrix from a position and orientation quaternion, delegating to `Mat4::look_at`.
   - `math::look_at_rotation(Vec3 eye, Vec3 center, Vec3 up) -> Quat` — computes a rotation quaternion that orients `(0,0,-1)` to look from `eye` toward `center`, using GLM types internally (satisfying ADR-002/ADR-019).

4. **Update `CameraComponent` API** — Remove the `camera()` accessor. Add projection-only constructors, `set_perspective()`, `fov_y()`, `aspect()`, `near_plane()`, `far_plane()`, `projection_matrix()`, `view_matrix()`, `view_projection_matrix()`, and two `look_at()` overloads. The lifecycle (`on_attach()` / destructor) remains unchanged.

## Consequences

### Positive

- **Eliminates duplicate storage** — Camera position/orientation now lives in exactly one place: the entity's `Transform`. No manual synchronization needed.
- **Consistent with light components** — All components that need position/orientation read from `entity().transform()`, following the same pattern.
- **Simpler math module** — All math types in `src/engine/math/` are now pure computation with no mutable state. The `.cpp` file count in the math module decreases by 2.
- **Smaller API surface** — One fewer class to learn. `CameraComponent` is now self-contained with no delegation to an internal object.

### Negative

- **Breaking change for external code** — Any code using `math::Camera`, `#include "math/camera.h"`, or `CameraComponent::camera()` will not compile. All 13 app files, 2 test files, and 2 engine files needed migration.
- **`look_at(eye, center, up)` now modifies Transform directly** — Previously it modified the internal `math::Camera`, which could be in a different location than the entity's Transform. Now it always modifies the entity's Transform, which is the intended behavior.

### Migration

All call sites were migrated:
- `cam_comp.camera().view_projection_matrix()` → `cam_comp.view_projection_matrix()`
- `cam_comp.camera().position()` → `cam_comp.entity().transform().position`
- `cam_comp.camera().set_position(vec)` → `entity().transform().position = vec`
- `cam_comp.camera().look_at(...)` → `cam_comp.look_at(...)`
- `math::Camera` standalone construction → entity + CameraComponent + Transform

See the implementation contract (IMPL-2026-06-001) for the full migration table.

## Related

- **ADR-002** (glm-wrapper-math): The `view_matrix()` free function delegates to `Mat4::look_at` (which wraps `glm::lookAt`), and `look_at_rotation()` uses GLM types internally.
- **ADR-019** (architecture-boundaries): The free functions live in `math/math.h`, which is the only file permitted to use GLM types directly.
- **ADR-005** (optional-ref-component-api): `CameraComponent` accesses `entity().transform()` after `on_attach()`.
- **Deprecates**: The old `camera()` accessor pattern on `CameraComponent`.
