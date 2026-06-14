# IMPL-F-07 — Properties Panel Undo Polish

## Source spec

`.specs/sprint-2026-06/properties-panel-undo-polish/spec.md`

## Goal

Add undo support for transform edits (Position, Rotation, Scale) via a new `SetTransformCommand` that captures all three properties as native math types. Fix double-label display in component tables by using `##` hidden labels for float/int/bool/string editors. Replace the float DragFloat editor with a composite InputFloat + gray drag-handle widget for single-click editing. Flip the `draw_axis_widget()` layout so the colored drag handle appears on the RIGHT of the InputFloat. Implement drag-undo merging via `CommandStack::peek_undo()` and `Command::try_update_new_value()` so a single continuous drag produces exactly one undo step.

## Non-goals

- No changes to `inspector_editors.h` — only `.cpp` changes.
- No changes to `properties_panel.h` — only `.cpp` changes.
- No changes to `editor_selection`, `Editor`, `EditorContext`, or any engine files.
- No changes to entity name field behavior (`draw_entity_name()`).
- No changes to no-selection state.
- No changes to `draw_fallback_readonly()` — it remains untouched.
- No changes to Color editor (already uses `##color` labels).
- No changes to int editor behavior (stays DragInt, only label fix).
- No changes to Quat angle display format or wrapping.
- No new dependencies — only `<imgui.h>`, `<yaml-cpp/yaml.h>`, and existing editor headers.
- No headless ImGui unit tests (manual smoke test + screenshot only).
- No changes to `CMakeLists.txt` — new `.h` files are auto-discovered via GLOB_RECURSE.
- No changes to any engine files (`src/engine/`).

## Relevant ADRs

- **ADR-029**: Editor UX Decisions — confirms fixed panel layout per tab type. No constraints on undo, labels, or widget styling.
- **ADR-026**: ImGui docking branch v1.91.8-docking, provides `ImGui::InputFloat`, `ImGui::InvisibleButton`, `ImGui::GetMouseDragDelta()`.

## Files to inspect

| File | Purpose |
|---|---|
| `src/editor/command.h` | Read existing `Command` base class; add `try_update_new_value()` virtual method |
| `src/editor/command_stack.h` | Read `CommandStack` API; add `peek_undo()` |
| `src/editor/command_stack.cpp` | Read existing implementation; add `peek_undo()` body |
| `src/editor/commands/set_component_property_command.h` | Read existing command; add `try_update_new_value()` override |
| `src/editor/inspector_editors.cpp` | Read current editors; modify float/editor, flip `draw_axis_widget()`, fix hidden labels |
| `src/editor/panels/properties_panel.cpp` | Read `draw_transform_section()` and `draw_component_sections()`; add command creation and merge logic |
| `src/engine/scene/transform.h` | Read `Transform` struct (Vec3 position, Quat rotation, Vec3 scale) |
| `.specs/sprint-2026-06/properties-panel-undo-polish/spec.md` | Source spec for acceptance criteria |

## Files allowed to change

| File | Change description |
|---|---|
| `src/editor/command.h` | Add virtual `try_update_new_value(YAML::Node const& new_value, EditorContext const& ctx) -> bool` with default `false` return. Forward-declare `YAML::Node`. Include `editor_context.h` if not already included (for `EditorContext` parameter). |
| `src/editor/command_stack.h` | Add `[[nodiscard]] auto peek_undo() noexcept -> Command*;` declaration |
| `src/editor/command_stack.cpp` | Implement `peek_undo()` — returns `undo_stack_.empty() ? nullptr : undo_stack_.back().get()` |
| `src/editor/commands/set_component_property_command.h` | Add `try_update_new_value(YAML::Node const& new_value, EditorContext const& ctx) -> bool override` that compares incoming YAML with `new_value_`, updates via `YAML::Clone` if different, returns true |
| `src/editor/commands/set_transform_command.h` | **New file**: `SetTransformCommand` — stores `entity_id_`, `old_position_`, `old_rotation_`, `old_scale_`, `new_position_`, `new_rotation_`, `new_scale_` as native Vec3/Quat. Overrides `execute()`, `undo()`, `name()`, `try_update_new_value()`. All three properties are captured in every command (all-in-one). |
| `src/editor/inspector_editors.cpp` | **(1)** Flip `draw_axis_widget()`: InputFloat first, then drag handle on right. **(2)** Replace float DragFloat editor with composite InputFloat + gray drag handle (no text label on handle). **(3)** Change float/int/bool/string editors to use `##val` hidden labels. |
| `src/editor/panels/properties_panel.cpp` | **(1)** In `draw_transform_section()`: snapshot old transform values BEFORE drawing, check `changed` after drawing, use `peek_undo` + `try_update_new_value` to merge, else push new `SetTransformCommand`. **(2)** In `draw_component_sections()`: before pushing new `SetComponentPropertyCommand`, call `peek_undo()` + `try_update_new_value()` for merge. |

