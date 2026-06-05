# Spec Review — model-multi-material (SPEC-020) — Re-review (v4, loop-back resolution)

This review is a fresh re-review triggered by the human's explicit request to re-run all
critics before human-validated implementation. The spec-author has resolved BL-001 and BL-002
from the previous review. This review verifies all Definition of Ready criteria, checks for
new issues not caught previously, and validates consistency with CONST-001, ADR-001,
ADR-003, ADR-010, and existing project conventions.

## Prior review resolution

The previous spec-critic review (v3, loop-back) found two blocking issues (BL-001, BL-002).
The spec-author has resolved both. All previously resolved items remain resolved:

### Before this cycle (pre-v3 resolutions)
- [x] `add_submesh()` return type — `add_submesh()` removed entirely.
- [x] Missing "Documents requiring updates" inventory — Now present.
- [x] Non-indexed null-material draw undefined — Non-indexed removed.
- [x] Non-indexed two-phase creates a dead-end code path — Two-phase API removed.

### Resolved this cycle (v3 → v4)
- [x] **BL-001: Test file migrations not listed.** Now documented in migration table
      (lines 270--272) and "Documents requiring updates" (lines 413--415).
- [x] **BL-002: Missing edge case for index range OOB.** Now documented in Edge cases
      table (line 402).

## Definition of Ready criteria audit

### Clarity & Completeness

- [x] **Scope is clearly defined (what is included and what is explicitly excluded)**
  - Goals (31–41), Non-goals (43–55), Out of scope (421–431) are thorough and explicit.

- [x] **Dependencies on other features, modules, or external systems are identified**
  - SPEC-009 (superseded), RenderDevice, Material, Error categories. CMake GLOB_RECURSE assumption (A-06).

- [x] **Edge cases and error conditions are described**
  - 11 edge cases (392–402), draw() pseudocode with fallback logic (158–171), factory validation (152–154).
  - Covers empty submeshes, empty materials, null materials, OOB material_index, moved-from models,
    buffer creation failures, multiple sharing, duplicate creation, undefined-behavior cases.

- [x] **The expected behavior is unambiguous and testable**
  - SubMesh struct exactly specified (71–77). Full Model API with signatures (86–144).
  - draw() pseudocode (158–164). Factory behavior (147–155). 24 ACs with concrete verification.

### Verification

- [x] **The spec defines how the feature will be verified end-to-end**
  - E2E Verification section (380–383): run existing demos, run `buddd run multi-material`,
    headless test suite.

- [x] **Acceptance criteria are specific, measurable, and verifiable**
  - 24 ACs (AC-001–AC-024) with concrete verification methods:
    compile checks, headless tests, code review, CLI execution.

- [x] **Success and failure states are described**
  - Factory returns `Result<Model>` with `InvalidArgument` or `ResourceCreationFailed`.
  - draw() is void (consistent with ADR-003). Fallback handles null/OOB.
  - Edge cases table documents 11 scenarios.

### Documentation

- [x] **Interface changes (CLI flags, API signatures, config keys) are documented**
  - SubMesh struct, full Model API, primitive helpers, `RenderDevice::fallback_material()`,
    multi-material demo registration. All with exact C++ signatures.

- [x] **Existing documentation that must be updated is listed**
  - SPEC-009, wiki module-map, wiki glossary, ADR-010, all app files, and all three
    affected test files (lines 405--415). BL-001 is now resolved.

### Technical

- [x] **Technical constraints are identified (system APIs, libraries, build changes)**
  - No new external dependencies. New files listed. CMake auto-discovery (A-06).
  - Uses existing error categories.

- [x] **Risks or unknowns are surfaced**
  - Breaking change acknowledged. Full migration required. 6 assumptions documented (A-01–A-06).

- [x] **Performance or resource implications, if any, are noted**
  - N submeshes = N draw calls per draw(). No artificial submesh limit. No optimization in scope.

## Consistency checks

