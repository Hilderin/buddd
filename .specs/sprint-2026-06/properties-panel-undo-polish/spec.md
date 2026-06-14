# SPEC-F-07 — Properties Panel Undo Polish

## Problem

The Properties Panel (implemented in F-05, UX-polished in F-06) has four usability and correctness issues that affect the editing experience:

1. **Transform edits have no undo** — modifying entity Position, Rotation, or Scale via `draw_transform_section()` directly mutates `entity.transform()` without pushing a Command to the `CommandStack`. Component property edits (via `draw_component_sections()`) use `SetComponentPropertyCommand` and are undoable. The inconsistency means users cannot undo accidental transform edits via Ctrl+Z.

2. **Double labels for float/int/bool/string editors** — In `draw_component_sections()`, the property name is displayed in table column 0 via `ImGui::TextUnformatted()`, and the editor is called in column 1 with the same `prop_name` as label. For float (`ImGui::DragFloat`), int (`ImGui::DragInt`), bool (`ImGui::Checkbox`), and string (`ImGui::InputText`), the label appears both in column 0 AND embedded inside the widget, creating visual clutter.

3. **Float fields lack single-click editing** — The float editor uses `ImGui::DragFloat` which requires click+drag (not single-click text entry). The Vec editors already use the composite axis widget (`draw_axis_widget()`) with `ImGui::InputFloat` for single-click entry. Additionally, the vec editor layout places the drag handle on the LEFT of the input field, which is inconsistent with the desired right-side placement.

4. **Drag-based undo granularity floods the undo stack** — During a drag operation on any property (float/vec/transform), every frame that changes the value pushes a new command. This requires many Ctrl+Z presses to fully revert a single drag gesture. Commands targeting the same entity + same property should be merged instead of stacked.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Transform undo**: Modifying Position, Rotation, or Scale in the Transform section pushes a `SetTransformCommand` to the `CommandStack`, supporting undo/redo via Ctrl+Z/Ctrl+Shift+Z. |
| G-02 | **Hidden labels for simple editors**: Float, int, bool, and string editors use `##`-prefixed hidden labels so the internal label is not displayed. The caller (table column 0) remains the sole source of the property name label. |
| G-03 | **Float single-click editing**: Float editor uses the same composite widget pattern as Vec editors: a gray drag handle for drag-to-scrub + `ImGui::InputFloat` for single-click text entry. The drag handle is positioned on the RIGHT side. |
| G-04 | **Axis handle on right side**: All vec2, vec3, vec4, and quat editors flip the layout so the colored drag handle is on the RIGHT side of the InputFloat (instead of the current left-side placement). |
| G-05 | **Drag undo merging**: Consecutive commands targeting the same entity + same property are merged (update `new_value` on the existing command instead of pushing a new one), reducing the number of Ctrl+Z presses to revert a drag gesture. |
| G-06 | **Non-regression**: All existing tests pass. Zero new compiler warnings. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No engine changes** — only `src/editor/` files are modified. No changes to math types (Vec3, Quat, Transform, etc.). |
| NG-02 | **No entity name field changes** — the editable name field at the top stays as-is. |
| NG-03 | **No new dependencies** — only `<imgui.h>` and existing editor headers. |
| NG-04 | **No headless ImGui tests** — manual smoke test + screenshot verification only. |
| NG-05 | **No Play-mode read-only changes** (deferred). |
| NG-06 | **No multi-select editing** (deferred). |
| NG-07 | **No component section layout changes** — only label hiding and the float editor widget change. |
| NG-08 | **No changes to int editor behavior** — int editor remains `ImGui::DragInt` (no drag handle added), only the hidden label fix applies. |
| NG-09 | **No changes to Color editor** — the Color editor already uses `##color` hidden labels and requires no modification. |
| NG-10 | **No change to Quat angle display format** — rotation continues to use `"%.2f"` format and [-180, 180] wrapping. |
| NG-11 | **No change to the existing `draw_fallback_readonly()` function** — it remains dead but harmless. |
| NG-12 | **Superseded** — SetTransformCommand now covers Scale alongside Position and Rotation (all-in-one approach). This non-goal is no longer applicable. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, selects an entity in the Scene Panel, edits transform (Position, Rotation, Scale) and component properties in the Properties Panel. Expects Ctrl+Z to undo any edit, and expects single-click text entry on float fields. |
| **Command author** | Implements the `SetTransformCommand` class and the merge logic (peek_undo + try_update_new_value) for both `SetTransformCommand` and `SetComponentPropertyCommand`. |

