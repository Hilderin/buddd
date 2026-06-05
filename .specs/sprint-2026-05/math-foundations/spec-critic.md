# Spec Review — Math Foundations (SPEC-004)

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

The spec defines a linear algebra foundation for the Buddd Engine: `Vec2`, `Vec3`, `Vec4`, `Mat4`, `Quat`, and `Camera` types as thin C++ wrappers around GLM, living under `src/engine/math/` in the `buddd::engine::math` namespace. It is well-structured, covers the core types needed for 3D rendering, and demonstrates a mature understanding of the project's architectural constraints.

The initial review identified 8 blocking issues. All 8 blocking issues have been resolved in the spec revision (see checked items below). The spec is resubmitted for re-review.

### Resolutions applied

1. **GLM leak in Camera header**: Replaced `glm::radians(60.0f)` with pre-computed float `1.0471975512f`.
2. **User stories using GLM functions**: Added `math::radians()`, `math::degrees()`, and `math::constants` namespace (pi, half_pi, two_pi, epsilon). User stories updated to use these.
3. **Mat4::inverse() ambiguity**: Spec now states "delegates to glm::inverse; singular matrices produce NaN/inf" (pure delegation, no detection).
4. **Missing Mat4 * Vec3 operator**: Added `operator*(Mat4, Vec3)` and `operator*(Vec3, Mat4)`.
5. **Missing constructors**: Added explicit constructors to Vec2, Vec3, Vec4, Mat4, Quat API listings.
6. **Missing Quat component constructor**: Added `Quat(float w, float x, float y, float z)`.
7. **Untestable ACs**: All AC-001 through AC-006 now specify comparison against GLM with `1e-5f` tolerance.
8. **reinterpret_cast claim**: Replaced with `.glm()` accessor pattern + `static_assert` for standard_layout and size equality. Added AC-017 and AC-018.

## Positive aspects

- **Well-scoped**: The goals/non-goals are clearly delineated and appropriate for a math foundations layer. Out-of-scope items (SIMD, double precision, scene graph, physics) are correctly excluded.
- **Good architectural awareness**: The spec explicitly addresses CONST-001 (architecture boundary) and correctly places all math types inside `src/engine/` with GLM hidden behind wrappers.
- **Thorough type coverage**: The six types (Vec2, Vec3, Vec4, Mat4, Quat, Camera) cover the essential linear algebra needs for rendering a model on screen.
- **Clear memory layout documentation**: The table of sizes, alignments, and column-major layout for Mat4 is excellent and directly actionable for GPU interop.
- **Edge cases documented**: Zero-length normalization, singular matrix inverse, parallel up-vector in lookAt, and slerp edge cases are all called out.
- **Open questions resolved**: Q-01 through Q-04 show good deliberation (GLM version pinning, singular matrix error handling, no caching, namespace choice).
- **Consistent naming**: Files use `snake_case`, classes use `PascalCase`, namespace matches directory structure — all consistent with project conventions from SPEC-001/SPEC-002.
- **Operator design is good**: The set of arithmetic operators is comprehensive and idiomatic C++. The `normalize()` vs `normalized()` mutating/non-mutating distinction is a good convention.
- **Camera API is sufficient** for the stated use case ("draw a basic model on screen without texture but with a camera"): position, orientation, look_at, perspective projection, and view/projection matrix accessors cover the essential needs.

## Issues

All issues found during review are listed below. Blocking issues are marked with **[BLOCKING]** in their description and replicated in the Blocking Issues section.

### Architecture boundary violations and GLM leakage

- [x] **[BLOCKING]** **Camera default member initializer uses `glm::radians()` directly.** The member declaration `float fov_y_{glm::radians(60.0f)};` introduces a GLM function call in a header that may be transitively included by consumers. **Resolution: Replaced with pre-computed float `1.0471975512f`.**

- [x] **[BLOCKING]** **User stories use `glm::radians()` and `glm::half_pi()` but no wrapper equivalents exist.** **Resolution: Added `math::radians()`, `math::degrees()`, and `math::constants` (`pi()`, `half_pi()`, `two_pi()`, `epsilon()`). User stories updated to use these.**

### Ambiguous or incomplete behavior

- [x] **[BLOCKING]** **`Mat4::inverse()` singular matrix behavior is ambiguous.** **Resolution: Spec now states "delegates to glm::inverse; singular matrices produce NaN/inf" (pure delegation, no detection).**

- [x] **[BLOCKING]** **Missing `operator*(Mat4, Vec3)` and `operator*(Vec3, Mat4)`.** **Resolution: Added both overloads to the Mat4 API listing.**

- [x] **[BLOCKING]** **No explicit constructors shown for Vec2/Vec3/Vec4.** **Resolution: Added explicit constructors (`Vec2()`, `Vec2(x,y)`, etc.) to all vector type API listings.**

- [x] **[BLOCKING]** **`Quat` constructors from components not shown.** **Resolution: Added `Quat(float w, float x, float y, float z)` constructor to API listing.**

### Acceptance criteria testability

- [x] **[BLOCKING]** **AC-001 through AC-006: "correct results" is not a testable criterion.** **Resolution: All AC verification columns now specify "must match equivalent GLM output within 1e-5f tolerance".**

- [x] **AC-005 Quat +, - operators listed in AC but absent from API.** **Resolution: AC-005 now correctly lists only * operator (composition and rotate vector), not + or -.**

- [ ] **AC-015 verification is code-review-only.** This is acceptable for an initial spec but should note it as a known limitation.

### Re-review findings (revision round)