## Files forbidden to change

- `src/editor/inspector_editors.h` — no API changes
- `src/editor/panels/properties_panel.h` — no structural changes needed
- Any file in `src/engine/` — no engine changes (NG-01)
- Any `CMakeLists.txt` — new `.h` file discovered via GLOB_RECURSE
- Any `.yaml`, `.json`, or configuration files
- Any existing test files — existing tests must pass unchanged
- `src/editor/commands/create_entity_command.h` — no changes
- `src/editor/commands/delete_entity_command.h` — no changes
- `src/editor/commands/rename_entity_command.h` — no changes
- `src/editor/commands/quit_command.h` — no changes

## Existing conventions to follow

- **Namespace**: `buddd::editor` for editor code.
- **C++ style**: Trailing return types (`auto foo() -> Bar`), `[[nodiscard]]` on query methods.
- **Logging**: `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", "...")` for command execution/merge debug logs.
- **ImGui patterns**: `PushID`/`PopID` for ID scoping; `ImGui::GetWindowDrawList()` for custom drawing.
- **No `using namespace ImGui`** — use explicit `ImGui::` prefix throughout.
- **Command classes**: Defined entirely in header files (as `final` class), matching the pattern of `set_component_property_command.h`.
- **Dirty marking**: Each editor calls `ctx.editor.mark_dirty()` internally when value changes.
- **EditorFlags defaults**: `min_value = -std::numeric_limits<float>::max()`, `max_value = std::numeric_limits<float>::max()`, `step_value = 0.0f`.
- **YAML::Node ownership**: Use `YAML::Clone()` for deep copies when storing node values, not bare assignment (which creates aliases).
- **Include style**: New `.h` command in `src/editor/commands/` follows existing pattern — no `.cpp` file needed.

## Required implementation behavior

### A. `command.h` — add `try_update_new_value()`

Add a forward declaration of `YAML::Node` at the top of the file, and add the virtual method to `Command`:

```cpp
#pragma once

#include <string_view>

// Forward declaration for try_update_new_value parameter
namespace YAML {
class Node;
}

namespace buddd::editor {

struct EditorContext;

class Command {
public:
    virtual ~Command() = default;
    virtual auto execute(EditorContext const& ctx) -> void = 0;
    virtual auto undo(EditorContext const& ctx) -> void = 0;
    [[nodiscard]] virtual auto name() const -> std::string_view = 0;

    /// Attempt to update this command's new value from incoming state.
    /// @param new_value The latest value (used by SetComponentPropertyCommand).
    /// @param ctx       Editor context (used by SetTransformCommand to read current entity transform).
    /// @return true if the command accepted the update (caller should NOT push a new command).
    [[nodiscard]] virtual auto try_update_new_value(YAML::Node const& new_value,
                                                     EditorContext const& ctx) -> bool;
};

} // namespace buddd::editor
```

