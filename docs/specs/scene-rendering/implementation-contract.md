# IMPL-011 — Scene-Based Rendering: CameraComponent, MeshRenderer, RenderSystem

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

| | |
|---|---|
| Approved by | guillaume (human) |
| Date | 2026-05-30 |
| Time | After spec-critic + contract-critic reviews, all issues resolved. Spec and contract both Accepted. |

## Source spec

- `docs/specs/scene-rendering/spec.md` (SPEC-011), accepted.
- `docs/specs/scene-rendering/spec-critic.md` — re-review verdict: `Accepted with warnings`. All 8 blocking issues resolved. Non-blocking concerns W-08 (headless uniform query), W-09 (headless counters), and W-10 (AC-023 escape hatch) are addressed in this contract.

## Goal

Bridge the scene graph (`src/engine/scene/`) and render pipeline (`src/engine/render/`) by: (1) making every `Component` entity-aware with `world_`/`entity_id_` members, an `entity()` accessor, and a virtual `on_attach()` lifecycle hook; (2) adding `World::each<T>()` for type-based entity iteration with early-exit support; (3) adding camera registration API to `World` that stores a `CameraComponent&` directly (not an `Entity`); (4) creating `CameraComponent` (ECS wrapper for `math::Camera`) that auto-registers/unregisters with `World`; (5) creating `MeshRenderer` (ECS component holding `shared_ptr<Model>`); (6) creating `RenderSystem` that orchestrates rendering from a `World` each frame; (7) creating a new cube-scene demo (`cube_scene_demo.*`) that uses `World + RenderSystem` instead of manual camera/MVP/draw (existing `cube_demo.*` is left untouched).

## Non-goals

- No camera system (follow, look-at-target, orbit behaviors).
- No multi-camera support — single active camera, last-registered-wins for v1.
- No frustum culling — all MeshRenderer entities are drawn regardless of visibility.
- No render graph, render passes, or post-processing.
- No material instancing or batching — one draw call per MeshRenderer.
- No component pooling or archetype ECS.
- No `on_detach()` lifecycle hook — only `on_attach()` and destructor for cleanup.
- No component enable/disable state.
- No serialization of camera or mesh renderer state.
- No lighting or shadow components.
- No entity tag or layer filtering for rendering.
- No render order sorting (opaque only, draws in iteration order).
- No const-qualified `each<T>()` overload (deferred).
- No changes to build system files (CMake `file(GLOB_RECURSE)` auto-discovers new files).
- No modifications to `src/engine/scene/world.cpp` — all new World methods are inlined in the header (templates are header-only; camera registration methods are trivially small).
- No modifications to `src/engine/scene/entity.cpp`.
- No modifications to `src/cmd/demo/cube_demo.cpp` or `src/cmd/demo/cube_demo.h` — they stay exactly as-is.

## Relevant constitution rules

- **CONST-001** (Architecture Boundaries): `CameraComponent` in `scene/` depends only on `math/` and `scene/`. `MeshRenderer` and `RenderSystem` in `render/` may depend on `scene/` for `Component` and `World`, but must NOT leak backend-specific types (OpenGL, SDL3, GLM) in their public headers. Forward declarations used where possible. `render_system.h` forward-declares `RenderDevice` and `World`.
- **CONST-002** (Testing Policy): All acceptance criteria must have corresponding unit tests in `tests/scene_rendering_tests.cpp`. Tests must pass.

## Relevant ADRs

- **ADR-001** (`docs/adr/001-result-error-pattern.md`): `set_uniform()` returns `Result<void>`. `draw()` returns `void` per ADR-003 exception. `CameraComponent` constructor and `on_attach()` are infallible. `World::each<T>()` returns `size_t`.
- **ADR-003** (`docs/adr/003-render-pipeline-architecture.md`): `RenderDevice::draw()` and `draw_indexed()` return `void` — precondition violations are UB. `RenderSystem::render()` must NOT attempt error checking on draw calls.
- **ADR-005** (`docs/adr/005-optional-ref-component-api.md`): `get_component<CameraComponent>()` returns `std::optional<CameraComponent&>`. `World::active_camera()` follows the same pattern with `std::optional<CameraComponent&>`, consistent with ADR-010's mandate to use `std::optional<T&>` over raw pointers.
- **ADR-006** (`docs/adr/006-rtti-component-dispatch.md`): `World::each<T>()` must use `dynamic_cast` for type matching, consistent with existing `get_component<T>()` and `remove_component<T>()`.

## Files to inspect

The Code Agent must read these files before editing:

