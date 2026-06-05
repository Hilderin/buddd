# Governance Review — SPEC-004 Math Foundations (Vec2, Vec3, Vec4, Mat4, Quat, Camera)

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

---

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **21/21 ACs satisfied** — Spec defines AC-001 through AC-021; implementation contract implements all 21; code review confirms all 21 are met. The actual source files (`src/engine/math/*.h`, `src/engine/math/camera.cpp`) match the contract pseudo-code with only functionally equivalent divergences (see Warnings).

- [x] **Spec → Contract alignment** — The spec's API listings, user stories, acceptance criteria, and edge cases are faithfully reproduced in the implementation contract. All four re-review findings from spec-critic (AC-010 wording, normalize() edge case unification, Vec3*Mat4 semantics, AC-006 forward definition) are resolved.

- [x] **Contract → Code alignment** — All 9 required files exist with correct content. The code review verifies every public method, static assert, and GLM delegation pattern against the contract pseudo-code.

- [ ] **Implementation contract internal contradiction: "Files forbidden to change" vs "Required tests"**  
  The contract's "Files forbidden to change" section lists `tests/` (any file) as forbidden, but its "Required tests" section mandates 71 tests that must be compiled into the `buddd_tests` binary. This requires modifying `tests/CMakeLists.txt` to register `math_test.cpp`. The code review (Warning #1) notes this as a procedural contradiction.  
  **Impact:** Non-blocking — the modification was necessary and pragmatically correct.

- [ ] **Test file includes GLM headers directly**  
  The spec's architecture boundary states: "Test files in `tests/` may include the math wrapper headers but NOT GLM headers directly." The actual `tests/math_test.cpp` includes `<glm/glm.hpp>`, `<glm/gtc/quaternion.hpp>`, `<glm/vec2.hpp>`, etc. directly (lines 3–11). This is necessary for the verification pattern (comparing wrapper output against GLM reference results), and the spec acknowledges this with "No automated guard at this stage" (AC-015) and "Code review catches violations" (AC-015 verification). However, this represents a design tension between test verification requirements and the stated architecture boundary.  
  **Impact:** Non-blocking — acknowledged limitation, no constitution violation (CONST-001 covers platform/graphics/windowing libraries, not GLM).

- [ ] **Camera matrix methods lack `noexcept`**  
  The implementation contract's conventions table states: "All math operations are `noexcept`." However, `Camera::view_matrix()`, `Camera::projection_matrix()`, and `Camera::view_projection_matrix()` are not marked `noexcept`. The spec and contract declarations also omit `noexcept` for these methods, so the implementation is consistent with the explicit API surface — but inconsistent with the stated convention.  
  **Impact:** Non-blocking — minor convention inconsistency.

- [x] **Minor implementation divergences (functionally equivalent)**  
  - `normalize()` uses `glm()` accessor style (`glm() = glm::normalize(glm())`) vs contract's `reinterpret_cast` pattern.  
  - `length_squared()` uses manual `x*x + y*y + ...` vs contract's `glm::length2()` delegation.  
  Both produce identical results. Marked as non-issues by code review.

---

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries) — Compliant**  
  - GLM headers are only included inside `src/engine/math/` in engine source code. Verified via `grep -rn 'glm/' src/engine/ --include='*.h' --include='*.cpp' | grep -v 'src/engine/math/' | grep -v 'CMakeLists.txt'` — zero matches outside `src/engine/math/`.
  - `camera.h` has zero GLM includes — only uses the wrapper types (`Vec3`, `Quat`, `Mat4`).
  - CONST-001 specifically targets "platform, graphics, or windowing library headers (e.g., `<SDL3/`, `<GL/`, `<glad/>`)." GLM is a math library and is not covered by CONST-001. The stricter boundary (no GLM in public API) is a self-imposed spec requirement, not a constitutional requirement.

- [x] **CONST-002 (Testing Policy) — Compliant**  
  - 71 test cases (T-01 through T-71) covering all math operations, utility functions, interop, and edge cases.
  - Every method on every type is exercised.
  - Edge cases (NaN on zero-length normalization, singular matrix inverse, slerp edge cases, division by zero, degenerate parameters) are tested.
  - All tests use `1e-5f` tolerance as required.
  - Tests are headless (no display, no GPU required).

- [ ] **CONST-003 (Documentation Policy) — Cannot evaluate (placeholder)**  
  CONST-003 is a TODO placeholder ("Rule: TODO. Rationale: TODO."). No compliance assessment is possible. This is a pre-existing condition, not caused by SPEC-004.

- [ ] **CONST-004 (Security Policy) — Cannot evaluate (placeholder)**  
  CONST-004 is a TODO placeholder ("Rule: TODO. Rationale: TODO."). The SPEC-004 math types are pure computation with no I/O, networking, filesystem access, or elevated privileges — no security concerns exist regardless. This is a pre-existing condition.

