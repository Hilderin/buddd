# Workflow Coordination: Inspector — Component Properties

## Orchestrator

**Feature**: inspector-component-properties
**Status**: completed
**Current step**: completed
**Initial instructions**: Implémenter F-06 (Inspector — Component Properties) du feature breakdown. Afficher tous les composants attachés à l'entité sélectionnée sous forme de sections repliables avec des éditeurs de propriétés typés (float→DragFloat, bool→Checkbox, Color→ColorEdit3, etc.). Les boutons Add/Remove Component sont exclus du scope. Toutes les modifications de propriétés doivent passer par des Commands undo/redo. Les propriétés doivent être affichées dans un tableau 2 colonnes aligné avec la table Transform existante.

**Notes**:
- Scope réduit: pas de Add/Remove Component
- Utilisation de commands undo/redo pour les propriétés
- L'InspectorTypeEditorRegistry doit être enrichi pour gérer le dispatch type_index → draw via std::any
- Engine change nécessaire: ajout de property_serialize/property_deserialize à ComponentInfoBase
- Engine change nécessaire: ajout de yaml_encode/yaml_decode type-erased à TypeRegistry
- Editor change: ajout de draw_any() à InspectorTypeEditor / TypedInspectorEditor
- PropertiesPanel itère les composants de l'entité et utilise le registry pour le dessin

## Decision Log

| # | Decision | Rationale |
|---|---|---|
| D-01 | **Property editing via Commands** | Les modifications de propriétés de composants passent par `SetComponentPropertyCommand` pour undo/redo |
| D-02 | **YAML-based property value transport** | Les getters/setters des propriétés sont des lambdas YAML, pas d'accès mémoire direct possible |
| D-03 | **std::any dispatch dans InspectorTypeEditorRegistry** | Permet au registry de dessiner l'éditeur typé à partir d'un type_index sans que le panel connaisse le type |
| D-04 | **Engine changes approuvées** | ComponentInfoBase: property_serialize/property_deserialize; TypeRegistry: yaml_encode/yaml_decode type-erased |
| D-05 | **2-column table pour les propriétés** | Même pattern que la section Transform: colonne 0 = nom, colonne 1 = éditeur |
| D-06 | **Pas de Add/Remove Component** | Exclu du scope, pourra être ajouté plus tard |

## spec-author

**Status**: completed
**Summary**:
Wrote a full functional spec (SPEC-F-06) for Inspector — Component Properties. Spec covers:
- Component section rendering (collapsible headers below Transform, default collapsed)
- Per-property editing via 2-column table matching Transform layout
- Runtime type dispatch via `InspectorTypeEditorRegistry::draw_any()` with `std::any` + `type_index`
- Engine changes: `ComponentInfoBase::property_serialize`/`property_deserialize`, `TypeRegistry::yaml_encode`/`yaml_decode` type-erased
- Editor changes: `InspectorTypeEditor::draw_any()`, `SetComponentPropertyCommand`, `PropertiesPanel::draw_component_sections()`
- PropertyFlags → EditorFlags mapping, type-to-editor widget mapping table (Color→ColorEdit3 with "rgb" tag)
- 30 acceptance criteria, 5 user stories (P1/P2), 13 edge cases, 9 error cases, DoR self-validation
- Deviations from north-star UX spec (component sections default collapsed, no Add/Remove buttons) documented in D-01, D-02
**Artifacts**:
- `.specs/sprint-2026-06/inspector-component-properties/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Spec Validation

**Status**: approved
**Summary**:
User verbally approved the spec after review.
**Date**: 2026-06-14
**Feedback**: "sound good" — approved.

## implementation-contract-author

**Status**: completed
**Loop note**: Fixed all 4 blocking issues from critic rejection:
1. Replaced all `comp.type_name()` calls (execute, undo, draw_component_sections, AC-25 test) with SceneSaver `typeid` pattern
2. Replaced `EntityId::max_index()` sentinels with `std::optional<size_t>`
3. Added explicit `#include <any>` to `inspector_editors.h` in "Files allowed to change"
4. Replaced `draw_component_sections()` find_info lambda with `std::type_index` → `ComponentInfoBase*` map
**Summary**:
Applied targeted edits to fix 4 blocking issues found by critic:
- B-01: Replaced all 4 `comp.type_name()` calls with SceneSaver `typeid` pattern (execute, undo, draw_component_sections, AC-25 test)
- B-02: Replaced `EntityId::max_index()` sentinels with `std::optional<size_t>` in SetComponentPropertyCommand
- B-03: Added explicit `#include <any>` mention in "Files allowed to change" for `inspector_editors.h`
- B-04: Replaced `draw_component_sections()` find_info lambda with `std::type_index` → `ComponentInfoBase*` map using SceneSaver pattern
Contract is now consistent with established codebase patterns.
**Artifacts**:
- `.specs/sprint-2026-06/inspector-component-properties/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Loop note**: Re-review after fixes to B-01, B-02, B-03, B-04 applied — all resolved, contract approved.
**Summary**:
Approved. All 4 blocking issues resolved: B-01 (comp.type_name() → SceneSaver typeid pattern), B-02 (EntityId::max_index() → std::optional<size_t>), B-03 (#include <any> added), B-04 (draw_component_sections() type_index map lookup). Contract is complete, well-structured, and follows codebase conventions.
**Artifacts**:
- `.specs/sprint-2026-06/inspector-component-properties/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- W-01: TypeEntry private struct modifications should be explicitly noted
- W-02: Null YAML node from OOB property_serialize not guarded before yaml_decode
- W-03: No test for ColorEdit4 fallback (Color without "rgb" tag)
- W-04: Non-pure draw_any default returns false — risk for future custom editors
- W-05: draw_any() implementation placement preference may be inconsistent with existing code
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-14
**Warnings**:
<none>
**Notes**: User approved the spec + contract via question tool. Proceeding to implementation.

