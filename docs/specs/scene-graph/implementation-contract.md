# IMPL-008 — Scene Graph (World, Entity, Transform, Components, Hierarchy)

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|
| Approved by | Guillaume |
| Date | 2026-05-30 |
| Time | (not specified — approved via conversation) |

## Source spec

`docs/specs/scene-graph/spec.md` (SPEC-008), accepted (`docs/specs/scene-graph/spec-critic.md` 2nd review verdict: `Accepted`, all 5 blocking issues resolved).

## Goal

Implement the foundational scene graph for the Buddd Engine: provide `EntityId` (8-byte handle with generation counter), `Transform` (position/rotation/scale value type with local and world matrix computation), `Component` (polymorphic base class with virtual destructor only), `Entity` (16-byte lightweight handle/view that delegates to `World`), and `World` (top-level container managing entity lifecycle, tree hierarchy, per-entity component storage, and deferred destruction). All types live under `src/engine/scene/` in namespace `buddd::engine`, depend only on math wrapper types (`Vec3`, `Quat`, `Mat4`) and standard C++ headers, and are automatically picked up by the existing CMake `file(GLOB_RECURSE)` configuration.

## Non-goals

- No ECS flat-array / archetype optimization (v1 uses per-entity `std::vector<std::unique_ptr<Component>>`).
- No mesh, model, geometry, or rendering integration.
- No camera attachment to entities.
- No serialization or deserialization of world state.
- No component lifecycle hooks (`on_attach`, `on_detach`).
- No event system for entity/component lifecycle.
- No physics, audio, scripting, or gameplay systems.
- No threading or parallel entity iteration.
- No editor, inspector, or GUI tooling.
- No `Result<T>` error returns (scene graph operations are infallible or result in UB on precondition violation — a deliberate exception to ADR-001).
- No modifications to files outside `src/engine/scene/` and `tests/CMakeLists.txt`.
- No changes to build system files other than `tests/CMakeLists.txt`.
- No modification of existing source files except `tests/CMakeLists.txt`.

## Relevant constitution rules

- **CONST-001-architecture-boundaries.md**: All scene graph types must be inside `src/engine/scene/`. No GLM, SDL3, or OpenGL headers may be included in `src/engine/scene/` public headers. The scene module may include math wrappers (`Vec3`, `Quat`, `Mat4`) which are the engine's abstraction layer.
- **CONST-002-testing-policy.md**: All testable code must have corresponding unit tests. The `tests/scene_graph_tests.cpp` file must cover all acceptance criteria with Catch2 v3.

## Relevant ADRs

- **ADR-001** (`docs/adr/001-result-error-pattern.md`): Establishes `Result<T>` / `Error` for fallible engine APIs. The scene graph intentionally does NOT use `Result<T>` — scene graph operations either succeed unconditionally (trivial value-type computations), are infallible internal operations, or result in **undefined behavior** on precondition violation. Memory allocation failure (OOM) is treated as an unrecoverable system-level condition, consistent with the render pipeline's `draw()`/`draw_indexed()` precedent. This is a documented exception.
- **ADR-002** (`docs/adr/002-glm-wrapper-math.md`): Establishes the GLM wrapper pattern for math types (`Vec3`, `Quat`, `Mat4`). The scene graph depends on these wrapper types exclusively.

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/error.h` | Verify `Result<T>` and `Error` definition. Scene graph does NOT use them, but the pattern must be understood for the ADR-001 rationale. |
| `src/engine/version.h` | Style reference for engine headers (`#pragma once`, namespace `buddd::engine`, trailing return types). |
| `src/engine/math/vec3.h` | `Vec3` type — used by `Transform`. Confirm `Vec3::zero()`, `Vec3::one()` exist with correct signatures. |
| `src/engine/math/quat.h` | `Quat` type — used by `Transform`. Confirm `Quat::identity()`, `Quat::to_mat4()` exist. |
| `src/engine/math/mat4.h` | `Mat4` type — used by `Transform::local_matrix()`. Confirm `Mat4::translate()`, `Mat4::scale()`, `Mat4::identity()` exist. |
| `src/engine/CMakeLists.txt` | Confirm `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` auto-discovers new files under `src/engine/scene/`. No changes needed. |
| `tests/CMakeLists.txt` | Must be read to understand the current test build configuration before modifying. |
| `tests/math_test.cpp` | Style reference for test files (Catch2 v3, `#include` paths relative to `src/engine/`, `TEST_CASE` formatting with `[tag]` annotations, helper functions, tolerance usage). |
| `docs/specs/math-foundations/implementation-contract.md` | Style reference for contract format, level of detail, and pseudo-code conventions. |
| `docs/specs/render-pipeline/implementation-contract.md` | Reference for the ADR-001 exception pattern (`draw()`/`draw_indexed()` return `void`). |

## Files allowed to change

### New files to create (8 files)

All paths are relative to the repository root.

| # | File | Purpose |
|---|---|---|
| 1 | `src/engine/scene/entity_id.h` | `EntityId` struct — 8-byte handle (index + generation). Header-only. |
| 2 | `src/engine/scene/transform.h` | `Transform` struct — position, rotation, scale, local_matrix(), world_matrix(). Header-only. |
| 3 | `src/engine/scene/component.h` | `Component` base class — virtual destructor, non-copyable, non-movable. Header-only. |
| 4 | `src/engine/scene/entity.h` | `Entity` class — 16-byte handle. Inline/template method definitions for small methods. |
| 5 | `src/engine/scene/entity.cpp` | `Entity` non-inline method implementations. |
| 6 | `src/engine/scene/world.h` | `World` class — top-level container, entity lifecycle, hierarchy, deferred destruction. |
| 7 | `src/engine/scene/world.cpp` | `World` implementation (including internal `EntityNode` type). |
| 8 | `tests/scene_graph_tests.cpp` | Catch2 v3 test cases covering all acceptance criteria. |

### Files to modify (1 file)

| # | File | Change |
|---|---|---|
| 1 | `tests/CMakeLists.txt` | Add `scene_graph_tests.cpp` to the `buddd_tests` source list in **both** the `BUDDD_HAS_DISPLAY` and non-display branches. |

## Files forbidden to change

