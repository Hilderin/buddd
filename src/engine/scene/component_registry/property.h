#pragma once

#include "error.h"

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

// Forward declare YAML types — no yaml-cpp include in public header.
namespace YAML {
class Node;
}

namespace buddd::engine {

class Component;
struct SerializationContext;

struct PropertyFlags {
    // Numeric constraints
    float min_value = -std::numeric_limits<float>::max();
    float max_value = std::numeric_limits<float>::max();
    float step_value = 0.0f;  // 0 means no step constraint

    // Enum choices (UI display names for int32_t properties)
    std::vector<std::string> enum_choices;

    auto min(float v) noexcept -> PropertyFlags& { min_value = v; return *this; }
    auto max(float v) noexcept -> PropertyFlags& { max_value = v; return *this; }
    auto step(float v) noexcept -> PropertyFlags& { step_value = v; return *this; }
    auto choices(std::vector<std::string> c) noexcept -> PropertyFlags& { enum_choices = std::move(c); return *this; }
};

/// Functor that checks whether a property's current value equals its registered default.
/// Returns true if the property is at its default value (should be omitted during save).
using DefaultChecker = std::function<bool(const Component&, const SerializationContext&)>;

/// Internal type-erased property descriptor.
/// Stores getter/setter lambdas and TypeRegistry-backed YAML/string/validate callbacks.
/// NOT user-facing — users interact via ComponentInfo<T>::add_property<PropType>().
class Property {
public:
    using GetterFn = std::function<YAML::Node(const Component&, const SerializationContext&)>;
    using SetterFn = std::function<Result<void>(Component&, const YAML::Node&, const SerializationContext&)>;

    Property(std::string name,
             std::type_index type_index,
             GetterFn getter,
             SetterFn setter,
             PropertyFlags flags = {},
             DefaultChecker default_checker = nullptr);

    [[nodiscard]] auto name() const noexcept -> std::string_view;
    [[nodiscard]] auto type_index() const noexcept -> const std::type_index&;
    [[nodiscard]] auto flags() const noexcept -> const PropertyFlags&;

    /// Serialize this property's value from the component to a YAML node.
    [[nodiscard]] auto serialize(const Component& comp, const SerializationContext& ctx) const -> YAML::Node;

    /// Deserialize this property's value from a YAML node into the component.
    [[nodiscard]] auto deserialize(Component& comp, const YAML::Node& node, const SerializationContext& ctx) const -> Result<void>;

    /// Returns true if this property has a default checker registered.
    [[nodiscard]] auto has_default() const noexcept -> bool;

    /// Convert this property's value to a string.
    [[nodiscard]] auto to_string(const Component& comp) const -> std::string;

    /// Parse a string and set this property's value.
    [[nodiscard]] auto from_string(Component& comp, const std::string& str) const -> Result<void>;

    /// Validate this property's current value.
    [[nodiscard]] auto validate(const Component& comp) const -> Result<void>;

private:
    std::string name_;
    std::type_index type_index_;
    PropertyFlags flags_;
    GetterFn getter_;
    SetterFn setter_;
    DefaultChecker default_checker_;
};

} // namespace buddd::engine
