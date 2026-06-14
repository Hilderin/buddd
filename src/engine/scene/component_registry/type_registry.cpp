#include "scene/component_registry/type_registry.h"

namespace buddd::engine {

auto TypeRegistry::entry_map() -> std::unordered_map<std::type_index, TypeEntry>& {
    static std::unordered_map<std::type_index, TypeEntry> map;
    return map;
}

auto TypeRegistry::yaml_encode(std::type_index type, const std::any& value,
                                const SerializationContext& ctx) -> Result<YAML::Node> {
    auto& map = entry_map();
    auto it = map.find(type);
    if (it == map.end()) {
        return make_error(Error::Category::InvalidArgument,
            "yaml_encode: type '" + std::string(type.name()) + "' not registered");
    }
    if (!it->second.yaml_encode_any) {
        return make_error(Error::Category::InvalidArgument,
            "yaml_encode: type '" + std::string(type.name()) + "' has no yaml_encode_any dispatch");
    }
    return it->second.yaml_encode_any(value, ctx);
}

auto TypeRegistry::yaml_decode(std::type_index type, const YAML::Node& node,
                                const SerializationContext& ctx) -> Result<std::any> {
    auto& map = entry_map();
    auto it = map.find(type);
    if (it == map.end()) {
        return make_error(Error::Category::InvalidArgument,
            "yaml_decode: type '" + std::string(type.name()) + "' not registered");
    }
    if (!it->second.yaml_decode_any) {
        return make_error(Error::Category::InvalidArgument,
            "yaml_decode: type '" + std::string(type.name()) + "' has no yaml_decode_any dispatch");
    }
    return it->second.yaml_decode_any(node, ctx);
}

} // namespace buddd::engine
