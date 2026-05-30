# Governance Review — Scene Graph (SPEC-008 / IMPL-008)

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

This is the final governance gate for the scene graph feature. All prior workflow steps completed successfully:
- **SPEC-008**: Accepted (spec-critic 3rd review, no blocking issues)
- **IMPL-008**: Accepted with warnings (implementation-contract-critic 6th review, no blocking issues)
- **Code**: Accepted (code review, no blocking issues)
- **Tests**: All 49 tests pass

The implementation is complete, correct, and constitutionally compliant. All 32 acceptance criteria are covered (31 tested, 1 documentation-only). Cross-document coherence is strong — the spec, contract, code, tests, and wiki are mutually consistent.

**No blocking issues exist.** Three non-blocking warnings are noted, of which two are pre-existing documentation debts unrelated to scene graph.

---

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec (SPEC-008) vs Implementation Contract (IMPL-008)**: All 32 ACs from the spec are explicitly mapped to 49 tests in the contract. Coverage is complete: AC-001 through AC-031 are tested; AC-032 is documentation-only (dangling Component pointer is UB). ✓
- [x] **Implementation Contract vs Code**: Every method signature, `noexcept` specification, storage strategy (`vector<unique_ptr<Component>>` per node), iterative traversal (`mark_for_destroy` with explicit stack), deferred destruction order (pre-order mark, reverse-order flush), and transformation math matches the contract. ✓
- [x] **Code Review vs Code**: All claims verified. N-01 (weak AC-028 order test) matches the actual test code. N-02 (unused `<span>`) confirmed present. N-03 (redundant `EntityId::operator!=`) confirmed present. N-04 (`flush_destroyed noexcept` with `push_back`) confirmed. N-05 (const-correctness nuance in `Entity::get_component() const`) confirmed. N-06 (public `get_transform`/`is_pending_destroy`) confirmed. ✓
- [x] **Spec API listing vs A-10 (noexcept residual)**: The spec's `Transform::local_matrix()` and `Transform::world_matrix()` API listings (lines 116, 122) still omit `noexcept` despite A-10 (line 603) claiming they are `noexcept`. The contract and code correctly use `noexcept`. This is a minor spec-level inconsistency that does not affect implementation correctness. (Pre-existing from spec-critic W-01; not re-opened for this review.)

- [ ] **ADR-001 internal compiler baseline contradiction** — ADR-001 line 17 correctly states "GCC 16+, Clang 22+" but line 97 still says "GCC 14+, Clang 19+". This is an internal contradiction within a governance document (authority level 3). It does not affect scene graph (all scene-relevant documents use the correct GCC 16+/Clang 22+ baseline), but it creates ambiguity for future features. *Non-blocking — see Warnings.*

- [ ] **Wiki ADR index missing ADR-002, ADR-003, ADR-004** — `docs/wiki/decisions/adr-index.md` states "All accepted ADRs are listed at the top of this page" but omits ADR-002 (GLM wrapper math), ADR-003 (Render pipeline architecture), and ADR-004 (Demo system architecture). *Non-blocking — see Warnings.*

---

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)**: All scene graph types live under `src/engine/scene/`. No GLM, SDL3, or OpenGL headers are included in any scene header. Scene headers depend only on math wrappers (`Vec3`, `Quat`, `Mat4`) from `src/engine/math/` and standard C++ headers. Architecture boundary is fully respected. ✓
- [x] **CONST-002 (Testing Policy)**: All testable code has corresponding unit tests. 49 tests cover all 32 ACs. Tests pass. Test file is headless (no display/GPU required) and compiled in both `BUDDD_HAS_DISPLAY` branches. ✓
- [x] **CONST-003 (Documentation Policy)**: Rule is still TODO. Not applicable to scene graph. No violation. ✓
- [x] **CONST-004 (Security Policy)**: Rule is still TODO. Scene graph is pure memory management and spatial computation — no I/O, no secrets, no privileges. No violation. ✓
- [x] **Engineering Principles**: No contradictions. Prefers existing conventions (trailing returns, `#pragma once`, `snake_case`). Testable requirements. Small scoped changes (7 new files, 1 modified). ✓

None of the TODO constitution rules (CONST-003, CONST-004) affect scene graph governance. They are pre-existing governance gaps outside this feature's scope.

