# Workflow Coordination: scene-source-and-saver

## Orchestrator

**Feature**: `scene-source-and-saver`
**Status**: completed
**Current step**: completed
**Loop history**: spec-critic rejected spec → fixed. Now impl-contract-critic rejected contract with 2 blocking issues: (1) const-correctness violation in build_type_to_info_map() calling non-const create() on const pointers, (2) missing AC-007/AC-008 test coverage. Fixes applied: const_cast with safety comment, added Tests 4/4b for model source tracking and model-child None source. Renumbered subsequent tests.
**Loop history**: spec-critic rejected spec with 2 blocking issues: (1) Observability table contradicts Error cases table for component serialization failure behavior, (2) Edge case for empty components still says "consistent decision needed" though Q01 resolved it. Fixes applied to spec.md.
**Initial instructions**: Add source tracking to entities (prefab/model origin) so the scene saver knows when to stop expanding the entity tree. Then implement scene saving with unit tests.
**Notes**: User identified a flaw: entities loaded from prefabs or models lose their source information. When saving the scene back to YAML, all entities would be saved as expanded trees instead of stopping at model/prefab boundaries with references. Need to add `EntitySource` tracking and a `SceneSaver` class.

**Decisions from grill-me**:
- EntitySource stored on EntityNode with Entity accessors (source/set_source)
- Prefab entities: save only prefab ref + name + transform overrides (immutable)
- Model entities: save only model ref + name + transform overrides (immutable)
- Model/prefab entities cannot have extra children or components added (enforced by editor)
- SceneSaver in src/engine/scene/scene_saver.h/.cpp

## spec-author

**Status**: completed
**Summary**:
Fixed 2 blocking issues from spec-critic: reconciled Observability/Error case contradiction (component serialization → fail entire save), updated edge case for empty components to match Q01 resolution (omit entirely). Added pending-destroy and component-info-reverse-lookup edge cases/risks per critic warnings.
**Artifacts**:
- `.specs/sprint-2026-06/scene-source-and-saver/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review: Both previously blocking issues are confirmed fixed. (1) Error cases and Observability now agree that component serialization failure logs ERROR and fails the entire save. (2) Edge case table now correctly reads "`components:` key is omitted entirely (resolved in Q01)." The spec also added a pending-destroy edge case and documented the Component*→ComponentInfoBase* reverse-lookup risk (R-05). No new blocking issues found. The spec passes all Definition of Ready criteria.

**Artifacts**:
- `.specs/sprint-2026-06/scene-source-and-saver/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- Acceptance criteria lack priority labels to distinguish minimum-viable vs. stretch.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Reworked Step 4b to use user's approach: DefaultChecker stored on Property, computed once in add_property() via default T{} + getter, no component creation at serialization time. Property.h modified with DefaultChecker type and null-node return in serialize(). ComponentInfo.h modified to pass checker in add_property() and skip null nodes in serialize() [4th critic fix].
**Artifacts**:
- `.specs/sprint-2026-06/scene-source-and-saver/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Cycle 1: Initial review. Found 2 blocking issues (const-correctness, missing AC-007/AC-008 tests). ✓ Resolved.
Cycle 2: Re-review after human feedback update. Found 4 new blocking issues: (1) AC-020/021/022 missing from Done criteria, (2) no tests for AC-020/021/022, (3) multiple contradictions on transform omission, (4) SerializationContext missing from yaml_encode call in Step 4b. Verdict: rejected.
Cycle 3 (final): All 4 previously blocking issues confirmed fixed. Contract author added AC-020/021/022 to Done criteria, added Tests 12/13/14 with full code, fixed all 4 transform omission contradictions, and replaced the yaml_encode approach with raw PropType comparison (DefaultChecker). No new blocking issues. Verdict: accepted.
**Artifacts**:
- `.specs/sprint-2026-06/scene-source-and-saver/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- AC-014 (`save_to_file`) is in Done criteria but not in the minimum test set (Test 11 is only "recommended"). Implementer should ensure AC-014 is still tested.
- Duplicate "Test 5" numbering (two tests labeled Test 5) and stale duplicate minimum set text still present — cosmetic only.
- `build_type_to_info_map()` creates and discards temporary component instances at construction time. Minor performance consideration.
- Spec AC-021 mentions `TypeRegistry::yaml_encode` for default computation but contract correctly uses raw PropType comparison. Behaviorally identical.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-11 20:24 EDT
**Warnings**:
none
**Notes**: Approved after incorporating default-value omission feedback (DefaultChecker approach, no component creation at serialize time).

## code-implementer

