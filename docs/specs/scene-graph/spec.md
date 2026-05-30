# SPEC-008 — Scene Graph: World, Entity, Transform, Components, Hierarchy

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|
| Approved by | |
| Date | |
| Time | |

## Problem

The Buddd Engine has no way to represent objects in a 3D world. Without a scene graph:

- There is no container for entities, transforms, and components — every rendering or gameplay path would need to invent its own storage.
- There is no hierarchy to compute parent-relative or world-space transforms — cameras, models, lights, and effects cannot be composed spatially.
- There is no `Transform` abstraction — position, rotation, and scale are loose floats with no uniform access pattern.
- There is no `Entity` identity — objects cannot be referenced, queried, or destroyed in a safe way.
- Every feature that follows (camera attachment to entities, model rendering, physics colliders, audio sources) depends on a shared scene graph foundation.

## Goals

- Provide a `World` container as the top-level scope in which entities live. Multiple conceptual "scenes" can coexist in a single `World` as root-level entity subtrees.
- Provide an `Entity` lightweight handle type (16 bytes: `World*` + `EntityId`) that wraps a handle and delegates operations to `World`.
- Provide an `EntityId` small handle type (`uint32_t index` + `uint32_t generation`) for safe dangling-reference detection.
- Provide a `Transform` struct (position/Vec3, rotation/Quat, scale/Vec3) that is mandatory for every entity and accessible via `entity.transform()`.
- Provide a `Component` polymorphic base class (virtual destructor only) and per-entity storage via `entity.add_component<T>()`, `entity.get_component<T>()`, `entity.remove_component<T>()`.
- Provide deferred destruction: `entity.destroy()` marks an entity and its children for removal; `world.flush_destroyed()` actually reclaims resources.
- Provide hierarchy: parent pointer, children list, `entity.create_child()`, `entity.reparent(new_parent)`.
- Provide `Transform::local_matrix()` and `Transform::world_matrix(entity)` that compute T×R×S from local and chain up the hierarchy.
- Place all types under `src/engine/scene/` in namespace `buddd::engine`.
- Design the public API so it is forward-compatible with future flat-array storage (ECS-like) — the storage strategy is hidden behind the `World` implementation.

## Non-goals

- No ECS flat-array / archetype optimization — v1 uses `std::vector<std::unique_ptr<Component>>` per entity.
- No mesh, model, or geometry abstractions (future spec).
- No camera integration with the scene graph (future spec — `Camera` is currently a standalone math type per SPEC-004; attaching cameras to entities is deferred).
- No rendering, draw calls, or material system integration.
- No serialization or deserialization of world state.
- No event system or component lifecycle hooks (`on_attach`/`on_detach` are deferred).
- No physics, collision detection, or audio.
- No scripting or reflection system.
- No multi-world support in v1 (a single `World` object per application).
- No threading or parallel entity iteration — all operations are single-threaded in v1.
- No editor, inspector, or GUI tooling.

## Actors

| Actor | Description |
|---|---|
| Engine developer | A developer adding engine features that require spatial objects — rendering, cameras, lights, physics, audio. Creates entities, attaches components, builds hierarchies, computes world matrices. |
| Application developer | A developer building on top of the engine who needs to place objects in a 3D world. Uses `Entity`, `Transform`, and `Component` to compose game objects. |
| Test suite | Catch2 v3 tests that verify entity lifecycle, component management, transform computation, hierarchy operations, and deferred destruction — no display or GPU required. |

## User-visible behavior

### Type overview

All scene graph types live under `src/engine/scene/` in namespace `buddd::engine`. They depend on the math types (`Vec3`, `Quat`, `Mat4`) from `src/engine/math/` (SPEC-004).

| C++ type | Header | Purpose |
|---|---|---|
| `EntityId` | `scene/entity_id.h` | Small handle (index + generation) for safe entity references |
| `Transform` | `scene/transform.h` | Position, rotation, scale with local and world matrix computation |
| `Entity` | `scene/entity.h` | Lightweight handle (16 bytes) wrapping `World*` + `EntityId` |
| `Component` | `scene/component.h` | Polymorphic base class (virtual destructor only) |
| `World` | `scene/world.h` | Top-level container managing entity lifecycle, hierarchy, and component storage |

### EntityId

```cpp
namespace buddd::engine {

struct EntityId {
    uint32_t index;       // slot index
    uint32_t generation;  // incremented on reuse for stale-handle detection

    /// Returns a sentinel EntityId representing "no entity".
    static constexpr auto none() noexcept -> EntityId;

    auto operator==(const EntityId&) const noexcept -> bool = default;
    auto operator!=(const EntityId&) const noexcept -> bool = default;
};

static_assert(std::is_trivially_copyable_v<EntityId>);
static_assert(sizeof(EntityId) == 8);

} // namespace buddd::engine
```

- `EntityId::none()` returns `{ UINT32_MAX, UINT32_MAX }` (or similar sentinel). Comparison with `none()` checks for null/invalid handles.
- `EntityId` is trivially copyable and 8 bytes.
- The generation counter is incremented each time a slot is reused after `flush_destroyed()`. A stale `EntityId` (pointing to a recycled slot with a different generation) is detectable by comparing generations.

### Transform