The base implementation returns `false` (inline in header or defined in a `.cpp` — but since there's no `command.cpp`, define it inline):

```cpp
inline auto Command::try_update_new_value(YAML::Node const&, EditorContext const&) -> bool {
    return false;
}
```

### B. `command_stack.h` — add `peek_undo()`

Add after `can_undo()`:

```cpp
/// Returns a pointer to the most recent command on the undo stack, or nullptr if empty.
[[nodiscard]] auto peek_undo() noexcept -> Command*;
```

### C. `command_stack.cpp` — implement `peek_undo()`

```cpp
auto CommandStack::peek_undo() noexcept -> Command* {
    if (undo_stack_.empty()) {
        return nullptr;
    }
    return undo_stack_.back().get();
}
```

### D. `set_component_property_command.h` — add `try_update_new_value() override`

Add to the class declaration (within the existing `class SetComponentPropertyCommand final : public Command`):

```cpp
[[nodiscard]] auto try_update_new_value(YAML::Node const& new_value,
                                         EditorContext const& ctx) -> bool override;
```

Implementation (add before or after the `name()` override, inside the class body):

```cpp
auto try_update_new_value(YAML::Node const& new_value, EditorContext const& ctx) -> bool override {
    // Safety: prevent cross-entity merge — command's entity_id_ must match
    // the currently selected primary entity.
    auto primary = ctx.editor.selection().primary();
    if (!primary.has_value() || *primary != entity_id_) {
        return false;
    }

    // Belt-and-suspenders: verify the target entity still exists.
    // The component_type_name_ and property_name_ are immutable (set at
    // construction); entity existence confirms the command's identity
    // is still valid, preventing stale-entity merge.
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id_);
    if (entity.id() == buddd::engine::EntityId::none()) {
        return false;
    }

    if (new_value == new_value_) {
        return false;  // Same value — nothing to update
    }

    new_value_ = YAML::Clone(new_value);
    BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
        "Merged SetComponentPropertyCommand: entity={} comp={} prop={}",
        entity_id_.index, component_type_name_, property_name_);
    return true;
}
```

### E. New file: `src/editor/commands/set_transform_command.h`

Full implementation:

```cpp
#pragma once

#include "command.h"
#include "editor.h"
#include "editor_context.h"
#include "editor_selection.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/transform.h"

#include <string_view>

namespace buddd::editor {

/// Command that stores all three transform properties (Position, Rotation, Scale)
/// as native math types (Vec3/Quat). Every SetTransformCommand captures all 3
/// properties regardless of which specific property was changed.
/// No YAML, no TransformProperty enum.
class SetTransformCommand final : public Command {
public:
    SetTransformCommand(
        buddd::engine::EntityId entity_id,
        buddd::engine::math::Vec3 old_position,
        buddd::engine::math::Quat old_rotation,
        buddd::engine::math::Vec3 old_scale,
        buddd::engine::math::Vec3 new_position,
        buddd::engine::math::Quat new_rotation,
        buddd::engine::math::Vec3 new_scale)
        : entity_id_(entity_id)
        , old_position_(old_position)
        , old_rotation_(old_rotation)
        , old_scale_(old_scale)
        , new_position_(new_position)
        , new_rotation_(new_rotation)
        , new_scale_(new_scale)
    {}

    auto execute(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetTransformCommand: entity {} not found", entity_id_.index);
            return;
        }

        auto& t = entity.transform();
        t.position = new_position_;
        t.rotation = new_rotation_;
        t.scale = new_scale_;
        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "SetTransform: entity={}", entity_id_.index);
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetTransformCommand UNDO: entity {} not found", entity_id_.index);
            return;
        }

        auto& t = entity.transform();
        t.position = old_position_;
        t.rotation = old_rotation_;
        t.scale = old_scale_;
        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "SetTransform UNDO: entity={}", entity_id_.index);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Set Transform";
    }

    [[nodiscard]] auto try_update_new_value(YAML::Node const& /*new_value*/,
                                             EditorContext const& ctx) -> bool override {
        // Safety: prevent cross-entity merge — command's entity_id_ must match
        // the currently selected primary entity.
        auto primary = ctx.editor.selection().primary();
        if (!primary.has_value() || *primary != entity_id_) {
            return false;
        }

        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        if (entity.id() == buddd::engine::EntityId::none()) {
            return false;
        }

        auto& t = entity.transform();
        new_position_ = t.position;
        new_rotation_ = t.rotation;
        new_scale_ = t.scale;

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "Merged SetTransformCommand for entity={}", entity_id_.index);
        return true;
    }

private:
    buddd::engine::EntityId entity_id_;

    buddd::engine::math::Vec3 old_position_;
    buddd::engine::math::Quat old_rotation_;
    buddd::engine::math::Vec3 old_scale_;

    buddd::engine::math::Vec3 new_position_;
    buddd::engine::math::Quat new_rotation_;
    buddd::engine::math::Vec3 new_scale_;
};

} // namespace buddd::editor
```

### F. `inspector_editors.cpp` — flip `draw_axis_widget()` layout

Reorder the content inside `draw_axis_widget()`: draw InputFloat FIRST, then the colored drag handle on the RIGHT via `SameLine(0.0f, 0.0f)`.

The function signature and return type remain unchanged. Only the internal layout order changes.

**New structure** (flipped):

```
[ InputFloat("##input") ][■ Label]   (handle on right)
                      ^-- SameLine(0,0)
```

Implementation sketch:

```cpp
auto draw_axis_widget(const char* id, float* value, ImVec4 color,
                      float drag_speed, const EditorContext& ctx,
                      const char* tooltip = nullptr) -> bool {
    (void)ctx;

    ImGui::PushID(id);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float widget_height = ImGui::GetFrameHeight();

    // ── InputFloat (LEFT side) ──
    ImGui::SetNextItemWidth(60.0f);
    bool input_changed = ImGui::InputFloat("##input", value, 0.0f, 0.0f, "%.2f");

    // ── Colored drag handle (RIGHT side, flush against input) ──
    ImGui::SameLine(0.0f, 0.0f);

    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

    // Colored rectangle
    draw_list->AddRectFilled(cursor_pos,
                              ImVec2(cursor_pos.x + 20.0f, cursor_pos.y + widget_height),
                              ImGui::ColorConvertFloat4ToU32(color));

    // Centered text
    ImVec2 text_size = ImGui::CalcTextSize(id);
    draw_list->AddText(
        ImVec2(cursor_pos.x + (20.0f - text_size.x) * 0.5f,
               cursor_pos.y + (widget_height - text_size.y) * 0.5f),
        IM_COL32(255, 255, 255, 255), id);

    // InvisibleButton for hit testing
    ImGui::InvisibleButton("##handle", ImVec2(20.0f, widget_height));

    // Tooltip
    if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("%s", tooltip);
    }

    // Drag handling
    static std::unordered_map<const void*, float> initial_values;
    bool drag_changed = false;

    if (ImGui::IsItemActive()) {
        if (ImGui::IsItemActivated()) {
            initial_values[static_cast<const void*>(value)] = *value;
        }
        float pixel_delta = ImGui::GetMouseDragDelta().x;
        float new_val = initial_values[static_cast<const void*>(value)]
                        + pixel_delta * drag_speed * 0.01f;
        if (new_val != *value) {
            *value = new_val;
            drag_changed = true;
        }
    }

    if (ImGui::IsItemDeactivated()) {
        initial_values.erase(static_cast<const void*>(value));
    }

    ImGui::PopID();

    return drag_changed || input_changed;
}
```

### G. `inspector_editors.cpp` — float editor composite widget

Replace the DragFloat-based float editor lambda:

```cpp
// float — composite widget: InputFloat + gray drag handle
InspectorTypeEditorRegistry::register_editor<float>(
    [](const std::string& label, float& value, const EditorFlags& flags,
       const EditorContext& ctx) -> bool {
        float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;

        ImGui::PushID(label.c_str());

        // ── InputFloat (left side) ──
        ImGui::SetNextItemWidth(60.0f);
        bool changed = ImGui::InputFloat("##val", &value, 0.0f, 0.0f, "%.2f");

        // ── Gray drag handle (right side) ──
        ImGui::SameLine(0.0f, 0.0f);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        float widget_height = ImGui::GetFrameHeight();

        ImVec4 gray_color(0.5f, 0.5f, 0.5f, 1.0f);
        draw_list->AddRectFilled(
            cursor_pos,
            ImVec2(cursor_pos.x + 20.0f, cursor_pos.y + widget_height),
            ImGui::ColorConvertFloat4ToU32(gray_color));

        ImGui::InvisibleButton("##handle", ImVec2(20.0f, widget_height));

        // Drag-to-scrub
        static std::unordered_map<const void*, float> initial_values;
        if (ImGui::IsItemActive()) {
            if (ImGui::IsItemActivated()) {
                initial_values[static_cast<const void*>(&value)] = value;
            }
            float pixel_delta = ImGui::GetMouseDragDelta().x;
            float new_val = initial_values[static_cast<const void*>(&value)]
                            + pixel_delta * speed * 0.01f;
            if (new_val != value) {
                value = new_val;
                changed = true;
            }
        }
        if (ImGui::IsItemDeactivated()) {
            initial_values.erase(static_cast<const void*>(&value));
        }

        ImGui::PopID();

        if (changed) {
            value = std::clamp(value, flags.min_value, flags.max_value);
            ctx.editor.mark_dirty();
        }
        return changed;
    }
);
```

### H. `inspector_editors.cpp` — hidden labels for int/bool/string editors

Change the label parameter from `label.c_str()` to `"##val"` in each editor lambda:

| Editor | Old | New |
|---|---|---|
| int (DragInt) | `ImGui::DragInt(label.c_str(), &value, ...)` | `ImGui::DragInt("##val", &value, ...)` |
| bool (Checkbox) | `ImGui::Checkbox(label.c_str(), &value)` | `ImGui::Checkbox("##val", &value)` |
| string (InputText) | `ImGui::InputText(label.c_str(), buf, BUF_SIZE)` | `ImGui::InputText("##val", buf, BUF_SIZE)` |

The `label` parameter is still received for PushID scoping at the caller level — no changes to the `PushID` pattern needed since the caller's `InspectorTypeEditorRegistry::draw<T>()` handles ID scope via the `label` parameter in the registry `draw` function.

### I. `properties_panel.cpp` — `draw_transform_section()` undo integration

Restructure `draw_transform_section()` to:

1. Snapshot all three old transform values BEFORE drawing any editors.
2. Track `changed` across all three editor draws (Position, Rotation, Scale).
3. After all editors are drawn and the table is ended, if `changed` is true, attempt merge via `peek_undo()` + `try_update_new_value()`. If merge fails, push a new `SetTransformCommand`.

Implementation sketch (changes highlighted):

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

    // ── Snapshot old transform values BEFORE any edits ──
    auto old_position = transform.position;
    auto old_rotation = transform.rotation;
    auto old_scale = transform.scale;

    bool changed = false;

    // ── 2-column table (no headers) ──
    constexpr int COLUMNS = 2;
    if (ImGui::BeginTable("##transform_table", COLUMNS, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed,
                                ImGui::CalcTextSize("Rotation").x + 16.0f);
        ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

        // ── Position row ──
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Position");
        ImGui::TableSetColumnIndex(1);
        changed |= InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
            "Position", transform.position, EditorFlags{}, ctx);

        // ── Rotation row ──
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Rotation");
        ImGui::TableSetColumnIndex(1);
        changed |= InspectorTypeEditorRegistry::draw<buddd::engine::math::Quat>(
            "Rotation", transform.rotation, EditorFlags{}, ctx);

        // ── Scale row ──
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Scale");
        ImGui::TableSetColumnIndex(1);
        EditorFlags scale_flags;
        scale_flags.min_value = 0.001f;
        changed |= InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
            "Scale", transform.scale, scale_flags, ctx);

        ImGui::EndTable();
    } else {
        // Graceful degradation
        changed |= InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
            "Position", transform.position, EditorFlags{}, ctx);
        changed |= InspectorTypeEditorRegistry::draw<buddd::engine::math::Quat>(
            "Rotation", transform.rotation, EditorFlags{}, ctx);
        EditorFlags scale_flags;
        scale_flags.min_value = 0.001f;
        changed |= InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
            "Scale", transform.scale, scale_flags, ctx);
    }

    // ── After all edits: push or merge SetTransformCommand ──
    if (changed) {
        auto* last = ctx.editor.command_stack().peek_undo();
        YAML::Node empty;
        if (last && last->try_update_new_value(empty, ctx)) {
            // Successfully merged into existing command
            BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
                "Merged SetTransformCommand for entity={}", entity_id.index);
        } else {
            auto cmd = std::make_unique<SetTransformCommand>(
                entity_id,
                old_position, old_rotation, old_scale,
                transform.position, transform.rotation, transform.scale
            );
            ctx.editor.command_stack().execute(std::move(cmd), ctx);
        }
    }
}
```

### J. `properties_panel.cpp` — `draw_component_sections()` merge support

In the existing `if (changed)` block (around line 345-364 of the current file), wrap the command push with a merge check:

```cpp
if (changed) {
    // Encode back to YAML
    auto new_yaml = buddd::engine::TypeRegistry::yaml_encode(prop_type, *any_result, ser_ctx);
    if (!new_yaml) {
        BUDDD_LOG_TAGGED_WARN("Editor:ComponentProperties",
            "Failed to encode property '{}' after edit: {}",
            prop_name, new_yaml.error().message);
        continue;
    }

    // Try merge with last command
    auto* last = ctx.editor.command_stack().peek_undo();
    if (last && last->try_update_new_value(*new_yaml, ctx)) {
        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "Merged SetComponentPropertyCommand for entity={} prop={}",
            entity_id.index, prop_name);
    } else {
        // Create and execute new command
        auto cmd = std::make_unique<SetComponentPropertyCommand>(
            entity_id,
            std::string(type_name),
            std::string(prop_name),
            yaml_node,
            std::move(*new_yaml)
        );
        ctx.editor.command_stack().execute(std::move(cmd), ctx);
    }
}
```

### K. Add include for `set_transform_command.h` in `properties_panel.cpp`

Add at the top with the other command includes:

```cpp
#include "commands/set_transform_command.h"
```

## Required tests

### Unit tests

No new unit test files. Existing tests (`properties_panel_tests.cpp`) continue to pass unchanged.

### E2E / Integration verification

- **Manual smoke test**: Run `buddd edit` with a scene loaded. Select an entity. Verify:
  1. Position edit → Ctrl+Z reverts Position, Rotation, AND Scale to pre-edit state.
  2. Rotation edit → Ctrl+Z reverts full transform.
  3. Scale edit → Ctrl+Z reverts full transform.
  4. After undo, Ctrl+Shift+Z redoes the transform edit.
  5. Component property float edit → Ctrl+Z reverts to pre-drag value.
  6. Single click on float field enters text edit mode (cursor appears).
  7. Gray drag handle appears on the RIGHT side of float editors.
  8. Vec axis handles appear on the RIGHT side of InputFloat (X=red, Y=green, Z=blue).
  9. No duplicate labels in component property tables: label in column 0 only, widget area in column 1 is clean.
  10. Dragging a property continuously produces ONE undo step (not one per frame).
  11. Editing two different properties sequentially produces TWO separate undo steps.
  12. Scene dirty marker (star on title) appears on any edit.
- **Screenshot capture**: Before/after screenshots of: (a) component properties with no duplicate labels; (b) float editor with gray handle on right; (c) Vec3 Position layout with right-side axis handles.
- **Build verification**: `cmake --build --preset debug` with zero new warnings.
- **Test execution**: `ctest --preset debug` — all existing tests pass.

## Edge cases

| Case | Required handling |
|---|---|
| **Drag gesture starting and ending at same value** | Command has old==new. Merge succeeds (`try_update_new_value` reads current values, updates new_* to same as old). Undo is a no-op. |
| **Rapid alternating edits between two properties** | Each property produces its own command. No cross-property merging. |
| **All-in-one undo groups all three transform properties** | `SetTransformCommand` captures all three. Changing any one of Position/Rotation/Scale captures all three as old → undo restores all three to pre-edit state. |
| **Same property on two different entities** | No merge — entity_id differs. `SetTransformCommand::try_update_new_value` uses `entity_id_`; `SetComponentPropertyCommand::try_update_new_value` uses the command's own `entity_id_` + component/property (implicit). |
| **Very long drag with many intermediate values** | Only one command on stack per continuous drag gesture (merged each frame). |
| **Vec component clamp (Scale min_value = 0.001)** | Existing clamp in Vec3 editor preserved. `SetTransformCommand` captures the clamped value as-is. |
| **Rotation near gimbal lock** | Existing Quat→Euler→Quat round-trip unchanged. |
| **Stale entity ID (destroyed entity)** | `SetTransformCommand::execute()` / `undo()` / `try_update_new_value()` all check entity validity. If `entity.id() == EntityId::none()`, logs a warning and returns early. |
| **Float editor with negative values** | `InputFloat` supports negative values natively. Gray drag handle scrubbing also supports negatives. |
| **Float editor with very large values (1e10)** | `InputFloat` displays and edits large floats without overflow. |
| **Float editor: empty string entry** | ImGui `InputFloat` rejects non-numeric input natively; reverts to last valid value on focus loss. |
| **Int editor: hidden label effect on clickable area** | `##val` does not affect clickable area — widget remains fully interactive. |
| **Bool editor: Checkbox without visible label** | Checkbox square remains clickable. No text appears next to it. |
| **String editor: InputText without visible label** | Input field remains interactive. No label text appears. |
| **Scene switch during a drag** | In-progress drag terminates at frame boundary. Selection clears. No stale command pushed. |
| **Multi-select (primary entity edited)** | Only primary entity's transform/components are edited. No cross-entity merging. |
| **`Command::try_update_new_value()` called on non-mergable command type** | Base implementation returns false → new command is pushed normally. |
| **`peek_undo()` on empty stack** | Returns `nullptr` → new command is always pushed. |

