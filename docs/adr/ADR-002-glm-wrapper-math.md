# ADR-002: GLM Wrapper Pattern for Math Types

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The Buddd Engine needs fundamental linear algebra types: vectors, matrices, quaternions, and a camera abstraction. These types are required by every subsystem that deals with spatial computations — rendering, transform hierarchies, physics, and audio spatialisation.

Before this decision, the project had no math library at all. The choices were:

1. **Use GLM directly** throughout the engine codebase, accepting its C-style free-function API (`glm::normalize(v)`) and GLSL-influenced naming.
2. **Hand-roll** a custom math library with no external dependency.
3. **Wrap GLM** in project-namespaced types with an object-oriented API (`v.normalized()`) and zero-overhead interop.

The decision was made concretely in SPEC-004 (Math Foundations), which introduced thin wrapper types (`Vec2`, `Vec3`, `Vec4`, `Mat4`, `Quat`) around GLM types using `reinterpret_cast` for ABI compatibility. This ADR records the architectural rationale, the alternatives considered, and the binding consequences.

### Factors influencing the decision

- **Project coding style**: The existing engine code (Platform, Window, RenderDevice) uses PascalCase types, trailing return types, and object-oriented method syntax (`platform.create_window()`). Using raw GLM free functions (`glm::normalize`, `glm::cross`) would introduce a different calling convention inconsistent with the rest of the codebase.
- **Architecture boundary**: The project constitution requires a hard boundary between engine internals and external dependencies. GLM must not leak into headers outside `src/engine/`. Wrapping GLM inside project types makes this boundary explicit and enforceable by convention.
- **Zero-overhead requirement**: Game engine math is performance-critical. The wrapper must not add runtime overhead — no heap allocation, no virtual dispatch, no data copying, no layout conversion.
- **OpenGL interop**: `Mat4` values are passed directly to `glUniformMatrix4fv`. The memory layout must match GLM's column-major layout byte-for-byte.
- **C++26 availability**: `reinterpret_cast`-based interop can be statically guarded with `static_assert(std::is_standard_layout_v)`, `static_assert(sizeof(T) == sizeof(GLMType))`, and `static_assert(std::is_trivially_copyable_v<T>)`, all of which are available in C++26 and check layout compatibility at compile time.

## Decision

We adopt a **thin GLM wrapper pattern** for all math primitives:

1. Each wrapper type (`Vec2`, `Vec3`, `Vec4`, `Mat4`, `Quat`) is a **standard-layout struct** with the **same public member layout** as its corresponding GLM type — same number of members, same types, same order.
2. A `.glm()` accessor returns a `reinterpret_cast` reference to the underlying GLM type: `auto glm() noexcept -> glm::vec3& { return reinterpret_cast<glm::vec3&>(*this); }`.
3. Every method delegates to the equivalent GLM function, either directly (using `.glm()` to pass to GLM) or by component arithmetic for trivial operations.
4. Every wrapper type is compile-time verified with three `static_assert` declarations:
   - `std::is_standard_layout_v<T>` — guarantees that the object's memory layout follows a predictable order matching the C struct layout rules, which is required for `reinterpret_cast` between types to be valid.
   - `sizeof(T) == sizeof(GLMType)` — guarantees identical storage size.
   - `std::is_trivially_copyable_v<T>` — guarantees memcpy-safe and enables passing by value in registers.
5. **No GLM headers are included outside `src/engine/math/`**. The wrappers are the sole public API for math.
6. The convenience accessor `.glm()` is the official interop path for code that needs to pass data to GLM or OpenGL directly.
7. The `Camera` type does **not** follow the wrapper pattern — it is a user-defined class that *uses* the wrapper types. There is no `glm::camera` to wrap.

### Why `reinterpret_cast` and not composition

A composition-based wrapper would contain a private `glm::vec3` member and expose it via accessors:

```cpp
struct Vec3 {
    glm::vec3 data;  // private member, composited
    auto normalized() const -> Vec3 { return Vec3{glm::normalize(data)}; }
};
```

This works but has downsides:
- Every method must go through the member access, adding syntactic noise.
- Aggregate initialisation is not possible (`Vec3{1,2,3}` requires a constructor).
- The memory layout is still identical (`static_assert` would still pass) but the API is less ergonomic.
- No benefit over `reinterpret_cast` — the layout is the same either way.

