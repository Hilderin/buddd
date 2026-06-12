#pragma once

#include "editor_panel.h"

#include <imgui.h>
#include <string_view>

namespace buddd::editor {

class ProjectPanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "project"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Project"; }

    auto draw_ui(EditorContext const& /*ctx*/) -> void override {
        // (empty — placeholder for future content)
    }
};

} // namespace buddd::editor