- Any file outside `src/engine/scene/` and `tests/CMakeLists.txt`.
- `src/engine/CMakeLists.txt`
- `src/engine/version.h`, `src/engine/version.cpp`
- `src/engine/error.h`
- `src/engine/math/` (any file)
- `src/engine/platform/` (any file)
- `src/engine/window/` (any file)
- `src/engine/render/` (any file)
- `src/cmd/` (any file)
- `src/editor/` (any file)
- `tests/` except `tests/CMakeLists.txt` and `tests/scene_graph_tests.cpp`.
- `tests/CMakeLists.txt` — only the `scene_graph_tests.cpp` addition. No other changes.
- `docs/adr/` (any file)
- `docs/constitution/` (any file)
- Root `CMakeLists.txt`, `CMakePresets.json`, `.clang-format`, `.vscode/`, `AGENTS.md`, `opencode.json`.

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Namespace | `buddd::engine` for all scene types. No nested sub-namespace for scene types. |
| File naming | `snake_case` — `entity_id.h`, `transform.h`, `component.h`, `entity.h`, `world.h`. |
| Directory naming | `snake_case` — `src/engine/scene/`. |
| Class/struct naming | PascalCase — `EntityId`, `Transform`, `Component`, `Entity`, `World`. |
| Header guards | `#pragma once` (no `#ifndef` guards). |
| Function style | Trailing return type syntax (`auto foo() -> int`). |
| Formatting | `.clang-format` at repo root: LLVM style, 4-space indent, 100 column limit. |
| Local includes | Use `#include "scene/entity_id.h"` — quoted paths relative to `src/engine/`, resolved via PUBLIC include directory of `buddd_engine`. |
| Include order | 1. Standard library headers (`<angle brackets>`), 2. Engine headers (`"quotes"`). Empty line between groups. |
| noexcept | Non-allocating methods are `noexcept`. Allocating methods (`destroy()`, `add_component()`, `remove_component()`, `create_child()`, `reparent()`, `World::create_entity()`, `World::destroy_entity()`, `Entity::create()`) are NOT `noexcept`. See the `noexcept` specification table below. |
| Test style | Catch2 v3, `TEST_CASE` with descriptive name and `[tag]` annotations, `REQUIRE`/`CHECK` macros. `#include` paths relative to `src/engine/`. Use `using Catch::Approx` with `1e-5f` tolerance for float comparisons. |
| static_assert for size/triviality | `EntityId` requires `static_assert(is_trivially_copyable_v<EntityId>)` and `static_assert(sizeof(EntityId) == 8)`. `Entity` requires `static_assert(sizeof(Entity) == 16)`. |

### noexcept specification table

| Method | noexcept? | Rationale |
|---|---|---|
| `EntityId` all methods | Yes | Value type, pure computation |
| `Transform::local_matrix()` | Yes | Pure math, no allocation |
| `Transform::world_matrix()` | Yes | Pure computation, uses fixed-size stack array |
| `Component` all methods | Yes | Trivial destructor |
| `Entity::id()` | Yes | Pure getter |
| `Entity::world()` | Yes | Pure getter |
| `Entity::none()` | Yes | Static factory, trivial construction |
| `Entity::destroy()` | No | May allocate (pending_destroy_ list append) |
| `Entity::is_pending_destroy()` | Yes | Pure getter |
| `Entity::transform()` | Yes | Getter, no allocation |
| `Entity::get_component()` | Yes | Const/non-const, lookup only, no allocation |
| `Entity::add_component()` | No | Allocates component storage |
| `Entity::remove_component()` | No | May deallocate |
| `Entity::parent()` | Yes | Pure getter |
| `Entity::child_count()` | Yes | Pure getter, no allocation |
| `Entity::get_child()` | Yes | Pure getter, no allocation |
| `Entity::create_child()` | No | Allocates new entity node |
| `Entity::reparent()` | No | May allocate (child list resize) |
| `Entity::world_matrix()` | Yes | Delegates to `Transform::world_matrix()` |
| `Entity::operator==` | Yes | Comparison, no allocation |
| `Entity::create()` | No | Delegates to `World::create_entity()` |
| `World::create_entity()` | No | Allocates new entity node |
| `World::destroy_entity()` | No | May allocate (pending_destroy_ list grow) |
| `World::flush_destroyed()` | Yes | Reclaims already-marked storage, no allocation |
| `World::is_pending_destroy()` | Yes | Pure lookup |
| `World::get_transform()` | Yes | Pure lookup |

## Required implementation behavior

### 0. `tests/CMakeLists.txt` — Add scene graph test file

Insert `scene_graph_tests.cpp` after `math_test.cpp` in the source list for **both** the `if(BUDDD_HAS_DISPLAY)` and `else()` branches:

```cmake
# Before (both branches):
add_executable(buddd_tests
    version_test.cpp
    platform_abstraction_test.cpp
    math_test.cpp          # last entry
)

# After (both branches):
add_executable(buddd_tests
    version_test.cpp
    platform_abstraction_test.cpp
    math_test.cpp
    scene_graph_tests.cpp  # <-- new
)
```

For the `if(BUDDD_HAS_DISPLAY)` branch, keep `sdl3_backend_test.cpp` in its existing position. The scene graph test is headless and belongs in both branches.

No changes to `target_link_libraries` or `catch_discover_tests`. The scene graph test needs only `buddd_engine` and `Catch2::Catch2WithMain`, which are already linked.

### 1. `src/engine/scene/entity_id.h`

Header-only, no `.cpp` file.

```cpp
#pragma once

#include <cstdint>
#include <type_traits>

namespace buddd::engine {

struct EntityId {
    uint32_t index;
    uint32_t generation;

    static constexpr auto none() noexcept -> EntityId {
        return EntityId{UINT32_MAX, UINT32_MAX};
    }

    auto operator==(const EntityId&) const noexcept -> bool = default;
    auto operator!=(const EntityId&) const noexcept -> bool = default;
};

static_assert(std::is_trivially_copyable_v<EntityId>,
    "EntityId must be trivially copyable");
static_assert(sizeof(EntityId) == 8,
    "EntityId must be 8 bytes");

} // namespace buddd::engine
```

**Requirements:**
- `index` and `generation` are `uint32_t` public members. No getters/setters — direct member access (`id.index`, `id.generation`).
- `none()` returns `{UINT32_MAX, UINT32_MAX}` — a sentinel value that cannot collide with valid slot 0, generation 0.
- `operator==` and `operator!=` are defaulted (C++20 member-wise comparison).
- Both `static_assert` checks must be present at file scope, after the struct definition.
- `EntityId` is trivially copyable — no user-declared constructors, destructor, or copy/move operations. The `static_assert` guarantees this.
- Includes only `<cstdint>` and `<type_traits>`.

### 2. `src/engine/scene/transform.h`

Header-only, no `.cpp` file. Must forward-declare `Entity` because `world_matrix()` takes `const Entity&`.

```cpp
#pragma once

#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"

namespace buddd::engine {

// Forward declaration (Entity is defined in entity.h, which includes this header).
class Entity;

struct Transform {
    math::Vec3 position{math::Vec3::zero()};
    math::Quat rotation{math::Quat::identity()};
    math::Vec3 scale{math::Vec3::one()};

    auto local_matrix() const noexcept -> math::Mat4;

    /// Walks the parent chain of `entity`, accumulating local transforms
    /// root-to-leaf. Uses a fixed-size stack array (up to 4096 levels).
    /// Returns the world matrix for this transform's entity.
    auto world_matrix(const Entity& entity) const noexcept -> math::Mat4;
};

inline auto Transform::local_matrix() const noexcept -> math::Mat4 {
    return math::Mat4::translate(position)
         * rotation.to_mat4()
         * math::Mat4::scale(scale);
}

// Note: Transform::world_matrix() is defined after Entity is fully defined.
// See entity.h for the inline definition.

} // namespace buddd::engine
```

