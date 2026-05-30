# SPEC-011 — Scene-Based Rendering: CameraComponent, MeshRenderer, RenderSystem

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

| | |
|---|---|
| Approved by | guillaume (human) |
| Date | 2026-05-30 |
| Time | After spec-critic + contract-critic reviews, all issues resolved. Spec and contract both Accepted. |

## Problem

The Buddd Engine has two mature but disconnected systems:

- **Scene graph** (`src/engine/scene/`): `World`, `Entity`, `Transform`, `Component`. ECS infrastructure with hierarchy, deferred destruction, and polymorphic component storage — but no way to iterate entities by component type, no rendering integration, and no camera integration.
- **Render pipeline** (`src/engine/render/`): `RenderDevice`, `Shader`, `Material`, `VertexBuffer`, `IndexBuffer`, `Model`. Full GPU abstraction with OpenGL 4.5 Core and Headless backends — but no connection to the scene graph.

Currently, rendering a 3D object (as in the cube demo `src/cmd/demo/cube_demo.cpp`) requires:

- Manual creation of a standalone `Camera` math object and per-frame MVP computation.
- Manual iteration over hardcoded objects and draw calls.
- No reuse of the scene graph's hierarchy or transform system for rendering.

Every future feature (entity-attached lights, particle systems on entities, debug rendering of entity bounds, camera switching) depends on bridging these two systems.

## Goals

- **Component entity awareness**: Every `Component` knows its owning entity, with an `on_attach()` lifecycle hook called by `World` when the component is added.
- **`World::each<T>()` iteration**: A template method to iterate all entities that have a specific component type, enabling systems to query the scene.
- **Camera as an ECS component**: `CameraComponent` wrapping `math::Camera`, registered with `World` as the active camera on attach, unregistered on detach.
- **MeshRenderer component**: An ECS component holding a `shared_ptr<Model>`, allowing any entity to be rendered.
- **`RenderSystem`**: An engine-level system (not app-level) that orchestrates rendering from a scene graph each frame: gets the active camera, iterates entities with `MeshRenderer`, computes MVP from the camera and entity `world_matrix()`, sets the `u_mvp` uniform, and issues draw calls.
- **Integration demo**: Update the cube demo to use `World` + `RenderSystem` instead of manual draw calls, proving the bridge works end-to-end.
- **Uniform convention**: Use `u_mvp` for the combined model-view-projection matrix, consistent with existing demo code.

## Relationship to SPEC-008 (Scene Graph)

SPEC-011 modifies the `Component` base class defined in SPEC-008 by adding:
- Entity awareness (`world_`, `entity_id_`, `entity()` accessor).
- A virtual `on_attach()` lifecycle hook.

SPEC-008 (accepted) states: *"No component lifecycle hooks (`on_attach`/`on_detach` are deferred)"* and lists lifecycle hooks as out of scope.

**SPEC-011 supersedes SPEC-008 on this specific point.** The `on_attach()` hook is the minimal lifecycle mechanism needed to bridge scene graph and render pipeline (specifically, for `CameraComponent` to auto-register with `World`). This is a targeted, justified exception to SPEC-008's deferral — not a general approval for arbitrary lifecycle hooks.

All other SPEC-008 contracts (non-copyable/non-movable `Component`, `vector<unique_ptr<Component>>` storage, `dynamic_cast` dispatch, deferred destruction lifecycle) remain unchanged.

## Non-goals

- No camera system (no follow, look-at-target, or orbit behaviors) — the camera is a static component set once.
- No multi-camera support — single active camera (last-registered-wins) for v1.
- No frustum culling — all `MeshRenderer` entities are drawn regardless of visibility.
- No render graph, render passes, or post-processing.
- No material instancing or batching — one draw call per `MeshRenderer`.
- No component pooling or archetype ECS — v1 keeps `vector<unique_ptr<Component>>` per entity.
- No `on_detach()` lifecycle hook — only `on_attach()` and the destructor for cleanup.
- No component enable/disable state.
- No serialization of camera or mesh renderer state.
- No lighting or shadow components.
- No entity tag or layer filtering for rendering.
- No render order sorting (opaque only, draws in iteration order).

## Actors

| Actor | Description |
|---|---|
| Engine developer | Adds engine features that require rendering from the scene graph — cameras attached to entities, renderable components, debug visualization. Uses `World::each<T>()` to write systems that query components. |
| Application developer | Builds on top of the engine. Creates entities, attaches `CameraComponent` and `MeshRenderer`, and calls `RenderSystem::render()` each frame without managing cameras, MVP, or draw calls manually. |
| Test suite | Catch2 v3 tests that verify component entity awareness, `World::each<T>()` iteration, camera registration lifecycle, `MeshRenderer` storage, and `RenderSystem` execution — all in headless mode (no GPU required). |

## User-visible behavior

### 1. Component base class changes (`src/engine/scene/component.h`)

`Component` gains entity awareness while remaining minimal:

```cpp
// Forward declaration — Entity is defined in entity.h which includes this header.
class Entity;

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

private:
    friend class World;   // World sets world_ and entity_id_ after construction.

    World* world_ = nullptr;
    EntityId entity_id_ = EntityId::none();
};
```

- `Component::entity()` is defined after `Entity` is fully defined (see below).
- `World::add_component<T>()` is updated to set `world_` and `entity_id_` on the newly created component, then call `on_attach()`.
- The `on_attach()` contract prohibits modifying the entity's component list to prevent iterator invalidation and re-entrancy issues.

