#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "mat4.h"
#include "quat.h"

namespace buddd::engine::math {

// -- Mathematical constants (constexpr variables) --
inline constexpr float pi       = 3.14159265358979323846f;
inline constexpr float half_pi  = 1.57079632679489661923f;
inline constexpr float two_pi   = 6.28318530717958647693f;
inline constexpr float epsilon  = 1.0e-6f;

// -- Conversion --
inline auto radians(float degrees) noexcept -> float { return glm::radians(degrees); }
inline auto degrees(float radians) noexcept -> float { return glm::degrees(radians); }

// -- Common math functions --
inline auto sin(float angle) noexcept -> float { return glm::sin(angle); }
inline auto cos(float angle) noexcept -> float { return glm::cos(angle); }
inline auto tan(float angle) noexcept -> float { return glm::tan(angle); }
inline auto asin(float x) noexcept -> float { return glm::asin(x); }
inline auto acos(float x) noexcept -> float { return glm::acos(x); }
inline auto atan(float y_over_x) noexcept -> float { return glm::atan(y_over_x); }
inline auto atan2(float y, float x) noexcept -> float { return glm::atan(y, x); }
inline auto sqrt(float x) noexcept -> float { return glm::sqrt(x); }

// -- Look-at rotation (contains GLM-dependent math; must be in math/ per ADR-002/ADR-019) --
/// Computes a rotation quaternion that orients the forward direction
/// (0,0,-1) to look from `eye` toward `center` with the given `up` vector.
inline auto look_at_rotation(const Vec3& eye, const Vec3& center, const Vec3& up) noexcept -> Quat {
    Vec3 forward = (center - eye).normalized();
    Vec3 right = forward.cross(up).normalized();
    Vec3 ortho_up = right.cross(forward);
    glm::mat3 rot_mat(1.0f);
    rot_mat[0] = right.glm();
    rot_mat[1] = ortho_up.glm();
    rot_mat[2] = (-forward).glm();
    return Quat{glm::quat_cast(rot_mat)};
}

// -- View matrix --
inline auto view_matrix(const Vec3& position, const Quat& orientation) noexcept -> Mat4 {
    Vec3 forward = orientation * Vec3(0.0f, 0.0f, -1.0f);
    Vec3 up = orientation * Vec3(0.0f, 1.0f, 0.0f);
    return Mat4::look_at(position, position + forward, up);
}

} // namespace buddd::engine::math
