#pragma once

#include "math/vec4.h"

#include <cstdint>

namespace buddd::engine::detail {

constexpr int k_max_lights = 8;

/// Per-light data packed for GPU uniform passing.
/// Each field maps to the corresponding GLSL flat array.
struct LightData {
    math::Vec4 position_or_dir; // .xyz = position/direction, .w = type: 0=directional, 1=point, 2=spot
    math::Vec4 color;          // .rgb = color * intensity pre-multiplied, .a = unused
    float range;                // Attenuation range (ignored for directional)
    math::Vec4 spot_direction;  // For spot lights: normalized direction (.w unused)
    float inner_cone_cos;       // Cosine of inner half-angle (spot only)
    float outer_cone_cos;       // Cosine of outer half-angle (spot only)
};
static_assert(sizeof(LightData) == sizeof(math::Vec4) * 3 + sizeof(float) * 3,
              "LightData struct size must match packed layout");

} // namespace buddd::engine::detail