#### Component::entity() definition (in `src/engine/scene/entity.h`)

After `Entity` is fully defined, `Component::entity()` is defined inline:

```cpp
inline auto Component::entity() const noexcept -> Entity {
    return Entity(*world_, entity_id_);
}
```

To support this, `Entity` gains `friend class Component;` alongside the existing `friend class World;`, since `Component::entity()` needs to call the `Entity(World&, EntityId)` constructor that is currently private to `World`:

```cpp
class Entity {
    // ...
private:
    friend class World;
    friend class Component;   // ADDED — Component::entity() constructs Entity handles.
    Entity(World& world, EntityId id) noexcept;
    // ...
};
```

#### World::add_component<T>() updated

The existing template in `world.h` is modified to set entity info and call `on_attach()`:

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
    ptr->on_attach();   // NEW: lifecycle hook after entity info is set
    return *ptr;
}
```

### 2. World extensions (`src/engine/scene/world.h`)

#### World::each<T>(Func&&)

```cpp
/// Iterates all alive, non-pending-destroy entities that have a component
/// of type T. Calls `func(entity, component)` for each match.
///
/// The callback receives (Entity, T&) where Entity is the owning entity
/// and T& is the component reference. The callback must return bool:
///   - `true`  → continue iteration
///   - `false` → stop iteration (early exit)
///
/// Returns the total number of matches visited (can be less than total
/// matches if early-exit was triggered).
///
/// Behaviour is undefined if the callback modifies the entity's component
/// list (adds, removes, or destroys components) during iteration.
///
/// @tparam T  The component type to query (must derive from Component).
/// @tparam Func  Invocable<bool(Entity, T&)>.
template<typename T, typename Func>
auto each(Func&& func) -> size_t;
```

Implementation iterates the `slots_` array linearly. This is O(n) in total entities, which is acceptable for v1:

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

- Uses `dynamic_cast` matching the existing component retrieval pattern.
- Stops searching a node's components after finding the first match (at most one component of a given type per entity).
- Skips pending-destroy entities (consistent with `get_component<T>()` behaviour).
- The callback receives a fresh `Entity` handle constructed from `World*` + `EntityId`.
- Returns the number of matches visited (useful for callers that don't need early-exit but want a count).
- When early-exit is not needed, callers can use `[](Entity, T&) { return true; }` — or a helper adapter can be provided later.

#### Camera registration API

```cpp
/// Registers a CameraComponent as the active camera.
/// Last-registered component wins (single active camera for v1).
/// Safe to call multiple times for the same component (idempotent).
/// If another camera was already registered, it is replaced.
void register_camera(CameraComponent& camera);

/// Unregisters a CameraComponent from being the active camera.
/// If the given component is not the active camera, this is a no-op.
/// Safe to call even if no camera is registered.
void unregister_camera(const CameraComponent& camera);

/// Returns a reference to the currently active CameraComponent, or
/// std::nullopt if no camera has been registered (or the last
/// registered camera was unregistered).
auto active_camera() const noexcept -> std::optional<CameraComponent&>;
```

Internal state:

```cpp
private:
    // ... existing members ...
    std::optional<CameraComponent&> active_camera_;  // NEW — tracks active camera
```

- `World` must forward-declare `CameraComponent` and include `<optional>` (for `std::optional<CameraComponent&>`).
- `register_camera()` stores a reference to the CameraComponent in `active_camera_`.
- `unregister_camera()` clears `active_camera_` only if it refers to the same component (address comparison).
- `active_camera()` returns the stored `std::optional<CameraComponent&>` or `std::nullopt`.
- No reference counting — v1 supports only a single active camera.

### 3. CameraComponent (`src/engine/scene/camera_component.h`)

`CameraComponent` bridges `math::Camera` (a pure math type) into the ECS as a component. Placed in `scene/` because it is primarily an ECS component and `scene/` already depends on `math/` (via `Transform` using `Vec3`, `Quat`, `Mat4`).

```cpp
#pragma once

#include "math/camera.h"
#include "scene/component.h"

namespace buddd::engine {

/// ECS component wrapping a math::Camera.
/// When attached to an entity via World::add_component, on_attach()
/// registers this entity as the active camera on the World.
/// When the component is destroyed (entity destruction or removal),
/// the destructor unregisters from the World.
class CameraComponent : public Component {
public:
    /// Default constructor creates a default math::Camera.
    CameraComponent() = default;

    /// Constructs from an existing Camera configuration.
    explicit CameraComponent(const math::Camera& camera);

    /// Returns the wrapped camera (mutable).
    auto camera() noexcept -> math::Camera&;

    /// Returns the wrapped camera (const).
    auto camera() const noexcept -> const math::Camera&;

    // -- Lifecycle --
    /// Registers this entity as the active camera on the World.
    /// Contract: entity() and entity().world() are valid.
    auto on_attach() -> void override;

