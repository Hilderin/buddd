#include "scene/camera_component.h"
#include "scene/entity.h"     // Component::entity(), Entity::world()
#include "scene/world.h"      // World::register_camera(), unregister_camera()
#include "math/math.h"        // math::view_matrix(), math::look_at_rotation()

#include "log/log.h"

BUDDD_LOG_TAG("Scene:ECSCamera");

namespace buddd::engine {

CameraComponent::CameraComponent(float fov_y, float aspect, float near_plane, float far_plane)
    : fov_y_(fov_y), aspect_(aspect), near_(near_plane), far_(far_plane) {}

auto CameraComponent::set_perspective(float fov_y, float aspect, float near_plane, float far_plane) -> void {
    fov_y_ = fov_y;
    aspect_ = aspect;
    near_ = near_plane;
    far_ = far_plane;
}

auto CameraComponent::fov_y() const noexcept -> float { return fov_y_; }
auto CameraComponent::aspect() const noexcept -> float { return aspect_; }
auto CameraComponent::near_plane() const noexcept -> float { return near_; }
auto CameraComponent::far_plane() const noexcept -> float { return far_; }

auto CameraComponent::projection_matrix() const -> math::Mat4 {
    return math::Mat4::perspective(fov_y_, aspect_, near_, far_);
}

auto CameraComponent::view_matrix() const -> math::Mat4 {
    auto& t = entity().transform();
    return math::view_matrix(t.position, t.rotation);
}

auto CameraComponent::view_projection_matrix() const -> math::Mat4 {
    return projection_matrix() * view_matrix();
}

void CameraComponent::look_at(math::Vec3 target) {
    auto& t = entity().transform();
    t.rotation = math::look_at_rotation(t.position, target, math::Vec3::unit_y());
}

void CameraComponent::look_at(math::Vec3 eye, math::Vec3 center, math::Vec3 up) {
    auto& t = entity().transform();
    t.position = eye;
    t.rotation = math::look_at_rotation(eye, center, up);
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