| File | Purpose |
|---|---|
| `src/engine/scene/component.h` | Current `Component` base class — must be modified. |
| `src/engine/scene/entity.h` | Current `Entity` class — must add `friend class Component;` and `Component::entity()` definition. Contains `Transform::world_matrix()` inline definition pattern. |
| `src/engine/scene/world.h` | Current `World` class — must add `each<T>()`, camera API using `CameraComponent&` references, forward-declare `CameraComponent`, include `<optional>`. |
| `src/engine/scene/entity_id.h` | `EntityId` struct — verify `EntityId::none()` sentinel value. |
| `src/engine/render/render_device_headless.h` | Headless device — must add counter members and public accessors. |
| `src/engine/render/render_device_headless.cpp` | `begin_frame()`/`end_frame()` must increment counters. |
| `src/engine/render/material_headless.h` | Headless material — must add `get_uniform_mat4()` public accessor. |
| `src/engine/render/material_headless.cpp` | Headless material implementation reference. |
| `src/engine/render/model.h` | `Model` class — confirms `draw(RenderDevice&)` and `material()` signatures. |
| `src/engine/render/render_device.h` | `RenderDevice` abstract base — confirms `begin_frame()`, `end_frame()`, `draw()` signatures. |
| `src/engine/render/material.h` | `Material` base class — confirms `set_uniform(string_view, Mat4)` returns `Result<void>`. |
| `src/engine/error.h` | `Error`, `Result<T>`, `make_error()`, `to_string()` definitions. |
| `src/engine/math/camera.h` | `math::Camera` — confirms `view_projection_matrix()` returns `math::Mat4`. |
| `src/cmd/demo/cube_demo.h` | Existing cube demo header — must NOT be modified but inspected for the `run_cube_demo` signature. |
| `src/cmd/demo/cube_demo.cpp` | Existing cube demo implementation — must NOT be modified but inspected as a pattern for the new `cube_scene_demo`. |
| `src/cmd/demo/demo_helpers.h` | `setup_cube()` signature and `CubeResources` struct. |
| `src/cmd/demo/demo_helpers.cpp` | `setup_cube()` implementation — confirms `CubeResources{shared_ptr<Material>, Model}` and move semantics. |
| `tests/scene_graph_tests.cpp` | Style reference: Catch2 v3, anonymous namespace helpers, `[tag]` conventions, `Approx`/`.margin()`. |

## Files allowed to change

### New files to create (9 files)

| # | File | Purpose |
|---|---|---|
| 1 | `src/engine/scene/camera_component.h` | `CameraComponent` class declaration. |
| 2 | `src/engine/scene/camera_component.cpp` | `CameraComponent` implementation (constructor, `on_attach()`, destructor). |
| 3 | `src/engine/render/mesh_renderer.h` | `MeshRenderer` class declaration. |
| 4 | `src/engine/render/mesh_renderer.cpp` | `MeshRenderer` implementation (constructor, accessors). |
| 5 | `src/engine/render/render_system.h` | `RenderSystem` class declaration. |
| 6 | `src/engine/render/render_system.cpp` | `RenderSystem::render()` implementation. |
| 7 | `tests/scene_rendering_tests.cpp` | All AC-001 through AC-030 tests. |
| 8 | `src/cmd/demo/cube_scene_demo.h` | New demo header declaring `run_cube_scene_demo`. |
| 9 | `src/cmd/demo/cube_scene_demo.cpp` | New demo implementation using World + RenderSystem. |

### Modified files (6 files)

| # | File | Change |
|---|---|---|
| 1 | `src/engine/scene/component.h` | Add `world_`, `entity_id_` members (protected), `entity()` accessor (declaration), `on_attach()` virtual hook, `friend class World;`. |
| 2 | `src/engine/scene/entity.h` | Add `friend class Component;`. Add `Component::entity()` inline definition after `Entity` is fully defined (after `Transform::world_matrix()` block at end of file). |
| 3 | `src/engine/scene/world.h` | Add `each<T>()` template method. Add camera registration API (`register_camera`, `unregister_camera`, `active_camera`). Add `active_camera_` member (`std::optional<CameraComponent&>`). Modify `add_component<T>()` to set `world_`/`entity_id_` and call `on_attach()`. Add forward declaration of `CameraComponent`. Add `#include <optional>` (for `std::optional<CameraComponent&>`). Add `#include <type_traits>` (needed for `std::is_base_of_v` in `each<T>()`). |
| 4 | `src/engine/render/render_device_headless.h` | Add `frame_begin_count_`, `frame_end_count_` private members. Add public accessors `frame_begin_count()`, `frame_end_count()`, `draw_call_count()`. |
| 5 | `src/engine/render/render_device_headless.cpp` | Increment `frame_begin_count_` in `begin_frame()`, `frame_end_count_` in `end_frame()`. |
| 6 | `src/engine/render/material_headless.h` | Add `get_uniform_mat4(std::string_view name) -> std::optional<math::Mat4>` public accessor (const). |

## Files forbidden to change

- `src/engine/scene/entity_id.h` — No changes needed.
- `src/engine/scene/transform.h` — No changes needed.
- `src/engine/scene/world.cpp` — All new World methods are inline in header.
- `src/engine/scene/entity.cpp` — No changes needed.
- `src/engine/math/camera.h` — Consumed but not modified.
- `src/engine/error.h` — Error types already sufficient.
- `src/engine/render/model.h` and `model.cpp` — No changes needed.
- `src/engine/render/material.h` — Abstract interface unchanged.
- `src/engine/render/render_device.h` — Abstract interface unchanged.
- `src/engine/render/render_device_opengl.*` — No changes needed.
- `src/engine/render/material_opengl.*` — No changes needed.
- `src/engine/render/shader*.*` — No changes needed.
- `src/engine/render/index_buffer*.*` — No changes needed.
- `src/engine/render/vertex_buffer*.*` — No changes needed.
- `src/cmd/demo/demo_helpers.*` — No changes needed.
- `src/cmd/demo/triangle_demo.*` — No changes needed.
- `src/cmd/demo/cube_demo.*` — Must NOT be modified; left exactly as-is.
- `tests/CMakeLists.txt` — Not touched; `file(GLOB)` auto-discovers new test files.
- Any other file not listed in "Files allowed to change".

## Existing conventions to follow

