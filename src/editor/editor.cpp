#include "editor.h"

#include "engine_context.h"
#include "error.h"
#include "imgui/engine_imgui.h"

#include <imgui.h>

namespace be = buddd::engine;

namespace buddd::editor {

Editor::Editor() = default;

Editor::~Editor() {
    shutdown();
}

auto Editor::setup(be::EngineContext const& ctx) -> be::Result<void> {
    engine_ = &ctx.services;
    window_ = &ctx.window;
    initialized_ = true;

    if (!be::engine_imgui::is_initialized()) {
        return make_error(be::Error::Category::InitFailed,
            "ImGui is not initialized. The editor requires a display with working ImGui.");
    }

    return {};
}

auto Editor::draw_ui(be::EngineContext const& /*ctx*/) -> void {
    if (!initialized_) {
        return;
    }

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
}

auto Editor::shutdown() -> void {
    initialized_ = false;
    engine_ = nullptr;
    window_ = nullptr;
}

} // namespace buddd::editor