- [x] **[NEW - Contradiction]** **AC-010 contradicts the `.glm()` accessor.** **Resolution: AC-010 reworded to add exception: "No GLM types appear in the public API except via the explicit .glm() interop accessor."**
- [x] **[NEW - Minor inconsistency]** **Edge case and error case for `normalize()` on zero-length vector are contradictory.** **Resolution: Unified to "Returns a vector with NaN components (GLM behaviour: division by 0 → NaN)."**
- [x] **[NEW - Minor ambiguity]** **`Vec3 * Mat4` semantics are underspecified.** **Resolution: Added edge case entry documenting both `Mat4 * Vec3` (column-vector) and `Vec3 * Mat4` (row-vector) conventions.**
- [x] **[NEW - Minor wording]** **AC-006 uses undefined term `forward`.** **Resolution: AC-006 now explicitly defines `forward = orientation * Vec3(0, 0, -1)` and `up = orientation * Vec3(0, 1, 0)`.**

### Missing operations and edge cases

- [x] **No `radians()`/`degrees()` conversion utility provided, yet examples depend on it.** **Resolution: Added `radians()` and `degrees()` utility functions to the spec.**

- [x] **No math constants provided.** **Resolution: Added `math::constants` namespace with `pi()`, `half_pi()`, `two_pi()`, `epsilon()`.**

- [x] **No `lerp()` for vectors.** **Resolution: Added `Vec3::lerp()` method and AC-020.**

- [x] **`operator[]` out-of-bounds behavior not documented.** **Resolution: Added entry to edge cases table.**

- [x] **Division by zero for scalar `operator/` not in error cases.** **Resolution: Added entry to error cases table.**

- [x] **`Mat4::perspective()` with negative or zero FOV not documented.** **Resolution: Added entry to error cases table.**

- [x] **No way to construct a `Camera` with non-default values directly.** **Resolution: Added parameterized constructor to Camera: `Camera(Vec3, Quat, float, float, float, float)`.**

### Architecture boundary concerns

- [x] **GLM is a PUBLIC dependency, making the architecture boundary convention-only.** **Resolution: Added A-10 explicitly documenting this risk, matching the existing pattern for SDL3/OpenGL.**

### Acceptance criteria completeness

- [x] **No AC for verifying that Camera matrices are consistent.** **Resolution: AC-006 now specifies that view_matrix() must match glm::lookAt() output, which inherently verifies the inverse relationship.**
- [x] **No AC for verifying that Camera::view_projection_matrix() returns projection * view (not vice versa).** **Resolution: AC-016 is now in the AC table.**
- [ ] **No AC for verifying the wrapper is zero-overhead.** SC-003 says "Manual review" — this is reasonable but AC-013 (no warnings) partially covers it.

### Naming and consistency

- [x] **Inconsistent static vs member API.** **Resolution: Removed static forms of `dot()` and `cross()` — only member forms remain. All operations that make sense as member API use member form only. Static factories (Mat4::perspective, etc.) remain static as they construct new objects.**

- [x] **`operator*=` IS listed for Quat** (line 317: `auto operator*=(Quat other) -> Quat&;`). Resolved — no issue.

### ABI and interop concerns

- [x] **[BLOCKING]** **`reinterpret_cast` between wrapper and GLM types is claimed safe without qualification.** **Resolution: Spec now recommends `.glm()` accessor as the official interop path, includes `static_assert` for `std::is_standard_layout_v` and `sizeof` equality, and adds AC-017 and AC-018 to verify this.**

### Performance and implementation

- [ ] **No caching for Camera matrices.** The spec documents this as intentional (KISS), which is fine, but `view_projection_matrix()` calls both `view_matrix()` and `projection_matrix()` independently, doubling the work. The spec documents this as an intentional trade-off. Acceptable.
- [ ] **Camera is perspective-only.** The spec documents that `Mat4::ortho()` can be used directly for orthographic projections. Acceptable for the initial implementation.

## Blocking issues

Items that must be resolved before the artifact can be accepted:

- [x] **Camera default member initializer uses `glm::radians()` directly.** → Replaced with pre-computed float `1.0471975512f`.
- [x] **User stories use `glm::radians()`/`glm::half_pi()` but no wrapper equivalents.** → Added `math::radians()`, `math::degrees()`, `math::constants`.
- [x] **`Mat4::inverse()` singular matrix behavior is ambiguous.** → Specified: delegates to glm::inverse (NaN/inf on singular, no detection).
- [x] **Missing `operator*(Mat4, Vec3)` and `operator*(Vec3, Mat4)`.** → Added both overloads.
- [x] **Missing explicit constructors for Vec2/Vec3/Vec4.** → Added to API listings.
- [x] **Missing Quat component constructor.** → Added `Quat(float w, float x, float y, float z)`.
- [x] **AC-001–006 "correct results" not testable.** → All now specify `1e-5f` tolerance vs GLM.
- [x] **`reinterpret_cast` claim needs hardening.** → `.glm()` accessor + static_assert for standard_layout/size.

## Non-blocking issues / Warnings

Non-blocking concerns for awareness (remaining after revision):

- **Camera is perspective-only** — orthographic users must use `Mat4::ortho()` directly. Acceptable for initial scope.
- **`view_projection_matrix()` performance concern** — calls both view and projection independently each time; documented KISS trade-off.
- **AC-010 vs .glm() accessor contradiction** — see re-review findings above.
- **normalize() zero-vector wording inconsistency** — see re-review findings above.
- **Vec3 * Mat4 semantics underspecified** — see re-review findings above.
- **AC-006 uses undefined `forward`** — see re-review findings above.

## Verdict

**Accepted.** All 8 original blocking issues resolved. All 4 re-review wording issues resolved. The architecture, API surface, and acceptance criteria verification strategies are sound. The spec is ready to proceed to the implementation contract phase.

The scope, naming conventions, and architectural approach are sound. All remaining unchecked items are acknowledged limitations that do not prevent acceptance.