**Requirements:**
- Default values: `position = Vec3::zero()`, `rotation = Quat::identity()`, `scale = Vec3::one()`.
- `local_matrix()` computes `Translate * Rotate * Scale` in that order — correct TRS convention for game engines.
- `local_matrix()` is `noexcept` — pure math computation.
- `world_matrix()` is declared in `transform.h` but defined inline in `entity.h` (after the `Entity` class), OR defined in `entity.cpp` to break the circular dependency. **Preferred approach**: define `Transform::world_matrix()` inline in `entity.h` after the `Entity` class definition.
- `world_matrix()` **must not** use recursion — it must use iterative traversal with a fixed-size stack array (max 4096 elements) to avoid stack overflow.
- `world_matrix()` is `noexcept` — uses stack allocation only.

### 3. `src/engine/scene/component.h`

Header-only, no `.cpp` file.

```cpp
#pragma once

namespace buddd::engine {

class Component {
public:
    virtual ~Component() = default;

    Component(const Component&) = delete;
    auto operator=(const Component&) -> Component& = delete;
    Component(Component&&) = delete;
    auto operator=(Component&&) -> Component& = delete;

protected:
    Component() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- Virtual destructor — defaulted, not pure virtual.
- Non-copyable, non-movable (all four special member functions deleted).
- Protected default constructor — only derived classes may construct.
- No other methods, virtual functions, or data members.
- No includes required.

### 4. `src/engine/scene/entity.h`

```cpp
#pragma once

#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

#include "scene/entity_id.h"
#include "scene/transform.h"
#include "scene/component.h"

namespace buddd::engine {

class World;

class Entity {
public:
    // -- Identity --
    auto id() const noexcept -> EntityId { return id_; }
    auto world() const noexcept -> World& { return *world_; }

    static auto none() noexcept -> Entity { return Entity{}; }

    // -- Lifecycle --
    void destroy();
    auto is_pending_destroy() const noexcept -> bool;

    // -- Transform --
    auto transform() noexcept -> Transform&;
    auto transform() const noexcept -> const Transform&;

    // -- Components --
    template<typename T, typename... Args>
    auto add_component(Args&&... args) -> T&;

    template<typename T>
    auto get_component() const noexcept -> std::optional<const T&>;

    template<typename T>
    auto get_component() noexcept -> std::optional<T&>;

    template<typename T>
    auto remove_component() -> bool;

    // -- Hierarchy --
    auto parent() const noexcept -> Entity;
    auto child_count() const noexcept -> size_t;
    auto get_child(size_t index) const noexcept -> Entity;
    auto create_child() -> Entity;
    void reparent(Entity new_parent);

    // -- Convenience --
    auto world_matrix() const noexcept -> math::Mat4;

    // -- Comparison --
    friend auto operator==(const Entity&, const Entity&) noexcept -> bool = default;

    // -- Factory --
    static auto create(World& world) -> Entity;

private:
    friend class World;

    World* world_ = nullptr;
    EntityId id_ = EntityId::none();

    Entity(World& world, EntityId id) noexcept;
};

static_assert(sizeof(Entity) == 16,
    "Entity must be 16 bytes (pointer + EntityId)");

// -- Template method implementations (inline, delegate to World) --

template<typename T, typename... Args>
inline auto Entity::add_component(Args&&... args) -> T& {
    return world_->add_component<T>(id_, std::forward<Args>(args)...);
}

template<typename T>
inline auto Entity::get_component() const noexcept -> std::optional<const T&> {
    return world_->get_component<T>(id_);
}

template<typename T>
inline auto Entity::get_component() noexcept -> std::optional<T&> {
    return world_->get_component<T>(id_);
}

template<typename T>
inline auto Entity::remove_component() -> bool {
    return world_->remove_component<T>(id_);
}

// -- Transform::world_matrix() inline definition (requires Entity to be complete) --

inline auto Transform::world_matrix(const Entity& entity) const noexcept -> math::Mat4 {
    // Walk the parent chain from entity up to root, collecting local transforms.
    // Use a fixed-size stack array for deep hierarchy support without recursion.
    constexpr size_t MAX_DEPTH = 4096;
    math::Mat4 chain[MAX_DEPTH];
    size_t depth = 0;

    Entity current = entity;
    chain[depth++] = current.transform().local_matrix();

    math::Mat4 result = math::Mat4::identity();

    while (current.parent().id() != EntityId::none()) {
        current = current.parent();
        if (depth >= MAX_DEPTH) {
            // Depth exceeded: store the current (unaccumulated) entity's
            // local matrix into result, then break. The secondary loop
            // will prepend remaining ancestors.
            result = current.transform().local_matrix();
            break;
        }
        chain[depth++] = current.transform().local_matrix();
    }

    // Accumulate from root (last element) to entity (first element)
    // In the normal case, result starts as identity and chain[] contains
    // all ancestor transforms. In the overflow case, result already
    // contains the break entity's local matrix, and chain[] contains
    // the MAX_DEPTH ancestors below it.
    for (size_t i = depth; i > 0; --i) {
        result = result * chain[i - 1];
    }

    // If hierarchy is shallow (no overflow), this loop does nothing
    // because current.parent() is EntityId::none().
    // If overflow occurred, current is the break entity whose matrix
    // is already in result; continue prepending remaining ancestors.
    while (current.parent().id() != EntityId::none()) {
        current = current.parent();
        result = current.transform().local_matrix() * result;
    }

    return result;
}

} // namespace buddd::engine
```

**Requirements:**
- `Entity` is a 16-byte handle: `World*` (8 bytes) + `EntityId` (8 bytes). The `static_assert` guarantees this.
- Default constructor creates a null entity (`world_ = nullptr`, `id_ = EntityId::none()`).
- `Entity::none()` is a static method returning a default-constructed (null) `Entity`.
- `id()` returns `id_` by value. `noexcept`.
- `world()` returns `*world_`. UB if called on a null entity. `noexcept`.
- `operator==` compares both `world_` and `id_` — defaulted member-wise comparison does this correctly.
- `destroy()` delegates to `world_->destroy_entity(*this)`. NOT `noexcept`.
- `is_pending_destroy()` delegates to `world_->is_pending_destroy(id_)`. `noexcept`.
- `transform()` delegates to `world_->get_transform(id_)`. Both const and non-const overloads. `noexcept`.
- `parent()` calls `world_->get_parent(id_)`. `noexcept`.
- `child_count()` calls `world_->get_child_count(id_)`. `noexcept`.
- `get_child(index)` calls `world_->get_child(id_, index)`. `noexcept`.
- `create_child()` delegates to `world_->create_child(id_)`. NOT `noexcept`.
- `reparent(new_parent)` delegates to `world_->reparent(id_, new_parent.id_)`. NOT `noexcept`.
- `world_matrix()` calls `transform().world_matrix(*this)`. `noexcept`.
- `Entity(World& world, EntityId id)` is the private constructor used by `World` (friend class). `noexcept`.
- Template methods `add_component`, `get_component`, `remove_component` are defined inline in the header and delegate to `World` methods.
- `Transform::world_matrix()` is defined inline after the `Entity` class, using iterative traversal with a fixed-size stack array. The two-phase algorithm handles hierarchies deeper than `MAX_DEPTH` by falling through to a secondary accumulation loop.
- `operator!=` is NOT explicitly declared — C++20 synthesizes it from `operator==`.

### 5. `src/engine/scene/entity.cpp`

```cpp
#include "scene/entity.h"
#include "scene/world.h"