## code-implementer

**Status**: completed
**Summary**:
Implemented F-06 Inspector — Component Properties feature according to accepted implementation contract:

1. **Engine: ComponentInfoBase** — Added `property_serialize()` and `property_deserialize()` pure virtual methods to `component_info.h`. Implemented in `ComponentInfo<T>`. Added parallel `raw_getters_` vector to bypass the default-skipping behavior of `Property::serialize()` for editor use.

2. **Engine: TypeRegistry** — Extended `TypeEntry` with `yaml_encode_any`/`yaml_decode_any` dispatch functions. Added type-erased `yaml_encode(type_index, any, ctx)` and `yaml_decode(type_index, node, ctx)` static methods to `TypeRegistry`. Updated `register_type<T>()` to populate dispatch functions automatically.

3. **Editor: InspectorTypeEditor** — Added `draw_any()` virtual method with default `return false` to `InspectorTypeEditor`. Added type-extracting override in `TypedInspectorEditor<T>`. Added static `draw_any()` dispatch to `InspectorTypeEditorRegistry`.

4. **Editor: SetComponentPropertyCommand** — Created header-only command in `src/editor/commands/set_component_property_command.h` with YAML-based value transport, SceneSaver `typeid` pattern for component resolution, `std::optional<size_t>` sentinels.

5. **Editor: PropertiesPanel** — Added `draw_component_sections()` helper to `properties_panel.h/.cpp`. Calls it from `draw_ui()` after `draw_transform_section()`. Uses per-frame type_index→ComponentInfoBase map (SceneSaver pattern) to iterate entity components and render property editors.

6. **Tests** — Created 3 new test files (21 test cases): `component_info_property_serialize_tests.cpp`, `type_registry_type_erased_tests.cpp`, `component_property_commands_tests.cpp`.

