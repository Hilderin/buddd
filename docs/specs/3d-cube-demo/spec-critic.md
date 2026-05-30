# SPEC-009 Critic Review — Model Utility & 3D Cube Demo

## Status

`Rejected`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

SPEC-009 is a **thorough, well-structured spec** that follows the established project template and conventions. The `Model` utility class design is clean — factory methods returning `Result<Model>`, RAII ownership via `unique_ptr`, movable but not copyable, and a `draw()` convenience wrapping `RenderDevice` draw calls. The cube demo provides a meaningful end-to-end exercise of the render pipeline: indexed drawing, Camera integration, per-frame MVP computation, and per-face uniform changes. The spec is well-scoped with clear non-goals, comprehensive edge cases (22 items), and complete error coverage (12 cases).

However, the spec is **rejected** in its current state due to five categories of issues:

1. **Unresolved open questions** — Five questions marked `[NEEDS CLARIFICATION]` (Q-01 through Q-05) directly affect core API semantics, testability, and visual correctness. Following the established pattern (SPEC-002, SPEC-004, SPEC-005 were accepted only after all open questions were resolved), these must be answered before the spec is complete.

2. **Missing Camera configuration** — The Camera's `set_perspective()` is not called to set the aspect ratio to 800/600 (~1.333). The Camera default aspect is 16:9 (~1.778). This would produce a distorted cube. The `up` vector for `look_at()` is also unspecified.

3. **`Model::draw()` returns `void` without justification** — ADR-003 and SPEC-005 justify `RenderDevice::draw()` returning `void` because it is on a hot path. `Model::draw()` is a convenience wrapper (not on the hot path — it calls `device.draw()` which is the hot path) and its `void` return is inconsistent with ADR-001 without rationale.

4. **Observability pseudocode has a type error** — The logging example cannot compile (`const char* + uint32_t`).

5. **Missing edge cases and testability gaps** — A few important edge cases are not covered, and the `up` vector and aspect ratio are not tested.

## Positive aspects

- **Comprehensive coverage**: Problem, goals, non-goals, actors, user stories, ACs (26 items), SCs (5 items), edge cases (22 items), error cases (12 items), permissions, observability, assumptions, and open questions are all present and well-organized.
- **Clean API design**: `Model::create()` and `Model::create_indexed()` static factories returning `Result<Model>` are consistent with the project's `Result<T>` error-handling pattern (ADR-001). The non-copyable/movable semantics follow the established RAII convention.
- **Consistency with existing specs**: Namespace (`buddd::engine`, `buddd::cmd::demo`), error categories (`ShaderCompilationFailed`, `LinkingFailed`, etc.), demo dispatch pattern (if/else in `demo_command.cpp`), and `setup_cube()` error handling (print + `std::exit`) all align with SPEC-005, SPEC-004, SPEC-006, and SPEC-007.
- **Architecture boundary preservation**: AC-026 explicitly verifies that `cube_demo.h` exposes no backend types. `Model` lives inside `src/engine/render/` which is inside the engine boundary. No violation of CONST-001.
- **Testability awareness**: Headless backend support is baked into every AC. AC-024/AC-025 specifically target headless uniform tracking. The headless backend's simulation capability is leveraged correctly.
- **Thorough edge cases**: 22 edge cases covering null data, zero stride, zero attributes, shader failure, moved-from state, multi-face drawing, and early window closure demonstrate careful consideration.
- **Good separation of concerns**: The spec correctly separates the reusable `Model` abstraction (engine layer) from the cube demo (application layer), with `setup_cube()` in the demo namespace creating cube-specific data and `Model::create_indexed()` handling resource bundling.

## Issues

### (Blocking) B-01: Five open questions (Q-01 through Q-05) remain unresolved

**Description**: Five questions remain marked `[NEEDS CLARIFICATION]`:

- **Q-01**: Whether `Model::draw()` should reset uniform state. The spec currently says "the caller is responsible" — `draw()` is a pure dispatch. This is the correct design (draw should not have side effects on material state). The question asks for validation.
- **Q-02**: Whether the cube demo should have a `--test` mode (like the old triangle demo had). The existing demo architecture (SPEC-007) doesn't have a separate `--test` flag — all demos run via `buddd demo <name>` with their own frame count. This question is effectively already answered by the existing architecture; no `--test` flag is needed.
- **Q-03**: Whether the face order matters for acceptance. The spec defines order +X, -X, +Y, -Y, +Z, -Z with specific colours. For headless uniform-tracking tests (AC-016), the order matters because the test verifies which colour is set per draw call. The order should be fixed.
- **Q-04**: Whether `Model::create()` should accept an explicit `PrimitiveTopology` parameter. Currently assumes `Triangles`. Adding a defaulted parameter now avoids a breaking change later.
- **Q-05**: What is the exact winding order for cube face triangles. The spec says "counter-clockwise when viewed from outside" but this needs explicit confirmation. If the winding order is wrong and the OpenGL backend enables back-face culling, the cube faces will be invisible.

**Impact**: Without resolution:
- Q-01: Implementers don't know if `draw()` must track prior uniform state.
- Q-02: The cube demo may need a `--test` mode that the spec doesn't define.
- Q-03: Headless uniform-tracking tests (AC-016) are underspecified.
- Q-04: `Model` API may need a breaking change later to add topology support.
- Q-05: The cube may render incorrectly (invisible faces) on the OpenGL backend.

This is inconsistent with the established pattern: SPEC-002, SPEC-004, and SPEC-005 were accepted only after all open questions were marked `[RESOLVED]`.

**Suggested resolution**: The orchestrator should present these questions to the human. Draft recommended answers:

| ID | Recommended answer |
|---|---|
| Q-01 | **No reset.** `draw()` is a pure dispatch — it does not touch uniforms. This is the current spec and should be confirmed. |
| Q-02 | **No `--test` flag.** The existing demo architecture (SPEC-007) uses frame-count-based demos via `buddd demo <name>`. The cube demo's 120-frame loop is the only mode. |
| Q-03 | **Order matters for determinism.** The spec's order (+X, -X, +Y, -Y, +Z, -Z) should be fixed. Headless tests verify uniform `u_color` is set for each face in this order. |
| Q-04 | **Add topology parameter with default.** Change `Model::create()` and `create_indexed()` to accept `PrimitiveTopology topology = PrimitiveTopology::Triangles`. This is forward-compatible and non-breaking. |
| Q-05 | **Counter-clockwise when viewed from outside.** Confirm the winding order, and document that the OpenGL backend uses `GL_BACK` culling (or equivalent default). Add an AC for this. |

---

### (Blocking) B-02: Camera aspect ratio not explicitly set (distorted cube)

**Description**: The spec says:

> "Assumes a fixed aspect ratio matching the 800×600 window (aspect = 800/600 ≈ 1.333)."

But the `Camera` default constructor (defined in SPEC-004 and in `src/engine/math/camera.h`) sets `aspect_` to `16.0f / 9.0f ≈ 1.778`. There is no explicit call to `camera.set_perspective(60.0f, 800.0f/600.0f, 0.1f, 100.0f)` or equivalent in the spec's cube demo loop description.

**Impact**: The cube will appear vertically squished (aspect mismatch between the window and the projection matrix). This is a visual correctness bug.

**Suggested resolution**: Add an explicit step in the cube demo loop (Section "Demo loop behaviour", step 2):
```cpp
camera.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
```

---

### (Blocking) B-03: Camera `up` vector not specified for `look_at`

**Description**: The spec says:
```
camera.look_at(eye, center, up)
```
But does not specify what `up` is. While the project convention is Y-up (SPEC-004 A-08), the spec should be explicit. If a different `up` vector is used (e.g., `Vec3::unit_z()`), the cube would appear tilted.

**Impact**: Ambiguity in the demo specification could lead to incorrect camera orientation.

**Suggested resolution**: Add explicit `up` vector to step 2:
```cpp
auto const up = buddd::engine::math::Vec3::unit_y();
camera.look_at({3.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, up);
```

---

### (Blocking) B-04: `Model::draw()` returns `void` without justification of inconsistency with ADR-001

