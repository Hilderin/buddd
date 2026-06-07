#pragma once

#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "scene/entity_id.h"
#include "scene/transform.h"
#include "scene/component.h"

namespace buddd::engine {

class World;

class Entity {
public:
    // Default constructor creates a null entity.
    Entity() noexcept = default;

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
    [[nodiscard]] auto get_component() const noexcept -> std::optional<const T&>;

    template<typename T>
    [[nodiscard]] auto get_component() noexcept -> std::optional<T&>;

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

private:
    friend class World;
    friend class Component;   // Component::entity() constructs Entity handles.

    World* world_ = nullptr;
    EntityId id_ = EntityId::none();

    Entity(World& world, EntityId id) noexcept;
};

static_assert(sizeof(Entity) == 16,
    "Entity must be 16 bytes (pointer + EntityId)");

// Note: Entity template method implementations (add_component, get_component,
// remove_component) are defined in world.h after World is fully defined.
// Transform::world_matrix() is defined below after Entity is fully defined.

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

// -- Component::entity() inline definition (requires Entity to be complete) --

inline auto Component::entity() const noexcept -> Entity {
    return Entity(*world_, entity_id_);
}

} // namespace buddd::engine
