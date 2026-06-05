# Workflow Coordination: model-multi-material

## Orchestrator

**Feature**: `model-multi-material`
**Status**: in-progress
**Current step**: critics-re-review-complete
**Initial instructions**: Rendre le Model capable de gérer plusieurs matériaux via une décomposition en SubMesh. Chaque SubMesh contient une plage d'indices et un Material. Garder la rétrocompatibilité avec l'API mono-matériau existante. Ajouter un material magenta de fallback sur RenderDevice.

**Notes**:

Loop-back: human a demandé de relancer les deux critics (spec-critic et implementation-contract-critic) avant la validation humaine.

Décisions prises pendant le grill-me :

| # | Décision (finale) |
|---|---|---|
| 1 | **SubMesh** : `{index_start, index_count, material_index}` — material reference par index |
| 2 | **SubMesh public** : `vector<SubMesh>` exposé en lecture seule |
| 3 | **Factory unique** : `Model::create_indexed(device, format, vertex_data, index_data, type, submeshes, materials)` — tout upfront |
| 4 | **material() supprimé** : utiliser `model.materials()[i]` |
| 5 | **add_submesh() / set_material()** : supprimés (immutable après création) |
| 6 | **Non-indexé** : supprimé. `Model::create()` et `has_indices()` n'existent plus |
| 7 | **Primitives engine** : `create_cube(device, material)`, `create_triangle(device, material)`, `create_quad(device, material)` — le material est obligatoire, les helpers sont des factories de géométrie uniquement |
| 8 | **Fallback material** : `RenderDevice::fallback_material()` pour submesh null/out-of-bounds |
| 9 | **Migration totale** : toutes les apps migrées, breaking change assumé |
| 10 | **Commande** : `buddd run multi-material` (pattern existant) |
| 11 | **Scope** : Model redesign + primitives + migration. glTF deferré. |

## spec-author

**Status**: completed
**Summary**:
Fixed both blocking issues from the spec-critic re-review:
- BL-001: Added `tests/model_tests.cpp`, `tests/lighting_tests.cpp`, and `tests/scene_rendering_tests.cpp` to both the migration table and "Documents requiring updates" section, with clear instructions for rewriting each test file.
- BL-002: Added `submesh.index_start + submesh.index_count exceeds index buffer length` edge case to the Edge cases table as undefined behavior (caller error, not validated at creation).
**Artifacts**:
- `docs/specs/model-multi-material/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review (v4, loop-back resolution) completed. Both previous blocking issues (BL-001, BL-002) are verified as resolved. Test files now listed in migration table and Documents requiring updates. Missing edge case now present. No new blocking issues found. All 12 Definition of Ready criteria satisfied. Spec is consistent with CONST-001, ADR-001, ADR-003, ADR-010, and codebase conventions. Spec is ready for human validation.
**Artifacts**:
- `docs/specs/model-multi-material/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- AC-006 verification depends on implicit test infrastructure (material tracking on headless device).
- AC-014 "renders magenta" is not headless-verifiable; acceptable as code-review invariant.
- Old `create_indexed()` callers in non-test code: resolved for test files via BL-001 fix; remaining engine callers covered by migration intent.
- New: No AC for empty-materials vector fallback (edge case documented but not explicitly AC-tested).
- New: No AC for "invalid vertex format" validation (existing behavior, minor gap).
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Rewrote contract matching final design. Covers: new Model (SubMesh.material_index, unified factory, no material/add_submesh/set_material), primitives.h (create_cube/triangle/quad), RenderDevice::fallback_material(), full migration of all apps, multi-material demo. Implementation pseudocode for every file.
**Artifacts**:
- `docs/specs/model-multi-material/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review (v3, loop-back resolution) completed. No new blocking issues found. Contract is complete, consistent with SPEC-010, and follows all conventions (ADR-001, ADR-010, ADR-003, CONST-001, CONST-002). All 24 ACs covered. Spec-critic v4 declared spec ready; contract is aligned. All 5 previous warnings (W-001 through W-005) remain valid and carried forward. No spec-level changes required.
**Artifacts**:
- `docs/specs/model-multi-material/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- **W-001**: Fallback material verification in headless tests (AC-007, AC-008, AC-014) cannot be automated with current headless backend — no `last_bound_material()` accessor exists. Implementer must either add tracking or accept code-review verification.
- **W-002**: Step 8 (render_system.cpp migration) instruction `model.materials()[0]` is imprecise — returns `shared_ptr<Material>`, not `Material&`. Requires `->` and null-guard. Verified against actual render_system.cpp line 113.
- **W-003**: `triangle_app.h` listed twice in "Files to modify" (entries #14 and #31).
- **W-004**: `multi_material_app.cpp` includes `<span>` unnecessarily (pulled in by engine headers).
- **W-005**: Multi-material vertex data 24-byte stride is correct per spec assumption A-02.
**Blocking issues**:
none

## Human Validation

**Status**: pending
**Approver**: <git user name>
**Date**: <date and time>
**Warnings**:
<none>
**Notes**: <any human feedback or conditions>

## code-implementer

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- <list of files created or modified>
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## code-reviewer

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- `docs/specs/model-multi-material/code-review.md`
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## adr-agent

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- <list of ADR files created or modified, or "none">
**Decisions needed**:
<none>
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## constitution-agent

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- <list of constitution files, or "none">
**Changes needed**:
<none>
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## wiki-agent

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- <list of wiki files created or modified, or "none">
**Changes made**:
<none>
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## governance-reviewer

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- `docs/specs/model-multi-material/governance-review.md`
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