**Description**: `Model::draw()` returns `void` (line 130 in the spec). ADR-001 states: "All public API functions that can fail return `Result<T>`." ADR-003/SPEC-005 justify `RenderDevice::draw()` returning `void` because it is "on a performance-sensitive hot path where per-frame error checking is impractical." `Model::draw()` is a convenience wrapper — it is NOT on the same hot path; it delegates to `device.draw()` which is already `void`. The inconsistency with ADR-001 is not acknowledged or justified.

**Impact**: An implementer reading the spec might be confused about when to use `Result<T>` vs `void`. If `Model::draw()` can fail (e.g., precondition violations), the `void` return silently ignores this.

**Suggested resolution**: The spec should include a brief rationale for `Model::draw()` returning `void`:

> `Model::draw()` returns `void` (not `Result<void>`) consistent with `RenderDevice::draw()`. Drawing is a hot-path operation where precondition violations (invalid state, out-of-bounds access) are undefined behavior — the caller must ensure correct state before drawing. This is consistent with the exception established in ADR-003 for `RenderDevice` draw methods.

Add this to the "Drawing" subsection of "User-visible behavior" or to the Assumptions section.

---

### (Blocking) B-05: Observability pseudocode has a compile error

**Description**: The Observability section (page "Observability") shows:
```cpp
std::cerr << "Model created (" << vertex_count << " vertices"
          << (has_indices ? ", " + index_count + " indices" : "") << ")\n";
```
The expression `", " + index_count + " indices"` will not compile because `index_count` is `uint32_t`, and there is no `operator+(const char(&)[N], uint32_t)`. The string literal `", "` decays to `const char*`, and `const char* + uint32_t` performs pointer arithmetic (undefined behavior for non-zero values), not string concatenation.

**Impact**: The provided observability code cannot be compiled as written. An implementer copying this code would encounter a compile error or (worse) undefined behavior via pointer arithmetic.

**Suggested resolution**: Change the logging expression to:
```cpp
std::cerr << "Model created (" << vertex_count << " vertices"
          << (has_indices ? ", " + std::to_string(index_count) + " indices" : "") << ")\n";
```
Or use stream chaining:
```cpp
std::cerr << "Model created (" << vertex_count << " vertices";
if (has_indices) {
    std::cerr << ", " << index_count << " indices";
}
std::cerr << ")\n";
```

---

### (Non-blocking) W-01: Missing edge case — `Model::draw()` after `RenderDevice` destruction

**Description**: The spec documents that `Model` owns GPU resources created through `RenderDevice`. If the `RenderDevice` is destroyed before the `Model`, the GPU resources (buffers, material) are dangling. The spec does not mention this scenario in edge cases or error cases.

**Impact**: Low — this is a programming error that would be caught in code review. But documenting it as undefined behavior (consistent with the precondition-based design) would be thorough.

**Suggestion**: Add an edge case: "Model::draw() called after the RenderDevice that created it is destroyed → Undefined behavior (the GPU resources are no longer valid)."

---

### (Non-blocking) W-02: Default-constructed `Model` accessibility is underspecified

**Description**: The spec says "A default-constructed or moved-from `Model` is in a 'null' state" but does not show an explicit default constructor. The class definition shows only deleted copy and defaulted move — no default constructor. The compiler would implicitly generate one (since all `unique_ptr` members are default-constructible), but the spec doesn't clarify whether `Model m;` is valid public API.

**Impact**: Low — the implicit default constructor exists and works. But explicitly documenting the null state contract improves clarity.

**Suggestion**: Add an explicit default constructor declaration:
```cpp
Model() noexcept = default;
```

---

### (Non-blocking) W-03: Missing `Model::set_uniform()` convenience

**Description**: The spec requires callers to do `model.material().set_uniform(name, value)`. A convenience method `model.set_uniform(name, value)` that forwards to `material().set_uniform()` would reduce boilerplate for the common case.

**Impact**: Low — the current design is explicit and clear. But the convenience would reduce verbosity in per-frame code like the cube demo.

**Suggestion**: Add (optionally) a forwarding method:
```cpp
template<typename T>
auto set_uniform(std::string_view name, T&& value) -> Result<void> {
    return material().set_uniform(name, std::forward<T>(value));
}
```

---

### (Non-blocking) W-04: Winding order not verified by any acceptance criterion

