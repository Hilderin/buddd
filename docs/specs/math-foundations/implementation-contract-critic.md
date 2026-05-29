# Implementation Contract Review — Math Foundations (IMPL-004)

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

The implementation contract for SPEC-004 (Math Foundations) is thorough, well-constrained, and faithfully implements all 20 acceptance criteria from the accepted spec. It provides complete pseudo-code for all 9 source files (8 new, 1 modified), specifies 68 comprehensive tests, and correctly enforces the architecture boundary (CONST-001). The Camera implementation is mathematically correct: `view = inverse(transform)` via `Mat4::look_at`, `projection = perspective(...)`, `view_projection = projection * view`. All GLM API calls are correct and properly delegated.

However, several convention violations and documentation ambiguities were identified — none functionally blocking, but all should be corrected before the implementation phase to maintain internal consistency.

## Positive aspects

- **Complete AC coverage**: All 20 acceptance criteria from SPEC-004 are faithfully implemented. Every AC has a corresponding code path and test.

- **Excellent level of constraint**: The contract provides complete pseudo-code for every file, leaving zero architectural decisions to the Code Agent. No ambiguity about what to implement.

- **Architecture boundary rigorously enforced**: GLM includes are confined to `src/engine/math/`. The `camera.h` header has zero GLM includes. Verification commands in the Done criteria section explicitly check the boundary.

- **Camera math is correct**: `view_matrix()` correctly computes `forward = orientation * (0,0,-1)` and `up = orientation * (0,1,0)`, then delegates to `Mat4::look_at(position, position + forward, up)`. `projection_matrix()` delegates to `Mat4::perspective`. `view_projection_matrix()` returns `projection * view` (OpenGL convention). The `look_at(eye, center, up)` method correctly constructs the orientation quaternion from the look-at rotation matrix via `glm::quat_cast`.

- **Comprehensive test matrix**: 68 tests across 8 categories (Vec2, Vec3, Vec4, Mat4, Quat, Camera, Utility/Interop, Edge cases) all with `1e-5f` tolerance and headless requirements.

- **Edge cases documented**: Zero-length normalization (→ NaN), singular matrix inverse (→ NaN/inf), slerp edge cases, division by zero, degenerate perspective/ortho parameters — all specified.

- **Static assertions present**: Every wrapper type has `static_assert` for `std::is_standard_layout_v`, `sizeof` equality, and `std::is_trivially_copyable_v`.

- **GLM API usage is correct**: All GLM function calls (`glm::normalize`, `glm::length2`, `glm::cross`, `glm::mix`, `glm::inverse`, `glm::perspective`, `glm::lookAt`, `glm::angleAxis`, `glm::quat_cast`, `glm::slerp`, etc.) are used correctly.

- **CMake changes are precise**: The GLM `FetchContent` block is inserted at the correct location (after SDL3, before `find_package(OpenGL)`), tagged at `1.0.1`, and linked as PUBLIC.

- **Non-goals properly scoped**: No `.cpp` files for primitives (header-only), no rendering pipeline changes, no serialization, no SIMD, no double-precision types, no test files created by the implementation author.

## Issues

### [[BLOCKING]] (0 issues)

No blocking issues found. The contract is functionally complete and correctly implements all acceptance criteria.

### Include order convention violations

- [x] **IC-01 — mat4.h: local includes before GLM includes violates convention**  
  **Resolution**: Include order corrected: GLM headers first, then <type_traits>, then local "vec3.h"/"vec4.h" last.
- [x] **IC-02 — quat.h: local includes before GLM includes violates convention**  
  **Resolution**: Include order corrected: GLM headers first, then <type_traits>, then local "vec3.h"/"mat4.h" last.
- [x] **IC-03 — math.h: local includes before GLM includes violates convention**  
  **Resolution**: Include order corrected: `<glm/glm.hpp>` and `<glm/gtc/constants.hpp>` first, then local headers.

### noexcept convention violations

- [x] **IC-04 — Mat4 static factory methods missing `noexcept`**  
  **Resolution**: `noexcept` added to all six Mat4 factory methods (`perspective`, `ortho`, `look_at`, `translate`, `rotate`, `scale`) in both declarations and implementations.
