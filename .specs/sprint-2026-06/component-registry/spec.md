# SPEC-NNNN — Component Registration & Property System

## Problem

The engine has five concrete `Component` subclasses (`CameraComponent`, `PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`, `MeshRenderer`) but no uniform way to:

- Discover what component types exist at runtime.
- Create a component instance by string name (e.g., `"camera"`).
- Iterate a component's properties (name, type, current value) for inspection or editing.
- Serialize or deserialize a component's data to/from YAML without per-component boilerplate.

Previously, serialization required a rigid `PropertyType` enum that had to be modified every time a new type was added, making the system closed to extension by external code. External game code could not register custom property types without modifying the engine.

Without this spec, every downstream feature (scene serialization, editor property panel, prefabs) would need to re-solve the same discovery, metadata, and serialization problems from scratch.

## Goals

| ID | Goal |
|---|---|
| G-01 | Implement a **TypeRegistry** — a static registry that maps each C++ type to its YAML encode/decode, string conversion, and validation behavior. Each TypeRegistry callback receives a `const SerializationContext&` parameter for context-dependent operations (e.g., asset resolution). Built-in types (`float`, `int32_t`, `bool`, `std::string`, `Vec3`, `Vec4`, `Quat`, `std::shared_ptr<Model>`) are pre-registered at startup. External code registers custom types via `TypeRegistry::register_type<T>()`. |
| G-02 | Define an internal **`Property` descriptor** — one per component field — containing a human-readable name, type-erased getter/setter lambdas, YAML encode/decode callbacks (delegating to TypeRegistry), string conversion callbacks, a validation callback, and optional `PropertyFlags` constraints. `Property` is a purely internal implementation detail, not a user-facing concept. |
| G-03 | Define a **`ComponentInfoBase`** type-erased base class — one per component type — containing a canonical string name (e.g., `"camera"`), a factory function `() -> unique_ptr<Component>`, and type-erased property descriptors. Define a **`ComponentInfo<T>`** template class deriving from `ComponentInfoBase` — adds three overloads of a typed `add_property<PropType>()` method for convenient property population: convention-based (auto-detects `get_<name>()`/`set_<name>()`), simple lambdas, and context-aware lambdas. |
| G-04 | Implement a **`ComponentRegistry`** class with: `register_component<T>(string_name) -> ComponentInfo<T>&`, `create(string_name) -> Result<unique_ptr<Component>>`, `describe(string_name) -> const ComponentInfoBase*`, `all_types() -> span<const ComponentInfoBase*>`. |
| G-05 | Add `YAML::convert` specializations for engine math types (`Vec3`, `Vec4`, `Quat`) in their own headers alongside each type. These are used by TypeRegistry's pre-registration. |
| G-06 | Implement generic YAML serialization helpers: `serialize_component(const ComponentInfoBase&, const Component&, const SerializationContext&) -> YAML::Node` and `deserialize_component(const ComponentInfoBase&, const YAML::Node&, Component&, const SerializationContext&) -> Result<void>` using the property system and TypeRegistry. |
| G-07 | Register all five existing engine components (`CameraComponent`, `PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`, `MeshRenderer`) with their properties. |
| G-08 | Pre-register the eight built-in property types in TypeRegistry: `float`, `int32_t`, `bool`, `std::string`, `Vec3`, `Vec4`, `Quat`, `std::shared_ptr<Model>`. |
| G-09 | Record an ADR for the architectural decision (handled by adr-agent). |
| G-10 | Write unit tests covering: TypeRegistry operations, property access, factory creation, YAML round-trip serialization for each registered component. |
| G-11 | Update wiki documentation (handled by wiki-agent). |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No scene file format or World save-load.** This spec creates the *building blocks* for per-component serialization, not a full scene graph (de)serializer. |
| NG-02 | **No editor property panel UI.** The editor's Properties panel remains a placeholder — this spec enables it but does not implement it. |
| NG-03 | **No scene panel / entity tree.** |
| NG-04 | **No prefab system.** |
| NG-05 | **No entity-reference serialization.** References from one component to another entity are not supported. |
| NG-06 | **No versioning or migration.** Component formats are assumed current — no forward/backward compatibility layer. |
| NG-07 | **No special handling for `Transform`.** `Transform` is NOT a `Component` (it is stored inline in `EntityNode`). Serializing/deserializing transforms is deferred as a future concern. |
| NG-08 | **No runtime component addition/removal of properties.** The property list for a component type is static after registration. |
| NG-09 | **No nested or compound properties.** Each property is a single value of a supported type — no arrays, no sub-objects within a property. |
| NG-10 | **No `Mat4` YAML::convert or TypeRegistry entry.** `Mat4` is not used as a component property in any existing component. It can be added later if needed. |

## Actors

| Actor | Description |
|---|---|
| **Engine developer** | A developer adding new component types to the engine. Registers the component and its properties via the `register_all_components()` function. Uses `TypeRegistry::register_type<T>()` if a custom property type is needed. |
| **External game developer** | A developer writing game code outside the engine. Registers custom property types via `TypeRegistry::register_type<T>()`, then uses them in component properties with the same `add_property<T>()` API as built-in types. |
| **Editor developer** | A developer building the editor property panel (future work). Uses `ComponentRegistry::describe()` and property descriptors to enumerate properties. Uses `TypeRegistry::to_string<T>()` / `from_string<T>()` for type-agnostic display. The `PropertyValue` variant is an internal helper for built-in types in the UI layer. |
| **Scene serialization developer** | A developer implementing World save/load (future work). Uses `serialize_component()` and `deserialize_component()` for per-component YAML I/O. |
| **Test suite** | Automated tests that verify TypeRegistry operations, property access, factory creation, YAML round-trip, and error handling for every registered component. |

