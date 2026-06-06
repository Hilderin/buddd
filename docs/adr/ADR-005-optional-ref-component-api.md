# ADR-005: `std::optional<T&>` for Component Lookup API

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The scene graph module (SPEC-008 / IMPL-008) needs a type-safe way to return an optional reference to a component attached to an entity. The primary use case is `entity.get_component<T>()`, which returns a reference to the component of type `T` if it exists, or a sentinel "not found" value if it does not.

Several options were considered:

1. **Raw pointer `T*`** — return `nullptr` when the component is absent. Simple and familiar in C and C++ game engines, but loses type safety — the caller must remember to check for `nullptr`, and there is no semantic distinction between "optional reference" and "nullable pointer" at the type system level.

2. **`std::optional<T&>`** (C++26, P2988R5) — a type-safe optional reference that clearly expresses "may or may not contain a value" in the function signature. Supports the full `std::optional` API (`has_value()`, `value()`, `operator->`, `operator*`, comparison with `std::nullopt`).

3. **`std::optional<std::reference_wrapper<T>>`** — works with C++17 and later, but requires awkward double-dereference (`opt->get()` or `(*opt).get()`) and does not benefit from the ergonomic improvements of `std::optional<T&>`.

4. **Custom `OptionalRef<T>` wrapper** — full control but adds maintenance burden and is unnecessary when the standard library provides the feature.

## Decision

We use `std::optional<T&>` as the return type for `get_component<T>()` in the scene graph module, and adopt it as the project-wide pattern for any future API that needs type-safe optional reference semantics.

### Rationale

- **Compiler support is present**: GCC 16+ and Clang 22+ have full support for `std::optional<T&>`. This is consistent with the project's C++26 minimum compiler baseline (see ADR-001).

- **Type safety**: Unlike raw pointers, `std::optional<T&>` makes the optional nature of the return value explicit in the type system. The compiler can warn on unused optionals (with `-Wunused-result`), and the caller cannot accidentally dereference a null pointer.

- **Ergonomics**: `std::optional<T&>` supports the same API as `std::optional<T>` — `has_value()`, `value()`, `operator->`, `operator*`, and `operator bool`. Callers write `if (auto opt = entity.get_component<Health>()) { opt->hp; }` — clear and concise.

- **Consistency**: Using `std::optional<T&>` aligns with the project's principle (stated in ADR-001) of adopting modern C++ standard library features rather than reinventing them.

### When not to use

- If compiler support for `std::optional<T&>` is not available (pre-GCC 16, pre-Clang 22), fall back to `T*` with `nullptr` for absent.
- For optional value semantics (not references), use `std::optional<T>`.

## Consequences

- All scene graph `get_component<T>()` methods return `std::optional<T&>` or `std::optional<const T&>`.
- Future engine APIs that need optional reference semantics should follow this pattern.
- The `docs/wiki/architecture.md` should be updated to document this convention.
- No changes needed for existing modules — this is a forward-looking convention for new code.

## Related

- ADR-001: Project-wide `Result<T>` / `Error` Pattern — established the project's C++26 compiler baseline.
- SPEC-008 / IMPL-008: Scene Graph — the first module to use this pattern.
