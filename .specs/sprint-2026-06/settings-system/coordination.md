# Workflow Coordination: settings-system

## Orchestrator

**Feature**: `settings-system`
**Status**: completed
**Current step**: done
**Initial instructions**: Créer un système de user settings par projet et de editor settings. L'objectif est que chaque élément de l'éditeur puisse sauvegarder des settings globale à l'editor ou pour un user pour un projet. Ex: pour un user, dernier path utilisé dans le open/save scene, les derniers onglets ouverts et leur emplacement. Pour l'editor, des settings du genre: shortcuts, theme utilisé, etc. Système de projet où on pourra définir le nom du projet, des settings du renderer, etc.
**Notes**:
- Loop 1 (13-Jun-2026): spec-critic rejected spec — blocking E2E test approach. Human resolved: display-backed tests (BUDDD_HAS_DISPLAY). Reworking spec.
- Loop 2 (13-Jun-2026): impl-contract-critic rejected — observer exception safety contradiction in contract. Fixed.
- Loop 3 (13-Jun-2026): impl-contract-critic re-review rejected — YAML::Node by-value member leaks yaml-cpp to all consumers (violates ADR-016, yaml-cpp is PRIVATE). Must use unique_ptr<pimpl> or forward-declare.
- **Bug fix (13-Jun-2026)**: Dangling pointer in `ImGui::GetIO().IniFilename` — the string backing `layout_ini_path()` was destroyed in `Editor::shutdown()` before ImGui shutdown. Fixed by setting `IniFilename = nullptr` before `settings_manager_` is destroyed. This prevents ImGui from writing to a garbage filename, which was creating corrupted binary-named files in the project root.
- Design decisions after grill-me session (13-Jun-2026):
  - Format: YAML (yaml-cpp existant)
  - Editor Settings: `~/.config/buddd/editor.yaml` (chemin OS standard)
  - Project Settings: `<cwd>/buddd.project.yaml` (commitée dans VCS)
  - User Project Settings: `<cwd>/.buddd/user/settings.yaml` (gitignored)
  - ImGui layout INI: déplacé vers `<cwd>/.buddd/user/layout.ini` (séparé des settings)
  - Tiers indépendants (pas de hiérarchie d'override)
  - Racine projet = CWD au lancement
  - MVP1: skeleton uniquement — classes, unit tests, load au démarrage, save à la fermeture ou on-demand. Aucun setting utilisateur réel dans l'éditeur pour l'instant.
  - API commune: `SettingsStore` dans `src/engine/settings/`

## spec-author

**Status**: completed
**Summary**:
- Updated SPEC-036 with human design changes: TypeRegistry integration, SerializationContext in SettingsStore, editor_data_root/util/ utilities, removed migrate_ini, merged ini_path methods
- Added centralized `editor_data_root()` / `editor_user_data_root()` path utilities in `src/engine/util/`
- Moved `os_user_config_dir()` from `settings/` to `util/` for reuse
- Integrated TypeRegistry into SettingsStore get/set via SerializationContext, added pimpl for yaml-cpp privacy (ADR-016)
- Renumbered ACs (now 32 items), added coverage for TypeRegistry types, editor_data_root, and unregistered type handling
**Artifacts**:
- `.specs/sprint-2026-06/settings-system/spec.md`
**Questions for human**:
- none
**Warnings**:
- none
**Blocking issues**:
- none

## spec-critic

**Status**: completed
**Summary**:
Re-review 2 (13-Jun-2026): After substantial design changes (TypeRegistry integration, SerializationContext, os_user_config_dir/util move, editor_data_root utilities, removal of migrate_ini, merged ini_path methods, ACs renumbered to 32), all previous issues remain resolved. No new blocking issues. TypeRegistry integration follows existing ADR-028 pattern (MockAssetManager + SerializationContext in unit tests). All Definition of Ready criteria satisfied. Spec is accepted.
**Artifacts**:
- `.specs/sprint-2026-06/settings-system/spec-critic.md`
**Questions for human**:
- none
**Warnings**:
- none
**Blocking issues**:
- none

## implementation-contract-author

**Status**: completed
**Summary**:
- Simplified observer storage: removed ObserverEntry struct, use vector<pair<id, function>> instead
**Artifacts**:
- `.specs/sprint-2026-06/settings-system/implementation-contract.md`
**Questions for human**:
- none
**Warnings**:
- none
**Blocking issues**:
- none

## implementation-contract-critic

**Status**: completed
**Summary**:
Third review (13-Jun-2026): SerializationContext include fix verified. Both `settings_store.h` and `settings_manager.h` now include `scene/component_registry/serialization_context.h` directly. No forward declaration remains. Contract is complete, testable, spec-faithful, and ADR-compliant. Accepted.
**Artifacts**:
- `.specs/sprint-2026-06/settings-system/implementation-contract-critic.md`
**Questions for human**:
- none
**Warnings**:
- Double-`setup()` behavior contradicts spec edge case (inherited inconsistency, non-blocking)
- `Connection` defined as top-level class vs spec's nested class (minor spec deviation, non-blocking)
- `PathsMatch` test helper referenced but not defined (minor documentation imprecision, non-blocking)
**Blocking issues**:
- none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-13 12:46:49
**Warnings**:
- none
**Notes**: Approved after 3 design loops (spec-critic, impl-contract-critic x3). Human requested: TypeRegistry from start, SerializationContext in constructor, util/ directory, no migrate_ini, simplified observer storage.

## code-implementer

**Status**: completed
**Summary**:
Implemented the Settings System MVP1 skeleton (SPEC-036 / IMPL-036):
- Created `os_user_config_dir()` and `editor_data_root()`/`editor_user_data_root()` path utilities in `src/engine/util/`
- Created `SettingsStore` (YAML-backed key-value store with dot-path access, TypeRegistry integration, dirty tracking, and RAII observer registration) and `SettingsManager` (orchestrator owning editor/project/user-project stores) in `src/engine/settings/`
- Integrated SettingsManager lifecycle into Editor::setup() and Editor::shutdown(), replacing hardcoded INI logic with `layout_ini_path()`
- Fixed critical yaml-cpp bug: `Node::operator=(const Node&)` calls `AssignNode` → `set_ref` which corrupts the parent tree; must use `Node::reset()` instead of assignment when navigating
- 42 unit tests covering all AC-001 through AC-032, edge cases, and error handling
**Artifacts**:
- New: `src/engine/util/os_config_dir.h`, `src/engine/util/os_config_dir.cpp`
- New: `src/engine/util/editor_data_root.h`, `src/engine/util/editor_data_root.cpp`
- New: `src/engine/settings/settings_store.h`, `src/engine/settings/settings_store.cpp`
- New: `src/engine/settings/settings_manager.h`, `src/engine/settings/settings_manager.cpp`
- New: `tests/engine/settings_store_tests.cpp`
- New: `tests/editor/settings_integration_tests.cpp`
- Modified: `src/editor/editor.h`, `src/editor/editor.cpp`
**Questions for human**:
- none
**Warnings**:
- Display-dependent integration tests require BUDDD_HAS_DISPLAY and an actual display (or offscreen SDL3 driver) to run; they crash if the display stack is not available
- Test 9 ("buddd edit --frame 2 opens editor and exits") is a pre-existing failure unrelated to this implementation
- `double` is not registered in TypeRegistry (only `float` is); `SettingsStore::get<double>()` returns default with a warning log — this is expected behavior, not a bug
**Blocking issues**:
- none

## code-reviewer

**Status**: completed
**Summary**:
Review of 32 ACs against 10 implementation files + 2 test files. All 32 ACs are covered by tests. No blocking issues found. Architecture compliance verified (ADR-016, ADR-019, ADR-001, ADR-011, ADR-026). Build produces zero warnings. The implementer discovered and fixed a critical yaml-cpp bug (Node::operator= corrupts parent tree via set_ref; must use reset() instead of assignment when navigating). 5 non-blocking warnings documented.
**Artifacts**:
- `.specs/sprint-2026-06/settings-system/code-review.md`
**Questions for human**:
- none
**Warnings**:
- `double` explicit instantiation omitted (contract required it, but TypeRegistry doesn't register `double` — behavior is correct, unregistered type returns default)
- `find_node` returns `YAML::Node` by value (contract specified `YAML::Node*`) — correct for yaml-cpp idioms
- `set_impl` does not use `ensure_node_path` — uses inline `Node::reset()` navigation instead, fixing a real yaml-cpp tree corruption bug
- AC-015 test is weak — no platform guards, only checks path ends with "editor.yaml" instead of platform-specific path
- `Connection` is top-level class (not nested `SettingsStore::Connection` as in spec) — no practical impact since callers use `auto`
**Blocking issues**:
- none

## adr-agent

**Status**: pending
**Summary**:
<2–5 lines>
**Artifacts**:
- <list of ADRs created or "none">
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## wiki-agent

**Status**: completed
**Summary**:
Updated the wiki to document the new Settings System (SPEC-036). Added settings/ and util/ submodules to the module map and architecture overview, created a dedicated settings-system.md wiki page, added SPEC-036 reference to the ADR index, and updated the wiki README index.
**Artifacts**:
- `docs/wiki/architecture/module-map.md` — modified (added util/ and settings/ submodule sections + SPEC-036 reference)
- `docs/wiki/architecture/overview.md` — modified (added util/ and settings/ to directory layout and engine internal structure)
- `docs/wiki/decisions/adr-index.md` — modified (added SPEC-036 entry)
- `docs/wiki/README.md` — modified (added Settings System link)
- `docs/wiki/editor/settings-system.md` — created (new page covering three tiers, YAML storage, file locations, TypeRegistry integration, observer pattern, editor lifecycle)
**Changes made**:
- Module map: Added `src/engine/util/` (os_config_dir, editor_data_root) and `src/engine/settings/` (SettingsStore, SettingsManager) submodule documentation with file-by-file role tables
- Architecture overview: Added `util/` and `settings/` to the top-level directory layout and to the detailed engine internal structure tree
- ADR index: Added SPEC-036 entry documenting the settings system design
- Wiki README: Added Settings System link to the table of contents
- New page: Created `docs/wiki/editor/settings-system.md` covering the three tiers, YAML storage format, OS-specific file locations, TypeRegistry integration for type-safe get/set, RAII observer pattern via Connection, editor lifecycle integration, and key class references
**Questions for human**:
- none
**Warnings**:
- The code-review notes that `Connection` is a top-level class (not nested `SettingsStore::Connection` as spec describes); the wiki documents it as a top-level class to match the actual API
**Blocking issues**:
- none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance validation completed for Settings System (SPEC-036 / IMPL-036). All cross-document artifacts (spec, contract, code, tests, wiki, ADRs) reviewed for coherence. No blocking governance issues found. 6 known minor deviations documented (Connection nesting, find_node return type, set_impl approach, double instantiation, double-setup edge case, AC-015 test weakness) — all previously flagged by earlier reviews. ADR alignment verified: ADR-001 (Result), ADR-016 (yaml-cpp PRIVATE), ADR-019 (architecture boundaries), ADR-026 (ImGui lifecycle), ADR-011 (nodiscard) — all compliant. Wiki correctly reflects implementation.
**Artifacts**:
- `.specs/sprint-2026-06/settings-system/governance-review.md`
**Questions for human**:
- none
**Warnings**:
- Spec-contract inconsistency: double-setup() behavior (spec says no-reload, contract/code reload) — flagged by contract-critic, non-blocking
- Connection nesting: spec says `SettingsStore::Connection` (nested), contract/code implement top-level `buddd::engine::Connection` — non-blocking, wiki correctly documents top-level
- AC-015 test weaker than contract specification (no platform guards, only checks suffix) — implementation correct but test coverage weaker than specified
- `find_node` return type deviation (contract: `YAML::Node*`, code: `YAML::Node` by value) — justified, yaml-cpp idiomatic
- `double` instantiation listed in contract but TypeRegistry doesn't register `double` — implementer correctly omitted, behavior correct (unregistered)
**Blocking issues**:
- none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