## User-visible behavior

### TypeRegistry API

`TypeRegistry` is a static class whose template methods are available after including `type_registry.h`. Built-in types are pre-registered automatically (during static initialization or a one-time lazy init). Users register custom types before using them in component properties.

```cpp
// Register a custom type — all five callbacks are required.
// Each callback receives const SerializationContext& for context-dependent operations.
TypeRegistry::register_type<MyCustomType>(
    [](const MyCustomType& v, const SerializationContext&) -> YAML::Node {
        return YAML::Node(v.serialize_to_yaml());
    },
    [](const YAML::Node& n, const SerializationContext&) -> Result<MyCustomType> {
        return MyCustomType::deserialize_from_yaml(n);
    },
    [](const MyCustomType& v, const SerializationContext&) -> std::string {
        return v.to_string();
    },
    [](const std::string& s, const SerializationContext&) -> Result<MyCustomType> {
        return MyCustomType::from_string(s);
    },
    [](const MyCustomType& v, const SerializationContext&) -> Result<void> {
        return v.is_valid() ? Result<void>{} : make_error("invalid custom type");
    }
);

// Query registered type info (optional, for tooling/debugging):
auto registered = TypeRegistry::is_registered<std::string>();  // true (built-in)
```

### Component Registration API

A central startup function `register_all_components(ComponentRegistry&)` registers all known components. Each registration binds a canonical string name (e.g., `"camera"`, `"point_light"`) and a factory function.

Properties are populated by calling one of three `add_property<PropType>()` overloads on the returned `ComponentInfo<T>&`. Overload (A) auto-detects `get_<name>()`/`set_<name>()` accessors by naming convention at compile time. Overload (B) accepts simple getter/setter lambdas without `SerializationContext`. Overload (C) accepts context-aware lambdas that receive `const SerializationContext&`. All overloads internally wire to `TypeRegistry::yaml_encode<PropType>()`, `TypeRegistry::yaml_decode<PropType>()`, `TypeRegistry::to_string<PropType>()`, `TypeRegistry::from_string<PropType>()`, and `TypeRegistry::validate<PropType>()` for serialization and display behavior.

Duplicate type names log a WARNING and return the existing reference (no error).

### Registration examples

```cpp
void register_camera(ComponentRegistry& reg) {
    auto& info = reg.register_component<CameraComponent>("camera");
    
    // (B) Simple lambdas — no SerializationContext needed
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
```

```cpp
void register_point_light(ComponentRegistry& reg) {
    auto& info = reg.register_component<PointLightComponent>("point_light");
    
    // (B) Simple lambdas — no SerializationContext needed
    info.add_property<Vec3>("colour",
        [](const PointLightComponent& c) -> Vec3 { return c.colour(); },
        [](PointLightComponent& c, const Vec3& v) -> Result<void> { c.colour() = v; return {}; }
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
```

```cpp
void register_mesh_renderer(ComponentRegistry& reg) {
    auto& info = reg.register_component<MeshRenderer>("mesh_renderer");
    
    // (C) Context-aware lambdas — receives SerializationContext (here unused,
    // but TypeRegistry handles asset ID ↔ Model conversion internally).
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
```

### Registry queries

```cpp
auto reg = ComponentRegistry();

// After registration:
auto info = reg.describe("camera");  // -> const ComponentInfoBase* (nullptr if unknown)
auto comp = reg.create("camera");     // -> Result<unique_ptr<Component>>
auto all  = reg.all_types();          // -> span<const ComponentInfoBase*>
```

### YAML serialization (property-driven)

```cpp
SerializationContext ctx{asset_manager};
YAML::Node node = serialize_component(*info, *comp, ctx);
// Produces:
// fov_y: 1.0471975512
// aspect: 1.7777777778
// near: 0.1
// far: 100.0
// model: "models/crate.gltf"  (for MeshRenderer — asset ID string, encoded via TypeRegistry)

auto result = deserialize_component(*info, node, *comp, ctx);
// info is const ComponentInfoBase&
// result is Result<void> — warning logged for unknown keys (skipped); error on invalid value.
```

Internally, `serialize_component()` iterates each property and calls the property's internal `to_yaml` callback, which delegates to `TypeRegistry::yaml_encode<PropType>()`. `deserialize_component()` delegates to `TypeRegistry::yaml_decode<PropType>()`. No `PropertyType` enum switching is involved.

### YAML::convert for math types

```yaml
# Vec3 serializes as:
{x: 1.0, y: 2.0, z: 3.0}

# Quat serializes as:
{x: 0.0, y: 0.0, z: 0.0, w: 1.0}
```

`YAML::convert<math::Vec3>` specializations exist in separate files alongside each type (e.g., `vec3_yaml.h`), NOT in the primary type header, to avoid forcing a yaml-cpp dependency on all math includes. These specializations are used by the TypeRegistry's pre-registration of built-in types.

