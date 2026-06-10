#pragma once

#include "scene/component_registry/component_info.h"
#include "error.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::engine {

class ComponentRegistry {
public:
    ComponentRegistry() = default;

    /// Register a component type. Returns a reference to the typed ComponentInfo<T>
    /// so the caller can immediately call add_property() on it.
    /// @tparam T The concrete Component subclass.
    /// @param type_name Canonical string name (e.g. "camera").
    /// @return Reference to the ComponentInfo<T> for this type.
    /// If the type_name is already registered, logs a warning and returns the existing info.
    template<typename T>
    auto register_component(std::string_view type_name) -> ComponentInfo<T>&;

    /// Create a component instance by type name.
    [[nodiscard]] auto create(std::string_view type_name) -> Result<std::unique_ptr<Component>>;

    /// Describe a registered component type. Returns nullptr if unknown.
    [[nodiscard]] auto describe(std::string_view type_name) const noexcept -> const ComponentInfoBase*;

    /// Returns a span of all registered component info pointers.
    [[nodiscard]] auto all_types() const noexcept -> std::span<const ComponentInfoBase*>;

private:
    void invalidate_cache() const {
        all_types_cache_valid_ = false;
    }

    // Stores ComponentInfo<T> objects (polymorphic via ComponentInfoBase)
    std::vector<std::unique_ptr<ComponentInfoBase>> infos_;

    // Mutable cache for all_types(): rebuilt lazily after registrations.
    mutable std::vector<const ComponentInfoBase*> all_types_cache_;
    mutable bool all_types_cache_valid_ = false;
};

// Template implementation:
template<typename T>
auto ComponentRegistry::register_component(std::string_view type_name) -> ComponentInfo<T>& {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

    // Check for duplicate — log warning and return existing
    for (const auto& info_ptr : infos_) {
        if (info_ptr->type_name() == type_name) {
            BUDDD_LOG_TAGGED_WARN("ComponentRegistry",
                "Duplicate component type '{}' — returning existing registration", type_name);
            return static_cast<ComponentInfo<T>&>(*info_ptr);
        }
    }

    auto info = std::make_unique<ComponentInfo<T>>(std::string(type_name));
    auto& ref = *info;
    infos_.push_back(std::move(info));
    invalidate_cache();  // all_types() cache must be rebuilt
    BUDDD_LOG_TAGGED_DEBUG("ComponentRegistry", "Registered component type: {}", type_name);
    return ref;
}

} // namespace buddd::engine