namespace buddd::engine {

Entity::Entity(World& world, EntityId id) noexcept
    : world_(&world)
    , id_(id)
{}

void Entity::destroy() {
    world_->destroy_entity(*this);
}

auto Entity::is_pending_destroy() const noexcept -> bool {
    return world_->is_pending_destroy(id_);
}

auto Entity::transform() noexcept -> Transform& {
    return world_->get_transform(id_);
}

auto Entity::transform() const noexcept -> const Transform& {
    return world_->get_transform(id_);
}

auto Entity::parent() const noexcept -> Entity {
    return world_->get_parent(id_);
}

auto Entity::child_count() const noexcept -> size_t {
    return world_->get_child_count(id_);
}

auto Entity::get_child(size_t index) const noexcept -> Entity {
    return world_->get_child(id_, index);
}

auto Entity::create_child() -> Entity {
    return world_->create_child(id_);
}

void Entity::reparent(Entity new_parent) {
    world_->reparent(id_, new_parent.id_);
}

auto Entity::world_matrix() const noexcept -> math::Mat4 {
    return transform().world_matrix(*this);
}

auto Entity::create(World& world) -> Entity {
    return world.create_entity();
}

} // namespace buddd::engine
```

**Requirements:**
- Every method delegates to the corresponding `World` method.
- All non-allocating methods carry `noexcept` per the specification table above.
- The `.cpp` file includes both `"scene/entity.h"` and `"scene/world.h"` (the `.h` only forward-declares `World`).
- Template methods (`add_component`, `get_component`, `remove_component`) are NOT in this file — they are defined inline in `entity.h`.

### 6. `src/engine/scene/world.h`

```cpp
#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "scene/entity_id.h"
#include "scene/entity.h"
#include "scene/transform.h"
#include "scene/component.h"

namespace buddd::engine {

class World {
public:
    World();
    ~World();

    World(const World&) = delete;
    auto operator=(const World&) -> World& = delete;
    World(World&&) = delete;
    auto operator=(World&&) -> World& = delete;

    // -- Entity lifecycle --
    auto create_entity() -> Entity;
    void destroy_entity(Entity entity);
    void flush_destroyed() noexcept;

    // -- Internal (called by Entity) --
    auto is_pending_destroy(EntityId id) const noexcept -> bool;
    auto get_transform(EntityId id) noexcept -> Transform&;
    auto get_transform(EntityId id) const noexcept -> const Transform&;

    // -- Component management (called by Entity templates) --
    template<typename T, typename... Args>
    auto add_component(EntityId id, Args&&... args) -> T&;

    template<typename T>
    auto get_component(EntityId id) noexcept -> std::optional<T&>;

    template<typename T>
    auto get_component(EntityId id) const noexcept -> std::optional<const T&>;

    template<typename T>
    auto remove_component(EntityId id) -> bool;

private:
    friend class Entity;

    struct EntityNode;

    // -- Internal helpers --
    auto lookup_node(EntityId id) noexcept -> EntityNode*;
    auto lookup_node(EntityId id) const noexcept -> const EntityNode*;

    auto create_child(EntityId parent_id) -> Entity;
    auto get_parent(EntityId id) const noexcept -> Entity;
    auto get_child_count(EntityId id) const noexcept -> size_t;
    auto get_child(EntityId id, size_t index) const noexcept -> Entity;

    void reparent(EntityId id, EntityId new_parent_id);
    void mark_for_destroy(EntityNode* node);

    // -- Storage --
    struct Slot {
        std::unique_ptr<EntityNode> node;
        uint32_t generation = 0;
        bool alive = false;
    };