### TypeRegistry built-in type behavior

| C++ type | YAML format | String format | Validation |
|---|---|---|---|
| `float` | YAML scalar | e.g. `"1.5"` | Always valid (IEEE 754 passthrough). Property-level constraints use `PropertyFlags::min`/`max`. |
| `int32_t` | YAML scalar (integer) | e.g. `"42"` | Always valid. |
| `bool` | YAML scalar (`true`/`false`) | `"true"` / `"false"` | Always valid. |
| `std::string` | YAML scalar | Identity | Always valid. |
| `math::Vec3` | Mapping with `x`, `y`, `z` keys | e.g. `"(1.0, 2.0, 3.0)"` | Always valid (NaN/Inf pass through). |
| `math::Vec4` | Mapping with `x`, `y`, `z`, `w` keys | e.g. `"(1.0, 2.0, 3.0, 4.0)"` | Always valid. |
| `math::Quat` | Mapping with `x`, `y`, `z`, `w` keys | e.g. `"(0.0, 0.0, 0.0, 1.0)"` | Always valid. |
| `std::shared_ptr<Model>` | YAML scalar (asset ID string) | Asset ID string e.g. `"models/crate.gltf"` | Null shared_ptr encodes as empty string; empty string decodes to null shared_ptr. |

## Key entities

### TypeRegistry (static class)

```
TypeRegistry (static/global) {
    // All callbacks receive const SerializationContext& for context-dependent operations.
    template register_type<T>(
        function<YAML::Node(const T&, const SerializationContext&)> yaml_encode,
        function<Result<T>(const YAML::Node&, const SerializationContext&)> yaml_decode,
        function<string(const T&, const SerializationContext&)> to_string,
        function<Result<T>(const string&, const SerializationContext&)> from_string,
        function<Result<void>(const T&, const SerializationContext&)> validate
    ) -> void

    template yaml_encode<T>(const T&, const SerializationContext&) -> YAML::Node
    template yaml_decode<T>(const YAML::Node&, const SerializationContext&) -> Result<T>
    template to_string<T>(const T&, const SerializationContext&) -> string
    template from_string<T>(const string&, const SerializationContext&) -> Result<T>
    template validate<T>(const T&, const SerializationContext&) -> Result<void>
    
    template is_registered<T>() -> bool
}
```

Internally, `TypeRegistry` stores a `std::unordered_map<std::type_index, TypeEntry>` where `TypeEntry` holds the five type-erased callbacks. Template functions dispatch via `std::type_index(typeid(T))`.

### Property (internal, not user-facing)

`Property` is a purely internal class. Users do not construct or interact with `Property` directly — properties are added via `add_property<PropType>()` on `ComponentInfo<T>`.

```
Property {
    name: string
    type_index: std::type_index     // identifies the PropType at runtime
    get: type-erased getter (Component&, SerializationContext&) -> any
    set: type-erased setter (Component&, any, SerializationContext&) -> Result<void>
    to_yaml: type-erased (any) -> YAML::Node       // delegates to TypeRegistry
    from_yaml: type-erased (YAML::Node) -> Result<any>  // delegates to TypeRegistry
    to_string: type-erased (any) -> string          // delegates to TypeRegistry
    from_string: type-erased (string) -> Result<any> // delegates to TypeRegistry
    validate: type-erased (any) -> Result<void>     // delegates to TypeRegistry
    flags: PropertyFlags (optional: min, max, step, enum_choices)
}
```

The `PropertyValue` variant (holding `float`, `int32_t`, `bool`, `std::string`, `Vec3`, `Vec4`, `Quat`) is an editor/UI-layer concept only, not a core serialization mechanism. Custom types are displayed via `TypeRegistry::to_string<T>()` / `from_string<T>()`.

### ComponentInfoBase (type-erased base)

```
ComponentInfoBase {
    type_name: string
    create: function<unique_ptr<Component>()>
    // internal: vector<Property> (not directly exposed)
}
```

### ComponentInfo<T> (typed template, derives from ComponentInfoBase)

```
ComponentInfo<T> : ComponentInfoBase {
    // (A) Convention-based — auto-detects get_<name>() and set_<name>(value) at compile time.
    //     Documented API but implementation deferred for v1 — use (B) or (C) instead.
    template<typename PropType>
    add_property(string_view name, PropertyFlags flags = {}) -> void

    // (B) Simple lambdas — no SerializationContext needed.
    template<typename PropType>
    add_property(string_view name,
        function<PropType(const T&)> getter,
        function<Result<void>(T&, PropType)> setter,
        PropertyFlags flags = {}) -> void

    // (C) Context-aware lambdas — receives SerializationContext for context-dependent logic.
    template<typename PropType>
    add_property(string_view name,
        function<PropType(const T&, const SerializationContext&)> getter,
        function<Result<void>(T&, PropType, const SerializationContext&)> setter,
        PropertyFlags flags = {}) -> void
}
```

Overload (A) uses template metaprogramming to detect `get_<name>()` / `set_<name>(value)` methods on T at compile time. If the accessors are not found, a compile error is produced. This is the zero-boilerplate path for components with standard accessor naming. Overload (B) wraps the simple lambdas into context-accepting ones that ignore the context. Overload (C) is the most general — explicit context-aware lambdas.

