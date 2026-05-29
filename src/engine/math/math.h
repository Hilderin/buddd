#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "mat4.h"
#include "quat.h"
#include "camera.h"

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

} // namespace buddd::engine::math