    std::vector<Slot> slots_;
    std::vector<std::unique_ptr<EntityNode>> roots_;
    std::vector<EntityNode*> pending_destroy_;
    std::vector<uint32_t> free_slots_;
    uint32_t next_slot_ = 0;
};

// -- Template method implementations (inline, defined in header) --

template<typename T, typename... Args>
inline auto World::add_component(EntityId id, Args&&... args) -> T& {
    auto* node = lookup_node(id);
    // UB if node is null, slot dead, or component of type T already exists.
    // Debug builds may assert(!node->has_component<T>()).
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = component.get();
    node->components_.push_back(std::move(component));
    return *ptr;
}

template<typename T>
inline auto World::get_component(EntityId id) noexcept -> std::optional<T&> {
    auto* node = lookup_node(id);
    if (!node || node->pending_destroy_) {
        return std::nullopt;
    }
    for (auto& c : node->components_) {
        auto* typed = dynamic_cast<T*>(c.get());
        if (typed) {
            return std::optional<T&>(*typed);
        }
    }
    return std::nullopt;
}

template<typename T>
inline auto World::get_component(EntityId id) const noexcept -> std::optional<const T&> {
    auto* node = lookup_node(id);
    if (!node || node->pending_destroy_) {
        return std::nullopt;
    }
    for (const auto& c : node->components_) {
        auto* typed = dynamic_cast<const T*>(c.get());
        if (typed) {
            return std::optional<const T&>(*typed);
        }
    }
    return std::nullopt;
}

template<typename T>
inline auto World::remove_component(EntityId id) -> bool {
    auto* node = lookup_node(id);
    // UB if node is null or pending_destroy_.
    // Debug builds may assert.
    for (auto it = node->components_.begin(); it != node->components_.end(); ++it) {
        if (dynamic_cast<T*>(it->get())) {
            node->components_.erase(it);
            return true;
        }
    }
    return false;
}

} // namespace buddd::engine
```

**Requirements:**
- `World` is non-copyable, non-movable (all four special members deleted).
- The `Slot` struct tracks each potential entity slot's node (owned via `unique_ptr`), its current generation (incremented on slot reuse), and whether it is alive.
- `lookup_node(id)` checks that `id.index < slots_.size()`, `slots_[id.index].alive == true`, and `slots_[id.index].generation == id.generation`. Returns `nullptr` on any mismatch. `noexcept`.
- Children are stored per-entity in `EntityNode::children_` (a `std::vector<std::unique_ptr<EntityNode>>`). No shared buffer is needed. `get_child_count()` returns `node->children_.size()`. `get_child(index)` returns `Entity(*world_, node->children_[index]->id_)`.

### 7. `src/engine/scene/world.cpp`

Contains the `EntityNode` definition and all non-inline `World` methods.

**EntityNode structure:**

```cpp
struct World::EntityNode {
    EntityId id_;
    Transform transform_;
    EntityNode* parent_ = nullptr;                             // raw, non-owning
    std::vector<std::unique_ptr<EntityNode>> children_;
    std::vector<std::unique_ptr<Component>> components_;
    World* world_ = nullptr;                                   // back-reference
    bool pending_destroy_ = false;
};
```

**World methods (pseudo-code / precise contract):**

#### `World::World()`
- Initialize `slots_`, `roots_`, `pending_destroy_`, `free_slots_` as empty.
- `next_slot_ = 0`.

#### `World::~World()`
- All `EntityNode` destructors run via `unique_ptr` — this destroys all components and children recursively. No explicit `flush_destroyed()` is required.

#### `World::create_entity() -> Entity`
- Allocate a slot index: if `free_slots_` is not empty, pop the last index and reuse. Otherwise use `next_slot_++` (growing `slots_` with a new `Slot`).
- Create a new `EntityNode` with `id_ = {index, slots_[index].generation}`, identity `Transform`, `parent_ = nullptr`, `world_ = this`.
- Store the node in `slots_[index].node` and set `slots_[index].alive = true`.
- Add the node to `roots_` as `unique_ptr`.
- Return `Entity(*this, id)`.
- NOT `noexcept` (allocates).

#### `World::destroy_entity(Entity entity)`
- Calls `mark_for_destroy(lookup_node(entity.id()))`.
- NOT `noexcept` (may allocate in `pending_destroy_`).

#### `World::flush_destroyed() noexcept`
- **Iterate `pending_destroy_` in reverse** (deepest children first, then parents):
  - For each `node` in `pending_destroy_` (from back to front):
    - **Unlink from parent**: 
      - If `node->parent_` is not null, find and release the `unique_ptr` from `node->parent_->children_` (linear scan matching `it->get() == node`).
      - If `node->parent_` is null (root), find and release from `roots_`.
    - The `unique_ptr` destruction calls `~EntityNode()` which destroys all components in reverse order of addition (LIFO), then destroys child nodes recursively.
    - Mark `slots_[node->id_.index].alive = false`.
    - Increment `slots_[node->id_.index].generation`.
    - Push `node->id_.index` onto `free_slots_`.
- Clear `pending_destroy_`.
- `noexcept` (does not allocate; only reclaims existing resources).

#### `World::mark_for_destroy(EntityNode* node)`
- Iterative traversal (NOT recursive) to avoid stack overflow:
  - Push `node` onto a local `std::vector<EntityNode*>` stack.
  - While stack is not empty:
    - Pop `n`.
    - If `n->pending_destroy_` is already true, continue (idempotent).
    - Set `n->pending_destroy_ = true`.
    - Append `n` to `pending_destroy_` (pre-order: parent before children — list is reversed during flush for deepest-first destruction).
    - For each child in `n->children_`, push `child.get()` onto the stack.
- **Effect**: After `mark_for_destroy()` completes, pending-destroy entities are marked but **still linked in their parent's children list** and in the tree. Parent's `child_count()` and `get_child()` still include them. This preserves iteration consistency during the mark phase. Unlinking happens during `flush_destroyed()`.
- NOT `noexcept` (may allocate for the stack or `pending_destroy_` growth).

#### `World::get_parent(EntityId id) const noexcept -> Entity`
- Look up node via `lookup_node(id)`. Return null `Entity` if node is null or has no parent.
- Return `Entity(*world_, node->parent_->id_)`.
- `noexcept`.

#### `World::get_child_count(EntityId id) const noexcept -> size_t`
- Look up node via `lookup_node(id)`.
- Return `node->children_.size()` (or 0 if node is null / pending_destroy).
- `noexcept` — pure getter.

#### `World::get_child(EntityId id, size_t index) const noexcept -> Entity`
- Look up node via `lookup_node(id)`.
- UB if `index >= node->children_.size()`.
- Return `Entity(*world_, node->children_[index]->id_)`.
- `noexcept` — pure getter.

#### `World::reparent(EntityId id, EntityId new_parent_id)`
- Look up `node` via `lookup_node(id)`. Look up `new_parent` via `lookup_node(new_parent_id)` (may be null if `new_parent_id == EntityId::none()`).
- UB if:
  - `node` is null.
  - `new_parent` is non-null and `new_parent->world_ != node->world_` (cross-world).
  - `new_parent == node` (self-reparent).
  - `new_parent` is a descendant of `node` (cycle) — debug builds may walk ancestor chain to detect this.
- If `node->parent_ == new_parent` (or both null), return immediately (no-op).
- Ownership transfer:
  1. If `node->parent_` is not null: find the `unique_ptr<EntityNode>` in `node->parent_->children_` by linear scan matching `it->get() == node`. Release from old parent: `auto owned = std::move(*it); it = node->parent_->children_.erase(it);`.
  2. If `node->parent_` is null (root): find in `roots_` by linear scan. Release: `auto owned = std::move(*it); it = roots_.erase(it);`.
  3. If `new_parent` is not null: `new_parent->children_.push_back(std::move(owned)); node->parent_ = new_parent;`.
  4. If `new_parent` is null: `roots_.push_back(std::move(owned)); node->parent_ = nullptr;`.
- NOT `noexcept` (may allocate in children list push_back).

#### `World::lookup_node(EntityId id) noexcept -> EntityNode*`
- If `id.index >= slots_.size()`: return nullptr.
- If `!slots_[id.index].alive`: return nullptr.
- If `slots_[id.index].generation != id.generation`: return nullptr.
- Return `slots_[id.index].node.get()`.
- `noexcept`.

#### `World::get_transform(EntityId id) noexcept -> Transform&`
- `lookup_node(id)` — UB if null.
- Return `node->transform_`.
- `noexcept`.

#### `World::is_pending_destroy(EntityId id) const noexcept -> bool`
- `auto* node = lookup_node(id);`
- Return `node && node->pending_destroy_`.
- Returns `false` for null/stale handles.
- `noexcept`.

### World method noexcept guarantees

The following World methods are `noexcept`:
- `get_transform()` — pure lookup, no allocation.
- `is_pending_destroy()` — pure lookup, no allocation.
- `lookup_node()` — pure computation, no allocation.
- `flush_destroyed()` — reclaims existing resources, no allocation.
- `get_parent()` — pure lookup, no allocation.
- `get_child_count()` — pure getter, no allocation.
- `get_child()` — pure getter, no allocation.

The following World methods are NOT `noexcept`:
- `create_entity()` — allocates nodes and slots.
- `destroy_entity()` — may allocate (pending_destroy_ list growth).
- `create_child()` — allocates new node.
- `reparent()` — may allocate (children list push_back).
- `add_component()` — allocates component storage.
- `remove_component()` — may deallocate.
- `mark_for_destroy()` — may allocate (stack and pending_destroy_).

## Required tests

The test file `tests/scene_graph_tests.cpp` must contain the following Catch2 v3 TEST_CASE entries. All tests are headless (no display, no GPU required). Use `1e-5f` tolerance for float comparison via `Catch::Approx`.

All component types used in tests must derive from `Component` and be defined in an anonymous namespace within the test file.

### EntityId tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-01 | `"EntityId default construction"` | `[scene]` `[entity_id]` | Default `EntityId{}` has `index == 0` and `generation == 0`. |
| T-02 | `"EntityId::none() returns sentinel"` | `[scene]` `[entity_id]` | `EntityId::none().index == UINT32_MAX` and `EntityId::none().generation == UINT32_MAX`. |
| T-03 | `"EntityId comparison"` | `[scene]` `[entity_id]` | `EntityId{0,0} == EntityId{0,0}`. `EntityId{0,0} != EntityId{1,0}`. `EntityId::none() == EntityId::none()`. A valid `EntityId{0,0} != EntityId::none()`. |
| T-04 | `"EntityId static_asserts pass"` | `[scene]` `[entity_id]` | Verify `sizeof(EntityId) == 8` and `std::is_trivially_copyable_v<EntityId>` — tested via static_assert in header but also verifiable in a test. |

### Transform tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-05 | `"Transform default values"` | `[scene]` `[transform]` | Default `Transform{}` has `position == Vec3::zero()`, `rotation == Quat::identity()`, `scale == Vec3::one()`. |
| T-06 | `"Transform::local_matrix() TRS order"` | `[scene]` `[transform]` | For transform with position `(1,2,3)`, rotation 90° around Y, scale `(2,1,1)`, compute `local_matrix()` and verify a point transforms correctly. |
| T-07 | `"Transform::world_matrix() for root entity equals local_matrix()"` | `[scene]` `[transform]` | Create a root entity, set position `(10,0,0)`. `entity.transform().world_matrix(entity)` equals `entity.transform().local_matrix()` within `1e-5f`. |
| T-08 | `"Transform::world_matrix() accumulates parent transforms"` | `[scene]` `[transform]` | Create parent at `(10,0,0)` with identity rotation/scale. Create child via `parent.create_child()`. Child's `world_matrix()` has translation `(10,0,0)`. |
| T-09 | `"Transform::world_matrix() accumulates grandparent transforms"` | `[scene]` `[transform]` | Grandparent → parent → child chain with different transforms. Verify `world_matrix()` accumulates all three levels correctly within `1e-5f`. |

### Component tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-10 | `"Component base class"` | `[scene]` `[component]` | Define `struct MyComp : Component { int x; };``. Verify it compiles and can be destroyed through `Component*`. Verify non-copyable, non-movable. |
| T-11 | `"Entity::add_component and get_component"` | `[scene]` `[component]` | `entity.add_component<MyComp>(42)`, `auto opt = entity.get_component<MyComp>()` — `opt.has_value()` true, `opt->x == 42`. |
| T-12 | `"Entity::add_component unique per type"` | `[scene]` `[component]` | Two distinct component types on the same entity are independently stored and retrieved. |
| T-13 | `"Entity::get_component returns nullopt for missing type"` | `[scene]` `[component]` | `entity.get_component<OtherComp>()` returns `std::nullopt`. |
| T-14 | `"Entity::remove_component"` | `[scene]` `[component]` | Add component, remove returns `true`, `get_component` returns `std::nullopt`. Second remove returns `false`. |
| T-15 | `"Entity::get_component returns nullopt on pending-destroy entity"` | `[scene]` `[component]` | Mark entity for destruction, `get_component` returns `std::nullopt`. |
| T-16 | `"Entity::get_component const overload"` | `[scene]` `[component]` | `const Entity&` version returns `std::optional<const T&>`. |

