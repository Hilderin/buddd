# ADR-010: Raw Pointers Prohibited in Public API Signatures

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The Buddd Engine codebase currently has no formal rule about raw pointers in public API signatures. As the codebase grows, raw pointer parameters and return types create ambiguity about ownership, nullability, and lifetime, and are a common source of bugs.

The absence of a clear convention means each developer independently decides whether to use `T*`, `T&`, `std::optional<T&>`, `std::reference_wrapper<T>`, or another mechanism — leading to inconsistency across modules and cognitive overhead during code review.

The recent scene-rendering work (SPEC-011 / IMPL-011) highlighted this issue directly. The initial design used `CameraComponent*` as a return type. During review, the team decided to use `std::optional<std::reference_wrapper<T>>` instead of `T*` for nullable references, and to avoid raw pointers in all public API. The review discussion revealed that without a documented convention, each developer spends time re-litigating the same trade-offs.

Several common patterns in the codebase are affected:

1. **Nullable return values from lookup functions** — e.g., `get_component<T>()` returning `T*` or `nullptr`. ADR-005 already established `std::optional<T&>` for this case in the scene graph module, but the pattern has not been adopted project-wide.

2. **Nullable input parameters** — functions that accept an optional object, currently taking `T*` with the convention that `nullptr` means "not provided".

3. **Output parameters** — functions that modify an object through a pointer parameter, e.g., `void load_model(const char* path, Model* out)`.

4. **Cross-references to container-managed objects** — using raw pointers to refer to entities or components stored in a `std::vector` or similar container, where the pointer may dangle after container reallocation.

5. **Contiguous range parameters** — functions that accept `T* data, size_t count` to pass an array or buffer.

6. **C string parameters** — `const char*` for file paths, error messages, and platform interop.

## Decision

**Raw pointers (`T*` and `T* const`) must not appear in public API function signatures (parameters or return types), except in explicitly documented cases listed under Exceptions below.**

This applies to all headers shipped as part of the engine's public interface: `include/`, `src/engine/` headers that are included across modules, and any header that forms part of a module's documented API surface. It does **not** apply to strictly private implementation details within `.cpp` files or anonymous namespaces.

### Replacement mappings

| Raw pointer pattern | Replacement | When to use |
|---|---|---|
| `T*` (nullable, return or parameter) | `std::optional<T&>` (C++26) | A reference that may be absent. Clearer than the `nullptr` convention because the optional nature is explicit in the type. See ADR-005 for compiler support. |
| `T*` (guaranteed non-null, stored/rebound) | `std::reference_wrapper<T>` | A reference that is always valid and needs to be stored in a container, reassigned, or passed through a template. |
| `T*` (guaranteed non-null, parameter only) | `T&` | A reference that is always valid and is not stored beyond the function call. |
| `T*` (cross-reference to container-managed object) | `EntityId`, `ComponentHandle`, or similar typed identifier | An opaque handle that the caller can use to look up the object through the owning container. Avoids dangling pointer bugs on container reallocation. |
| `T* data, size_t count` | `std::span<T>` | A contiguous range of elements that may be empty. Self-documenting — the size is part of the type. |
| `const char*` (string parameter) | `std::string_view` | A non-null string parameter. Does not assume null-termination and works with `std::string`, string literals, and string views alike. |
| `T* out` (output parameter) | `T&` or `std::unique_ptr<T>&` | A `T&` when the function modifies an existing object; `std::unique_ptr<T>&` when the function transfers ownership or allocates a new object. |

### Exceptions

The following cases are **exempt** from the rule:

1. **`const char*` for C string literal interop** — error messages, file paths in platform APIs, and any interface where a null-terminated string is required by an external C API. Prefer `std::string_view` in engine-internal APIs; reserve `const char*` for the boundary with platform or third-party C libraries.

2. **Legacy C interop in the platform abstraction layer** — files under `src/engine/platform/` that wrap C APIs (e.g., SDL3, Vulkan, POSIX). The platform layer's public headers may expose C-compatible signatures that use raw pointers. These must be wrapped by a C++ type-safe layer before reaching higher-level engine code.

3. **Non-owning observer pointers in strictly private implementation** — raw pointers may be used within a `.cpp` file, a function-local scope, or a private inner class that is not exposed in any public header. These are implementation details and are not part of the public API.

4. **Callback contexts passed as `void*` in C-style callback registration** — when registering a callback with a C API that accepts a `void* user_data` parameter, the `void*` is unavoidable at the boundary. The C++ wrapper around such an API **must** encapsulate the `void*` cast and expose a typed API (e.g., a `std::function` or typed callback) to all callers above the platform layer.

### Rationale