## User-visible behavior

### Transform undo (Issue 1)

When the user edits Position, Rotation, or Scale in the Transform section, a `SetTransformCommand` is created and pushed to the `CommandStack`. The command stores **all three** transform properties natively as math types (no YAML):

- `entity_id`
- `old_position_`, `old_rotation_`, `old_scale_` — captured BEFORE the edit
- `new_position_`, `new_rotation_`, `new_scale_` — captured AFTER the edit

The command's `execute()` writes all three new values to `entity.transform()` and calls `ctx.editor.mark_dirty()`. The `undo()` restores all three old values and calls `ctx.editor.mark_dirty()`.

Because every `SetTransformCommand` captures all three properties, undo/redo always restores the full transform state before/after the edit — not just the single property that changed.

### Hidden labels for simple editors (Issue 2)

The float, int, bool, and std::string editors in `register_builtin_inspector_editors()` are updated to use `##`-prefixed labels so the internal ImGui label is never displayed:

| Editor | Old | New |
|---|---|---|
| float | `ImGui::DragFloat(label.c_str(), ...)` | `ImGui::DragFloat(("##val"), ...)` (the `label` parameter is solely for PushID scoping, not display) |
| int | `ImGui::DragInt(label.c_str(), ...)` | `ImGui::DragInt(("##val"), ...)` |
| bool | `ImGui::Checkbox(label.c_str(), &value)` | `ImGui::Checkbox(("##val"), &value)` |
| std::string | `ImGui::InputText(label.c_str(), buf, ...)` | `ImGui::InputText(("##val"), buf, ...)` |

The `label` parameter is still accepted for ImGui PushID scoping (the caller passes the property name, which is used by `InspectorTypeEditorRegistry::draw<T>()` to create a unique ID scope). The hidden `##` suffix ensures no text is displayed by the widget itself.

This matches the pattern already used by Vec2/Vec3/Vec4/Quat editors (which removed label rendering in F-06).

### Float editor composite widget (Issue 3)

The float editor is refactored from:

```
[ DragFloat(label) ]   ← old: single DragFloat with visible label
```

To:

```
[ InputFloat ][■]      ← new: InputFloat (text entry) + gray drag handle
```

The new float widget consists of:
- **Left side**: `ImGui::InputFloat` with format `"%.2f"` — single-click to enter text edit mode, Enter to confirm, Escape to cancel. Standard width (~60px).
- **Right side**: A gray drag handle (`ImVec4(0.5f, 0.5f, 0.5f, 1.0f)`) — same pattern as the colored axis handles in `draw_axis_widget()`, but without semantic color. Click+drag left/right scrubs the float value.

The drag handle technique matches `draw_axis_widget()` exactly:
1. A ~20px-wide gray rectangle is drawn via `ImDrawList`.
2. An `ImGui::InvisibleButton` is overlaid for hit-testing.
3. `ImGui::GetMouseDragDelta()` scrubs the value on each frame while active.
4. On deactivation, the initial value is cleaned up.

The drag speed (sensitivity) comes from `flags.step_value` if > 0, else defaults to 0.1.

**Layout**:
```
[InputFloat][■]
```

### Axis handles on right side (Issue 3b)

All vec2, vec3, vec4, and quat editors are updated to flip the axis handle from left to right:

```
Old:  [■ X][ input ] [■ Y][ input ] [■ Z][ input ]
New:  [ input ][■ X] [ input ][■ Y] [ input ][■ Z]
```

The `draw_axis_widget()` function is updated to render the InputFloat first, then the colored drag handle on the right. The function signature remains unchanged (same parameters, same return type).

### Drag undo merging (Issue 4)

Two new mechanisms are added to the Command system:

#### 1. `CommandStack::peek_undo()`

```cpp
/// Returns a pointer to the most recent command on the undo stack, or nullptr if empty.
[[nodiscard]] auto peek_undo() noexcept -> Command*;
```

