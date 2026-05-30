#pragma once

#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"

namespace buddd::engine {

// Forward declaration (Entity is defined in entity.h, which includes this header).
class Entity;

struct Transform {
    math::Vec3 position{math::Vec3::zero()};
    math::Quat rotation{math::Quat::identity()};
    math::Vec3 scale{math::Vec3::one()};

    auto local_matrix() const noexcept -> math::Mat4;

    /// Walks the parent chain of `entity`, accumulating local transforms
    /// root-to-leaf. Uses a fixed-size stack array (up to 4096 levels).
    /// Returns the world matrix for this transform's entity.
    auto world_matrix(const Entity& entity) const noexcept -> math::Mat4;
};

inline auto Transform::local_matrix() const noexcept -> math::Mat4 {
    return math::Mat4::translate(position)
         * rotation.to_mat4()
         * math::Mat4::scale(scale);
}

// Note: Transform::world_matrix() is defined after Entity is fully defined.
// See entity.h for the inline definition.

} // namespace buddd::engine
