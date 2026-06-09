#pragma once

#include "editor_panel.h"

#include <string_view>

namespace buddd::editor {

class ScenePanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "scene"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Scene"; }

    auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void override {
        // (empty — placeholder for future content)
        // Editor wraps this in ImGui::Begin(title) / End()
    }
};

} // namespace buddd::editor