```cpp
namespace buddd::engine {

struct Transform {
    Vec3 position{Vec3::zero()};
    Quat rotation{Quat::identity()};
    Vec3 scale{Vec3::one()};

    /// Returns the local transformation matrix: T(position) * R(rotation) * S(scale).
    /// Equivalent to Mat4::translate(position) * rotation.to_mat4() * Mat4::scale(scale).
    auto local_matrix() const -> math::Mat4;

    /// Returns the world transformation matrix by walking the parent chain
    /// and accumulating local transforms: parent.world_matrix() * local_matrix().
    /// For root entities (no parent), this is equivalent to local_matrix().
    /// Walks up to the root each call — no caching in v1.
    auto world_matrix(const Entity& entity) const -> math::Mat4;
};

} // namespace buddd::engine
```

- `Transform` is a value type stored inline in the entity node (not heap-allocated).
- `world_matrix()` walks the parent chain each call. No caching or dirty-flag optimization in v1.
- Transformation order is T×R×S (translate, then rotate, then scale), matching the standard affine transform convention for game engines.

### Component

```cpp
namespace buddd::engine {

class Component {
public:
    virtual ~Component() = default;

    // Non-copyable, non-movable (identity matters once attached to an entity)
    Component(const Component&) = delete;
    auto operator=(const Component&) -> Component& = delete;
    Component(Component&&) = delete;
    auto operator=(Component&&) -> Component& = delete;

protected:
    Component() = default;
};

} // namespace buddd::engine
```

