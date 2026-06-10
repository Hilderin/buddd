#pragma once

#include "error.h"
#include "log/log.h"

#include <yaml-cpp/yaml.h>

#include <any>
#include <functional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace buddd::engine {

struct SerializationContext;

/// Static registry mapping C++ types to YAML/string/validation callbacks.
/// Built-in types (float, int32_t, bool, std::string, Vec3, Vec4, Quat)
/// are pre-registered at startup via register_builtin_types().
/// External code registers custom types before using them in component properties.
class TypeRegistry {
public:
    TypeRegistry() = delete;  // static only

    template<typename T>
    struct TypeInfo {
        std::function<YAML::Node(const T&, const SerializationContext&)> yaml_encode;
        std::function<Result<T>(const YAML::Node&, const SerializationContext&)> yaml_decode;
        std::function<std::string(const T&, const SerializationContext&)> to_string;
        std::function<Result<T>(const std::string&, const SerializationContext&)> from_string;
        std::function<Result<void>(const T&, const SerializationContext&)> validate;
    };

    /// Register callbacks for type T. If T is already registered, logs WARNING
    /// and overwrites the existing entry.
    template<typename T>
    static auto register_type(TypeInfo<T> info) -> void;

    /// Get the TypeInfo for type T. Returns nullptr if not registered.
    template<typename T>
    static auto get() -> const TypeInfo<T>*;

    /// Convenience: encode value to YAML using the registered callbacks.
    /// Returns error if type T is not registered.
    template<typename T>
    static auto yaml_encode(const T& value, const SerializationContext& ctx) -> Result<YAML::Node>;

    /// Convenience: decode YAML node to value of type T.
    template<typename T>
    static auto yaml_decode(const YAML::Node& node, const SerializationContext& ctx) -> Result<T>;

    /// Convenience: convert value to string.
    template<typename T>
    static auto to_string(const T& value, const SerializationContext& ctx) -> Result<std::string>;

    /// Convenience: parse string to value of type T.
    template<typename T>
    static auto from_string(const std::string& str, const SerializationContext& ctx) -> Result<T>;

    /// Convenience: validate value of type T.
    template<typename T>
    static auto validate(const T& value, const SerializationContext& ctx) -> Result<void>;

    /// Check if a type T is registered.
    template<typename T>
    static auto is_registered() -> bool;

private:
    struct TypeEntry {
        std::any info;  // holds TypeInfo<T>
    };

    static auto entry_map() -> std::unordered_map<std::type_index, TypeEntry>&;
};

// ── Template implementations ──

template<typename T>
auto TypeRegistry::register_type(TypeInfo<T> info) -> void {
    auto& map = entry_map();
    auto key = std::type_index(typeid(T));
    auto [it, inserted] = map.insert({key, TypeEntry{}});
    if (!inserted) {
        // Log warning using tagged macro — but this function is in a header
        // so we use runtime check. The actual log call happens in the .cpp
        // via a helper, or we just overwrite.
        BUDDD_LOG_TAGGED_WARN("TypeRegistry",
            "Overwriting existing registration for type '{}'", typeid(T).name());
    }
    it->second.info = std::move(info);
}

template<typename T>
auto TypeRegistry::get() -> const TypeInfo<T>* {
    auto& map = entry_map();
    auto it = map.find(std::type_index(typeid(T)));
    if (it == map.end()) return nullptr;
    return &std::any_cast<TypeInfo<T>&>(it->second.info);
}

template<typename T>
auto TypeRegistry::yaml_encode(const T& value, const SerializationContext& ctx) -> Result<YAML::Node> {
    auto* info = get<T>();
    if (!info) {
        return make_error(Error::Category::InvalidArgument,
            "Type not registered: " + std::string(typeid(T).name()));
    }
    return info->yaml_encode(value, ctx);
}

template<typename T>
auto TypeRegistry::yaml_decode(const YAML::Node& node, const SerializationContext& ctx) -> Result<T> {
    auto* info = get<T>();
    if (!info) {
        return make_error(Error::Category::InvalidArgument,
            "Type not registered: " + std::string(typeid(T).name()));
    }
    return info->yaml_decode(node, ctx);
}

template<typename T>
auto TypeRegistry::to_string(const T& value, const SerializationContext& ctx) -> Result<std::string> {
    auto* info = get<T>();
    if (!info) {
        return make_error(Error::Category::InvalidArgument,
            "Type not registered: " + std::string(typeid(T).name()));
    }
    return info->to_string(value, ctx);
}

template<typename T>
auto TypeRegistry::from_string(const std::string& str, const SerializationContext& ctx) -> Result<T> {
    auto* info = get<T>();
    if (!info) {
        return make_error(Error::Category::InvalidArgument,
            "Type not registered: " + std::string(typeid(T).name()));
    }
    return info->from_string(str, ctx);
}

template<typename T>
auto TypeRegistry::validate(const T& value, const SerializationContext& ctx) -> Result<void> {
    auto* info = get<T>();
    if (!info) {
        return make_error(Error::Category::InvalidArgument,
            "Type not registered: " + std::string(typeid(T).name()));
    }
    return info->validate(value, ctx);
}

template<typename T>
auto TypeRegistry::is_registered() -> bool {
    auto& map = entry_map();
    return map.find(std::type_index(typeid(T))) != map.end();
}

} // namespace buddd::engine