All overloads create a `Property` whose callbacks are wired to `TypeRegistry::yaml_encode<PropType>()`, `TypeRegistry::yaml_decode<PropType>()`, `TypeRegistry::to_string<PropType>()`, `TypeRegistry::from_string<PropType>()`, and `TypeRegistry::validate<PropType>()`. The getter/setter lambdas are type-erased internally.

### ComponentRegistry

```
ComponentRegistry {
    register_component<T>(string_view type_name) -> ComponentInfo<T>&
    create(string_view type_name) -> Result<unique_ptr<Component>>
    describe(string_view type_name) -> const ComponentInfoBase*
    all_types() -> span<const ComponentInfoBase*>
}
```

### SerializationContext

```
SerializationContext {
    assets: AssetManager&
}
```

A struct holding a reference to the `AssetManager`. Passed to both `serialize_component()` and `deserialize_component()` to enable asset-reference resolution during serialization.

### Registration entry point

```
register_all_components(ComponentRegistry&) -> void
```

Called once during engine startup, before any component creation or serialization. Calling `register_all_components()` twice is safe — each `register_component<T>()` call logs a WARNING and returns the existing `ComponentInfo<T>&` on duplicate registration.

### YAML serialization functions (free functions)

```
serialize_component(const ComponentInfoBase& info, const Component& comp, const SerializationContext& ctx) -> YAML::Node
deserialize_component(const ComponentInfoBase& info, const YAML::Node& node, Component& comp, const SerializationContext& ctx) -> Result<void>
```

### PropertyFlags

```
PropertyFlags {
    min<float>        // numeric minimum (float properties)
    max<float>        // numeric maximum (float properties)
    step<float>       // UI step increment
    enum_choices<vector<string>>  // display names for enum-like int32_t properties
}
```

PropertyFlags provide PROPERTY-level constraints. `TypeRegistry::validate<T>()` provides TYPE-level validation (e.g., is this a valid float?). The two are independent — `TypeRegistry::validate<float>(3.14f)` returns success, while `PropertyFlags::min(0.001f).max(3.14159f)` on a specific `fov_y` property constrains the range further.

## User stories

### Story 1 — Register a component with properties (Priority: P1)

**As an** engine developer,
**I want to** register a new component type with its string name and properties,
**So that** the registry can create instances and access properties by name.

**Given** a `ComponentRegistry` and a component type `CameraComponent`
**When** `register_all_components(registry)` is called
**Then** `registry.describe("camera")` returns a valid `ComponentInfoBase*`
**And** `registry.create("camera")` returns a `Result` containing a `unique_ptr<Component>` that is-a `CameraComponent`
**And** `registry.all_types()` contains an entry with `type_name` equal to `"camera"`

### Story 2 — Create a component by string name (Priority: P1)

**As an** scene serialization developer,
**I want to** create a component instance from its string name,
**So that** I can deserialize a scene without compile-time type knowledge.

**Given** a `ComponentRegistry` with `CameraComponent` registered as `"camera"`
**When** `registry.create("camera")` is called
**Then** the result is a `unique_ptr<Component>` whose dynamic type is `CameraComponent`
**And** its properties have default-constructed values

### Story 3 — Query property information (Priority: P1)

**As an** editor developer,
**I want to** enumerate the properties of a registered component type,
**So that** I can build a generic property inspector UI.

**Given** a `ComponentInfoBase` for `CameraComponent` with type name `"camera"`
**When** I iterate its properties
**Then** I get at least `fov_y`, `aspect`, `near`, `far` (all of type `float`)
**And** each property has a name, type information, getter, and setter

### Story 4 — Serialize a component to YAML (Priority: P1)

**As an** scene serialization developer,
**I want to** serialize a component's properties to a YAML node,
**So that** I can store component state in a file.

**Given** a `ComponentInfoBase` for `CameraComponent`, a `CameraComponent` with `fov_y=1.0`, `aspect=2.0`, `near=0.01`, `far=500.0`, and a `SerializationContext`
**When** `serialize_component(info, comp, ctx)` is called
**Then** the result is a `YAML::Node` mapping with keys `fov_y`, `aspect`, `near`, `far`
**And** each key maps to the correct float value

### Story 5 — Deserialize a component from YAML (Priority: P1)

**As an** scene serialization developer,
**I want to** deserialize a YAML node back into a component's properties,
**So that** I can restore component state from a file.

**Given** a `ComponentInfoBase` for `CameraComponent`, a default-constructed `CameraComponent`, and a `SerializationContext`
**When** `deserialize_component(info, yaml_node, comp, ctx)` is called with a valid YAML mapping
**Then** the component's properties are updated to match the YAML values
**And** the result is `Result<void>` containing success

### Story 6 — Math type YAML round-trip (Priority: P2)

**As a** developer,
**I want to** serialize and deserialize `Vec3`, `Vec4`, and `Quat` using `YAML::convert`,
**So that** component properties of these types can be stored and restored.

**Given** a `Vec3(1.0, 2.0, 3.0)`
**When** I convert it to YAML and back via `YAML::convert<math::Vec3>`
**Then** the round-tripped value equals the original

**Given** a `Quat(1.0, 0.0, 0.0, 0.0)` (identity)
**When** I convert it to YAML and back via `YAML::convert<math::Quat>`
**Then** the round-tripped value equals the original