The `reinterpret_cast` approach preserves the same public member layout as GLM, enabling direct aggregate initialisation, direct field access (`v.x = 1.0f`), and zero-overhead GLM interop with a single cast. The casts are safe because of the triple `static_assert` guarantee.

## Alternatives considered

### Use GLM directly throughout the codebase

- **Pros**: No wrapper code to write or maintain; GLM is mature and well-tested; immediate access to all GLM features including obscure matrix operations.
- **Cons**: GLM's free-function API (`glm::normalize(v)`) is inconsistent with the project's OO style (`v.normalized()`). GLM headers would be transitively included by every engine header, making the architecture boundary impossible to enforce. GLM naming conventions (`glm::vec3`, `glm::mat4`) differ from the project's PascalCase style (`Vec3`, `Mat4`). If the project later needed to switch math libraries, every file in the codebase would need changes.
- **Verdict**: Rejected. Violates the architecture boundary principle and the project's coding conventions. The wrapper cost is small (one-time implementation of ~600 lines of header code).

### Hand-rolled custom math library

- **Pros**: No external dependency; complete control over API and implementation; no license concerns; can be optimised for exact project needs.
- **Cons**: Huge implementation and testing burden; matrix inversion, SVD, quaternion slerp, perspective projection, and look-at matrices are non-trivial to implement correctly; would need ongoing maintenance for edge cases and numerical stability; no community testing.
- **Verdict**: Rejected. Would cost significantly more development time than wrapping GLM, with higher risk of numerical bugs. GLM is a well-tested, header-only library with a permissive license.

### Use Eigen instead of GLM

- **Pros**: Eigen is well-known, well-documented, and widely used in graphics and robotics; supports fixed-size vectors and matrices with SIMD optimisation.
- **Cons**: Eigen is large (many headers, complex template metaprogramming); its ABI is not trivially compatible with OpenGL (Eigen defaults to column-major but has different alignment requirements); Eigen's expression template system can produce confusing compiler errors; Eigen has a more restrictive license (MPL2 with some GPL components).
- **Verdict**: Rejected. GLM is simpler, header-only with minimal template complexity, has a permissive (MIT) license, and is purpose-built for OpenGL graphics. It is the de-facto standard for OpenGL math in C++.

### Use DirectXMath

- **Pros**: High-performance SIMD math from Microsoft; used by many game engines; supports both Windows and Xbox.
- **Cons**: Windows-only; uses different coordinate conventions (left-handed) than the project's OpenGL convention (right-handed); requires a compiled library component; not available on Linux without translation layers.
- **Verdict**: Rejected. The project targets cross-platform (Linux and potentially macOS); DirectXMath is not portable.

### Inheritance-based wrapper (private inheritance from GLM types)

- **Pros**: Inherits GLM's memory layout automatically; can add methods without `reinterpret_cast`.
- **Cons**: Private inheritance in C++ is not standard-layout, breaking the ABI guarantee and `reinterpret_cast` safety; GLM types are not designed for inheritance (no virtual destructor); derived types cannot be used where base types are expected; accessing the base class requires an explicit cast anyway.
- **Verdict**: Rejected. Private inheritance breaks the `std::is_standard_layout_v` guarantee, which is the foundation of the zero-overhead interop claim.

### Use `std::bit_cast` instead of `reinterpret_cast`

- **Pros**: `std::bit_cast` is constexpr-friendly (since C++20/C++26) and avoids strict-aliasing concerns in principle.
- **Cons**: `std::bit_cast` requires trivially copyable types (satisfied here) and identical sizes (checked). However, `std::bit_cast` returns by value — it copies the object — making it unsuitable for the `.glm()` accessor which must return a *reference* to enable mutation of the underlying GLM state and to pass `&m.glm()` to OpenGL functions. `reinterpret_cast` is the only standard mechanism for zero-overhead type punning to a reference.
- **Verdict**: Rejected for the reference accessor. `std::bit_cast` is not a replacement for `reinterpret_cast` when a reference is required. The `static_assert` guards make the `reinterpret_cast` safe.