1. **Clearer ownership semantics at the API boundary**. A `T&` parameter says "I borrow this, it must be valid." A `std::optional<T&>` return says "I may or may not have a value." A `std::span<T>` parameter says "I need a contiguous range of elements." Raw pointers say nothing — they could mean any of these, and the caller must read the documentation (or the implementation) to disambiguate.

2. **Eliminates ambiguity between "nullable" and "non-null"**. In a codebase with no pointer convention, every `T*` parameter or return value creates a question: "Can this be null? Do I need to check before dereferencing?" Using `T&` (guaranteed non-null) and `std::optional<T&>` (explicitly nullable) makes the contract visible in the type signature with no documentation required.

3. **Self-documenting APIs**. A developer reading `void render_scene(const Camera& cam, std::span<const Light> lights)` knows immediately that `cam` is required and `lights` may be empty. The original `void render_scene(const Camera* cam, const Light* lights, size_t light_count)` leaves both cases ambiguous.

4. **Catches null-dereference bugs at access point rather than at storage point**. When a `std::optional<T&>` is accessed without checking, the standard library provides defined behaviour (an exception or assertion) rather than undefined behaviour. When a `std::reference_wrapper<T>` is default-constructed and later accessed, the reference is still valid (it was never null). Raw pointer null-dereferences are undefined behaviour — the compiler may optimise away the null check, or the program may crash far from the root cause.

5. **Consistency with ADR-005**. ADR-005 already established `std::optional<T&>` for component lookup in the scene graph module. This ADR extends that principle to the entire public API surface.

## Consequences

### Positive

- **Clear ownership semantics**: Every public API signature explicitly conveys nullability, ownership, and lifetime expectations through its types.
- **Reduced cognitive overhead**: Developers and reviewers no longer need to ask "can this be null?" for every `T*` parameter — the type system answers the question.
- **Safer by default**: `std::optional<T&>` and `std::reference_wrapper<T>` eliminate the class of bugs caused by null pointer dereference in public API boundaries.
- **Consistent with modern C++ practice**: The rule aligns with the C++ Core Guidelines (ES.47, F.7, F.16, F.21, F.22) and the project's commitment to modern C++ idioms (see ADR-001, ADR-005).
- **Gradual adoption**: Existing code is not required to change immediately; the rule applies to new code and major refactors. The wiki documents the convention for new contributions.

### Negative

- **Slightly more verbose syntax**: `cam_opt->method()` vs `cam_ptr->method()` adds `.has_value()` checks at the call site when the optional is used without a prior check. This is most noticeable in hot code paths with frequent optional access.
- **`std::optional<T&>` requires C++26**: This feature requires GCC 16+, Clang 22+, or MSVC 2025+ (see ADR-001 and ADR-005). No fallback to `std::optional<std::reference_wrapper<T>>` is provided — the project targets C++26 unconditionally.
- **`std::span` requires care with temporaries**: `std::span` does not own its data. Passing a `std::span` that points to a temporary vector can create a dangling span. Callers must ensure the backing storage outlives the span.
- **`std::reference_wrapper` comparison surprises**: `std::reference_wrapper<T>` is not equality-comparable with `T&` in all contexts. Comparisons between `std::reference_wrapper` instances compare the referents, which may differ from pointer comparison semantics.
- **Migration effort**: Existing code that uses raw pointers in public APIs must be refactored. This is not required immediately but should be done opportunistically when touching those APIs for other reasons.

### Compliance

- All new public header files SHALL comply with this rule.
- Existing public headers MAY be refactored to comply as part of regular maintenance.
- Code review SHALL flag raw pointers in public API signatures unless they fall under one of the documented exceptions.
- The `docs/wiki/architecture.md` SHALL be updated to document this convention (see ADR-005, consequence regarding wiki update).

## Related documents

- ADR-001 (`docs/adr/001-result-error-pattern.md`): Project-wide `Result<T>` / `Error` pattern — established the C++26 compiler baseline that enables `std::optional<T&>`.
- ADR-005 (`docs/adr/005-optional-ref-component-api.md`): `std::optional<T&>` for component lookup API — precedent for the nullable-reference replacement pattern.
- SPEC-011 / IMPL-011: Scene Rendering — the work that surfaced the need for this ADR.
- C++ Core Guidelines:
  - ES.47: Use `T*` rather than `T&` if you must use a pointer and may need to rebind it.
  - F.7: For general use, take `T*` arguments only if null is valid.
  - F.16: For input parameters, prefer "in" values over "in/out" pointers.
  - F.21: To return multiple "out" values, prefer returning a struct or tuple.
  - F.22: Use `T*` to designate a non-owning position (this ADR narrows this to private implementation only).
