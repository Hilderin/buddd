#include "scene/directional_light_component.h"

namespace buddd::engine {

DirectionalLightComponent::DirectionalLightComponent(math::Vec3 colour, float intensity)
    : colour_(colour), intensity_(intensity) {}

auto DirectionalLightComponent::colour() noexcept -> math::Vec3& {
    return colour_;
}

auto DirectionalLightComponent::colour() const noexcept -> const math::Vec3& {
    return colour_;
}

auto DirectionalLightComponent::intensity() noexcept -> float& {
    return intensity_;
}

auto DirectionalLightComponent::intensity() const noexcept -> float {
    return intensity_;
}

} // namespace buddd::engine
