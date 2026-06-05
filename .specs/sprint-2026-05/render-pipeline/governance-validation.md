# Governance Validation — SPEC-005 / IMPL-005

## Status

`Accepted with warnings`

## Summary

The render pipeline feature (SPEC-005 / IMPL-005) passes all blocking governance checks. The architecture boundary (CONST-001) is fully preserved, ADR-003 is ratified and consistent with implementation, cross-document coherence is maintained, and human approval exists for both spec and contract.

Three non-blocking issues were found: (1) the status fields in `spec.md` and `implementation-contract.md` were not updated from `In Review` to `Accepted` after human sign-off, (2) no render pipeline unit tests exist yet (a tracked gap deferred to test-author per contract), and (3) the implementation-contract-critic's note about file-count heading inconsistency remains unresolved. None of these are blocking — the feature is architecturally sound, constitution-compliant, and ready for follow-up test-author work.

## Documents reviewed

### Specs and contracts
- `.specs/sprint-2026-05/render-pipeline/spec.md` — SPEC-005 (Render Pipeline)
- `.specs/sprint-2026-05/render-pipeline/spec-critic.md` — Spec critic review
- `.specs/sprint-2026-05/render-pipeline/implementation-contract.md` — IMPL-005
- `.specs/sprint-2026-05/render-pipeline/implementation-contract-critic.md` — Contract critic review
- `.specs/sprint-2026-05/render-pipeline/code-review.md` — Code review

### Constitution
- `docs/constitution/charter.md`
- `docs/constitution/principles.md`
- `docs/constitution/rules/CONST-001-architecture-boundaries.md` (incl. AMEND-2026-001, AMEND-2026-002 SUPERSEDED)
- `docs/constitution/rules/CONST-002-testing-policy.md`
- `docs/constitution/rules/CONST-003-documentation-policy.md`
- `docs/constitution/rules/CONST-004-security-policy.md`

### ADRs
- `docs/adr/001-result-error-pattern.md`
- `docs/adr/002-glm-wrapper-math.md`
- `docs/adr/003-render-pipeline-architecture.md`

