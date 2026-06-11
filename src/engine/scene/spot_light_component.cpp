#include "scene/spot_light_component.h"

namespace buddd::engine {

SpotLightComponent::SpotLightComponent(math::Vec3 color, float intensity, float range,
                                       float inner_angle, float outer_angle)
    : color_(color), intensity_(intensity), range_(range),
      inner_angle_(inner_angle), outer_angle_(outer_angle) {}

auto SpotLightComponent::color() noexcept -> math::Vec3& {
    return color_;
}

auto SpotLightComponent::color() const noexcept -> const math::Vec3& {
    return color_;
}

auto SpotLightComponent::intensity() noexcept -> float& {
    return intensity_;
}

auto SpotLightComponent::intensity() const noexcept -> float {
    return intensity_;
}

auto SpotLightComponent::range() noexcept -> float& {
    return range_;
}

auto SpotLightComponent::range() const noexcept -> float {
    return range_;
}

auto SpotLightComponent::inner_angle() noexcept -> float& {
    return inner_angle_;
}

auto SpotLightComponent::inner_angle() const noexcept -> float {
    return inner_angle_;
}

auto SpotLightComponent::outer_angle() noexcept -> float& {
    return outer_angle_;
}

auto SpotLightComponent::outer_angle() const noexcept -> float {
    return outer_angle_;
}

} // namespace buddd::engine