## Security impact

None. No file I/O, no authentication, no network access. All edits are in-memory data mutations.

## Data and migration impact

None. No schema changes, migrations, or seed data changes.

## API compatibility impact

- **`Command::try_update_new_value()`**: New virtual method. All existing command subclasses inherit the default implementation (returns false). Backward-compatible.
- **`CommandStack::peek_undo()`**: New method on existing class. No existing callers affected.
- **`SetTransformCommand`**: New command class. No existing code affected.
- **`draw_axis_widget()`**: File-local helper in `inspector_editors.cpp`. Layout change only (InputFloat first, handle second). Signature unchanged. Callers (Vec2/Vec3/Vec4/Quat editors) unchanged.
- **Float editor**: Changed from `DragFloat` to composite widget. The `InspectorTypeEditorRegistry::draw<float>()` API is unchanged — callers still pass `(label, value, flags, ctx)`. The hidden label `##val` ensures no visible label duplication.
- **Int/bool/string editors**: Label changed to `##val`. `InspectorTypeEditorRegistry::draw<T>()` API unchanged.
- **No ABI break**: Editor is a static library; ABI concerns do not apply.

## Documentation impact

- **README**: None.
- **Wiki pages** (to be updated by wiki-agent):
  - `docs/wiki/editor/editor-panels.md`: Update Properties Panel section to describe undo support for transforms, the composite float widget, hidden labels, and axis handle layout position.
