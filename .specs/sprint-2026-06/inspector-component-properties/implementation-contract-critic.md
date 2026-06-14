# Implementation Contract Review — IMPL-F-06 — Inspector — Component Properties

## Blocking issues

Items that must be resolved before the artifact can be accepted. All previous blocking issues are now resolved.

- [x] **B-01 [CRITICAL] `comp.type_name()` does not exist on `Component` (4 locations)** — **RESOLVED**: Contract now uses SceneSaver `typeid` pattern throughout: `SetComponentPropertyCommand` creates a temporary via `info->create()` and uses `std::type_index(typeid(*tmp))` for component matching; `draw_component_sections()` builds a `std::type_index → ComponentInfoBase*` map using the same pattern; AC-25 test uses `std::type_index(typeid(entity.component_at(0)))`.
- [x] **B-02 `EntityId::max_index()` does not exist** — **RESOLVED**: All sentinel values replaced with `std::optional<size_t>` for `component_index` and `prop_index`.
- [x] **B-03 Missing `#include <any>` in `inspector_editors.h`** — **RESOLVED**: Contract now explicitly lists `#include <any>` as part of allowed changes to `inspector_editors.h` (line 67: `add #include <any>; add draw_any()...`).
- [x] **B-04 `PropertiesPanel::draw_component_sections()` uses `type_name` lookup** — **RESOLVED**: `draw_component_sections()` now builds a `std::type_index → ComponentInfoBase*` map using the SceneSaver pattern and looks up `typeid(comp)` directly; no `find_info` lambda.

## Warnings

Non-blocking concerns for awareness:

- **W-01 `TypeEntry` struct is private, modifications documented but could surprise** — The contract adds `yaml_encode_any` and `yaml_decode_any` members to `TypeRegistry::TypeEntry`, which is defined in the `private` section of `TypeRegistry` (line 71 of `type_registry.h`). This is technically fine (the class modifies its own private members), but the contract should explicitly note that the struct is private and the modifications are within the class definition, so no access changes are needed.

- **W-02 `property_serialize` returns null YAML node on out-of-bounds — downstream not checked** — `ComponentInfo<T>::property_serialize()` returns `YAML::Node()` (null node) when `index >= properties_.size()`. This null node is then passed to `TypeRegistry::yaml_decode()` in `draw_component_sections()`. The behavior of `yaml_decode` with a null node is undefined (depends on the registered callback). Consider either: (a) adding a null-check before `yaml_decode`, or (b) documenting that `yaml_decode` callbacks must handle null nodes gracefully. In practice this path should never be hit since the panel iterates within bounds, but defensive code is preferable.

- **W-03 Missing test for ColorEdit4 fallback (Color without "rgb" tag)** — AC-22 tests ColorEdit3 for properties with `tag("rgb")`. There is no test for `math::Color` properties that lack the "rgb" tag, which should render with `ImGui::ColorEdit4` (with alpha). Add a unit test that verifies the draw fallback or at minimum add a note that this is implicitly tested by the existing Color editor.

- **W-04 `draw_any` on base `InspectorTypeEditor` returns `false` by default** — The contract makes `draw_any()` a non-pure virtual returning `false`. Future custom editors that inherit directly from `InspectorTypeEditor` (not via `TypedInspectorEditor<T>`) must override `draw_any()` to be usable through the type-erased dispatch. The contract documents this correctly but worth flagging: if someone adds a custom editor and forgets to override `draw_any()`, it silently returns `false` (no edit possible). Consider making it pure virtual with a comment, or keep as-is but note the risk.

- **W-05 `InspectorTypeEditorRegistry::draw_any()` implementation location ambiguity** — The contract says "Preference: keep in .h (inline)" for symmetry with `draw<T>()` template, but `draw_any()` is not a template — it can live in the `.cpp`. The existing `get()` and `draw_fallback_readonly()` are in the `.cpp`. For consistency with the existing code layout, consider putting the implementation in `inspector_editors.cpp` instead of the header, unless there's a measurable performance reason to inline. Not a blocker but a style inconsistency.

- **W-06 `StaticCast` for `component_info_cache_` as `std::vector<const ComponentInfoBase*>`** — `all_types()` returns `std::span<const ComponentInfoBase*>`, which is cheap to copy. Storing as a `std::vector` member and re-assigning each frame is fine, but consider just holding a local `span` and building the `type_index` map from it, to avoid the vector allocation per frame.

## Required changes (all resolved)

All previously requested changes have been applied:

1. ✅ **Replace all `comp.type_name()` calls** — Done: SceneSaver `typeid` pattern used throughout.
2. ✅ **Replace `EntityId::max_index()`** — Done: `std::optional<size_t>` used.
3. ✅ **Add `#include <any>` to `inspector_editors.h`** — Done: explicitly listed in "Files allowed to change".
4. ✅ **Rewrite `draw_component_sections()` lookup** — Done: `type_index` map via SceneSaver pattern.
5. ✅ **Fix AC-25 test assertion** — Done: uses `typeid(entity.component_at(0))`.

## Suggested improvements

Optional ideas (not required):

- Add a short section documenting the "type_index resolution pattern" as a reference for future developers, pointing to `SceneSaver::build_type_to_info_map()` as the canonical implementation.
- In `draw_component_sections()`, add a null-check on the YAML node before calling `yaml_decode`, with a skip+warning.
- Add a unit test for `math::Color` property without "rgb" tag verifying ColorEdit4 usage (could be a snapshot-string test or documented as implicitly covered).
- Consider putting `InspectorTypeEditorRegistry::draw_any()` in `inspector_editors.cpp` for consistency with `get()` and `draw_fallback_readonly()`.

## Gate decision

- **Status**: **approved** — all 4 blocking issues resolved; contract now correctly uses SceneSaver `typeid` pattern, `std::optional<size_t>`, includes `<any>`, and has proper `type_index` map lookup.
- **Blocking issues**: none (all 4 resolved)
- **Questions for human**: none
- **Warnings**:
  - W-01: `TypeEntry` private struct modifications should be explicitly noted
  - W-02: Null YAML node from OOB `property_serialize` not guarded before `yaml_decode`
  - W-03: No test for ColorEdit4 fallback (Color without "rgb" tag)
  - W-04: Non-pure `draw_any` default returns `false` — risk for future custom editors
  - W-05: `draw_any()` implementation placement preference may be inconsistent with existing code
  - W-06: (partially addressed) Per-frame map allocation noted as negligible by contract

## Review summary

**Approved.** All 4 previously blocking issues have been resolved:
- **B-01**: `comp.type_name()` replaced with SceneSaver `typeid` pattern in all 4 locations.
- **B-02**: `EntityId::max_index()` sentinel replaced with `std::optional<size_t>`.
- **B-03**: `#include <any>` added and explicitly listed in "Files allowed to change".
- **B-04**: `draw_component_sections()` now uses a `std::type_index → ComponentInfoBase*` map built via the SceneSaver pattern.

The contract is well-structured, covers all spec goals, follows established codebase patterns, and contains sufficient test coverage. 5 non-blocking warnings remain (W-01 through W-05) for awareness.
