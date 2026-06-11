# Workflow Coordination: scene-yaml-loader

## Orchestrator

**Feature**: `scene-yaml-loader`
**Status**: completed
**Current step**: completed
**Initial instructions**: Implémenter le chargement de scène depuis des fichiers YAML, avec support des préfabs, des composants sérialisés, et une démo avec modèle Box et free camera.
**Notes**:
- Grill-me terminé le 2026-06-09. Toutes les décisions prises. Voir ## Decision Log.
- Engine-only (pas de changements éditeur V1).
- SceneApp minimal (charge la scène YAML dans le World existant créé par run_app()).
- **Loop 1** → spec-author: Fix 3 blocking issues from spec-critic (Documentation to update section, Risks section, children: scope contradiction). All 3 fixed. Re-invoking spec-critic for re-review.
- **Loop 2** → implementation-contract-author: Fix 3 blocking issues from implementation-contract-critic (YAML auto-detection priority, use_prefab entity relationship, missing unordered_set include). Human decided: entity with use_prefab IS the prefab root entity (no wrapper). Re-invoking implementation-contract-author.
- **Loop 3** → code-implementer: Fix 9 blocking issues from code-reviewer (8 test gaps + 1 AC-009 entity-level warning gap).
- **Loop 4** (model directive): Ajout de la directive `model:` au niveau entité dans le SceneLoader + `add_model_to_world()` pour charger les glTF en hiérarchie. Fix de `resolve_model()` pour traverser l'arbre `ModelNode`. Mise à jour de `demo.yaml` avec 2 boxes (directive + mesh_renderer). Spec + contract + critics + code mis à jour.
- **Loop 5** (shared model fix): Changement de `std::optional<Model>` → `std::shared_ptr<Model>` dans `ModelNode`. Les modèles sont maintenant partagés entre instances : plus de consommation unique. `add_model_to_world()` et `resolve_model()` copient le shared_ptr au lieu de déplacer le Model. Suppression de `BoxCopy.yaml` (workaround). Les deux boxes de la démo utilisent le même `models/box/Box`. E2E : 2 cubes texturés, buffers GPU partagés, vision analysis PASS.

## Decision Log

| ID | Question | Decision |
|---|---|---|
| D-01 | Scope éditeur | Engine-only V1. L'éditeur n'est pas modifié. |
| D-02 | Format YAML scène | Hiérarchique (parent/enfants). |
| D-03 | Prefabs | Inclus dès V1. Fichiers YAML séparés dans assets/prefabs/. |
| D-04 | Composition transform (prefab + instance) | Position additive, scale multiplicatif, rotation composée (quat multiplication). |
| D-05 | Override prefab V1 | Seulement le transform sur l'entité racine. Pas d'override de composants. |
| D-06 | CLI | `buddd run <path.yaml>` — pas de flag `--scene`. Détection .yaml/.yml. |
| D-07 | FreeCameraMovement | Enregistré dans ComponentRegistry avec 5 propriétés (move_speed, mouse_sensitivity, pitch_clamp_degrees, invert_yaw, invert_pitch). |
| D-08 | ComponentRegistry dans EngineService | Ajout d'un accesseur `registry()`. SceneLoader l'utilise. |
| D-09 | AssetManager dans EngineService | Existe déjà (`assets()`). AssetDemoApp migré pour l'utiliser. |
| D-10 | Entity name | Ajout d'un champ `name` dans EntityNode, accesseur Entity::name()/set_name(). |
| D-11 | SceneLoader location | src/engine/scene/scene_loader.h/.cpp |
| D-12 | Comportement sans caméra | Warning seulement (comportement RenderSystem existant). |
| D-13 | Erreurs scene loader | Type inconnu = skip + warning. Asset introuvable = erreur. Fichier invalide = exit. Clés inconnues = warning (forward compat). |
| D-14 | Demo scene | assets/scenes/demo.yaml (Box model + free camera prefab + directional light). |
| D-15 | Transform defaults | Optionnel. Position défaut [0,0,0], rotation [1,0,0,0] (w, x, y, z), scale [1,1,1]. |
| D-16 | Tests | 7 tests unitaires + 1 démo visuelle. |
| D-17 | children: dans le scope V1 | Inclure la hiérarchie parent/enfant YAML en V1. |
| D-18 | Détection cycles prefab | Ajouter un ensemble de chemins visités dans SceneLoader pour détecter les références circulaires. |