1. **Include guard**: `#pragma once` for all new headers.
2. **Namespace**: `namespace buddd::engine { ... }` with `} // namespace buddd::engine` closing comment.
3. **Trailing return types**: `auto method() -> Type` syntax.
4. **`noexcept` usage**: Mark `noexcept` on accessors (`entity()`, `camera()`, `model()`, `active_camera()`, counter accessors). Do NOT mark `noexcept` on `on_attach()`, `render()`, `register_camera()`, `unregister_camera()` (these may log, allocate, or invoke user code).
5. **`const` correctness**: Provide const and non-const overloads for reference accessors (`camera()`, `model()`).
6. **Include paths**: Relative to `src/engine/` — e.g., `"scene/component.h"`, `"render/model.h"`, `"math/camera.h"`.
7. **Test style**: Catch2 v3 `TEST_CASE` with `[scene_rendering]` tag, anonymous namespace for helper types, `REQUIRE`/`REQUIRE_FALSE`, `Catch::Approx`/`.margin(TOL)` for float comparisons.
8. **Observability**: `std::cerr` for warnings, consistent with `render_device_headless.cpp` pattern.
9. **`Entity(World&, EntityId)` constructor**: Already private with `friend class World;`. Adding `friend class Component;` allows `Component::entity()` to construct Entity handles.
10. **`using namespace`**: Not used in headers. `using namespace buddd::engine;` is acceptable in `.cpp` files (existing pattern in tests).
11. **Tolerance**: `constexpr float TOL = 1e-5f;` for float comparisons.

## Required implementation behavior

### 0. Spec-critic resolutions (W-08, W-09, W-10)

- **W-08 (uniform query)**: `MaterialHeadless` gains `get_uniform_mat4(std::string_view name) const -> std::optional<math::Mat4>` — see section 2 below.
- **W-09 (headless counters)**: `RenderDeviceHeadless` gains `frame_begin_count_`, `frame_end_count_` members, public accessors, and counter increments — see section 1 below.
- **W-10 (AC-023 escape hatch)**: AC-023's verification is `headless_device.frame_begin_count() == 1`. No "(or verify no crash)" fallback.

### 1. RenderDeviceHeadless counter extensions

**File**: `src/engine/render/render_device_headless.h`

Add private members:
```cpp
int frame_begin_count_{0};
int frame_end_count_{0};
```

Add public accessors (inline, in the class body, after existing public methods):
```cpp
auto frame_begin_count() const noexcept -> int { return frame_begin_count_; }
auto frame_end_count() const noexcept -> int { return frame_end_count_; }
auto draw_call_count() const noexcept -> int { return draw_call_count_; }
```

Note: `draw_call_count_` already exists as a private member. The accessor makes it publicly readable.

**File**: `src/engine/render/render_device_headless.cpp`

Modify `begin_frame()`:
```cpp
auto RenderDeviceHeadless::begin_frame() -> void {
    ++frame_begin_count_;
}
```

Modify `end_frame()`:
```cpp
auto RenderDeviceHeadless::end_frame() -> void {
    ++frame_end_count_;
}
```

### 2. MaterialHeadless uniform query

**File**: `src/engine/render/material_headless.h`

Add public accessor:
```cpp
/// Returns the last-set Mat4 value for the given uniform name, or
/// std::nullopt if the uniform has not been set or is not of Mat4 type.
auto get_uniform_mat4(std::string_view name) const -> std::optional<math::Mat4>;
```

**File**: `src/engine/render/material_headless.cpp`

Implementation:
```cpp
auto MaterialHeadless::get_uniform_mat4(std::string_view name) const -> std::optional<math::Mat4> {
    auto it = uniform_values_.find(std::string(name));
    if (it == uniform_values_.end()) {
        return std::nullopt;
    }
    if (!std::holds_alternative<math::Mat4>(it->second)) {
        return std::nullopt;
    }
    return std::get<math::Mat4>(it->second);
}
```

### 3. Component base class changes

**File**: `src/engine/scene/component.h`

Add forward declaration before `class Component`:
```cpp
class Entity;
```

Modify `Component` class:

```cpp
class Component {
public:
    virtual ~Component() = default;

    Component(const Component&) = delete;
    auto operator=(const Component&) -> Component& = delete;
    Component(Component&&) = delete;
    auto operator=(Component&&) -> Component& = delete;

    /// Returns the Entity that owns this component.
    /// Behaviour is undefined if the component has not been attached to an entity
    /// (e.g., a component that was created but never added to an entity).
    auto entity() const noexcept -> Entity;

    /// Lifecycle hook: called by World after this component is attached
    /// to an entity and its entity() accessor is valid.
    ///
    /// Contract:
    ///   - entity() and entity().world() are valid.
    ///   - The component is already in the entity's component list.
    ///   - The hook must NOT add, remove, or modify other components on the entity.
    ///     Doing so is undefined behaviour.
    ///   - The hook may call world().register_camera() (or other World methods).
    ///
    /// Default implementation is a no-op.
    virtual auto on_attach() -> void {}

protected:
    Component() = default;

    // NOTE: These are protected (not private) so that derived components
    // such as CameraComponent can access them in their destructors.
    // friend class World; is declared below and retains write access.
    World* world_ = nullptr;
    EntityId entity_id_ = EntityId::none();

private:
    friend class World;
};
```

> **Rationale for `protected` members**: The spec pseudo-code places `world_` and `entity_id_` as private. However, `CameraComponent`'s destructor needs to access `world_` to call `unregister_camera()`. Making them `protected` avoids needing `friend` declarations for every derived component. `friend class World;` remains in the `private:` section (friendship is not affected by access specifiers) so `World::add_component<T>()` can set these members.

### 4. World::add_component<T>() updated

**File**: `src/engine/scene/world.h`

