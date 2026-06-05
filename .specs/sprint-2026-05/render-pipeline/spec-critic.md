# SPEC-005 Critic Review — Render Pipeline (Shader, Material, VertexBuffer, IndexBuffer)

## Status

`Rejected`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

SPEC-005 is an **exceptionally thorough, well-structured spec** that demonstrates careful thought about the render pipeline abstractions, their lifecycle, error handling, edge cases, and testability. The problem statement is clear, the goals are well-scoped, the non-goals are appropriately bounded, and the 19 assumptions show deep consideration of implementation details.

However, the spec is **rejected** in its current state due to three categories of issues:

1. **Unresolved open questions** — Q-01, Q-02, and Q-03 are marked `[NEEDS CLARIFICATION]` and directly affect core API behavior (uniform type safety, headless backend semantics, error category granularity). Following the established pattern (SPEC-002, SPEC-004 were accepted only after all open questions were resolved), these must be answered before the spec is complete.

2. **Missing `set_uniform(bool)` overload** — GLSL `bool` uniforms are a common use case (feature toggles, flags) and there is no overload for `bool`. Callers must resort to `int32_t` workarounds, which is a functional gap.

3. **`draw`/`draw_indexed` return `void` without justification of inconsistency with ADR-001** — ADR-001 states "All public API functions that can fail return `Result<T>`." Draw calls can fail (invalid state, GPU errors, OOM). The spec documents these as undefined behavior. This is a valid design choice, but the inconsistency with ADR-001 is not acknowledged or justified.

4. **Headless backend linking error simulation is underspecified** — AC-022 requires testing linking failures, but the headless backend has no specified mechanism to simulate them.

Despite this rejection, the spec is **close to ready**. The blocking issues are small in scope and the overall architecture, API design, and test strategy are sound.

## Positive aspects

- **Comprehensive coverage**: Problem, goals, non-goals, actors, user stories, ACs, SCs, edge cases, error cases, permissions, observability, assumptions, and open questions are all present and well-organized.
- **Clear API design**: The factory pattern on `RenderDevice`, ownership transfer via `unique_ptr`, `Result<T>` return types, and value-type enums/structs are consistent with existing project conventions.
- **Architecture boundary preservation**: SC-003 explicitly verifies that no backend types leak through public headers, and the OpenGL backend lives inside `src/engine/render/`. This is fully consistent with CONST-001.
- **Testability focus**: Headless backend, ACs with verification methods, and the clear delineation between headless and OpenGL testing demonstrate strong test awareness.
- **18 edge cases and 13 error cases** are documented — this level of thoroughness is excellent and covers most practical failure scenarios.
- **Consistent with existing specs**: Namespace (`buddd::engine`), trailing return types, non-copyable/non-movable abstract classes, `Result<T>` pattern, and math types (`math::Vec3`, `math::Mat4`) all align with SPEC-002, SPEC-004, and ADR-001/ADR-002.
- **Observability section**: Logging via `std::cerr` for key lifecycle events is consistent with SPEC-002's approach.
- **Backend-specific assumptions**: A-12 through A-18 provide detailed OpenGL DSA implementation guidance that will be invaluable to the implementer.

## Issues

### (Blocking) B-01: Open questions Q-01, Q-02, Q-03 are unresolved

**Description**: Three questions remain marked `[NEEDS CLARIFICATION]`:
- **Q-01**: Whether to type-check uniform values against GLSL declarations. This affects whether `set_uniform` silently succeeds or returns an error for type mismatches.
- **Q-02**: How the headless backend simulates uniform discovery for `has_uniform`. This affects headless test behavior.
- **Q-03**: Whether to add many fine-grained `Error::Category` values or one `RenderPipelineError` with sub-codes. This affects error handling ergonomics across the pipeline.

**Impact**: Without resolution, implementers cannot know:
- Whether `set_uniform` with wrong types should be checked or silently passed through (Q-01).
- Whether `has_uniform` requires an explicit uniform name list at material construction (Q-02).
- How many new `Error::Category` values to add and where (Q-03).

This is inconsistent with the established pattern: SPEC-002 and SPEC-004 were accepted only after all open questions were marked `[RESOLVED]`.

**Suggested resolution**: The human (or orchestrator) should answer all three questions before re-review. Draft resolutions are recommended in the review below (see `Required changes`).

---

### (Blocking) B-02: Missing `set_uniform` overload for `bool`

**Description**: The `set_uniform` API provides overloads for `float`, `int32_t`, `Vec3`, `Vec4`, and `Mat4`, but not `bool`. GLSL supports `bool` uniforms (internally set via `glUniform1i` with 0/1), which are commonly used for shader feature toggles (e.g., `u_enable_fog`, `u_use_texture`, `u_flip_normals`).

**Impact**: Application developers cannot set `bool` uniforms through the abstract API without casting to `int32_t`. This is a functional gap and a usability issue.

