#pragma once

#include "scene/entity_id.h"

namespace buddd::engine {

class Entity;
class World;

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

} // namespace buddd::engine
