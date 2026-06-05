# Workflow Coordination: model-multi-material

## Orchestrator

**Feature**: `model-multi-material`
**Status**: completed
**Current step**: completed
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
Re-review (v3, loop-back resolution) completed. No new blocking issues found. Contract is complete, consistent with SPEC-020, and follows all conventions (ADR-001, ADR-010, ADR-003, CONST-001, CONST-002). All 24 ACs covered. Spec-critic v4 declared spec ready; contract is aligned. All 5 previous warnings (W-001 through W-005) remain valid and carried forward. No spec-level changes required.
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

**Status**: approved
**Approver**: Hilderin
**Date**: Fri Jun  5 10:20:36 AM EDT 2026
**Warnings**:
<none>
**Notes**: Human approved implementation after both critics re-reviews passed without blocking issues.

## code-implementer

**Status**: completed
**Summary**:
Implemented the complete multi-material Model redesign: new SubMesh struct with material_index, unified Model::create_indexed factory (single-path, takes vector<SubMesh> + vector<shared_ptr<Material>>), removed Model::create()/material()/has_indices(), engine primitive helpers (create_cube/create_triangle/create_quad), RenderDevice::fallback_material() (magenta shader, lazy-created in both OpenGL and Headless backends), full migration of all 11 existing apps, new multi-material demo app, and comprehensive test suite (317 tests, all passing). demo_helpers cleaned of old CubeResources/setup_cube/setup_triangle.
**Artifacts**:
- Created: `src/engine/render/primitives.h`, `src/engine/render/primitives.cpp`, `src/cmd/apps/multi_material_app.h`, `src/cmd/apps/multi_material_app.cpp`
- Modified: `src/engine/render/model.h`, `src/engine/render/model.cpp`, `src/engine/render/render_device.h`, `src/engine/render/render_device_opengl.h`, `src/engine/render/render_device_opengl.cpp`, `src/engine/render/render_device_headless.h`, `src/engine/render/render_device_headless.cpp`, `src/engine/render/render_system.cpp`, `src/cmd/main.cpp`, `src/cmd/demo/demo_helpers.h`, `src/cmd/demo/demo_helpers.cpp`, `src/cmd/apps/cube_app.h`, `src/cmd/apps/cube_app.cpp`, `src/cmd/apps/triangle_app.h`, `src/cmd/apps/triangle_app.cpp`, `src/cmd/apps/cube_scene_app.cpp`, `src/cmd/apps/phong_app.cpp`, `src/cmd/apps/free_camera_app.cpp`, `src/cmd/apps/textured_cube_app.cpp`, `src/cmd/apps/asset_demo_app.cpp`, `src/cmd/apps/hot_reload_app.cpp`, `tests/model_tests.cpp`, `tests/lighting_tests.cpp`, `tests/scene_rendering_tests.cpp`
**Questions for human**:
none
**Warnings**:
- W-001: Fallback material verification in headless tests is limited — cannot verify magenta rendering programmatically in headless mode. AC-007/AC-008/AC-014 verified via null-guard/oob checks not crashing.
- W-003: triangle_app.h was listed twice in implementation contract; implemented once only.
- W-004: <span> include was not included in multi_material_app.cpp (avoided as per warning).
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Implementation accepted. All 24 ACs covered by tests, all 317 tests pass (100%), multi-material demo verified visually (red/green/blue face pairs), existing demos (triangle, cube) work without regression. No blocking issues found. Key warnings: (1) fallback material cannot be programmatically verified as magenta in headless mode (W-001 carried forward), (2) vertices()/indices() accessors dereference null on default-constructed Model (minor), (3) demo_helpers files retained as empty placeholders. Spec, contract, ADR, and constitution compliance confirmed.
**Artifacts**:
- `docs/specs/model-multi-material/code-review.md`
**Questions for human**:
none
**Warnings**:
- W-001 (carried forward): Fallback material verification in headless tests (AC-007/008/014) cannot prove magenta was bound — only verifies no-crash and correct draw call count. Test infrastructure limitation.
- W-002: vertices()/indices() accessors dereference unique_ptr without null guard. Calling on default-constructed Model is UB.
- W-003: demo_helpers.h/.cpp retained as empty placeholders with comments. Could delete, but harmless.
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
Updated ADR-013 to add an explicit exception for custom vertex formats used by primitive helpers and low-level API callers. The standard 72-byte Vertex format remains required for the engine-managed path (AssetManager, standard shaders), while simpler custom formats are permitted for self-contained code paths (primitives, direct low-level API usage). Also updated the stale reference to setup_triangle()/setup_cube() in the "How existing demos were updated" section to reflect their removal by SPEC-020/ADR-017, and removed the compliance rule requiring them as reference implementations.
**Artifacts**:
- `docs/adr/ADR-017-multi-material-model.md`
- `docs/adr/013-standard-vertex-format.md` (modified)
**Decisions needed**:
none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## constitution-agent

