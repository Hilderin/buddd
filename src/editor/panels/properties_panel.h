#pragma once

#include "editor_panel.h"
#include "editor_selection.h"

#include "scene/entity_id.h"

#include <cstdint>
#include <imgui.h>
#include <optional>
#include <string>
#include <string_view>

namespace buddd::editor {

class PropertiesPanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "properties"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Properties"; }

    auto draw_ui(EditorContext const& ctx) -> void override;

    // Request keyboard focus to move from filter field into the list on next frame
    inline auto request_focus_list() -> void { pending_focus_list_ = true; }

private:
    // ── Entity name editing state ──
    std::optional<buddd::engine::EntityId> editing_entity_;
    std::string rename_buffer_;
    Selection previous_selection_snapshot_;
    uint64_t last_selection_gen_ = 0;

    // ── Add Component popup state ──
    char add_component_filter_[64] = "";
    std::optional<size_t> pending_auto_expand_index_;
    std::optional<size_t> pending_focus_first_prop_index_;
    bool pending_focus_list_ = false;
    int popup_selected_index_ = -1; // index within currently visible filtered list
    ImVec2 add_component_popup_pos_{0.0f, 0.0f};

    // ── Helper methods ──
    auto draw_entity_name(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_transform_section(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_component_sections(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_add_component_button(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_add_component_popup(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_no_selection_state() -> void;
};

} // namespace buddd::editor
