#pragma once

#include "editor_panel.h"

#include "scene/entity_id.h"

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

private:
    // ── Entity name editing state ──
    std::optional<buddd::engine::EntityId> editing_entity_;
    std::string rename_buffer_;

    // ── Helper methods ──
    auto draw_entity_name(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_transform_section(EditorContext const& ctx, buddd::engine::EntityId entity_id) -> void;
    auto draw_no_selection_state() -> void;
};

} // namespace buddd::editor
