#include "scene/directional_light_component.h"

namespace buddd::engine {

DirectionalLightComponent::DirectionalLightComponent(math::Vec3 color, float intensity)
    : color_(color), intensity_(intensity) {}

auto DirectionalLightComponent::color() noexcept -> math::Vec3& {
    return color_;
}

auto DirectionalLightComponent::color() const noexcept -> const math::Vec3& {
    return color_;
}

auto DirectionalLightComponent::intensity() noexcept -> float& {
    return intensity_;
}

auto DirectionalLightComponent::intensity() const noexcept -> float {
    return intensity_;
}

} // namespace buddd::engine