**Suggested resolution**: Add an overload:
```cpp
auto set_uniform(std::string_view name, bool value) -> Result<void>;
```
This would internally map to `glUniform1i(location, value ? 1 : 0)` for the OpenGL backend and store the boolean value for the headless backend.

---

### (Blocking) B-03: Headless backend linking error simulation is underspecified (AC-022)

**Description**: AC-022 states that shader linking failure (e.g., mismatched stage inputs/outputs) returns an error from `create_material` with a message containing the link error. The verification says "Unit test (headless or OpenGL with offscreen) verifies error is returned for incompatible shaders."

For the OpenGL backend, real linking detects this. But for the **headless backend**, there is no specification for how linking errors are simulated. Unlike AC-021 (which explicitly says "a simulated error (headless backend)"), AC-022 does not specify headless behavior.

**Impact**: If only headless tests are available (CI without GPU), AC-022 cannot be tested deterministically — the headless backend doesn't know what constitutes "incompatible shaders."

**Suggested resolution**: Either:
- (a) Explicitly state that the headless backend simulates linking errors when shader names/sources contain specific markers (e.g., source contains `#error`), OR
- (b) State that AC-022 is only verified with the OpenGL+offscreen backend and is excluded from headless-only test runs, OR
- (c) Provide a mechanism in `HeadlessMaterialConfig` to specify which uniforms/sources would fail linking.

---

### (Blocking) B-04: `draw`/`draw_indexed` return `void` without justification of inconsistency with ADR-001

**Description**: ADR-001 states: "All public API functions that can fail return `Result<T>`. Exceptions are not used for control flow in engine code." (Section "Where this applies") and "Functions that cannot logically fail (pure getters, predicates, trivial computations) — these return plain values." (Section "Where this does NOT apply").

The `draw` and `draw_indexed` methods return `void`, but drawing can fail for many reasons (invalid program state, out-of-bounds vertex access, OpenGL errors, GPU hang, OOM). The spec documents these failures as "undefined behavior" (edge cases lines 268–270) — effectively treating them as precondition violations rather than runtime errors.

**Impact**: This is a valid and common API design choice (many graphics APIs treat draw-call errors as UB to avoid per-frame branching). However, it is inconsistent with ADR-001's blanket statement, and the spec does not acknowledge or justify this inconsistency. An implementer reading the spec might be confused about when to use `Result<T>` vs. `void`.

**Suggested resolution**: Add a brief rationale in the spec (e.g., in the User-visible behavior or Assumptions section) explaining:
- Why draw methods return `void` (performance-sensitive hot path, precondition-based design).
- That precondition violations (invalid topology, out-of-bounds access, unlinked material) are undefined behavior — the caller must ensure correct state before drawing.
- That this is a deliberate exception to the general `Result<T>` pattern, consistent with real-time graphics API conventions.

---

### (Non-blocking) W-01: `VertexFormat` container type is unspecified

**Description**: AC-006 states "A `VertexFormat` struct exists with `uint32_t stride` and a container of `VertexAttribute` entries." The container type (e.g., `std::vector<VertexAttribute>`, `std::array<VertexAttribute, N>`, or a fixed-size list) is not specified.

**Impact**: Minor — implementers can choose. But if different implementations pick different containers, API compatibility may differ. Not blocking.

**Suggestion**: Specify `std::vector<VertexAttribute>` for flexibility, or note that the container type is implementation-defined.

---

### (Non-blocking) W-02: `VertexAttributeType` lacks signed `Byte` and `Short` variants

**Description**: The `VertexAttributeType` enum includes `UByte`, `UByte4`, `UByte4Norm`, `Int`, `Int2`, `Int3`, `Int4`, `Float`, `Float2`, `Float3`, `Float4` — but no signed `Byte` (or `Byte4`, `Byte4Norm`) or `Short` (or `Short2`, `Short4`) variants.

**Impact**: Limited use cases (e.g., signed byte normals, short texture coordinates). Common formats like `GL_BYTE`, `GL_SHORT`, `GL_SHORT_NORM` are not representable. Not blocking because the most common formats are covered, but worth noting for future expansion.

---

### (Non-blocking) W-03: `Material` has no getter for vertex/fragment shader sources

**Description**: The `Material` abstraction has no way to retrieve the shader source strings or the linked program status after creation. This is consistent with the non-goals (no serialisation, no caching), but it makes debugging harder — a developer cannot inspect which shaders are in a material.

**Impact**: Low. Debugging is handled via the Observability section (`std::cerr` output at creation time). Not blocking.

---

### (Non-blocking) W-04: Story 1 pseudo-code omits error handling