#### 2. `Command::try_update_new_value()` (virtual)

```cpp
/// Attempt to update the new_value of this command (for drag merging).
/// Returns true if the command accepted the update (same target entity + same property).
/// Base implementation returns false.
[[nodiscard]] virtual auto try_update_new_value(YAML::Node new_value) noexcept -> bool;
```

#### SetComponentPropertyCommand changes

`SetComponentPropertyCommand` overrides `try_update_new_value()`:
- Checks if the incoming `new_value` matches the current `new_value_` (same YAML) → return false (nothing to update).
- If the incoming value is different but for the same entity/component/property (implied by the type identity), updates `new_value_` and returns true.
- Uses `yaml-cpp` equality comparison (`YAML::Node::operator==`).

#### SetTransformCommand changes

`SetTransformCommand` overrides `try_update_new_value()`:
- Stores `entity_id_` and all 3 old/new transform properties as math types (Vec3/Quat).
- Checks if the incoming command targets the same `entity_id_`.
- If so, updates ALL THREE `new_*` values from the current `entity.transform()` state and returns true.
- During a drag, only one property actually changes (e.g., Position X), but all three `new_*` values are refreshed. The unchanged properties self-copy — no data is lost because the old values remain intact.

#### Properties panel integration

In `draw_component_sections()`, before pushing a new `SetComponentPropertyCommand`:
```cpp
auto* last = ctx.editor.command_stack().peek_undo();
if (last && last->try_update_new_value(*new_yaml)) {
    // Successfully merged — no new command pushed
} else {
    // Push new command as before
    auto cmd = std::make_unique<SetComponentPropertyCommand>(...);
    ctx.editor.command_stack().execute(std::move(cmd), ctx);
}
```

Same pattern in `draw_transform_section()` when creating `SetTransformCommand` for any transform property edit (Position, Rotation, or Scale). All three properties are captured together regardless of which one changed.

## User stories

### Story 1 — Undo Transform Edit (Priority: P1)

As an editor user, I want to undo any transform edit (Position, Rotation, or Scale) using Ctrl+Z, so that I can revert accidental changes.

**Given** entity "Player" is selected with Position (0, 0, 0), Rotation (0, 0, 0), Scale (1, 1, 1)
**When** I change Position X from 0.00 to 5.00 (via InputFloat or drag handle)
**Then** the entity's Position X becomes 5.00
**And** the scene is marked dirty

**Given** Player's Position X has been changed to 5.00
**When** I press Ctrl+Z
**Then** the entity's full transform is restored: Position returns to (0, 0, 0), Rotation to (0, 0, 0), Scale to (1, 1, 1)
**And** the Properties Panel Transform section reflects the restored values
**And** the scene is marked dirty

**Given** Player's transform has been undone to the initial state
**When** I press Ctrl+Shift+Z (Redo)
**Then** the entity's Position X returns to 5.00
**And** the Properties Panel reflects the re-done value

### Story 2 — Undo Any Transform Property (Priority: P1)

As an editor user, I want to undo any single transform property change, knowing that undo restores ALL transform properties to the pre-edit state.

**Given** entity "Player" is selected with Position (10, 0, 0), Rotation (0, 0, 0), Scale (2, 2, 2)
**When** I change the Yaw (Y) rotation field from 0 to 90
**Then** the entity rotates 90 degrees around the Y axis
**And** pressing Ctrl+Z reverts Position to (10, 0, 0), Rotation to (0, 0, 0), Scale to (2, 2, 2)

**Given** entity "Player" is selected with Position (10, 0, 0), Rotation (0, 0, 0), Scale (2, 2, 2)
**When** I change Scale X from 2.00 to 3.00
**Then** the entity's Scale X becomes 3.00
**And** pressing Ctrl+Z reverts all three properties to their original values

### Story 4 — No Double Labels in Component Tables (Priority: P1)

As an editor user, I want each property to have a single label in column 0, not a duplicate label inside the editor widget.

**Given** an entity with a float property "Speed" (value 10.0) is selected
**When** I view the component's property table
**Then** the label "Speed" appears only once — in column 0 (the property name column)
**And** the editor widget in column 1 does NOT display any text label