    /// Unregisters this entity from the World's active camera slot.
    /// Guards against null world_ (component never attached to an entity).
    ~CameraComponent() override;

private:
    math::Camera camera_;
};

} // namespace buddd::engine
```

- `on_attach()` calls `entity().world().register_camera(*this)`.
- The destructor guards against null `world_` (component never attached) before calling unregister:
  ```cpp
  ~CameraComponent() override {
      if (world_) {
          world_->unregister_camera(*this);
      }
  }
  ```
  This handles the case where a `CameraComponent` is constructed but never added to an entity.
- If the `CameraComponent` is removed via `entity.remove_component<CameraComponent>()`, the destructor runs and unregisters.
- If the entity is destroyed and flushed, the destructor also runs (component is destroyed during entity teardown, `world_` is still valid).
- After unregistration, `World::active_camera()` returns `std::nullopt` (or the next registered camera if another was registered).
- Moving a `CameraComponent` is not possible because `Component` is non-movable.

### 4. MeshRenderer component (`src/engine/render/mesh_renderer.h`)

`MeshRenderer` is a render-side component that makes any entity renderable. Placed in `render/` because it depends on `Model`, `Material`, and `RenderDevice` — types that belong to the render module. It inherits `Component` from `scene/`, introducing a `render -> scene` dependency (acceptable for v1; `scene/` has no reverse dependency on `render/`).

```cpp
#pragma once

#include "render/model.h"
#include "scene/component.h"

#include <memory>

namespace buddd::engine {

/// ECS component that marks an entity as renderable.
/// Holds a shared_ptr<Model> for shared ownership of GPU resources
/// (multiple MeshRenderer components may reference the same Model).
class MeshRenderer : public Component {
public:
    /// Constructs a MeshRenderer that wraps the given Model.
    /// The Model is shared (not copied) via shared_ptr.
    explicit MeshRenderer(std::shared_ptr<Model> model);

    /// Returns a reference to the Model.
    auto model() noexcept -> Model&;
    auto model() const noexcept -> const Model&;

private:
    std::shared_ptr<Model> model_;
};

} // namespace buddd::engine
```

- `MeshRenderer` does **not** override `on_attach()` — it has no registration needs.
- The shared_ptr ownership model allows multiple entities to share the same geometry and material (e.g., instancing a cube across many entities).
- The `Model` reference is valid as long as any `MeshRenderer` holds a `shared_ptr` to it.
- `MeshRenderer` is movable in principle (it only contains a `shared_ptr`), but `Component` is non-movable, so `MeshRenderer` is non-movable too.

### 5. RenderSystem (`src/engine/render/render_system.h`)

`RenderSystem` is an engine-level orchestrator that connects a `RenderDevice` to a `World` and renders the scene each frame. It lives in `render/` and depends on `scene/` for `World`, `Entity`, `CameraComponent`, and `MeshRenderer`.

```cpp
#pragma once

#include <memory>

