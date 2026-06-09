#pragma once

#include "editor_panel.h"

#include <string_view>

namespace buddd::editor {

class ConsolePanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "console"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Console"; }

    auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void override {
        // (empty — placeholder for future content)
    }
};

} // namespace buddd::editor
