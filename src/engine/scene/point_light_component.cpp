#include "scene/point_light_component.h"

namespace buddd::engine {

PointLightComponent::PointLightComponent(math::Color color, float intensity, float range)
    : color_(color), intensity_(intensity), range_(range) {}

auto PointLightComponent::color() noexcept -> math::Color& {
    return color_;
}

auto PointLightComponent::color() const noexcept -> const math::Color& {
    return color_;
}

auto PointLightComponent::intensity() noexcept -> float& {
    return intensity_;
}

auto PointLightComponent::intensity() const noexcept -> float {
    return intensity_;
}

auto PointLightComponent::range() noexcept -> float& {
    return range_;
}

auto PointLightComponent::range() const noexcept -> float {
    return range_;
}

} // namespace buddd::engine
