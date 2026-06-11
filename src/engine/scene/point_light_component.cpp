#include "scene/point_light_component.h"

namespace buddd::engine {

PointLightComponent::PointLightComponent(math::Vec3 color, float intensity, float range)
    : color_(color), intensity_(intensity), range_(range) {}

auto PointLightComponent::color() noexcept -> math::Vec3& {
    return color_;
}

auto PointLightComponent::color() const noexcept -> const math::Vec3& {
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
