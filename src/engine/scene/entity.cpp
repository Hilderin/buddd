#include "scene/entity.h"
#include "debug/assert.h"
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
    if (!world_) return false;
    return world_->is_pending_destroy(id_);
}

auto Entity::transform() noexcept -> Transform& {
    BUDDD_ASSERT(world_ != nullptr);
    return world_->get_transform(id_);
}

auto Entity::transform() const noexcept -> const Transform& {
    return world_->get_transform(id_);
}

auto Entity::parent() const noexcept -> Entity {
    return world_->get_parent(id_);
}

auto Entity::child_count() const noexcept -> size_t {
    if (!world_) return 0;
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
