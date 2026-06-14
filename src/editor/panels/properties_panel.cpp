#include "panels/properties_panel.h"

#include "editor.h"
#include "editor_selection.h"
#include "editor_context.h"
#include "inspector_editors.h"
#include "commands/rename_entity_command.h"
#include "commands/set_component_property_command.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/type_registry.h"

#include <imgui.h>

#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace buddd::editor {

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

    // ── 2-column table (no headers) ──
    // Column 0: property name (fixed width based on "Rotation" text)
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

        // Collapsible header — default closed
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
        bool open = ImGui::CollapsingHeader(type_name.data(), ImGuiTreeNodeFlags_None);
        ImGui::PopStyleVar();

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

                    // Create and execute command
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

            ImGui::EndTable();
        }
        ImGui::Unindent(4.0f);
    }
}

} // namespace buddd::editor