**Description**: Q-05 asks about the exact winding order. The spec says "winding order: counter-clockwise when viewed from outside the cube" (step 4 of setup_cube). But no AC verifies this winding order is correct. If the winding is clockwise, the cube faces will be back-face culled on the OpenGL backend.

**Impact**: Medium — the cube may render as invisible on the OpenGL backend if the winding order is wrong. Code review catches it, but an automated test would be better.

**Suggestion**: Add an AC: "Cube face triangles use counter-clockwise winding when viewed from outside. Verification: Code review of the index buffer data, or headless test that verifies index ordering matches the specified winding."

---

### (Non-blocking) W-05: `PrimitiveTopology` parameter on `Model::create()` (Q-04 follow-up)

**Description**: Q-04 asks whether to add a `PrimitiveTopology` parameter. This is a genuine API completeness concern. If a future use case needs `TriangleStrip` or `Lines`, the current API would require a breaking change.

**Impact**: Low-medium. Adding a defaulted parameter now is non-breaking and cheap.

**Suggestion**: Add `PrimitiveTopology topology = PrimitiveTopology::Triangles` to both `Model::create()` and `Model::create_indexed()` with clear documentation that the topology is fixed at creation time and cannot be changed.

---

### (Non-blocking) W-06: Index type `Uint16` safety not validated in ACs

**Description**: The spec uses `Uint16` for cube indices (values 0–23), which fits safely. But there is no AC that validates the index values do not exceed 65535. If the vertex data is modified (e.g., more vertices added), the index type might need to be `Uint32`.

**Impact**: Low for the current cube demo. But a defensive AC would prevent future regressions.

**Suggestion**: Add an edge case note or AC: "Cube demo indices are verified to be within [0, 23], safely representable as Uint16."

---

### (Non-blocking) W-07: No mention of `FaceCulling` state for cube rendering

**Description**: The cube demo uses flat-shaded faces with a single material. The OpenGL backend's default face culling state (typically `GL_BACK`) combined with the winding order determines whether faces are visible. The spec doesn't mention whether the demo should set face culling or assume it's disabled.

**Impact**: Minor — if face culling is enabled with the wrong winding, the cube is invisible. The spec mentions winding order but not culling state.

**Suggestion**: Add a note in the demo behaviour section: "The cube demo assumes back-face culling is enabled (OpenGL default) and uses counter-clockwise winding, so faces viewed from outside are visible." Or state that the demo disables face culling.

---

### (Non-blocking) W-08: `setup_cube` error handling duplicates `setup_triangle` pattern but diverges from AC-012 cleanup behavior

**Description**: The spec says `setup_cube` prints FATAL to stderr and calls `std::exit(EXIT_FAILURE)` on failure, matching `setup_triangle`. However, AC-012 requires that on factory failure, prior resources are cleaned up and "ASAN-clean on failure path." The `setup_cube` path uses `std::exit`, which does not run destructors (it calls `_exit` on some platforms) — so ASAN may report leaks despite logically clean code.

**Impact**: Low — this is consistent with the existing `setup_triangle` pattern. If ASAN reports false positives, the test infrastructure should handle it.

**Suggestion**: Clarify that `setup_cube` failure cleanup is handled by the `Model::create()` factory (which cleans up via RAII before propagating the error), and `setup_cube`'s `std::exit` is only reached after model creation has already cleaned up. Add an assumption or note.

---

### (Non-blocking) W-09: AC-018 verification method is weak

**Description**: AC-018 says "Frame count is verified via instrumentation or timing (or code review of the loop bound)." The weakest option ("code review") bypasses automated verification entirely.

**Impact**: Low — the frame count is hardcoded and trivially confirmed by code review. But for headless CI testing, an automated frame count would be better.

**Suggestion**: Tighten AC-018 verification: "Headless test: run_cube_demo completes, and the headless backend's draw-call counter confirms exactly 120 frames × 6 draw calls = 720 draw_indexed calls."

---

### (Non-blocking) W-10: No AC for `Model` being movable but not copyable (AC-009 uses `static_assert` which is compile-time only)

**Description**: AC-009 says `static_assert(!std::is_copy_constructible_v<Model>)` passes and moving transfers ownership. The verification only checks compile-time traits, not runtime behavior (move transfers ownership, source becomes null).

