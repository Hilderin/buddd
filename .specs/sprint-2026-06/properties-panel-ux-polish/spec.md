# SPEC-F-06 — Properties Panel UX Polish

## Problem

The Properties Panel's Transform section (implemented in F-05) has several UX deficiencies compared to modern engine editors (Godot, Unity):

1. **Misaligned values**: Each property row (Position, Rotation, Scale) uses `ImGui::SameLine()` to place the property name next to multi-field inputs, but the labels and inputs have no consistent alignment. Label widths vary with text length, causing component fields to appear at different horizontal offsets per row.
2. **Double-click to edit**: The current `ImGui::DragFloat` widgets require a double-click to enter text-input mode. Users expect single-click text entry (as in Godot/Unity).
3. **No visual axis labeling**: X, Y, Z fields are plain unlabeled DragFloat widgets or have only tiny text prefixes (`"X: "`, `"Y: "`, `"Z: "`) with no colored axis indicator. Users cannot visually distinguish axes at a glance.
4. **Unstructured layout**: The flat inline layout does not communicate a tabular structure. Modern editors use a table with a property-name column and a value column.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Table layout**: Transform section renders Position/Rotation/Scale rows in a 2-column table (property name \| value) without column headers |
| G-02 | **Godot-style axis inputs**: Each component field (X/Y/Z) uses a composite widget: colored axis drag-handle on the left + `ImGui::InputFloat` on the right. Drag-handle supports click+drag to scrub value; InputFloat supports single-click text entry. |
| G-03 | **Axis-colored labels**: Dark red (#B31A1A) for X, darker green for Y (`ImVec4(0.0f, 0.55f, 0.0f, 1.0f)`), Blue (#4444FF) for Z. Small colored rectangle with white text label. |
| G-04 | **Rotation**: Display using X, Y, Z labels (matching Position and Scale convention). Tooltips on the drag handles explain the meaning: "Pitch (rotation around X axis)", "Yaw (rotation around Y axis)", "Roll (rotation around Z axis)". Values in degrees, wrap [-180, 180]. Uses the same axis-colored composite widget pattern (X=red, Y=green, Z=blue). |
| G-05 | **Vec2, Vec3, Vec4, Quat editors updated**: All multi-component editors use the new axis-colored composite widget with X/Y(/Z/W) labels. |
| G-06 | **Existing behavior preserved**: Entity name field, no-selection state, dirty marking, per-property undo (via `ctx.editor.mark_dirty()`) all remain unchanged. |
| G-07 | **Non-regression**: All existing tests pass. Zero new compiler warnings. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No engine changes** — only `src/editor/` files are modified. No changes to math types (Vec3, Quat, etc.). |
| NG-02 | **No entity name field changes** — the editable name field at the top stays as-is. |
| NG-03 | **No new dependencies** — only `<imgui.h>` and existing editor headers. |
| NG-04 | **No headless ImGui tests** — manual smoke test + screenshot verification only. |
| NG-05 | **No Play-mode read-only changes** (deferred to F-15). |
| NG-06 | **No multi-select editing** (deferred). |
| NG-07 | **No component property sections** (only Transform). |
| NG-08 | **No changes to Scale editing behavior** — Scale remains editable Vec3 (same UX as Position). |
| NG-09 | **The `draw_fallback_readonly()` function is not modified** — it remains dead but harmless. |
| NG-10 | **No per-component color customization** — axis colors are hardcoded to the RGB convention. |
| NG-11 | **No change to RenameEntityCommand** — entity name editing remains identical to F-05. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, selects an entity in the Scene Panel, and edits the entity's position, rotation, and scale in the Properties Panel's Transform section. Interacts with the new composite axis widgets via click-drag on colored handles and single-click text entry. |

## User-visible behavior

### Table Layout (Transform section)

Inside the Transform `CollapsingHeader`, a 2-column `ImGui::Table` is used with no column headers:
- **Column 0**: Property name (e.g., "Position", "Rotation", "Scale") — left-aligned text. **Rendered by `draw_transform_section()`** (the PropertiesPanel), not by the editor registry.
- **Column 1**: Value area — contains the multi-component widgets inline. **Rendered by calling `InspectorTypeEditorRegistry::draw<T>()`** — the `id` parameter is used only for ImGui PushID scoping (not displayed). The editor registry draws only the composite axis widgets (the value cell content).

The table spans the full content region width. Each row (Position, Rotation, Scale) occupies one table row. The section is responsible for:
1. Creating the `ImGui::Table` (2 columns, no headers)
2. Rendering "Position" text in column 0
3. Calling `InspectorTypeEditorRegistry::draw<Vec3>(...)` in column 1
4. Same for Rotation and Scale rows

Column widths:
- Column 0: width enough for the property name text (~60px fixed or `ImGui::CalcTextSize` width, determined by implementation)
- Column 1: remaining width

### Composite Axis Input Widget (new helper)

Each individual component (X, Y, Z) is rendered by a new helper function with the following signature:

```
auto draw_axis_widget(const char* id, float* value, ImVec4 color,
                      float drag_speed, const EditorContext& ctx,
                      const char* tooltip = nullptr) -> bool;
```

- `@param id` — ImGui ID for PushID scoping.
- `@param value` — Pointer to the float value being edited.
- `@param color` — Axis color for the drag-handle rectangle.
- `@param drag_speed` — Sensitivity for pixel-to-value conversion during drag.
- `@param ctx` — Editor context (reserved for future use; `mark_dirty()` is called by the parent editor, not by this function).
- `@param tooltip` — Optional tooltip text shown on hover over the drag handle (e.g., "Pitch (rotation around X axis)").
- `@return` — `true` if the value changed this frame.

The widget has two parts side-by-side:

```
[■ X] [123.45]
```

Where:
- **Left side**: A small colored rectangle (axis color) approximately 20px wide with white text label (axis letter/name). The rectangle height matches `ImGui::GetFrameHeight()` to align with the adjacent InputFloat. This rectangle acts as a drag handle — left-click + drag left/right scrubs the value. The drag speed (sensitivity) is comparable to the existing DragFloat speeds (0.1 for position/scale, 0.5 for rotation). An optional tooltip (e.g., "Pitch (rotation around X axis)") can be shown on hover over the drag handle.
- **Right side**: An `ImGui::InputFloat` with format `"%.2f"` — single-click to enter text edit mode, Enter to confirm, Escape to cancel. The input field has a reasonable default width (e.g., ~60px).

Both parts share the same underlying float value. Changing the value via either mechanism updates the same variable.

**Label ownership**: The composite axis input widget does NOT render its own property-name label (e.g., "Position", "Rotation"). Labels are exclusively the caller's responsibility (e.g., `draw_transform_section()` renders the label in column 0 of the table). The widget only renders the component inputs themselves. Calls like `InspectorTypeEditorRegistry::draw<Vec3>("Position", ...)` pass the `id` string to the editor, but the Vec3 editor uses it only for ImGui PushID scoping (not for display) and renders only the composite axis widgets.

**Drag-handle technique**: The drag-handle uses **`ImGui::InvisibleButton` + manual drag**:
1. A small ~20px-wide colored rectangle is drawn using `ImDrawList` (via `ImGui::GetWindowDrawList()`) in the axis color. The rectangle height matches `ImGui::GetFrameHeight()` to align with the adjacent InputFloat.
2. An `ImGui::InvisibleButton` of the same size is overlaid on top for hit-testing.
3. On each frame while the button is active (held), `ImGui::GetMouseDragDelta()` provides the delta to scrub the float value.
4. If a tooltip string is provided, it is rendered via `ImGui::SetTooltip()` when the drag-handle is hovered (`ImGui::IsItemHovered()`).
5. On mouse release, the final change is applied and `ctx.editor.mark_dirty()` is called.

### Axis Colors

| Axis | Color | ImGui Color (`ImVec4`) |
|---|---|---|
| X / Pitch | Dark Red | `(0.7f, 0.1f, 0.1f, 1.0f)` |
| Y / Yaw | Green | `(0.0f, 0.55f, 0.0f, 1.0f)` |
| Z / Roll | Blue | `(0.27f, 0.27f, 1.0f, 1.0f)` |

For Vec4 (XYZW): W uses a neutral gray:
| W | Gray | `(0.7f, 0.7f, 0.7f, 1.0f)` |

### Layout example

```
┌──────────────────────────────────────────────┐
│ Properties                                    │
├──────────────────────────────────────────────┤
│ [Name field]                            [text]│
├──────────────────────────────────────────────┤
│ ▼ Transform                                   │
│                                                │
│   Position  [■ X][  0.00][■ Y][  0.00][■ Z][  0.00] │
│   Rotation  [■ X][  0.00][■ Y][  0.00][■ Z][  0.00] │
│   Scale     [■ X][  1.00][■ Y][  1.00][■ Z][  1.00] │
└──────────────────────────────────────────────┘
```

> Note: The `[■]` symbol above represents the colored rectangle in the spec; the actual rendering uses an `ImDrawList` rectangle with text overlay. The labels "Position", "Rotation", "Scale" (column 0 of the table) are rendered by `draw_transform_section()`, not by the editor registry.

### Dirty Marking

All editors continue to call `ctx.editor.mark_dirty()` when a value changes. The per-property undo granularity is preserved — any component change to Position marks dirty (and similarly for Rotation, Scale).

### Existing State Handling

- **No-selection state** (centered "No entity selected") — UNCHANGED
- **Entity name field** — UNCHANGED
- **Stale entity detection** — UNCHANGED
- **Multi-select behavior** (shows primary entity only) — UNCHANGED

### Rotation (Quat editor)

The Quat editor continues to:
1. Convert `Quat` to Euler radians via `Quat::to_euler()`.
2. Convert radians to degrees for display.
3. Wrap each angle to [-180, 180] degrees.
4. Display three composite axis widgets labelled "X", "Y", "Z" with axis colors (X=red, Y=green, Z=blue).
5. Tooltips on the drag handles explain the underlying Euler angle meaning: "Pitch (rotation around X axis)", "Yaw (rotation around Y axis)", "Roll (rotation around Z axis)".
6. On edit: wrap the new value, convert back to radians, construct quaternion via `Quat::from_euler()`.
7. Speed: preserve existing 0.5 drag speed.
8. **No label rendering**: The Quat editor does NOT render a property-name label — only the composite axis widgets.

### Vec2, Vec3, Vec4 editors

The built-in editors for `Vec2`, `Vec3`, `Vec4` are updated to use the same composite axis widget pattern:
- Vec2: 2 composite widgets (X=red, Y=green)
- Vec3: 3 composite widgets (X=red, Y=green, Z=blue)
- Vec4: 4 composite widgets (X=red, Y=green, Z=blue, W=gray)

**Label removal**: Visual label rendering is **removed** from Vec2/Vec3/Vec4/Quat editor draw methods. The editor draw function — called via `InspectorTypeEditorRegistry::draw<T>()` — no longer calls `ImGui::TextUnformatted()`. It renders only the composite axis widgets (the value cell content). The property-name label is the exclusive responsibility of the caller (e.g., `draw_transform_section()` renders "Position"/"Rotation"/"Scale" in column 0 of the table). The `id` parameter passed to `draw<T>()` is still accepted but is now used only for ImGui PushID scoping (not rendered visually).

## User stories

### Story 1 — Aligned Transform Section with Table Layout (Priority: P1)

As an editor user, I want the Position, Rotation, and Scale rows to be neatly aligned so that I can visually scan the values without eye movement between rows.

**Given** an entity is selected and the Properties Panel is visible
**When** I view the Transform section
**Then** Position, Rotation, and Scale are rendered as rows in a 2-column table
**And** all X components are vertically aligned, all Y components are aligned, and all Z components are aligned

**Given** the Transform section uses a table
**When** I resize the Properties Panel
**Then** the value column expands to fill the available width

### Story 2 — Single-Click Text Entry (Priority: P1)

As an editor user, I want to click once on a numeric field to type a precise value, without having to double-click first.

**Given** the entity "Player" is selected with Position X = 5.00
**When** I single-click the X InputFloat field
**Then** the field enters text edit mode immediately (cursor appears)
**When** I type "10" and press Enter
**Then** the value changes to 10.00
**And** the scene is marked dirty

**Given** I have started editing a value
**When** I press Escape before confirming
**Then** the value reverts to the previous value

### Story 3 — Axis-Colored Drag Handles (Priority: P2)

As an editor user, I want colored X/Y/Z indicators on each component field so that I can visually identify which axis I am editing.

**Given** the entity "Player" is selected
**When** I view the Transform section's Position row
**Then** the X component has a red drag-handle labelled "X"
**And** the Y component has a green drag-handle labelled "Y"
**And** the Z component has a blue drag-handle labelled "Z"

**Given** I view the Rotation row
**Then** the X component has a red drag-handle labelled "X"
**And** the Y component has a green drag-handle labelled "Y"
**And** the Z component has a blue drag-handle labelled "Z"
**And** hovering over the X drag-handle shows tooltip "Pitch (rotation around X axis)"
**And** hovering over the Y drag-handle shows tooltip "Yaw (rotation around Y axis)"
**And** hovering over the Z drag-handle shows tooltip "Roll (rotation around Z axis)"

**Given** I view the Scale row
**Then** the component colors match the Position row pattern

**Given** an entity with a Vec4 property is visible
**Then** the X, Y, Z handles use the standard axis colors
**And** the W handle uses a neutral gray

### Story 4 — Drag-to-Scrub on Colored Handle (Priority: P2)

As an editor user, I want to click and drag on the colored axis handle to scrub the value, just like DragFloat but with a visible target area.

**Given** the entity "Player" is selected with Position X = 0.00
**When** I left-click on the red "X" drag-handle and drag to the right
**Then** Position X increases as I drag
**When** I release the mouse
**Then** Position X retains the dragged value
**And** the scene is marked dirty

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | Transform section renders Position, Rotation, Scale rows in a 2-column `ImGui::Table` with no column headers | Manual inspection of Properties Panel |
| AC-02 | Each component field (X/Y/Z, Pitch/Yaw/Roll) shows a colored drag-handle on the left side | Visual inspection |
| AC-03 | Dragging the colored handle left/right changes the component value | Manual interaction test |
| AC-04 | Single-clicking the value field (InputFloat) enters text edit mode immediately (no double-click required) | Manual interaction test |
| AC-05 | X drag-handle is dark red (#B31A1A), Y is green (darker green, `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)`), Z is blue (#4444FF) | Visual inspection |
| AC-06 | Pitch drag-handle is dark red (#B31A1A), Yaw is green (darker green, `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)`), Roll is blue (#4444FF) | Visual inspection |
| AC-07 | Vec2 and Vec4 editors also use the new composite axis widget pattern with colored drag-handles | Manual inspection |
| AC-08 | Vec4 W component uses neutral gray drag-handle | Visual inspection |
| AC-09 | Entity name field and no-selection state are unchanged from F-05 | Manual inspection |
| AC-10 | Changing any component marks the scene dirty (star on window title) | Manual inspection |
| AC-11 | Rotation values are displayed in degrees and wrapped to [-180, 180] | Manual inspection |
| AC-12 | All existing unit tests pass | `ctest --preset debug` |
| AC-13 | Zero new compiler warnings | Build output |
| AC-14 | The editor label (e.g., "Position") is NOT rendered inside the value cell (column 1) — it appears only in column 0 of the table | Manual inspection: verify no duplicate label text inside the value area |

## E2E Verification

| Method | Description |
|---|---|
| **Manual smoke test** | Run `buddd edit` with a scene loaded. Select an entity. Verify: (1) Transform section uses 2-column table layout with no headers; (2) Position/Rotation/Scale each show axis-colored drag-handles; (3) single-click on value field enters text mode; (4) dragging colored handle scrubs the value; (5) Rotation displays in degrees with X/Y/Z labels and tooltips show "Pitch (rotation around X axis)" etc.; (6) editing any component marks scene dirty. |
| **Screenshot capture** | Capture a screenshot of the Properties Panel with an entity selected. Submit with implementation PR for visual comparison. |
| **Build verification** | `cmake --build --preset debug` with zero new warnings. `ctest --preset debug` with all tests passing. |

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | User can edit a Position/Rotation/Scale component via single-click text entry (Enter to confirm) within one attempt |
| SC-002 | User can identify field axis (X/Y/Z) by color alone without reading the label |
| SC-003 | All component fields are vertically aligned across the Position, Rotation, and Scale rows |

## Edge cases

| Case | Expected behaviour |
|---|---|
| **Rapid clicking on drag handle** | The widget should not lose/gain focus incorrectly. ImGui handles active ID state correctly. |
| **Extreme values (e.g., 1e10)** | InputFloat does not clamp; values can be any float range. Display shows "%.2f" format. |
| **Vec4 W component** | Uses neutral gray color `(0.7, 0.7, 0.7)` for the drag handle, with label "W". |
| **Empty string entry on InputFloat** | ImGui handles validation natively (shows last valid value on focus loss). |
| **Gimbal lock (pitch ≈ ±90°)** | Existing Quat editor behavior is unchanged: `glm::eulerAngles` produces a valid result; display values are deterministic. |
| **Very long entity name (>256 chars)** | No change from F-05 — `ImGui::InputText` truncates at buffer size. |
| **Scene switch while editing** | No change from F-05 — selection clears, in-progress drag terminates at frame boundary. |
| **Multiple transforms visible** (future multi-select) | Each selected entity's panel is separate; no shared state issues. |

## Error cases

| Case | Expected behaviour |
|---|---|
| **ImGui::Table creation failure** | If `ImGui::BeginTable` returns false, the editor degrades gracefully (renders rows without table layout, using the old `SameLine` approach). This is an unlikely edge case with modern ImGui. |
| **Value too large for display** | `ImGui::InputFloat` handles formatting with "%.2f" format string. Very large values display as scientific notation if needed, or show "%.2f" truncated. |
| **Invalid float entry (non-numeric text)** | ImGui InputFloat rejects non-numeric input natively. The value does not change. |
| **Entity destroyed during drag** | Not possible in practice — entity destruction is deferred to `flush_destroyed()` between frames. Stale entity detection is unchanged from F-05. |

## Permissions and security

- No changes to permissions or security posture.
- The Properties Panel reads and writes entity data from the in-memory World. No new file I/O.
- Transform edits are not persisted until the user explicitly saves the scene.
- No authentication or authorization boundaries are crossed.

## Observability

| Signal | Source |
|---|---|
| **New composite widget usage** | No new log signals required — the existing `ctx.editor.mark_dirty()` logging and Command execution logging from F-05 cover value-change visibility. |
| **Table layout rendering** | No logging needed — the table rendering is a structural change, not a runtime behaviour change. |
| **Debug build** | Compile with `CMAKE_BUILD_TYPE=Debug` to verify zero warnings. |

## Documentation impact

The following wiki pages must be updated when this spec is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Update the Inspector Panel section to describe the new table layout and composite axis widget UX in the Transform section. Update the Property Editors table (Vec2/Vec3/Vec4/Quat rows) to reflect axis-colored composite widgets. |

## Out of scope

- Changes to entity name field or no-selection state
- Play-mode read-only enforcement
- Multi-select simultaneous editing
- Component property sections (MeshRenderer, Light, etc.)
- New Command types or undo granularity changes
- Engine-side changes (math types, world API)
- Drag-and-drop asset references into fields
- Search/filter in the Properties Panel
- Reordering of transform/component sections
- Color picker widgets
- Keyboard shortcuts for Properties Panel fields
- Headless ImGui unit tests for the composite widget

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | ImGui Tables API (`ImGui::BeginTable`, `TableSetupColumn`, `TableNextRow`, `TableSetColumnIndex`) is available in the used ImGui version (docking branch). |
| A-02 | The composite widget approach (colored drag handle + InputFloat) provides the best Godot-like UX for axis inputs. The drag-handle uses the `ImGui::InvisibleButton` + `ImGui::GetMouseDragDelta()` approach (not DragBehavior). No external libraries are needed. |
| A-03 | Existing `mark_dirty()` mechanism is sufficient for undo support — per-property granularity is preserved. |
| A-04 | Vec2, Vec3, Vec4, Quat editors are all in `inspector_editors.cpp` and can be updated together. |
| A-05 | No engine rebuild needed — only editor library recompilation (changes are limited to `src/editor/`). |
| A-06 | The `draw_fallback_readonly()` function is not called by any updated path (all built-in types remain registered). It remains unchanged and dead. |
| A-07 | The Quat editor no longer renders any property-name label — it only renders the composite axis widgets with X/Y/Z labels on the drag-handles. Tooltips on the drag-handles provide the full angle name ("Pitch (rotation around X axis)", etc.). The editor no longer uses a formatted string like `"Pitch: %.1f"` for DragFloat; the axis label appears on the colored drag-handle. |
| A-08 | The existing drag speed values (0.1 for position/scale, 0.5 for rotation) are preserved as the sensitivity for the drag-handle scrubbing behavior. |
| A-09 | Rotation continues to use the `Quat::to_euler()` / `Quat::from_euler()` round-trip from F-05. Display format changes to `"%.2f"` on InputFloat (axis labels "X", "Y", "Z" on the drag-handles; tooltips convey "Pitch", "Yaw", "Roll" meaning). The "Rotation" property-name label is rendered by `draw_transform_section()` in column 0, not by the Quat editor. |

## Open questions

*No unresolved open questions.*
