# Governance Review — model-multi-material (SPEC-020) — FINAL VALIDATION

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **SPEC-010 number conflict — RESOLVED**: The model-multi-material spec originally claimed SPEC-010, conflicting with the capture spec (also SPEC-010). Renamed to SPEC-020. All cross-references in spec.md, implementation-contract.md, spec-critic.md, implementation-contract-critic.md, code-review.md, coordination.md, ADR-017, and module-map.md verified as using SPEC-020/IMPL-020. No stale SPEC-010/IMPL-010 references remain (only historical mentions in coordination.md governance-reviewer narrative).

- [x] **Module map missing spec reference — RESOLVED**: Module-map reference section now includes SPEC-020 + IMPL-020 entries (line 340-341), properly ordered numerically after SPEC-019. Capture feature correctly referenced as SPEC-010 (not SPEC-020).

- [x] **ADR-013 compliance tension — RESOLVED**: ADR-013 now includes an explicit "Exception: Primitive helpers and low-level API usage" section (lines 119-132) permitting custom 24-byte vertex formats in `primitives.h/.cpp` while maintaining the standard 72-byte `Vertex` requirement for the engine-managed path. Also correctly references SPEC-020/ADR-017 (line 66).

- [x] **ADR-013's expectation about demo_helpers — RESOLVED**: ADR-013's "How existing demos were updated" section updated (lines 64-67) to reflect removal of `setup_cube()`/`setup_triangle()` and their replacement by engine-level primitive helpers. The stale compliance directive was replaced by the new exception section.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **No constitution violations found.** Specifically:
  - CONST-001 (architecture boundaries): ✅ Compliant — Model and primitives in `src/engine/render/`, apps in `src/cmd/apps/`. No backend headers leak.
  - CONST-002 (testing policy): ✅ Compliant — 317 tests passing, all ACs covered.
  - CONST-003 (documentation policy): ✅ Compliant — All required documentation exists (spec, contract, ADR, wiki updates).
  - CONST-004 (security policy): ✅ Not applicable — No security implications.
  - Engineering principles: ✅ Compliant — Explicit contracts, existing conventions respected (`Result<T>`, `shared_ptr`, `unique_ptr`), testable requirements.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-017 (multi-material-model)**: ✅ Exists, `Accepted`. Accurately references SPEC-020 (line 200). Covers all 7 key decisions with rationale, alternatives, and consequences. Complete.
- [x] **ADR-001 (Result pattern)**: ✅ Compliant — `create_indexed()` returns `Result<Model>`, `draw()` returns `void` per ADR-003 exception.
- [x] **ADR-003 (draw returns void)**: ✅ Compliant — `Model::draw()` returns void.
- [x] **ADR-010 (no raw pointers)**: ✅ Compliant — `shared_ptr<Material>`, `std::span`, const-ref accessors. No raw `T*` in new/changed public API.
- [x] **ADR-013 (standard vertex format)**: ✅ Updated with explicit exception for primitive helpers and low-level API usage (lines 119-132). Correctly references SPEC-020/ADR-017 (line 66). No remaining compliance tension.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **module-map.md**: ✅ Updated correctly. Render submodule entries for `model.h`, `model.cpp`, `primitives.h`, `primitives.cpp` are accurate. `multi_material_app` entry exists. `demo_helpers` marked as empty placeholders.
- [x] **glossary.md**: ✅ Updated correctly. `Model` definition updated, `SubMesh` added, `CubeResources` removed, `create_cube`/`create_triangle`/`create_quad`/`fallback material` added.
- [x] **module-map.md "Reference" section**: ✅ Now includes SPEC-020 + IMPL-020 entries (lines 340-341), properly ordered. No missing entries.

## Warnings

Non-blocking concerns for awareness:

- **W-001 (carried forward)**: Fallback material verification in headless tests (AC-007, AC-008, AC-014) cannot be fully automated — the headless backend does not track which material was bound during `draw_indexed()`. No `last_bound_material()` diagnostic accessor exists. Tests verify no-crash and correct draw call count but cannot programmatically confirm magenta was rendered. Acceptable as code-review-only invariant.
- **W-002 (carried forward)**: `Model::vertices()` and `Model::indices()` accessors dereference `unique_ptr` without null guard. Calling on default-constructed (null) Model is undefined behavior. Existing code avoids this, but it's a latent issue.
- **W-003 (carried forward)**: `demo_helpers.h/.cpp` retained as empty placeholder files rather than being fully deleted. Harmless but incomplete cleanup.
- **W-004**: Module map reference section has existing spec numbering disorder (SPEC-008 listed after SPEC-016, SPEC-013 used by two features). Not introduced by this feature, but worth noting.
- **W-005**: ADR-017 lists ADR-013 in "Related documents" as "Standard Vertex struct used by primitives and all meshes," but the primitives use 24-byte custom format, not the 72-byte standard Vertex. Minor imprecision.

## Required governance updates — all completed

All required and recommended governance updates have been implemented and verified:

- [x] **Spec ID resolved**: Renamed SPEC-010 → SPEC-020 (and IMPL-010 → IMPL-020). All cross-references across 10+ documents updated and verified. No stale SPEC-010/IMPL-010 references remain (modulo historical record in coordination.md).
- [x] **Module-map reference entry added**: SPEC-020 + IMPL-020 entries at module-map.md lines 340-341, ordered numerically. Previously redundant/duplicate entries cleaned up.
- [x] **ADR-013 updated**: Added explicit exception section (lines 119-132) for custom vertex formats in primitive helpers and low-level API callers. Updated demo_helpers narrative (lines 64-67) to reflect removal of setup_cube()/setup_triangle(). Correctly cross-references SPEC-020/ADR-017.
