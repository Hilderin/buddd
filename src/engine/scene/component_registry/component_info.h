#pragma once

#include <yaml-cpp/yaml.h>

#include "scene/component_registry/property.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component.h"
#include "error.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_set>
#include <vector>

namespace buddd::engine {

// ── Type-erased base (for storage in Registry) ──
class ComponentInfoBase {
public:
    virtual ~ComponentInfoBase() = default;

    [[nodiscard]] virtual auto type_name() const -> std::string_view = 0;
    virtual auto create() -> std::unique_ptr<Component> = 0;

    /// Serialize all properties of the component to a YAML mapping node.
    virtual auto serialize(const Component& comp, const SerializationContext& ctx) const -> YAML::Node = 0;
    /// Deserialize all properties from a YAML mapping node into the component.
    virtual auto deserialize(Component& comp, const YAML::Node& node, const SerializationContext& ctx) const -> Result<void> = 0;

    // Property metadata access (for editor inspection, tests)
    [[nodiscard]] virtual auto property_count() const -> size_t = 0;
    [[nodiscard]] virtual auto property_name(size_t index) const -> std::string_view = 0;
    [[nodiscard]] virtual auto property_type_index(size_t index) const -> const std::type_index& = 0;
    [[nodiscard]] virtual auto property_flags(size_t index) const -> const PropertyFlags& = 0;
};

// ── Typed template (for registration) ──
template<typename T>
class ComponentInfo : public ComponentInfoBase {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");

public:
    explicit ComponentInfo(std::string type_name)
        : type_name_(std::move(type_name)) {}

    // -- ComponentInfoBase overrides --
    [[nodiscard]] auto type_name() const -> std::string_view override { return type_name_; }
    auto create() -> std::unique_ptr<Component> override { return std::make_unique<T>(); }

    auto serialize(const Component& comp, const SerializationContext& ctx) const -> YAML::Node override {
        YAML::Node node;

        for (const auto& prop : properties_) {
            node[std::string(prop.name())] = prop.serialize(comp, ctx);
        }

        return node;
    }

    auto deserialize(Component& comp, const YAML::Node& node, const SerializationContext& ctx) const -> Result<void> override {
        if (!node.IsMap()) {
            return {};
        }

        // Collect property names for unknown-key detection
        std::unordered_set<std::string> known_property_names;
        for (const auto& prop : properties_) {
            known_property_names.insert(std::string(prop.name()));
        }

        // Deserialize known properties
        for (const auto& prop : properties_) {
            auto key = prop.name();
            if (!node[std::string(key)]) {
                continue;
            }

            auto result = prop.deserialize(comp, node[std::string(key)], ctx);
            if (!result) {
                return make_error(Error::Category::InvalidArgument,
                    "Failed to deserialize property '" + std::string(key) + "' of component '"
                    + type_name_ + "': " + result.error().message);
            }
        }

        // Detect unknown keys (forward-compatible: warn, not error)
        for (const auto& kv : node) {
            auto key = kv.first.as<std::string>();
            if (known_property_names.find(key) == known_property_names.end()) {
                BUDDD_LOG_TAGGED_WARN("ComponentRegistry",
                    "Unknown key '{}' skipped for component type '{}' (forward-compatible)",
                    key, type_name_);
            }
        }

        return {};
    }

    [[nodiscard]] auto property_count() const -> size_t override { return properties_.size(); }
    [[nodiscard]] auto property_name(size_t index) const -> std::string_view override { return properties_[index].name(); }
    [[nodiscard]] auto property_type_index(size_t index) const -> const std::type_index& override { return properties_[index].type_index(); }
    [[nodiscard]] auto property_flags(size_t index) const -> const PropertyFlags& override { return properties_[index].flags(); }

    /// (A) Convention-based — NOT implemented in v1.
    template<typename PropType>
    auto add_property(std::string_view /*name*/, PropertyFlags /*flags*/ = {}) -> void {
        static_assert(!sizeof(PropType*),
            "convention-based add_property not yet implemented in v1. "
            "Use overload (B) (simple lambdas) or (C) (context-aware lambdas) instead.");
    }