Modify the existing `add_component<T>()` template (currently lines 97–105) to set entity info and call `on_attach()`:

```cpp
template<typename T, typename... Args>
inline auto World::add_component(EntityId id, Args&&... args) -> T& {
    auto* node = lookup_node(id);
    // UB if node is null, slot dead, or component of type T already exists.
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = component.get();
    ptr->world_ = this;
    ptr->entity_id_ = id;
    node->components_.push_back(std::move(component));
    ptr->on_attach();
    return *ptr;
}
```

### 5. Entity gains friend class Component + Component::entity() definition

**File**: `src/engine/scene/entity.h`

In the `Entity` class `private:` section, add `friend class Component;`:
```cpp
private:
    friend class World;
    friend class Component;   // Component::entity() constructs Entity handles.

    World* world_ = nullptr;
    EntityId id_ = EntityId::none();

    Entity(World& world, EntityId id) noexcept;
};
```

At the **end of the file** (after the `Transform::world_matrix()` inline definition block, which itself is placed after the Entity class and its static_assert), add `Component::entity()` definition:

```cpp
// -- Component::entity() inline definition (requires Entity to be complete) --

inline auto Component::entity() const noexcept -> Entity {
    return Entity(*world_, entity_id_);
}
```

> Placement note: This must appear after `Entity` is fully defined and after all Entity inline/template method implementations. The existing `Transform::world_matrix()` definition at the bottom of `entity.h` serves as the pattern — `Component::entity()` goes after it.

### 6. World::each<T>() template

**File**: `src/engine/scene/world.h`

Add declaration in the `public:` section (after existing component management methods, before camera API):

```cpp
template<typename T, typename Func>
auto each(Func&& func) -> size_t;
```

Add inline implementation after existing template implementations (at the bottom of the file, before the closing namespace brace):

```cpp
template<typename T, typename Func>
inline auto World::each(Func&& func) -> size_t {
    static_assert(std::is_base_of_v<Component, T>,
        "World::each<T>: T must derive from Component");
    size_t count = 0;
    for (auto& slot : slots_) {
        if (!slot.alive) continue;
        auto* node = slot.node;
        if (!node || node->pending_destroy_) continue;
        for (auto& c : node->components_) {
            auto* typed = dynamic_cast<T*>(c.get());
            if (typed) {
                ++count;
                if (!func(Entity(*this, node->id_), *typed)) {
                    return count;  // early exit
                }
                break;  // at most one component of type T per entity
            }
        }
    }
    return count;
}
```

Implementation requirements:
- Must include `static_assert` with descriptive message.
- Skips dead slots and pending-destroy nodes (consistent with `get_component<T>()`).
- Uses `dynamic_cast` per ADR-006 for type matching.
- Constructs fresh `Entity` handle via `Entity(*this, node->id_)` (valid because `World` is a friend of `Entity`).
- `func` receives `(Entity, T&)` and returns `bool` (true=continue, false=exit).
- `break` after finding a match (at most one component of type T per entity).
- Returns total matches visited, which may be less than total matches if early-exit was triggered.

### 7. World camera registration API

**File**: `src/engine/scene/world.h`

Add forward declaration of `CameraComponent` at the top of the file (before `namespace buddd::engine` or inside it, before `class World`):

```cpp
#include <optional>  // std::optional<CameraComponent&>
// ...
namespace buddd::engine {
class CameraComponent;  // forward declaration
// ...
```

Add `#include <optional>` in the include block at the top of the file (alongside `<type_traits>`).

Add public method declarations (after `each<T>()`, before `private:`):

```cpp
/// Registers a CameraComponent as the active camera.
/// Last-registered camera wins (single active camera for v1).
/// Safe to call multiple times for the same camera (idempotent).
/// If another camera was already registered, it is replaced.
auto register_camera(CameraComponent& camera) -> void;

/// Unregisters a CameraComponent from being the active camera.
/// If the given camera is not the active camera, this is a no-op.
/// Safe to call even if no camera is registered.
auto unregister_camera(const CameraComponent& camera) -> void;

/// Returns the currently active camera component, or std::nullopt if
/// no camera has been registered (or the last registered camera
/// was unregistered).
auto active_camera() const noexcept -> std::optional<CameraComponent&>;
```

Add private member (in `private:` section, alongside other member variables):
```cpp
std::optional<CameraComponent&> active_camera_;
```

Add inline implementations (after the class definition, alongside other inline method definitions):

```cpp
inline auto World::register_camera(CameraComponent& camera) -> void {
    active_camera_ = camera;  // std::optional<CameraComponent&> stores the reference
}

inline auto World::unregister_camera(const CameraComponent& camera) -> void {
    // Address comparison: only clear if this component is the active camera
    if (active_camera_.has_value() && &*active_camera_ == &camera) {
        active_camera_.reset();
    }
}

inline auto World::active_camera() const noexcept -> std::optional<CameraComponent&> {
    if (!active_camera_.has_value()) {
        return std::nullopt;
    }
    return active_camera_;
}
```

Implementation requirements:
- `register_camera()` stores a reference to the given `CameraComponent` in `std::optional<CameraComponent&>`.
- `unregister_camera()` compares by address (`&*active_camera_ == &camera`): clears only if the stored reference points to the exact same `CameraComponent` object.
- `active_camera()` returns a copy of the stored optional (no entity/pending-destroy checks needed because the `CameraComponent` destructor always calls `unregister_camera(*this)`).
- No `const_cast` needed — `std::optional<CameraComponent&>` stores the reference directly with proper const-correctness.

### 8. CameraComponent

