#include "panels/properties_panel.h"

#include "editor.h"
#include "editor_selection.h"
#include "editor_context.h"
#include "inspector_editors.h"
#include "commands/rename_entity_command.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <imgui.h>

#include <cmath>
#include <cstring>
#include <memory>
#include <string>
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
        editing_entity_ = entity_id;
        rename_buffer_ = current_name;
    }

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
        // Confirm: push RenameEntityCommand if name changed and non-empty
        std::string new_name(rename_buffer_);
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
}

// ═══════════════════════════════════════════════════════════════════════════
// draw_transform_section
// ═══════════════════════════════════════════════════════════════════════════

auto PropertiesPanel::draw_transform_section(EditorContext const& ctx,
                                              buddd::engine::EntityId entity_id) -> void {
    auto& world = ctx.editor.world();
    auto entity = world.entity(entity_id);
    auto& transform = entity.transform();

    // Transform section header — always expanded
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
    bool open = ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleVar();

    if (!open) return;  // Should never happen with DefaultOpen, but defensive

    // ── Position row ──
    // The Vec3 editor handles dirty marking internally via ctx.editor.mark_dirty()
    // when the value changes.
    static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
        "Position", transform.position, EditorFlags{}, ctx));

    // ── Rotation row ──
    // The Quat editor handles dirty marking internally.
    // The editor converts Quat→Euler degrees for display and Euler→Quat on edit.
    static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Quat>(
        "Rotation", transform.rotation, EditorFlags{}, ctx));

    // ── Scale row ──
    static_cast<void>(InspectorTypeEditorRegistry::draw<buddd::engine::math::Vec3>(
        "Scale", transform.scale, EditorFlags{}, ctx));
}

} // namespace buddd::editor