**Status**: completed
**Summary**:
Created entity_source.h with EntitySourceType enum and EntitySource struct. Modified world.h/cpp to add source_ field to EntityNode, source accessors, root entity iteration, and component iteration. Modified entity.h/cpp to add source(), set_source(), component_count(), component_at() delegating to World. Modified scene_loader.cpp to set source type (Prefab/Model) on loaded entities. Added DefaultChecker to Property in property.h/cpp for default-value omission. Modified component_info.h to compute default checkers in add_property() and skip null-node properties in serialize(). Created scene_saver.h/cpp with full SceneSaver class (save_to_file, save_to_yaml, save_entity, save_transform, reverse-lookup map). Created tests/scene_saver_tests.cpp with 15 test cases covering all acceptance criteria. Updated component_registry_tests.cpp for default-skipping behavior.
**Artifacts**:
- CREATE: src/engine/scene/entity_source.h
- MODIFY: src/engine/scene/world.h
- MODIFY: src/engine/scene/world.cpp
- MODIFY: src/engine/scene/entity.h
- MODIFY: src/engine/scene/entity.cpp
- MODIFY: src/engine/scene/scene_loader.cpp
- MODIFY: src/engine/scene/component_registry/property.h
- MODIFY: src/engine/scene/component_registry/property.cpp
- MODIFY: src/engine/scene/component_registry/component_info.h
- CREATE: src/engine/scene/scene_saver.h
- CREATE: src/engine/scene/scene_saver.cpp
- CREATE: tests/scene_saver_tests.cpp
- MODIFY: tests/component_registry_tests.cpp
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
All 22 acceptance criteria are verified: source tracking (AC-001-008), SceneSaver API and basic save (AC-009-019), and default-value omission (AC-020-022). Implementation matches the contract in structure and behavior. All 15 new scene_saver tests pass (82 assertions) plus all existing tests (500/500 total). No blocking issues found. 4 non-blocking warnings noted (minor scope deviations and a suggested test improvement).
**Artifacts**:
- `.specs/sprint-2026-06/scene-source-and-saver/code-review.md`
**Questions for human**:
none
**Warnings**:
1. `tests/component_registry_tests.cpp` was modified to update a mesh renderer test for new default-omission behavior — not in the explicit allowed files list, but a necessary consequence of Step 4b.
2. `root_entity_count()`/`get_root_entity()` placed in public instead of private as specified in contract — functionally harmless.
3. `property.cpp` modified in support of `property.h` changes — acceptable as the .cpp must match header declarations.
4. `friend class SceneSaver` is technically unnecessary (SceneSaver uses only public Entity/World API) — harmless.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated `docs/wiki/architecture/module-map.md` — added `entity_source.h`, `scene_saver.h/.cpp`, `Entity::source()`/`set_source()`, `DefaultChecker` on `Property`, default-value skip on `ComponentInfo::serialize()`, entity source iteration helpers on `World`. Updated `docs/wiki/architecture/overview.md` — added `entity_source.h` and `scene_saver.h/.cpp` to directory tree, added entity source tracking and SceneSaver key behaviors. Updated `docs/wiki/domain/business-rules.md` — documented entity source types with assignment rules, SceneSaver YAML output format by source type, and default-value omission rules for transforms and component properties.
**Artifacts**:
- modified: docs/wiki/architecture/module-map.md
- modified: docs/wiki/architecture/overview.md
- modified: docs/wiki/domain/business-rules.md
**Changes made**:
- module-map.md: added entity_source.h and scene_saver.h/.cpp to scene submodule table; updated entity.h/cpp, world.h/cpp, property.h, component_info.h descriptions for source tracking and default-value serialization
- overview.md: added entity_source.h and scene_saver.h/.cpp to scene directory tree; added source tracking and SceneSaver bullet points to key behaviors
- business-rules.md: added "Entity source types" section (table, assignment rules), "SceneSaver YAML output format" section (output structure by source type), "Default-value omission rules" section (transform/property defaults omitted)
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance validation passed. No blocking issues found. All 22 acceptance criteria verified by tests. Spec, contract, code, wiki, and ADRs are coherent. Minor warnings: save_to_yaml() returns plain YAML::Node (deviates slightly from ADR-001 but documented/acceptable), tests/component_registry_tests.cpp modified outside strict allowed list (necessary consequence of Step 4b), root_entity_count()/get_root_entity() placed public instead of private (harmless).
**Artifacts**:
- `.specs/sprint-2026-06/scene-source-and-saver/governance-review.md`
**Questions for human**:
none
**Warnings**:
- `save_to_yaml()` returns plain `YAML::Node` instead of `Result<YAML::Node>` — documented deviation from ADR-001's strict rule; exceptions caught at `save_to_file()` call site.
- `tests/component_registry_tests.cpp` modified outside explicit "allowed to change" list — necessary consequence of Step 4b default-omission behavior.
- `root_entity_count()`/`get_root_entity()` placed public instead of private per contract — functionally harmless.
- Duplicate "Test 5" numbering and stale duplicate minimum-set text in contract — cosmetic.
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