**File**: `src/engine/scene/camera_component.h`

The header only needs forward declarations; it does NOT include `world.h` or `entity.h`.

```cpp
#pragma once

#include "math/camera.h"
#include "scene/component.h"

namespace buddd::engine {

class CameraComponent : public Component {
public:
    CameraComponent() = default;
    explicit CameraComponent(const math::Camera& camera);

    auto camera() noexcept -> math::Camera&;
    auto camera() const noexcept -> const math::Camera&;

    // -- Lifecycle --
    auto on_attach() -> void override;
    ~CameraComponent() override;

private:
    math::Camera camera_;
};

} // namespace buddd::engine
```

**File**: `src/engine/scene/camera_component.cpp`

The `.cpp` includes `entity.h` and `world.h` for the implementation:

```cpp
#include "scene/camera_component.h"
#include "scene/entity.h"     // Component::entity(), Entity::world()
#include "scene/world.h"      // World::register_camera(), unregister_camera()

#include <iostream>           // std::cerr (debug logging)

namespace buddd::engine {

CameraComponent::CameraComponent(const math::Camera& camera)
    : camera_(camera) {}

auto CameraComponent::camera() noexcept -> math::Camera& {
    return camera_;
}

auto CameraComponent::camera() const noexcept -> const math::Camera& {
    return camera_;
}

auto CameraComponent::on_attach() -> void {
    entity().world().register_camera(*this);
#ifndef NDEBUG
    std::cerr << "CameraComponent: registered entity "
              << entity().id().index << " as active camera\n";
#endif
}

CameraComponent::~CameraComponent() {
    if (world_) {
        world_->unregister_camera(*this);
#ifndef NDEBUG
        std::cerr << "CameraComponent: unregistered entity "
                  << entity_id_.index << "\n";
#endif
    }
}

} // namespace buddd::engine
```

Implementation requirements:
- `on_attach()` calls `entity().world().register_camera(*this)` — passes the component itself directly, not the entity.
- Destructor guards against null `world_` (component never attached). Uses direct member access (`world_`, `entity_id_`), which is allowed because they are `protected` members of `Component`. Calls `world_->unregister_camera(*this)` — passes `*this` (the component itself) for address-based comparison.
- Debug-only logging per spec's observability section (guarded by `#ifndef NDEBUG`):
  - `on_attach()`: `std::cerr << "CameraComponent: registered entity " << entity().id().index << " as active camera\n";`
  - Destructor: `std::cerr << "CameraComponent: unregistered entity " << entity_id_.index << "\n";` (use `entity_id_` directly since `entity()` may be unsafe in destructor)

### 9. MeshRenderer

**File**: `src/engine/render/mesh_renderer.h`

```cpp
#pragma once

#include "render/model.h"
#include "scene/component.h"

#include <memory>

namespace buddd::engine {

class MeshRenderer : public Component {
public:
    explicit MeshRenderer(std::shared_ptr<Model> model);

    auto model() noexcept -> Model&;
    auto model() const noexcept -> const Model&;

private:
    std::shared_ptr<Model> model_;
};

} // namespace buddd::engine
```

**File**: `src/engine/render/mesh_renderer.cpp`

```cpp
#include "render/mesh_renderer.h"

namespace buddd::engine {

MeshRenderer::MeshRenderer(std::shared_ptr<Model> model)
    : model_(std::move(model)) {}

auto MeshRenderer::model() noexcept -> Model& {
    return *model_;
}

auto MeshRenderer::model() const noexcept -> const Model& {
    return *model_;
}

} // namespace buddd::engine
```

Implementation requirements:
- `MeshRenderer` does NOT override `on_attach()` — no registration needs.
- The `shared_ptr<Model>` is moved into the component.
- Calling `model()` on a `MeshRenderer` constructed with null `shared_ptr` is UB (no default constructor, so this can only happen via a moved-from state, which Component prohibits).

### 10. RenderSystem

**File**: `src/engine/render/render_system.h`

```cpp
#pragma once

namespace buddd::engine {

class RenderDevice;
class World;

class RenderSystem {
public:
    RenderSystem(RenderDevice& device, World& world);

    /// Renders one frame. Must be called once per frame.
    /// Behaviour is undefined if called re-entrantly or from within
    /// a World::each() callback.
    auto render() -> void;

private:
    RenderDevice* device_;
    World* world_;
};

} // namespace buddd::engine
```

**File**: `src/engine/render/render_system.cpp`

```cpp
#include "render/render_system.h"

#include "error.h"            // to_string()
#include "render/render_device.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "render/mesh_renderer.h"

#include <iostream>

namespace buddd::engine {

RenderSystem::RenderSystem(RenderDevice& device, World& world)
    : device_(&device), world_(&world) {}

auto RenderSystem::render() -> void {
    device_->begin_frame();

    auto cam_opt = world_->active_camera();
    if (!cam_opt.has_value()) {
        std::cerr << "RenderSystem: no active camera — rendering skipped\n";
        device_->end_frame();
        return;
    }
    auto& cam_comp = *cam_opt;  // optional<CameraComponent&> — operator* yields CameraComponent&
    auto vp = cam_comp.camera().view_projection_matrix();

    world_->each<MeshRenderer>([&](Entity entity, MeshRenderer& mr) -> bool {
        auto world_mat = entity.world_matrix();
        auto mvp = vp * world_mat;

        auto uniform_result = mr.model().material().set_uniform("u_mvp", mvp);
        if (!uniform_result) {
            std::cerr << "RenderSystem: set_uniform(u_mvp) failed for entity "
                      << entity.id().index << ": "
                      << to_string(uniform_result.error()) << "\n";
            return true;
        }

        mr.model().draw(*device_);
        return true;
    });

    device_->end_frame();
}

} // namespace buddd::engine
```