### Story 7 — Custom type registration (Priority: P2)

**As an** external game developer,
**I want to** register a custom property type via `TypeRegistry::register_type<T>()`
**So that** I can use it in component properties with the same `add_property<T>()` API as built-in types.

**Given** a custom type `MyType` registered in TypeRegistry with YAML/string/validate callbacks
**When** a component property is declared with `add_property<MyType>("data", getter, setter)`
**Then** the property serializes and deserializes using the registered callbacks
**And** the property displays via `to_string`/`from_string` in UI context

### Story 8 — Error on unknown component type (Priority: P2)

**As a** developer,
**I want to** get a clear error when trying to create an unregistered component type,
**So that** I can catch typos and configuration mistakes.

**Given** a `ComponentRegistry` without `"unknown_type"` registered
**When** `registry.create("unknown_type")` is called
**Then** the result is an error with `Error::Category::InvalidArgument`
**And** the error message contains `"unknown_type"`

### Story 9 — Warning on unknown YAML property (Priority: P2)

**As a** developer,
**I want to** get a warning (not error) when deserializing YAML with an unknown property name,
**So that** scene files from future engine versions remain forward-compatible.

**Given** a `ComponentInfoBase` for `CameraComponent` and a YAML node with an unknown key `"invalid_prop"`
**When** `deserialize_component(info, node, comp, ctx)` is called
**Then** the result is success (no error)
**And** a warning is logged identifying `"invalid_prop"` as the unknown property being skipped

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `ComponentRegistry::register_component<T>()` accepts a string name and returns a `ComponentInfo<T>&` for property population. | Unit test: register a test component, call `describe()` to verify non-null. |
| AC-002 | `ComponentRegistry::create(type_name)` returns a `unique_ptr<Component>` for registered types. | Unit test: register and create; verify dynamic_cast succeeds. |
| AC-003 | `ComponentRegistry::create(type_name)` returns error for unregistered type names. | Unit test: `create("nonexistent")` returns error. |
| AC-004 | `ComponentRegistry::describe(type_name)` returns `nullptr` for unknown types. | Unit test: `describe("nonexistent")` returns nullptr. |
| AC-005 | `ComponentRegistry::all_types()` returns a span of `const ComponentInfoBase*` containing all registered component type descriptors. | Unit test: register 3 components, verify `all_types().size() == 3`. |
| AC-006 | Each `Property` (internal) has a name, type index, getter, and setter. Properties are populated via one of the three `add_property<PropType>()` overloads on `ComponentInfo<T>`. | Unit test: read property metadata from a registered component. |
| AC-007 | Property getter returns the current value of the component field. | Unit test: set a field, read via property getter, verify value. |
| AC-008 | Property setter updates the component field to the given value. | Unit test: use property setter, read via component accessor, verify value. |
| AC-009 | Property setter validates numeric constraints (min/max) where defined, returning error on violation. | Unit test: set a property below min or above max, verify error. |
| AC-010 | `CameraComponent` is registered with type name `"camera"` and 4 float properties. | Unit test: `describe("camera")` returns info with 4 properties of type float. |
| AC-011 | `PointLightComponent` is registered with type name `"point_light"` and properties: `colour` (Vec3), `intensity` (float, `min>0`), `range` (float, `min>0`). | Unit test: verify names, types, and min constraints. |
| AC-012 | `DirectionalLightComponent` is registered with type name `"directional_light"` and properties: `colour` (Vec3), `intensity` (float, `min>0`). | Unit test: verify names, types, and min constraints. |
| AC-013 | `SpotLightComponent` is registered with type name `"spot_light"` and properties: `colour` (Vec3), `intensity` (float, `min>0`), `range` (float, `min>0`), `inner_angle` (float, `min>0`), `outer_angle` (float, `min>0`). | Unit test: verify names, types, and min constraints. |
| AC-014 | `MeshRenderer` is registered with type name `"mesh_renderer"` and property: `model` (type `std::shared_ptr<Model>`). The TypeRegistry handles asset ID ↔ Model conversion internally via `SerializationContext`. | Unit test: verify name, type, and round-trip with mock AssetManager. |
| AC-015 | `register_all_components(registry)` registers all 5 engine components. | Unit test: call `register_all_components`, verify `all_types().size() == 5`. |
| AC-016 | `serialize_component()` produces a `YAML::Node` mapping with one key per property. | Unit test: serialize `CameraComponent`, verify YAML keys match property names. |
| AC-017 | `deserialize_component()` updates component properties from a valid YAML node. | Unit test: serialize, modify YAML, deserialize back, verify new values. |
| AC-018 | YAML round-trip for `CameraComponent`: serialize then deserialize produces an equivalent component. | Unit test: serialize default CameraComponent → YAML → deserialize into new instance, verify all properties match. |
| AC-019 | YAML round-trip for `PointLightComponent`. | Same pattern as AC-018. |
| AC-020 | YAML round-trip for `DirectionalLightComponent`. | Same pattern as AC-018. |
| AC-021 | YAML round-trip for `SpotLightComponent`. | Same pattern as AC-018. |
| AC-022 | YAML round-trip for `MeshRenderer` (model as `shared_ptr<Model>`, asset ID resolved via TypeRegistry). | Same pattern as AC-018 (with mock AssetManager providing round-trip). |
| AC-023 | `YAML::convert<math::Vec3>` serializes as mapping with `x`, `y`, `z` keys. | Unit test: convert `Vec3(1,2,3)` to YAML and verify `node["x"].as<float>() == 1.0`. |
| AC-024 | `YAML::convert<math::Vec3>` deserializes correctly from mapping. | Unit test: create YAML node, convert to `Vec3`, verify values. |
| AC-025 | `YAML::convert<math::Vec4>` and `YAML::convert<math::Quat>` work symmetrically. | Unit test: round-trip both types. |
| AC-026 | `deserialize_component()` logs a warning and skips unknown property name in YAML (forward-compatible). | Unit test: deserialize YAML with extra unknown key, verify no error and warning logged. |
| AC-027 | `deserialize_component()` returns error on type mismatch (e.g., non-numeric value for float property). | Unit test: deserialize YAML with string value where float expected, verify error. |
| AC-028 | `deserialize_component()` returns error on out-of-range value (violates min/max constraint). | Unit test: deserialize YAML with `fov_y: 0.0` where min is `0.001`, verify error. |
| AC-029 | All YAML serialization/deserialization tests verify round-trip in-memory (no file I/O). | Inspection: test code uses `YAML::Node` directly, not file read/write. |
| AC-030 | `Component` base class is NOT modified by this feature. | Diff check: no changes to `src/engine/scene/component.h`. |
| AC-031 | `ComponentRegistry::register_component<T>()` logs a WARNING and returns the existing `ComponentInfo<T>&` when registering a duplicate type name (no error). | Unit test: register `"test_type"`, register again with same name, verify no error and same reference returned. |
| AC-032 | `TypeRegistry::register_type<T>()` accepts five callbacks each receiving `const SerializationContext&` (yaml_encode, yaml_decode, to_string, from_string, validate) and stores them for type `T`. | Unit test: register a test type with all five callbacks, then verify each operation returns expected results. |
| AC-033 | `TypeRegistry::yaml_encode<T>()` and `TypeRegistry::yaml_decode<T>()` round-trip correctly for a registered type. | Unit test: register type, encode value to YAML, decode back, verify equality. |
| AC-034 | `TypeRegistry::to_string<T>()` and `TypeRegistry::from_string<T>()` round-trip correctly for a registered type. | Unit test: register type, convert to string and back, verify equality. |
| AC-035 | `TypeRegistry::validate<T>()` returns error for invalid values and success for valid values of a registered type. | Unit test: register type with custom validate, test valid and invalid inputs. |
| AC-036 | Built-in types (`float`, `int32_t`, `bool`, `std::string`, `Vec3`, `Vec4`, `Quat`, `std::shared_ptr<Model>`) are pre-registered in TypeRegistry without explicit user registration. | Unit test: `TypeRegistry::yaml_encode<float>(3.14f)` returns valid YAML; `TypeRegistry::to_string<float>(3.14f)` returns `"3.14"` (or similar). Verify `shared_ptr<Model>` round-trip with mock AssetManager. |
| AC-037 | `add_property<PropType>()` uses `TypeRegistry::yaml_encode<PropType>()` / `yaml_decode<PropType>()` for serialization (not PropertyType enum switching). | Unit test: register component with `add_property<float>`; serialize and verify YAML output is produced correctly (indirect verification: round-trip passes). |
| AC-038 | `TypeRegistry::register_type<T>()` for an already-registered type logs a WARNING and overwrites the existing entry. | Unit test: register a type, register again with different callbacks, verify new callbacks are active. |
| AC-039 | `TypeRegistry` operations (encode/decode/string/validate) for an unregistered type produce a compile error (static_assert or requires clause). | Unit test: attempt to call `TypeRegistry::yaml_encode<UnregisteredType>(...)` and verify compilation fails. (Acceptable: if SFINAE is used, verify at runtime with error result.) |