7. **Build fix** — Added yaml-cpp include path to editor CMakeLists.txt (needed for editor code that transitively includes engine headers using `<yaml-cpp/yaml.h>`).
**Artifacts**:
- `src/engine/scene/component_registry/component_info.h` — modified (added property_serialize/property_deserialize, raw_getters_)
- `src/engine/scene/component_registry/type_registry.h` — modified (added TypeEntry dispatch functions, static yaml_encode/yaml_decode declarations, updated register_type<T>)
- `src/engine/scene/component_registry/type_registry.cpp` — modified (implemented type-erased yaml_encode/yaml_decode)
- `src/editor/inspector_editors.h` — modified (added #include <any>, draw_any methods, draw_fallback_readonly guard)
- `src/editor/inspector_editors.cpp` — modified (added ImGui context guard to draw_fallback_readonly)
- `src/editor/panels/properties_panel.h` — modified (added draw_component_sections declaration)
- `src/editor/panels/properties_panel.cpp` — modified (added draw_component_sections implementation and includes)
- `src/editor/commands/set_component_property_command.h` — created (new file)
- `src/editor/CMakeLists.txt` — modified (added yaml-cpp include path)
- `tests/engine/component_info_property_serialize_tests.cpp` — created (new test file, 4 test cases)
- `tests/engine/type_registry_type_erased_tests.cpp` — created (new test file, 5 test cases)
- `tests/editor/component_property_commands_tests.cpp` — created (new test file, 12 test cases)
**Questions for human**:
none
**Warnings**:
- The PropertiesPanel rendering test (`draw_ui` calling ImGui) is guarded — it only tests panel construction and entity selection in headless mode, as full ImGui rendering requires a display.
- The `raw_getters_` parallel vector in `ComponentInfo<T>` is needed because `Property::serialize()` skips default values (designed for scene saving), but `property_serialize()` needs the raw value.
**Blocking issues**:
none

## tester

**Status**: completed
**Summary**:
Ran comprehensive tests for F-06 Inspector — Component Properties. All 772 tests pass (768 pre-existing + 4 new). Build is clean with zero warnings. All 30 ACs are covered: 23 verified by automated tests, 3 verified by code review, and 4 identified as manual-only (visual ImGui rendering). No blocking issues found. Added 4 new tests covering AC-09 (compile check), AC-22 (Color tag), AC-28 (PropertyFlags→EditorFlags mapping), and edge case (default value serialization). E2E capture confirms editor launches correctly with Properties panel visible. Manual smoke tests documented for visual verification.
**Artifacts**:
- `.specs/sprint-2026-06/inspector-component-properties/test-report.md`
**Questions for human**:
none
**Warnings**:
- The `raw_getters_` parallel vector in `ComponentInfo<T>` is an implementation detail needed because `Property::serialize()` skips default values. Documented and tested.
- 4 visual-only tests require manual verification (component collapsed state, 2-column table layout, ColorEdit3 vs ColorEdit4 picker, "No editable properties" text).
**Blocking issues**:
none
**Manual tests required**:
See test-report.md section "Manual Tests Required" for 6 manual test scenarios (M-01 through M-06) with step-by-step instructions for visual/display verification.

## Manual Test Validation

**Status**: passed
**Instructions**:
M-01 through M-06 as described in test-report.md
**Human feedback**:
"All passed"
**Date**: 2026-06-14
**Notes**: All 6 manual test scenarios passed successfully. Visual verification complete.

## wiki-agent

**Status**: completed
**Summary**:
Updated the operational wiki for F-06 Inspector — Component Properties. Three wiki pages were modified:
- `docs/wiki/editor/editor-panels.md`: Documented component section rendering, draw_any() flow, SetComponentPropertyCommand, PropertyFlags→EditorFlags mapping, and updated the property editors table.
- `docs/wiki/architecture/module-map.md`: Added set_component_property_command.h to commands list, updated ComponentInfoBase and TypeRegistry descriptions, updated inspector_editors and properties_panel entries.
- `docs/wiki/domain/glossary.md`: Added SetComponentPropertyCommand, draw_any(), property_serialize/property_deserialize, and yaml_encode(type_index)/yaml_decode(type_index) terms.
**Artifacts**:
- `docs/wiki/editor/editor-panels.md` (modified)
- `docs/wiki/architecture/module-map.md` (modified)
- `docs/wiki/domain/glossary.md` (modified)
**Changes made**:
- `editor-panels.md`: Updated status line to reference F-06 component properties; added component section rendering description to Inspector Panel; added draw_any() flow, PropertyFlags→EditorFlags mapping table, and SetComponentPropertyCommand documentation to the Inspector Property Editors subsection.
- `module-map.md`: Added set_component_property_command.h to concrete commands table; updated component_info.h and type_registry.h/.cpp entries with F-06 extensions; updated inspector_editors.h/.cpp with draw_any() additions; updated properties_panel.cpp with draw_component_sections().
- `glossary.md`: Added 4 new terms: SetComponentPropertyCommand, draw_any(), property_serialize/property_deserialize, yaml_encode(type_index)/yaml_decode(type_index).
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above (spec-author → Human Spec Validation → implementation-contract-author → implementation-contract-critic → Human Validation → code-implementer → tester → Manual Test Validation → wiki-agent).
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
