# ADR-006: RTTI-based `dynamic_cast<T*>()` Component Dispatch

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The scene graph module (SPEC-008 / IMPL-008) needs a type-safe mechanism to retrieve, query, and remove polymorphic components attached to entities. Components derive from a common base class `Component` and are stored as `std::vector<std::unique_ptr<Component>>` per entity node.

The component API consists of three template operations on `World`:

- `get_component<T>()` — find and return a reference to component of type `T`, or `std::nullopt` if absent.
- `add_component<T>(args...)` — construct and store a new component of type `T`.
- `remove_component<T>()` — find and erase component of type `T`.

The `add_component` operation does not need dispatch (it knows the concrete type at construction time), but `get_component` and `remove_component` need a mechanism to identify which component in the per-entity vector matches the requested type `T`.

Several dispatch strategies were evaluated:

### Option 1: `dynamic_cast<T*>()` (chosen)

Use C++ RTTI via `dynamic_cast<T*>()` on each `Component*` in the entity's component vector until a match is found or the vector is exhausted.

```cpp
for (auto& c : node->components_) {
    auto* typed = dynamic_cast<T*>(c.get());
    if (typed) { return *typed; }
}
```

- **Pros**: Zero boilerplate in component types. No manual type registration. No modification needed to the `Component` base class (which has only a virtual destructor). Works with forward-declared types. Standard C++ with no external dependencies.
- **Cons**: Requires RTTI (`-frtti`), the default in most C++ compilers, but prevents building with `-fno-rtti`. O(n) linear scan per lookup. `dynamic_cast` has a small runtime cost (type info traversal).

### Option 2: Static type ID via virtual `type_id()` method

Add a virtual `type_id() -> TypeId` method to the `Component` base class, where `TypeId` is an enum or integer constant uniquely identifying each component type.

```cpp
class Component {
public:
    virtual ~Component() = default;
    virtual auto type_id() const noexcept -> TypeId = 0;
};
```

- **Pros**: No RTTI required. O(n) linear scan but with a cheaper integer comparison instead of `dynamic_cast`. Full control over type identity.
- **Cons**: Every component type must declare a `type_id()` override — more boilerplate. `TypeId` values must be managed (manual enum or compile-time counter). Adding a new component type requires registering a new ID. The `Component` base class becomes more opinionated.

### Option 3: Static type ID via compile-time type counter

Use a compile-time type counter (e.g., a CRTP helper that generates sequential IDs via `static inline` counters or `__COUNTER__` extensions).

```cpp
template<typename T>
auto type_id() noexcept -> uint32_t {
    static uint32_t id = next_id++;
    return id;
}
```

- **Pros**: No RTTI required. No manual ID registration — each unique type automatically gets a unique ID. Faster dispatch than `dynamic_cast`.
- **Cons**: ODR-sensitive — the same type across translation units may get different IDs unless inline variables are used with external linkage. Not standard C++ (relies on sequential static initialization ordering). Debugging is harder — type identity is an opaque integer rather than a type system feature.

### Option 4: Visitor / double dispatch

Use the visitor pattern on `Component`: each component type implements a `visit()` or `accept()` method.

- **Pros**: No RTTI. Compile-time safety (missing a visitor case is a compile error). Well-established pattern.
- **Cons**: Adding a new component type requires adding a new overload to every visitor — violates the Open/Closed principle. The `get_component<T>()` use case (find one specific type by template parameter) is not a natural fit for the visitor pattern, which works best when you want to perform an operation on *all* variants.

### Option 5: Type-erased function pointers per component

Store a type-erased function pointer alongside each component that can cast it to the right type.

```cpp
struct ComponentSlot {
    std::unique_ptr<Component> component;
    bool (*is_type)(Component*) = nullptr;  // type-erased type check
};
```

- **Pros**: No RTTI. O(1) type check per slot (function pointer call).
- **Cons**: Adds storage overhead per component slot. Template machinery needed to generate the `is_type` function. More complex than `dynamic_cast` with no clear benefit for the project's scale.

## Decision

**We use `dynamic_cast<T*>()` for component dispatch in the scene graph module.**

This applies to the three dispatch operations in `World`:

- `World::get_component<T>()` — iterates the component vector and returns `dynamic_cast<T*>(c.get())` for the first match.
- `World::get_component<T>() const` — same for const access.
- `World::remove_component<T>()` — iterates the component vector and erases the first element where `dynamic_cast<T*>(it->get())` is non-null.

### Rationale

1. **Zero boilerplate in component types**: A component is defined with just `struct MyComp : Component { ... };` — no type ID registration, no virtual method overrides, no macros. The `Component` base class remains minimal (virtual destructor only).

2. **Established pattern in C++ game engines**: `dynamic_cast` for polymorphic component lookup is a well-known pattern in C++ game and application frameworks. It prioritises developer ergonomics over peak dispatch performance, which is appropriate for a v1 scene graph where entities typically have < 10 components.

