#pragma once

#include "editor_panel.h"
#include "editor_context.h"

#include "scene/entity_id.h"
#include "scene/world.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::editor {

/// Panel that renders the entity hierarchy tree and supports entity operations
/// (Create Empty, Delete, Rename) via right-click context menu and keyboard shortcuts.
class ScenePanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override;
    [[nodiscard]] auto title() const -> std::string_view override;
    auto draw_ui(EditorContext const& ctx) -> void override;

private:
    // ── Inline rename state ──
    std::optional<buddd::engine::EntityId> renaming_entity_;
    char rename_buffer_[256] = {};

    // ── Context menu state ──
    buddd::engine::EntityId context_menu_entity_ = buddd::engine::EntityId::none();

    // ── Delete confirmation state ──
    bool show_delete_confirmation_ = false;
    std::vector<buddd::engine::EntityId> pending_deletion_ids_;
    size_t pending_deletion_with_children_ = 0;
    std::string pending_deletion_first_name_;

    // ── Command helpers ──
    auto execute_create_entity(EditorContext const& ctx,
                               std::optional<buddd::engine::EntityId> parent = std::nullopt) -> void;
    auto execute_delete_entity(EditorContext const& ctx) -> void;
    auto start_rename(EditorContext const& ctx, buddd::engine::EntityId id) -> void;
    auto confirm_rename(EditorContext const& ctx) -> void;
    auto cancel_rename() -> void;
    auto draw_delete_confirmation_modal(EditorContext const& ctx) -> void;

    /// Collect EntityIds in depth-first tree order between anchor and clicked (inclusive).
    auto collect_range(buddd::engine::World& world,
                       buddd::engine::EntityId anchor,
                       buddd::engine::EntityId clicked) const
        -> std::vector<buddd::engine::EntityId>;

    /// Collect all EntityIds in depth-first tree order.
    auto collect_all(buddd::engine::World& world) const
        -> std::vector<buddd::engine::EntityId>;
};

} // namespace buddd::editor
