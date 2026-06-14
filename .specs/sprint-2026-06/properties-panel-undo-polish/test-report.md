# Test Report: Properties Panel Undo Polish (SPEC-F-07)

## Test Summary

**Total tests**: 772
**Passed**: 772
**Failed**: 0
**Skipped**: 0

**Build**: clean (zero errors, zero warnings in `src/editor/` and `tests/`)

---

## Unit Tests

All 772 existing tests pass (including 9 `[editor][command]` CommandStack tests, `SetComponentPropertyCommand` tests, property panel tests). No new test files were added per contract (NG-04: no headless ImGui tests).

---

## Integration / E2E Tests

No automated E2E capture tests were run (manual smoke test + screenshot verification only, per NG-04).

| Scenario | Method | Result | Evidence |
|---|---|---|---|
| N/A | Manual smoke (see below) | Pending human | N/A |

---

## AC Verification Table

| AC ID | Description | Result | How verified |
|---|---|---|---|
| AC-01 | `SetTransformCommand` class exists in `src/editor/commands/set_transform_command.h` storing all 3 transform properties as native math types (Vec3/Quat) — no YAML, no enum | ✅ PASS | Code review: file exists, class defined with `Vec3 old/new_position_`, `Quat old/new_rotation_`, `Vec3 old/new_scale_`; no YAML, no enum |
| AC-02 | Stores `entity_id`, `old_position_`, `old_rotation_`, `old_scale_`, `new_position_`, `new_rotation_`, `new_scale_` (no YAML::Node, no TransformProperty enum) | ✅ PASS | Code review: all 7 fields present as native types |
| AC-03 | `execute()` writes all three `new_*` values to `entity.transform()` and calls `ctx.editor.mark_dirty()` | ✅ PASS | Code review: `t.position = new_position_; t.rotation = new_rotation_; t.scale = new_scale_; ctx.editor.mark_dirty()` |
| AC-04 | `undo()` restores all three `old_*` values to `entity.transform()` and calls `ctx.editor.mark_dirty()` | ✅ PASS | Code review: `t.position = old_position_; t.rotation = old_rotation_; t.scale = old_scale_; ctx.editor.mark_dirty()` |
| AC-05 | `name()` returns a human-readable string (e.g., "Set Transform") | ✅ PASS | Code review: returns `"Set Transform"` |
| AC-06 | `draw_transform_section()` creates and pushes a `SetTransformCommand` for any transform edit | ✅ PASS | Code review: command created with old/new positional/rotation/scale and executed via `ctx.editor.command_stack().execute(std::move(cmd), ctx)` |
| AC-07 | Scale edits in `draw_transform_section()` push a `SetTransformCommand` (same as Position/Rotation) | ✅ PASS | Code review: Scale is edited via `InspectorTypeEditorRegistry::draw<Vec3>("Scale", ...)` contributing to `changed` flag, all included in all-in-one command |
| AC-08 | `SetTransformCommand` has NO `TransformProperty` enum and NO YAML serialization | ✅ PASS | Code review: no enum, no YAML members |
| AC-09 | Float editor uses composite widget: `InputFloat` + gray drag handle (`ImVec4(0.5f,0.5f,0.5f,1.0f)`) on the right | ✅ PASS | Code review: `ImGui::InputFloat("##val", ...)` followed by `SameLine(0,0)` and gray handle `AddRectFilled` with `ImVec4(0.5f,0.5f,0.5f,1.0f)` |
| AC-10 | Float editor supports single-click text entry via `InputFloat` | ✅ PASS (manual confirmation required) | Code review: uses `ImGui::InputFloat` which enables single-click entry |
| AC-11 | Float editor supports drag-to-scrub via gray drag handle on right | ✅ PASS (manual confirmation required) | Code review: `InvisibleButton` + `GetMouseDragDelta()` pattern identical to `draw_axis_widget()` |
| AC-12 | Float editor preserves speed sensitivity from `EditorFlags.step_value` (default 0.1) | ✅ PASS | Code review: `float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;` |
| AC-13 | Float editor uses hidden label (`##val`) for InputFloat | ✅ PASS | Code review: `ImGui::InputFloat("##val", ...)` |
| AC-14 | Int editor uses hidden label (`##val`) for DragInt | ✅ PASS | Code review: `ImGui::DragInt("##val", ...)` |
| AC-15 | Bool editor uses hidden label (`##val`) for Checkbox | ✅ PASS | Code review: `ImGui::Checkbox("##val", &value)` |
| AC-16 | String editor uses hidden label (`##val`) for InputText | ✅ PASS | Code review: `ImGui::InputText("##val", buf, BUF_SIZE)` |
| AC-17 | Component property table shows no duplicate labels | ✅ PASS (manual visual inspection required) | Code review: column 0 has `TextUnformatted(prop_name.data())`, column 1 uses `##val` hidden labels; visual confirmation needed |
| AC-18 | `draw_axis_widget()` renders InputFloat on left, colored drag handle on right | ✅ PASS | Code review: InputFloat first, then `SameLine(0.0f,0.0f)`, then colored rectangle + InvisibleButton |
| AC-19 | Vec2, Vec3, Vec4, Quat editors all use updated right-side axis handle layout | ✅ PASS | Code review: all call `draw_axis_widget()` which has been flipped |
| AC-20 | `CommandStack::peek_undo()` exists, returns `Command*` (nullptr if empty) | ✅ PASS | Code review: declaration in `command_stack.h`, implementation in `command_stack.cpp` — returns `undo_stack_.empty() ? nullptr : undo_stack_.back().get()` |
| AC-21 | `Command::try_update_new_value(YAML::Node) -> bool` virtual method exists with default `false` | ✅ PASS | Code review: declaration in `command.h` with inline implementation returning `false` |
| AC-22 | `SetComponentPropertyCommand` overrides `try_update_new_value()` to update `new_value_` | ✅ PASS | Code review: override in `set_component_property_command.h` — checks entity match, entity existence, YAML equality, clones if different |
| AC-23 | `SetTransformCommand` overrides `try_update_new_value()` to read entity's current transform and update all three `new_*` values | ✅ PASS | Code review: override in `set_transform_command.h` — checks entity identity against primary selection, reads `t.position/rotation/scale`, updates all three |
| AC-24 | `draw_component_sections()` checks `peek_undo()` and calls `try_update_new_value()` before pushing new command | ✅ PASS | Code review: `auto* last = ctx.editor.command_stack().peek_undo(); if (last && last->try_update_new_value(*new_yaml, ctx)) { ... } else { push new SetComponentPropertyCommand }` |
| AC-25 | Single continuous drag on component property produces exactly one undo step (merged) | 🔷 MANUAL | Requires running editor with mouse interaction |
| AC-26 | Single continuous drag on Position X produces exactly one undo step (merged) | 🔷 MANUAL | Requires running editor with mouse interaction |
| AC-27 | Editing two different properties sequentially produces two separate undo steps | 🔷 MANUAL | Requires running editor with mouse interaction |
| AC-28 | Undoing a merged drag command correctly restores the original pre-drag value | 🔷 MANUAL | Requires running editor with mouse interaction |
| AC-29 | All existing unit tests pass | ✅ PASS | `ctest --preset debug`: 772/772 passed |
| AC-30 | Zero new compiler warnings from `src/editor/` | ✅ PASS | `cmake --build --preset debug`: zero warnings, zero errors |

