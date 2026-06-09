#pragma once

#include "editor_panel.h"

#include <imgui.h>
#include <string_view>

namespace buddd::editor {

class PropertiesPanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "properties"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Properties"; }

    auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void override {
        // (empty — placeholder for future content)
    }
};

} // namespace buddd::editor
