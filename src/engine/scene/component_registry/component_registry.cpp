#include "scene/component_registry/component_registry.h"
#include "scene/component.h"

#include <yaml-cpp/yaml.h>

BUDDD_LOG_TAG("ComponentRegistry");

namespace buddd::engine {

auto ComponentRegistry::create(std::string_view type_name) -> Result<std::unique_ptr<Component>> {
    for (const auto& info : infos_) {
        if (info->type_name() == type_name) {
            auto comp = info->create();
            if (!comp) {
                return make_error(Error::Category::InvalidArgument,
                    "Factory for component type '" + std::string(type_name) + "' returned nullptr");
            }
            return comp;
        }
    }
    return make_error(Error::Category::InvalidArgument,
        "Unknown component type: '" + std::string(type_name) + "'");
}

auto ComponentRegistry::describe(std::string_view type_name) const noexcept -> const ComponentInfoBase* {
    for (const auto& info : infos_) {
        if (info->type_name() == type_name) {
            return info.get();
        }
    }
    return nullptr;
}

auto ComponentRegistry::all_types() const noexcept -> std::span<const ComponentInfoBase*> {
    if (!all_types_cache_valid_) {
        all_types_cache_.clear();
        all_types_cache_.reserve(infos_.size());
        for (const auto& info : infos_) {
            all_types_cache_.push_back(info.get());
        }
        all_types_cache_valid_ = true;
    }
    return all_types_cache_;
}

} // namespace buddd::engine