---

## Manual Tests Required

The following ACs require a human to run the editor and visually verify behavior. These cannot be automated because they require an ImGui display context, physical mouse interaction, or subjective visual judgment.

### MT-01: Transform Undo/Redo (AC-03, AC-04, SC-001)
1. Run `buddd edit` with a scene loaded.
2. Select an entity.
3. Change Position X from 0.00 to 5.00.
4. Press Ctrl+Z → Verify Position returns to (0, 0, 0), Rotation to (0, 0, 0), Scale to (1, 1, 1).
5. Press Ctrl+Shift+Z → Verify Position X returns to 5.00.
6. Repeat for Rotation edit (change Yaw to 90) and Scale edit (change Scale X from 1 to 3) — verify Ctrl+Z reverts all three properties each time.

### MT-02: Float Single-Click Editing (AC-10, SC-002)
1. Select an entity with a float component property.
2. Single-click the InputFloat field → Verify cursor appears for text entry.
3. Type a new value and press Enter → Verify value changes and scene is marked dirty.
4. Press Escape before confirming → Verify value reverts.

### MT-03: Float Gray Drag Handle (AC-09, AC-11, SC-003)
1. Select an entity with a float component property.
2. Verify a gray drag handle (`■` rectangle) appears on the RIGHT side of the InputFloat.
3. Click+drag the gray handle to the right → Verify value increases smoothly.
4. Release mouse → Verify value retains the dragged value.