### Wiki
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/architecture/overview.md`
- `docs/wiki/domain/glossary.md`
- `docs/wiki/decisions/adr-index.md`

### Other specs (cross-references)
- `.specs/sprint-2026-05/platform-abstraction/spec.md`
- `.specs/sprint-2026-05/math-foundations/spec.md`

## Validation results

| # | Check | Result | Details |
|---|---|---|---|
| 1 | **No contradictions** | ✅ | All governance documents are coherent. No cross-document contradictions found. The only minor inconsistency (status field vs. actual approval state) is within a single document, not a cross-document contradiction. |
| 2 | **ADR-003 ratified** | ✅ | ADR-003 is marked `Accepted`. Rationale (draw methods return void; `Platform::poll_events()` instead of SDL3 in main.cpp) is fully consistent with the implemented code as verified by the code review. |
| 3 | **CONST-001 architecture boundary** | ✅ | `grep -E '(GL_|gl[A-Z]|SDL_|GLAD)'` on `shader.h`, `material.h`, `vertex_buffer.h`, `index_buffer.h`, `vertex_format.h`, `primitive_topology.h` — **zero matches**. Abstract headers have no backend types. |
| 4 | **CONST-002 tests specified** | ⚠️ | 28 tests (RP-T-01 through RP-T-28) are specified in the implementation contract. **No test files exist** in `tests/`. The contract explicitly defers test creation to a test-author. This is a tracked gap — see Issue #2. |
| 5 | **Constitution principles** | ✅ | **Explicit contracts**: SPEC-005 and IMPL-005 are highly detailed. **Small scoped changes**: Follows existing abstraction patterns. **Existing conventions**: Uses `Result<T>`, `buddd::engine` namespace, non-copyable/non-movable, trailing return types. **Testable requirements**: All ACs have specific verification methods. |
| 6 | **Wiki consistency** | ✅ | Module map, overview, glossary, and ADR index accurately reflect the implementation. All render pipeline files, types, and behaviors are documented correctly. |
| 7 | **Approval sections** | ✅ | Both `spec.md` and `implementation-contract.md` have human approval entries (Guillaume, 2026-05-29). |
| 8 | **Architecture boundary (grep)** | ✅ | `grep -E '(GL_|gl[A-Z]|SDL_|GLAD)'` on all 6 abstract headers — **zero matches**. See #3 above. |
| 9 | **Headless independence (grep)** | ✅ | `grep -E '(GL_|gl[A-Z]|SDL_)'` on `src/engine/render/*_headless.*` — **zero matches**. All 10 headless files (`shader_headless.h/cpp`, `material_headless.h/cpp`, `vertex_buffer_headless.h/cpp`, `index_buffer_headless.h/cpp`, `render_device_headless.h/cpp`) are clean. |
| 10 | **main.cpp cleanliness (grep)** | ✅ | `grep -E '(SDL_|GL_)' src/cmd/main.cpp` — **zero matches**. `main.cpp` uses only abstract `Platform::poll_events()`. The previous CONST-001 violation (W-01) is resolved. |
| 11 | **Spec status correctness** | ⚠️ | `spec.md` status is `In Review` but the human has approved it (Approval section filled) and the wiki lists it as `Accepted`. Should be `Accepted`. See Issue #1. |
| 12 | **Contract status correctness** | ⚠️ | `implementation-contract.md` status is `In Review` but same issue as above — human approval exists. Should be `Accepted`. See Issue #1. |
| 13 | **File counts in contract** | ⚠️ | The implementation-contract-critic review (W-01) flagged that headings say "20 new files" and "5 modified files" but the actual lists contain 22 new and 6 modified files. Unresolved in current documents. See Issue #3. |
| 14 | **CONST-003 / CONST-004** | ✅ | These rules are still `TODO` placeholders. No render pipeline content contradicts them (they have no substantive rules yet). Not an issue for this validation. |

## Issues

### (Non-blocking) Issue #1: Status fields in spec.md and implementation-contract.md not updated to `Accepted`

**Files**: `.specs/sprint-2026-05/render-pipeline/spec.md` (line 5), `.specs/sprint-2026-05/render-pipeline/implementation-contract.md` (line 5)

**Description**: Both documents have status `In Review` despite having human approval entries (Guillaume, 2026-05-29) indicating acceptance. The "Allowed values" line in both documents also has a typo — `Accepted` appears twice. The wiki (`docs/wiki/decisions/adr-index.md`) correctly lists both SPEC-005 and IMPL-005 as `Accepted`.

**Impact**: Low. The human approval in the Approval section is the authoritative signal. The status field is cosmetic but inconsistent. The code review marks the implementation as `Accepted`, confirming the intent.

**Recommended fix**: Update the status from `In Review` to `Accepted` in both `spec.md` and `implementation-contract.md`. Fix the "Allowed values" to list each value once.

---

### (Non-blocking) Issue #2: No render pipeline unit tests exist (CONST-002 gap)

**Files**: `tests/` — no render pipeline test files exist

**CONST-002 rule**: "All testable code added or modified in this project must have corresponding unit tests. Those tests must pass (i.e., the code must work)."

**Description**: 28 tests (RP-T-01 through RP-T-28) are specified in the implementation contract's "Required tests" section (lines 1912–1947), covering every error case, normal path, and edge case. The contract explicitly states "The implementation-author does NOT create test files" (line 1914), deferring them to a test-author. As of this review, no test files for the render pipeline exist in `tests/`.

**Status**: This is a **tracked gap** with the human's awareness and approval (the contract was accepted with this deferral). The code review (W-02) flags the same issue. No blocking because the contract explicitly scopes this out.

**Impact**: Without these tests, the render pipeline has no automated regression coverage. CONST-002 is technically not fully satisfied, but the gap is known, accepted by the human, and scheduled for a test-author.

**Recommended fix**: A test-author must create the required test files for RP-T-01 through RP-T-28. Tracked in code-review as W-02.

---

### (Non-blocking) Issue #3: File count headings inconsistent in implementation contract

**File**: `.specs/sprint-2026-05/render-pipeline/implementation-contract.md`

**Description**: The implementation-contract-critic review (W-01) flagged that:
- The heading says "New files to create (20 files)" but 22 files are listed (items 1–22).
- The heading says "Files to modify (5 files)" but 6 files are listed (items 23–28).
- The total says "22 new files + 5 modified files = 27 files changed" but the correct sum is 22 + 6 = 28.

The actual file lists and numbering (1–22 new, 23–28 modified) are correct. Only the headings and totals are wrong.

**Impact**: Very low. An implementer following the numbered lists will create/modify the correct files.

**Recommended fix**: Update the headings and totals to match the actual file counts.

---

### (Non-blocking) Issue #4: OpenGL backend uses `<SDL3/SDL_opengl.h>` instead of `<GL/gl.h>`

**Files**: `src/engine/render/shader_opengl.h`, `src/engine/render/material_opengl.h`, `src/engine/render/vertex_buffer_opengl.h`, `src/engine/render/index_buffer_opengl.h`, `src/engine/render/render_device_opengl.cpp`

**Description**: The implementation contract specifies using `<GL/gl.h>` for OpenGL includes (line 187 of contract: "Use `<GL/gl.h>` for OpenGL types"). All OpenGL backend files use `<SDL3/SDL_opengl.h>` instead. Both headers provide identical GL type declarations, but this deviates from the contract's convention and bypasses CMake's `find_package(OpenGL)` dependency tracking.

**Impact**: Low — functionally correct. Flagged in code review as W-03.

**Recommended fix**: Either replace `<SDL3/SDL_opengl.h>` with `<GL/gl.h>` across all OpenGL backend files, or update the contract convention to acknowledge the SDL3 variant as acceptable.

---

### (Non-blocking) Issue #5: Minor code quality items from code review

**Description**: Several non-blocking code quality issues were identified in the code review (W-04 through W-06): headless draw debug output prints `"?"` instead of actual values, `VertexBufferOpenGL` stores an unused `byte_size_` field, and `IndexBufferOpenGL` has a redundant `index_type()` method. None affect correctness or architecture boundary compliance.

**Impact**: Very low. Cosmetic and optimization items.

**Recommended fix**: Address in follow-up cleanup work per code review suggestions.

## Verdict

**`Accepted with warnings`**

The render pipeline feature (SPEC-005 / IMPL-005) is governance-compliant. All blocking checks pass:

- ✅ Architecture boundary (CONST-001) fully preserved — abstract headers are backend-type-free, headless files have no SDL3/OpenGL, `main.cpp` uses `Platform::poll_events()` with no SDL3 includes.
- ✅ ADR-003 ratified and consistent with implementation.
- ✅ Human approval on both spec and contract.
- ✅ Cross-document coherence maintained — wiki accurately reflects implementation, constitution principles respected.
- ✅ Draw-methods-void exception to ADR-001 properly documented and justified in both spec and ADR-003.
- ✅ AMEND-2026-002 marked SUPERSEDED with reference to ADR-003.

The five non-blocking issues (status field inconsistency, missing tests per CONST-002, contract file-count headings, OpenGL include convention deviation, minor code quality items) should be addressed in follow-up work but do not block the feature.

The most significant open action is **Issue #2**: the 28 specified render pipeline tests (RP-T-01 through RP-T-28) must be created by a test-author before the feature is considered fully verified.