### Entity lifecycle tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-17 | `"Entity::create returns valid non-null entity"` | `[scene]` `[entity]` | `Entity::create(world)` returns entity with `id() != EntityId::none()` and `&entity.world() == &world`. |
| T-18 | `"Entity::none() returns null entity"` | `[scene]` `[entity]` | `Entity::none().id() == EntityId::none()`, `Entity::none().is_pending_destroy() == false`. |
| T-19 | `"Entity comparison"` | `[scene]` `[entity]` | Two handles to the same entity are equal. Handles to different entities are not equal. Null entity equals only null entity. |
| T-20 | `"Entity::transform modify persists"` | `[scene]` `[entity]` | Set `entity.transform().position = Vec3(1,2,3)`. Read back: `entity.transform().position == Vec3(1,2,3)`. |
| T-21 | `"Entity::destroy and is_pending_destroy"` | `[scene]` `[entity]` | `entity.is_pending_destroy()` is `false`. After `entity.destroy()`, `entity.is_pending_destroy()` is `true`. |
| T-22 | `"Entity::destroy idempotent"` | `[scene]` `[entity]` | Call `entity.destroy()` twice. `is_pending_destroy()` returns `true` after both calls. No crash. |

### World tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-23 | `"World::flush_destroyed on empty world is safe"` | `[scene]` `[world]` | Call `flush_destroyed()` on fresh `World`. No crash. Call again with entities that exist but are not destroyed. No crash. |
| T-24 | `"World::flush_destroyed reclaims entities"` | `[scene]` `[world]` | Create entity, destroy it, flush, create another. The new entity has a different (higher or wrapped) `generation` than the destroyed one. |
| T-25 | `"World::destroy_entity equivalent to entity.destroy()"` | `[scene]` `[world]` | Call `world.destroy_entity(entity)` and verify `entity.is_pending_destroy() == true`. |
| T-26 | `"World::flush_destroyed idempotent"` | `[scene]` `[world]` | Call `flush_destroyed()`, then call again. Second call is a no-op with no crash. |

### Hierarchy tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-27 | `"Entity::create_child creates child with parent link"` | `[scene]` `[hierarchy]` | `parent.create_child()` returns entity whose `parent()` equals `parent`. |
| T-28 | `"Entity::child_count and get_child after create_child"` | `[scene]` `[hierarchy]` | After creating one child, `parent.child_count() == 1` and `parent.get_child(0)` equals the child. |
| T-29 | `"Entity::reparent to root"` | `[scene]` `[hierarchy]` | Create parent P and child C. Call `C.reparent(Entity::none())`. `C.parent().id() == EntityId::none()`. `P.child_count() == 0`. |
| T-30 | `"Entity::reparent to another parent"` | `[scene]` `[hierarchy]` | Create P1, P2, C (child of P1). `C.reparent(P2)`. `C.parent() == P2`. `P1.child_count() == 0`. `P2.child_count() == 1` and `P2.get_child(0) == C`. |
| T-31 | `"Entity::reparent current parent is no-op"` | `[scene]` `[hierarchy]` | `child.reparent(child.parent())` — children lists unchanged, no crash. |

