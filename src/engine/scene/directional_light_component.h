#pragma once

#include "math/vec3.h"
#include "scene/component.h"

namespace buddd::engine {

/// Directional light: infinite light source.
/// Direction is derived from the entity's world rotation (-Z forward).
/// No position or range — affects all surfaces equally regardless of distance.
class DirectionalLightComponent : public Component {
public:
    DirectionalLightComponent(
        math::Vec3 colour = math::Vec3{1.0f, 1.0f, 1.0f},
        float intensity = 1.0f
    );

    auto colour() noexcept -> math::Vec3&;
    auto colour() const noexcept -> const math::Vec3&;
    auto intensity() noexcept -> float&;
    auto intensity() const noexcept -> float;

    auto on_attach() -> void override {}

private:
    math::Vec3 colour_{1.0f, 1.0f, 1.0f};
    float intensity_ = 1.0f;
};

} // namespace buddd::engine