| Check | Result | Notes |
|---|---|---|
| **CONST-001 (architecture boundaries)** | ✅ Compliant | Model and primitives in `src/engine/render/`. Demos in `src/cmd/apps/`. No backend leakage. |
| **ADR-001 (Result pattern)** | ✅ Compliant | `create_indexed()` returns `Result<Model>`. `draw()` void per ADR-003 exception. Primitive helpers return `Result<Model>`. |
| **ADR-010 (no raw pointers)** | ✅ Compliant | `shared_ptr<Material>`, `std::span<const std::byte>`, const-ref accessors, `Material&` fallback. No raw `T*` in new/changed public API. |
| **ADR-003 (draw returns void)** | ✅ Compliant | `draw()` remains void. No change. |
| **SPEC-009 supersession** | ✅ Correct | SPEC-009's single-material Model, `create()`, `has_indices()`, `material()`, `CubeResources`, `setup_cube/triangle` fully superseded. |
| **Error category reuse** | ✅ Correct | `InvalidArgument`, `ResourceCreationFailed` cover all factory errors. No new categories needed. |
| **Migration table accuracy** | ✅ Complete | App files listed correctly. Test files now listed (lines 270--272). BL-001 resolved. |

## Blocking issues

- [x] **BL-001: Test file migration not addressed.** — RESOLVED. Test files now listed in
  migration table (lines 270--272) and Documents requiring updates (lines 413--415).

- [x] **BL-002: Unindexed submesh index-range OOB not listed as undefined behavior.** — RESOLVED.
  Edge case added (line 402).

### New blocking issues (this review cycle)

**None.** The spec resolves all previously identified blocking issues. A fresh review of all
Definition of Ready criteria, consistency checks with CONST-001, ADR-001, ADR-003, ADR-010,
and a thorough search for new issues found **no new blocking issues**.

## Warnings (non-blocking)

- **AC-006 verification depends on implicit test infrastructure**: The verification for AC-006
  ("verify materials used match expected via material tracking") requires a mechanism on the
  headless RenderDevice to track which material was bound during each draw call. This is not
  described in the spec. While implementable, it is an implicit dependency on test infrastructure
  that may not exist yet.

- **AC-014 describes visual behavior not verifiable headless**: AC-014 says the fallback material
  "renders magenta" but the verification only checks "it's a valid material." The RGB color output
  cannot be verified in a headless test. This is acceptable (color is an implementation invariant
  checkable by code review) but the AC text is slightly imprecise.

- **Old `create_indexed()` callers in non-test code (RESOLVED for test files)**: The previous
  warning about callers of the old single-material `create_indexed()` signature is now resolved
  for test files (covered by BL-001 fix at lines 270--272). Remaining engine-level callers
  (`demo_helpers.cpp` → `create_cube()`) and the "manual" app entries are covered by the
  migration table's intent. No further spec changes needed.

### New warnings (this cycle)

- **No AC for empty-materials vector fallback**: The edge case "Empty materials vector → All
  submeshes use fallback (magenta)" is documented but has no dedicated AC. AC-005 (N draw calls)
  and AC-007/AC-008 (fallback behavior) partially cover it. Consider adding an AC or note that
  this is implicitly covered by the fallback mechanism.

- **No AC for invalid vertex format**: The factory behavior section states validation of "valid
  vertex format" returning `InvalidArgument`, but no AC tests this. Existing behavior from
  current code; minor gap.

## Questions for human

none

## Summary

The spec is well-structured, comprehensive, and now satisfies **all** Definition of Ready
criteria after the spec-author resolved both previous blocking issues (BL-001, BL-002).

**Resolution verification:**
- **BL-001** ✅ — Test files (`tests/model_tests.cpp`, `tests/lighting_tests.cpp`,
  `tests/scene_rendering_tests.cpp`) are now listed in the migration table (lines 270--272)
  and "Documents requiring updates" (lines 413--415).
- **BL-002** ✅ — The edge case `submesh.index_start + submesh.index_count exceeds index buffer
  length` is now listed in the Edge cases table (line 402).

**Fresh re-review findings:**
- No new blocking issues identified.
- The spec is consistent with CONST-001, ADR-001, ADR-003, ADR-010, and existing codebase
  conventions (module-map, glossary).
- All 12 Definition of Ready criteria are satisfied.
- Previous warnings (AC-006 test infrastructure dependency, AC-014 magenta imprecision)
  remain as non-blocking observations. Two minor new warnings noted (no AC for empty-materials
  fallback, no AC for invalid vertex format).

The spec is **ready** for human validation and implementation contracting.

## Suggested improvements (optional)

1. Add explicit cross-references to ADR-001 (`Result<T>` pattern) and ADR-010 (no raw pointers)
   in the spec body to demonstrate compliance, since these are checked during review.
2. Shorten the "Out of scope" section (lines 421–431) since it largely duplicates "Non-goals"
   (lines 43–55). Consider removing redundancy or merging.
