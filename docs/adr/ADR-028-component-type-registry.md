# ADR-028: Component Type Registry with TypeRegistry and SerializationContext

## Status

Accepted

## Context

The engine has five concrete `Component` subclasses (`CameraComponent`, `PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`, `MeshRenderer`) but no uniform way to discover component types at runtime, create instances by string name, iterate properties for inspection, or serialize/deserialize component data to/from YAML.

Previously, serialization required a rigid `PropertyType` enum approach that had to be modified every time a new type was added, making the system closed to extension by external code. External game code could not register custom property types without modifying the engine.

Several upcoming features depend on component metadata and serialization:

- **Scene save/load** — must serialize and deserialize component state without compile-time type knowledge.
- **Editor property panel** — must enumerate component properties (name, type, value) for generic UI.
- **Prefabs** — must clone component state by name.

C++ has no native reflection mechanism, so a manual registration system is necessary. yaml-cpp is already a project dependency (established in ADR-016), making it the natural serialization format.

The engine may be used as a library by external projects that define custom `Component` subclasses and custom property types. The system must be extensible without modifying engine source code.

## Decision

We implement a **TypeRegistry-based component property system** with the following architectural decisions:

1. **TypeRegistry** — A static registry that maps each C++ type (identified by `std::type_index`) to five callbacks: `yaml_encode`, `yaml_decode`, `to_string`, `from_string`, and `validate`. All callbacks receive a `const SerializationContext&` parameter for context-dependent operations (e.g., asset ID resolution during YAML I/O).

2. **SerializationContext** — A lightweight struct carrying an `AssetManager&` reference, passed to all TypeRegistry callbacks and serialization helpers. This enables asset-reference types (like `shared_ptr<Model>`) to resolve asset IDs without coupling the property system to the asset subsystem.

3. **Property** (internal, not user-facing) — A descriptor containing a human-readable name, a `std::type_index` identifying the property type, type-erased getter/setter lambdas, and serialization/validation callbacks that delegate to TypeRegistry. The class is an implementation detail — users never interact with it directly.

4. **ComponentInfoBase** — A type-erased base class holding a canonical string name (e.g. `"camera"`), a factory function `() -> unique_ptr<Component>`, and a vector of `Property` descriptors.

5. **ComponentInfo<T>** — A typed template class deriving from `ComponentInfoBase`. Provides three overloads of `add_property<PropType>()`:
   - **(A) Convention-based** — auto-detects `get_<name>()` / `set_<name>()` accessors at compile time. Deferred for v1 (produces a compile-time "not implemented" diagnostic).
   - **(B) Simple lambdas** — getter/setter without `SerializationContext`.
   - **(C) Context-aware lambdas** — getter/setter that receive `const SerializationContext&`.

6. **ComponentRegistry** — A class with methods: `register_component<T>(string_name) -> ComponentInfo<T>&`, `create(string_name) -> Result<unique_ptr<Component>>`, `describe(string_name) -> const ComponentInfoBase*`, `all_types() -> span<const ComponentInfoBase*>`.

7. **Eight built-in types pre-registered** in TypeRegistry: `float`, `int32_t`, `bool`, `std::string`, `Vec3`, `Vec4`, `Quat`, `std::shared_ptr<Model>`. External code registers custom types via `TypeRegistry::register_type<T>()`.

8. **Free function serialization helpers** — `serialize_component()` and `deserialize_component()` iterate a component's properties, delegating to TypeRegistry for type-specific encoding/decoding. Unknown YAML keys produce a warning (forward-compatible skip). Type mismatches and constraint violations produce errors.

## Alternatives considered

1. **PropertyType enum + PropertyValue variant** — A closed enum of supported types with a `std::variant`-based `PropertyValue`. Every new type required modifying the enum and variant definition, making the system closed to external extension. External projects could not register custom property types without forking the engine.

2. **YAML lambdas on every property** — Each property defines its own `to_yaml`/`from_yaml` lambdas directly, without a shared TypeRegistry. This avoids the abstraction but forces repetitive serialization code for common types (every `float` property would duplicate float-to-YAML logic). It also makes it impossible for the editor to display custom types without the registration function including display logic.

3. **Compile-time reflection via macros** — Using `X_MACRO` or code-generation macros to enumerate types and generate serialization code. Fragile, hard to debug, and not extensible from external code without the header being available at generation time. Macro-based solutions also pollute the global namespace and make IDE tooling more difficult.

4. **Chosen: TypeRegistry with typed `add_property<PropType>()`** — Balances type safety (typed template getters/setters prevent mismatch errors at compile time), extensibility (external code registers any type), and separation of concerns (serialization/display behaviors are defined once per type, not per property).

## Consequences

### Positive

- **Fully extensible from external code** — External projects call `TypeRegistry::register_type<T>()` and then use `add_property<T>()` with the same API as built-in types. No engine modification needed.
- **Type-safe property accessors** — Template getter/setter lambdas prevent type mismatches. No `static_cast` in user code.
- **Separation of concerns** — Serialization (YAML encode/decode) and display (to_string/from_string) behaviors are defined once per type in TypeRegistry, not duplicated per property.
- **Serialization context** — The `SerializationContext` parameter enables context-dependent operations (like asset ID resolution) without coupling the property system to the asset subsystem.
- **Forward-compatible deserialization** — Unknown YAML keys produce a warning and are skipped, allowing scene files from future engine versions to be partially loaded.
- **No changes to existing component API** — The `Component` base class is not modified. Existing component getters/setters are used through lambdas.

### Negative

- **Boilerplate for custom type registration** — Each custom type requires five callbacks (yaml_encode, yaml_decode, to_string, from_string, validate). However, this is a one-time cost per type, not per property.
- **Template metaprogramming complexity** — The `ComponentInfo<T>` internals use type erasure and template deduction, increasing compilation time and debugging difficulty.
- **Runtime check for unregistered types** — Using an unregistered type in `add_property<PropType>()` produces a runtime error (not compile-time), because the TypeRegistry lookup happens via `std::type_index` at runtime.

### Neutral

- **yaml-cpp becomes a wider dependency** — Template methods in `ComponentInfo<T>` and `TypeRegistry` headers require `<yaml-cpp/yaml.h>` to be included from public headers (necessary for template inline definitions), widening the yaml-cpp dependency from the ADR-016 baseline.

## Related documents

- **ADR-006** — RTTI-based `dynamic_cast<T*>()` dispatch for component queries (the runtime component querying strategy that this system complements).
- **ADR-016** — yaml-cpp dependency for asset metadata (established yaml-cpp as the project's YAML library).
- **ADR-027** — Editor architecture (the editor's property panel will consume `ComponentRegistry::describe()` and TypeRegistry display callbacks).
