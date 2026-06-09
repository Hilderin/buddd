#pragma once

#include <string_view>

namespace buddd::engine { struct EngineContext; }

namespace buddd::editor {

/// Base class for dockable editor panels.
class EditorPanel {
public:
    virtual ~EditorPanel() = default;

    /// Unique identifier (e.g., "scene", "properties").
    [[nodiscard]] virtual auto id() const -> std::string_view = 0;

    /// ImGui window title displayed in the panel title bar.
    [[nodiscard]] virtual auto title() const -> std::string_view = 0;

    /// Per-frame logic. Called every frame from Editor::update().
    virtual auto update(buddd::engine::EngineContext const& /*ctx*/) -> void {}

    /// Per-frame UI rendering. Called every frame from Editor::draw_ui()
    /// inside the dockspace, between ImGui::Begin(title) and ImGui::End().
    virtual auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void {}
};

} // namespace buddd::editor
