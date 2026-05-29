#pragma once

#include "vec3.h"
#include "quat.h"
#include "mat4.h"

namespace buddd::engine::math {

class Camera {
public:
    /// Default: position (0,0,0), identity orientation, 60 FOV, 16:9 aspect, near 0.1, far 100.
    Camera() = default;

    /// Convenience constructor: sets position, orientation, and perspective parameters.
    Camera(Vec3 position, Quat orientation,
           float fov_y, float aspect, float near_plane, float far_plane);

    // -- Position / orientation --
    auto position() const noexcept -> Vec3;
    auto set_position(Vec3 position) -> void;

    auto orientation() const noexcept -> Quat;
    auto set_orientation(Quat orientation) -> void;

    // -- Look-at convenience --
    /// Orients the camera to look at `target` without changing position.
    auto look_at(Vec3 target) -> void;
    /// Sets both position and orientation to look from `eye` at `center` with given `up`.
    auto look_at(Vec3 eye, Vec3 center, Vec3 up) -> void;

    // -- Projection parameters (perspective) --
    auto set_perspective(float fov_y, float aspect, float near, float far) -> void;
    auto fov_y() const noexcept -> float;
    auto aspect() const noexcept -> float;
    auto near_plane() const noexcept -> float;
    auto far_plane() const noexcept -> float;

    // -- Matrix computation (recomputed on each call; no caching) --
    auto view_matrix() const -> Mat4;
    auto projection_matrix() const -> Mat4;
    auto view_projection_matrix() const -> Mat4;

private:
    Vec3 position_{0.0f, 0.0f, 0.0f};
    Quat orientation_{Quat::identity()};
    float fov_y_{1.0471975512f};        // 60 degrees in radians (pi/3)
    float aspect_{16.0f / 9.0f};
    float near_{0.1f};
    float far_{100.0f};
};

} // namespace buddd::engine::math