Key differences from the old contract's RenderSystem:
- `auto cam_opt = world_->active_camera();` — returns `std::optional<CameraComponent&>`.
- `auto& cam_comp = *cam_opt;` — extracts the CameraComponent reference directly via `optional<CameraComponent&>::operator*`.
- `auto vp = cam_comp.camera().view_projection_matrix();` — no intermediate `get_component<CameraComponent>()` call.
- No second error check for missing CameraComponent — the new API directly stores and returns a CameraComponent&, so the component is guaranteed to exist if `active_camera()` returns a value.

### 11. Cube scene demo

**DO NOT modify** `src/cmd/demo/cube_demo.cpp` or `src/cmd/demo/cube_demo.h`.

Create **two new files**:

**File**: `src/cmd/demo/cube_scene_demo.h`

```cpp
#pragma once

namespace buddd::engine {
class Platform;
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

/// Runs the scene-based cube demo: 120-frame render loop using
/// World + RenderSystem instead of manual camera/MVP/draw calls.
///
/// @param platform  The engine platform (for event polling).
/// @param device    The render device (for rendering).
/// @param argc      Argument count (argv[0] is the demo name).
/// @param argv      Argument vector (argv[0] is the demo name).
/// @return 0 on success, non-zero on error.
[[nodiscard]] auto run_cube_scene_demo(buddd::engine::Platform& platform,
                                       buddd::engine::RenderDevice& device,
                                       int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo
```

**File**: `src/cmd/demo/cube_scene_demo.cpp`

```cpp
#include "demo/cube_scene_demo.h"
#include "demo/demo_helpers.h"

#include "platform/platform.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "scene/world.h"
#include "scene/camera_component.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace be = buddd::engine;

auto buddd::cmd::demo::run_cube_scene_demo(
    be::Platform& platform, be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int
{
    // 1. Create a World (scene container)
    be::World world;

    // 2. Create a single entity that will be both the camera and the renderable cube
    auto entity = be::Entity::create(world);

    // 3. Attach CameraComponent (with same camera params as before)
    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );
    entity.add_component<be::CameraComponent>(camera);

    // 4. Attach MeshRenderer with the cube model
    auto cube = setup_cube(device);
    // cube.material is deliberately discarded — Model already holds a
    // shared_ptr<Material> internally (accessed via Model::material()).
    entity.add_component<be::MeshRenderer>(std::move(cube.model));

    // 5. Create the RenderSystem (bridges World + RenderDevice)
    be::RenderSystem render_system(device, world);

    // 6. Render loop: ~120 frames at 60 FPS
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16);
    auto demo_start = std::chrono::steady_clock::now();

    std::cerr << "Demo started: cube (scene-based, " << target_frames << " frames)\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!platform.poll_events()) {
            std::cerr << "Demo aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        // Update the entity's rotation each frame
        auto elapsed = std::chrono::steady_clock::now() - demo_start;
        float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
        float angle = elapsed_seconds * 0.5f;
        entity.transform().rotation =
            be::math::Quat::from_axis_angle(be::math::Vec3::unit_y(), angle);

        // Let RenderSystem handle all rendering
        render_system.render();

        // Frame rate limiting
        auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
        if (frame_elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_elapsed);
        }
    }

    std::cerr << "Demo complete: cube (scene-based, " << target_frames << " frames rendered)\n";
    return EXIT_SUCCESS;
}
```

Key differences from the existing `cube_demo.cpp` (which stays untouched):
- No standalone `Camera` variable — camera is an ECS component.
- No manual MVP computation per frame — `RenderSystem` handles it.
- No explicit `device.begin_frame()`/`device.end_frame()` — `RenderSystem` handles it.
- `setup_cube`'s returned `Material` shared_ptr is discarded (Model already holds a shared_ptr to it).
- Entity transform is updated directly; `RenderSystem` reads `entity.world_matrix()`.
- `RenderSystem` is created once and reused.

### 12. Tests

All tests go in `tests/scene_rendering_tests.cpp`. See Required tests section for the complete test specification.

## Required tests

All tests are in a single new file: `tests/scene_rendering_tests.cpp`.

### Test file structure

- Use `using namespace buddd::engine;` (existing pattern).
- Use `constexpr float TOL = 1e-5f;` for float comparisons.
- Anonymous namespace for any helper component types needed.
- Tag all tests with `[scene_rendering]`.

### Test cases mapped to acceptance criteria

