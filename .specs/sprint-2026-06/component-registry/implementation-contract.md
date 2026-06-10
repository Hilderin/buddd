# IMPL-NNNN — Component Registration & Property System (TypeRegistry Design)

## Source spec

`.specs/sprint-2026-06/component-registry/spec.md`

## Goal

Implement a component registration and property system with a **TypeRegistry** that maps C++ types to YAML/string/validation callbacks. Built-in types (`float`, `int32_t`, `bool`, `std::string`, `math::Vec3`, `math::Vec4`, `math::Quat`, `std::shared_ptr<Model>`) are pre-registered at startup via `register_builtin_types()`. A `Property` class (internal, type-erased) stores getter/setter lambdas and delegates serialization to the TypeRegistry. The `ComponentRegistry` enables runtime discovery, factory creation by string name, and property enumeration. `ComponentInfo<T>::add_property<PropType>()` uses TypeRegistry internally — no explicit YAML lambdas on property registration. Register all five existing engine components (`CameraComponent`, `PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`, `MeshRenderer`). Provide `YAML::convert` specializations for `math::Vec3`, `math::Vec4`, and `math::Quat`.

## Non-goals

- No scene file format or World save-load (NG-01).
- No editor property panel UI (NG-02).
- No prefab system (NG-04).
- No entity-reference serialization (NG-05).
- No versioning or migration (NG-06).
- No Transform handling (NG-07 — Transform is not a Component).
- No runtime property addition/removal (NG-08).
- No nested or compound properties (NG-09).
- No `PropertyType` enum or `PropertyValue` variant in the core serialization path (NG-10 for Mat4, PropertyValue demoted to UI-layer helper only).
- No changes to `src/engine/scene/component.h`, `src/engine/scene/entity.h`, `src/engine/scene/world.h` (AC-030, SC-005).
- No new external dependencies (A-01 — yaml-cpp already available).
- No individual CameraComponent setters — use existing `set_perspective()` in property lambdas.
- No `add_asset_ref()` method — asset references use typed properties (e.g., `add_property<std::shared_ptr<Model>>()`). The TypeRegistry callbacks for `shared_ptr<Model>` handle asset ID ↔ Model conversion internally via `SerializationContext::assets`.

## Relevant ADRs

| ADR | Constraint |
|---|---|
| ADR-001 | All error-returning functions use `Result<T>` (`std::expected<T, Error>`). No exceptions for control flow. |
| ADR-005 | Optional reference API pattern — consistent with modern C++ approach. |
| ADR-006 | The project uses `dynamic_cast<T*>()` for component dispatch. Registry factories return type-erased `unique_ptr<Component>`. |
| ADR-016 | yaml-cpp is a PRIVATE dependency of `buddd_engine`. `YAML::Exception` caught and converted to `Result<void>` errors. YAML types must NOT appear in public headers. |
| ADR-019 | Architecture boundaries — engine public headers must not expose yaml-cpp types. Header files use forward declarations; yaml-cpp includes only in `.cpp` files. |

## Files to inspect

Before making changes, read these files to understand existing patterns:

| File | What to learn |
|---|---|
| `src/engine/error.h` | `Result<T>`, `Error`, `make_error()` signatures and usage. |
| `src/engine/log/log.h` | `BUDDD_LOG_TAG()`, `BUDDD_LOG_*()` macro signatures. |
| `src/engine/scene/component.h` | Confirm **this file is NOT modified** (AC-030). |
| `src/engine/scene/camera_component.h` / `.cpp` | Existing getters (`fov_y()`, `aspect()`, etc.) and `set_perspective()`. |
| `src/engine/scene/point_light_component.h` / `.cpp` | `colour()`, `intensity()`, `range()` mutable-ref accessors. |
| `src/engine/scene/directional_light_component.h` / `.cpp` | `colour()`, `intensity()` mutable-ref accessors. |
| `src/engine/scene/spot_light_component.h` / `.cpp` | `colour()`, `intensity()`, `range()`, `inner_angle()`, `outer_angle()` mutable-ref accessors. |
| `src/engine/render/mesh_renderer.h` / `.cpp` | `model()` returns `Model&` via `shared_ptr<Model>`. No setter exists. |
| `src/engine/render/model.h` | `Model` class definition (needed for `shared_ptr<Model>` member). |
| `src/engine/asset/asset_manager.h` / `.cpp` / `.tpp` | Existing `create<T>()` template, cache iteration pattern. |
| `src/engine/math/vec3.h`, `vec4.h`, `quat.h` | Math type layout. |
| `src/engine/engine_service.cpp` | Where `register_builtin_types()` + `register_all_components()` will be called. |
| `src/engine/CMakeLists.txt` | `file(GLOB_RECURSE)` picks up new files on re-configure. |
| `tests/math_tests.cpp` | Catch2 test style, `Catch::Approx`, tolerance constants. |
| `tests/asset_manager_tests.cpp` | Pattern for creating headless `EngineService`, `ProjectRootGuard`. |
| `.clang-format` | `IndentWidth: 4`, `ColumnLimit: 100`, `BasedOnStyle: LLVM`. |

## Files allowed to change

### New files to create

```
src/engine/scene/component_registry/type_registry.h
src/engine/scene/component_registry/type_registry.cpp
src/engine/scene/component_registry/property.h
src/engine/scene/component_registry/component_info.h
src/engine/scene/component_registry/component_registry.h
src/engine/scene/component_registry/component_registry.cpp
src/engine/scene/component_registry/serialization_context.h
src/engine/scene/component_registry/serialization.h
src/engine/scene/component_registry/serialization.cpp
src/engine/scene/component_registry/register_all_components.h
src/engine/scene/component_registry/register_all_components.cpp
src/engine/math/vec3_yaml.h
src/engine/math/vec4_yaml.h
src/engine/math/quat_yaml.h
tests/component_registry_tests.cpp
```

### Existing files to modify

| File | What to change |
|---|---|
| `src/engine/render/mesh_renderer.h` | Add `set_model(std::shared_ptr<Model>)` public method (if not already present). No `model_asset_id_` member or accessors. |
| `src/engine/render/mesh_renderer.cpp` | Implement `set_model()`. |
| `src/engine/asset/asset_manager.h` | Add `find_asset_id(const Model&) -> std::string` and `resolve_model(const std::string&) -> Result<std::shared_ptr<Model>>` public method declarations. |
| `src/engine/asset/asset_manager.cpp` | Implement `find_asset_id()` and `resolve_model()`. |
| `src/engine/engine_service.cpp` | Include `register_all_components.h`. After `asset_manager_` is assigned (line 46), call `register_builtin_types()` then create `ComponentRegistry` and call `register_all_components(registry)`. |

### Files removed compared to old contract

The file `property_type.h` (containing `PropertyType` enum and `PropertyValue` variant) is **NOT created** — these concepts are eliminated from the core design. `PropertyValue` exists only as a UI-layer helper (not part of this sprint).

## Files forbidden to change

- `src/engine/scene/component.h` — MUST NOT be modified (AC-030).
- `src/engine/scene/entity.h` — MUST NOT be modified.
- `src/engine/scene/world.h` — MUST NOT be modified.
- `src/engine/scene/camera_component.h` / `.cpp` — MUST NOT be modified (existing `set_perspective()` suffices for property getter/setter lambdas).
- `src/engine/scene/point_light_component.h` / `.cpp` — MUST NOT be modified.
- `src/engine/scene/directional_light_component.h` / `.cpp` — MUST NOT be modified.
- `src/engine/scene/spot_light_component.h` / `.cpp` — MUST NOT be modified.
- Any file not listed in "Files allowed to change".

## Existing conventions to follow

1. **Namespace**: All new code in `namespace buddd::engine`.
2. **Indentation**: 4 spaces, 100-column limit (`.clang-format`).
3. **Function declarations**: trailing return type style — `auto function() -> ReturnType`.
4. **Includes**: project-relative paths (e.g., `"scene/component_registry/type_registry.h"`).
5. **Logging**: `BUDDD_LOG_TAG("ComponentRegistry")` at file scope in `.cpp` files. Use `BUDDD_LOG_*` macros.
6. **Error handling**: `Result<T>` for all fallible functions. Use `make_error()` helper.
7. **Naming**: `snake_case` files, `snake_case` functions/methods, `PascalCase` classes.
8. **Header guards**: `#pragma once`.
9. **Smart pointers**: `std::unique_ptr` for ownership, `std::shared_ptr` for shared resources.
10. **RTTI**: Use `dynamic_cast` per ADR-006.
11. **yaml-cpp**: Throw-free wrappers — catch `YAML::Exception` and return `Result<void>` errors (ADR-016 pattern).
12. **Math types**: Public member `x, y, z` (Vec3) / `x, y, z, w` (Vec4, Quat) — direct member access, no getters.
13. **YAML headers**: Math type `YAML::convert` specializations are in separate `*_yaml.h` files alongside each type (NOT in the primary type header) to avoid forcing yaml-cpp dependency on all math includes.
14. **Trailing return types**: Used consistently throughout, including lambda return types where the type is not trivially deduced (e.g., `-> Result<void>` for setters).

