#pragma once

#include "math/color.h"
#include "scene/component.h"

namespace buddd::engine {

/// Directional light: infinite light source.
/// Direction is derived from the entity's world rotation (-Z forward).
/// No position or range — affects all surfaces equally regardless of distance.
class DirectionalLightComponent : public Component {
public:
    DirectionalLightComponent(
        math::Color color = math::Color{1.0f, 1.0f, 1.0f},
        float intensity = 1.0f
    );

    auto color() noexcept -> math::Color&;
    auto color() const noexcept -> const math::Color&;
    auto intensity() noexcept -> float&;
    auto intensity() const noexcept -> float;

    auto on_attach() -> void override {}

private:
    math::Color color_{1.0f, 1.0f, 1.0f};
    float intensity_ = 1.0f;
};

} // namespace buddd::engine
