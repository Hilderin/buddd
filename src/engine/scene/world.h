#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "scene/entity_id.h"
#include "scene/entity.h"
#include "scene/transform.h"
#include "scene/component.h"
#include "scene/entity_source.h"
#include "scene/updatable.h"
#include "engine_context.h"

namespace buddd::engine {

class CameraComponent;  // forward declaration
class SceneSaver;       // forward declaration (for friend access)

class World {
public:
    World();
    ~World();

    World(const World&) = delete;
    auto operator=(const World&) -> World& = delete;
    World(World&&) = delete;
    auto operator=(World&&) -> World& = delete;

    // -- Entity lifecycle --
    auto add_entity() -> Entity;
    void destroy_entity(Entity entity);
    void flush_destroyed() noexcept;

    // -- Internal (called by Entity) --
    auto is_pending_destroy(EntityId id) const noexcept -> bool;

    /// Returns the Transform for the given entity.
    /// Returned reference is valid until the entity is destroyed or
    /// the World is destroyed. Do not store across frames.
    auto get_transform(EntityId id) noexcept -> Transform&;

    /// Returns the Transform for the given entity (const).
    /// Returned reference is valid until the entity is destroyed or
    /// the World is destroyed. Do not store across frames.
    auto get_transform(EntityId id) const noexcept -> const Transform&;

    // -- Component management (called by Entity templates) --
    template<typename T, typename... Args>
    auto add_component(EntityId id, Args&&... args) -> T&;

    /// Returns a component of type T for the given entity.
    /// Returned reference is valid until the component is removed,
    /// the entity is destroyed, or the World is destroyed.
    /// Do not store across frames.
    template<typename T>
    [[nodiscard]] auto get_component(EntityId id) noexcept -> std::optional<T&>;

    /// Returns a component of type T for the given entity (const).
    /// Returned reference is valid until the component is removed,
    /// the entity is destroyed, or the World is destroyed.
    /// Do not store across frames.
    template<typename T>
    [[nodiscard]] auto get_component(EntityId id) const noexcept -> std::optional<const T&>;

    template<typename T>
    auto remove_component(EntityId id) -> bool;

    // -- Updatable auto-dispatch --
    /// Iterates all registered Updatable components, calling update() on each.
    auto update_updatables(const EngineContext& ctx) -> void;

    // -- Type-based iteration --
    template<typename T, typename Func>
    requires std::is_base_of_v<Component, T>
    auto each(Func&& func) -> size_t;

    // -- Entity introspection --
    /// Returns the number of currently alive (not pending destroy) entities.
    auto entity_count() const noexcept -> size_t;

    /// Returns the number of root entities (including pending_destroy — caller should filter).
    auto root_entity_count() const noexcept -> size_t;
    /// Returns the root entity at the given index. Returns Entity{} if index out of bounds.
    auto get_root_entity(size_t index) const noexcept -> Entity;

    // -- Camera registration --
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
    [[nodiscard]] auto active_camera() const noexcept -> std::optional<CameraComponent&>;

    /// Attach a runtime-typed component to an entity (for SceneLoader / deserialization).
    /// The component is moved into the World's storage.
    auto add_component_raw(EntityId id, std::unique_ptr<Component> component) -> Component&;

private:
    friend class Entity;
    friend class SceneSaver;

    // -- Internal EntityNode —
    // Defined here (not in world.cpp) so that template methods in this
    // header can access its members (components_, pending_destroy_).
    struct EntityNode {
        EntityId id_;
        Transform transform_;
        std::string name_;
        EntitySource source_;
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

    // -- Entity name --
    auto get_name(EntityId id) const noexcept -> const std::string&;
    auto set_name(EntityId id, const std::string& name) -> void;

    // -- Entity source --
    auto get_source(EntityId id) const noexcept -> const EntitySource&;
    auto set_source(EntityId id, const EntitySource& source) -> void;

    // -- Component raw iteration (for SceneSaver) --
    auto component_count(EntityId id) const noexcept -> size_t;
    auto get_component_at(EntityId id, size_t index) noexcept -> Component&;
    auto get_component_at(EntityId id, size_t index) const noexcept -> const Component&;

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

    // -- Updatable registry (raw pointers, non-owning) --
    std::vector<Updatable*> updatables_;

    // ADR-011: raw pointer allowed as private data member (implementation detail).
    // Non-owning observer; must not be stored across frames or after
    // the pointed-to CameraComponent is destroyed.
    CameraComponent* active_camera_ = nullptr;
};

// -- World template method implementations (inline, defined in header) --

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

    // Auto-register Updatable components
    if constexpr (std::is_base_of_v<Updatable, T>) {
        updatables_.push_back(static_cast<Updatable*>(ptr));
    }

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

// -- World::each<T>() implementation --

template<typename T, typename Func>
requires std::is_base_of_v<Component, T>
inline auto World::each(Func&& func) -> size_t {
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

// Camera API implementations are in world.cpp (require CameraComponent to be complete).

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