- **Other specs**: None.

## ADR impact

No new ADR required. The implementation follows established patterns (Command pattern, ImGui drawing, EditorContext access pattern). No architectural decisions are changed or created.

## Done criteria

| # | Criterion | Verification |
|---|---|---|
| 1 | `SetTransformCommand` class exists in `src/editor/commands/set_transform_command.h` storing all 3 transform properties as native Vec3/Quat with no YAML and no enum | Code review: file exists, class stores `Vec3 position`, `Quat rotation`, `Vec3 scale` for old/new |
| 2 | `SetTransformCommand::execute()` writes all three new_* values to `entity.transform()` and calls `ctx.editor.mark_dirty()` | Code review |
| 3 | `SetTransformCommand::undo()` restores all three old_* values to `entity.transform()` and calls `ctx.editor.mark_dirty()` | Code review |
| 4 | `SetTransformCommand::name()` returns "Set Transform" | Code review |
| 5 | `SetTransformCommand::execute()`/`undo()`/`try_update_new_value()` check for stale entity ID and return early with warning; `try_update_new_value()` also checks `entity_id_` matches `ctx.editor.selection().primary()` | Code review: `entity.id() == EntityId::none()` check with `BUDDD_LOG_TAGGED_WARN`; entity identity check via `primary()` comparison |
| 6 | `SetTransformCommand::try_update_new_value()` reads all 3 current values from `entity.transform()` and updates `new_*` fields | Code review |
| 7 | `Command::try_update_new_value(YAML::Node const&, EditorContext const&) -> bool` virtual method exists in `command.h` with default `false` return | Code review |
| 8 | `CommandStack::peek_undo()` method exists in `command_stack.h`, returns `Command*` (nullptr if empty) | Code review |
| 9 | `SetComponentPropertyCommand::try_update_new_value()` override exists, checks `entity_id_` matches `ctx.editor.selection().primary()`, verifies entity still exists, compares incoming YAML with `new_value_`, updates via `YAML::Clone` if different | Code review |
| 10 | `draw_transform_section()` captures all 3 old transform values BEFORE drawing | Code review: snapshots before editor draws |
| 11 | `draw_transform_section()` checks `changed` after all editors, uses `peek_undo()` + `try_update_new_value()` to merge | Code review |
| 12 | `draw_transform_section()` pushes new `SetTransformCommand` when merge fails or stack is empty | Code review |
| 13 | Scale edits in `draw_transform_section()` push a `SetTransformCommand` (not direct mutation) | Code review |
| 14 | `draw_component_sections()` checks `peek_undo()` and calls `try_update_new_value()` before pushing new `SetComponentPropertyCommand` | Code review |
| 15 | `draw_axis_widget()` renders InputFloat on LEFT, colored drag handle on RIGHT | Code review: InputFloat before handle, `SameLine(0.0f, 0.0f)` after InputFloat |
| 16 | Float editor uses composite widget: `ImGui::InputFloat("##val", ...)` + gray drag handle (`ImVec4(0.5f, 0.5f, 0.5f, 1.0f)`) on the right | Code review: label is `##val`, gray handle, drag speed from step_value (default 0.1) |
| 17 | Float editor supports single-click text entry via `InputFloat` and drag-to-scrub via gray handle | Code review + manual |
| 18 | Int editor uses hidden label `"##val"` for `DragInt` | Code review |
| 19 | Bool editor uses hidden label `"##val"` for `Checkbox` | Code review |
| 20 | String editor uses hidden label `"##val"` for `InputText` | Code review |
| 21 | Vec2, Vec3, Vec4, Quat editors all use updated right-side axis handle layout | Code review: all call `draw_axis_widget()` which is flipped |
| 22 | Include `"commands/set_transform_command.h"` added to `properties_panel.cpp` | Code review |
| 23 | `inspector_editors.h` is NOT modified | Git diff: no changes to header |
| 24 | `properties_panel.h` is NOT modified | Git diff: no changes to header |
| 25 | All existing tests pass | `ctest --preset debug` passes |
| 26 | Zero new compiler warnings | `cmake --build --preset debug` shows zero new warnings |
| 27 | Observer logging: `SetTransformCommand` execution and merge events logged via `BUDDD_LOG_TAGGED_DEBUG("Editor:Command", ...)` | Code review |
| 28 | `SetTransformCommand::try_update_new_value()` checks `entity_id_` against `ctx.editor.selection().primary()` before merging, preventing cross-entity merge | Code review: `!primary.has_value() || *primary != entity_id_` guard |
| 29 | `SetComponentPropertyCommand::try_update_new_value()` checks `entity_id_` against `ctx.editor.selection().primary()` and verifies entity exists before merging | Code review: primary guard + `entity.id() == EntityId::none()` check |