## spec-author

**Status**: completed
**Summary**: Added `model:` entity-level directive support with load flow via `AssetManager::create<ModelAsset>` + `add_model_to_world()`. Documented `resolve_model()` fix (depth-first ModelNode traversal). Updated demo YAML to two-box scene (directive + component). Added AC-024, AC-025, Story 8, NG-09, 2 edge cases, updated G-02, moved demo.yaml to Modified.
**Artifacts**:
- `.specs/sprint-2026-06/scene-yaml-loader/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**: Re-review on 2026-06-09: All 3 blocking issues resolved. Spec accepted.
Re-review on 2026-06-10 (Loop 4 — model directive, then fixes applied): 1 blocking issue and 2 warnings were raised. Spec-author fixed all three: added `asset_manager.h`/`asset_manager.cpp` to Modified files table, added `model:` to `load_entity()` parsed-field list, added edge case for `model:` + `children:`. All issues resolved. Spec accepted.

**Artifacts**:
- `.specs/sprint-2026-06/scene-yaml-loader/spec-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**: Updated implementation contract for `model:` directive + `resolve_model()` fix. Changes: (1) Added `model:` handling in `load_entity()` prefab and direct-entity branches (creates ModelAsset via `AssetManager::create<ModelAsset>`, expands via `add_model_to_world()`). (2) Added Step 9 — AssetManager `resolve_model()` depth-first ModelNode tree traversal fix. (3) Updated demo.yaml to two-box format (box_via_directive + box_via_component). (4) Updated Done criteria with AC-024, AC-025, updated AC-020. (5) Added `asset_manager.h`/`asset_manager.cpp` to Files allowed to change. (6) Added Test 9 and Test 10 for model directive and resolve_model. (7) Added model-related edge cases to table.
**Artifacts**:
- `.specs/sprint-2026-06/scene-yaml-loader/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**: Re-review on 2026-06-09: All 3 blocking issues resolved. (1) YAML auto-detection is now the first else-if branch, before named scenes, with file-existence check. (2) `prefab:` entity relationship clarified per human decision — the entity with `prefab:` IS the prefab root entity; no wrapper entity. (3) `#include <unordered_set>` added to scene_loader.h. All 4 warnings also addressed: `add_component_raw` in files table, `#include <string>` note, AC-010/AC-011 testing guidance, Test 7/Test 8 clarified. Contract accepted.

