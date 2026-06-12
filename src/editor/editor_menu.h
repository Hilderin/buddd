#pragma once

#include <string_view>

namespace buddd::editor {

struct EditorContext;

/// Base class for editor overlay elements rendered before the dockspace.
class EditorMenu {
public:
    virtual ~EditorMenu() = default;

    /// Unique identifier (e.g., "menu_bar").
    [[nodiscard]] virtual auto id() const -> std::string_view = 0;

    /// Per-frame logic. Called every frame from Editor::update().
    virtual auto update(EditorContext const& /*ctx*/) -> void {}

    /// Per-frame UI rendering. Called every frame from Editor::draw_ui()
    /// before ImGui::DockSpaceOverlay().
    virtual auto draw_ui(EditorContext const& /*ctx*/) -> void {}
};

} // namespace buddd::editor
