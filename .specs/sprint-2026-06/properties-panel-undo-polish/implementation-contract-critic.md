# Implementation Contract Review — Properties Panel Undo Polish

## Overview

The implementation contract covers all 30 acceptance criteria from the spec and provides detailed, implementable code samples for every changed file. The design decisions (all-in-one SetTransformCommand, dual-parameter try_update_new_value, hidden labels, right-side handles, merge-via-peek) faithfully reflect the approved spec. **Both previous blocking issues (B1, B2) have been resolved**: both `try_update_new_value()` overrides now check `entity_id_` against `ctx.editor.selection().primary()` and verify entity existence before merging. The contract is ready to accept.

## Strengths

1. **Complete AC coverage**: All 30 acceptance criteria map to specific sections and done criteria.
2. **Detailed code samples**: Every complex section (SetTransformCommand, draw_axis_widget flip, float composite widget, merge integration) includes ready-to-use C++ code.
3. **Spec inconsistency resolved**: The spec had an inconsistency between the base `try_update_new_value(YAML::Node)` signature (line 134-139 of spec.md) and the Q-02 resolution requiring `EditorContext const&`. The contract correctly uses a unified dual-parameter signature `(YAML::Node const&, EditorContext const&)`.
4. **Forward-declaration hygiene**: YAML::Node is forward-declared in command.h rather than pulling in the full yaml-cpp header.
5. **Non-goals respected**: No changes to `.h` files, no engine changes, no CMakeLists.txt changes, no new dependencies.
6. **Correct layout flip**: The `draw_axis_widget()` restructuring correctly places InputFloat on the left and the drag handle on the right via `SameLine(0.0f, 0.0f)`.
7. **Proper merge flow**: The `peek_undo()` + `try_update_new_value()` pattern before pushing commands is correctly implemented in both `draw_transform_section()` and `draw_component_sections()`.

## Issues

### Blocking issues

- [x] **B1 — `SetComponentPropertyCommand::try_update_new_value()` missing entity identity check** (RESOLVED): The override (section D) now checks `entity_id_` against `ctx.editor.selection().primary()` and verifies entity existence (`entity.id() == EntityId::none()`) before accepting a merge. Cross-entity merge is correctly prevented.

- [x] **B2 — `SetTransformCommand::try_update_new_value()` missing entity identity check** (RESOLVED): The override (section E) now checks `entity_id_` against `ctx.editor.selection().primary()` and verifies entity existence before accepting a merge. Cross-entity merge is correctly prevented.

### Warnings

- **W1 — Spec debug log format simplified**: The spec specifies detailed debug logging (`"SetTransform: entity={} pos=({},{},{}) rot=({},{},{}) scale=({},{},{})"`). The contract uses a shorter format (`"SetTransform: entity={}"`). Consider restoring the full format for better observability, or confirm the simpler format is acceptable.

- **W2 — `draw_axis_widget()` doc comment stale**: The ASCII art in the function comment still shows `[■ LABEL] [ 0.00 ]` (handle on left). After flipping the layout, the comment should be updated to `[ 0.00 ] [■ LABEL]` to avoid misleading readers.

- **W3 — `EditorFlags scale_flags` duplicated**: The restructured `draw_transform_section()` in section I constructs `EditorFlags scale_flags; scale_flags.min_value = 0.001f;` twice — once in the table branch and once in the fallback branch. Consider extracting the scale flags to a single location to reduce duplication.

## Spec alignment

| Spec requirement | Contract coverage | Status |
|---|---|---|
| SetTransformCommand with native Vec3/Quat, no YAML | Section E — all-in-one, 6 member fields, no YAML | ✅ |
| SetTransformCommand::execute/undo mark_dirty | Section E — both call `ctx.editor.mark_dirty()` | ✅ |
| Hidden labels for float/int/bool/string | Sections G, H — `##val` for all four editors | ✅ |
| Float composite widget (InputFloat + gray handle) | Section G — correct layout, drag speed from step_value | ✅ |
| Axis handle on RIGHT side | Section F — InputFloat first, handle after `SameLine(0,0)` | ✅ |
| peek_undo() | Sections B, C — returns `Command*` or nullptr | ✅ |
| try_update_new_value() virtual | Section A — dual param, base returns false | ✅ |
| SetComponentPropertyCommand::try_update_new_value override | Section D — primary() check + entity existence + YAML comparison + Clone | ✅ |
| SetTransformCommand::try_update_new_value override | Section E — primary() check + entity existence + reads current transform | ✅ |
| draw_transform_section() merge integration | Section I — snapshot before, peek+merge after changed | ✅ |
| draw_component_sections() merge integration | Section J — peek+merge before push | ✅ |
| Scale uses SetTransformCommand (not direct) | Section I — all-in-one capture, same as Position/Rotation | ✅ |
| No inspector_editors.h changes | Non-goal — all changes in .cpp | ✅ |
| No properties_panel.h changes | Non-goal — all changes in .cpp | ✅ |
| Existing tests pass, zero warnings | Done criteria 25, 26 | ✅ |

## Required changes (all resolved)

Both required changes from the initial review have been implemented in the updated contract:

1. **Entity identity guard in `SetComponentPropertyCommand::try_update_new_value()`** — ✅ Section D now includes `primary()` check and entity existence verification.
2. **Entity identity guard in `SetTransformCommand::try_update_new_value()`** — ✅ Section E now includes `primary()` check and entity existence verification.

## Suggested improvements

- Restore the spec's detailed debug log format with position/rotation/scale values for better observability.
- Update the `draw_axis_widget()` doc comment ASCII art to show the flipped layout.
- Extract `EditorFlags scale_flags` to a single declaration before the table/fallback branches.

## Recommendation

**accepted** — All blocking issues from the initial review (B1, B2) have been resolved. The contract is well-structured, covers all acceptance criteria, and is safe to proceed with implementation. The non-blocking warnings (debug log format, doc comment, scale_flags duplication) remain minor style suggestions.