- `Component` is intentionally minimal — just a virtual destructor. Lifecycle hooks (`on_attach`, `on_detach`, `on_enable`, `on_disable`) are deferred to a future spec.
- Non-copyable, non-movable. Once attached to an entity, a component has identity tied to that entity.
- Concrete components inherit from `Component` publicly.
- An entity can have at most one component of each type `T` (singleton-per-type model, matching Unity's `GetComponent<T>()` semantics).

### Entity

```cpp
namespace buddd::engine {

class Entity {
public:
    // ── Identity ──
    /// Returns the unique handle for this entity.
    auto id() const noexcept -> EntityId;

    /// Returns a reference to the owning World.
    auto world() const noexcept -> World&;

    /// Returns a null sentinel Entity (default-constructed).
    static auto none() noexcept -> Entity { return Entity{}; }

    // ── Lifecycle ──
    /// Marks this entity (and all descendants) for deferred destruction.
    /// Does not reclaim resources immediately — call World::flush_destroyed().
    /// Safe to call multiple times on the same entity (idempotent).
    /// Behavior is undefined if called on a null entity.
    void destroy();

    /// Returns true if this entity has been marked for destruction
    /// (via destroy()) but has not yet been flushed.
    /// Safe to call on a null entity (returns false).
    auto is_pending_destroy() const noexcept -> bool;

    // ── Transform (always valid for every entity) ──
    /// Returns a reference to the entity's Transform.
    /// Behavior is undefined if called on a null entity.
    auto transform() noexcept -> Transform&;
    /// Behavior is undefined if called on a null entity.
    auto transform() const noexcept -> const Transform&;

    // ── Components ──
    /// Creates and attaches a new component of type T.
    /// Args are forwarded to T's constructor.
    /// Returns a reference to the newly created component.
    /// Behavior is undefined if a component of type T already exists on this entity
    /// or if called on a null or pending-destroy entity.
    template<typename T, typename... Args>
    auto add_component(Args&&... args) -> T&;

    /// Returns an optional reference to the component of type T, or std::nullopt if absent.
    /// Returns std::nullopt for null or pending-destroy entities.
    /// Uses std::optional<T&> (C++26) for type-safe optional reference semantics.
    template<typename T>
    auto get_component() const noexcept -> std::optional<const T&>;
    template<typename T>
    auto get_component() noexcept -> std::optional<T&>;

    /// Removes and destroys the component of type T.
    /// Returns true if a component was removed, false if none existed.
    /// Behavior is undefined if called on a null or pending-destroy entity.
    template<typename T>
    auto remove_component() -> bool;

    // ── Hierarchy ──
    /// Returns the parent entity, or a null Entity (id() == EntityId::none()) if root.
    /// Behavior is undefined if called on a null entity.
    auto parent() const noexcept -> Entity;

    /// Returns the number of direct children.
    /// Behavior is undefined if called on a null entity.
    auto child_count() const noexcept -> size_t;

    /// Returns the child at the given index (0 <= index < child_count()).
    /// Behavior is undefined if index is out of bounds.
    auto get_child(size_t index) const noexcept -> Entity;

    /// Creates a new entity as a child of this entity.
    /// The new entity is created with identity transform.
    /// Returns the created child Entity.
    /// Behavior is undefined if called on a null or pending-destroy entity.
    auto create_child() -> Entity;

    /// Reparents this entity to new_parent.
    /// If new_parent is null (new_parent.id() == EntityId::none()), this entity
    /// becomes a root entity.
    /// If new_parent is the same as the current parent, this is a no-op.
    /// Behavior is undefined if:
    ///   - new_parent is this entity or a descendant of this entity (cycle).
    ///   - new_parent belongs to a different World.
    ///   - called on a null or pending-destroy entity.
    void reparent(Entity new_parent);

    /// Convenience: returns the world-space transform matrix for this entity.
    /// Equivalent to transform().world_matrix(*this).
    /// Behavior is undefined if called on a null entity.
    auto world_matrix() const noexcept -> math::Mat4;

    // ── Comparison ──
    friend auto operator==(const Entity& a, const Entity& b) noexcept -> bool;
    // operator!= is synthesized from operator== in C++20.

    // ── Factory ──
    /// Creates a root entity (no parent) in the given world.
    static auto create(World& world) -> Entity;

private:
    World* world_ = nullptr;
    EntityId id_ = EntityId::none();

    // Only World may construct non-null Entity objects.
    friend class World;
    Entity(World& world, EntityId id) noexcept;
};

static_assert(sizeof(Entity) == 16);  // pointer + EntityId

} // namespace buddd::engine
```

- `Entity` is a lightweight handle (16 bytes). It does not own the entity data — `World` does.
- `Entity::create(world)` is the only public way to create entities. It delegates to `World::create_entity()`.
- `Entity::none()` returns a default-constructed (null) Entity. A null entity has `id() == EntityId::none()`.
- Comparison operators compare `world_` and `id_` — two `Entity` values are equal if they point to the same entity in the same `World`.
- A default-constructed `Entity` is a null entity. Safe operations on a null entity: `id()`, `world()`, `operator==`, `operator!=`, `is_pending_destroy()` (returns false), `none()` (static). All other operations are undefined behavior.
- `add_component<T>()` for a type that already exists is undefined behavior (caller should check with `get_component<T>()` first, or the implementation may assert in debug builds).
- **Pending-destroy entity contract**: Read-only operations on the destroyed entity itself:
  - `get_component<T>()` → `std::nullopt`
  - `is_pending_destroy()` → `true`
  - `id()`, `parent()`, `transform()` → return pre-destroy values (safe to read)
  - `destroy()` → idempotent no-op
  - All mutating operations (`add_component`, `remove_component`, `create_child`, `reparent`) → undefined behavior.
- **Parent's view of a destroyed child**: After `destroy()` and before `flush_destroyed()`, the destroyed entity **remains visible** in its parent's `child_count()` and `get_child()` — it is still logically part of the tree until flushed. This ensures that any code iterating the children list between `destroy()` and `flush_destroyed()` sees a consistent view. The entity is unlinked from its parent during `flush_destroyed()`, not at mark time.
- **Flush order**: `flush_destroyed()` processes entities in reverse depth order — deepest children first, then their parents. This ensures that during cleanup, a parent entity is still valid when a child's components are destroyed. The pending_destroy_ list is populated in pre-order (parent before children) during `destroy()`, so reversing it yields post-order (children before parent).
- **Component lifecycle**: Components are destroyed when their owning entity's storage is reclaimed during `flush_destroyed()` (or during `~World()`). Component destructors run in reverse order of addition (LIFO). The entity's slot is freed after all component destructors have completed.
- **Dangling reference safety**: `Entity` handles are 8-byte `EntityId` values with generation counters. After an entity is flushed, its slot's generation is incremented, making any pre-existing `EntityId` stale — detectable by `lookup_node()` returning null. `Component*` pointers (obtained from `get_component<T>()`) become dangling after the component is removed or the entity is flushed — using them is undefined behavior. The recommended pattern is to call `get_component<T>()` each time access is needed rather than caching raw pointers across mutation points. The design rationale: a pending-destroy entity is observable but not mutable.

### World

```cpp
namespace buddd::engine {

class World {
public:
    World();
    ~World();

    // Non-copyable, non-movable (World is the unique owner of all entities)
    World(const World&) = delete;
    auto operator=(const World&) -> World& = delete;
    World(World&&) = delete;
    auto operator=(World&&) -> World& = delete;

    // ── Entity lifecycle ──
    /// Creates a root entity (parent = EntityId::none()) with identity transform.
    /// Returns a handle to the new entity.
    auto create_entity() -> Entity;

    /// Marks an entity and all its descendants for deferred destruction.
    /// Equivalent to entity.destroy().
    void destroy_entity(Entity entity);

    /// Actually reclaims all entities that were marked for destruction.
    /// After this call, all handles to destroyed entities are stale
    /// (generation mismatch) — using them is undefined behavior.
    /// Safe to call when no entities are pending (no-op).
    /// Must not be called from within a user callback that is iterating
    /// the entity tree (would invalidate iterators).
    void flush_destroyed() noexcept;

    // ── Internal (called by Entity) ──
    auto get_transform(EntityId id) noexcept -> Transform&;
    auto get_transform(EntityId id) const noexcept -> const Transform&;

private:
    // Internal storage: tree of entity nodes (v1: unique_ptr tree).
    // Exact structure is implementation-defined.
    // ...
};

} // namespace buddd::engine
```

- `World` is the sole owner of all entity data. Destroying a `World` destroys all entities and components.
- `flush_destroyed()` iterates the flat `pending_destroy_` list, reclaims each entity's resources, and updates generation counters for reuse.
- After `flush_destroyed()`, any `Entity` handle that referred to a destroyed entity has a stale generation; using it is undefined behavior (detectable by checking `EntityId::generation` against the slot's current generation).

## User stories

### Story 1 — Create and use entities (Priority: P1)

As an application developer, I want to create entities in a world, read and modify their transforms, and destroy them, so that I can place and remove objects in the scene.

**Given** a `World` instance
**When** I write:
```cpp
auto& world = */* ... */;
auto entity = Entity::create(world);
entity.transform().position = Vec3(1.0f, 2.0f, 3.0f);
auto pos = entity.transform().position;
entity.destroy();
world.flush_destroyed();
```
**Then** the entity is created with identity transform, position is set and read correctly, and after `flush_destroyed()` the entity's storage is reclaimed.

### Story 2 — Create and traverse a hierarchy (Priority: P1)

As an engine developer, I want to create parent-child entity hierarchies, reparent entities, and compute world transforms, so that objects move together as a group.

**Given** a `World` with a root entity `parent` and a child `child = parent.create_child()`
**When** I set `parent.transform().position = Vec3(10.0f, 0.0f, 0.0f)` and compute `child.transform().world_matrix(child)`
**Then** the world matrix of `child` includes the parent's translation.

**When** I call `child.reparent(Entity::none())` (make root)
**Then** `child.parent().id() == EntityId::none()` and `child` appears in the world's root entities.

**When** I call `child.reparent(parent)` to reattach
**Then** `child.parent()` returns `parent`, `parent.child_count() == 1`, and `parent.get_child(0) == child`.

### Story 3 — Add, retrieve, and remove components (Priority: P1)

As an engine developer, I want to attach custom components to entities, retrieve them by type, and remove them, so that I can extend entities with gameplay data.

**Given** a custom component type `struct HealthComponent : public Component { int hp; };`
**When** I write:
```cpp
auto entity = Entity::create(world);
auto& health = entity.add_component<HealthComponent>(100);
auto opt = entity.get_component<HealthComponent>();
bool removed = entity.remove_component<HealthComponent>();
auto after = entity.get_component<HealthComponent>();
```
**Then** `opt.has_value()` is `true` and `opt->hp` equals `100` (same component as `health`), `removed` is `true`, and `after.has_value()` is `false`.

### Story 4 — Deferred destruction cascades to children (Priority: P2)

As an application developer, I want destroying a parent to also destroy its children, so that whole subtrees are cleaned up consistently.

**Given** a root entity `parent` with children `child1` and `child2`, and `child1` has a grandchild `grandchild`
**When** `parent.destroy()` is called followed by `world.flush_destroyed()`
**Then** all four entities (`parent`, `child1`, `child2`, `grandchild`) are destroyed and their storage is reclaimed.

### Story 5 — Stale handle detection via generation (Priority: P2)

As an engine developer, I want stale entity handles to be detectable (generation mismatch), so that dangling references do not silently corrupt memory.

**Given** an entity with `EntityId id = entity.id()` and a known `generation`
**When** the entity is destroyed and `flush_destroyed()` is called
**Then** if the slot is reused for a new entity, the new entity has a different `EntityId::generation`.
If a stale handle is used, behavior is undefined (detectable in debug builds via generation assertion).

### Story 6 — Reparenting preserves world transform (deferred) (Priority: P3)

As an application developer, I want the ability to reparent an entity while preserving its world-space transform (i.e., adjusting local transform so the entity does not visually move). This is a convenience method deferred to a future spec. In v1, `reparent()` simply changes the parent link without adjusting the local transform.

**Given** an entity `child` at local position `(5, 0, 0)` with parent `old_parent` at `(10, 0, 0)` (so world position is `(15, 0, 0)`)
**When** `child.reparent(new_parent)` is called where `new_parent` is at world position `(0, 0, 0)`
**Then** `child`'s local position remains `(5, 0, 0)` and its world position changes to `(5, 0, 0)` — no automatic world-space preservation.

### Story 7 — Destroyed entity still visible in parent until flush (Priority: P2)

As an application developer, I want a destroyed entity to remain visible in its parent's `child_count()` and `get_child()` between `destroy()` and `flush_destroyed()`, so that iteration over the parent's children is not disrupted during the mark phase.

**Given** a parent entity with two children C1 and C2
**When** I call `C1.destroy()`
**Then** `parent.child_count() == 2` — C1 is still counted before flush.
**And** `C1.is_pending_destroy() == true` — the entity is marked.
**And** `C1.get_component<MyComp>()` returns `std::nullopt` — read-only safe, component access disabled.
**When** I call `world.flush_destroyed()`
**Then** `parent.child_count() == 1` and `parent.get_child(0) == C2` — C1 is removed after flush.

### Story 8 — Flush destroys in reverse depth order (deepest first) (Priority: P2)

As an engine developer, I want `flush_destroyed()` to destroy entities in reverse depth order (deepest child first, then parent), so that component destructors see valid ancestors during cleanup.

**Given** a chain root → parent → child (depth 3)
**When** I call `root.destroy()` followed by `world.flush_destroyed()`
**Then** the destruction order inside `flush_destroyed()` is: child (depth 3), parent (depth 2), root (depth 1) — deepest first.
**And** during each entity's component destructors, the parent entity is still accessible (not yet reclaimed).

### Story 9 — Components are destroyed with their entity (Priority: P1)

As a component author, I want my component's destructor to be called when the owning entity is destroyed (either via `entity.destroy()` + `flush_destroyed()`, or via `World` destruction), so that I can release resources owned by the component.

**Given** a component type `struct ResourceHolder : Component { ~ResourceHolder() { /* cleanup */ } };`
**When** I attach it to an entity and the entity is destroyed and flushed
**Then** the `ResourceHolder` destructor is called during `flush_destroyed()` (or during `~World()` if the world is destroyed first).
**And** The destructor order within a single entity is: all components are destroyed in reverse order of addition (LIFO), then the entity's slot is reclaimed.

### Story 10 — Dangling references after entity/component destruction (Priority: P3)

As an engine developer, I want to understand the lifetime rules for handles and pointers to entities and components, so that I can avoid dangling references.

**Given** an entity handle `Entity h = entity.id()` and a component pointer `auto* comp = entity.get_component<T>().operator->()`
**When** the entity is destroyed and flushed
**Then** `h` is a stale handle: `World::lookup_node(h)` returns `nullptr` (generation mismatch). Using `h` in any operation except `id()` or comparison is undefined behavior.
**And** `comp` is a dangling pointer. Any access through it is undefined behavior.

**Given** an entity handle `h` to a pending-destroy entity (destroyed but not flushed)
**When** I call `h.is_pending_destroy()` — returns `true` (safe, read-only).
**When** I call `h.get_component<T>()` — returns `std::nullopt` (safe, read-only).
**When** I call `h.destroy()` — no-op / idempotent (safe).
**When** I call `h.add_component<T>(args)` — undefined behavior (mutating).
**Then** the pending-destroy contract (read-only safe, mutating UB) applies.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `EntityId` exists with `index` (`uint32_t`) and `generation` (`uint32_t`) public members, `none()` static method, `==` and `!=`. `sizeof(EntityId) == 8`. Trivially copyable. | `static_assert` checks pass; unit test verifies `none()` compares equal to itself and not-equal to a default-constructed `EntityId{0, 0}`. |
| AC-002 | `Transform` exists with `position` (`Vec3`), `rotation` (`Quat`), `scale` (`Vec3`) public members. Default values: position zero, rotation identity, scale one. | Default-constructed `Transform` has `position == Vec3::zero()`, `rotation == Quat::identity()`, `scale == Vec3::one()`. |
| AC-003 | `Transform::local_matrix()` returns `Mat4::translate(position) * rotation.to_mat4() * Mat4::scale(scale)`. | For a transform with position `(1,2,3)`, rotation 90° around Y, scale `(2,1,1)`, the matrix transforms a point `(0,0,0,1)` to the expected result within `1e-5f` tolerance. |
| AC-004 | `Transform::world_matrix(entity)` walks the parent chain computing T×R×S at each level. For a root entity it equals `local_matrix()`. For a child with a translated parent, the result includes the parent's translation. | A child at `(0,0,0)` of a parent at `(10,0,0)` yields world translation `(10,0,0)`. A grandchild yields the accumulated transform of all ancestors. Tolerance `1e-5f`. |
| AC-005 | `Component` exists with virtual destructor, is non-copyable, non-movable, and default-constructible from derived classes. | A derived `struct MyComp : public Component {}` compiles and can be destroyed through a `Component*` pointer. |
| AC-006 | `World` can be default-constructed. `Entity::create(world)` returns a valid `Entity` whose `id()` is not `EntityId::none()` and whose `world()` references the same `World`. | Unit test creates a `World`, creates an entity, verifies `id().index` and `id().generation` are sensible, and `&entity.world() == &world`. |
| AC-007 | `Entity::transform()` returns a mutable reference to the entity's `Transform`. Modifications through the reference persist. | Set `entity.transform().position = Vec3(1,2,3)` and verify `entity.transform().position == Vec3(1,2,3)`. |
| AC-008 | `entity.add_component<T>(args...)` creates a component, returns `T&`, and `entity.get_component<T>()` returns `std::optional<T&>` containing the same component. `entity.get_component<U>()` returns `std::nullopt` for a non-existent type. | Unit test with two distinct component types verifies each is independently stored and retrieved. |
| AC-009 | `entity.remove_component<T>()` destroys the component and returns `true`. A second call returns `false`. After removal, `get_component<T>()` returns `std::nullopt`. | Unit test verifies removal, double-removal, and absence after removal. |
| AC-010 | `entity.destroy()` marks the entity as pending destruction. `entity.is_pending_destroy()` returns `true` after `destroy()` and `false` before. `entity.destroy()` is idempotent (second call is a no-op). | Unit test verifies state transitions. |
| AC-011 | `world.flush_destroyed()` reclaims all pending entities. After `flush_destroyed()`, `entity.is_pending_destroy()` on the original handle is unreachable (stale handle — UB). Storage is available for new entities (generation counter increments). | Create entity, destroy it, flush, create another entity — verify the new entity has a higher `generation` than the destroyed one. |
| AC-012 | `entity.create_child()` creates a new entity whose `parent()` returns the creating entity. The creating entity's `child_count()` is incremented and `get_child()` includes the new child. The child is a root-level entity in terms of the world's root list only if the parent is root; otherwise the child is in the parent's subtree. | Unit test verifies parent/child links and `child_count()` / `get_child()`. |
| AC-013 | `entity.reparent(new_parent)` changes the parent, moving the entity from the old parent's children list to the new parent's children list. `reparent(Entity::none())` makes the entity a root (no parent). `reparent(current_parent)` is a no-op. | Unit test verifies children lists before and after reparent. |
| AC-014 | Destroying an entity marks all descendants for destruction (cascade). After flush, the entire subtree is reclaimed. | Create parent → child → grandchild. Destroy parent. Flush. Verify all handles are stale. Verify the world can create new entities that reuse the freed slots. |
| AC-015 | `World::flush_destroyed()` is safe to call when no entities are pending (no-op, no crash). | Call `flush_destroyed()` on a fresh `World` with no entities, then with entities but none destroyed. No crash. |
| AC-016 | `Entity::operator==` and `operator!=` compare both `world_` and `id_`. Two handles to the same entity are equal; handles to different entities are not equal. A null `Entity` is equal only to another null `Entity`. | Unit test verifies equality semantics. |
| AC-017 | `Entity::create(world)` is a static factory. The returned entity has identity transform and no parent. | Verify `entity.transform()` is identity, `entity.parent().id() == EntityId::none()`. |
| AC-018 | An entity can have at most one component of a given type `T`. Calling `add_component<T>()` when `T` already exists is undefined behavior (v1 may assert in debug builds). | Unit test verifies that after `add_component<Health>(100)`, `get_component<Health>()` returns an optional containing a reference to the same instance. The UB case is documented but not tested. |
| AC-019 | All scene graph types reside under `src/engine/scene/` in namespace `buddd::engine`. | Files exist at `src/engine/scene/entity_id.h`, `scene/transform.h`, `scene/component.h`, `scene/entity.h`, `scene/world.h` (and corresponding `.cpp` files as needed). |
| AC-020 | No GLM, SDL3, or OpenGL headers are included in `src/engine/scene/` public headers. The scene module depends only on math wrapper types (`Vec3`, `Quat`, `Mat4`) and standard C++ headers. | Code review catches violations. No automated guard in v1. |
| AC-021 | `sizeof(Entity) == 16` (pointer + EntityId). | `static_assert` passes. |
| AC-022 | A default-constructed `Entity` is null: `id() == EntityId::none()`. `Entity::none()` returns a null entity. Safe operations on a null entity: `id()`, `world()`, `is_pending_destroy()` (returns false), `==`, `!=`. All other operations are UB. | Unit test verifies that `Entity::none().id() == EntityId::none()`, `Entity::none().is_pending_destroy() == false`, and null entities compare equal. |
| AC-023 | `child_count()` returns the number of direct children. `get_child(index)` returns the child at the given index. Combined they provide safe iteration. Index `0 <= idx < child_count()` yields a valid entity. | Unit test verifies that after `create_child()`, `child_count() == 1` and `get_child(0)` is the created child. |
| AC-024 | `Entity::world_matrix()` is a convenience method equivalent to `transform().world_matrix(*this)`. | Unit test verifies both calls produce the same result for a chain of entities with different transforms. Tolerance `1e-5f`. |
| AC-025 | `world.destroy_entity(entity)` is equivalent to `entity.destroy()`. Both mark the entity as pending-destroy and cascade to children. | Unit test verifies the same state after both call paths (`is_pending_destroy()` true, children also pending). |
| AC-026 | `destroy()` cascade on a deeply nested hierarchy (10,000 levels) does not cause a stack overflow. The implementation uses iterative traversal or bounded recursion. | Test creates a chain of 10,000 parent-child entities, calls `destroy()` on the root, and verifies no stack overflow. |
| AC-027 | A destroyed (pending_destroy) entity remains visible in its parent's `child_count()` and `get_child()` until `flush_destroyed()`. After flush, it is removed. | Create parent with two children, destroy one. Before flush: `child_count() == 2`. After flush: `child_count() == 1` and the destroyed child is no longer returned. |
| AC-028 | `flush_destroyed()` destroys entities in reverse depth order (deepest children first). A chain root → parent → child is destroyed as child first, then parent, then root. | Create chain of 3 entities with destructor-tracking components (increment a static counter on destruction). Destroy root, flush, verify counter order matches deepest-first. |
| AC-029 | Components are destroyed when their owning entity is destroyed via `flush_destroyed()`. Component destructors run before the entity slot is reclaimed. | Create entity with component whose destructor sets a flag. Destroy and flush. Verify the flag was set. |
| AC-030 | `World` destruction destroys all entities and their components without requiring an explicit `flush_destroyed()` call. | Create entities with components, destroy some but not all. Destroy the World. Verify no leaks (ASAN-clean) and component destructors ran. |
| AC-031 | After `flush_destroyed()`, a stale `EntityId` (from a destroyed entity) is detectable: `lookup` returns null, `is_pending_destroy` returns false on a reconstructed Entity handle. | Create entity, capture its `EntityId`, destroy and flush. Verify that reconstructing `Entity` from the stale id produces a handle where `is_pending_destroy() == false` (the generation changed). |
| AC-032 | Component pointers obtained before entity destruction are dangling after flush — documented as UB. | (Documentation only — no test for UB.) |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An engine developer can create entities, build hierarchies, attach components, and compute world transforms using only `buddd::engine` public API — no knowledge of internal storage is required. | All user story code examples compile and produce correct results using only public headers under `src/engine/scene/`. |
| SC-002 | All scene graph tests pass in headless CI (no display, no GPU). | `cmake --build --preset debug && ctest --preset debug` — all scene graph tests pass. |
| SC-003 | Entity creation and destruction does not leak memory. | Valgrind or ASAN build shows no leaks in a test that creates and destroys 10,000 entities with components. |
| SC-004 | `flush_destroyed()` runs in O(n) where n is the number of pending entities (flat list iteration, no tree traversal). | Performance test verifies that flushing N pending entities completes in roughly linear time with no unexpected spikes. |
| SC-005 | The public API hides the storage strategy completely — all tests pass without relying on internal tree structure. | No test accesses `World` private members or assumes `std::unique_ptr` tree layout. |

## Edge cases

| Case | Expected behavior |
|---|---|---|
| `entity.destroy()` called multiple times | Second and subsequent calls are no-ops (idempotent). `is_pending_destroy()` continues to return `true`. |
| `flush_destroyed()` called with empty pending list | No-op. No crash, no iteration. |
| `entity.reparent(entity)` — self-reparent | Undefined behavior. The spec documents this; debug builds may assert. |
| `entity.reparent(descendant)` — cycle creation | Undefined behavior. The spec documents this; debug builds may assert. The implementation may detect the cycle by walking the new parent's ancestor chain, but this is not required in v1. |
| `entity.create_child()` on a pending-destroy entity | Undefined behavior (mutating operation on a logically dead entity). |
| `entity.add_component<T>()` when `T` already exists | Undefined behavior. Callers must check with `get_component<T>()` first. |
| `entity.get_component<T>()` on a pending-destroy entity — read-only safe | Returns `std::nullopt` for all types. Part of the read-only-safe contract. |
| `entity.transform()` on a pending-destroy entity — read-only safe | Returns a valid reference (Transform is inline storage and survives until flush). |
| `entity.parent()` on a pending-destroy entity — read-only safe | Returns the pre-destroy parent value (the parent link is not severed until flush). |
| `entity.child_count()` / `get_child()` on a pending-destroy entity | Children that are NOT pending_destroy are still returned normally. The entity's own pending_destroy status does not affect its children access. |
| Parent's view: `parent.child_count()` / `parent.get_child()` after a child is destroyed | The destroyed child remains visible until `flush_destroyed()`. Between `destroy()` and flush, `child_count()` still includes it and `get_child()` still returns it. This ensures consistent iteration during the mark phase. |
| `entity.add_component()` / `remove_component()` on a pending-destroy entity | Undefined behavior (mutating operation). |
| `entity.reparent()` on a pending-destroy entity | Undefined behavior (mutating operation). |
| `entity.reparent()` across different `World` instances | Undefined behavior. The target entity must belong to the same World. |
| `entity.reparent(other)` where `other` belongs to a different World | Undefined behavior (cross-world operation). |
| `World` destroyed with pending entities | `~World()` destroys all entities regardless of pending state. No crash. All storage is reclaimed. |
| `get_child(index)` with out-of-bounds index | Undefined behavior (index >= child_count()). Debug builds may assert. |
| `child_count()` and `get_child()` after children list mutation | The new child count and children are immediately visible. No stale span concerns. |
| `flush_destroyed()` order with nested hierarchy | Entities are destroyed in reverse depth order: deepest children first, then parents. This ensures parent entities are still valid when child components' destructors run. |
| Component destructor order within a single entity | Components are destroyed in reverse order of addition (LIFO) during `flush_destroyed()`. |
| `Component*` pointer cached before entity destruction | Becomes dangling after `flush_destroyed()`. Any access through it is UB. The safe pattern is to call `get_component<T>()` fresh each time, or use Entity handles (which are generation-protected). |
| `Entity` handle cached before entity destruction | After `flush_destroyed()`, the handle is stale — `lookup_node()` returns null. The Entity should be discarded or checked via `is_pending_destroy()` before the flush. |
| `Entity::none()` created via `Entity(World&, EntityId::none())` | Not publicly constructible. Only `World` creates Entity values. Application code creates null entities via default construction or `Entity::none()`. |
| Multiple `flush_destroyed()` calls without intervening `create_entity()` | Second call is a no-op. |
| Entity destruction cascade with deep hierarchy (10,000+ levels) | `destroy()` cascade MUST use iterative traversal (or bounded recursion) to avoid stack overflow. A hierarchy of 10,000 nested entities must not cause a stack overflow. |
| `reparent()` to a pending-destroy parent | Undefined behavior (parent is logically dead, mutating operation on dying entity). |
| `world_matrix()` called on entity with a cycle in ancestor chain | Undefined behavior (likely infinite loop). Can only happen if `reparent()` was called with a descendant despite the UB contract. |

## Error cases

| Case | Expected behavior |
|---|---|---|
| Using a stale `Entity` handle after `flush_destroyed()` | Undefined behavior. No detection in release builds. Debug builds may assert via generation mismatch check. |
| Using a default-constructed (null) `Entity` for any operation except `id()`, `==`, `!=`, `is_pending_destroy()`, `world()` | Undefined behavior. Null entity access is not guarded in release builds. |
| `add_component<T>()` with a type that already exists | Undefined behavior. Debug builds may `assert(false)` or log a warning. |
| `reparent()` creating a cycle | Undefined behavior. Debug builds may detect and assert. |
| `reparent()` to self | Undefined behavior. Debug builds may detect and assert. |
| `reparent()` across different `World` instances | Undefined behavior. |
| `flush_destroyed()` called from within a callback that iterates the entity tree | Undefined behavior (iterator invalidation). Documentation must warn callers. |
| Calling a mutating operation (`add_component`, `remove_component`, `create_child`, `reparent`) on a pending-destroy entity | Undefined behavior. |
| Calling `create_child()` on a pending-destroy entity | Undefined behavior. |

## Permissions and security

- No elevated privileges required.
- No secrets, credentials, or environment variables consumed.
- No I/O, network, or filesystem access.
- The scene graph types are pure memory management and spatial computation.
- There are no access control restrictions — any code with a `World&` can create, destroy, or modify any entity.

## Observability

- No observability output is generated by scene graph types in v1.
- Debug builds may optionally assert on precondition violations (stale handle use, null entity use, duplicate component addition, cycle detection).
- Memory allocation for entities and components can be tracked externally via custom allocators (future spec).
- Logging hooks for entity lifecycle events are deferred to a future observability spec.

## Out of scope

- ECS flat-array / archetype optimization (v1 uses per-entity `vector<unique_ptr<Component>>`).
- Entity lookup by name or tag.
- Entity iteration / query APIs (e.g., "find all entities with component T").
- Scene serialization / deserialization (JSON, binary, or otherwise).
- Scene loading / unloading from files.
- Prefab system or entity templates.
- Component lifecycle hooks (`on_attach`, `on_detach`, `on_enable`, `on_disable`).
- Event system (entity created/destroyed, component added/removed).
- Camera attachment to entities (camera as a component).
- Mesh / model / renderer components.
- Light components.
- Physics components or rigidbodies.
- Audio components.
- Scripting / behavior components.
- Editor / inspector integration.
- Multi-world or world partitioning.
- Thread safety (all operations are single-threaded in v1).
- Custom allocators for entity/component storage.
- `Transform` caching or dirty flags for world matrices.
- `reparent()` with world-space preservation (v1 changes parent without adjusting local transform).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The math types (`Vec3`, `Quat`, `Mat4`) from SPEC-004 are available and stable. `Vec3::zero()`, `Vec3::one()`, `Quat::identity()`, `Mat4::translate()`, `Mat4::rotate()`, `Mat4::scale()`, `Quat::to_mat4()` are present with the signatures described in SPEC-004. |
| A-02 | `std::expected<T, Error>` (aliased as `Result<T>`) is available from `src/engine/error.h` for any future API that needs error returns. The scene graph v1 does not use `Result<T>` because all operations are infallible or result in UB on precondition violation. |
| A-03 | v1 uses `std::vector<std::unique_ptr<Component>>` per entity for component storage. This is an internal implementation detail hidden from the public API. The API is designed so that a future flat-array ECS refactor does not break client code. |
| A-04 | v1 uses a tree of heap-allocated entity nodes internally. Each node holds the `Transform` inline, a `std::vector<std::unique_ptr<Component>>`, a parent pointer, a children list (e.g., `std::vector<...>` of handles or indices), and the `EntityId`. The exact internal representation is implementation-defined. |
| A-05 | `Entity` is a lightweight value-type handle (16 bytes). It is cheap to pass by value. It is not intended to be stored in a `std::unique_ptr` or heap-allocated — it is a view. |
| A-06 | The generation counter in `EntityId` is `uint32_t` and wraps on overflow. Wrapping is extremely unlikely (4 billion creations per slot) and is accepted as safe in practice. |
| A-07 | The `World` destructor destroys all entities and components. No explicit `flush_destroyed()` call is required before `World` destruction. Any pending-destroy entities are cleaned up in `~World()`. |
| A-08 | `flush_destroyed()` uses a flat `pending_destroy_` list of `EntityId` values. During flush, the implementation iterates this list, reclaims each entity's resources (calling component destructors, freeing the node), and updates the slot's generation counter. No tree traversal is needed during collection because children are added to the flat list at mark time. |
| A-09 | `entity.destroy()` marks the entity and recursively iterates all descendants, adding each to the `pending_destroy_` flat list. This is O(subtree size) and happens at mark time, not at flush time. |
| A-10 | The project convention of `noexcept` on value-type operations applies. `EntityId` methods and `Transform` pure computation methods (`local_matrix()`, `world_matrix()`) are `noexcept`. `Entity` methods that do not allocate memory (`id()`, `world()`, `none()`, `is_pending_destroy()`, `transform()`, `get_component()`, `parent()`, `child_count()`, `get_child()`, `operator==`, `world_matrix()`) are `noexcept`. Methods that may allocate (`destroy()`, `add_component()`, `remove_component()`, `create_child()`, `reparent()`, `create()`) are not `noexcept`. `World` methods follow the same principle: `get_transform()` and `flush_destroyed()` are `noexcept`; `create_entity()`, `destroy_entity()` are not. |
| A-11 | The scene module directory is `src/engine/scene/`. Files are added to the build automatically via the existing `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` pattern in `src/engine/CMakeLists.txt`. |
| A-12 | A root entity is defined as an entity with no parent (parent is `EntityId::none()` or equivalent). Root entities are tracked in `World::roots_` (an implementation detail). |

## Open questions

| ID | Question | Impact |
|---|---|---|---|
| Q-01 | [RESOLVED] `Entity::children()` replaced by `child_count()` + `get_child(index)` pair. No span, no buffer, no allocation. Patterns follows Unity's `transform.childCount` / `transform.GetChild()`. Iteration: `for (size_t i = 0; i < e.child_count(); ++i) { auto child = e.get_child(i); ... }`. **Resolution**: `child_count()` + `get_child()`. | API design and internal storage strategy. |
| Q-02 | [RESOLVED] `Entity::create_child()` always creates with identity transform in v1. The caller can immediately set `entity.transform().position = ...` after creation. An overload accepting a `Transform` parameter is deferred. **Resolution**: Identity transform in v1. | API ergonomics. |
| Q-03 | [RESOLVED] `flush_destroyed()` returns `void` in v1. A `size_t` return can be added later without breaking API compatibility. **Resolution**: `void`. | Observability vs API simplicity. |
| Q-04 | [RESOLVED] `EntityId::none()` returns `{ UINT32_MAX, UINT32_MAX }`. This avoids ambiguity with valid slot 0, generation 0. **Resolution**: `{ UINT32_MAX, UINT32_MAX }`. | Handle safety. |
| Q-05 | [RESOLVED] `add_component<T>(args...)` forwards arguments directly (no `std::in_place_t`). Example: `entity.add_component<Health>(100)`. If disambiguation is needed later, an `add_component(std::in_place_t, Args&&...)` overload can be added. **Resolution**: Direct args forwarding. | API ergonomics. |
| Q-06 | [RESOLVED] `remove_component<T>()` returns `bool` (true if a component was removed, false if none existed). Callers can inspect via `get_component<T>()` before removal. **Resolution**: `bool`. | API flexibility. |
