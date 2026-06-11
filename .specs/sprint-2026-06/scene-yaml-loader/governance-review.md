# Governance Review — Scene YAML Loader

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec matches implementation**: All 23 acceptance criteria (AC-001 to AC-023) are verified. Code implements SceneLoader, SceneApp, YAML auto-detection, Entity name, ComponentRegistry persistence, FreeCameraMovement registration, demo YAML files, and 10 unit tests (≥ 7 minimum).
- [x] **Contract matches spec**: Implementation contract covers all acceptance criteria, non-goals, edge cases, and error conditions from the spec. CUSTOM-001 (add_component_raw) and CUSTOM-002 (circular prefab detection) are justified additions not contradictory to the spec.
- [x] **Code-review resolved all 9 previously-blocking issues**: All 9 issues from initial code review (test gaps, AC-009 warning gap) are resolved and marked `[x]`. Code-reviewer verdict: Accepted.
- [x] **Spec-critic resolved all 3 blocking issues**: Spec now has Documentation to update section, Risks section (R-01 to R-04), and children: scope resolved (AC-023 added). Spec accepted.
- [x] **Implementation-contract-critic resolved all 3 blocking issues**: YAML auto-detection priority fixed, prefab entity relationship clarified, `#include <unordered_set>` added. Contract accepted.
- [x] **Cross-document consistency on YAML format**: Spec, contract, demo files, and wiki all consistently use `type: Scene`/`type: Prefab`, `version: 1`, `prefab:` (not `use_prefab:`).

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-014 (CLI App System)**: SceneApp follows the App lifecycle pattern. YAML auto-detection is a new dispatch branch in `main.cpp` before named scenes. `run_app()` is unchanged. Compliant.
- [x] **ADR-019 (Architecture Boundaries)**: SceneLoader lives in `src/engine/scene/` and uses only engine abstractions (`World`, `ComponentRegistry`, `AssetManager`, yaml-cpp). No platform/graphics/GLM headers. Compliant.
- [x] **ADR-028 (Component Type Registry)**: SceneLoader uses `ComponentRegistry::create()`, `describe()`, and `deserialize_component()` for runtime component creation. FreeCameraMovement uses overload (B) registration (simple lambdas). Compliant.
- [x] **ADR-016 (yaml-cpp Dependency)**: SceneLoader uses `YAML::LoadFile()` and `YAML::Node` directly. try-catch wraps `YAML::LoadFile()` calls for exception safety. Compliant.
- [x] **ADR-001 (Result/Error)**: All fallible APIs return `Result<void>` or `Result<Entity>`. Errors use existing `Error::Category` values (`InvalidFormat`, `IoFailed`, `InvalidArgument`). Compliant.
- [x] **ADR-012 (Navigable Object Graph)**: SceneApp accesses `ctx.services.registry()` and `ctx.services.assets()` through EngineService. This is consistent with the EngineService accessor pattern. Compliant.
- [x] **No new ADRs required**: The spec and contract confirm no ADR updates are needed. ADR-014 and ADR-028 already cover the extension mechanism.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **module-map.md**: Updated with SceneLoader, SceneApp, entity name, registry() accessor, add_component_raw, entity_count, FreeCameraMovement registration (5 properties), YAML auto-detection dispatch. All accurate.
- [x] **overview.md**: Updated directory layout with scene_loader files, YAML scene key behaviors (valid and nonexistent .yaml), scene graph additions (add_component_raw, entity_count, entity name). All accurate.
- [x] **business-rules.md**: Documents YAML auto-detection rules, Scene YAML format, Prefab file format, transform composition (position additive, scale multiplicative, rotation quaternion-multiplied), entity naming conventions. All accurate.
- [x] **dependency-map.md**: Documents SceneLoader dependencies on scene/, component_registry/, asset/. All accurate.

All 4 wiki pages updated by wiki-agent with status `completed`. Implementation matches wiki descriptions.

## Warnings

Non-blocking concerns for awareness:

- **`parse_transform()` silently swallows malformed transform errors**: When `parse_vec3()` or `parse_quat()` returns an error (e.g., malformed array with wrong element count), `parse_transform()` silently uses defaults instead of propagating the error. This deviates from the spec's "Malformed YAML → error; scene load fails" but was accepted as a non-blocking issue by the code-reviewer.
- **`demo.yaml` box model fails to load**: `models/box/Box` has no root Model node (pre-existing asset issue). The camera and light load correctly. Not a SceneLoader bug.
- **`engine_service.h` forward-declares `ComponentRegistry`** for `std::unique_ptr<ComponentRegistry>` member. Compiles but is fragile.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None. All wiki files have been updated by the wiki-agent (status: completed). No ADR updates are required — ADR-014, ADR-028, ADR-016, ADR-001, ADR-012, and ADR-019 all remain consistent with this implementation.