## Required implementation behavior

### 1. TypeRegistry (static class)

File: `type_registry.h` / `type_registry.cpp`

```cpp
// type_registry.h
#pragma once

#include "error.h"

#include <functional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

// Forward declare YAML types — no yaml-cpp include in public header.
namespace YAML {
class Node;
}

namespace buddd::engine {

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
        std::function<Result<void>(const T&, const SerializationContext&)> validate;  // success if valid, error with message if invalid
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
        void* yaml_encode_fn;   // type-erased, stored as std::function<YAML::Node(const void*)>
        void* yaml_decode_fn;
        void* to_string_fn;
        void* from_string_fn;
        void* validate_fn;
    };

    static auto entry_map() -> std::unordered_map<std::type_index, TypeEntry>&;
};

} // namespace buddd::engine
```

**Implementation notes** (`type_registry.cpp`):
- The singleton `entry_map()` uses a function-local static `std::unordered_map<std::type_index, TypeEntry>`.
- Template methods are defined in a `.tpp` file included at the bottom of `type_registry.h`, or inline in the header.
- Type erasure: each `*_fn` in `TypeEntry` is a `void*` pointing to a heap-allocated `std::any` or similar. Simpler approach: store a `std::function<YAML::Node(const void*)>` and use `reinterpret_cast` internally. **Recommended approach**: store `std::function<YAML::Node(const std::any&)>` and pass `std::any(value)` at call sites. The implementer may choose the type-erasure mechanism as long as it is type-safe (no UB on misuse — misuse is a programming error caught at compile time by the template interface).
- `register_type<T>()`: stores the five callbacks type-erased into `entry_map()[std::type_index(typeid(T))]`. If an entry already exists, log WARNING and overwrite.
- `get<T>()`: looks up `entry_map()` by `std::type_index(typeid(T))`. Returns `nullptr` if not found. The checked `TypeInfo<T>*` is reconstructed from the type-erased storage.
- `yaml_encode<T>()`, `yaml_decode<T>()`, etc.: convenience wrappers that check `get<T>()` returns non-null (else return error `InvalidArgument` stating type is not registered), then call the corresponding callback with the provided `SerializationContext&`.

**Thread safety**: TypeRegistry is populated during single-threaded startup. Read queries after startup are safe but not guaranteed thread-safe during concurrent registration.

### 2. Built-in type pre-registration

File: `register_all_components.h` / `register_all_components.cpp`

```cpp
// In register_all_components.h:
/// Pre-register the eight built-in types in TypeRegistry:
/// float, int32_t, bool, std::string, math::Vec3, math::Vec4, math::Quat, std::shared_ptr<Model>.
/// Must be called once during engine startup, after AssetManager is available,
/// and before register_all_components().
void register_builtin_types();
```

**`register_builtin_types()` implementation** (in `register_all_components.cpp`):

| Type | yaml_encode | yaml_decode | to_string | from_string | validate |
|---|---|---|---|---|---|
| `float` | `YAML::Node(v)` | `node.as<float>()` (catch exception → error) | `std::to_string(v)` | `std::from_chars` → float | Always success |
| `int32_t` | `YAML::Node(v)` | `node.as<int32_t>()` | `std::to_string(v)` | `std::from_chars` → int32_t | Always success |
| `bool` | `YAML::Node(v)` | `node.as<bool>()` | `v ? "true" : "false"` | Compare to "true"/"false" | Always success |
| `std::string` | `YAML::Node(v)` | `node.as<std::string>()` | Identity | Identity | Always success |
| `math::Vec3` | Uses `YAML::convert<math::Vec3>` | Uses `YAML::convert<math::Vec3>` | `"(x, y, z)"` format | Parse from `"(x, y, z)"` | Always success |
| `math::Vec4` | Uses `YAML::convert<math::Vec4>` | Uses `YAML::convert<math::Vec4>` | `"(x, y, z, w)"` format | Parse from `"(x, y, z, w)"` | Always success |
| `math::Quat` | Uses `YAML::convert<math::Quat>` | Uses `YAML::convert<math::Quat>` | `"(x, y, z, w)"` format | Parse from `"(x, y, z, w)"` | Always success |
| `std::shared_ptr<Model>` | `ctx.assets.find_asset_id(*model)` or empty Node if null | `ctx.assets.resolve_model(id)` or null if id empty | `ctx.assets.find_asset_id(*model)` or empty string if null | `ctx.assets.resolve_model(str)` or null if str empty | Always success |

**Important**: Every callback receives `const SerializationContext&` as the last parameter. For types that don't need context (all built-ins except `shared_ptr<Model>`), the lambda simply ignores the parameter.

**Float/Int string conversion detail**: Use `std::from_chars` for parsing. On parse failure, return `InvalidArgument` error with message including the type name and the input string.

**Vec3/4/Quat string format**: `"(1.0, 2.0, 3.0)"` for Vec3, `"(1.0, 2.0, 3.0, 4.0)"` for Vec4/Quat. `from_string` parses by stripping parentheses and using `std::from_chars` for each component.

**Validate for all built-in types**: Always returns success (no validation beyond type-level). Property-level constraints (min/max) are enforced separately via `PropertyFlags`.

### 3. PropertyFlags

File: `property.h`

```cpp
#pragma once

#include "error.h"

#include <limits>
#include <string>
#include <vector>

namespace buddd::engine {

struct PropertyFlags {
    // Numeric constraints
    float min_value = -std::numeric_limits<float>::max();
    float max_value = std::numeric_limits<float>::max();
    float step = 0.0f;  // 0 means no step constraint

    // Enum choices (UI display names for int32_t properties)
    std::vector<std::string> enum_choices;

    auto min(float v) noexcept -> PropertyFlags& { min_value = v; return *this; }
    auto max(float v) noexcept -> PropertyFlags& { max_value = v; return *this; }
    auto step(float v) noexcept -> PropertyFlags& { step = v; return *this; }
    auto choices(std::vector<std::string> c) noexcept -> PropertyFlags& { enum_choices = std::move(c); return *this; }
};

} // namespace buddd::engine
```

**Min/max validation behavior** (enforced in the Property deserialization path):
- For `float` and `int32_t` property types: if the value is below `min_value` or above `max_value`, return `InvalidArgument` error with message containing `"out of range"` and the property name.
- For other types (bool, string, Vec3, Vec4, Quat), min/max constraints are silently ignored.
- `enum_choices` is a UI-layer hint only — not used in serialization/deserialization logic.

### 4. Property class (internal, type-erased)

File: `property.h`

```cpp
#pragma once

// ...include for PropertyFlags above...

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>

namespace YAML {
class Node;
}

namespace buddd::engine {

class Component;
struct SerializationContext;

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
             PropertyFlags flags = {});

    [[nodiscard]] auto name() const noexcept -> std::string_view;
    [[nodiscard]] auto type_index() const noexcept -> const std::type_index&;
    [[nodiscard]] auto flags() const noexcept -> const PropertyFlags&;

    /// Serialize this property's value from the component to a YAML node.
    [[nodiscard]] auto serialize(const Component& comp, const SerializationContext& ctx) -> YAML::Node;

    /// Deserialize this property's value from a YAML node into the component.
    [[nodiscard]] auto deserialize(Component& comp, const YAML::Node& node, const SerializationContext& ctx) -> Result<void>;

    /// Convert this property's value to a string.
    [[nodiscard]] auto to_string(const Component& comp) -> std::string;

    /// Parse a string and set this property's value.
    [[nodiscard]] auto from_string(Component& comp, const std::string& str) -> Result<void>;

    /// Validate this property's current value.
    /// NOTE: to_string, from_string, and validate are declared for future editor UI use.
    /// They are NOT exercised by any production code or tests in this sprint.
    /// They will become functional when the editor property panel is implemented.
    [[nodiscard]] auto validate(const Component& comp) -> Result<void>;

private:
    std::string name_;
    std::type_index type_index_;
    PropertyFlags flags_;
    GetterFn getter_;
    SetterFn setter_;
};

} // namespace buddd::engine
```

The `Property::serialize()` method calls the stored `getter_` and returns the `YAML::Node` it produces. The `Property::deserialize()` method calls the stored `setter_` with the decoded `YAML::Node`. Both the getter and setter lambdas internally call TypeRegistry — the Property itself is agnostic to the type.

