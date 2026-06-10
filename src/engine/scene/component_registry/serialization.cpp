#include "scene/component_registry/serialization.h"
#include "scene/component_registry/component_info.h"
#include "scene/component.h"
#include "scene/component_registry/serialization_context.h"

#include <yaml-cpp/yaml.h>

namespace buddd::engine {

auto serialize_component(const ComponentInfoBase& info, const Component& comp, const SerializationContext& ctx) -> YAML::Node {
    return info.serialize(comp, ctx);
}

auto deserialize_component(const ComponentInfoBase& info, const YAML::Node& node, Component& comp, const SerializationContext& ctx) -> Result<void> {
    try {
        return info.deserialize(comp, node, ctx);
    } catch (const YAML::Exception& e) {
        return make_error(Error::Category::InvalidArgument,
            "YAML error during deserialization of component '" + std::string(info.type_name()) + "': " + e.what());
    }
}

} // namespace buddd::engine
