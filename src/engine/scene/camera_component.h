#pragma once

#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "scene/component.h"

namespace buddd::engine {

class CameraComponent : public Component {
public:
    // -- Constructors --
    CameraComponent() = default;
    CameraComponent(float fov_y, float aspect, float near_plane, float far_plane);

    // -- Projection setters/getters --
    auto set_perspective(float fov_y, float aspect, float near_plane, float far_plane) -> void;
    auto fov_y() const noexcept -> float;
    auto aspect() const noexcept -> float;
    auto near_plane() const noexcept -> float;
    auto far_plane() const noexcept -> float;

    // -- Matrix computation (recomputed per call, no caching) --
    auto projection_matrix() const -> math::Mat4;
    auto view_matrix() const -> math::Mat4;                  // uses entity().transform()
    auto view_projection_matrix() const -> math::Mat4;       // projection * view

    // -- Look-at convenience (modifies entity Transform) --
    /// Orients the entity's rotation to look at `target` (keeps current position).
    auto look_at(math::Vec3 target) -> void;
    /// Sets entity's position to `eye`, orients to look at `center`.
    auto look_at(math::Vec3 eye, math::Vec3 center, math::Vec3 up) -> void;

    // -- Lifecycle (unchanged behavior) --
    auto on_attach() -> void override;
    ~CameraComponent() override;

private:
    float fov_y_ = 1.0471975512f;    // 60 degrees in radians (pi/3)
    float aspect_ = 16.0f / 9.0f;
    float near_ = 0.1f;
    float far_ = 100.0f;
};

} // namespace buddd::engine
