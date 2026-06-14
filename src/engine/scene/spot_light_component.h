#pragma once

#include "math/color.h"
#include "scene/component.h"

namespace buddd::engine {

/// Spot light: conical light with position, direction, and cone angles.
/// Position from entity world_matrix() translation.
/// Direction from entity world rotation (-Z forward).
/// inner_angle and outer_angle define the cone falloff in radians.
class SpotLightComponent : public Component {
public:
    SpotLightComponent(
        math::Color color = math::Color{1.0f, 1.0f, 1.0f},
        float intensity = 1.0f,
        float range = 10.0f,
        float inner_angle = 0.785f,    // ~45 degrees
        float outer_angle = 1.047f     // ~60 degrees
    );

    auto color() noexcept -> math::Color&;
    auto color() const noexcept -> const math::Color&;
    auto intensity() noexcept -> float&;
    auto intensity() const noexcept -> float;
    auto range() noexcept -> float&;
    auto range() const noexcept -> float;
    auto inner_angle() noexcept -> float&;
    auto inner_angle() const noexcept -> float;
    auto outer_angle() noexcept -> float&;
    auto outer_angle() const noexcept -> float;

    auto on_attach() -> void override {}

private:
    math::Color color_{1.0f, 1.0f, 1.0f};
    float intensity_ = 1.0f;
    float range_ = 10.0f;
    float inner_angle_ = 0.785f;
    float outer_angle_ = 1.047f;
};

} // namespace buddd::engine