- [x] **Engineering Principles (principles.md) — Compliant**  
  - "Prefer explicit contracts over implicit assumptions" — the spec, contract, pseudo-code, tests, and acceptance criteria are explicit throughout.
  - "Prefer small scoped changes over broad rewrites" — the implementation touches only `src/engine/math/` and `src/engine/CMakeLists.txt` (plus required test registration).
  - "Prefer existing conventions over new patterns" — naming conventions match SPEC-001/SPEC-002 (`snake_case` files, PascalCase types, trailing return types, `#pragma once`).
  - "Prefer testable requirements over vague intent" — all ACs specify concrete tolerance thresholds (`1e-5f` vs GLM).
  - "Governance documents must not contradict each other" — see cross-document coherence findings above; no contradictions between separate governance documents.

---

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result/Error Pattern) — Consistent**  
  The implementation contract explicitly addresses ADR-001: "Math functions are pure computation with no error returns, so `Result<T>` is NOT used in the math layer." This falls squarely under ADR-001's "Where this does NOT apply" clause: "Functions that cannot logically fail (pure getters, predicates, trivial computations) — these return plain values." The math types return NaN/inf on invalid input (division by zero, singular matrix inverse, degenerate parameters) rather than using `Result<T>` — consistent with GLM's behavior and the thin-wrapper design.

- [x] **No new ADR required**  
  The patterns used (thin GLM wrappers, header-only design, `FetchContent` for GLM, Camera as perspective view) are design decisions documented in the spec. The implementation contract's ADR impact section correctly states: "No architectural decision requires an ADR."

---

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/architecture/overview.md` — Updated**  
  The overview already reflects the math module:
  - Directory listing includes `src/engine/math/` with all 8 files.
  - Build system section lists "GLM (fetched via `FetchContent`)" as a dependency.
  - Architecture boundary section describes GLM boundary and `.glm()` accessor.
  - References SPEC-004 and IMPL-004.
  - Engine library internal structure diagram includes all math files.

- [ ] **`docs/wiki/architecture/dependency-map.md` — Missing GLM entry**  
  The dependency map table lists Catch2, SDL3, and OpenGL but does NOT include GLM. The GLM entry should be added with version `1.0.1`, source `https://github.com/g-truc/glm.git`, fetch method `CMake FetchContent`, and link type `PUBLIC`.  
  **Impact:** Non-blocking — the overview.md has the correct information; dependency-map.md is lagging.

- [x] **`docs/wiki/architecture/module-map.md` — Not updated (acceptable)**  
  The module map does not yet list the math module. However, the overview.md supersedes it for structural documentation. A future wiki update should add the math module to the module map.

- [x] **Wiki does not become law** — The wiki README explicitly states: "The wiki describes the current operational understanding of the system. It is not a source of mandatory rules." No constitutional contradictions arise from wiki content.

---

## Warnings

Non-blocking concerns for awareness:

1. **Implementation contract internal contradiction** — Section "Files forbidden to change" forbids modifying `tests/`, but the "Required tests" section requires 71 tests that necessitate modification of `tests/CMakeLists.txt`. The code was correctly implemented despite this contradiction.

2. **Test file includes GLM headers directly** — `tests/math_test.cpp` includes `<glm/glm.hpp>` and other GLM headers. This is necessary for the verification pattern (comparing wrappers against GLM reference) but violates the spec's stated architecture boundary (AC-015). The spec acknowledges this limitation ("No automated guard at this stage").

3. **Camera matrix methods lack `noexcept`** — `view_matrix()`, `projection_matrix()`, and `view_projection_matrix()` are not `noexcept`, inconsistent with the contract convention "All math operations are `noexcept`." These are pure computation methods and could be `noexcept`.

4. **Wiki `dependency-map.md` missing GLM** — The dependency map does not list GLM despite it being a new PUBLIC dependency of `buddd_engine`. The overview.md is correct, but the dependency map is incomplete.

5. **CONST-003 and CONST-004 remain placeholder TODOs** — Pre-existing condition. These constitution rules have no normative content, so no compliance check is possible. Not caused by SPEC-004.

6. **Approval "Time" field** — Both the spec and implementation contract approval sections use `(approved)` for the Time field rather than an actual timestamp. This is a cosmetic formatting choice and does not affect validity.

---

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **`docs/wiki/architecture/dependency-map.md`** — Add GLM entry to the external dependencies table:
  - Dependency: GLM
  - Version: 1.0.1
  - Source: `https://github.com/g-truc/glm.git`
  - Fetch method: CMake `FetchContent`
  - Link type: PUBLIC (to `buddd_engine`)

- **`docs/wiki/architecture/module-map.md`** — Add the math module section under `buddd_engine`:
  - Directory: `src/engine/math/`
  - Files: `math.h`, `vec2.h`, `vec3.h`, `vec4.h`, `mat4.h`, `quat.h`, `camera.h`, `camera.cpp`
  - Description: Linear algebra wrappers around GLM

- **`docs/wiki/engineering/testing.md`** — Add reference to math test file `tests/math_test.cpp` in the test table, listing the 71 math test cases.

These updates are non-blocking recommendations. The wiki overview.md is already accurate and the most authoritative wiki document.