| Test case | ACs | Description |
|---|---|---|
| `Component entity awareness` | AC-001, AC-002, AC-005, AC-006 | Create entity with a component that records entity info in `on_attach()`. Verify `component.entity().id() == entity.id()`. Also verify `Component` has `world_`/`entity_id_` members and `friend class World` (compile-time check via access pattern). |
| `Component on_attach is called` | AC-003, AC-004 | Create a component that sets a flag in `on_attach()`. Add it to an entity. Verify the flag is set. |
| `World::each basic iteration` | AC-007 | Create 5 entities, attach `TagComp` to 3. Call `each<TagComp>` with counter callback. Verify callback called 3 times, return value is 3. |
| `World::each skips pending-destroy` | AC-008 | Create entity with `TagComp`, mark for destroy, call `each<TagComp>` — verify callback NOT called. After flush, verify still not called. |
| `World::active_camera lifecycle` | AC-009, AC-010, AC-011 | Fresh world returns nullopt. Create `CameraComponent` A (standalone, not attached to an entity), register it, verify `active_camera()` returns reference to A. Register `CameraComponent` B, verify returns reference to B. Unregister with different `CameraComponent` C, verify no change. Unregister B, verify nullopt. |
| `CameraComponent auto-registers` | AC-012, AC-013, AC-014, AC-015 | Create entity, add `CameraComponent`. Verify `world.active_camera()` has value and `*world.active_camera()` yields the component. Verify `camera()` accessor returns mutable reference. |
| `CameraComponent destructor unregisters` | AC-016 | Add `CameraComponent` to entity, verify active. Remove via `remove_component<CameraComponent>`. Verify `world.active_camera()` returns nullopt. |
| `CameraComponent destructor guards null world` | AC-028 | Create `CameraComponent` on stack (never attached to entity). Destroy it — no crash (null `world_` guard). |
| `MeshRenderer storage and access` | AC-017, AC-018 | Create `MeshRenderer` with a `shared_ptr<Model>`. Verify `model()` returns the same Model (mutable and const). |
| `RenderSystem construction` | AC-019 | `RenderSystem` can be constructed with a `RenderDevice&` and `World&`. |
| `RenderSystem begin_frame/end_frame counters` | AC-020 | World with no camera. Call `render()`. Verify `frame_begin_count() == 1`, `frame_end_count() == 1` using headless device. |
| `RenderSystem draw call count` | AC-021 | World with camera entity + 1 MeshRenderer entity. Call `render()`. Verify `draw_call_count() == 1`. |
| `RenderSystem MVP computation` | AC-022 | World with camera at origin looking down -Z + MeshRenderer entity at (10,0,0) with identity rotation/scale. Call `render()`. Verify via `headless_material.get_uniform_mat4("u_mvp")` that the matrix translates by (10,0,0). |
| `RenderSystem no camera warning` | AC-023 | World with no camera. Call `render()`. Verify `frame_begin_count() == 1`. Capture/count `std::cerr` output containing "no active camera". |
| `RenderSystem set_uniform failure skip` | AC-024 | Two MeshRenderer entities: one valid, one with material lacking `u_mvp`. Call `render()`. Verify `draw_call_count() == 1` (only the valid entity drawn). No crash. |
| `World::each empty world` | AC-029 | Call `each<MeshRenderer>` on fresh World with no entities. No crash, callback never invoked, return 0. |
| `World::each zero matches` | AC-027 | World with entities but no `CameraComponent`. Call `each<CameraComponent>`. No crash, callback never invoked. |
| `World::each early exit` | AC-030 | 5 entities with `TagComp`, callback returns `false` on 3rd. Verify only 3 callbacks, return value is 3. |
| `Extra: World::each static_assert` | (implicit) | Verify `each<NonComponentType>` fails to compile. Use `STATIC_REQUIRE` with `std::is_base_of` or a SFINAE test in a helper. Or verify by attempting to instantiate with `int` in a `decltype` and checking it's ill-formed. |

### Additional notes on test setup

- **Camera at origin looking down -Z**: Use `math::Camera` default (origin, identity orientation). The view_projection_matrix will project from origin looking down -Z.
- **Entity at (10,0,0)**: Create entity, set `entity.transform().position = math::Vec3(10.0f, 0.0f, 0.0f)`.
- **Material without `u_mvp`**: Create material via headless device without providing `known_uniforms` containing `"u_mvp"`, OR create a material from a shader pair that doesn't declare `uniform mat4 u_mvp;`.
- **Frame counter accessors**: Use `RenderDeviceHeadless` directly in tests.
- **`std::cerr` capture**: Use a `std::stringstream` redirected from `std::cerr` via `std::streambuf` swap. Existing Catch2 pattern: save `std::cerr.rdbuf()`, replace, restore after test.
- **Camera registration tests**: To test `register_camera()`/`unregister_camera()` directly without going through `add_component`, create `CameraComponent` instances on the stack or via `std::unique_ptr` (not attached to any entity). Verify by comparing addresses via `&*world.active_camera()`.
- **`active_camera()` returns `std::optional<CameraComponent&>`**: Use `has_value()` to check presence, then `*opt` to obtain the `CameraComponent&`. Example:

  ```cpp
  world.register_camera(cam);
  REQUIRE(world.active_camera().has_value());
  REQUIRE(&*world.active_camera() == &cam);
  ```

## Edge cases

All edge cases from SPEC-011 section "Edge cases" must be tested where possible. Specifically:

| Case | How it's handled |
|---|---|
| `World::each<T>()` on empty world | Tested (AC-029). |
| `World::each<T>()` with modifying callback | UB — documented, not tested. |
| `CameraComponent` destructor during `World::~World()` | Works: `world_` pointer stays valid during World destruction. Destructor calls `world_->unregister_camera(*this)` which compares by address. Tested via `CameraComponent` stack-allocated + never-attached case. |
| `CameraComponent` destructor after `remove_component` | Tested (AC-016). |
| `MeshRenderer` with null `shared_ptr` | Not possible via public API (no default constructor). UB if it happens — not tested. |
| `active_camera()` returns CameraComponent that was already unregistered | Not possible — the CameraComponent destructor always calls `unregister_camera(*this)`. If someone manually unregisters, `active_camera()` correctly returns `std::nullopt`. |
| Multiple `CameraComponent` instances on different entities | Last-registered-wins; destructor of CameraComponent A checks `&camera == &*active_camera_` so doesn't clear B. |
| Multiple `CameraComponent` instances on same entity | Not possible (existing UB constraint). |
| `render()` with null device/world | UB — documented. |
| `RenderSystem` destroyed before world/device | UB — documented. |
| Standalone `CameraComponent` (never attached) registered directly | Works — `register_camera()` stores a reference in `std::optional<CameraComponent&>`. The component must outlive the registration. |