## E2E Verification

- **Method**: A dedicated test file `component_registry_tests.cpp` (or equivalent) in `tests/` provides programmatic verification via Catch2 test cases. All acceptance criteria AC-001 through AC-039 are covered by automated tests. No manual E2E test is required — this is a library-level feature with no user-facing UI in this sprint.
- The test file runs as part of `buddd_tests` (the existing test executable). All tests pass under `ctest` or `./build/buddd_tests`.
- YAML round-trip tests verify in-memory serialization only — no file I/O. See AC-029.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All 5 existing engine components are registered and pass YAML round-trip within 1 sprint. |
| SC-002 | A new component type (not previously existing) can be registered with fewer than 20 lines of registration code. |
| SC-003 | Property enumeration for any registered component returns the correct number and types of properties (verified by unit tests). |
| SC-004 | `deserialize_component()` error paths cover: unknown component type, type mismatch, constraint violation. Unknown property names produce a warning (skip). All produce meaningful messages. |
| SC-005 | No changes to `component.h`, `entity.h`, `world.h`, or any existing component's public API. |
| SC-006 | A custom property type can be registered by external code via `TypeRegistry::register_type<T>()` and used in `add_property<T>()` without modifying engine source. |
| SC-007 | Built-in types (`float`, `int32_t`, `bool`, `std::string`, `Vec3`, `Vec4`, `Quat`, `std::shared_ptr<Model>`) work in `add_property<T>()` without any user registration call. |