**Given** an entity with a bool property "Enabled" (value true)
**When** I view the component's property table
**Then** the label "Enabled" appears only in column 0
**And** the Checkbox widget in column 1 shows only the checkbox (no text label next to it)

**Given** an entity with an int property "Count" (value 5)
**When** I view the component's property table
**Then** the label "Count" appears only in column 0
**And** the DragInt widget in column 1 does NOT display a text label

**Given** an entity with a string property "Name" (value "test")
**When** I view the component's property table
**Then** the label "Name" appears only in column 0
**And** the InputText widget in column 1 does NOT display a text label

### Story 5 — Float Single-Click Editing (Priority: P2)

As an editor user, I want to click once on a float field to type a precise value, without needing to double-click or click+drag.

**Given** the component property "Speed" = 10.00 is displayed for the selected entity
**When** I single-click the InputFloat field
**Then** the field enters text edit mode immediately (cursor appears)
**When** I type "20" and press Enter
**Then** the value changes to 20.00
**And** the scene is marked dirty

**Given** I have started editing a float value
**When** I press Escape before confirming
**Then** the value reverts to the previous value

**Given** a float property is displayed
**When** I click+drag the gray drag handle to the right
**Then** the value increases as I drag
**When** I release the mouse
**Then** the value retains the dragged value

### Story 6 — Axis Handle on Right Side (Priority: P2)

As an editor user, I want the colored axis drag handle to appear on the right side of the input field, consistent with the single-float editor layout.

**Given** entity "Player" is selected and the Transform section is visible
**When** I view the Position row's X component
**Then** the layout is `[InputFloat][■ X]` (input field on left, drag handle on right)

**Given** I view the Rotation row's Y component
**Then** the layout is `[InputFloat][■ Y]` (input field on left, colored drag handle on right)

**Given** I view a Vec4 property with a W component
**Then** the layout is `[InputFloat][■ W]` (input field on left, gray drag handle on right)

**Given** I view any vec2, vec3, vec4, or quat editor
**Then** all axis handles are positioned on the right side of their respective InputFloat

### Story 7 — Drag Undo Merging (Priority: P1)

As an editor user, I want a single drag gesture on a property to produce exactly one undo step, not one step per frame of drag.

**Given** entity "Player" is selected with Position X = 0.00, and unchanged Rotation/Scale
**When** I click+drag the X drag handle from 0.00 to 5.00 in one continuous gesture
**Then** after I release the mouse, pressing Ctrl+Z once reverts Position X directly to 0.00 (not to an intermediate value)
**And** Rotation and Scale are also reverted to their original values (all-in-one restore)

**Given** entity "Player" has a float component property "Speed" = 10.00
**When** I click+drag the gray handle from 10.00 to 25.00 in one continuous gesture
**Then** after release, pressing Ctrl+Z once reverts Speed directly to 10.00