### Destroy cascade tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-32 | `"Entity::destroy cascades to children"` | `[scene]` `[destroy]` | Create parent → child → grandchild chain. Destroy parent. All three `is_pending_destroy()` return true. |
| T-33 | `"flush_destroyed after cascade reclaims all"` | `[scene]` `[destroy]` | Same chain as T-32. After `flush_destroyed()`, slot reuse shows generation incremented for all three. |
| T-34 | `"Deep hierarchy destroy does not stack overflow"` | `[scene]` `[destroy]` | Create chain of 10,000 parent-child entities. Destroy root. No stack overflow. |
| T-35 | `"flush_destroyed with no pending entities is no-op"` | `[scene]` `[destroy]` | Call `flush_destroyed()` immediately after `World` construction. No crash. |

### world_matrix tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-36 | `"Entity::world_matrix convenience method"` | `[scene]` `[transform]` | `entity.world_matrix()` equals `entity.transform().world_matrix(entity)` within `1e-5f`. |
| T-37 | `"Entity::world_matrix for chain with different transforms"` | `[scene]` `[transform]` | Chain of 3 entities with different positions/rotations/scales. Verify computed world matrix matches expected output within `1e-5f`. |

### Null entity safety tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-38 | `"Null entity safe operations"` | `[scene]` `[null_entity]` | `Entity::none().id() == EntityId::none()`. `Entity::none().is_pending_destroy() == false`. `Entity::none() == Entity::none()`. `Entity::none() != Entity::create(world)`. |

### Pending-destroy entity behavior tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-39 | `"Pending-destroy entity get_component returns nullopt"` | `[scene]` `[pending_destroy]` | Add component, destroy entity, `get_component<T>()` returns `std::nullopt`. |
| T-40 | `"Pending-destroy entity transform is accessible"` | `[scene]` `[pending_destroy]` | Destroy entity, `transform()` returns valid reference. |

### Cross-world and UB contract tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-41 | `"Null entity get_component returns nullopt"` | `[scene]` `[null_entity]` | `Entity::none().get_component<MyComp>()` returns `std::nullopt` (read-only safe). |
| T-42 | `"Null entity child_count returns 0"` | `[scene]` `[null_entity]` | `Entity::none().child_count() == 0` (read-only safe). |

### Edge case tests

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-43 | `"flush_destroyed multiple calls"` | `[scene]` `[world]` | Call `flush_destroyed()` twice without intervening `create_entity()`. Second call is no-op. |
| T-44 | `"World destructor with pending entities"` | `[scene]` `[world]` | Create entities, destroy some but do NOT flush. Destroy `World`. No crash, no leak (ASAN-clean). |
| T-45 | `"Destroyed entity still visible in parent before flush"` | `[scene]` `[hierarchy]` | Parent with 2 children. Destroy C1. Before flush: `parent.child_count() == 2`. After flush: `parent.child_count() == 1`, `parent.get_child(0) == C2`. |
| T-46 | `"flush_destroyed reverse depth order"` | `[scene]` `[destroy]` | Chain root → parent → child, each with a counter component. Destroy root, flush. The destruction order counter shows child (3), parent (2), root (1). |
| T-47 | `"Component destructor called on entity destroy"` | `[scene]` `[component]` | Add component with destructor flag. Destroy entity, flush. Verify flag is set. |
| T-48 | `"World destruction with pending entities — components destroyed"` | `[scene]` `[world]` | Create entities with components, mark some for destruction. Destroy World. Verify component destructors ran via flag. |
| T-49 | `"Stale EntityId after flush"` | `[scene]` `[entity]` | Capture `EntityId` before destroy. Flush. Reconstruct `Entity` from stale id: `lookup` returns null, `is_pending_destroy()` returns false. |

## Edge cases

All edge cases from the spec (see `docs/specs/scene-graph/spec.md#edge-cases`) must be handled. The implementation must handle these cases as follows:

| Edge case | Required behavior |
|---|---|
| `entity.destroy()` called multiple times | Idempotent — second and subsequent calls are no-ops. `is_pending_destroy()` continues to return `true`. |
| `flush_destroyed()` called with empty pending list | No-op. No crash, no iteration. |
| `entity.reparent(entity)` — self-reparent | Undefined behavior. Debug builds may assert. |
| `entity.reparent(descendant)` — cycle creation | Undefined behavior. Debug builds may assert (optional cycle detection via ancestor walk). |
| `entity.create_child()` on a pending-destroy entity | Undefined behavior. |
| `entity.add_component<T>()` when `T` already exists | Undefined behavior. Debug builds may assert. |
| `entity.get_component<T>()` on a pending-destroy entity | Returns `std::nullopt` for all types (read-only safe). |
| `entity.transform()` on a pending-destroy entity | Returns a valid reference (Transform is inline storage and survives until flush). |
| `entity.parent()` on a pending-destroy entity | Returns the pre-destroy parent value (parent link not severed until flush). |
| `entity.child_count()` / `get_child()` on a pending-destroy entity | Children that are NOT pending_destroy are still returned normally. The entity's own pending status does not affect children access. |
| Parent's view: `parent.child_count()` / `parent.get_child()` after a child is destroyed | The destroyed child remains visible until `flush_destroyed()`. Between `destroy()` and flush, `child_count()` still includes it, ensuring iteration consistency during the mark phase. |
| `entity.add_component()` / `remove_component()` on a pending-destroy entity | Undefined behavior (mutating operation). |
| `entity.reparent()` on a pending-destroy entity | Undefined behavior (mutating operation). |
| `entity.reparent()` across different `World` instances | Undefined behavior. Debug builds may assert cross-world check. |
| `World` destroyed with pending entities | `~World()` destroys all entities regardless of pending state. No crash. All storage is reclaimed. |
| `get_child(index)` with out-of-bounds index | Undefined behavior (index >= child_count()). Debug builds may assert. |
| `child_count()` and `get_child()` after children list mutation (create_child, reparent) | The new child count and children are immediately visible. |
| Multiple `flush_destroyed()` calls without intervening `create_entity()` | Second call is a no-op. |
| Entity destruction cascade with deep hierarchy (10,000+ levels) | `destroy()` cascade MUST use iterative traversal (not recursion) to avoid stack overflow. |
| `reparent()` to a pending-destroy parent | Undefined behavior. |
| `world_matrix()` called on entity with a cycle in ancestor chain | Undefined behavior (likely infinite loop). Can only happen if `reparent()` created a cycle despite the UB contract. |

## RTTI requirement

The component dispatch (`get_component<T>()`, `remove_component<T>()`) uses `dynamic_cast<T*>()` for type-safe component lookup. This requires C++ RTTI (`-frtti`, which is enabled by default). Building with `-fno-rtti` will cause compilation errors in the scene graph module. If `-fno-rtti` support is needed in the future, the dispatch should be replaced with a static type ID pattern (e.g., a virtual `type_id()` method or a compile-time type counter).

