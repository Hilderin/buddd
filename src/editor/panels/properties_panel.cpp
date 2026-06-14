#include "panels/properties_panel.h"

#include "editor.h"
#include "editor_selection.h"
#include "editor_context.h"
#include "inspector_editors.h"
#include "commands/add_component_command.h"
#include "commands/remove_component_command.h"
#include "commands/rename_entity_command.h"
#include "commands/set_component_property_command.h"
#include "commands/set_transform_command.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/component_info.h"
#include "scene/component_registry/type_registry.h"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace buddd::editor {

// Local helper: InputText callback to catch Tab and Arrow Up/Down while in the
// filter field and request focus transfer to the component list.
static int AddComponentFilterCallback(ImGuiInputTextCallbackData* data) {
    if (!data || !data->UserData) return 0;
    auto* self = reinterpret_cast<PropertiesPanel*>(data->UserData);
    // CallbackCompletion is triggered by Tab; CallbackHistory by Up/Down.
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion ||
        data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        self->request_focus_list();
        // Do not modify text, just request focus shift.
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_ui — main entry point
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_ui(EditorContext const& ctx) -> void {
    auto primary = ctx.editor.selection().primary();

    if (!primary.has_value()) {
        draw_no_selection_state();
        return;
    }

    auto entity_id = *primary;

    // Defensive: check for valid entity
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    if (entity.id() == buddd::engine::EntityId::none()) {
        // Stale/invalid entity — clear selection and show no-selection state
        ctx.editor.selection().clear();
        draw_no_selection_state();
        return;
    }

    // ── Entity name field ──
    draw_entity_name(ctx, entity_id);
    ImGui::Separator();

    // ── Transform section ──
    draw_transform_section(ctx, entity_id);

    // ── Component sections ──
    draw_component_sections(ctx, entity_id);

    // ── Add Component button ──
    draw_add_component_button(ctx, entity_id);
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_no_selection_state
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_no_selection_state() -> void {
    auto avail = ImGui::GetContentRegionAvail();
    auto text_size = ImGui::CalcTextSize("No entity selected");
    ImGui::SetCursorPosY((avail.y - text_size.y) * 0.5f);
    ImGui::SetCursorPosX((avail.x - text_size.x) * 0.5f);
    ImGui::TextUnformatted("No entity selected");
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_entity_name
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_entity_name(EditorContext const& ctx,
                                        buddd::engine::EntityId entity_id) -> void {
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    auto current_name = entity.name();

    // Detect selection change and update buffer
    if (editing_entity_ != entity_id) {
        // Commit any pending rename for the previous entity before switching
        if (editing_entity_.has_value()) {
            auto old_entity = world.entity(*editing_entity_);
            if (old_entity.id() != buddd::engine::EntityId::none()
                && rename_buffer_ != old_entity.name()) {
                auto cmd = std::make_unique<RenameEntityCommand>(
                    *editing_entity_,
                    std::string(old_entity.name()),
                    rename_buffer_,
                    previous_selection_snapshot_
                );
                ctx.editor.command_stack().execute(std::move(cmd), ctx);
            }
        }
        editing_entity_ = entity_id;
        rename_buffer_ = current_name;
    }

    // Save the pending rename BEFORE any sync, so we can detect
    // deactivation-after-edit even if the sync resets rename_buffer_.
    std::string pending_rename = rename_buffer_;

    // Sync buffer with external name changes (e.g., Scene Panel rename)
    if (rename_buffer_ != current_name && !ImGui::IsItemActive()) {
        rename_buffer_ = current_name;
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    char buf[256];
    std::strncpy(buf, rename_buffer_.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
    bool confirmed = ImGui::InputText("##entity_name", buf, sizeof(buf), input_flags);

    // Update the buffer from input
    rename_buffer_ = buf;

    if (confirmed || ImGui::IsItemDeactivatedAfterEdit()) {
        // Use the actual input buffer on Enter, or the pre-sync value if
        // the user clicked away (deactivated) to avoid losing edits from the
        // sync reset above.
        std::string new_name = confirmed ? rename_buffer_ : pending_rename;
        if (!new_name.empty() && new_name != current_name) {
            auto cmd = std::make_unique<RenameEntityCommand>(
                entity_id,
                std::string(current_name),    // old_name
                std::move(new_name)           // new_name
            );
            ctx.editor.command_stack().execute(std::move(cmd), ctx);
            // After command execution, re-read the name
            rename_buffer_ = entity.name();
        } else if (new_name.empty()) {
            // Revert to previous name (no command pushed)
            rename_buffer_ = current_name;
        }
    }

    // Save current selection snapshot for undo/redo correctness when
    // auto-committing a rename on entity switch (only on actual change).
    auto gen = ctx.editor.selection().generation();
    if (gen != last_selection_gen_) {
        previous_selection_snapshot_ = ctx.editor.selection().snapshot();
        last_selection_gen_ = gen;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_transform_section
// ═══════════════════════════════════════════════════════════════════════════

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

    // ── 2-column table (no headers) ──
    // Column 0: property name (fixed width based on "Rotation" text)
    // Column 1: value area (remaining width)
    constexpr int COLUMNS = 2;
    EditorFlags scale_flags;
    scale_flags.min_value = 0.001f;

    // Helper: push or merge a SetTransformCommand for a specific property
    auto push_or_merge = [&](TransformProperty prop, std::string_view prop_name) {
        auto* last = ctx.editor.command_stack().peek_undo();
        YAML::Node empty;
        if (last && last->try_update_new_value(empty, ctx, prop_name)) {
            BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
                "Merged SetTransformCommand for entity={} prop={}",
                entity_id.index, prop_name);
        } else {
            auto cmd = std::make_unique<SetTransformCommand>(
                entity_id, prop,
                old_position, old_rotation, old_scale,
                transform.position, transform.rotation, transform.scale
            );
            ctx.editor.command_stack().execute(std::move(cmd), ctx);
        }
    };

    bool table_ok = false;

    if (ImGui::BeginTable("##transform_table", COLUMNS, ImGuiTableFlags_None)) {
        table_ok = true;
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
        if (InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
                "Position", transform.position, EditorFlags{}, ctx)) {
            push_or_merge(TransformProperty::Position, "Position");
        }

        // ── Rotation row ──
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Rotation");
        ImGui::TableSetColumnIndex(1);
        if (InspectorTypeEditorRegistry::draw<buddd::engine::math::Quat>(
                "Rotation", transform.rotation, EditorFlags{}, ctx)) {
            push_or_merge(TransformProperty::Rotation, "Rotation");
        }

        // ── Scale row ──
        // F-05 spec requires Scale minimum value of 0.001 to prevent negative/zero scale.
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Scale");
        ImGui::TableSetColumnIndex(1);
        if (InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
                "Scale", transform.scale, scale_flags, ctx)) {
            push_or_merge(TransformProperty::Scale, "Scale");
        }

        ImGui::EndTable();
    }

    if (!table_ok) {
        // Graceful degradation: if BeginTable fails, fall back to inline layout
        if (InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
                "Position", transform.position, EditorFlags{}, ctx)) {
            push_or_merge(TransformProperty::Position, "Position");
        }
        if (InspectorTypeEditorRegistry::draw<buddd::engine::math::Quat>(
                "Rotation", transform.rotation, EditorFlags{}, ctx)) {
            push_or_merge(TransformProperty::Rotation, "Rotation");
        }
        if (InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
                "Scale", transform.scale, scale_flags, ctx)) {
            push_or_merge(TransformProperty::Scale, "Scale");
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_component_sections
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_component_sections(EditorContext const& ctx,
                                               buddd::engine::EntityId entity_id) -> void {
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    if (entity.id() == buddd::engine::EntityId::none()) return;

    auto& registry = ctx.engine.services.registry();
    auto& assets = ctx.engine.services.assets();

    // Build type_index → ComponentInfoBase* map (SceneSaver pattern).
    // Rebuilt each frame — cheap (<20 registered component types).
    std::unordered_map<std::type_index, const buddd::engine::ComponentInfoBase*> type_to_info;
    for (const auto* info : registry.all_types()) {
        auto* mutable_info = const_cast<buddd::engine::ComponentInfoBase*>(info);
        auto tmp = mutable_info->create();
        type_to_info[std::type_index(typeid(*tmp))] = info;
    }

    size_t component_count = entity.component_count();

    // Log when selection changes (first draw or new entity)
    static buddd::engine::EntityId last_logged_entity = buddd::engine::EntityId::none();
    if (entity_id != last_logged_entity) {
        BUDDD_LOG_TAGGED_DEBUG("Editor:Properties",
            "Showing entity {} with {} components", entity_id.index, component_count);
        last_logged_entity = entity_id;
    }

    // Separator before first component section
    if (component_count > 0) {
        ImGui::Separator();
    }

    for (size_t i = 0; i < component_count; ++i) {
        auto& comp = entity.component_at(i);

        // Look up ComponentInfoBase* by type_index (keyed on the actual Component subclass)
        auto it = type_to_info.find(std::type_index(typeid(comp)));
        if (it == type_to_info.end()) {
            BUDDD_LOG_TAGGED_DEBUG("Editor:Properties",
                "Skipping component at index {} — no ComponentInfoBase found for type",
                i);
            continue;
        }
        const auto* info = it->second;

        auto type_name = info->type_name();
        size_t prop_count = info->property_count();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Properties",
            "Drawing component section '{}' ({} properties)", type_name, prop_count);

        // Auto-expand for newly added component (matched by index) and prepare focus on first property
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
        if (pending_auto_expand_index_.has_value() && i == *pending_auto_expand_index_) {
            // Prefer SetNextItemOpen for reliability
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            // Also request focusing first property field in this section
            pending_focus_first_prop_index_ = i;
            pending_auto_expand_index_.reset();
        }

        // Use ImGui's built-in close button via p_open parameter.
        // When the user clicks the X on the header, ImGui sets *p_open = false.
        // We detect this and push a RemoveComponentCommand instead of hiding the section.
        ImGui::PushID(static_cast<int>(i));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
        bool component_visible = true;
        bool open = ImGui::CollapsingHeader(type_name.data(), &component_visible, flags);
        ImGui::PopStyleVar();
        if (!component_visible) {
            // ImGui's built-in close button was clicked → remove this component
            auto cmd = std::make_unique<RemoveComponentCommand>(entity_id, std::string(type_name), i);
            ctx.editor.command_stack().execute(std::move(cmd), ctx);
            ImGui::PopID();
            break; // Exit loop — component removed, indices have shifted
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::SetTooltip("Remove %s component", type_name.data());
        }
        ImGui::PopID();

        if (!open) continue;

        if (prop_count == 0) {
            // Centered "No editable properties" text in disabled style
            auto avail = ImGui::GetContentRegionAvail();
            auto text_size = ImGui::CalcTextSize("No editable properties");
            ImGui::SetCursorPosX((avail.x - text_size.x) * 0.5f);
            ImGui::TextDisabled("No editable properties");
            continue;
        }

        // 2-column table matching Transform section layout
        constexpr int COLUMNS = 2;
        ImGui::Indent(4.0f);  // slight indent for visual hierarchy
        if (ImGui::BeginTable("##prop_table", COLUMNS, ImGuiTableFlags_None)) {
            // Column 0: fixed width based on longest property name
            float max_label_width = 60.0f;  // minimum width
            for (size_t j = 0; j < prop_count; ++j) {
                float w = ImGui::CalcTextSize(info->property_name(j).data()).x;
                if (w > max_label_width) max_label_width = w;
            }
            ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed,
                                    max_label_width + 12.0f);
            ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

            auto ser_ctx = buddd::engine::SerializationContext{assets};

            for (size_t j = 0; j < prop_count; ++j) {
                auto prop_name = info->property_name(j);
                auto prop_type = info->property_type_index(j);
                auto prop_flags = info->property_flags(j);

                // Read current value as YAML
                auto yaml_node = info->property_serialize(comp, j, ser_ctx);

                // Decode YAML to std::any
                auto any_result = buddd::engine::TypeRegistry::yaml_decode(prop_type, yaml_node, ser_ctx);
                if (!any_result) {
                    BUDDD_LOG_TAGGED_WARN("Editor:ComponentProperties",
                        "Failed to decode property '{}': {}",
                        prop_name, any_result.error().message);
                    continue;
                }

                // Map PropertyFlags to EditorFlags
                EditorFlags editor_flags;
                editor_flags.min_value = prop_flags.min_value;
                editor_flags.max_value = prop_flags.max_value;
                editor_flags.step_value = prop_flags.step_value;
                editor_flags.tags_ = prop_flags.tags_;

                // Draw the editor
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(prop_name.data());
                ImGui::TableSetColumnIndex(1);
                // If this is the newly added component, focus the first property's input widget
                if (pending_focus_first_prop_index_.has_value() && i == *pending_focus_first_prop_index_ && j == 0) {
                    ImGui::SetKeyboardFocusHere();
                    // clear so we don't repeatedly steal focus
                    pending_focus_first_prop_index_.reset();
                }

                bool changed = InspectorTypeEditorRegistry::draw_any(
                    std::string(prop_name), *any_result, prop_type, editor_flags, ctx);

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
                    if (last && last->try_update_new_value(*new_yaml, ctx, prop_name)) {
                        // Merged into existing command — also execute to write
                        // the updated value to the entity (the editor only modified
                        // a local any_result, not the entity itself).
                        last->execute(ctx);
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
            }

            ImGui::EndTable();
        }
        ImGui::Unindent(4.0f);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_add_component_button
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_add_component_button(EditorContext const& ctx,
                                                  buddd::engine::EntityId entity_id) -> void {
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::Indent(8.0f);
    float button_width = ImGui::GetContentRegionAvail().x;
    // Draw button and capture its screen position (for popup positioning)
    ImGui::Button("+ Add Component", ImVec2(button_width, 0.0f));
    add_component_popup_pos_ = ImGui::GetItemRectMin();
    if (ImGui::IsItemActivated()) {
        ImGui::OpenPopup("Add Component");
        add_component_filter_[0] = '\0';
        auto& registry = ctx.engine.services.registry();
        BUDDD_LOG_TAGGED_DEBUG("Editor:Properties",
            "AddComponent popup opened, {} types available", registry.all_types().size());
    }
    ImGui::Unindent(8.0f);
    draw_add_component_popup(ctx, entity_id);
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_add_component_popup
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_add_component_popup(EditorContext const& ctx,
                                                 buddd::engine::EntityId entity_id) -> void {
    if (!ImGui::IsPopupOpen("Add Component")) return;

    auto& registry = ctx.engine.services.registry();
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    if (entity.id() == buddd::engine::EntityId::none()) return;

    // Position the popup below the Add Component button (dropdown style)
    // Pivot (0.0, 0.0) = top-left of popup aligns with top-left of the button
    ImGui::SetNextWindowPos(add_component_popup_pos_, ImGuiCond_Appearing, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(280, 320), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Add Component", nullptr, ImGuiWindowFlags_None)) {
        // Auto-focus filter on first frame the popup opens
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
            popup_selected_index_ = 0; // default to first item
        }
        // Use callback to catch Tab/Arrow keys while in filter field
        ImGuiInputTextFlags filter_flags = ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_EnterReturnsTrue;
        bool pressed_enter_in_filter = ImGui::InputTextWithHint("##filter", "Filter types...", add_component_filter_,
                                  sizeof(add_component_filter_), filter_flags, AddComponentFilterCallback, this);
        ImGui::Separator();

        // Get filter string (lowercase)
        std::string filter_str(add_component_filter_);
        std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(),
            [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });

        // Collect all registered types, sort alphabetically, and produce filtered visible list
        std::vector<const buddd::engine::ComponentInfoBase*> types;
        for (const auto* info : registry.all_types()) types.push_back(info);
        std::sort(types.begin(), types.end(), [](const auto* a, const auto* b) { return a->type_name() < b->type_name(); });
        std::vector<const buddd::engine::ComponentInfoBase*> visible_types;
        visible_types.reserve(types.size());
        for (const auto* info : types) {
            auto tn = info->type_name();
            if (!filter_str.empty()) {
                std::string tn_lower(tn);
                std::transform(tn_lower.begin(), tn_lower.end(), tn_lower.begin(),
                    [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
                if (tn_lower.find(filter_str) == std::string::npos) continue;
            }
            visible_types.push_back(info);
        }
        // Clamp selection within visible range
        if (visible_types.empty()) {
            popup_selected_index_ = -1;
        } else if (popup_selected_index_ < 0) {
            popup_selected_index_ = 0;
        } else if (popup_selected_index_ >= static_cast<int>(visible_types.size())) {
            popup_selected_index_ = static_cast<int>(visible_types.size()) - 1;
        }

        // First visible type for Enter-in-filter
        std::string first_visible_type;
        if (!visible_types.empty()) first_visible_type = visible_types.front()->type_name();

        // Enter in filter → add current selected item if any, else first visible; if none, no-op
        if (pressed_enter_in_filter) {
            if (!visible_types.empty()) {
                int sel = popup_selected_index_;
                if (sel < 0 || sel >= static_cast<int>(visible_types.size())) sel = 0; // default to first
                auto tn = visible_types[sel]->type_name();
                auto cmd = std::make_unique<AddComponentCommand>(entity_id, std::string(tn));
                ctx.editor.command_stack().execute(std::move(cmd), ctx);
                // Newly added component is at the back
                pending_auto_expand_index_ = entity.component_count() - 1;
                pending_focus_first_prop_index_ = *pending_auto_expand_index_;
                ImGui::CloseCurrentPopup();
            }
        }
        // Tab/Down in filter → request focusing first visible list item on NEXT frame (more reliable)
        if (ImGui::IsItemActive() && (ImGui::IsKeyPressed(ImGuiKey_Tab) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))) {
            pending_focus_list_ = true;
        }

        // Scrollable list
        ImGui::BeginChild("##comp_list", ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()),
                          ImGuiChildFlags_Borders, ImGuiWindowFlags_NavFlattened);
        bool any_visible = false;
        // Handle Up/Down navigation when list has focus
        bool list_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        if (list_focused && !visible_types.empty()) {
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                popup_selected_index_ = std::min(popup_selected_index_ + 1, static_cast<int>(visible_types.size()) - 1);
            } else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                popup_selected_index_ = std::max(popup_selected_index_ - 1, 0);
            }
        }

        bool focus_set_on_first = false;
        for (int vi = 0; vi < static_cast<int>(visible_types.size()); ++vi) {
            auto tn = visible_types[vi]->type_name();
            any_visible = true;
            bool is_selected = (vi == popup_selected_index_);
            if (pending_focus_list_ && !focus_set_on_first) {
                ImGui::SetItemDefaultFocus();
                ImGui::SetKeyboardFocusHere();
                focus_set_on_first = true;
                pending_focus_list_ = false;
            }
            if (ImGui::Selectable(tn.data(), is_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    // Double-click: add component
                    auto cmd = std::make_unique<AddComponentCommand>(entity_id, std::string(tn));
                    ctx.editor.command_stack().execute(std::move(cmd), ctx);
                    pending_auto_expand_index_ = entity.component_count() - 1;
                    pending_focus_first_prop_index_ = *pending_auto_expand_index_;
                    ImGui::CloseCurrentPopup();
                } else {
                    // Single-click: update selection only (do not add)
                    popup_selected_index_ = vi;
                }
            }
            // Keyboard Enter activates selected item
            if (list_focused && is_selected && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                auto cmd = std::make_unique<AddComponentCommand>(entity_id, std::string(tn));
                ctx.editor.command_stack().execute(std::move(cmd), ctx);
                pending_auto_expand_index_ = entity.component_count() - 1;
                pending_focus_first_prop_index_ = *pending_auto_expand_index_;
                ImGui::CloseCurrentPopup();
                break;
            }
        }
        if (!any_visible) {
            ImGui::TextDisabled("No matching components");
        }
        ImGui::EndChild();

        // Close button
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace buddd::editor