**Min/max constraint enforcement** is embedded inside the `setter_` lambda that `ComponentInfo<T>::add_property<PropType>()` constructs. Specifically:
1. TypeRegistry's `yaml_decode<PropType>()` decodes the YAML to a `PropType` value.
2. If `PropType` is `float` or `int32_t`, check min/max constraints from `PropertyFlags`.
3. Call the user-provided setter.
4. Return result.

### 5. ComponentInfoBase and ComponentInfo\<T\>

File: `component_info.h`

```cpp
#pragma once

#include "scene/component_registry/property.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/component.h"
#include "error.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

// Forward declare YAML types — no yaml-cpp include in public header per ADR-016 / ADR-019.
namespace YAML {
class Node;
}

namespace buddd::engine {

// Forward declarations
class Component;
struct SerializationContext;

// ── Type-erased base (for storage in Registry) ──
class ComponentInfoBase {
public:
    virtual ~ComponentInfoBase() = default;

    [[nodiscard]] virtual auto type_name() const -> std::string_view = 0;
    virtual auto create() -> std::unique_ptr<Component> = 0;

    /// Serialize all properties of the component to a YAML mapping node.
    virtual auto serialize(const Component& comp, const SerializationContext& ctx) -> YAML::Node = 0;
    /// Deserialize all properties from a YAML mapping node into the component.
    virtual auto deserialize(Component& comp, const YAML::Node& node, const SerializationContext& ctx) -> Result<void> = 0;

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
    explicit ComponentInfo(std::string type_name);

    // -- ComponentInfoBase overrides --
    [[nodiscard]] auto type_name() const -> std::string_view override;
    auto create() -> std::unique_ptr<Component> override;
    auto serialize(const Component& comp, const SerializationContext& ctx) -> YAML::Node override;
    auto deserialize(Component& comp, const YAML::Node& node, const SerializationContext& ctx) -> Result<void> override;
    [[nodiscard]] auto property_count() const -> size_t override;
    [[nodiscard]] auto property_name(size_t index) const -> std::string_view override;
    [[nodiscard]] auto property_type_index(size_t index) const -> const std::type_index& override;
    [[nodiscard]] auto property_flags(size_t index) const -> const PropertyFlags& override;

    /// (A) Convention-based — auto-detects get_<name>() / set_<name>(value).
    ///     NOT implemented in v1 — produces compile-time error with guidance message.
    template<typename PropType>
    auto add_property(std::string_view name, PropertyFlags flags = {}) -> void;

    /// (B) Simple lambdas — no SerializationContext needed.
    ///     Wraps the simple lambdas to ignore the context parameter.
    template<typename PropType>
    auto add_property(
        std::string_view name,
        std::function<PropType(const T&)> getter,
        std::function<Result<void>(T&, PropType)> setter,
        PropertyFlags flags = {}
    ) -> void;

    /// (C) Context-aware lambdas — receives SerializationContext for
    ///     context-dependent operations (e.g. asset resolution).
    template<typename PropType>
    auto add_property(
        std::string_view name,
        std::function<PropType(const T&, const SerializationContext&)> getter,
        std::function<Result<void>(T&, PropType, const SerializationContext&)> setter,
        PropertyFlags flags = {}
    ) -> void;

private:
    std::string type_name_;
    std::vector<Property> properties_;
};

} // namespace buddd::engine

// Template implementations at the bottom of the header (or in a .tpp):
#include "scene/component_registry/type_registry.h"

namespace buddd::engine {

template<typename T>
ComponentInfo<T>::ComponentInfo(std::string type_name)
    : type_name_(std::move(type_name)) {}

template<typename T>
auto ComponentInfo<T>::type_name() const -> std::string_view { return type_name_; }

template<typename T>
auto ComponentInfo<T>::create() -> std::unique_ptr<Component> {
    return std::make_unique<T>();
}

template<typename T>
auto ComponentInfo<T>::property_count() const -> size_t { return properties_.size(); }

template<typename T>
auto ComponentInfo<T>::property_name(size_t index) const -> std::string_view {
    return properties_[index].name();
}

template<typename T>
auto ComponentInfo<T>::property_type_index(size_t index) const -> const std::type_index& {
    return properties_[index].type_index();
}

template<typename T>
auto ComponentInfo<T>::property_flags(size_t index) const -> const PropertyFlags& {
    return properties_[index].flags();
}

// ── Overload (A): Convention-based (v1 compile-error stub) ──
template<typename T>
template<typename PropType>
auto ComponentInfo<T>::add_property(
    std::string_view name,
    PropertyFlags flags
) -> void
{
    // This overload is NOT implemented in v1. The static_assert below
    // produces a clear compile-time error if someone tries to use it.
    static_assert(!sizeof(PropType*),
        "convention-based add_property not yet implemented in v1. "
        "Use overload (B) (simple lambdas) or (C) (context-aware lambdas) instead.");
}

// ── Overload (B): Simple lambdas (wraps to add context) ──
template<typename T>
template<typename PropType>
auto ComponentInfo<T>::add_property(
    std::string_view name,
    std::function<PropType(const T&)> getter,
    std::function<Result<void>(T&, PropType)> setter,
    PropertyFlags flags
) -> void
{
    // Delegate to overload (C) by wrapping simple lambdas in context-ignoring wrappers
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

// ── Overload (C): Context-aware lambdas (core implementation) ──
template<typename T>
template<typename PropType>
auto ComponentInfo<T>::add_property(
    std::string_view name,
    std::function<PropType(const T&, const SerializationContext&)> getter,
    std::function<Result<void>(T&, PropType, const SerializationContext&)> setter,
    PropertyFlags flags
) -> void
{
    // Runtime check: PropType must be registered in TypeRegistry.
    // This catches programmer errors (type not registered before use) at startup.
    const auto* type_info = TypeRegistry::get<PropType>();
    if (!type_info) {
        BUDDD_LOG_TAGGED_FATAL("ComponentRegistry",
            "Type '{}' is not registered in TypeRegistry. "
            "Call TypeRegistry::register_type<{}>() before using it in add_property<>().",
            typeid(PropType).name(), typeid(PropType).name());
        std::abort();
    }

    // Create type-erased getter: calls user's typed getter, then TypeRegistry::yaml_encode
    Property::GetterFn yaml_getter = [=, g = std::move(getter)](const Component& comp, const SerializationContext& ctx) -> YAML::Node {
        const auto& typed_comp = static_cast<const T&>(comp);
        auto value = g(typed_comp, ctx);
        auto result = TypeRegistry::yaml_encode<PropType>(value, ctx);
        if (!result) {
            // Should not happen for registered types, but handle gracefully
            return YAML::Node();  // Return empty node on error
        }
        return *std::move(result);
    };

    // Create type-erased setter: calls TypeRegistry::yaml_decode, then checks min/max, then calls user's setter
    Property::SetterFn yaml_setter = [=, s = std::move(setter)](Component& comp, const YAML::Node& node, const SerializationContext& ctx) -> Result<void> {
        // Decode via TypeRegistry
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

        // Call the user-provided typed setter
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

template<typename T>
auto ComponentInfo<T>::serialize(const Component& comp, const SerializationContext& ctx) -> YAML::Node {
    const auto& typed_comp = static_cast<const T&>(comp);
    YAML::Node node;

    for (const auto& prop : properties_) {
        node[prop.name().data()] = prop.serialize(comp, ctx);
    }

    return node;
}

template<typename T>
auto ComponentInfo<T>::deserialize(Component& comp, const YAML::Node& node, const SerializationContext& ctx) -> Result<void> {
    if (!node.IsMap()) {
        // Empty or non-map node — no properties to update, success.
        return {};
    }

    auto& typed_comp = static_cast<T&>(comp);

    // Collect property names for unknown-key detection
    std::unordered_set<std::string> known_property_names;
    for (const auto& prop : properties_) {
        known_property_names.insert(std::string(prop.name()));
    }

    // Deserialize known properties
    for (const auto& prop : properties_) {
        auto key = prop.name();
        if (!node[key.data()]) {
            continue;  // Key not present — skip (leave default value)
        }

        auto result = prop.deserialize(comp, node[key.data()], ctx);
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

} // namespace buddd::engine
```