## Consequences

### Positive

- **Zero runtime overhead**: Every wrapper method is a single inline delegation to the equivalent GLM function. No branching, allocation, vtable dispatch, or data copying. The generated code is identical to raw GLM usage.
- **Clean public API**: Engine code reads as `v.normalized()` and `Mat4::perspective(...)` rather than `glm::normalize(v)` and `glm::perspective(...)`. This is consistent with the rest of the codebase.
- **Architecture boundary enforced**: GLM headers are included only in `src/engine/math/`. Engine consumers (outside `src/engine/`) never see GLM. Switching math libraries later would only require changing the wrapper implementations.
- **OpenGL interop**: `Mat4` has the exact same layout as `glm::mat4` (column-major, 4 `Vec4` columns), making it directly compatible with `glUniformMatrix4fv` with `GL_FALSE`. No transpose or copy needed.
- **Safety verified at compile time**: The triple `static_assert` on each wrapper type ensures that any change to the struct layout that breaks ABI compatibility is caught immediately.
- **Header-only primitives**: Vec2, Vec3, Vec4, Mat4, Quat have no `.cpp` files. Only Camera needs a translation unit (because its methods involve non-trivial logic). This simplifies build system management (the existing `GLOB_RECURSE` picks them up automatically).
- **Consistent with ADR-001**: Math functions are pure computation with no error paths, so `Result<T>` is not used — consistent with ADR-001's "Where this does not apply" clause.

### Negative

- **`reinterpret_cast` in public API**: The `.glm()` accessor exposes a `reinterpret_cast`-based conversion. While safe due to `static_assert` guards, it may trigger warnings in some static analysis tools or raise eyebrows in code review. Mitigation: the technique is well-documented and the safety invariants are explicit.
- **GLM version coupling**: The wrappers are tied to GLM's internal layout. If a future GLM version changes the layout of `glm::quat` (w, x, y, z) or `glm::vec4` (x, y, z, w), the `static_assert` checks will catch the breakage, but the wrapper code will need updating. This is acceptable because GLM has maintained stable layout for these types across many versions.
- **Not all operations are `constexpr`**: GLM does not support `constexpr` for `inverse`, `determinant`, `perspective`, `ortho`, `lookAt`, `rotate`, `scale`, or `slerp`. The wrappers inherit this limitation. This is a minor ergonomic loss — these operations cannot be used in compile-time contexts.
- **No error handling for singular inputs**: `normalize()` on zero-length vectors returns NaN; `inverse()` on singular matrices returns NaN/inf. The wrappers do not detect or handle these cases, matching GLM's behaviour. Callers must guard against degenerate inputs. This is a deliberate trade-off to maintain zero overhead.
- **Camera is not a wrapper**: The `Camera` class lives in `src/engine/math/` but follows a different pattern (plain class, not GLM wrapper, has a `.cpp` file). This inconsistency within the same module may confuse developers. Mitigation: the rationale is clear — there is no `glm::camera` type to wrap — and the Camera uses the wrapper types internally.

### Precedent

This wrapper pattern establishes a template for any future dependency wrapping in the project:
- Expose project-namespaced types in the public API.
- Hide the external dependency inside implementation files or wrapper headers.
- Use `reinterpret_cast` only when identical layout can be verified by `static_assert`.
- Keep the wrapper thin — each method delegates to the underlying library.
- Provide an explicit interop accessor (`.glm()`, `.sdl()`, etc.) for code that needs the raw type.

## References

- SPEC-004 (Math Foundations): Authoritative specification for the wrapper types' API and behaviour.
- IMPL-004 (Math Foundations Implementation Contract): Detailed pseudo-code for all wrapper headers.
- Code review `.specs/sprint-2026-05/math-foundations/code-review.md`: Verifies the implementation matches the spec.
- ADR-001 (`docs/adr/ADR-001-result-error-pattern.md`): Establishes `Result<T>` for error propagation; this ADR documents that math operations do NOT use `Result<T>` (pure computation, no error paths).
- CONST-001 (`docs/constitution/architecture-boundaries.md`): Mandates the architecture boundary that this wrapper pattern enforces.
- Wiki `architecture/overview.md`: Operational description of the math module and the GLM boundary.
