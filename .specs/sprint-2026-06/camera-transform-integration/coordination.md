# Workflow Coordination: camera-transform-integration

## Orchestrator

**Feature**: `camera-transform-integration`
**Status**: completed
**Current step**: completed
**Initial instructions**: "CameraComponent ne devrait pas exposer Camera au publique. Devrait avoir les méthodes de set directement dans CameraComponent pour simplifier le code du genre: auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera(); cam.set_position(...); cam.set_orientation(...); cam.set_perspective(...);"

Après grill-me, la portée a évolué vers :
- Supprimer math::Camera (pas besoin de backward compat)
- CameraComponent gère la projection directement (fov_y, aspect, near, far)
- Position/orientation de la caméra → via entity.transform()
- Fonction libre math::view_matrix(position, orientation) dans math/
- look_at() sur CameraComponent (modifie le Transform)
- Migrer tous les call sites existants
- Migrer cube_app, multi_material_app vers CameraComponent
- Migrer FreeCameraMovement vers entity.transform()
- camera() accessor supprimé (privé)
- Créer un ADR
- Mettre à jour le wiki

**Notes**: Grill-me terminé. Toutes les décisions clarifiées avec l'humain.

Loop 1 → implementation-contract-author: Fix 2 blocking issues from critic:
1. GLM architecture boundary: move look_at rotation math into `math::rotation_from_look_at()` free function in math/math.h
2. Implementation order: defer camera.h/camera.cpp deletion until after all app/test file migrations

Loop 2 → direct fix: ADR-024 missing from ADR Index (docs/wiki/decisions/adr-index.md) — added entry.

**Feature complete.**

## spec-author

**Status**: completed
**Summary**: Wrote complete functional spec (SPEC-NNNN) for Camera → Transform integration. Covers: problem statement (duplicate position/orientation), full API design for CameraComponent (projection-only), free function `math::view_matrix()`, migration plan for all 13 app files, 2 test files, render_system, free_camera_movement, and wiki. Includes 30 acceptance criteria (AC-001 through AC-030), edge/error cases, impact analysis with every file listed, and Definition of Ready compliance.
**Artifacts**:
- `.specs/sprint-2026-06/camera-transform-integration/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**: Review complete. Spec is thorough, well-structured, and satisfies all Definition of Ready criteria. No blocking issues found. Three warnings noted: (1) `math/math.h` include of `camera.h` not listed for removal, (2) incomplete wiki file list (missing glossary.md, overview.md), (3) E2E verification partially relies on visual inspection.
**Artifacts**:
- `.specs/sprint-2026-06/camera-transform-integration/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- `math/math.h` currently includes `#include "camera.h"` (line 12). The spec lists `math/math.h` for adding `view_matrix()` but does not mention removing this include. When `camera.h` is deleted, the include will fail.
- The wiki documentation update list is incomplete: `docs/wiki/domain/glossary.md` and `docs/wiki/architecture/overview.md` both reference `math::Camera` directly but are not listed in the impact analysis.
- E2E verification section offers "visual inspection" as an alternative to automated `--capture` screenshot comparison — visual inspection is not automated or repeatable.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**: Fixed both blocking issues from critic review: (1) Extracted GLM-dependent look-at rotation math into new `math::look_at_rotation(Vec3, Vec3, Vec3)` free function in `math/math.h` (per ADR-002/ADR-019), removed all GLM usage from `camera_component.cpp`. (2) Reordered implementation steps so camera.h/camera.cpp deletion (step 8) occurs AFTER all app/test file migrations (steps 4-7). Also fixed related warnings: replaced undefined `look_at_rotation` helper with `math::look_at_rotation()`, and added explicit `#include "math/camera.h"` removal tracking for every file migration step.
**Artifacts**:
- `.specs/sprint-2026-06/camera-transform-integration/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**: Re-review complete. Both previous blocking issues verified as resolved: (1) GLM-dependent math extracted to `math::look_at_rotation()` in `math/math.h` — architecture boundary satisfied. (2) camera.h/camera.cpp deletion moved to step 8 (after all app/test file migrations) — no build break risk. All previous warnings also addressed. No new blocking issues found. Contract is ready for acceptance.
**Artifacts**:
- `.specs/sprint-2026-06/camera-transform-integration/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- `CameraComponent::view_matrix()` member added — not in spec's explicit API list (minor scope creep). Non-blocking.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: Sun Jun  7 06:04:00 PM EDT 2026
**Warnings**:
<none>
**Notes**: Approuvé par l'humain. Prêt pour l'implémentation.

## code-implementer