---

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result\<T\> pattern)**: The scene graph's non-use of `Result<T>` is a documented, justified exception — identical to the precedent set by `draw()`/`draw_indexed()` in the render pipeline (IMPL-005). Spec A-02 and contract line 38 provide the rationale. ✓
- [x] **ADR-005 (`std::optional<T&>` component API)**: The `get_component<T>()` return type uses `std::optional<T&>` as specified. ADR-005 correctly states the compiler baseline (GCC 16+, Clang 22+) matching cppreference. ✓
- [x] **ADR-006 (RTTI component dispatch)**: `get_component<T>()` and `remove_component<T>()` use `dynamic_cast<T*>()` as specified. The `Component` base class is minimal (virtual destructor only). ✓
- [x] **ADR cross-references**: ADR-005 references ADR-001 for compiler baseline. ADR-006 references ADR-005 for return types and ADR-001 for error pattern. All consistent. ✓
- [x] **No new ADR required**: The exception to ADR-001 follows the existing pattern from the render pipeline. No additional ADR is needed. ✓

---

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **Architecture overview** (`docs/wiki/architecture/overview.md`): Correctly lists scene graph types, deferred destruction, RTTI dispatch, `std::optional<T&>`, and architecture boundary. Links SPEC-008 and IMPL-008. ✓
- [x] **Module map** (`docs/wiki/architecture/module-map.md`): Lists all scene files with accurate descriptions, namespace (`buddd::engine`), file roles, template method locations, and the test range T-01 through T-49. ✓
- [x] **ADR index** (`docs/wiki/decisions/adr-index.md`): Lists ADR-001, ADR-005, ADR-006 with correct summaries. Scene graph decisions table is accurate. ⚠️ Missing ADR-002, ADR-003, ADR-004 from the full listing (see Warnings).
- [x] **Glossary** (`docs/wiki/domain/glossary.md`): Defines all scene graph terms (World, Entity, EntityId, Transform, Component, deferred destruction, pending-destroy, etc.) correctly. ✓
- [x] **Testing** (`docs/wiki/engineering/testing.md`): Correctly describes all 49 test cases grouped by category, with tags, test ranges, and constitution reference. ✓
- [x] **Wiki does not contradict governance**: All wiki content is subordinate to the constitution and specs. No wiki section asserts rules not found in higher-authority documents. ✓

---

## Warnings

Non-blocking concerns for awareness:

1. **ADR-001 internal compiler baseline contradiction (W-08 carried)**: Line 97 of ADR-001 still reads "GCC 14+, Clang 19+, MSVC 2025+" while line 17 correctly reads "GCC 16+, Clang 22+". This predates scene graph and was noted in the implementation-contract-critic's 6th review. Does not affect scene graph correctness, but should be fixed for governance hygiene across future features.

2. **Wiki ADR index incomplete**: `docs/wiki/decisions/adr-index.md` states "All accepted ADRs are listed" but omits ADR-002, ADR-003, and ADR-004. These are older ADRs outside the scene graph scope. Should be added for completeness.

3. **Spec API listing missing `noexcept` on Transform methods** (spec-critic W-01, never corrected in spec): `Transform::local_matrix()` and `Transform::world_matrix()` API listings at spec.md lines 116/122 lack `noexcept` despite A-10 and the contract requiring it. The code correctly implements `noexcept`. This is a spec documentation issue only.

4. **CONST-003 and CONST-004 are still TODO** (pre-existing): No actual rules exist under these constitution files. They have `Blocking` enforcement but empty bodies. Does not affect scene graph — applies project-wide.

---

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

1. **ADR-001 line 97**: Update "GCC 14+, Clang 19+" to "GCC 16+, Clang 22+" to match line 17 and the project's actual compiler baseline.
2. **Wiki ADR index**: Add ADR-002, ADR-003, ADR-004 to the "All accepted ADRs" listing at the top of `docs/wiki/decisions/adr-index.md`.
3. **Spec-008 API listing**: Add `noexcept` to `Transform::local_matrix() const` and `Transform::world_matrix(const Entity&) const` at spec.md lines 116 and 122 for consistency with A-10.

These are non-urgent documentation cleanups. None are blocking for the scene graph implementation.

---

## Verdict

| Check | Outcome |
|---|---|
| Spec matches human intent | ✓ |
| Contract matches accepted spec | ✓ |
| Code matches accepted contract | ✓ |
| Tests prove acceptance criteria | ✓ (31 tested, 1 doc-only) |
| Constitution not violated | ✓ |
| Required ADRs exist | ✓ (ADR-005, ADR-006) |
| Wiki reflects current state | ✓ (minor omissions — see Warnings) |
| Cross-document coherence | ✓ (minor residual: ADR-001 line 97 inconsistency, spec API listing missing noexcept) |
| **Verdict** | **Accepted** |

The scene graph feature is fully implemented, correctly governed, and passes all verification gates. No blocking issues exist. The three warnings and two optional governance updates noted above are pre-existing or minor documentation debts that do not affect the correctness or safety of the implementation.

**Accepted** — the feature is ready for use.
