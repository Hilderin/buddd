#include "scene/component_registry/type_registry.h"

namespace buddd::engine {

auto TypeRegistry::entry_map() -> std::unordered_map<std::type_index, TypeEntry>& {
    static std::unordered_map<std::type_index, TypeEntry> map;
    return map;
}

} // namespace buddd::engine
