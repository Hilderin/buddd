#pragma once

#include "math/vec3.h"
#include "scene/component.h"

namespace buddd::engine {

/// Point light: omni-directional light with position and range.
/// Position is derived from the entity's world_matrix() translation.
class PointLightComponent : public Component {
public:
    PointLightComponent(
        math::Vec3 color = math::Vec3{1.0f, 1.0f, 1.0f},
        float intensity = 1.0f,
        float range = 10.0f
    );

    auto color() noexcept -> math::Vec3&;
    auto color() const noexcept -> const math::Vec3&;
    auto intensity() noexcept -> float&;
    auto intensity() const noexcept -> float;
    auto range() noexcept -> float&;
    auto range() const noexcept -> float;

    auto on_attach() -> void override {}

private:
    math::Vec3 color_{1.0f, 1.0f, 1.0f};
    float intensity_ = 1.0f;
    float range_ = 10.0f;
};

} // namespace buddd::engine
