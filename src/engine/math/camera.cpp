#include "camera.h"

#include <glm/mat3x3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace buddd::engine::math {

Camera::Camera(Vec3 position, Quat orientation,
               float fov_y, float aspect, float near_plane, float far_plane)
    : position_(position)
    , orientation_(orientation)
    , fov_y_(fov_y)
    , aspect_(aspect)
    , near_(near_plane)
    , far_(far_plane)
{}

auto Camera::position() const noexcept -> Vec3 { return position_; }
auto Camera::set_position(Vec3 position) -> void { position_ = position; }

auto Camera::orientation() const noexcept -> Quat { return orientation_; }
auto Camera::set_orientation(Quat orientation) -> void { orientation_ = orientation; }

void Camera::look_at(Vec3 target) {
    look_at(position_, target, Vec3::unit_y());
}

void Camera::look_at(Vec3 eye, Vec3 center, Vec3 up) {
    position_ = eye;
    Vec3 forward = (center - eye).normalized();
    Vec3 right = forward.cross(up).normalized();
    Vec3 ortho_up = right.cross(forward);

    // Build column-major 3x3 rotation matrix: [right | ortho_up | -forward]
    glm::mat3 rot_mat(1.0f);
    rot_mat[0] = right.glm();
    rot_mat[1] = ortho_up.glm();
    rot_mat[2] = (-forward).glm();

    orientation_ = Quat{glm::quat_cast(rot_mat)};
}

auto Camera::fov_y() const noexcept -> float { return fov_y_; }
auto Camera::aspect() const noexcept -> float { return aspect_; }
auto Camera::near_plane() const noexcept -> float { return near_; }
auto Camera::far_plane() const noexcept -> float { return far_; }

void Camera::set_perspective(float fov_y, float aspect, float near, float far) {
    fov_y_ = fov_y;
    aspect_ = aspect;
    near_ = near;
    far_ = far;
}

auto Camera::view_matrix() const -> Mat4 {
    Vec3 forward = orientation_ * Vec3(0.0f, 0.0f, -1.0f);
    Vec3 up = orientation_ * Vec3(0.0f, 1.0f, 0.0f);
    return Mat4::look_at(position_, position_ + forward, up);
}

auto Camera::projection_matrix() const -> Mat4 {
    return Mat4::perspective(fov_y_, aspect_, near_, far_);
}

auto Camera::view_projection_matrix() const -> Mat4 {
    return projection_matrix() * view_matrix();
}

} // namespace buddd::engine::math