**`add_property<PropType>()` behavior summary**:
- **Overload (A)**: Convention-based — auto-detects `get_<name>()` / `set_<name>(value)` accessors at compile time. NOT implemented in v1. Produces a compile error: `"convention-based add_property not yet implemented in v1. Use overload (B) or (C) instead."`
- **Overload (B)**: Simple lambdas — wraps the user's `getter(T&) -> PropType` and `setter(T&, PropType) -> Result<void>` into context-accepting wrappers, then delegates to overload (C).
- **Overload (C)**: Context-aware lambdas (core implementation):
  1. Looks up `TypeRegistry::get<PropType>()` — if not registered, logs FATAL and aborts (programmer error: must register type before use).
  2. Constructs a `Property::GetterFn` that: casts `Component&` to `T&`, calls the user's typed getter to get `PropType`, then calls `TypeRegistry::yaml_encode<PropType>(value, ctx)` to get `YAML::Node`.
  3. Constructs a `Property::SetterFn` that: calls `TypeRegistry::yaml_decode<PropType>(node, ctx)`, checks min/max constraints (for float/int32_t), then calls the user's typed setter.
  4. Stores the `Property` in the internal `properties_` vector.
  5. The `type_index` stored in the Property is `std::type_index(typeid(PropType))`.

### 6. ComponentRegistry

File: `component_registry.h` / `component_registry.cpp`

```cpp
// component_registry.h
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
    /// Thread-safe for concurrent reads after registration (lazy caching via mutable member).
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
    auto* existing = describe(type_name);
    if (existing != nullptr) {
        BUDDD_LOG_TAGGED_WARN("ComponentRegistry",
            "Duplicate component type '{}' — returning existing registration", type_name);
        return static_cast<ComponentInfo<T>&>(*existing);
    }

    auto info = std::make_unique<ComponentInfo<T>>(std::string(type_name));
    auto& ref = *info;
    infos_.push_back(std::move(info));
    invalidate_cache();  // all_types() cache must be rebuilt
    BUDDD_LOG_TAGGED_DEBUG("ComponentRegistry", "Registered component type: {}", type_name);
    return ref;
}

} // namespace buddd::engine
```

**`component_registry.cpp` implementation**:

```cpp
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
```

**Thread safety**: Registration is single-threaded (startup). Queries (`create`, `describe`, `all_types`) may be called from any thread after registration completes. `all_types()` uses a `mutable` lazy cache that is thread-safe for concurrent reads because the cache is only rebuilt during registration (`register_component` calls `invalidate_cache()`) and subsequent `const` calls to `all_types()` only read the cache without modifying registration data.

### 7. SerializationContext

File: `serialization_context.h`

```cpp
#pragma once

namespace buddd::engine {

class AssetManager;

struct SerializationContext {
    AssetManager& assets;
};

} // namespace buddd::engine
```

### 8. YAML serialization / deserialization free functions

File: `serialization.h` / `serialization.cpp`

```cpp
// serialization.h
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
```

**Implementation** (in `serialization.cpp`):
```cpp
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
```

### 9. YAML::convert specializations for math types

File: `vec3_yaml.h`, `vec4_yaml.h`, `quat_yaml.h`

```cpp
// vec3_yaml.h
#pragma once

#include "math/vec3.h"
#include <yaml-cpp/yaml.h>

namespace YAML {

template<>
struct convert<buddd::engine::math::Vec3> {
    static auto encode(const buddd::engine::math::Vec3& v) -> Node {
        Node node;
        node["x"] = v.x;
        node["y"] = v.y;
        node["z"] = v.z;
        return node;
    }

    static auto decode(const Node& node, buddd::engine::math::Vec3& v) -> bool {
        if (!node.IsMap() || !node["x"] || !node["y"] || !node["z"]) {
            return false;
        }
        try {
            v.x = node["x"].as<float>();
            v.y = node["y"].as<float>();
            v.z = node["z"].as<float>();
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace YAML
```