**Impact**: Low — the compile-time check is sufficient for the non-copyable property. Move semantics are verified by AC-021 (moved-from draw is no-op).

**Suggestion**: Already covered by AC-021 and AC-009 together. No action needed.

---

## Blocking issues checklist

- [ ] B-01: Five open questions (Q-01 through Q-05) must be resolved (marked `[RESOLVED]` with answers) before acceptance.
- [ ] B-02: Camera `set_perspective()` must be called to set aspect ratio to 800/600; the assumed aspect is not the default.
- [ ] B-03: Camera `up` vector must be explicitly specified for `look_at`.
- [ ] B-04: Add rationale for `Model::draw()` returning `void`, acknowledging the inconsistency with ADR-001 and aligning with the ADR-003 exception for draw methods.
- [ ] B-05: Fix the Observability pseudocode compile error (`const char* + uint32_t`).

## Non-blocking issues / Warnings

- W-01: Missing edge case — `Model::draw()` after `RenderDevice` destruction (UB not documented).
- W-02: Default constructor for `Model` is implicit — should be explicit in the API contract.
- W-03: Consider adding `Model::set_uniform()` forwarding convenience.
- W-04: No AC verifies cube face winding order is counter-clockwise.
- W-05: Consider adding `PrimitiveTopology` parameter (with default) to `Model::create()` / `create_indexed()` for future-proofing (see Q-04).
- W-06: No AC validates that cube indices fit within Uint16 range.
- W-07: Face culling state for the cube demo is unspecified.
- W-08: `setup_cube` + `std::exit` may interact poorly with ASAN cleanup verification (AC-012).
- W-09: AC-018 verification allows "code review" instead of automated frame-count test — consider tightening.
- W-10: (No action needed — AC-009 + AC-021 already cover move semantics.)

## Open questions assessment

| ID | Question | Assessment |
|---|---|---|
| Q-01 | Should `Model::draw()` reset uniform state? | **Already implicitly answered by the spec.** The spec states "The caller is responsible for setting any desired uniforms on material() before calling draw()." This is the correct design — `draw()` is pure dispatch. Recommend resolving as: **No. Draw is a no-side-effect dispatch.** |
| Q-02 | Should cube demo have a `--test` mode? | **Already resolved by existing architecture.** SPEC-007 removed the old `--test` flag. Demos run via `buddd demo <name>`. No `--test` mode needed. Recommend resolving as: **No. The existing demo architecture does not use a `--test` flag. Align with SPEC-007.** |
| Q-03 | Does face order matter for acceptance? | **Yes, for deterministic testing.** AC-016 verifies which colour is set per face in order. Fixing the order (+X, -X, +Y, -Y, +Z, -Z) enables deterministic headless uniform tracking. Visual inspection alone doesn't require ordering, but headless tests do. Recommend resolving as: **Yes — the fixed order matters for headless testing of per-face uniform state.** |
| Q-04 | Should `Model::create()` accept `PrimitiveTopology`? | **Recommend yes, with default.** Adding `PrimitiveTopology topology = PrimitiveTopology::Triangles` now is non-breaking and avoids a future breaking change. See W-05. |
| Q-05 | What is the exact winding order? | **Must be resolved for visual correctness.** The spec says "counter-clockwise when viewed from outside" — confirm this matches the OpenGL backend's default face-culling convention (typically `GL_CCW` for front face). See W-04. |

## Verdict

`Rejected`

SPEC-009 is **architecturally sound, consistent with existing specs and conventions, and well-scoped**. The `Model` abstraction fills a genuine gap between the low-level render pipeline and application code. The cube demo exercises the right features (indexed drawing, Camera, MVP, per-face uniforms, render loop).

However, the **five blocking issues** (unresolved open questions, missing Camera configuration, missing `up` vector, undocumented `void` return on `Model::draw()`, and compile-error in Observability pseudocode) must be resolved before the spec can proceed to implementation. The recommended answers to Q-01 through Q-05 are provided above; once confirmed by the human and marked `[RESOLVED]`, they should resolve the first blocking issue.

Once these issues are addressed, the spec should be ready for `Accepted` status. The non-blocking warnings (W-01 through W-09) are suggestions for improvement that do not block acceptance.
