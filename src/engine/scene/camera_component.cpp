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
