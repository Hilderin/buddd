#pragma once

#include <memory>
#include <optional>
#include <span>
#include <utility>
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

    // -- Internal EntityNode —
    // Defined here (not in world.cpp) so that template methods in this
    // header can access its members (components_, pending_destroy_).
    struct EntityNode {
        EntityId id_;
        Transform transform_;
        EntityNode* parent_ = nullptr;
        std::vector<std::unique_ptr<EntityNode>> children_;
        std::vector<std::unique_ptr<Component>> components_;
        World* world_ = nullptr;
        bool pending_destroy_ = false;
    };

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
    // Slots track entity identity (generation counter + alive flag) plus
    // a non-owning pointer to the EntityNode. The node itself is owned
    // either by roots_ (for root entities) or by a parent's children_
    // vector (for child entities).
    struct Slot {
        EntityNode* node = nullptr;
        uint32_t generation = 0;
        bool alive = false;
    };

    std::vector<Slot> slots_;
    std::vector<std::unique_ptr<EntityNode>> roots_;
    std::vector<EntityNode*> pending_destroy_;
    std::vector<uint32_t> free_slots_;
    uint32_t next_slot_ = 0;
};

// -- World template method implementations (inline, defined in header) --

template<typename T, typename... Args>
inline auto World::add_component(EntityId id, Args&&... args) -> T& {
    auto* node = lookup_node(id);
    // UB if node is null, slot dead, or component of type T already exists.
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
    for (auto it = node->components_.begin(); it != node->components_.end(); ++it) {
        if (dynamic_cast<T*>(it->get())) {
            node->components_.erase(it);
            return true;
        }
    }
    return false;
}

// -- Entity template method implementations (delegate to World) --
// These are defined here (not in entity.h) because they require World
// to be a complete type.

template<typename T, typename... Args>
inline auto Entity::add_component(Args&&... args) -> T& {
    return world_->add_component<T>(id_, std::forward<Args>(args)...);
}

template<typename T>
inline auto Entity::get_component() const noexcept -> std::optional<const T&> {
    if (!world_) return std::nullopt;
    return world_->get_component<T>(id_);
}

template<typename T>
inline auto Entity::get_component() noexcept -> std::optional<T&> {
    if (!world_) return std::nullopt;
    return world_->get_component<T>(id_);
}

template<typename T>
inline auto Entity::remove_component() -> bool {
    return world_->remove_component<T>(id_);
}

} // namespace buddd::engine