**Given** I have dragged Position X from 0 to 5, then separately dragged Position Y from 0 to 10
**When** I press Ctrl+Z
**Then** both Position X and Y are reverted (Single command captured both drags if the second was a merge; but if a new command was pushed, Ctrl+Z reverts the Position Y command, which restores all of Position to the state before the Y edit)
**Then** pressing Ctrl+Z again reverts any remaining change

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | `SetTransformCommand` class exists in `src/editor/commands/set_transform_command.h` storing all 3 transform properties as native math types (`buddd::engine::math::Vec3` for position/scale, `buddd::engine::math::Quat` for rotation) — no YAML, no enum. | Code review: file exists, class defined |
| AC-02 | `SetTransformCommand` stores `entity_id`, `old_position_`, `old_rotation_`, `old_scale_`, `new_position_`, `new_rotation_`, `new_scale_` (no YAML::Node, no TransformProperty enum). | Code review |
| AC-03 | `SetTransformCommand::execute()` writes all three `new_*` values to `entity.transform()` and calls `ctx.editor.mark_dirty()`. | Code review + manual: edit any transform property, verify Ctrl+Z works |
| AC-04 | `SetTransformCommand::undo()` restores all three `old_*` values to `entity.transform()` and calls `ctx.editor.mark_dirty()`. | Code review + manual: Ctrl+Z after transform edit restores full transform |
| AC-05 | `SetTransformCommand::name()` returns a human-readable string (e.g., "Set Transform"). | Code review |
| AC-06 | `draw_transform_section()` creates and pushes a `SetTransformCommand` for any transform edit (Position, Rotation, or Scale) instead of direct mutation. | Code review |
| AC-07 | Scale edits in `draw_transform_section()` push a `SetTransformCommand` (same as Position/Rotation — all-in-one). | Code review |
| AC-08 | `SetTransformCommand` has NO `TransformProperty` enum and NO YAML serialization for transform data. | Code review |
| AC-09 | Float editor (`register_builtin_inspector_editors()`) uses composite widget: `ImGui::InputFloat` + gray drag handle (`ImVec4(0.5f, 0.5f, 0.5f, 1.0f)`) on the right. | Manual inspection |
| AC-10 | Float editor supports single-click text entry via `ImGui::InputFloat`. | Manual: click once, type, Enter confirms |
| AC-11 | Float editor supports drag-to-scrub via the gray drag handle on the right. | Manual: click+drag handle |
| AC-12 | Float editor preserves speed sensitivity from `EditorFlags.step_value` (default 0.1). | Code review |
| AC-13 | Float editor uses hidden label (`##` prefix) for the InputFloat widget. | Code review |
| AC-14 | Int editor uses hidden label (`##` prefix) for DragInt. | Code review |
| AC-15 | Bool editor uses hidden label (`##` prefix) for Checkbox. | Code review |
| AC-16 | String editor uses hidden label (`##` prefix) for InputText. | Code review |
| AC-17 | Component property table shows no duplicate labels: label in column 0 only, empty/clean widget in column 1. | Manual inspection: verify no text inside the widget in column 1 |
| AC-18 | `draw_axis_widget()` renders InputFloat on the left and colored drag handle on the right (updated layout). | Manual inspection + code review |
| AC-19 | Vec2, Vec3, Vec4, Quat editors all use the updated right-side axis handle layout. | Manual inspection of each type |
| AC-20 | `CommandStack::peek_undo()` method exists, returns `Command*` (nullptr if empty). | Code review |
| AC-21 | `Command::try_update_new_value(YAML::Node) -> bool` virtual method exists with default implementation returning `false`. | Code review |
| AC-22 | `SetComponentPropertyCommand` overrides `try_update_new_value()` to update its `new_value_` when the incoming value is different. | Code review |
| AC-23 | `SetTransformCommand` overrides `try_update_new_value()` using `EditorContext const&` to read the entity's current transform and updates all three `new_*` values (position, rotation, scale) when targeting the same entity. | Code review |
| AC-24 | `draw_component_sections()` checks `peek_undo()` and calls `try_update_new_value()` before pushing a new command. | Code review |
| AC-25 | A single continuous drag on a component property produces exactly one undo step (merged). | Manual: drag property, verify one Ctrl+Z reverts all the way |
| AC-26 | A single continuous drag on Position X produces exactly one undo step (merged). | Manual: drag Position X, verify one Ctrl+Z reverts |
| AC-27 | Editing two different properties sequentially (e.g., Pos X then Pos Y) produces two separate undo steps. | Manual: edit X, edit Y, Ctrl+Z reverts Y only |
| AC-28 | Undoing a merged drag command correctly restores the original pre-drag value. | Manual: drag value up then down to original, verify no change; drag up, Ctrl+Z reverts to pre-drag |
| AC-29 | All existing unit tests pass. | `ctest --preset debug` |
| AC-30 | Zero new compiler warnings from `src/editor/`. | Build output |

## E2E Verification