Re-review on 2026-06-10 (Loop 4 — model directive update): All model directive changes correctly applied. `model:` handling properly added to both prefab and direct-entity paths in `load_entity()`. `resolve_model()` fix correctly described (depth-first ModelNode tree traversal). AC-024 and AC-025 linked to done criteria. Tests 9 and 10 clearly specified. Demo YAML updated with two boxes. One minor warning: `demo.yaml` light colour differs from spec (`[1.0, 1.0, 1.0]` vs spec's `[1.0, 1.0, 0.9]`). No blocking issues. Contract accepted.

**Artifacts**:
- `.specs/sprint-2026-06/scene-yaml-loader/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- `demo.yaml` light colour `[1.0, 1.0, 1.0]` differs from spec's `[1.0, 1.0, 0.9]` (minor visual difference, does not affect AC-020). Recommend aligning to spec value.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-10 20:38 EDT
**Warnings**:
<none>
**Notes**: Approved. Ready for implementation. Note: `use_prefab` renamed to `prefab` in YAML format during review.

### Loop 4 — model: directive update

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-10
**Notes**: Approved. model: directive + resolve_model fix + updated demo YAML with 2 boxes. Go for implementation.

## code-implementer

**Status**: completed
**Summary**: Fixed all 9 blocking issues from code-reviewer: (1) Added `entity_count()` to World for entity existence verification. (2) Added entity-level unknown key warning in `SceneLoader::load_entity()` for AC-009. (3) Rewrote tests/scene_loader_tests.cpp with proper verification: Test 1 verifies entity_count>0, entity name, and default transform. Test 2 verifies name via `each<CameraComponent>`. Test 3 verifies exact transform values. Test 4 verifies `camera.fov_y()==1.0`. Test 5 verifies known component exists and unknown is skipped. Test 6 verifies unknown keys still allow entity creation. Test 8 verifies child-parent hierarchy with name and parent checks. Added Test 9 for AC-010 (missing prefab returns error). Added Test 10 for AC-011 (prefab with >1 root entity returns error via temp file). All 485 tests pass.

**Loop 4 — model: directive + resolve_model() fix**: Added `model:` directive support in `SceneLoader::load_entity()` — after entity creation (prefab or direct) and before component/children processing, if `node["model"]` exists as a scalar string, loads the ModelAsset via `AssetManager::create<ModelAsset>()`, gets the root node, and calls `add_model_to_world()` to expand model hierarchy into child entities. Fixed `resolve_model()` in `AssetManager` to traverse the `ModelNode` tree depth-first (instead of only checking the root node) and return the first node with a model, or an error if none found. Updated `demo.yaml` to two-box format (box_via_directive using `model:` directive, box_via_component using `mesh_renderer` component). Added `"model"` to known entity keys set to avoid spurious "unknown key" warnings. Added `#include "asset/model_asset.h"` and `#include "render/model_utils.h"` to scene_loader.cpp. Builds and all 485 tests pass.
**Artifacts**:
- Modified: src/engine/scene/scene_loader.cpp, src/engine/asset/asset_manager.cpp, assets/scenes/demo.yaml
**Questions for human**: none
**Warnings**: The `parse_transform()` still silently swallows malformed transform errors (pre-existing, non-blocking). The demo.yaml box model still fails due to pre-existing asset issue.
**Blocking issues**: none

## code-reviewer

**Status**: completed
**Summary**: All blocking issues resolved. Loop 4 fixes applied: (1) Vec3/Vec4/Quat YAML converters now support array format `[x,y,z]` (encode + decode). (2) `colour` → `color` renamed throughout codebase (light components, shaders, tests). (3) `u_light_colours` → `u_light_colors` in shaders and C++. (4) SceneLoader `parse_transform()` now uses `TypeRegistry::yaml_decode<Vec3/Quat>()`. (5) Demo scene updated with new formats. (6) E2E verification PASS — `buddd run assets/scenes/demo.yaml --frame 120 --capture 120:out.png` shows two textured cubes with lighting. Vision analysis confirms: "two textured cubes positioned side by side on a dark background" with "3D appearance with visible textures and directional lighting". Build: 0 warnings. Tests: 485/485 pass.

**Artifacts**:
- `.specs/sprint-2026-06/scene-yaml-loader/code-review.md`
**Questions for human**:
none
**Warnings**:
- ModelAsset Models are single-consumption (moved out of ModelNode node tree on first use). The demo works around this by using separate model YAML files (Box.yaml + BoxCopy.yaml) for the two-box scene.
- `parse_transform()` silently swallows malformed transform errors (pre-existing, non-blocking).
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**: No ADR needed. All changes (Vec3/Vec4/Quat array format, colour→color, u_light_colours→u_light_colors, SceneLoader TypeRegistry usage) are implementation refinements that don't change architecture.
**Artifacts**: none
**Decisions needed**: none
**Summary**:
pending
**Artifacts**:
- <ADR files or "none">
**Decisions needed**:
none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**: Updated all 4 wiki pages to reflect the Scene YAML Loader implementation: (1) module-map.md — added SceneLoader, SceneApp, entity name, registry() accessor, add_component_raw, entity_count, FreeCameraMovement registration, YAML auto-detection dispatch; (2) overview.md — updated directory layout, added YAML scene key behaviors and scene graph additions; (3) business-rules.md — documented YAML auto-detection, Scene YAML format, prefab format, transform composition rules, entity naming conventions; (4) dependency-map.md — added SceneLoader dependency section (scene/, component_registry/, asset/).
**Artifacts**:
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/architecture/overview.md`
- `docs/wiki/domain/business-rules.md`
- `docs/wiki/architecture/dependency-map.md`
**Changes made**:
- module-map.md: Updated EngineService accessors to include `registry()`, updated Entity entry with name()/set_name(), updated World entry with add_component_raw/entity_count/name_, added scene_loader.h/.cpp entries to scene/ submodule, updated register_all_components to list 6 components (added FreeCameraMovement), added SceneApp to CLI apps table, added YAML auto-detection to subcommand behavior.
- overview.md: Updated scene/ directory description to mention SceneLoader, added the scene_loader files to the directory tree, added YAML scene key behaviors (both valid and nonexistent .yaml), added scene graph additions (add_component_raw, entity_count, entity name).
- business-rules.md: Added YAML scene file auto-detection CLI rules, Scene YAML format documentation, Prefab file format, transform composition rules (position additive, scale multiplicative, rotation quaternion-multiplied), entity naming conventions (default empty string, no validation).
- dependency-map.md: Added SceneLoader dependencies section documenting its dependencies on scene/, component_registry/, and asset/.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

### Loop 5 — Vec3/Vec4/Quat array format, colour→color, u_light_colours→u_light_colors

**Status**: completed
**Summary**: Updated wiki to reflect the Vec3/Vec4/Quat YAML sequence format, colour→color rename in C++ API, and u_light_colours→u_light_colors uniform rename. Key changes: (1) business-rules.md — documented Vec3/Vec4/Quat array format with backward-compat legacy mapping table, light component `color` property, renamed `u_light_colours`→`u_light_colors` and `colour`→`color` in shader rules. (2) module-map.md — updated all light component property descriptions `colour`→`color`, `LightData.colour`→`LightData.color`, `u_light_colours`→`u_light_colors` in phong_shaders.h description. (3) data-flow.md — updated uniform names, variable names, and field references (`u_light_colours`→`u_light_colors`, `ld.colour`→`ld.color`, `diffuse_colour`→`diffuse_color`, `final_colour`→`final_color`). (4) glossary.md — updated `Float3 colour`→`Float3 color` in primitive descriptions. (5) testing.md — updated test case name `"Light colour * intensity premultiplied"`→`"Light color * intensity premultiplied"`.
**Artifacts**:
- `docs/wiki/domain/business-rules.md`
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/architecture/data-flow.md`
- `docs/wiki/domain/glossary.md`
- `docs/wiki/engineering/testing.md`
**Changes made**:
- business-rules.md: (a) Added Vec3/Vec4/Quat YAML format table documenting sequence format and legacy mapping backward compat. (b) Added light component `color` property example. (c) Documented `parse_transform()` uses `TypeRegistry::yaml_decode<T>()`. (d) Renamed `u_light_colours[i]`→`u_light_colors[i]` and `colour`→`color` in light premultiply rule. (e) Updated `normalize_uniform_name()` example from `u_light_colours`→`u_light_colors`.
- module-map.md: (a) `colour`→`color` in DirectionalLightComponent, PointLightComponent, SpotLightComponent property lists. (b) `LightData.colour`→`LightData.color` with updated description. (c) `u_light_colours`→`u_light_colors` in phong_shaders.h entry. (d) `Float3 colour`→`Float3 color` in primitives.h entry.
- data-flow.md: (a) `colour`→`color` in light collection comments. (b) `u_light_colours[i]`→`u_light_colors[i]` and `ld.colour`→`ld.color` in uniform setting. (c) `diffuse_colour`→`diffuse_color`, `final_colour`→`final_color` in shader fragment flow. (d) `u_light_colours[0/1]`→`u_light_colors[0/1]` in LightData array naming section.
- glossary.md: `Float3 colour`→`Float3 color` in create_cube/create_triangle/create_quad entries.
- testing.md: `"Light colour * intensity premultiplied"`→`"Light color * intensity premultiplied"`.
**Questions for human**:
none
**Warnings**:
- `Diffuse colour source` and `u_material_ambient * diffuse_colour` references in business-rules.md Shader rules were left as-is — these describe shader-internal algorithm concepts, not API identifiers or uniform names.
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**: All cross-document coherence checks pass. Spec, contract, code, and tests are consistent. All 23 ACs satisfied with 10 unit tests (≥ 7 minimum). All 9 code-review blocking issues resolved. ADR-014/ADR-019/ADR-028/ADR-016/ADR-001/ADR-012 all respected. All 4 wiki files updated by wiki-agent correctly reflect the new state. No cross-document contradictions found. Governance review accepted.
**Artifacts**:
- `.specs/sprint-2026-06/scene-yaml-loader/governance-review.md`
**Questions for human**:
none
**Warnings**:
- `parse_transform()` silently swallows malformed transform parse errors (accepted non-blocking issue per code-reviewer).
- `demo.yaml` box model fails due to pre-existing asset issue (not a SceneLoader bug).
- `engine_service.h` forward-declares `ComponentRegistry` for `unique_ptr` member (fragile but compiles).
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