## Edge cases

| # | Scenario | Expected behavior |
|---|---|---|
| EC-01 | Register the same component type name twice | WARNING logged, existing `ComponentInfo<T>&` returned (no error). |
| EC-02 | `create()` on a type that was registered but factory returns nullptr | Returns error result (not UB). |
| EC-03 | `describe()` on a type that was registered then deregistered (if deregistration is added) | Returns nullptr. |
| EC-04 | Property with no constraints (min/max absent) | Property accepts any value of the correct type. |
| EC-05 | `Vec3` with NaN or Inf components | Serialized as-is (IEEE 754). Deserialization passes through (no NaN check). |
| EC-06 | Empty YAML node passed to `deserialize_component()` | No properties updated; success result. |
| EC-07 | YAML node with null/undefined scalar (tilde `~`) for a required property | TypeRegistry decode returns error (type mismatch or missing value). |
| EC-08 | `Intensity` set to zero or negative value | Error: violates `min > 0` constraint on light component properties. |
| EC-09 | String property containing YAML special characters | Properly escaped by yaml-cpp emitter. |
| EC-10 | `MeshRenderer` with null model (`shared_ptr<Model>` is nullptr) | Serializes as empty string (via TypeRegistry encode). Deserializing empty string produces null `shared_ptr<Model>`. |
| EC-11 | `TypeRegistry::register_type<T>()` called after a component using type `T` has already been registered | Behavior is implementation-defined: callbacks are overwritten in the registry, but existing `Property` objects already captured the old callbacks. WARNING logged. (Recommendation: register custom types before component registration.) |
| EC-12 | Custom type producing invalid YAML (e.g., non-scalar where scalar expected, or missing required keys) | Error returned from `serialize_component()` or `deserialize_component()` with descriptive message. |
| EC-13 | `from_string` callback for a custom type receives user input that cannot be parsed | `Result<T>` error returned; editor UI receives the error message for display. |

## Error cases

| # | Condition | Error category | Error message contains |
|---|---|---|---|
| ER-01 | `create("unregistered_type")` | `InvalidArgument` | `"unregistered_type"` |
| ER-02 | `deserialize_component()` with unknown property key `"foo"` | Warning (logged, not an error) | Unknown key `"foo"` is skipped |
| ER-03 | `deserialize_component()` with string value for float property (TypeRegistry decode error) | `InvalidArgument` | Type name + property name |
| ER-04 | `deserialize_component()` with value below min constraint | `InvalidArgument` | `"out of range"` + property name |
| ER-05 | `deserialize_component()` with value above max constraint | `InvalidArgument` | `"out of range"` + property name |
| ER-06 | `TypeRegistry::yaml_decode<T>()` receives YAML of wrong type (e.g., mapping instead of scalar) | `InvalidArgument` | type name + `"expected scalar"` |
| ER-07 | `YAML::convert` for math type with missing required key (e.g., `Vec3` missing `z`) | `InvalidArgument` | `"missing key"` |
| ER-08 | yaml-cpp exception (`YAML::Exception` or subclass) thrown during `deserialize_component()` | `InvalidArgument` | Error message from yaml-cpp |
| ER-09 | `TypeRegistry::from_string<T>()` receives an unparseable string | `InvalidArgument` | `"cannot parse"` + type name |
| ER-10 | `TypeRegistry::validate<T>()` returns error for an invalid value | `InvalidArgument` | User-defined error message from validate callback |

## Permissions and security

**N/A.** This is a library-level metadata and serialization feature. It introduces no new network access, file system access (beyond what yaml-cpp already provides to the asset manager), user authentication, or privilege separation.

## Observability

| Aspect | Mechanism |
|---|---|
| Registration success/failure | Log at `DEBUG` level when each component type is registered: `"Registered component type: camera"`. `WARN` level on duplicate registration attempt: `"Component type 'camera' already registered"`. |
| TypeRegistry registration | Log at `DEBUG` level when each type is registered: `"Type registered: N5buddd5engine6Vec3E"` (mangled name). `WARN` on overwrite of existing type. |
| Factory creation success/failure | Log at `TRACE` level for each `create()` call. `ERROR` level if factory returns nullptr. |
| YAML deserialization errors | Log at `WARN` or `ERROR` level with the error message from the returned `Result`. |
| Unknown type queries | Log at `DEBUG` level for `describe()` or `create()` calls with unrecognized type names. |

All logging uses the existing `BUDDD_LOG_*` macros from `src/engine/log/`. The log tag is `"ComponentRegistry"`.

## Out of scope

Refer to the Non-goals section above. Additionally:

- No changes to `component.h`, `entity.h`, `world.h`, or existing component headers beyond what is strictly required for property registration (e.g., adding accessor methods if they don't exist — but CameraComponent already has getters, so no changes needed).
- No runtime component property editing in the editor (deferred to editor property panel spec).
- No hot-reload of component registrations.
- No schema evolution, migration, or versioning of serialized component data.
- No `PropertyType` enum — types are identified by C++ type + `std::type_index` at runtime.
- No `add_asset_ref()` method on `ComponentInfo<T>` — asset references use typed properties (e.g., `add_property<std::shared_ptr<Model>>()`). The asset-reference semantics are handled by the TypeRegistry callbacks for the asset type, not by the property system itself.

## Assumptions

| # | Assumption |
|---|---|
| A-01 | yaml-cpp is already a project dependency (confirmed: fetched in `src/engine/CMakeLists.txt`, linked via `yaml-cpp`). No new external dependencies are needed. |
| A-02 | The existing `Result<T>` / `Error` types (`src/engine/error.h`) are used for all error returns. No new error category is needed — `InvalidArgument` suffices. |
| A-03 | String-based type identification (not enum) is the agreed approach per coordination.md. |
| A-04 | The `register_all_components()` function is called during engine startup, after the registry is constructed and before any scene creation. The exact call site is determined during implementation but is expected to be in `EngineService` initialization or equivalent. |
| A-05 | `CameraComponent` already has the necessary public getter accessors (`fov_y()`, `aspect()`, `near_plane()`, `far_plane()`). `set_perspective()` serves as a combined setter. For serialization, the property setter may call `set_perspective()` and pass all 4 values, or individual setters may be added. The spec allows either approach — the contract is that round-trip preserves values. |
| A-06 | Light components (`PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`) currently expose mutable references via `colour() noexcept -> Vec3&`, `intensity() noexcept -> float&`. The property system uses explicit get/set lambdas. For the light components, the getter calls the const accessor and the setter assigns through the mutable accessor. |
| A-07 | `MeshRenderer` stores `shared_ptr<Model>`. The model property is registered via `add_property<std::shared_ptr<Model>>()` on `ComponentInfo<MeshRenderer>`, using simple getter/setter lambdas that return/set the shared pointer. The TypeRegistry handles asset ID ↔ Model conversion internally via `const SerializationContext&`: encode calls `ctx.assets.find_asset_id(model)` to produce an asset ID YAML string; decode calls `ctx.assets.resolve_model(id)` to resolve the asset ID back to a `shared_ptr<Model>`. `AssetManager::find_asset_id(const Model&)` and `AssetManager::resolve_model(const std::string&)` are new methods added to the engine. If the model is null, TypeRegistry encode produces an empty string; decoding an empty string produces a null `shared_ptr<Model>`. |
| A-08 | YAML::convert specializations for math types are defined in separate headers (`vec3_yaml.h`, `vec4_yaml.h`, `quat_yaml.h`) alongside their respective type headers, NOT in the primary header — to avoid forcing a yaml-cpp dependency on all math includes. |
| A-09 | The `PropertyValue` variant (holding `float`, `int32_t`, `bool`, `std::string`, `math::Vec3`, `math::Vec4`, `math::Quat`) exists only as an editor/UI-layer concept for efficient value access on built-in types. Custom types are displayed via `TypeRegistry::to_string<T>()` / `from_string<T>()`. The PropertyValue variant is NOT used in the serialization path. |
| A-10 | `ComponentRegistry` is populated once at startup (`register_all_components`) and is read-only thereafter. It is NOT thread-safe during registration, but read queries (`describe`, `create`, `all_types`) may be called from any thread after registration completes. |
| A-11 | `TypeRegistry` is populated once at startup (built-in types are pre-registered, then custom types are registered before component registration). It is NOT thread-safe during registration, but read queries (encode/decode/string/validate) may be called from any thread after registration completes. |
| A-12 | The internal `Property` type-erased callbacks capture the TypeRegistry callbacks at the time `add_property<PropType>()` is called. If a type is later re-registered in TypeRegistry, existing Property objects are NOT automatically updated. |
| A-13 | `TypeRegistry::register_type<T>()` for an already-registered type logs a WARNING and overwrites the existing entry. This applies to both built-in types (theoretically possible but not recommended) and custom types. |
| A-14 | `enum`-like properties use `int32_t` as the property type with `PropertyFlags::enum_choices` providing the string-name mapping. The YAML serialization stores the string name (from `enum_choices[index]`), not the numeric index. This is handled by the getter/setter and `PropertyFlags`, not by a dedicated `int32_t` TypeRegistry variant. |
| A-15 | Overload (A) (convention-based `add_property`) is part of the documented API surface — it produces a compile error saying "not implemented yet" if the accessors are not found. Implementation of the template metaprogramming detection is deferred for v1. Use overload (B) or (C) for all v1 component registrations. |

## Open questions

All open questions have been resolved on 2026-06-09:

1. **Duplicate registration behavior**: Logs WARNING and returns existing reference. Registration API returns `ComponentInfo<T>&`.
2. **Light component constraints**: `min > 0` enforced for `intensity`, `range`, `inner_angle`, `outer_angle`.
3. **MeshRenderer model**: `shared_ptr<Model>` registered as a built-in TypeRegistry type. TypeRegistry encode/decode uses `ctx.assets.find_asset_id()` / `ctx.assets.resolve_model()` internally. Component getter/setter operate on `shared_ptr<Model>` directly.
4. **Unknown YAML keys**: `deserialize_component()` logs a warning and skips unknown keys (forward-compatible).
5. **TypeRegistry vs PropertyType**: TypeRegistry replaces PropertyType enum. Types identified by `std::type_index`. External code can register custom types.
6. **PropertyValue role**: Demoted to UI-layer helper for built-in types. Custom types use `to_string`/`from_string`.