**Status**: completed
**Summary**:
Evaluated model-multi-material feature against all constitution rules (CONST-001 through CONST-004) and engineering principles. No violations found. The feature is entirely within the engine's rendering subsystem, follows established conventions (Result<T>, shared_ptr/unique_ptr ownership, no raw pointers per ADR-010), and was properly governed through the spec → implementation contract → human approval process. No new constitutional rules are warranted — the changes are subsystem-specific patterns, not durable cross-cutting principles.
**Artifacts**:
none
**Changes needed**:
none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated the project wiki to reflect the model-multi-material feature implementation. The module map now documents the new Model API (SubMesh struct, unified create_indexed factory, removed create/material/has_indices), new primitives module (create_cube/create_triangle/create_quad), and the fallback_material addition on RenderDevice. The glossary was updated to remove CubeResources/setup_cube and add SubMesh, create_cube, create_triangle, create_quad, and fallback material terms. App descriptions updated to reference engine::create_cube/create_triangle instead of demo helpers. Multi-material app entry added to the app subclasses table.
**Artifacts**:
- `docs/wiki/architecture/module-map.md` (modified)
- `docs/wiki/domain/glossary.md` (modified)
**Changes made**:
- module-map.md: Rewrote model.h/model.cpp entries for new API, added primitives.h/.cpp entries, updated app entries (triangle/cube → engine::create_*), added multi_material_app entry, updated demo_helpers to empty placeholder status, added multi-material to scene list, updated render_device.h with fallback_material
- glossary.md: Rewrote Model term (single factory, SubMesh vector, no material/has_indices), added SubMesh term, removed CubeResources term, added Fallback material/create_cube/create_triangle/create_quad terms, updated RenderDevice with fallback_material
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

**Status**: completed
**Summary**:
Cleaned up the Reference section in module-map.md: added missing SPEC-020 entry, removed duplicate SPEC-008/IMPL-008 entries (cli-app-system duplicated scene-graph's number), and reordered all entries in ascending numerical order.
**Artifacts**:
- `docs/wiki/architecture/module-map.md` (modified)
**Changes made**:
- module-map.md: Replaced Reference section — removed duplicate SPEC-008/IMPL-008 (cli-app-system), added SPEC-020 + IMPL-020 entries for model-multi-material, reordered all entries numerically (SPEC-001 through SPEC-020, then ADRs)
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Final re-validation completed. All three fixes from the SPEC-010 number conflict resolution verified:
(1) All cross-references updated from SPEC-010→SPEC-020 and IMPL-010→IMPL-020 across spec.md, implementation-contract.md, spec-critic.md, implementation-contract-critic.md, code-review.md, coordination.md, ADR-017, and module-map.md. No stale references.
(2) ADR-013 updated with explicit exception for custom vertex formats in primitive helpers (24-byte stride) and low-level API usage, plus cleaned up stale setup_cube()/setup_triangle() references.
(3) Module-map reference section properly ordered with SPEC-020+IMPL-020 entries, duplicates removed.
No constitution violations. All cross-document artifacts consistent. Feature is ready for merge.
**Artifacts**:
- `docs/specs/model-multi-material/governance-review.md`
**Questions for human**:
none
**Warnings**:
- W-001 (carried forward): Fallback material verification in headless tests limited — cannot programmatically confirm magenta rendering.
- W-002 (carried forward): vertices()/indices() accessors have null-dereference potential on default-constructed Model.
- W-003 (carried forward): demo_helpers retained as empty placeholders.
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