| Method | Description |
|---|---|
| **Manual smoke test** | Run `buddd edit` with a scene loaded. Select an entity. Verify: (1) Position edit → Ctrl+Z reverts it; (2) Rotation edit → Ctrl+Z reverts it; (3) Scale edit → Ctrl+Z reverts it; (4) Component property edit → Ctrl+Z reverts it; (5) Single click on float field enters text edit mode; (6) Gray drag handle appears on the right for float editors; (7) Vec axis handles appear on the RIGHT side of the InputFloat; (8) No duplicate labels in component property tables; (9) Dragging a property continuously produces one undo step. |
| **Screenshot capture** | Capture before/after screenshots of: (a) component properties table showing no duplicate labels; (b) float editor showing gray handle on right; (c) Vec3 Position layout showing right-side axis handles. |
| **Build verification** | `cmake --build --preset debug` with zero new warnings. `ctest --preset debug` with all tests passing. |

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | User can undo any transform edit (Position, Rotation, or Scale) via Ctrl+Z — all three transform properties restore to the exact pre-edit values. |
| SC-002 | User can single-click any float field (component property or transform) and type a precise value without double-clicking or dragging. |
| SC-003 | User can identify a float field's drag handle by the gray color on the right side of the input — no semantic confusion with axis-colored handles. |
| SC-004 | A single continuous drag gesture on any property produces exactly one undo step (not one per frame). |
| SC-005 | No duplicate text labels appear in component property tables — each property name is visible only in the left column. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| **Drag gesture that starts and ends at the same value** | The command's old and new values are identical. `CommandStack::execute()` should still push the command (the execute-time redundancy check in `SetComponentPropertyCommand` will no-op). However, since `try_update_new_value` prevented a new command from being pushed during the drag, and the final value equals the initial value, the merged command has old==new. The redundancy check in execute() sees current==new_value_ and returns early. Undo of such a command is a no-op. |
| **Rapid alternating edits between two properties** | Each property produces its own command; no merging across different properties. |
| **All-in-one undo groups all three transform properties** | Changing any of Position/Rotation/Scale pushes a SetTransformCommand that captures all three. Undo restores all three to the pre-edit state, even if only one property was changed. |
| **Editing the same property on two different entities** | No merging — entity ID must match. |
| **Very long drag with many intermediate values** | Only one command is pushed regardless of how many intermediate values were produced during the drag. |
| **Vec component clamp (Scale min_value = 0.001)** | Existing clamp behavior is preserved. The command captures the clamped value. |
| **Rotation values near gimbal lock** | Existing Quat→Euler→Quat round-trip behavior is unchanged. |
| **Editing a destroyed entity's transform** | `SetTransformCommand::execute()` should gracefully skip (entity ID is stale, no-op). |
| **Float editor with negative values** | `ImGui::InputFloat` supports negative values natively. Drag handle scrubbing also supports negative values. No change from existing behavior. |
| **Float editor with very large values (1e10)** | InputFloat displays and edits large floats without overflow. |
| **Float editor: empty string entry** | ImGui InputFloat rejects non-numeric input natively. On focus loss, reverts to last valid numeric value. |
| **Int editor: hidden label effect on clickable area** | Using `##val` as label does not affect clickable area — the widget is fully interactive. |
| **Bool editor: Checkbox without visible label** | The checkbox square is still clickable. No label text appears next to it. |
| **String editor: InputText without visible label** | The input field is still interactive. No label text appears above or beside it. |
| **Scene switch during a drag** | In-progress drag terminates at frame boundary. Selection clears. No stale command is pushed. |
| **Multi-select (primary entity edited)** | Only the primary entity's transform/components are edited. No command merging across entities. |

## Error cases

| Case | Expected behaviour |
|---|---|
| **SetTransformCommand::execute() on stale entity ID** | Entity lookup fails (returns invalid Entity). Command logs a warning and returns early without mutation. |
| **SetTransformCommand::undo() on stale entity ID** | Same graceful handling as execute(). |
| **SetComponentPropertyCommand::try_update_new_value() with YAML mismatch** | If the incoming YAML does not match the expected schema (different type), the merge is rejected (returns false) and a new command is pushed. This is an edge case — in practice, the YAML schema is always consistent for the same property. |
| **CommandStack::peek_undo() on empty stack** | Returns nullptr. The command-creation code handles this by always pushing when last is null. |
| **Command::try_update_new_value() on unsupported command type** | Base implementation returns false. A new command is pushed. |
| **Attempt to undo a Scale edit** | Scale edits push a SetTransformCommand (like Position/Rotation), so undo works normally — restores the full transform including Scale. |
| **ImGui frame skipped (panel collapsed)** | No edits occur. No commands pushed. |

## Permissions and security