## Compiler support note: `std::optional<T&>`

The `get_component<T>()` return type uses `std::optional<T&>` (C++26 feature P2988R12, adopted June 2024). The project's minimum compiler baseline (per ADR-001) is:

- **GCC 16+** — full support for `std::optional<T&>`
- **Clang 22+** — full support for `std::optional<T&>`

A fallback mechanism (e.g., `T*` with `nullptr` for absent) should be added if targeting compilers older than these versions in the future.

## Security impact

None. Scene graph types are pure memory management and spatial computation:
- No elevated privileges required.
- No secrets, credentials, or environment variables consumed.
- No I/O, network, or filesystem access.
- No access control restrictions — any code with a `World&` can create, destroy, or modify any entity.
- Precondition violations (null entity, stale handle, duplicate component) result in undefined behavior, not exploitable memory corruption in release builds. Debug builds may assert to aid development.
- `EntityId` generation counters prevent use-after-free in typical scenarios.

## Data and migration impact

None. The scene graph is a new module with no existing data to migrate:
- No schema changes to existing code.
- No seed data or data migration required.
- No persistent state.

## API compatibility impact

The scene graph is a new public API under `buddd::engine` namespace:
- No backward compatibility concerns (no prior API to break).
- All types are in new headers under `src/engine/scene/` — existing code is unaffected until it includes these headers.
- The API is designed for forward compatibility with future ECS flat-array storage: the `World` class hides the internal storage strategy, and all entity operations go through the `Entity` handle. No downstream code accesses `EntityNode` or internal storage directly.

## Documentation impact

None. The wiki and documentation are out of scope for implementation contracts. The scene graph API is self-documenting via the headers. The existing spec serves as the primary API reference.

## ADR impact

A new ADR (ADR-005) documents the project's adoption of `std::optional<T&>` for component lookup APIs, with updated minimum compiler baseline (GCC 16+, Clang 22+). ADR-001 has been updated to reflect the new baseline.

The scene graph's exception to ADR-001 (not using `Result<T>`) follows the same pattern established by `draw()`/`draw_indexed()` in the render pipeline (IMPL-005). No additional ADR is required for that exception.

## Constitution impact

None. The implementation respects CONST-001 (architecture boundaries — no GLM/SDL3/OpenGL in `src/engine/scene/` public headers) and CONST-002 (testing policy — all testable code has corresponding tests).

## Done criteria

The implementation is complete when ALL of the following are satisfied:

### Build and compilation
- [ ] All 7 new files under `src/engine/scene/` compile without errors:
  - `src/engine/scene/entity_id.h`
  - `src/engine/scene/transform.h`
  - `src/engine/scene/component.h`
  - `src/engine/scene/entity.h`
  - `src/engine/scene/entity.cpp`
  - `src/engine/scene/world.h`
  - `src/engine/scene/world.cpp`
- [ ] `tests/CMakeLists.txt` is modified to include `scene_graph_tests.cpp` in both branches.
- [ ] `tests/scene_graph_tests.cpp` compiles without errors.
- [ ] `cmake --build --preset debug` succeeds with no warnings related to scene graph code.
- [ ] No GLM, SDL3, or OpenGL headers are included in any file under `src/engine/scene/`. Verified by code review.
- [ ] `static_assert(sizeof(EntityId) == 8)` and `static_assert(std::is_trivially_copyable_v<EntityId>)` both compile and pass.
- [ ] `static_assert(sizeof(Entity) == 16)` compiles and passes.

### Test results
- [ ] All 49 test cases pass (`ctest --preset debug` or `./build/debug/tests/buddd_tests [scene]`):
  - T-01 through T-04 (EntityId): ALL pass
  - T-05 through T-09 (Transform): ALL pass
  - T-10 through T-16 (Component): ALL pass
  - T-17 through T-22 (Entity lifecycle): ALL pass
  - T-23 through T-26 (World): ALL pass
  - T-27 through T-31 (Hierarchy): ALL pass
  - T-32 through T-35 (Destroy cascade): ALL pass
  - T-36 through T-37 (world_matrix): ALL pass
  - T-38 (Null entity safety): passes
  - T-39 through T-40 (Pending-destroy): ALL pass
  - T-41 through T-42 (Null entity safety): ALL pass
  - T-43 through T-49 (Edge cases + lifecycle): ALL pass

### Acceptance criteria coverage
- [ ] AC-001 (EntityId properties): covered by T-01, T-02, T-03, T-04
- [ ] AC-002 (Transform defaults): covered by T-05
- [ ] AC-003 (local_matrix TRS): covered by T-06
- [ ] AC-004 (world_matrix): covered by T-07, T-08, T-09
- [ ] AC-005 (Component base class): covered by T-10
- [ ] AC-006 (World create entity): covered by T-17
- [ ] AC-007 (Transform modify): covered by T-20
- [ ] AC-008 (add/get component): covered by T-11, T-12, T-13, T-16
- [ ] AC-009 (remove component): covered by T-14
- [ ] AC-010 (destroy/is_pending_destroy): covered by T-21, T-22
- [ ] AC-011 (flush_destroyed reclaims): covered by T-24
- [ ] AC-012 (create_child hierarchy): covered by T-27, T-28
- [ ] AC-013 (reparent): covered by T-29, T-30, T-31
- [ ] AC-014 (destroy cascade to children): covered by T-32, T-33
- [ ] AC-015 (flush_destroyed safe with empty list): covered by T-23, T-35
- [ ] AC-016 (Entity comparison): covered by T-19
- [ ] AC-017 (Entity::create factory): covered by T-17
- [ ] AC-018 (at most one component per type): covered by T-12; UB case documented but not tested
- [ ] AC-019 (files under src/engine/scene/): verified by file listing
- [ ] AC-020 (no GLM/SDL3/OpenGL): verified by code review
- [ ] AC-021 (sizeof(Entity) == 16): verified by static_assert
- [ ] AC-022 (null entity behavior): covered by T-18, T-38
- [ ] AC-023 (child_count / get_child): covered by T-28
- [ ] AC-024 (world_matrix convenience): covered by T-36
- [ ] AC-025 (world.destroy_entity equivalence): covered by T-25
- [ ] AC-026 (deep hierarchy stack safety): covered by T-34
- [ ] AC-027 (destroyed entity visible in parent before flush): covered by T-45
- [ ] AC-028 (flush_destroyed reverse depth order): covered by T-46
- [ ] AC-029 (components destroyed with entity): covered by T-47
- [ ] AC-030 (World destruction destroys all): covered by T-48
- [ ] AC-031 (stale EntityId detection): covered by T-49
- [ ] AC-032 (dangling Component pointer is UB): documentation only

### Memory safety
- [ ] ASAN build (`-fsanitize=address`) shows no leaks or use-after-free in scene graph tests.
- [ ] `World` destructor properly reclaims all entity and component memory without requiring explicit `flush_destroyed()`.
