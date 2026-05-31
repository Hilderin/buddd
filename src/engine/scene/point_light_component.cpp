#include "scene/point_light_component.h"

namespace buddd::engine {

PointLightComponent::PointLightComponent(math::Vec3 colour, float intensity, float range)
    : colour_(colour), intensity_(intensity), range_(range) {}

auto PointLightComponent::colour() noexcept -> math::Vec3& {
    return colour_;
}

auto PointLightComponent::colour() const noexcept -> const math::Vec3& {
    return colour_;
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