**Status**: completed
**Summary**: Implemented the Camera → Transform integration per the accepted contract. Removed math::Camera class entirely. Rewrote CameraComponent as projection-only (fov_y, aspect, near, far). Added math::view_matrix(Vec3, Quat) and math::look_at_rotation(Vec3, Vec3, Vec3) free functions in math/math.h. Migrated all 13 app files, render_system.cpp, free_camera_movement.cpp, and 3 test files (scene_rendering_tests, lighting_tests, math_tests). Created ADR-024. Updated wiki pages (glossary, overview, business-rules, module-map). All 425 tests pass.
**Artifacts**:
- Deleted: src/engine/math/camera.h, src/engine/math/camera.cpp
- Created: docs/adr/ADR-024-camera-transform-integration.md
- Modified: src/engine/math/math.h, src/engine/scene/camera_component.h, src/engine/scene/camera_component.cpp, src/engine/render/render_system.cpp, src/engine/scene/free_camera_movement.cpp, src/cmd/apps/cube_app.h, src/cmd/apps/cube_app.cpp, src/cmd/apps/multi_material_app.h, src/cmd/apps/multi_material_app.cpp, src/cmd/apps/cube_scene_app.cpp, src/cmd/apps/textured_cube_app.cpp, src/cmd/apps/free_camera_app.cpp, src/cmd/apps/phong_app.cpp, src/cmd/apps/gltf_helmet_app.cpp, src/cmd/apps/gltf_demo_app.cpp, src/cmd/apps/hot_reload_app.cpp, src/cmd/apps/hot_reload_gltf_app.h, src/cmd/apps/hot_reload_gltf_app.cpp, src/cmd/apps/asset_demo_app.cpp, tests/scene_rendering_tests.cpp, tests/lighting_tests.cpp, tests/math_tests.cpp, docs/wiki/domain/glossary.md, docs/wiki/architecture/overview.md, docs/wiki/domain/business-rules.md, docs/wiki/architecture/module-map.md
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**: Implementation review complete. All 30 acceptance criteria verified as satisfied. The math::Camera class is fully removed, CameraComponent is projection-only, all 13 app files + render_system + free_camera_movement migrated to the new API, math::view_matrix() and math::look_at_rotation() free functions added, ADR-024 created, wiki updated. All 425 tests pass (100%). Build compiles with zero warnings in our code. No blocking issues found.
**Artifacts**:
- `.specs/sprint-2026-06/camera-transform-integration/code-review.md`
**Questions for human**:
none
**Warnings**:
- `tests/math_tests.cpp` was modified outside the contract's allowed file list (necessary change to replace old math::Camera tests with view_matrix/look_at_rotation tests).
- `docs/adr/ADR-024-camera-transform-integration.md` is untracked (not yet staged).
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**: Verified all 4 wiki pages previously modified by code-implementer for Camera → Transform integration. Found and fixed 5 remaining issues: (1) glossary.md still listed Camera in buddd_engine math module description, (2) module-map.md cube app description still said "Camera + MVP", (3) module-map.md math_tests.cpp description still listed Camera as a test category, (4) dependency-map.md listed Camera as a wrapper type, (5) dependency-map.md referenced deleted camera.cpp. All fixes applied.
**Artifacts**:
- `docs/wiki/domain/glossary.md` (verified + corrected)
- `docs/wiki/architecture/overview.md` (verified)
- `docs/wiki/domain/business-rules.md` (verified)
- `docs/wiki/architecture/module-map.md` (verified + corrected)
- `docs/wiki/architecture/dependency-map.md` (corrected)
**Changes made**: Removed Camera from buddd_engine math module list in glossary.md; changed "(Camera + MVP)" to "(camera entity + MVP)" in module-map.md; removed Camera from math_tests.cpp test categories in module-map.md; removed Camera from wrapper types list in dependency-map.md; updated reference from deleted camera.cpp to math.h in dependency-map.md.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**: Governance review complete. Blocking issue resolved: ADR-024 added to ADR Index (docs/wiki/decisions/adr-index.md). Three non-blocking warnings noted (minor spec omission, contract file list gap, untracked ADR file). All cross-document coherence checks pass for spec, contract, code, ADRs, and wiki.
**Artifacts**:
- `.specs/sprint-2026-06/camera-transform-integration/governance-review.md`
**Questions for human**:
none
**Warnings**:
- `CameraComponent::view_matrix()` member not in spec's explicit API list (minor omission).
- `tests/math_tests.cpp` was modified outside the contract's declared allowed file list (necessary change).
- `docs/adr/ADR-024-camera-transform-integration.md` is untracked (not yet staged in git).
**Blocking issues**:
- [x] **ADR-024 missing from ADR Index** — Fixed: added entry to `docs/wiki/decisions/adr-index.md`.

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