- [x] **IC-05 — Quat static factory methods missing `noexcept`**  
  **Resolution**: `noexcept` added to all three Quat methods (`slerp`, `angle_axis`, `from_euler`).

### Verification command and documentation issues

- [x] **IC-06 — Verification command for GLM in public API is misleading**  
  **Resolution**: Done criteria #6 clarified: "should only match lines containing `.glm()`. Implementation bodies may use `glm::` freely."
- [x] **IC-07 — Test T-62 verification method ambiguous**  
  **Resolution**: Verification clarified: "scanning header declarations (e.g., grep for `glm::` excluding `.glm()` lines)."
- [x] **IC-08 — Test T-64 references non-existent `Mat4::zero()`**  
  **Resolution**: Description now directly specifies workaround: "Create a zero matrix via `Mat4{} * 0.0f`."

### Minor implementation robustness concerns

- [x] **IC-09 — camera.cpp implicitly depends on transitive include for `glm::mat3`**  
  **Resolution**: Added `#include <glm/mat3x3.hpp>` explicitly to camera.cpp.
- [x] **IC-10 — mat4.h includes `<glm/gtc/type_ptr.hpp>` but never uses it**  
  **Resolution**: Removed the unused `<glm/gtc/type_ptr.hpp>` include from mat4.h.

## Blocking issues

None. All issues are non-blocking convention/documentation/robustness items.

## Warnings

Non-blocking concerns for awareness:

- **Include order violations (IC-01, IC-02, IC-03)** — The pseudo-code contradicts the contract's own conventions table. These will not affect compilation but should be harmonized to maintain consistency. The Code Agent must choose which to follow; the conventions table is the stated rule, so the pseudo-code should be adjusted to match.
- **noexcept violations (IC-04, IC-05)** — The conventions table mandates `noexcept` for all math operations. The pseudo-code for several factory methods omits it. This is a missed optimization opportunity and a contract-internal contradiction.
- **Verification commands (IC-06, IC-07)** — Could cause false positives/confusion during CI verification.
- **Transitive include dependency (IC-09)** — Fragile; could break if GLM internal include structure changes.
- **Unused include (IC-10)** — Minor; no functional impact.

## Required changes

None — all issues are non-blocking warnings. However, the following changes are **recommended** before implementation begins:

1. Reorder includes in `mat4.h`, `quat.h`, `math.h` to match the conventions table (local includes last).
2. Add `noexcept` to `Mat4::perspective()`, `ortho()`, `look_at()`, `translate()`, `rotate()`, `scale()`, and `Quat::slerp()`, `angle_axis()`, `from_euler()`.
3. Fix the verification command in Done criteria section #6 to accurately describe what is being checked.
4. Add explicit `#include <glm/mat3x3.hpp>` to `camera.cpp`.
5. Either remove or document the `<glm/gtc/type_ptr.hpp>` include in `mat4.h`.

## Suggested improvements

Optional ideas (not required):

- **Document why `glm::mat4{1.0f}` is used in `translate`/`rotate`/`scale` factories**: The current code passes an identity matrix as the first argument to GLM's transform functions. This is correct but a brief comment explaining that `glm::translate(identity, offset)` constructs a translation matrix from scratch (not composing on an existing matrix) would improve readability.

- **Consider adding `[[nodiscard]]` to pure-query methods**: Methods like `length()`, `normalized()`, `inverse()`, `determinant()`, `view_matrix()`, `projection_matrix()` could benefit from `[[nodiscard]]` to catch silent misuses. This is optional and should be discussed.

- **Camera::view_matrix() and projection_matrix() recompute on each call**: The spec documents this as intentional (KISS). A brief comment in camera.cpp noting this (or a future TODO for caching with a dirty flag) would help maintainers.

## Verdict

**Accepted.** The contract is functionally complete, correctly implements all 20 acceptance criteria, respects the architecture boundary, and leaves no architectural decisions to the Code Agent. The Camera implementation is mathematically sound (view = inverse of transform, projection = perspective, view_projection = projection * view). GLM API usage is correct throughout.

All 10 non-blocking issues (IC-01 through IC-10) have been resolved. The contract is ready for human validation and implementation.
