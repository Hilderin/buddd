#pragma once

#include "editor_panel.h"

#include <imgui.h>
#include <string_view>

namespace buddd::editor {

class AssetsPanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "assets"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Assets"; }

    auto draw_ui(EditorContext const& /*ctx*/) -> void override {
        // (empty — placeholder for future content)
    }
};

} // namespace buddd::editor
