#pragma once

#include "error.h"

// Forward declare YAML types — no yaml-cpp include in public header per ADR-016 / ADR-019.
namespace YAML {
class Node;
}

namespace buddd::engine {

class ComponentInfoBase;
class Component;
struct SerializationContext;

/// Serialize a component's properties to a YAML::Node.
/// Iterates the component's properties and delegates to Property::serialize().
[[nodiscard]] auto serialize_component(const ComponentInfoBase& info, const Component& comp, const SerializationContext& ctx) -> YAML::Node;

/// Deserialize a YAML::Node back into a component's properties.
/// Iterates the component's properties and delegates to Property::deserialize().
[[nodiscard]] auto deserialize_component(const ComponentInfoBase& info, const YAML::Node& node, Component& comp, const SerializationContext& ctx) -> Result<void>;

} // namespace buddd::engine