**Description**: Story 1 (line 136–141) shows:
```cpp
auto vs = device->create_shader(ShaderType::Vertex, vertex_source);
auto fs = device->create_shader(ShaderType::Fragment, fragment_source);
auto mat = device->create_material(std::move(*vs), std::move(*fs));
```
`create_shader` returns `Result<std::unique_ptr<Shader>>`, but the story code treats `vs` and `fs` as if they are already `unique_ptr` values. While `*vs` on a `Result` does give access to the contained value (via `std::expected::operator*`), using it without checking `.has_value()` is unsafe. This is acceptable for a user story (happy path), but it could confuse implementers who copy the pseudo-code literally.

**Impact**: Low — user stories are illustrative. The ACs and error cases clearly document error handling.

---

### (Non-blocking) W-05: Edge case — `IndexType::Uint16` with odd byte count not validated

**Description**: Edge case line 271: "`IndexType::Uint16` but index data size is not a multiple of 2 — The `create_index_buffer` factory may either return an error or round up; the spec does not mandate validation. Undefined if data is misaligned."

**Impact**: Callers have no portable way to know if their index data is valid. If implementation A returns an error and implementation B succeeds silently (with undefined behavior at draw time), portability is compromised. Not blocking, but adding a validation requirement (return error on misaligned data) would improve safety.

---

### (Non-blocking) W-06: `VertexBuffer` and `IndexBuffer` lack size/stride query methods

**Description**: The abstract `VertexBuffer` and `IndexBuffer` classes have no getters for:
- Vertex count (or byte size) of the buffer.
- `VertexFormat` of the buffer.
- `IndexType` of the index buffer.

**Impact**: Low. The spec restricts buffers to creation-time data (no streaming, no updates). If an engine developer needs this info, they must store it themselves. Not blocking, but it limits introspection.

---

### (Non-blocking) W-07: `PrimitiveTopology` values could use OpenGL-compatible integer backing

**Description**: `PrimitiveTopology` is an enum. If it maps directly to OpenGL enum values (`GL_TRIANGLES = 0x0004`, etc.), the OpenGL backend can cast trivially. If not, a switch statement is needed. The spec does not specify the enum integer values.

**Impact**: Low — the implementation contract or code can define this. Not blocking.

## Blocking issues checklist

- [ ] B-01: Open questions Q-01, Q-02, Q-03 must be resolved (marked `[RESOLVED]` with answers) before acceptance.
- [ ] B-02: Add `set_uniform(std::string_view name, bool value) -> Result<void>` overload.
- [ ] B-03: Specify how the headless backend simulates linking errors for AC-022, or clarify that AC-022 requires OpenGL backend testing.
- [ ] B-04: Add rationale for `draw`/`draw_indexed` returning `void` despite potential failure, acknowledging the inconsistency with ADR-001.

## Non-blocking issues / Warnings

- W-01: `VertexFormat` container type unspecified (consider `std::vector<VertexAttribute>`).
- W-02: `VertexAttributeType` lacks signed `Byte` and `Short` variants.
- W-03: `Material` lacks shader source getters (debugging convenience).
- W-04: Story 1 pseudo-code omits `.has_value()` checks on `Result<T>`.
- W-05: `IndexType` alignment validation not mandated (data misalignment = UB).
- W-06: `VertexBuffer`/`IndexBuffer` lack size/type query methods.
- W-07: `PrimitiveTopology` integer backing values unspecified.

## Open questions assessment

| ID | Question | Assessment |
|---|---|---|
| Q-01 | Uniform type checking — should `set_uniform` query `glGetActiveUniform` for type validation? | **Must be resolved.** Affects API contract. **Recommended answer**: Document as "caller responsibility; no type checking in the engine" — consistent with the non-goal of shader reflection and the principle of keeping the API simple. Add a note in the edge case table. |
| Q-02 | Headless `has_uniform` — should headless `Material` accept a list of uniform names, or return `true` only for previously-`set_uniform`-ed names? | **Must be resolved.** Affects headless backend design and testability. **Recommended answer**: Accept an optional `std::vector<std::string>` of known uniform names at construction (via a `HeadlessMaterialConfig` struct or constructor parameter). `has_uniform` returns `true` for names in this list OR names that have been previously set via `set_uniform`. This gives callers full control over test scenarios. |
| Q-03 | Error categories — add many fine-grained categories or one `RenderPipelineError` with sub-codes? | **Must be resolved.** Affects error-handling ergonomics. **Recommended answer**: Add fine-grained categories (`ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`) — consistent with the existing pattern (each failure mode has its own category), simplifies error matching at call sites, and is explicitly allowed by ADR-001. |

## Verdict

`Rejected`

The spec is **architecturally sound, well-scoped, and thoroughly detailed**. However, the four blocking issues (unresolved open questions, missing `bool` uniform overload, underspecified headless linking simulation, and the undocumented `draw`/`draw_indexed` inconsistency with ADR-001) must be resolved before the spec can proceed to implementation.

Once these issues are addressed, the spec should be ready for `Accepted` status. The non-blocking warnings (W-01 through W-07) are suggestions for improvement that do not block acceptance.