Similarly for `Vec4` (x, y, z, w) and `Quat` (x, y, z, w). `decode` returns `false` if required keys are missing or `as<float>()` throws. The caller (TypeRegistry's built-in type registration) checks the return value and produces an appropriate error.

### 10. Registration entry point

File: `register_all_components.h` / `register_all_components.cpp`

```cpp
// register_all_components.h
#pragma once

namespace buddd::engine {

class ComponentRegistry;

/// Pre-register the seven built-in types in TypeRegistry.
/// Must be called once during engine startup, before register_all_components().
void register_builtin_types();

/// Register all engine component types. Called once during engine startup,
/// after register_builtin_types().
/// Calling twice is safe — duplicates produce a logged WARNING.
void register_all_components(ComponentRegistry& registry);

} // namespace buddd::engine
```

**Registration details per component** (using spec examples exactly):

| Component | Type name | Properties |
|---|---|---|
| `CameraComponent` | `"camera"` | `fov_y` (float, min=0.001, max≈π), `aspect` (float), `near` (float), `far` (float) |
| `PointLightComponent` | `"point_light"` | `colour` (Vec3), `intensity` (float, min=0.0), `range` (float, min=0.0) |
| `DirectionalLightComponent` | `"directional_light"` | `colour` (Vec3), `intensity` (float, min=0.0) |
| `SpotLightComponent` | `"spot_light"` | `colour` (Vec3), `intensity` (float, min=0.0), `range` (float, min=0.0), `inner_angle` (float, min=0.0), `outer_angle` (float, min=0.0) |
| `MeshRenderer` | `"mesh_renderer"` | `model` (std::shared_ptr<Model>) |

**Complete `register_all_components()` implementation** (mirrors spec examples):

```cpp
// register_all_components.cpp
#include "scene/component_registry/register_all_components.h"
#include "scene/component_registry/type_registry.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/property.h"
#include "scene/camera_component.h"
#include "scene/point_light_component.h"
#include "scene/directional_light_component.h"
#include "scene/spot_light_component.h"
#include "render/mesh_renderer.h"
#include "render/model.h"

#include <yaml-cpp/yaml.h>  // Required for YAML::Node() construction in template instantiations

namespace buddd::engine {

void register_builtin_types() {
    // Register float
    // NOTE: Uses C++20 designated initializers (.yaml_encode = ...).
    // If the project targets C++17, replace with positional arguments or a constructor.
    TypeRegistry::register_type<float>({
        .yaml_encode = [](const float& v, const SerializationContext&) -> YAML::Node { return YAML::Node(v); },
        .yaml_decode = [](const YAML::Node& n, const SerializationContext&) -> Result<float> {
            try { return n.as<float>(); }
            catch (const YAML::Exception& e) {
                return make_error(Error::Category::InvalidArgument,
                    "float: expected scalar, got " + std::string(e.what()));
            }
        },
        .to_string = [](const float& v, const SerializationContext&) -> std::string { return std::to_string(v); },
        .from_string = [](const std::string& s, const SerializationContext&) -> Result<float> {
            float v;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
            if (ec != std::errc()) {
                return make_error(Error::Category::InvalidArgument,
                    "float: cannot parse '" + s + "'");
            }
            return v;
        },
        .validate = [](const float&, const SerializationContext&) -> Result<void> { return {}; }
    });

    // Register int32_t, bool, std::string, Vec3, Vec4, Quat similarly.
    // Vec3 uses YAML::convert<math::Vec3> for yaml_encode/yaml_decode.
    // String format for Vec3: "(x, y, z)" — parse using from_chars.
    // All built-in type validate callbacks return success unconditionally.
    // Each callback receives const SerializationContext& as the last parameter
    // (ignored by all built-in types except shared_ptr<Model>).
    //
    // Detailed registration for each remaining type follows the same pattern as float above.
    // The implementer MUST implement all eight built-in types.
    // TypeRegistry::register_type<T>() is idempotent for overwrite (logs WARNING).

    // Register shared_ptr<Model> — uses ctx.assets for asset ID ↔ Model conversion.
    TypeRegistry::register_type<std::shared_ptr<Model>>({
        .yaml_encode = [](const std::shared_ptr<Model>& model, const SerializationContext& ctx) -> YAML::Node {
            if (!model) return YAML::Node("");
            return YAML::Node(ctx.assets.find_asset_id(*model));
        },
        .yaml_decode = [](const YAML::Node& node, const SerializationContext& ctx) -> Result<std::shared_ptr<Model>> {
            auto id = node.as<std::string>();
            if (id.empty()) return std::shared_ptr<Model>(nullptr);
            return ctx.assets.resolve_model(id);
        },
        .to_string = [](const std::shared_ptr<Model>& model, const SerializationContext& ctx) -> std::string {
            if (!model) return "";
            return ctx.assets.find_asset_id(*model);
        },
        .from_string = [](const std::string& str, const SerializationContext& ctx) -> Result<std::shared_ptr<Model>> {
            if (str.empty()) return std::shared_ptr<Model>(nullptr);
            return ctx.assets.resolve_model(str);
        },
        .validate = [](const std::shared_ptr<Model>&, const SerializationContext&) -> Result<void> {
            return {};  // always valid
        }
    });
}

void register_all_components(ComponentRegistry& registry) {
    // ── CameraComponent: uses overload (B) — no SerializationContext needed ──
    {
        auto& info = registry.register_component<CameraComponent>("camera");

        info.add_property<float>("fov_y",
            [](const CameraComponent& c) { return c.fov_y(); },
            [](CameraComponent& c, float v) -> Result<void> {
                c.set_perspective(v, c.aspect(), c.near_plane(), c.far_plane());
                return {};
            },
            PropertyFlags{}.min(0.001f).max(3.14159f)
        );

        info.add_property<float>("aspect",
            [](const CameraComponent& c) { return c.aspect(); },
            [](CameraComponent& c, float v) -> Result<void> {
                c.set_perspective(c.fov_y(), v, c.near_plane(), c.far_plane());
                return {};
            }
        );

        info.add_property<float>("near",
            [](const CameraComponent& c) { return c.near_plane(); },
            [](CameraComponent& c, float v) -> Result<void> {
                c.set_perspective(c.fov_y(), c.aspect(), v, c.far_plane());
                return {};
            }
        );

        info.add_property<float>("far",
            [](const CameraComponent& c) { return c.far_plane(); },
            [](CameraComponent& c, float v) -> Result<void> {
                c.set_perspective(c.fov_y(), c.aspect(), c.near_plane(), v);
                return {};
            }
        );
    }

    // ── PointLightComponent: uses overload (B) — no SerializationContext needed ──
    {
        auto& info = registry.register_component<PointLightComponent>("point_light");

        info.add_property<math::Vec3>("colour",
            [](const PointLightComponent& c) -> math::Vec3 { return c.colour(); },
            [](PointLightComponent& c, const math::Vec3& v) -> Result<void> { c.colour() = v; return {}; }
        );

        info.add_property<float>("intensity",
            [](const PointLightComponent& c) { return c.intensity(); },
            [](PointLightComponent& c, float v) -> Result<void> { c.intensity() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("range",
            [](const PointLightComponent& c) { return c.range(); },
            [](PointLightComponent& c, float v) -> Result<void> { c.range() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );
    }

    // ── DirectionalLightComponent: uses overload (B) ──
    {
        auto& info = registry.register_component<DirectionalLightComponent>("directional_light");

        info.add_property<math::Vec3>("colour",
            [](const DirectionalLightComponent& c) -> math::Vec3 { return c.colour(); },
            [](DirectionalLightComponent& c, const math::Vec3& v) -> Result<void> { c.colour() = v; return {}; }
        );

        info.add_property<float>("intensity",
            [](const DirectionalLightComponent& c) { return c.intensity(); },
            [](DirectionalLightComponent& c, float v) -> Result<void> { c.intensity() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );
    }

    // ── SpotLightComponent: uses overload (B) ──
    {
        auto& info = registry.register_component<SpotLightComponent>("spot_light");

        info.add_property<math::Vec3>("colour",
            [](const SpotLightComponent& c) -> math::Vec3 { return c.colour(); },
            [](SpotLightComponent& c, const math::Vec3& v) -> Result<void> { c.colour() = v; return {}; }
        );

        info.add_property<float>("intensity",
            [](const SpotLightComponent& c) { return c.intensity(); },
            [](SpotLightComponent& c, float v) -> Result<void> { c.intensity() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("range",
            [](const SpotLightComponent& c) { return c.range(); },
            [](SpotLightComponent& c, float v) -> Result<void> { c.range() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("inner_angle",
            [](const SpotLightComponent& c) { return c.inner_angle(); },
            [](SpotLightComponent& c, float v) -> Result<void> { c.inner_angle() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );

        info.add_property<float>("outer_angle",
            [](const SpotLightComponent& c) { return c.outer_angle(); },
            [](SpotLightComponent& c, float v) -> Result<void> { c.outer_angle() = v; return {}; },
            PropertyFlags{}.min(0.0f)
        );
    }

    // ── MeshRenderer: uses overload (C) — context needed for shared_ptr<Model> resolution ──
    {
        auto& info = registry.register_component<MeshRenderer>("mesh_renderer");

        info.add_property<std::shared_ptr<Model>>("model",
            [](const MeshRenderer& c, const SerializationContext&) {
                return c.model();  // returns shared_ptr<Model>
            },
            [](MeshRenderer& c, std::shared_ptr<Model> model, const SerializationContext&) -> Result<void> {
                c.set_model(std::move(model));
                return {};
            }
        );
    }
}

} // namespace buddd::engine
```

**Important registration conventions**:
- CameraComponent uses overload (B) (simple lambdas) — getters/setters don't need `SerializationContext`.
- Light components use overload (B) — getter returns via const accessor, setter assigns through mutable-ref accessor.
- MeshRenderer uses overload (C) (context-aware lambdas) — the `shared_ptr<Model>` getter/setter operate on the shared pointer directly; TypeRegistry handles asset ID ↔ Model conversion internally via `ctx.assets`.
- PropertyFlags constraints use `min(0.0f)` (not `min(0.001f)`) for light component properties, consistent with the updated spec (where `intensity`, `range`, `inner_angle`, `outer_angle` have `min>=0`).

### 11. MeshRenderer changes

Add to `mesh_renderer.h`:
```cpp
auto set_model(std::shared_ptr<Model> model) -> void;
```

If `set_model()` is already present (prototyping), verify the signature matches `void(std::shared_ptr<Model>)`. No `model_asset_id_` member, no `model_asset_id()` accessor, no `set_model_asset_id()` — the `shared_ptr<Model>` is stored directly as the existing `model_` member.

Add to `mesh_renderer.cpp`:
```cpp
auto MeshRenderer::set_model(std::shared_ptr<Model> model) -> void {
    model_ = std::move(model);
}
```

### 12. AssetManager additions

The TypeRegistry's `shared_ptr<Model>` callbacks need two new AssetManager methods for asset ID ↔ Model resolution.

Declare in `asset_manager.h`:
```cpp
/// Reverse lookup: find the asset ID string for a given Model.
/// Returns empty string if the model is not owned by any registered ModelAsset.
[[nodiscard]] auto find_asset_id(const Model& model) const -> std::string;

/// Resolve an asset ID string to a shared_ptr<Model>.
/// Returns error if the ID is not registered or the asset is not a ModelAsset.
[[nodiscard]] auto resolve_model(const std::string& id) -> Result<std::shared_ptr<Model>>;
```

Implement in `asset_manager.cpp`:
```cpp
auto AssetManager::find_asset_id(const Model& model) const -> std::string {
    for (const auto& [id, asset] : cache_) {
        auto* model_asset = dynamic_cast<ModelAsset*>(asset.get());
        if (!model_asset) continue;
        if (!model_asset->root_node().model.has_value()) continue;
        if (&model_asset->root_node().model.value() == &model) {
            return id;
        }
    }
    return {};
}

auto AssetManager::resolve_model(const std::string& id) -> Result<std::shared_ptr<Model>> {
    auto asset_result = create<ModelAsset>(id);
    if (!asset_result) {
        return make_error(Error::Category::InvalidArgument,
            "Failed to resolve model asset '" + id + "': " + asset_result.error().message);
    }
    auto* model_asset = asset_result->get();
    auto& root = model_asset->root_node();
    if (!root.model.has_value()) {
        return make_error(Error::Category::InvalidArgument,
            "Model asset '" + id + "' has no root model");
    }
    auto model = std::make_shared<Model>(std::move(*root.model));
    root.model.reset();
    return model;
}
```

**Note**: `resolve_model()` destructively moves the Model out of the asset cache (ModelAsset retains ownership but the model value is moved out). This is acceptable for v1; a future version may use a non-destructive approach (clone or shared model ownership).

### 13. EngineService startup hook

In `engine_service.cpp`, after line 46 (`engine->asset_manager_ = std::move(*asset_mgr);`), add:

```cpp
#include "scene/component_registry/register_all_components.h"
// ...
// Register built-in types in TypeRegistry
register_builtin_types();

// Register all engine components
auto registry = ComponentRegistry();
register_all_components(registry);
```

The registry is a local variable. If future features need query access after startup, it can be promoted to an `EngineService` member — deferred from this scope.

### 14. CMakeLists changes

No explicit changes needed — `file(GLOB_RECURSE ENGINE_SOURCES ...)` in `src/engine/CMakeLists.txt` automatically picks up new `.h`/`.cpp` files in `src/engine/` subdirectories. The `tests/CMakeLists.txt` also uses `file(GLOB_RECURSE BUDDD_TEST_SOURCES ...)` for `*_tests.cpp` files. Ensure CMake is re-configured after creating new files.

## Required tests

All tests are in `tests/component_registry_tests.cpp`. They do NOT require a display or EngineService — the `ComponentRegistry` is testable in isolation.

### Test categories

#### Registration and querying (AC-001, AC-002, AC-003, AC-004, AC-005, AC-031)

1. `REGISTER_COMPONENT_AND_QUERY` — Create a minimal test component `TestComponent : Component`. Register it with name `"test"`. Verify `describe("test")` returns non-null. Verify `create("test")` succeeds and `dynamic_cast<TestComponent*>(result->get())` is non-null.
2. `REGISTER_DUPLICATE_WARNS_AND_RETURNS_SAME` — Register `"test"` twice via `register_component<TestComponent>("test")`. Verify second call returns a reference (does not throw/crash). Verify `&info1 == &info2` (same underlying `ComponentInfo<T>`).
3. `CREATE_UNKNOWN_TYPE_RETURNS_ERROR` — Call `create("nonexistent")`. Verify error with `InvalidArgument` and message containing `"nonexistent"`.
4. `DESCRIBE_UNKNOWN_TYPE_RETURNS_NULLPTR` — Call `describe("nonexistent")`. Verify nullptr.
5. `ALL_TYPES_COUNT` — Register 3 test components. Verify `all_types().size() == 3`.

#### TypeRegistry operations (AC-032, AC-033, AC-034, AC-035, AC-036, AC-038, AC-039)

6. `TYPE_REGISTRY_REGISTER_AND_ENCODE` — Register a test type `TestType` (e.g., a simple struct with int and float) with five callbacks. Verify `yaml_encode` produces expected YAML. Verify `yaml_decode` round-trips.
7. `TYPE_REGISTRY_STRING_ROUNDTRIP` — Register test type, verify `to_string` and `from_string` round-trip.
8. `TYPE_REGISTRY_VALIDATE` — Register test type with custom validate that rejects negative values. Validate a positive value (success) and a negative value (error).
9. `TYPE_REGISTRY_OVERWRITE_WARNS` — Register a type, then register again with different callbacks. Verify second call logs WARNING and new callbacks are active.
10. `TYPE_REGISTRY_BUILTIN_FLOAT` — Without explicit user registration, verify `TypeRegistry::yaml_encode<float>(3.14f)` returns valid YAML. Verify `TypeRegistry::to_string<float>(3.14f)` returns a string. (AC-036)
11. `TYPE_REGISTRY_BUILTIN_VEC3` — Verify `TypeRegistry::yaml_encode<math::Vec3>({1,2,3})` returns a YAML mapping with `x`, `y`, `z`.
12. `TYPE_REGISTRY_BUILTIN_SHARED_PTR_MODEL` — Verify `TypeRegistry::yaml_encode<std::shared_ptr<Model>>(nullptr, ctx)` returns a YAML node containing empty string. Verify `TypeRegistry::yaml_decode<std::shared_ptr<Model>>(YAML::Node(""), ctx)` returns a null `shared_ptr<Model>`. (AC-036)
13. `TYPE_REGISTRY_UNREGISTERED_RUNTIME_ERROR` — Attempt to call `TypeRegistry::yaml_encode<UnregisteredType>(...)` with a context and verify the call returns an error `Result` with `InvalidArgument`. The add_property implementation uses a runtime check (logs FATAL + aborts) — unit tests verify the `yaml_encode`/`yaml_decode` convenience methods return error for unregistered types.

#### Property metadata (AC-006, AC-010, AC-011, AC-012, AC-013, AC-014)

13. `PROPERTY_METADATA` — Create a `ComponentInfo<TestComponent>` (via `register_component<TestComponent>("test")`), add a float property. Verify `property_name(0)`, `property_type_index(0)` return correct values.
14. `CAMERA_COMPONENT_PROPERTIES` — After `register_all_components()`, verify `describe("camera")` returns non-null `ComponentInfoBase*`. Verify `property_count() == 4`. Verify names match `fov_y`, `aspect`, `near`, `far`. Verify `property_type_index` matches `typeid(float)` for all 4.
15. `POINT_LIGHT_COMPONENT_PROPERTIES` — Verify `describe("point_light")` has `property_count() == 3`, properties: `colour` (Vec3), `intensity` (float, min constraint), `range` (float, min constraint).
16. `DIRECTIONAL_LIGHT_COMPONENT_PROPERTIES` — Verify `property_count() == 2`, properties: `colour` (Vec3), `intensity` (float, min constraint).
17. `SPOT_LIGHT_COMPONENT_PROPERTIES` — Verify `property_count() == 5`, properties: `colour` (Vec3), `intensity` (float, min constraint), `range` (float, min constraint), `inner_angle` (float, min constraint), `outer_angle` (float, min constraint).
18. `MESH_RENDERER_COMPONENT_PROPERTIES` — Verify `property_count() == 1`, property: `model` (std::shared_ptr<Model>).

#### Property get/set (AC-007, AC-008, AC-009)

19. `PROPERTY_GET_SET` — Register test component with a float property. Create an instance, modify a field directly, then verify via serialization/deserialization that the getter returns the correct value and the setter updates it.
20. `PROPERTY_VALIDATION_MIN` — Register test component with float property having min constraint. Set value below min via deserialize. Verify error.
21. `PROPERTY_VALIDATION_MAX` — Same but above max.

#### YAML round-trip (AC-016, AC-017, AC-018, AC-019, AC-020, AC-021, AC-022, AC-029)

22. `SERIALIZE_CAMERA_COMPONENT` — Serialize a CameraComponent with known values. Verify YAML keys match property names and values match.
23. `DESERIALIZE_CAMERA_COMPONENT` — Create YAML node, deserialize into default CameraComponent. Verify values updated.
24. `ROUND_TRIP_CAMERA_COMPONENT` — Serialize default CameraComponent, deserialize into new instance. Verify all properties match (AC-018).
25. `ROUND_TRIP_POINT_LIGHT` — Same pattern for PointLightComponent (AC-019).
26. `ROUND_TRIP_DIRECTIONAL_LIGHT` — Same for DirectionalLightComponent (AC-020).
27. `ROUND_TRIP_SPOT_LIGHT` — Same for SpotLightComponent (AC-021).
28. `ROUND_TRIP_MESH_RENDERER` — Same for MeshRenderer (model field round-trips via `shared_ptr<Model>` TypeRegistry callbacks) (AC-022). The TypeRegistry encodes the model to an asset ID string (via `ctx.assets.find_asset_id()`) and decodes back (via `ctx.assets.resolve_model()`). Test verifies round-trip through serialize → deserialize → getter. Uses a mock or real AssetManager that provides `find_asset_id` and `resolve_model` stubs (see below).
29. All round-trip tests use in-memory YAML::Node, no file I/O (AC-029).

#### YAML::convert for math types (AC-023, AC-024, AC-025)

30. `YAML_CONVERT_VEC3` — Convert `Vec3(1,2,3)` to YAML. Verify `node["x"].as<float>() == 1.0` etc. Convert back. Verify round-trip.
31. `YAML_CONVERT_VEC4` — Same pattern.
32. `YAML_CONVERT_QUAT` — Same pattern.

#### Error cases (AC-026, AC-027, AC-028)

33. `DESERIALIZE_UNKNOWN_KEY_WARNING` — Deserialize a YAML node with an unknown key `"invalid_prop"` into a CameraComponent. Verify success result. Verify warning logged (use log spy or MemorySink capture if available; otherwise verify the function does not return error).
34. `DESERIALIZE_TYPE_MISMATCH` — Deserialize YAML with string value `"abc"` where float property expected. Verify error with `InvalidArgument`.
35. `DESERIALIZE_OUT_OF_RANGE` — Deserialize YAML with `fov_y: 0.0` where min is `0.001`. Verify error with `InvalidArgument` and message containing `"out of range"` and property name.

#### Factory creation for real components (AC-002)

36. `FACTORY_CREATES_CORRECT_TYPE` — After `register_all_components()`, verify `create("camera")` returns a component that is `dynamic_cast<CameraComponent*>` valid. Repeat for `point_light`, `directional_light`, `spot_light`, `mesh_renderer`.

### Mock / helper setup

For tests that need a `SerializationContext`:

```cpp
// A proper mock AssetManager for testing.
// Inheriting from AssetManager avoids undefined behavior from reinterpret_cast.
class MockAssetManager : public AssetManager {
public:
    // Constructor: pass a dummy RenderDevice and base path.
    // The test creates a minimal RenderDevice (or mock) as needed.
    MockAssetManager(RenderDevice& device, std::string_view base_path)
        : AssetManager(device, base_path) {}

    // Stub-based find_asset_id for test scenarios where reverse lookup is needed.
    auto find_asset_id(const Model& model) const -> std::string override {
        return find_asset_id_stub(model);
    }
    std::function<std::string(const Model&)> find_asset_id_stub =
        [](const Model&) -> std::string { return {}; };

    // Stub-based resolve_model for test scenarios where ID → Model resolution is needed.
    auto resolve_model(const std::string& id) -> Result<std::shared_ptr<Model>> override {
        return resolve_model_stub(id);
    }
    std::function<Result<std::shared_ptr<Model>>(const std::string&)> resolve_model_stub =
        [](const std::string& id) -> Result<std::shared_ptr<Model>> {
            return make_error(Error::Category::InvalidArgument,
                "MockAssetManager: resolve_model not configured for '" + id + "'");
        };
};
```

For simpler tests that do not exercise the AssetManager path (all non-MeshRenderer tests), use a default-constructed `SerializationContext{asset_mgr}` with a real or mock `AssetManager`. For the MeshRenderer round-trip test, configure the mock's `find_asset_id_stub` and `resolve_model_stub` to provide round-trip behavior (e.g., `resolve_model_stub` returns a `shared_ptr<Model>` with a known asset ID, and `find_asset_id_stub` returns the same ID when given that model).

The test should verify: (1) getter returns nullptr for a freshly constructed MeshRenderer, (2) serialization produces empty string for `model` key (TypeRegistry encodes null as empty string), (3) deserialization of a valid model stores the `shared_ptr<Model>` on the MeshRenderer, (4) the stored pointer is returned by subsequent getter calls. Full asset-cache resolution in tests is optional for this sprint (the TypeRegistry callbacks are tested directly via unit tests).

### Integration verification

The test file `tests/component_registry_tests.cpp` must compile and link with `buddd_tests`. Run:

```bash
cmake --build build --target buddd_tests && ./build/buddd_tests "[component-registry]"
```

All tests must pass.

## Edge cases

| # | Scenario | Expected behavior |
|---|---|---|
| EC-01 | Register the same type name twice | Warning logged, existing `ComponentInfo<T>&` returned. No error. |
| EC-02 | `create()` on type where factory returns nullptr | Not possible with `std::make_unique` (never returns nullptr). No explicit check needed. |
| EC-03 | `describe()` on unknown type | Returns nullptr. |
| EC-04 | Property with no constraints (min/max absent) | Property accepts any value of the correct type. Default min = -FLT_MAX, max = FLT_MAX. |
| EC-05 | Vec3 with NaN/Inf components | Serialized as-is (IEEE 754). Deserialization passes through (no NaN check). |
| EC-06 | Empty YAML node passed to `deserialize_component()` | No properties updated; success result. |
| EC-07 | YAML node with null/undefined scalar `~` for a required property | TypeRegistry decode returns error (type mismatch or yaml-cpp exception caught). |
| EC-08 | Intensity set to zero or negative value | Error: violates `min >= 0` constraint on light component properties. |
| EC-09 | String property containing YAML special characters | Properly escaped by yaml-cpp emitter. |
| EC-10 | MeshRenderer with null model (`shared_ptr<Model>` is nullptr) | Serializes as empty string (via TypeRegistry encode). Deserializing empty string produces null `shared_ptr<Model>`. |
| EC-11 | TypeRegistry::register_type<T>() called after component using type T is already registered | Behavior is implementation-defined: callbacks are overwritten, but existing Property objects already captured the old callbacks via capture-by-value. Warning logged. |
| EC-12 | Custom type produces invalid YAML (non-scalar where scalar expected) | TypeRegistry decode returns error. |
| EC-13 | `from_string` callback receives unparseable input | `Result<T>` error returned with descriptive message. |

## Security impact

None. This is a library-level metadata and serialization feature. No new network access, file system access (beyond existing yaml-cpp usage), user authentication, or privilege separation. Input validation of YAML values is handled by the deserialization path (TypeRegistry type checking, PropertyFlags constraint validation). yaml-cpp exceptions are caught at the engine boundary per ADR-016.

## Data and migration impact

None. No schema changes, data migrations, seed data, or data loss risks. The `find_asset_id()` method is additive and does not change existing `AssetManager` behavior. No component data format versioning implemented per NG-06.

## API compatibility impact

- `MeshRenderer`: **additive** — one new public method (`set_model(std::shared_ptr<Model>)`) if not already present. No `model_asset_id_` member or accessors. Existing `model()` accessor unchanged.
- `AssetManager`: **additive** — one new public method (`find_asset_id`). No existing API changes.
- `EngineService::create()`: **additive** — calls `register_builtin_types()` + `register_all_components()` internally. No signature change.
- All other changes are entirely new files with no backward compatibility impact.
- `CameraComponent`: **NO changes** — existing `set_perspective()` is used by property lambdas. No individual setters added.
- No `PropertyType` enum or `PropertyValue` variant added (removed from old design).

## Documentation impact

- **README**: None — no user-facing feature changes.
- **Wiki pages**: The `wiki-agent` will update relevant wiki pages (per spec G-10). This contract does not mandate wiki updates.
- **Other specs**: None.

## ADR impact

A new ADR should be created for the component registration and property system architecture decision (G-09, handled by adr-agent). This implementation does not deprecate any existing ADR.

## Done criteria

The implementation is complete when ALL of the following are verifiable:

### Code structure

- [ ] `src/engine/scene/component_registry/type_registry.h` exists with `TypeRegistry` static class (verified by reading file).
- [ ] `src/engine/scene/component_registry/type_registry.cpp` exists with type-erased entry storage (verified by reading file).
- [ ] `src/engine/scene/component_registry/property.h` exists with `PropertyFlags` struct and `Property` class (verified by reading file).
- [ ] `src/engine/scene/component_registry/component_info.h` exists with `ComponentInfoBase` (abstract) and `ComponentInfo<T>` (typed template) declaring `type_name()`, `create()`, `serialize()`, `deserialize()`, `add_property<PropType>()`, property metadata accessors (verified by reading file).
- [ ] `src/engine/scene/component_registry/component_registry.h` / `.cpp` exists with `register_component<T>()`, `create()`, `describe()`, `all_types()` (verified by reading file).
- [ ] `src/engine/scene/component_registry/serialization_context.h` exists with `SerializationContext` struct (verified by reading file).
- [ ] `src/engine/scene/component_registry/serialization.h` / `.cpp` exists with `serialize_component()`, `deserialize_component()` (verified by reading file).
- [ ] `src/engine/scene/component_registry/register_all_components.h` / `.cpp` exists with `register_builtin_types()` and `register_all_components()` (verified by reading file).
- [ ] `src/engine/math/vec3_yaml.h` exists with `YAML::convert<math::Vec3>` specialization (verified by reading file).
- [ ] `src/engine/math/vec4_yaml.h` exists with `YAML::convert<math::Vec4>` specialization (verified by reading file).
- [ ] `src/engine/math/quat_yaml.h` exists with `YAML::convert<math::Quat>` specialization (verified by reading file).
- [ ] `tests/component_registry_tests.cpp` exists (verified by reading file).
- [ ] File `property_type.h` does NOT exist (verified by `ls` — should not be present).

### Existing file modifications

- [ ] `src/engine/render/mesh_renderer.h` has `set_model(std::shared_ptr<Model>)` public method (if not already present). No `model_asset_id_` member or accessors (verified by diff).
- [ ] `src/engine/render/mesh_renderer.cpp` implements `set_model()` (verified by diff).
- [ ] `src/engine/asset/asset_manager.h` has `find_asset_id(const Model&) const -> std::string` and `resolve_model(const std::string&) -> Result<std::shared_ptr<Model>>` public methods (verified by diff).
- [ ] `src/engine/asset/asset_manager.cpp` implements `find_asset_id()` and `resolve_model()` (verified by diff).
- [ ] `src/engine/engine_service.cpp` includes `register_all_components.h` and calls `register_builtin_types()` then `register_all_components(registry)` after AssetManager creation (verified by diff).

### Files NOT modified

- [ ] `src/engine/scene/component.h` — no changes (verified by `git diff -- src/engine/scene/component.h` — empty).
- [ ] `src/engine/scene/entity.h` — no changes (verified by `git diff`).
- [ ] `src/engine/scene/world.h` — no changes (verified by `git diff`).
- [ ] `src/engine/scene/camera_component.h` / `.cpp` — no changes (verified by `git diff`).
- [ ] `src/engine/scene/point_light_component.h` / `.cpp` — no changes (verified by `git diff`).
- [ ] `src/engine/scene/directional_light_component.h` / `.cpp` — no changes (verified by `git diff`).
- [ ] `src/engine/scene/spot_light_component.h` / `.cpp` — no changes (verified by `git diff`).

### Build

- [ ] Engine library builds without errors: `cmake --build build --target buddd_engine 2>&1 | tail -20` shows no error.
- [ ] Test executable builds without errors: `cmake --build build --target buddd_tests 2>&1 | tail -20` shows no error.

### Test results (all pass)

Run `./build/buddd_tests "[component-registry]"`:

- [ ] `REGISTER_COMPONENT_AND_QUERY` — passes (AC-001, AC-002).
- [ ] `REGISTER_DUPLICATE_WARNS_AND_RETURNS_SAME` — passes (AC-031).
- [ ] `CREATE_UNKNOWN_TYPE_RETURNS_ERROR` — passes (AC-003).
- [ ] `DESCRIBE_UNKNOWN_TYPE_RETURNS_NULLPTR` — passes (AC-004).
- [ ] `ALL_TYPES_COUNT` — passes (AC-005).
- [ ] `TYPE_REGISTRY_REGISTER_AND_ENCODE` — passes (AC-032, AC-033).
- [ ] `TYPE_REGISTRY_STRING_ROUNDTRIP` — passes (AC-034).
- [ ] `TYPE_REGISTRY_VALIDATE` — passes (AC-035).
- [ ] `TYPE_REGISTRY_OVERWRITE_WARNS` — passes (AC-038).
- [ ] `TYPE_REGISTRY_BUILTIN_FLOAT` — passes (AC-036).
- [ ] `TYPE_REGISTRY_BUILTIN_VEC3` — passes (AC-036).
- [ ] `TYPE_REGISTRY_BUILTIN_SHARED_PTR_MODEL` — passes (AC-036).
- [ ] `TYPE_REGISTRY_UNREGISTERED_RUNTIME_ERROR` — passes (AC-039).
- [ ] `PROPERTY_METADATA` — passes (AC-006).
- [ ] `PROPERTY_GET_SET` — passes (AC-007, AC-008).
- [ ] `PROPERTY_VALIDATION_MIN` — passes (AC-009).
- [ ] `PROPERTY_VALIDATION_MAX` — passes (AC-009).
- [ ] `CAMERA_COMPONENT_PROPERTIES` — passes (AC-010).
- [ ] `POINT_LIGHT_COMPONENT_PROPERTIES` — passes (AC-011).
- [ ] `DIRECTIONAL_LIGHT_COMPONENT_PROPERTIES` — passes (AC-012).
- [ ] `SPOT_LIGHT_COMPONENT_PROPERTIES` — passes (AC-013).
- [ ] `MESH_RENDERER_COMPONENT_PROPERTIES` — passes (AC-014).
- [ ] `SERIALIZE_CAMERA_COMPONENT` — passes (AC-016).
- [ ] `DESERIALIZE_CAMERA_COMPONENT` — passes (AC-017).
- [ ] `ROUND_TRIP_CAMERA_COMPONENT` — passes (AC-018).
- [ ] `ROUND_TRIP_POINT_LIGHT` — passes (AC-019).
- [ ] `ROUND_TRIP_DIRECTIONAL_LIGHT` — passes (AC-020).
- [ ] `ROUND_TRIP_SPOT_LIGHT` — passes (AC-021).
- [ ] `ROUND_TRIP_MESH_RENDERER` — passes (AC-022).
- [ ] `YAML_CONVERT_VEC3` — passes (AC-023, AC-024).
- [ ] `YAML_CONVERT_VEC4` — passes (AC-025).
- [ ] `YAML_CONVERT_QUAT` — passes (AC-025).
- [ ] `DESERIALIZE_UNKNOWN_KEY_WARNING` — passes (AC-026).
- [ ] `DESERIALIZE_TYPE_MISMATCH` — passes (AC-027).
- [ ] `DESERIALIZE_OUT_OF_RANGE` — passes (AC-028).
- [ ] `FACTORY_CREATES_CORRECT_TYPE` — passes (AC-002, AC-015).

### Acceptance criteria mapping

| AC | Code change | Test |
|---|---|---|
| AC-001 | `component_registry.h` — `register_component<T>()` + `describe()` | `REGISTER_COMPONENT_AND_QUERY` |
| AC-002 | `component_registry.h` — `create()` | `REGISTER_COMPONENT_AND_QUERY` + `FACTORY_CREATES_CORRECT_TYPE` |
| AC-003 | `component_registry.cpp` — error on unknown | `CREATE_UNKNOWN_TYPE_RETURNS_ERROR` |
| AC-004 | `component_registry.cpp` — nullptr on unknown | `DESCRIBE_UNKNOWN_TYPE_RETURNS_NULLPTR` |
| AC-005 | `component_registry.h` — `all_types()` | `ALL_TYPES_COUNT` |
| AC-006 | `component_info.h` — property metadata (name/type_index) | `PROPERTY_METADATA` |
| AC-007 | `component_info.h` — property getter | `PROPERTY_GET_SET` |
| AC-008 | `component_info.h` — property setter | `PROPERTY_GET_SET` |
| AC-009 | `component_info.h` — add_property wrapper validates min/max | `PROPERTY_VALIDATION_MIN` + `PROPERTY_VALIDATION_MAX` |
| AC-010 | `register_all_components.cpp` — camera | `CAMERA_COMPONENT_PROPERTIES` |
| AC-011 | `register_all_components.cpp` — point_light | `POINT_LIGHT_COMPONENT_PROPERTIES` |
| AC-012 | `register_all_components.cpp` — directional_light | `DIRECTIONAL_LIGHT_COMPONENT_PROPERTIES` |
| AC-013 | `register_all_components.cpp` — spot_light | `SPOT_LIGHT_COMPONENT_PROPERTIES` |
| AC-014 | `register_all_components.cpp` — mesh_renderer | `MESH_RENDERER_COMPONENT_PROPERTIES` |
| AC-015 | `register_all_components.cpp` — all 5 | `FACTORY_CREATES_CORRECT_TYPE` (full count check) |
| AC-016 | `serialization.cpp` — serialize_component | `SERIALIZE_CAMERA_COMPONENT` |
| AC-017 | `serialization.cpp` — deserialize_component | `DESERIALIZE_CAMERA_COMPONENT` |
| AC-018 | `serialization.cpp` — round-trip | `ROUND_TRIP_CAMERA_COMPONENT` |
| AC-019 | `serialization.cpp` — point light round-trip | `ROUND_TRIP_POINT_LIGHT` |
| AC-020 | `serialization.cpp` — directional light round-trip | `ROUND_TRIP_DIRECTIONAL_LIGHT` |
| AC-021 | `serialization.cpp` — spot light round-trip | `ROUND_TRIP_SPOT_LIGHT` |
| AC-022 | `serialization.cpp` — mesh renderer round-trip | `ROUND_TRIP_MESH_RENDERER` |
| AC-023 | `vec3_yaml.h` — encode | `YAML_CONVERT_VEC3` |
| AC-024 | `vec3_yaml.h` — decode | `YAML_CONVERT_VEC3` |
| AC-025 | `vec4_yaml.h`, `quat_yaml.h` — convert | `YAML_CONVERT_VEC4` + `YAML_CONVERT_QUAT` |
| AC-026 | `serialization.cpp` — unknown key warning | `DESERIALIZE_UNKNOWN_KEY_WARNING` |
| AC-027 | `serialization.cpp` — type mismatch error | `DESERIALIZE_TYPE_MISMATCH` |
| AC-028 | `serialization.cpp` — out-of-range error | `DESERIALIZE_OUT_OF_RANGE` |
| AC-029 | All tests use in-memory YAML | Code inspection of test file (no file I/O) |
| AC-030 | `component.h` NOT modified | `git diff -- src/engine/scene/component.h` is empty |
| AC-031 | `register_component<T>()` duplicate returns same ref (warning) | `REGISTER_DUPLICATE_WARNS_AND_RETURNS_SAME` |
| AC-032 | `TypeRegistry::register_type<T>()` stores five callbacks | `TYPE_REGISTRY_REGISTER_AND_ENCODE` |
| AC-033 | `TypeRegistry::yaml_encode<T>()` / `yaml_decode<T>()` round-trip | `TYPE_REGISTRY_REGISTER_AND_ENCODE` |
| AC-034 | `TypeRegistry::to_string<T>()` / `from_string<T>()` round-trip | `TYPE_REGISTRY_STRING_ROUNDTRIP` |
| AC-035 | `TypeRegistry::validate<T>()` works | `TYPE_REGISTRY_VALIDATE` |
| AC-036 | Built-in types pre-registered | `TYPE_REGISTRY_BUILTIN_FLOAT` + `TYPE_REGISTRY_BUILTIN_VEC3` + `TYPE_REGISTRY_BUILTIN_SHARED_PTR_MODEL` |
| AC-037 | `add_property<PropType>()` uses TypeRegistry (not PropertyType enum) | Indirect: all round-trip tests pass (serialization works) |
| AC-038 | `TypeRegistry::register_type<T>()` duplicate logs WARNING and overwrites | `TYPE_REGISTRY_OVERWRITE_WARNS` |
| AC-039 | Unregistered type produces runtime error (convenience methods return `Result` error; add_property logs FATAL + aborts) | `TYPE_REGISTRY_UNREGISTERED_RUNTIME_ERROR` |