namespace buddd::engine {

class RenderDevice;
class World;

/// Engine-level system that renders a World to a RenderDevice.
/// Each frame, it:
///   1. Calls device.begin_frame().
///   2. Gets the active camera from the World.
///   3. Iterates all entities with MeshRenderer components.
///   4. For each, computes MVP = camera.view_projection() * entity.world_matrix(),
///      sets the u_mvp uniform on the material, and draws the model.
///   5. Calls device.end_frame().
///
/// If no active camera is registered, RenderSystem logs a warning and skips
/// rendering (only begin_frame/end_frame are still called to clear the buffer).
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

#### RenderSystem::render() — detailed behaviour

```cpp
auto RenderSystem::render() -> void {
    device_->begin_frame();

    auto cam_opt = world_->active_camera();
    if (!cam_opt.has_value()) {
        // No camera registered — still call begin_frame/end_frame
        // so the framebuffer is cleared (if the device clears on begin_frame).
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
            return true;  // skip this entity, continue to next
        }

        mr.model().draw(*device_);
        return true;  // continue iteration
    });

    device_->end_frame();
}
```

- `begin_frame()` and `end_frame()` are always called to maintain the frame lifecycle (clears the framebuffer, swaps buffers).
- If no active camera exists, a warning is logged and only begin/end_frame execute (the screen is cleared but nothing is drawn).
- If `set_uniform("u_mvp", ...)` fails for an entity, that entity is skipped (logged, not crashed).
- The `world_matrix()` call walks the entity's parent chain each time — no caching in v1.
- The MVP computation is `view_projection * world_matrix`, consistent with column-vector convention (Mat4 * Vec4).

### 6. Integration demo (new files: `src/cmd/demo/cube_scene_demo.h`, `src/cmd/demo/cube_scene_demo.cpp`)

A new cube scene demo is added alongside the existing `cube_demo.cpp` and `cube_demo.h` (which are left exactly as they were before this spec). It demonstrates the scene graph and `RenderSystem` instead of manual camera management and draw calls.

The demo is registered in `src/cmd/commands/demo_command.cpp` as a new subcommand: `"cube-scene"` (invoked as `buddd demo cube-scene`). The existing `"cube"` command is left unchanged.

**Flow in `cube_scene_demo.cpp`:**

```cpp
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

Key differences from the original `cube_demo.cpp`:
- No standalone `Camera` variable — camera is an ECS component.
- No manual MVP computation per frame — `RenderSystem` handles it.
- No explicit `device.begin_frame()`/`device.end_frame()` — `RenderSystem` handles it.
- Entity transform is updated directly; `RenderSystem` reads the world matrix.
- `RenderSystem` is created once and reused for all frames.

The demo continues to use `setup_cube(device)` for creating the cube Model and Material (unchanged from SPEC-009), but now the Model is transferred into a `MeshRenderer` component.

Note: `setup_cube` currently returns `CubeResources{shared_ptr<Material>, Model}`. The Model is non-copyable but movable, so `std::move(cube.model)` works.

## User stories

### Story 1 — Component knows its entity (Priority: P1)

As a component author, I want my component to know which entity owns it and to execute initialization logic when it is attached, so that I can register resources or establish connections.

**Given** a component type `struct MyComp : public Component { auto on_attach() -> void override { /* capture entity */ } };`
**When** I create an entity and call `entity.add_component<MyComp>()`
**Then** `MyComp::on_attach()` is called, and inside `on_attach()`, `this->entity()` returns a valid `Entity` handle pointing to the owning entity. After `add_component` returns, the component's entity info is set.

### Story 2 — World::each<T>() iterates entities with a specific component (Priority: P1)

As an engine developer, I want to find all entities with a specific component type and perform an operation on each, so that I can write systems that process subsets of the scene.

**Given** a `World` with 10 entities, 3 of which have a `HealthComponent` (HP)
**When** I call `world.each<HealthComponent>([](Entity e, HealthComponent& h) { h.hp += 10; });`
**Then** the callback is called exactly 3 times, once for each entity that has `HealthComponent`. The `HP` values of those 3 entities are increased by 10. Entities without `HealthComponent` are not visited.

### Story 3 — CameraComponent registers as active camera on attach (Priority: P1)

As an application developer, I want attaching a `CameraComponent` to an entity to automatically make that entity the active camera, so that I don't need to manually register cameras.

**Given** a `World` with no active camera
**When** I create an entity and `entity.add_component<CameraComponent>(camera)`
**Then** `world.active_camera()` returns a reference to that entity's `CameraComponent`. When the entity is destroyed and flushed, `world.active_camera()` returns `std::nullopt`.

### Story 4 — MeshRenderer makes an entity renderable (Priority: P1)

As an application developer, I want to attach a `MeshRenderer` with a `Model` to an entity, and have the `RenderSystem` draw it using the entity's world transform and the active camera.

**Given** a `World` with:
- An entity A with `CameraComponent`
- An entity B with `MeshRenderer` (holding a cube `Model`) at position `(2, 0, 0)` with identity rotation and scale
**When** `RenderSystem::render()` is called
**Then** a draw call is issued for entity B's model using MVP = `camera.view_projection * entity.world_matrix`. The cube appears at world position `(2, 0, 0)`.

### Story 5 — RenderSystem handles missing camera gracefully (Priority: P2)

As an engine developer, I want `RenderSystem::render()` to not crash when no camera exists, so that I can set up scenes incrementally.

**Given** a `World` with a `MeshRenderer` entity but no `CameraComponent` on any entity
**When** `RenderSystem::render()` is called
**Then** a warning is logged to `std::cerr`, `begin_frame()` and `end_frame()` are still called (the screen is cleared), and no draw calls are issued. No crash occurs.

### Story 6 — RenderSystem handles set_uniform failure gracefully (Priority: P2)

As an engine developer, I want a `MeshRenderer` whose material lacks the `u_mvp` uniform to be skipped rather than crash, so that a misconfigured material does not break the entire frame.

**Given** a `World` with a `MeshRenderer` whose `Material` does not have a `u_mvp` uniform
**When** `RenderSystem::render()` is called
**Then** that entity is skipped (logged to `std::cerr`), and other `MeshRenderer` entities in the world are rendered normally.

### Story 7 — CameraComponent destructor unregisters from World (Priority: P2)

As an engine developer, I want removing a `CameraComponent` (via `remove_component` or entity destruction) to unregister the camera, so that the active camera slot does not dangle.

**Given** a `World` where entity A is the active camera (has `CameraComponent`)
**When** `entity.remove_component<CameraComponent>()` is called on A
**Then** `world.active_camera()` returns `std::nullopt`. The `CameraComponent` destructor runs and unregisters.

### Story 8 — Scene-based cube demo renders identically to manual version (Priority: P3)

As an application developer, I want the scene-based cube demo to produce the same visual output as the original, so that I can trust the bridging layer.

**Given** the updated cube demo using `World` + `RenderSystem`
**When** I run `buddd demo cube`
**Then** a coloured cube appears and rotates smoothly around the Y axis, visually identical to the pre-bridge version. The six face colours are visible.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `Component` has private members `World* world_` and `EntityId entity_id_`, initialized to `nullptr` and `EntityId::none()`. `friend class World` is declared. | Static inspection: `component.h` contains the members and friend declaration. |
| AC-002 | `Component` has a `entity() const noexcept -> Entity` method, declared in `component.h` and defined inline after `Entity` is fully defined (in `entity.h` or equivalent location). | Code compiles; calling `component.entity()` returns a valid `Entity` handle after the component is added to an entity. |
| AC-003 | `Component` has a virtual `on_attach() -> void` method with a default no-op implementation. | Code compiles; derived component types can override `on_attach()`. |
| AC-004 | `World::add_component<T>()` sets `world_` and `entity_id_` on the newly created component before pushing it into the entity's component vector, and calls `ptr->on_attach()` after the push. | Unit test: create a component that records its `entity()` in `on_attach()`. After `add_component`, the recorded entity matches the owning entity. |
| AC-005 | `Component::entity()` returns the correct `Entity` handle for the owning entity. | Unit test: create entity, add component, verify `component.entity().id() == entity.id()`. |
| AC-006 | `Entity` has `friend class Component;` to allow `Component::entity()` to construct an `Entity` handle. | Code compiles (AC-005 passes). |
| AC-007 | `World::each<T>(Func&& func)` exists as a template method returning `size_t`. The callback receives `(Entity, T&)` and returns `bool` (`true` = continue, `false` = stop). The method returns the number of matches visited. | Unit test: create 5 entities, attach `TagComp` to 3, call `each<TagComp>` with a counting callback that returns `true`, verify callback called exactly 3 times and return value is 3. |
| AC-008 | `World::each<T>()` skips pending-destroy entities. | Unit test: create entity with `TagComp`, mark for destroy, call `each<TagComp>`, verify callback NOT called. After flush, verify still not called (entity reclaimed). |
| AC-009 | `World::active_camera()` returns `std::optional<CameraComponent&>` — `std::nullopt` when no camera is registered, a reference to the active camera component when one is registered. | Unit test: fresh world returns `std::nullopt`. Register camera, returns reference to the `CameraComponent`. Unregister, returns `std::nullopt`. |
| AC-010 | `World::register_camera(CameraComponent&)` stores a reference to the component as the active camera. Last-registered-wins (calling with a different component replaces the previous). | Unit test: register component A, verify `active_camera()` returns reference to A. Register component B, verify `active_camera()` returns reference to B. |
| AC-011 | `World::unregister_camera(const CameraComponent&)` clears the active camera only if it refers to the same component (address comparison). Calling with a non-matching component is a no-op. | Unit test: register component A, unregister with a different component (different address), verify `active_camera()` still returns A. Unregister with A, verify `std::nullopt`. |
| AC-012 | `CameraComponent` exists in `src/engine/scene/camera_component.h`, inherits `Component`, holds a `math::Camera` member. | File compiles; `CameraComponent` is a valid type. |
| AC-013 | `CameraComponent` constructor accepts `const math::Camera&`. Default constructor creates a default `math::Camera`. | Both constructors compile. |
| AC-014 | `CameraComponent::camera()` returns references to the wrapped `math::Camera` (mutable and const overloads). | Unit test: modify camera settings through the reference and verify they persist. |
| AC-015 | `CameraComponent::on_attach()` calls `entity().world().register_camera(*this)`. | Unit test: add `CameraComponent` to an entity, verify `world.active_camera()` returns a reference to that component. |
| AC-016 | `CameraComponent` destructor calls `world_->unregister_camera(*this)` (with null guard). | Unit test: add `CameraComponent` then remove it via `remove_component`, verify `world.active_camera()` returns `std::nullopt`. |
| AC-017 | `MeshRenderer` exists in `src/engine/render/mesh_renderer.h`, inherits `Component`, holds a `std::shared_ptr<Model>`. | File compiles; `MeshRenderer` is a valid type. |
| AC-018 | `MeshRenderer` constructor takes `std::shared_ptr<Model>`. Accessor `model()` returns `Model&` (mutable and const). | Unit test: create `MeshRenderer` with a model, verify `model()` returns the same model. |
| AC-019 | `RenderSystem` exists in `src/engine/render/render_system.h`, constructor takes `RenderDevice&` and `World&`. | File compiles; `RenderSystem` is a valid type. |
| AC-020 | `RenderSystem::render()` calls `device_->begin_frame()` and `device_->end_frame()` exactly once each, regardless of whether a camera exists. | Unit test (headless): create world with no camera, call render. Verify `headless_device.frame_begin_count() == 1` and `headless_device.frame_end_count() == 1`. The headless backend exposes these counters publicly. |
| AC-021 | `RenderSystem::render()` with an active camera and MeshRenderer entities issues one draw call per MeshRenderer entity. | Unit test (headless): world with 2 entities (1 camera + 1 mesh renderer). Call render. Verify `headless_device.draw_call_count() == 1`. |
| AC-022 | `RenderSystem::render()` computes MVP as `camera.view_projection_matrix() * entity.world_matrix()` and sets it as `u_mvp` on the material. | Unit test (headless): material tracks `u_mvp` value. Create camera at origin looking down -Z, entity at (10,0,0) with identity rotation/scale. Verify the set `u_mvp` matrix translates by (10,0,0). |
| AC-023 | `RenderSystem::render()` logs a warning to `std::cerr` if no active camera exists, and still calls begin/end_frame. | Unit test (headless): capture `std::cerr` output (or verify no crash) — warning message contains "no active camera". Verify `headless_device.frame_begin_count() == 1`. |
| AC-024 | `RenderSystem::render()` logs a warning if `set_uniform("u_mvp", ...)` fails and skips that entity, continuing to other entities. | Unit test (headless): create two MeshRenderer entities, one with a material lacking `u_mvp`. Call render, verify `headless_device.draw_call_count() == 1` (only the valid entity is drawn). No crash. |
| AC-025 | The new `cube_scene_demo` creates a `World`, an `Entity` with both `CameraComponent` and `MeshRenderer`, and a `RenderSystem`. No standalone `Camera` or manual MVP computation in the new demo. `cube_demo.cpp` and `cube_demo.h` are left exactly as they were before SPEC-011. | Code review confirms the new structure and verifies existing demo files are unmodified. |
| AC-026 | The `cube_scene_demo` rotates the entity by updating `entity.transform().rotation` each frame before calling `render_system.render()`. | Code review confirms rotation update before render call. |
| AC-027 | `World::each<T>()` is safe to call with no matching entities (zero iterations, no crash). | Unit test: call `each<CameraComponent>` on a world with entities but no CameraComponent. No crash, callback never invoked. |
| AC-028 | `CameraComponent` destroys successfully (destructor runs) even if `world()` is already destroyed (e.g., `World` destroyed before component — the destructor must handle a stale `world_` pointer safely OR the spec documents that the World must outlive all components). | Decision: The `World` destructor destroys all entities and their components. `CameraComponent` destructor only runs during World destruction or remove_component. During World destruction, `world_` is still valid (the World object still exists). No stale-world case occurs for properly managed lifecycles. Documented in assumptions. |
| AC-029 | `World::each<T>()` returns early and safely if the world has no entities (empty `slots_` vector). | Unit test: call `each<MeshRenderer>` on a fresh World with no entities. No crash, callback never invoked, return value is 0. |
| AC-030 | `World::each<T>()` early-exit works: when the callback returns `false`, iteration stops and no further entities are visited. | Unit test: 5 entities with `TagComp`, callback returns `false` on the 3rd. Verify only 3 callbacks, return value is 3. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An engine developer can render any entity with a `MeshRenderer` and an active camera using exactly one API call (`RenderSystem::render()`) — no manual MVP, no manual camera management. | Code review of a minimal program: create world, entity, add camera + MeshRenderer, create RenderSystem, call render in loop. |
| SC-002 | All new tests pass in headless CI (no GPU, no display). | `cmake --build --preset debug && ctest --preset debug` — all scene-rendering tests pass. |
| SC-003 | The new `cube_scene_demo` runs correctly with the scene-based approach — same visual result as the original manual version. | The new scene-based demo runs and displays a rotating coloured cube, visually identical to the original `cube_demo`. Manual visual verification. |
| SC-004 | No memory leaks in any code path (component lifecycle, camera registration, system creation/destruction). | ASAN build shows no leaks in a test that creates and destroys 100 worlds with cameras and mesh renderers. |
| SC-005 | Entity hierarchy is respected by `RenderSystem`: a child entity's world transform includes its parent's transform. | Unit test: parent at (10,0,0) with child at (0,0,0) → child's world position is (10,0,0). RenderSystem MVP reflects this. |

## Edge cases

| Case | Expected behavior |
|---|---|
| `World::each<T>()` called on an empty world | No crash, callback never invoked. |
| `World::each<T>()` iterates while an entity's component list is being modified (add/remove in callback) | Undefined behaviour (it's likely accessing a `vector` that is being reallocated or modified). The spec documents this precondition. |
| `CameraComponent::on_attach()` called but `entity()` component list is not yet fully settled | `on_attach()` is called after the component is already pushed into the entity's `components_` vector. The component is fully part of the entity. Safe to call `entity().world().register_camera()`. |
| `CameraComponent` destructor called during `World::~World()` | `world_` is still a valid pointer (the World object exists). `unregister_camera()` is safe. |
| `CameraComponent` destructor called after `remove_component<CameraComponent>()` | Normal flow. Destructor runs during `remove_component`. `world_` and `entity_id_` are still valid. |
| `MeshRenderer` with a null `shared_ptr<Model>` (default-constructed) | Not possible via the public constructor (requires `shared_ptr<Model>`). If one exists, calling `model()` is undefined behaviour. |
| `active_camera()` holds a `std::optional<CameraComponent&>` to a `CameraComponent` that has been destroyed (e.g. via `remove_component` without proper unregistration) | This is undefined behaviour (dangling reference). Callers must ensure that a `CameraComponent` outlives its registration as the active camera. The destructor of `CameraComponent` handles unregistration automatically; manual destruction without `remove_component` is a caller error. |
| A destroyed entity's `CameraComponent` is still registered as active camera before flush | The `CameraComponent` destructor runs during entity destruction (when the entity's component vector is cleared) and calls `unregister_camera(*this)`, which clears `active_camera_` before the entity is flushed. This is safe. |
| Multiple `CameraComponent` instances on different entities | Last-registered-wins. Each `on_attach()` calls `register_camera(*this)` which replaces the previous. Each destructor calls `unregister_camera(*this)`. If A is registered, then B is registered, then A is destroyed: B remains the active camera (since `unregister_camera(component_A)` does not clear B because the active camera already points to B). |
| Multiple `CameraComponent` instances on the same entity | Not possible — at most one component of each type per entity (existing constraint, UB if attempted). |
| `World::each<T>` with T being a non-Component type | Template instantiation will fail at compile time when `dynamic_cast<T*>` is applied (since T must be a polymorphic type deriving from Component). The `dynamic_cast` will produce a compile error or well-defined null. Prefer to document: T must derive from Component. |
| `World::register_camera()` called with a `CameraComponent` that belongs to a different `World` | Undefined behaviour (cross-world operation). The caller must ensure the component belongs to the same world. Debug builds may assert. |
| `RenderSystem` created, world is destroyed first, then `render()` is called | Undefined behaviour (dangling world_ pointer). The caller must ensure World outlives the RenderSystem. |
| `RenderSystem` created, device is destroyed first, then `render()` is called | Undefined behaviour (dangling device_ pointer). The caller must ensure RenderDevice outlives the RenderSystem. |
| Entity with MeshRenderer is destroyed and flushed during a frame's render call | Not possible — `RenderSystem::render()` is not re-entrant, and `destroy()` during `each<>()` callback is UB (modifying the slots list during iteration). |
| `register_camera(CameraComponent&)` / `unregister_camera(const CameraComponent&)` called during `World::each<T>()` iteration | Safe. Camera registration modifies `active_camera_` (a `std::optional<CameraComponent&>`), not the `slots_` array or any entity's component list. The `each<T>()` iteration is not affected. The new camera will be picked up on the next frame. |
| `World::each<T>` with a lambda that captures by reference and modifies entity state (e.g., transform) | Safe. The iteration only locks the `slots_` array, not the components themselves. Modifying a component's data or the entity's transform during iteration is legal (the data is not being read by the iteration mechanism). |
| Entity hierarchy with deeply nested transforms (10,000 levels) | `entity.world_matrix()` uses a fixed-size stack array (4096 entries) per `Transform::world_matrix()`. Beyond 4096, the algorithm uses a secondary loop. `World::each<T>` is unaffected. |

## Error cases

| Case | Expected behavior |
|---|---|
| `CameraComponent::on_attach()` called but `entity().world()` is null | Cannot happen — `on_attach()` is only called from `World::add_component()`, which sets `world_` before calling it. If it somehow happens, dereferencing `world_` is UB. |
| `RenderSystem::render()` called with no `RenderDevice` (device_ is null) | Undefined behaviour (dangling reference). The constructor requires a valid reference. |
| `set_uniform("u_mvp", ...)` fails during `RenderSystem::render()` | The entity is skipped (draw call not issued for that entity). A warning is logged to `std::cerr`. Other entities are rendered normally. |
| `World::active_camera()` returns an engaged `optional` but the `CameraComponent` reference is dangling | This is undefined behaviour. It can only happen if a `CameraComponent` was destroyed without going through the normal entity destruction or `remove_component` paths (which trigger the destructor and thus `unregister_camera`). |
| `World::register_camera()` called with a `CameraComponent` that belongs to a different `World` | Undefined behaviour (cross-world operation). The caller must ensure the component belongs to the same world. Debug builds may assert. |
| `World::unregister_camera()` called with a `CameraComponent` that belongs to a different `World` | Undefined behaviour (cross-world operation). The caller must ensure the component belongs to the same world. Debug builds may assert. |
| `CameraComponent` constructed but never added to an entity — destructor runs | `world_` is `nullptr`. The destructor must guard against this. Implementation: `if (world_) { world_->unregister_camera(*this); }`. |

## Permissions and security

- No elevated privileges required.
- No network, filesystem, or secret access involved.
- No new dependencies on third-party libraries beyond those already used by `scene/` and `render/`.
- The architecture boundary (CONST-001) is maintained: `render/` headers that expose types used outside the engine (e.g., `MeshRenderer`) must not leak backend-specific types (OpenGL, SDL3, GLM).
- `camera_component.h` (in `scene/`) includes `math/camera.h` which depends solely on math types — no backend headers.
- `mesh_renderer.h` (in `render/`) includes `render/model.h` and `scene/component.h` — both are abstract headers with no backend types.
- `render_system.h` (in `render/`) forward-declares `World` and `RenderDevice` — no backend leak.

## Observability

All observability uses `std::cerr` consistent with the project pattern.

| Signal | Source |
|---|---|
| `RenderSystem::render()` — no active camera | `std::cerr << "RenderSystem: no active camera — rendering skipped\n"` |
| `RenderSystem::render()` — `set_uniform(u_mvp)` failure per entity | `std::cerr << "RenderSystem: set_uniform(u_mvp) failed for entity " << id << ": " << error << "\n"` |
| `CameraComponent` on_attach success | `std::cerr << "CameraComponent: registered entity " << id << " as active camera\n"` (debug builds only) |
| `CameraComponent` destructor unregister | `std::cerr << "CameraComponent: unregistered entity " << id << "\n"` (debug builds only) |
| `World::register_camera()` / `unregister_camera()` | `std::cerr` output for diagnostics (debug builds only) |
| `World::each<T>()` iteration count | Not logged in v1 (hot path). |

## Out of scope

- Camera system (follow/look-at/orbit behaviors) — the camera is a static component.
- Multi-camera rendering (split-screen, picture-in-picture, camera cuts).
- Frustum culling or visibility determination.
- Render graph, render passes, or post-processing effects.
- Material instancing or draw-call batching.
- Component pooling, archetype ECS, or flat-array storage.
- `on_detach()` lifecycle hook (destructor is sufficient for v1).
- Component enable/disable toggling.
- Layer or tag filtering for `MeshRenderer` selection.
- Render order sorting (opaque-only, draws in iteration order).
- Shadow maps, lighting components, or light culling.
- Debug rendering (entity bounds, axes, or selection outlines).
- Per-entity material overrides or material property blocks.
- Entity transform caching or dirty flags (v1 recomputes `world_matrix()` each call).
- Automatic `u_mvp` binding through shader reflection or uniform buffers.
- Support for multiple `Model` references in a single `MeshRenderer`.
- Dynamic creation or removal of `MeshRenderer` during `RenderSystem::render()`.
- `World::each<T>()` with `const T&` callback (const-correct version can be added later).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `math::Camera` is stable and fully implemented in `src/engine/math/camera.h`. `view_matrix()`, `projection_matrix()`, and `view_projection_matrix()` are implemented and return `math::Mat4`. |
| A-02 | `Quat::from_axis_angle(axis, angle)` is available for the cube demo's rotation update. If not available, the demo uses the existing `Mat4::rotate(angle, axis)` pattern but applies it to the transform's rotation quaternion via equivalent math. |
| A-03 | `Model` is movable (move constructor and move assignment are `noexcept`). `setup_cube` returns `CubeResources` by value, and `cube.model` is moved into `MeshRenderer` via `std::move(cube.model)`. |
| A-04 | The `Material::set_uniform(name, value)` API returns `Result<void>` (as specified in SPEC-005). On failure, it returns an `Error` — this error is logged and the entity is skipped. |
| A-05 | The `Buddd Engine` uses the C++26 standard, supporting `std::optional<T&>` (or a polyfill equivalent). |
| A-06 | `World` must outlive `RenderSystem` and all `CameraComponent` / `MeshRenderer` instances. The `World` destructor destroys all entities and their components, and during this destruction `world_` pointers in components remain valid (the `World` object still exists). |
| A-07 | `RenderDevice` must outlive `RenderSystem`. The system stores a non-owning pointer. |
| A-08 | `World::each<T>()` does **not** guard against concurrent modification of the entity tree or component lists during iteration. The caller must not add/remove entities or components in the callback. |
| A-09 | `Entity` is 16 bytes and trivially copyable-adjacent. Passing by value in `each<>()` callbacks is cheap. |
| A-10 | The `render/` module already links against the `scene/` module sources (they are both compiled into `buddd_engine` via `file(GLOB_RECURSE)`). No CMake changes are needed beyond adding new `.h` and `.cpp` files. |
| A-11 | New files to be created:
- `src/engine/scene/camera_component.h`
- `src/engine/scene/camera_component.cpp`
- `src/engine/render/mesh_renderer.h`
- `src/engine/render/mesh_renderer.cpp`
- `src/engine/render/render_system.h`
- `src/engine/render/render_system.cpp`
- `src/cmd/demo/cube_scene_demo.h`
- `src/cmd/demo/cube_scene_demo.cpp`
Modified files:
- `src/engine/scene/component.h` — add entity awareness
- `src/engine/scene/world.h` — add `each<T>()`, camera registration (with `CameraComponent` forward declaration, `<optional>` include)
- `src/engine/scene/entity.h` — add `friend class Component` |
| A-12 | The `Godbolt` / `Compiler Explorer` reference for `dynamic_cast` in iteration is acceptable for v1 performance. O(n) scan with dynamic_cast is not intended for production use with thousands of entities but is sufficient for the current scale. |
| A-13 | `EntityId` sentinel `EntityId::none()` is `{ UINT32_MAX, UINT32_MAX }` (from SPEC-008). The `Component::entity_id_` default is `EntityId::none()`, and `Component::entity()` on an unattached component is UB. |
| A-14 | Tests for SPEC-011 acceptance criteria live in a new file `tests/scene_rendering_tests.cpp`, following the project convention of one test file per spec. |
| A-15 | The `#pragma once` include guard convention is used for all new headers, consistent with the existing codebase. |
| A-16 | The cube demo's `setup_cube()` returns a `CubeResources` struct. The `Model` inside is moved into `MeshRenderer`. The `shared_ptr<Material>` remains alive through the `MeshRenderer`'s `shared_ptr<Model>` which holds a `shared_ptr<Material>`. The original `CubeResources::material` shared_ptr may be discarded after `setup_cube` returns. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | [RESOLVED] Where should `Component::entity()` be defined? **Resolution**: In `entity.h` after `Entity` is fully defined (same pattern as `Transform::world_matrix()`). `Entity` gains `friend class Component` to allow construction of Entity handles. | API structure and access control. |
| Q-02 | [RESOLVED] Where do `CameraComponent`, `MeshRenderer`, and `RenderSystem` live? **Resolution**: `CameraComponent` in `scene/` (depends on `math/`, no render dependency). `MeshRenderer` in `render/` (depends on `scene/` for `Component`). `RenderSystem` in `render/` (depends on both). | Module dependency graph. |
| Q-03 | [RESOLVED] The `CubeResources::material` shared_ptr is redundant — `Model` already holds `shared_ptr<Material>` (confirmed by `model.h`). The RenderSystem accesses the material via `mr.model().material()`. The separate shared_ptr in `CubeResources` can be discarded after the Model is moved. **Resolution**: Verified by code inspection. | Demo code simplicity. |
| Q-04 | [RESOLVED] Hardcode `"u_mvp"` as the uniform name for v1. Consistent with SPEC-009 and existing demo code. If future materials need different uniform names, `RenderSystem` can be extended with a configurable name or multiple uniform bindings. **Resolution**: Hardcoded for v1. | API flexibility vs simplicity for v1. |
| Q-05 | [RESOLVED] `World::each<T>()` supports early-exit: the callback returns `bool` (`true` = continue, `false` = stop). The method returns `size_t` (number of matches visited). **Resolution**: Approved by human. | Query API completeness. |
| Q-06 | [RESOLVED] No caching. `RenderSystem::render()` calls `view_projection_matrix()` once per frame, and the `Camera` math type recomputes matrices on each call. This is negligible for v1. **Resolution**: No caching. | Performance vs simplicity. |
| Q-07 | [RESOLVED] The World destructor destroys all entities and their components while the World object is still alive, so `world_` pointers remain valid during component destruction. After `~World()` returns, any component pointer is dangling — consistent with SPEC-008 lifecycle rules. **Resolution**: Documented in assumptions (A-06) and AC-028. | Documentation and safety. |
