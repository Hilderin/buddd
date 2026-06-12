# Governance Review — Scene Source Tracking and Saver

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec intent matches implementation**: Spec (SPEC-2026-001) defines EntitySource tracking + SceneSaver with 22 acceptance criteria. Implementation contract covers all 22 ACs with detailed Step-by-step instructions. Code matches contract per code-review verdict ("No blocking issues found"). Tests verify all 22 ACs pass.
- [x] **Wiki reflects spec and contract**: `docs/wiki/domain/business-rules.md` documents entity source types, SceneSaver YAML output format, and default-value omission rules — consistent with spec §YAML output structure and AC-020/021/022. `docs/wiki/architecture/module-map.md` and `docs/wiki/architecture/overview.md` list `entity_source.h`, `scene_saver.h/.cpp`, source tracking accessors — matching the contract's file listing.
- [x] **Spec and contract agree on API shape**: Both define `SceneSaver::save_to_file(path) -> Result<void>` and `save_to_yaml() -> YAML::Node`. Both omit `Result` wrapper on `save_to_yaml()` — this is a documented exception to ADR-001 because exceptions are caught in `save_to_file()`.
- [x] **Contract and code agree on access levels**: Contract specified `root_entity_count()`/`get_root_entity()` as private with `friend class SceneSaver`. Implementer placed them public — code review notes this as a minor deviation but functionally harmless. All other access levels match.
- [x] **Default-value omission behavior consistent across all documents**: Spec AC-020/021/022, contract Step 4b + Tests 12/13/14, wiki business-rules.md §Default-value omission rules, and code all agree: default transform fields → omitted, default component properties → omitted, all-default component → no `properties:` key.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-028 (Component Type Registry)**: Respected — SceneSaver uses `registry_.all_types()` for reverse-lookup map, `ComponentInfoBase::serialize()` and `serialize_component()` for serialization. The `DefaultChecker` addition to `Property` and `ComponentInfo<T>` is an extension, not a violation. The reverse-lookup mechanism (build map via `create()` + `typeid()`) is implemented entirely within SceneSaver without modifying the registry.
- [x] **ADR-016 (yaml-cpp Dependency)**: Respected — Forward declaration of `YAML::Node` in public header `scene_saver.h` (no `<yaml-cpp/yaml.h>` include). Full include only in `scene_saver.cpp`. YAML exceptions caught and wrapped in `Result` errors in `save_to_file()`. PRIVATE linkage preserved.
- [x] **ADR-019 (Architecture Boundaries)**: Respected — `SceneSaver` lives in `src/engine/scene/`, uses engine abstractions only (`World`, `ComponentRegistry`, `AssetManager`). No SDL3/OpenGL/GLM headers outside `src/engine/`.
- [x] **ADR-001 (Result/Error Pattern)**: Mostly respected — `save_to_file()` returns `Result<void>` with `Error::Category::IoFailed`. Minor deviation: `save_to_yaml()` returns plain `YAML::Node` without `Result` wrapper; this is documented and consistent with the spec (exceptions from yaml-cpp are caught at the `save_to_file()` call site).
- [x] **ADR-011 (Ownership, Nullability, Lifetime)**: Respected — `SceneSaver` stores non-owning references (`World&`, `ComponentRegistry&`, `AssetManager&`). No raw-owner members. `type_to_info_` stores `const ComponentInfoBase*` obtained from `registry_.all_types()`.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/architecture/module-map.md` — Updated: `entity_source.h`, `scene_saver.h/.cpp`, `Entity::source()`/`set_source()`, `DefaultChecker` on `Property`, default-value skip on `ComponentInfo::serialize()`. Matches current state.
- [x] `docs/wiki/architecture/overview.md` — Updated: directory tree includes `entity_source.h` and `scene_saver.h/.cpp` under `scene/`. Key behaviors section adds entity source tracking and SceneSaver serialization. Matches current state.
- [x] `docs/wiki/domain/business-rules.md` — Updated: "Entity source types" section with assignment rules, "SceneSaver YAML output format" section with per-source-type output structure, "Default-value omission rules" section. Matches current state.
- [x] Wiki does not contradict ADRs — Business-rules.md describes source types and saver behavior as operational conventions, not as governance-level decisions. All ADR-level decisions (yaml-cpp, component registry, architecture boundaries) are referenced and respected.

## Warnings

Non-blocking concerns for awareness:

- `save_to_yaml()` returns plain `YAML::Node` instead of `Result<YAML::Node>`, deviating slightly from ADR-001's strict rule that all fallible APIs return `Result<T>`. This is consistent with the spec and contract, and exceptions are caught at the `save_to_file()` call site. Documented and acceptable.
- `tests/component_registry_tests.cpp` was modified to update a mesh renderer test for the new default-omission behavior — not in the explicit "allowed to change" list, but a necessary consequence of contract Step 4b. Not blocking.
- `root_entity_count()`/`get_root_entity()` placed in public instead of private per contract specification — functionally harmless.
- `property.cpp` modified in support of `property.h` changes — acceptable as the `.cpp` must match header declarations.
- Duplicate "Test 5" numbering in contract (two tests labeled Test 5) and stale duplicate minimum-set text — cosmetic only.
- `build_type_to_info_map()` creates temporary component instances at construction time — minor performance consideration.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

No ADR updates are required. The wiki has already been updated by the wiki-agent to reflect the new feature state. No governance documents need amendment.
