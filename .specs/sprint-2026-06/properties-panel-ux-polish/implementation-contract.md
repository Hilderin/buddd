# IMPL-F-06 — Properties Panel UX Polish

## Source spec

`.specs/sprint-2026-06/properties-panel-ux-polish/spec.md`

## Goal

Upgrade the Transform section of the Properties Panel from flat DragFloat-based rows to a 2-column table layout (property name | composite axis widgets). Replace Vec2/Vec3/Vec4/Quat DragFloat editors with a new composite axis input widget: a colored drag-handle (InvisibleButton + ImDrawList) on the left and an `ImGui::InputFloat` (single-click text entry) on the right. Axis colors: X/Pitch=red, Y/Yaw=green, Z/Roll=blue, W=gray. Quat editor uses "X"/"Y"/"Z" labels on drag-handles with descriptive tooltips ("Pitch", "Yaw", "Roll"). Property-name labels are removed from Vec2/Vec3/Vec4/Quat editors (labels are the caller's responsibility in the table column 0). Preserve all existing behavior: dirty marking, entity name field, no-selection state, per-property undo granularity.

## Non-goals

- No changes to `InspectorTypeEditor`, `TypedInspectorEditor<T>`, `InspectorTypeEditorRegistry`, or `EditorFlags` — the registry API is unchanged.
- No changes to `inspector_editors.h` — all modifications are in `.cpp` file.
- No changes to `properties_panel.h` — the header remains unchanged.
- No changes to `EditorSelection`, `EditorContext`, `Editor`, `CommandStack`, or any engine files.
- No changes to entity name field behavior (`draw_entity_name()`).
- No changes to no-selection state (`draw_no_selection_state()`).
- No changes to `draw_fallback_readonly()` — it remains dead and untouched.
- No headless ImGui unit tests for the composite widget (manual smoke test + screenshot only).
- No new dependencies — only `<imgui.h>` and existing editor headers.
- No color customization or per-component color overrides.
- No Command-system changes — undo granularity remains per-property via `mark_dirty()`.

## Relevant ADRs

- **ADR-029**: Editor UX Decisions — confirms the fixed panel layout per tab type. No constraints on axis colors, table layout, or input widget styling.
- **ADR-026**: ImGui docking branch v1.91.8-docking, which provides `ImGui::BeginTable`/`EndTable`, `ImGui::InvisibleButton`, `ImGui::GetMouseDragDelta()`, and `ImGui::InputFloat`.

## Files to inspect

| File | Purpose |
|---|---|
| `src/editor/inspector_editors.h` | Read existing `InspectorTypeEditorRegistry` API, `draw_fallback_readonly()` signature, `EditorFlags` struct; confirm no changes needed |
| `src/editor/inspector_editors.cpp` | Read current Vec2/Vec3/Vec4/Quat editors to be rewritten |
| `src/editor/panels/properties_panel.h` | Read existing `PropertiesPanel` declaration; confirm no changes needed |
| `src/editor/panels/properties_panel.cpp` | Read current `draw_transform_section()` to be rewritten |
| `.specs/sprint-2026-06/inspector-transform/spec.md` | Read F-05 spec for Scale min-value constraint (0.001), confirm it was spec'd but not implemented |
| `.specs/sprint-2026-06/inspector-transform/implementation-contract.md` | Read F-05 contract for context on what was intended vs implemented |

## Files allowed to change

| File | Change description |
|---|---|
| `src/editor/inspector_editors.cpp` | Add file-local `draw_axis_widget()` helper function. Rewrite Vec2, Vec3, Vec4, Quat editor lambdas to use the composite axis widget instead of DragFloat. Remove `ImGui::TextUnformatted(label.c_str())` from these editors. Update Quat editor display format from `"%.1f"` to `"%.2f"`. |
| `src/editor/panels/properties_panel.cpp` | Rewrite `draw_transform_section()` to use a 2-column `ImGui::Table` (no headers). Column 0 = property name text ("Position"/"Rotation"/"Scale"), Column 1 = editor widget via `InspectorTypeEditorRegistry::draw<T>()`. Scale row passes `EditorFlags{min_value=0.001f}`. |

## Files forbidden to change

- `src/editor/inspector_editors.h` — no header changes; API unchanged
- `src/editor/panels/properties_panel.h` — no structural changes needed
- Any file in `src/engine/` — no engine changes (NG-01)
- Any file in `src/editor/commands/` — no command changes
- Any `CMakeLists.txt` — editor uses `GLOB_RECURSE`
- Any `.yaml`, `.json`, or configuration files
- Any test files — no new unit tests required for this feature

## Existing conventions to follow

- **Namespace**: `buddd::editor` for editor code.
- **C++ style**: Trailing return types (`auto foo() -> Bar`), `[[nodiscard]]` on query methods.
- **Logging**: `BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector", "...")` for debug logging.
- **ImGui patterns**: `PushID`/`PopID` for ID scoping; `ImGui::GetWindowDrawList()` for custom drawing.
- **No `using namespace ImGui`** — use explicit `ImGui::` prefix throughout.
- **`draw_fallback_readonly()`** is a free function (not a member), declared in `inspector_editors.h`. It must NOT be modified, moved, or removed.
- **Dirty marking**: Each editor calls `ctx.editor.mark_dirty()` internally when a value changes. The PropertiesPanel never wraps draws with `if(draw()){ mark_dirty(); }`.
- **EditorFlags defaults**: `min_value = -std::numeric_limits<float>::max()`, `max_value = std::numeric_limits<float>::max()`, `step_value = 0.0f`.
- **Test patterns**: Not applicable (no new test files).

## Required implementation behavior

### A. New file-local helper: `draw_axis_widget()` (in `inspector_editors.cpp`)

Add a file-local (anonymous namespace or `static`) helper function:

```cpp
namespace {

/// Draw a composite axis input widget.
///
/// ┌──────────┬──────────┐
/// │ [■ LABEL]│ [ 0.00 ] │
/// └──────────┴──────────┘
///
/// Left side: a colored rectangle (~20px wide) drawn via ImDrawList with white text label.
/// An ImGui::InvisibleButton of the same size is overlaid for hit testing.
/// Click+drag left/right on the handle scrubs the float value.
///
/// Right side: an ImGui::InputFloat for single-click text entry, format "%.2f".
///
/// @param id         Short identifier and display text for the drag handle (e.g., "X", "Y", "Pitch"). Used for both PushID scoping and as the label text on the colored rectangle.
/// @param value      Pointer to the float value being edited.
/// @param color      Axis color as ImVec4 (e.g., red for X, green for Y, blue for Z).
/// @param drag_speed Sensitivity for drag-to-scrub (0.1 for position/scale, 0.5 for rotation).
/// @param ctx        EditorContext (reserved for future use).
/// @param tooltip    Optional tooltip text shown on hover over the drag handle (e.g., "Pitch"). Defaults to nullptr (no tooltip).
/// @return true if the value changed this frame.
auto draw_axis_widget(const char* id, float* value, ImVec4 color,
                      float drag_speed, const EditorContext& ctx,
                      const char* tooltip = nullptr) -> bool;
```

**Implementation details**:

1. **Store initial value**: Use a `static std::unordered_map<const void*, float> initial_values` keyed by `value` pointer to store the value when drag starts.
2. **Draw colored rectangle**: Before the InvisibleButton, use `ImGui::GetWindowDrawList()->AddRectFilled()` with the axis color. Rectangle dimensions: `ImGui::GetCursorScreenPos()` as top-left, width = 20.0f, height = `ImGui::GetFrameHeight()` (matched to the InputFloat height).
3. **Overlay text**: After the rectangle, use `ImDrawList::AddText()` with white color (`IM_COL32(255,255,255,255)`) centered in the rectangle. Use `ImGui::CalcTextSize(id)` to center the text.
4. **InvisibleButton**: `ImGui::InvisibleButton("##handle", ImVec2(20.0f, ImGui::GetFrameHeight()))` over the same area.
5. **Tooltip**: After the InvisibleButton, if `tooltip != nullptr` and `ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)`, call `ImGui::SetTooltip("%s", tooltip)` to show the descriptive tooltip (e.g., "Pitch (rotation around X axis)").
6. **Drag handling**: While `ImGui::IsItemActive()`:
   - If this is the first frame of drag (active just became true), store `initial_values[value_ptr] = *value`.
   - Each frame, compute: `float pixel_delta = ImGui::GetMouseDragDelta().x;`
   - Apply: `*value = initial_value + pixel_delta * drag_speed * 0.01f;`
7. **On release**: When `ImGui::IsItemDeactivated()`, remove the entry from `initial_values`.
8. **InputFloat**: `ImGui::SameLine()`, then `ImGui::SetNextItemWidth(60.0f)`, then `ImGui::InputFloat("##input", value, 0.0f, 0.0f, "%.2f")`. The InputFloat uses `ImGuiInputTextFlags_EnterReturnsTrue` which is the default for `InputFloat` (single-click enters edit mode, Enter confirms).
9. **ID scoping**: Wrap the entire widget in `ImGui::PushID(id)` / `ImGui::PopID()` to prevent ID collisions between multiple axis widgets.
10. **Return value**: Return `true` if either the drag-handle or the InputFloat caused a value change this frame.

**Pixel-to-value conversion factor**: `0.01f` multiplier provides drag sensitivity comparable to ImGui::DragFloat with the same speed parameter. For `drag_speed=0.1f` (position/scale): 10 pixels of drag → `10 * 0.1 * 0.01 = 0.01` units change. For `drag_speed=0.5f` (rotation): 10 pixels → `10 * 0.5 * 0.01 = 0.05` degrees change.

### B. Rewrite Vec2 editor (in `inspector_editors.cpp`)

Replace the current Vec2 editor lambda with:

```cpp
InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Vec2>(
    [](const std::string& id, buddd::engine::math::Vec2& value,
       const EditorFlags& flags,
       const EditorContext& ctx) -> bool {
        float vals[2] = {value.x, value.y};
        ImGui::PushID(id.c_str());  // Keep for ID scoping
        bool changed = false;
        float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;

        changed |= draw_axis_widget("X", &vals[0], ImVec4(0.7f, 0.1f, 0.1f, 1.0f), speed, ctx);
        ImGui::SameLine();
        changed |= draw_axis_widget("Y", &vals[1], ImVec4(0.0f, 0.55f, 0.0f, 1.0f), speed, ctx);

        ImGui::PopID();
        if (changed) {
            vals[0] = std::clamp(vals[0], flags.min_value, flags.max_value);
            vals[1] = std::clamp(vals[1], flags.min_value, flags.max_value);
            value.x = vals[0];
            value.y = vals[1];
            ctx.editor.mark_dirty();
        }
        return changed;
    }
);
```

**Dirty marking convention**: The `draw_axis_widget()` function does NOT call `mark_dirty()`. The parent Vec2/Vec3/Vec4/Quat editor lambdas are responsible for calling `ctx.editor.mark_dirty()` once when `changed` is true (after copying all component values back). This ensures per-property undo granularity (one dirty mark per Vec2/Vec3/Vec4/Quat change, not per-component).

The flow: `draw_axis_widget()` modifies `*value` directly during drag and returns `true` when value changes (by drag OR InputFloat). The parent editor checks the OR of all axis widget returns. If any changed, copies values back from the temp array and calls `mark_dirty()` once.

Key changes from F-05:
- **REMOVED**: `ImGui::TextUnformatted(label.c_str())` and `ImGui::SameLine()` — labels are caller's responsibility.
- **REMOVED**: `ImGui::PushItemWidth()` / `PopItemWidth()` — InputFloat widths are managed by the widget.
- **REMOVED**: `ImGui::DragFloat` for X/Y — replaced by `draw_axis_widget()` with axis colors.
- **KEPT**: `ImGui::PushID(id.c_str())` / `PopID()` — still needed for ID scoping.
- **KEPT**: `mark_dirty()` called once after all components copied back (per-property granularity).

### C. Rewrite Vec3 editor (in `inspector_editors.cpp`)

Same pattern as Vec2 but with 3 components (X, Y, Z):

```cpp
InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Vec3>(
    [](const std::string& id, buddd::engine::math::Vec3& value,
       const EditorFlags& flags,
       const EditorContext& ctx) -> bool {
        float vals[3] = {value.x, value.y, value.z};
        ImGui::PushID(id.c_str());
        bool changed = false;
        float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;

        changed |= draw_axis_widget("X", &vals[0], ImVec4(0.7f, 0.1f, 0.1f, 1.0f), speed, ctx);
        ImGui::SameLine();
        changed |= draw_axis_widget("Y", &vals[1], ImVec4(0.0f, 0.55f, 0.0f, 1.0f), speed, ctx);
        ImGui::SameLine();
        changed |= draw_axis_widget("Z", &vals[2], ImVec4(0.27f, 0.27f, 1.0f, 1.0f), speed, ctx);

        ImGui::PopID();
        if (changed) {
            value.x = vals[0];
            value.y = vals[1];
            value.z = vals[2];
            ctx.editor.mark_dirty();
        }
        return changed;
    }
);
```

### D. Rewrite Vec4 editor (in `inspector_editors.cpp`)

Same pattern with 4 components: X=red, Y=green, Z=blue, W=gray.

```cpp
InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Vec4>(
    [](const std::string& id, buddd::engine::math::Vec4& value,
       const EditorFlags& flags,
       const EditorContext& ctx) -> bool {
        float vals[4] = {value.x, value.y, value.z, value.w};
        ImGui::PushID(id.c_str());
        bool changed = false;
        float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;

        changed |= draw_axis_widget("X", &vals[0], ImVec4(0.7f, 0.1f, 0.1f, 1.0f), speed, ctx);
        ImGui::SameLine();
        changed |= draw_axis_widget("Y", &vals[1], ImVec4(0.0f, 0.55f, 0.0f, 1.0f), speed, ctx);
        ImGui::SameLine();
        changed |= draw_axis_widget("Z", &vals[2], ImVec4(0.27f, 0.27f, 1.0f, 1.0f), speed, ctx);
        ImGui::SameLine();
        changed |= draw_axis_widget("W", &vals[3], ImVec4(0.7f, 0.7f, 0.7f, 1.0f), speed, ctx);

        ImGui::PopID();
        if (changed) {
            vals[0] = std::clamp(vals[0], flags.min_value, flags.max_value);
            vals[1] = std::clamp(vals[1], flags.min_value, flags.max_value);
            vals[2] = std::clamp(vals[2], flags.min_value, flags.max_value);
            vals[3] = std::clamp(vals[3], flags.min_value, flags.max_value);
            value.x = vals[0];
            value.y = vals[1];
            value.z = vals[2];
            value.w = vals[3];
            ctx.editor.mark_dirty();
        }
        return changed;
    }
);
```

### E. Rewrite Quat editor (in `inspector_editors.cpp`)

Replace the current Quat editor lambda. Key changes from F-05:

1. **REMOVED**: `ImGui::TextUnformatted(label.c_str())` and `ImGui::SameLine()`.
2. **REMOVED**: `ImGui::DragFloat` for Pitch/Yaw/Roll — replaced by `draw_axis_widget()` with axis colors and "X"/"Y"/"Z" display labels (descriptive tooltips show "Pitch (rotation around X axis)" etc.).
3. **CHANGED**: Display format from `"Pitch: %.1f"` / `"Yaw: %.1f"` / `"Roll: %.1f"` to `"%.2f"` on InputFloat (axis label is now on the drag-handle, not in the format string).
4. **KEPT**: `ImGui::PushID(id.c_str())` / `PopID()` for ID scoping.
5. **KEPT**: The Quat→Euler→degrees conversion, wrap to [-180, 180], degrees→radians→Quat round-trip via `from_euler()`.
6. **KEPT**: Drag speed = 0.5 for rotation.
7. **KEPT**: No EditorFlags propagation for the Quat editor (rotation is special-cased).

Implementation sketch:

```cpp
InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Quat>(
    [](const std::string& id, buddd::engine::math::Quat& value,
       const EditorFlags&,
       const EditorContext& ctx) -> bool {
        // ── Constants ──
        static constexpr double RAD_TO_DEG = 180.0 / 3.14159265358979323846;
        static constexpr double DEG_TO_RAD = 3.14159265358979323846 / 180.0;

        // Convert quat to Euler radians, then to degrees
        auto euler_rad = value.to_euler();
        float pitch_deg = static_cast<float>(euler_rad.x * RAD_TO_DEG);
        float yaw_deg   = static_cast<float>(euler_rad.y * RAD_TO_DEG);
        float roll_deg  = static_cast<float>(euler_rad.z * RAD_TO_DEG);

        // Wrap to [-180, 180]
        auto wrap = [](float deg) -> float {
            deg = std::fmod(deg + 180.0f, 360.0f);
            if (deg < 0.0f) deg += 360.0f;
            return deg - 180.0f;
        };
        pitch_deg = wrap(pitch_deg);
        yaw_deg   = wrap(yaw_deg);
        roll_deg  = wrap(roll_deg);

        ImGui::PushID(id.c_str());
        bool changed = false;
        constexpr float speed = 0.5f;

        changed |= draw_axis_widget("X", &pitch_deg,
                                     ImVec4(0.7f, 0.1f, 0.1f, 1.0f), speed, ctx,
                                     "Pitch (rotation around X axis)");
        ImGui::SameLine();
        changed |= draw_axis_widget("Y", &yaw_deg,
                                     ImVec4(0.0f, 0.55f, 0.0f, 1.0f), speed, ctx,
                                     "Yaw (rotation around Y axis)");
        ImGui::SameLine();
        changed |= draw_axis_widget("Z", &roll_deg,
                                     ImVec4(0.27f, 0.27f, 1.0f, 1.0f), speed, ctx,
                                     "Roll (rotation around Z axis)");
        ImGui::PopID();

        if (changed) {
            pitch_deg = wrap(pitch_deg);
            yaw_deg   = wrap(yaw_deg);
            roll_deg  = wrap(roll_deg);

            float pitch_rad = static_cast<float>(pitch_deg * DEG_TO_RAD);
            float yaw_rad   = static_cast<float>(yaw_deg * DEG_TO_RAD);
            float roll_rad  = static_cast<float>(roll_deg * DEG_TO_RAD);
            value = buddd::engine::math::Quat::from_euler(pitch_rad, yaw_rad, roll_rad);
            ctx.editor.mark_dirty();
        }
        return changed;
    }
);
```

Note: `#include <imgui.h>` is already present in `inspector_editors.cpp`. No new includes needed.

### F. Rewrite `draw_transform_section()` in `properties_panel.cpp`

Replace the current implementation with a 2-column table layout:

```cpp
auto PropertiesPanel::draw_transform_section(EditorContext const& ctx,
                                              buddd::engine::EntityId entity_id) -> void {
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    auto& transform = entity.transform();

    // Transform section header
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
    bool open = ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleVar();

    if (!open) return;

    // ── 2-column table (no headers) ──
    // Column 0: property name (fixed width, ~60px or CalcTextSize)
    // Column 1: value area (remaining width)
    constexpr int COLUMNS = 2;
    if (ImGui::BeginTable("##transform_table", COLUMNS, ImGuiTableFlags_None)) {
        // Column 0: width derived from content (label text fits naturally)
        ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed,
                                ImGui::CalcTextSize("Rotation").x + 16.0f);
        // Column 1: stretches to fill remaining width
        ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

        // ── Position row ──
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Position");
        ImGui::TableSetColumnIndex(1);
        static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
            "Position", transform.position, EditorFlags{}, ctx));

        // ── Rotation row ──
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Rotation");
        ImGui::TableSetColumnIndex(1);
        static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Quat>(
            "Rotation", transform.rotation, EditorFlags{}, ctx));

        // ── Scale row ──
        // F-05 spec requires Scale minimum value of 0.001 to prevent negative/zero scale.
        // This constraint is carried forward in F-06 (NG-08: "no changes to Scale editing behavior").
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Scale");
        ImGui::TableSetColumnIndex(1);
        EditorFlags scale_flags;
        scale_flags.min_value = 0.001f;
        static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
            "Scale", transform.scale, scale_flags, ctx));

        ImGui::EndTable();
    } else {
        // Graceful degradation: if BeginTable fails, fall back to inline layout
        // (calling editors without table structure).
        static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
            "Position", transform.position, EditorFlags{}, ctx));
        static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Quat>(
            "Rotation", transform.rotation, EditorFlags{}, ctx));
        EditorFlags scale_flags;
        scale_flags.min_value = 0.001f;
        static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
            "Scale", transform.scale, scale_flags, ctx));
    }
}
```

Key design choices:
- **Column 0 width**: Fixed width computed from `ImGui::CalcTextSize("Rotation")` + 16px padding. This is the widest label ("Rotation") plus comfortable padding. This ensures all three labels align vertically.
- **Column 1 width**: Stretches to fill remaining panel width (`WidthStretch`).
- **Table flags**: No headers, no borders (`ImGuiTableFlags_None`).
- **Scale flags**: `EditorFlags{min_value=0.001f}` — carries forward the F-05 Scale minimum constraint.
- **Graceful degradation**: If `BeginTable` returns `false`, falls back to the old inline layout (spec error case). This is an unlikely edge case with modern ImGui but spec says it must be handled.
- **ImGui::SameLine() removal**: Not needed since the editors no longer render the label text — the table layout handles column positioning.

### G. Additional includes to verify

- `inspector_editors.cpp` already includes `<imgui.h>` → fine.
- `inspector_editors.cpp` already includes `"editor.h"` — verify for `mark_dirty()`. Actually, looking at the current code, the editors receive `ctx` which has `ctx.editor.mark_dirty()`. The `EditorContext` is defined in `editor_context.h` which is included via `inspector_editors.h`. So no additional includes needed.
- `properties_panel.cpp` already includes `<imgui.h>`, `"inspector_editors.h"`, `"scene/entity.h"`, `"scene/world.h"` → fine. No additional includes needed.
- **`#include <algorithm>`** — required for `std::clamp` in Vec2, Vec3, and Vec4 editor lambdas. Check if already included transitively. If not, add it to `inspector_editors.cpp`.

## Required tests

### Unit tests

No new unit test files are created. Existing F-05 tests continue to pass unchanged. Specific new testing:

- **No new unit tests for the composite axis widget** — per NG-04 ("No headless ImGui tests"). The widget requires a running ImGui context with display capabilities.

### E2E / Integration verification

- **Manual smoke test**: Run `buddd edit` with a scene loaded. Select an entity. Verify:
  1. Transform section uses a 2-column table layout with no headers.
  2. "Position", "Rotation", "Scale" labels are in column 0, all vertically aligned.
  3. Each component (X/Y/Z) shows a colored drag-handle on the left side of the InputFloat.
  4. Red for X/Pitch, green for Y/Yaw, blue for Z/Roll, gray for W (if a Vec4 property is visible).
  5. Drag-handle text shows "X", "Y", "Z" on Position, Scale, and Rotation.
  6. Single-click on an InputFloat enters text edit mode immediately.
  7. Dragging the colored handle left/right scrubs the value.
  8. Rotation values are displayed in degrees with "%.2f" precision.
  9. Editing any component marks the scene dirty (star on window title).
  10. Scale cannot go below 0.001 (attempt to drag below this value should clamp).
  11. Entity name field and no-selection state are unchanged.
- **Screenshot capture**: Capture a screenshot of the Properties Panel with an entity selected. Submit with implementation PR for visual comparison.
- **Build verification**: `cmake --build --preset debug` with zero new warnings.
- **Test execution**: `ctest --preset debug` — all existing tests pass.

## Edge cases

| Case | Required handling |
|---|---|
| **Rapid clicking on drag handle** | InvisibleButton handles ImGui ActiveId correctly. No focus loss/gain issues. |
| **Extreme values (e.g., 1e10)** | InputFloat displays "%.2f" format. Very large values may show scientific notation. No clamping. |
| **Empty string entry on InputFloat** | ImGui InputFloat rejects non-numeric input natively. Value reverts to last valid numeric value on focus loss. |
| **Vec4 W component** | Uses neutral gray color `(0.7f, 0.7f, 0.7f, 1.0f)` for the drag handle, with label "W". |
| **Gimbal lock (pitch ≈ ±90°)** | Unchanged from F-05 — `Quat::to_euler()` produces deterministic result. |
| **Scene switch while editing** | Unchanged from F-05 — selection clears, in-progress drag terminated at frame boundary. |
| **Multiple transforms visible** (future multi-select) | Each selected entity's panel is separate; no shared state issues. |
| **Scale min value enforcement** | The Vec3 editor receives `EditorFlags{min_value=0.001f}` for the Scale row. The DragFloat flags are NOT applied to the composite axis widget directly (the axis widget does not use `min_value`/`max_value` in its drag-handle). Instead, after the drag-handle modifies the value, clamp it to the EditorFlags range in the Vec3 editor lambda. This ensures the min/max constraints are enforced regardless of how the value was modified (drag or InputFloat). |

Clarification on min/max enforcement: Since `draw_axis_widget()` does not accept `EditorFlags`, the Vec3 editor must clamp each component value after change:
```cpp
if (changed) {
    vals[0] = std::clamp(vals[0], flags.min_value, flags.max_value);
    vals[1] = std::clamp(vals[1], flags.min_value, flags.max_value);
    vals[2] = std::clamp(vals[2], flags.min_value, flags.max_value);
    value.x = vals[0];
    value.y = vals[1];
    value.z = vals[2];
    ctx.editor.mark_dirty();
}
```

**Important**: The Vec3 editor is used by both Position (unbounded) and Scale (min=0.001). The clamping logic is generic based on `EditorFlags` — when `min_value` is `-max<float>()` (the default), `std::clamp` is a no-op. This means the clamping code must be present in the Vec3 (and Vec2, Vec4) editors, not just when constraints are needed. The clamp is harmless for unbounded cases because `std::clamp(x, -FLT_MAX, FLT_MAX)` returns `x` unchanged.

## Security impact

None. No file I/O, no authentication, no network access. Input validation is handled by ImGui (InputFloat rejects non-numeric input). All edits are to in-memory data only.

## Data and migration impact

None. No schema changes, no migrations, no seed data changes. Transform edits are in-memory until explicit save.

## API compatibility impact

- **`draw_axis_widget()`**: New file-local helper. Not part of the public API. No impact.
- **Vec2/Vec3/Vec4/Quat editors**: The `InspectorTypeEditorRegistry::draw<T>()` API is unchanged — callers still pass `id`, `value`, `flags`, `ctx`. The `id` is now **ignored for display** by Vec/Quat editors but is still consumed for `PushID` scoping. This is backward-compatible: all existing callers still compile and work, but the label text no longer appears as rendered output from the editor. The label must be rendered by the caller (e.g., `draw_transform_section()` in the table column 0).
- **`EditorFlags`**: Unchanged.
- **No ABI break**: The editor is a static library; ABI concerns do not apply.

## Documentation impact

- **README**: None.
- **Wiki pages** (to be updated by wiki-agent):
  - `docs/wiki/editor/editor-panels.md`: Update the Inspector Panel section to describe the new table layout and composite axis widget UX in the Transform section. Update the Property Editors table (Vec2/Vec3/Vec4/Quat rows) to reflect axis-colored composite widgets and label ownership change.
- **Other specs**: None.

## ADR impact

No new ADR required. The implementation follows established patterns (ImGui tables API, existing editor registration system). No architectural decisions are changed or created.

## Done criteria

| # | Criterion | Verification |
|---|---|---|
| 1 | File-local `draw_axis_widget()` helper exists in `inspector_editors.cpp` | Code review: anonymous namespace function with correct signature `(const char* id, float* value, ImVec4 color, float drag_speed, const EditorContext& ctx, const char* tooltip = nullptr) -> bool` |
| 2 | `draw_axis_widget()` draws a colored 20px-wide rectangle via ImDrawList with white text | Code review: uses `GetWindowDrawList()->AddRectFilled()` with axis color, `AddText()` with white text |
| 3 | `draw_axis_widget()` overlays an InvisibleButton for hit testing | Code review: `ImGui::InvisibleButton("##handle", ImVec2(20.0f, ImGui::GetFrameHeight()))` |
| 4 | `draw_axis_widget()` handles drag-to-scrub via `GetMouseDragDelta()` with pixel-to-value conversion factor `drag_speed * 0.01f` | Code review: drag logic with `initial_values` map, `GetMouseDragDelta().x`, and the conversion formula |
| 5 | `draw_axis_widget()` provides `ImGui::InputFloat("##input", value, 0.0f, 0.0f, "%.2f")` on the right | Code review: `SameLine()` + `SetNextItemWidth(60.0f)` + `InputFloat` |
| 6 | `draw_axis_widget()` returns `true` when value changes (by drag or InputFloat) | Code review: return value is OR of drag-change and InputFloat-change |
| 7 | `draw_axis_widget()` does NOT call `ctx.editor.mark_dirty()` — dirty marking is the parent editor's responsibility | Code review: no `mark_dirty()` call inside `draw_axis_widget()` |
| 8 | `draw_axis_widget()` wraps content in `PushID(id)` / `PopID()` | Code review: ID scoping present |
| 9 | Vec2 editor: label rendering removed, labels "X" and "Y" passed to axis widgets with correct axis colors (red, green) | Code review: no `TextUnformatted(label)`, no `SameLine()` before axis widgets |
| 10 | Vec3 editor: label rendering removed, labels "X"/"Y"/"Z" with correct axis colors (red, green, blue) | Code review: same pattern as Vec2 |
| 11 | Vec2, Vec3, Vec4 editors clamp component values to `[flags.min_value, flags.max_value]` after change | Code review: `std::clamp` applied to each `vals[i]` before copying to `value` in Vec2, Vec3, and Vec4 lambdas |
| 12 | Vec4 editor: label rendering removed, labels "X"/"Y"/"Z"/"W" with axis colors (red, green, blue, gray) | Code review: correct colors, W uses `(0.7f, 0.7f, 0.7f, 1.0f)` |
| 13 | Quat editor: label rendering removed, uses "X"/"Y"/"Z" labels with axis colors (red, green, blue) and descriptive tooltips | Code review: `draw_axis_widget("X", ..., ..., "Pitch (rotation around X axis)")` etc. |
| 14 | Quat editor: display format changed to "%.2f" on InputFloat (was "%.1f" on DragFloat) | Code review: `InputFloat` format is "%.2f", axis label text is on the drag-handle |
| 15 | Quat editor: Quat→Euler→degrees conversion, wrap to [-180, 180], degrees→radians→Quat round-trip preserved unchanged | Code review: same wrap logic, `to_euler()`/`from_euler()` round-trip as F-05 |
| 16 | Quat editor: drag speed = 0.5f for rotation | Code review: `constexpr float speed = 0.5f;` |
| 17 | `draw_transform_section()` uses 2-column `ImGui::BeginTable` with no column headers | Code review: `ImGui::BeginTable("##transform_table", 2, ImGuiTableFlags_None)` |
| 18 | `draw_transform_section()` renders "Position"/"Rotation"/"Scale" text in column 0 via `ImGui::TextUnformatted()` | Code review: `TableSetColumnIndex(0)`, `TextUnformatted("Position")` etc. |
| 19 | `draw_transform_section()` calls editors in column 1 | Code review: `TableSetColumnIndex(1)`, `draw<Vec3>("Position", ...)` etc. |
| 20 | `draw_transform_section()` passes `EditorFlags{min_value=0.001f}` for the Scale row | Code review: `scale_flags.min_value = 0.001f;` passed to `draw<Vec3>("Scale", transform.scale, scale_flags, ctx)` |
| 21 | `draw_transform_section()` gracefully degrades if `BeginTable` returns `false` | Code review: `if (ImGui::BeginTable(...)) { ... ImGui::EndTable(); } else { fallback ... }` |
| 22 | `draw_fallback_readonly()` is NOT modified, moved, or removed | Code review: no changes to the function or its declaration |
| 23 | `inspector_editors.h` is NOT modified | Git diff: no changes to header file |
| 24 | `properties_panel.h` is NOT modified | Git diff: no changes to header file |
| 25 | All existing tests pass | `ctest --preset debug` passes |
| 26 | Zero new compiler warnings | `cmake --build --preset debug` shows zero new warnings |
| 27 | `draw_axis_widget()` tooltip parameter is optional (defaults to `nullptr`) and documented | Code review: signature has `const char* tooltip = nullptr`, doc has `@param tooltip` |
| 28 | Colored rectangle height matches InputFloat height via `ImGui::GetFrameHeight()` | Code review: rectangle and InvisibleButton use `ImGui::GetFrameHeight()` for height |
| 29 | Green axis color uses the darker shade `ImVec4(0.0f, 0.55f, 0.0f, 1.0f)` | Code review: all "Y" widget calls pass the correct green color |
| 30 | Quat editor uses "X"/"Y"/"Z" labels on drag-handles with descriptive tooltips for Pitch/Yaw/Roll | Code review: `draw_axis_widget("X", ..., "Pitch (rotation ...)")`, etc. |