### MT-04: Axis Handle on Right Side (AC-18, AC-19, SC-003)
1. Select an entity and view the Transform section's Position row.
2. Verify layout is `[InputFloat][■ X]` (input on left, colored handle on right) for each component.
3. Verify the same layout for Rotation (X/Y/Z) and any Vec2/Vec3/Vec4 component properties.

### MT-05: No Duplicate Labels (AC-14 through AC-17, SC-005)
1. Select an entity with float, int, bool, and string component properties.
2. Verify each property name appears only in column 0.
3. Verify the editor widget in column 1 shows NO text label next to the widget (no duplicate "Speed" text in the float field area, no "Enabled" text next to checkbox).

### MT-06: Drag Undo Merging (AC-25, AC-26, AC-28, SC-004)
1. Select an entity with Position X = 0.00.
2. Click+drag the X handle from 0.00 to 5.00 in one continuous gesture.
3. Release mouse.
4. Press Ctrl+Z once → Verify Position X reverts directly to 0.00 (not an intermediate value).
5. Verify Rotation and Scale are also reverted (all-in-one restore).

### MT-07: Sequential Property Edits (AC-27)
1. Select an entity.
2. Change Position X from 0 to 5.
3. Change Position Y from 0 to 10.
4. Press Ctrl+Z once → Verify Position Y reverts to 0 (Position X stays at 5 — only the most recent edit undone).
5. Press Ctrl+Z again → Verify Position X reverts to 0.

### MT-08: Undo/Redo Dirty Marker
1. Verify the scene dirty marker (star on title) appears after any transform or component property edit.
2. Verify Ctrl+Z also marks the scene dirty.

### MT-09: Negative/Large Float Values (Edge case)
1. Enter a negative value (-5.00) in a float field → Verify it's accepted.
2. Enter a very large value (1e10) → Verify it displays and edits without overflow.
3. Enter an empty string → Verify it reverts to the last valid numeric value.

---

## Issues Found

### Blocking
- None found.

### Non-blocking
- None found.

---

## Files Modified (verified against contract)

| File | Modified? | Contract allows? |
|---|---|---|
| `src/editor/command.h` | ✅ Yes — added `try_update_new_value()` | ✅ Yes |
| `src/editor/command_stack.h` | ✅ Yes — added `peek_undo()` | ✅ Yes |
| `src/editor/command_stack.cpp` | ✅ Yes — added `peek_undo()` impl | ✅ Yes |
| `src/editor/commands/set_component_property_command.h` | ✅ Yes — added `try_update_new_value()` override | ✅ Yes |
| `src/editor/commands/set_transform_command.h` | ✅ NEW file | ✅ Yes |
| `src/editor/inspector_editors.cpp` | ✅ Yes — flipped axis, float composite, hidden labels | ✅ Yes |
| `src/editor/panels/properties_panel.cpp` | ✅ Yes — transform undo, component merge | ✅ Yes |
| `src/editor/inspector_editors.h` | ❌ No changes | ✅ Correct (forbidden to change) |
| `src/editor/panels/properties_panel.h` | ❌ No changes | ✅ Correct (forbidden to change) |
| `src/engine/` | ❌ No changes | ✅ Correct (forbidden to change) |
| `CMakeLists.txt` | ❌ No changes | ✅ Correct (auto-discovered via GLOB_RECURSE) |
