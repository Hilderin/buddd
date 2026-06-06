#include "scene/camera_component.h"
#include "scene/entity.h"     // Component::entity(), Entity::world()
#include "scene/world.h"      // World::register_camera(), unregister_camera()

#include "log/log.h"

BUDDD_LOG_TAG("Scene:ECSCamera");

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
    BUDDD_LOG_DEBUG("CameraComponent: registered entity {} as active camera",
        entity().id().index);
}

CameraComponent::~CameraComponent() {
    if (world_) {
        world_->unregister_camera(*this);
        BUDDD_LOG_DEBUG("CameraComponent: unregistered entity {}",
            entity_id_.index);
    }
}

} // namespace buddd::engine