- No changes to permissions or security posture.
- All commands respect the existing `EditorContext` access pattern.
- No new file I/O.
- No authentication or authorization boundaries are crossed.

## Observability

| Signal | Source |
|---|---|
| **SetTransformCommand execution** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "SetTransform: entity={} pos=({},{},{}) rot=({},{},{}) scale=({},{},{})", entity_id_.index, ...)` on execute and undo. |
| **Command merging** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "Merged SetTransformCommand for entity={}", ...)` when `try_update_new_value()` returns true. |
| **Peek/merge skipped** | Debug-level log: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "Peeked command does not support merging for entity={}", ...)` if peek_undo returns a command that does not accept the merge. |

## Out of scope

- Changes to entity name field or no-selection state.
- Play-mode read-only enforcement.
- Multi-select simultaneous editing.
- New Command types beyond `SetTransformCommand`.
- (Superseded) Changes to Scale editing behavior or undo support for Scale — now included via all-in-one SetTransformCommand.
- Changes to Color editor.
- Changes to the rename entity flow.
- Viewport gizmo integration.
- Drag-and-drop asset references into fields.
- Keyboard shortcuts for Properties Panel fields.
- Headless ImGui unit tests for the composite widget.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `CommandStack::peek_undo()` can be safely called from the main thread during `draw_ui()`. The CommandStack is not modified concurrently. |
| A-02 | `YAML::Node::operator==` provides deep equality comparison for the YAML values stored in commands. This is a yaml-cpp built-in. |
| A-03 | `dynamic_cast<SetComponentPropertyCommand*>(last)` or `dynamic_cast<SetTransformCommand*>(last)` is a reasonable way to check command type at runtime in the property panel code. The cast is safe because the commands are known types. For SetTransformCommand, entity_id_ matching is sufficient for merge (no per-property enum needed). |
| A-04 | The `SetTransformCommand` stores all three transform properties as native math types (`buddd::engine::math::Vec3` for position/scale, `buddd::engine::math::Quat` for rotation). No YAML serialization is used for transform data. |
| A-05 | Drag-to-scrub on the new float composite widget uses the same `InvisibleButton` + `GetMouseDragDelta()` technique as `draw_axis_widget()`. The sensitivity factor (`pixel_delta * drag_speed * 0.01f`) is the same as `draw_axis_widget()`. |
| A-06 | The gray color `ImVec4(0.5f, 0.5f, 0.5f, 1.0f)` provides sufficient contrast against the default ImGui background to be visible as a drag handle. |
| A-07 | The `##` hidden label pattern is compatible with all four ImGui widget types: `DragFloat`, `DragInt`, `Checkbox`, and `InputText`. |
| A-08 | `draw_axis_widget()` is a file-local helper in `inspector_editors.cpp`. Flipping the layout order (InputFloat first, then drag handle) does not require signature changes. |
| A-09 | No other code calls `draw_axis_widget()` outside of `inspector_editors.cpp`. The layout change applies universally. |
| A-10 | `SetTransformCommand` is created in `draw_transform_section()` of `properties_panel.cpp`. The `Entity::transform()` reference is valid at the time of capture. The command stores copies of the math values, so the transform reference does not need to outlive the capture. |
| A-11 | The `SetTransformCommand` does NOT need to store or restore the full entity selection state (unlike Create/Delete/Rename commands). Transform edits are purely data mutations. The all-in-one approach stores all three properties rather than a single enum + value. |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **Should `SetTransformCommand` support Scale as well?** F-05 explicitly deferred SetTransformCommand for Position and Rotation. Scale used direct mutation with no undo. | **RESOLVED — all-in-one approach includes Scale.** SetTransformCommand captures all three transform properties (Position, Rotation, Scale). Scale gets undo support alongside Position and Rotation. |
| Q-02 | **Should `try_update_new_value()` use `YAML::Node` or a typed parameter?** The `SetComponentPropertyCommand` uses YAML for value transport. `SetTransformCommand` uses the all-in-one approach with native math types (Vec3/Quat). | **Dual approach**: `SetComponentPropertyCommand::try_update_new_value()` takes `YAML::Node` (unchanged). `SetTransformCommand::try_update_new_value()` takes `EditorContext const&` and reads the entity's current transform directly — no YAML involved. |
