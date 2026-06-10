#include "scene/component_registry/property.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component.h"

#include <yaml-cpp/yaml.h>

namespace buddd::engine {

Property::Property(std::string name,
                   std::type_index type_index,
                   GetterFn getter,
                   SetterFn setter,
                   PropertyFlags flags)
    : name_(std::move(name))
    , type_index_(type_index)
    , flags_(std::move(flags))
    , getter_(std::move(getter))
    , setter_(std::move(setter)) {}

auto Property::name() const noexcept -> std::string_view {
    return name_;
}

auto Property::type_index() const noexcept -> const std::type_index& {
    return type_index_;
}

auto Property::flags() const noexcept -> const PropertyFlags& {
    return flags_;
}

auto Property::serialize(const Component& comp, const SerializationContext& ctx) const -> YAML::Node {
    return getter_(comp, ctx);
}

auto Property::deserialize(Component& comp, const YAML::Node& node, const SerializationContext& ctx) const -> Result<void> {
    return setter_(comp, node, ctx);
}

auto Property::to_string(const Component& /*comp*/) const -> std::string {
    // Stub for future editor UI — not yet implemented
    return {};
}

auto Property::from_string(Component& /*comp*/, const std::string& /*str*/) const -> Result<void> {
    // Stub for future editor UI — not yet implemented
    return {};
}

auto Property::validate(const Component& /*comp*/) const -> Result<void> {
    // Stub for future editor UI — not yet implemented
    return {};
}

} // namespace buddd::engine
