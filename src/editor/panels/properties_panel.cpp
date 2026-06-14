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

} // namespace buddd::editor