    /// (B) Simple lambdas — no SerializationContext needed.
    template<typename PropType>
    auto add_property(
        std::string_view name,
        std::function<PropType(const T&)> getter,
        std::function<Result<void>(T&, PropType)> setter,
        PropertyFlags flags = {}
    ) -> void {
        // Delegate to overload (C)
        add_property<PropType>(name,
            [g = std::move(getter)](const T& obj, const SerializationContext&) -> PropType {
                return g(obj);
            },
            [s = std::move(setter)](T& obj, PropType value, const SerializationContext&) -> Result<void> {
                return s(obj, std::move(value));
            },
            flags
        );
    }

    /// (C) Context-aware lambdas (core implementation).
    template<typename PropType>
    auto add_property(
        std::string_view name,
        std::function<PropType(const T&, const SerializationContext&)> getter,
        std::function<Result<void>(T&, PropType, const SerializationContext&)> setter,
        PropertyFlags flags = {}
    ) -> void {
        // Runtime check: PropType must be registered in TypeRegistry.
        const auto* type_info = TypeRegistry::get<PropType>();
        if (!type_info) {
            BUDDD_LOG_TAGGED_FATAL("ComponentRegistry",
                "Type '{}' is not registered in TypeRegistry. "
                "Call TypeRegistry::register_type<{}>() before using it in add_property<>().",
                typeid(PropType).name(), typeid(PropType).name());
            std::abort();
        }

        // Create type-erased getter
        Property::GetterFn yaml_getter = [=, g = std::move(getter)](const Component& comp, const SerializationContext& ctx) -> YAML::Node {
            const auto& typed_comp = static_cast<const T&>(comp);
            auto value = g(typed_comp, ctx);
            auto result = TypeRegistry::yaml_encode<PropType>(value, ctx);
            if (!result) {
                return YAML::Node();
            }
            return *std::move(result);
        };

        // Create type-erased setter
        Property::SetterFn yaml_setter = [=, s = std::move(setter)](Component& comp, const YAML::Node& node, const SerializationContext& ctx) -> Result<void> {
            auto decoded = TypeRegistry::yaml_decode<PropType>(node, ctx);
            if (!decoded) {
                return make_error(Error::Category::InvalidArgument,
                    "Failed to decode property '" + std::string(name) + "': " + decoded.error().message);
            }

            auto value = *std::move(decoded);

            // Apply PropertyFlags min/max constraints for float/int32_t types
            if constexpr (std::is_same_v<PropType, float>) {
                if (value < flags.min_value) {
                    return make_error(Error::Category::InvalidArgument,
                        "Property '" + std::string(name) + "' out of range: value " + std::to_string(value)
                        + " below minimum " + std::to_string(flags.min_value));
                }
                if (value > flags.max_value) {
                    return make_error(Error::Category::InvalidArgument,
                        "Property '" + std::string(name) + "' out of range: value " + std::to_string(value)
                        + " above maximum " + std::to_string(flags.max_value));
                }
            } else if constexpr (std::is_same_v<PropType, int32_t>) {
                if (static_cast<float>(value) < flags.min_value) {
                    return make_error(Error::Category::InvalidArgument,
                        "Property '" + std::string(name) + "' out of range: value " + std::to_string(value)
                        + " below minimum " + std::to_string(flags.min_value));
                }
                if (static_cast<float>(value) > flags.max_value) {
                    return make_error(Error::Category::InvalidArgument,
                        "Property '" + std::string(name) + "' out of range: value " + std::to_string(value)
                        + " above maximum " + std::to_string(flags.max_value));
                }
            }

            auto& typed_comp = static_cast<T&>(comp);
            return s(typed_comp, value, ctx);
        };

        properties_.push_back(Property{
            std::string(name),
            std::type_index(typeid(PropType)),
            std::move(yaml_getter),
            std::move(yaml_setter),
            flags
        });
    }

private:
    std::string type_name_;
    std::vector<Property> properties_;
};

} // namespace buddd::engine