## Security impact

None. No elevated privileges, network, filesystem, or secret access involved. No new third-party dependencies. The architecture boundary (CONST-001) is maintained: `mesh_renderer.h` and `render_system.h` expose no backend-specific types. `camera_component.h` depends only on `math/` headers.

## Data and migration impact

None. No schema changes, no data migration, no seed data. All new state (component entity info, active camera reference) is transient in-memory state within the ECS.

## API compatibility impact

- **`Component` base class**: Gains `entity()` accessor and `on_attach()` virtual method. Existing derived component types that do not override `on_attach()` are unaffected (default no-op). The non-copyable/non-movable contract is unchanged.
- **`World`**: Gains `each<T>()`, `register_camera(CameraComponent&)`, `unregister_camera(const CameraComponent&)`, `active_camera()`. Existing API is unchanged. `add_component<T>()` behavior changes: now sets entity info and calls `on_attach()`. Existing components without `on_attach()` override are unaffected (default no-op).
- **`Entity`**: Gains `friend class Component;`. No public API change.
- **`RenderDeviceHeadless`**: Gains three public accessors (`frame_begin_count()`, `frame_end_count()`, `draw_call_count()`). These are additive, not breaking.
- **`MaterialHeadless`**: Gains `get_uniform_mat4()`. Additive, not breaking.
- **`cube_demo.cpp`**: NOT modified. The existing `run_cube_demo` function is completely untouched. A new `run_cube_scene_demo` function is added in separate files.
- **`CameraComponent`** destructor now calls `world_->unregister_camera(*this)` (address comparison) instead of `world_->unregister_camera(entity_id_)` (ID comparison). The behavior change is that two different `CameraComponent` instances on different entities are now distinguished by address, not by entity ID. This is a more precise check.

## Documentation impact

- `docs/specs/scene-rendering/README.md` — should mention the new files and their roles (if such a README exists; otherwise defer).
- `docs/wiki/architecture/module-map.md` — should be updated to note the `render -> scene` dependency introduced by `MeshRenderer` and `RenderSystem`.

## ADR impact

None. The implementation does not require a new ADR. The relationship to SPEC-008 is already documented in the spec's "Relationship to SPEC-008" section. No existing ADR is superseded. ADR-010 (no raw pointers in public API) is adopted concurrently with this spec and mandates the `std::optional<CameraComponent&>` pattern used here.

## Constitution impact

None. The implementation does not require a constitution amendment.

## Done criteria

- [ ] **All new files created**: `camera_component.h`, `camera_component.cpp`, `mesh_renderer.h`, `mesh_renderer.cpp`, `render_system.h`, `render_system.cpp`, `tests/scene_rendering_tests.cpp`, `cube_scene_demo.h`, `cube_scene_demo.cpp`.
- [ ] **`component.h` modified**: protected members `world_`/`entity_id_`, `entity()` declaration, `on_attach()` virtual, `friend class World;`.
- [ ] **`entity.h` modified**: `friend class Component;` added. `Component::entity()` inline definition added at end of file.
- [ ] **`world.h` modified**: `add_component<T>()` sets entity info and calls `on_attach()`. `each<T>()` template added with `static_assert`. Camera API (`register_camera(CameraComponent&)`, `unregister_camera(const CameraComponent&)`, `active_camera()`) added. `active_camera_` member (`std::optional<CameraComponent&>`) added. Forward declaration of `CameraComponent` added. `#include <optional>` added.
- [ ] **`render_device_headless.h` modified**: `frame_begin_count_`, `frame_end_count_` members added. Public accessors `frame_begin_count()`, `frame_end_count()`, `draw_call_count()` added.
- [ ] **`render_device_headless.cpp` modified**: `++frame_begin_count_` in `begin_frame()`, `++frame_end_count_` in `end_frame()`.
- [ ] **`material_headless.h` modified**: `get_uniform_mat4(std::string_view name) const -> std::optional<math::Mat4>` added.
- [ ] **`cube_demo.cpp` and `cube_demo.h` NOT modified** (confirmed by `git diff`).
- [ ] **New demo files `cube_scene_demo.h`/`cube_scene_demo.cpp` created** with `run_cube_scene_demo` function using `World + RenderSystem`, no standalone `Camera`, no manual MVP, no explicit begin/end_frame.
- [ ] **All AC-001 through AC-030 tests pass** in headless mode (no GPU required).
- [ ] **All existing tests still pass** (`scene_graph_tests.cpp`, `render_device_tests.cpp`, etc.).
- [ ] **No memory leaks**: Test that creates and destroys 100 worlds with cameras and mesh renderers shows no leaks under ASAN.
- [ ] **Cube scene demo compiles and links**: `buddd demo cube_scene` runs and displays a rotating coloured cube (manual verification).
- [ ] **`static_assert(std::is_base_of_v<Component, T>)`** present in `World::each<T>()`.
- [ ] **No `(or verify no crash)` escape hatch** in any test (AC-023 uses `frame_begin_count() == 1`).
- [ ] **No `const_cast`** anywhere in the implementation — `std::optional<CameraComponent&>` is natively const-correct.
- [ ] **`camera_component.h` does NOT include `entity.h` or `world.h`** — uses only forward declarations in the header. The `.cpp` includes them.