3. **Flexibility for forward-declared and template component types**: `dynamic_cast` works with any polymorphic type without requiring the type to be registered in a central enum. This supports modular component libraries where component types may come from different translation units.

4. **No base class pollution**: The `Component` base class stays minimal (virtual destructor only). Adding a `type_id()` virtual method would make the base class more opinionated and force every future component type to implement it.

### Performance characteristics

- `get_component<T>()` and `remove_component<T>()` are **O(n)** in the number of components on the entity.
- `dynamic_cast` overhead per element is small — typically a few dozen instructions (type info pointer comparison + offset adjustment on match).
- For the v1 scene graph with < 10 components per entity, the linear scan is not a performance concern. Future optimisations (ECS flat arrays, archetype-based storage) will replace the dispatch mechanism entirely.

### RTTI requirement

This decision requires C++ RTTI to be enabled (`-frtti`, which is the default in GCC and Clang). Building with `-fno-rtti` will cause compilation errors in the scene graph module's `get_component<T>()` and `remove_component<T>()` methods.

### Future migration

If `-fno-rtti` support becomes necessary (e.g., for embedded targets, console SDKs, or code size optimisation), the dispatch should be replaced with a static type ID pattern. The most promising path is Option 2 (virtual `type_id()` method) because:

- It is standard C++ with no compiler extensions.
- It does not rely on ODR-sensitive static counters.
- It enables faster dispatch (integer comparison instead of `dynamic_cast`).
- The `Component` base class can be extended with the virtual method without breaking existing component types.

The migration would be:
1. Add `virtual auto type_id() const noexcept -> TypeId = 0;` to `Component`.
2. Add a `TypeId` type (enum or `uint32_t`) and a CRTP helper to auto-implement `type_id()` in derived types.
3. Replace `dynamic_cast<T*>(c.get())` with `c->type_id() == T::kTypeId` followed by `static_cast<T*>(c.get())`.
4. Update all component types to inherit from the CRTP helper.

This migration is backward-compatible: the `Component` base class interface changes, but no scene graph API signatures change.

## Consequences

### Positive

- **Minimal component authoring friction**: New component types require only `struct Foo : Component { ... };` — no type ID plumbing.
- **No external dependencies**: Uses standard C++ `dynamic_cast` — no new libraries or build system changes.
- **Simple implementation**: The dispatch code is three lines per method.
- **Flexible**: Works with component types from any translation unit, including template types and forward-declared types.
- **Consistent with common practice**: Many C++ game engines start with `dynamic_cast`-based dispatch before migrating to static type IDs or ECS patterns when performance demands it.

### Negative

- **RTTI permanently enabled**: The entire project (or at minimum the scene graph module) must be compiled with `-frtti`. This may be a constraint for targets where RTTI is typically disabled (embedded systems, console SDKs, code-size-optimised builds).
- **O(n) dispatch**: Linear scan per component lookup is slower than O(1) alternatives (static type ID map, type-erased function pointer), though acceptable for v1 with few components per entity.
- **`dynamic_cast` overhead**: Each `dynamic_cast` traverses the inheritance chain at runtime — a small but non-zero cost compared to integer comparison.
- **No compile-time type safety**: Unlike the visitor pattern, there is no compiler enforcement that a component type is handled. Missing a component type in a dispatch is a runtime no-op (the linear scan just skips it).
- **Harder to index**: The `dynamic_cast` approach does not naturally support a precomputed type-to-component map, which would be needed for O(1) lookup. Such an optimisation would require a different dispatch mechanism.

### Precedent

This decision does **not** establish a project-wide requirement for RTTI in all modules. Modules that do not interact with the scene graph's component dispatch can still be compiled with `-fno-rtti` if needed. However, any module that includes scene graph headers and calls `get_component<T>()` or `remove_component<T>()` at runtime depends on RTTI.

This decision also does **not** preclude a future migration to a static type ID pattern. The `dynamic_cast` dispatch is encapsulated within the `World` template methods — a migration would only change those methods and the `Component` base class, not the public API.

## References

- SPEC-008 / IMPL-008 (`docs/specs/scene-graph/`): Scene graph specification and implementation contract.
- ADR-001 (`docs/adr/001-result-error-pattern.md`): Project-wide `Result<T>` / `Error` pattern (this ADR does not modify or create exceptions to ADR-001).
- ADR-005 (`docs/adr/005-optional-ref-component-api.md`): `std::optional<T&>` for component lookup API (the return type used by `get_component<T>()`).
- `src/engine/scene/world.h`: Canonical implementation — `dynamic_cast<T*>()` at lines 114, 129, 142.
- `src/engine/scene/component.h`: The `Component` base class (virtual destructor only).
