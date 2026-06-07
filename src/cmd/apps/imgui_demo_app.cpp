#include "apps/imgui_demo_app.h"

#include <imgui.h>

namespace be = buddd::engine;

auto buddd::cmd::app::ImguiDemoApp::on_render(be::EngineContext const& /*ctx*/) -> void {
    // Show the Dear ImGui Demo window
    if (show_demo_window_) {
        ImGui::ShowDemoWindow();
    }

    // Custom info panel
    ImGui::Begin("ImGui Demo");
    ImGui::Checkbox("Show Demo Window", &show_demo_window_);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}
