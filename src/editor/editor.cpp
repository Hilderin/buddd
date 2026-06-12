#include "editor.h"

#include "panels/menu_bar.h"
#include "panels/scene_panel.h"
#include "panels/properties_panel.h"
#include "panels/console_panel.h"
#include "panels/project_panel.h"
#include "panels/assets_panel.h"
#include "engine_context.h"
#include "engine_service.h"
#include "error.h"
#include "imgui/engine_imgui.h"
#include "input/key_code.h"
#include "input/input_system.h"
#include "log/log.h"
#include "platform/platform.h"
#include "version.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdio>
#include <fstream>
#include <string>

BUDDD_LOG_TAG("Editor");

namespace be = buddd::engine;

namespace buddd::editor {

Editor::Editor()
    : world_(std::make_unique<be::World>())
{
    BUDDD_LOG_DEBUG("Editor: created empty World");
}

Editor::~Editor() {
    BUDDD_LOG_DEBUG("Editor: destroyed World");
    shutdown();
}

auto Editor::world() -> be::World& {
    return *world_;
}

auto Editor::setup(be::EngineContext const& ctx) -> be::Result<void> {
    engine_ = &ctx.services;
    window_ = &ctx.window;
    initialized_ = true;

    if (!be::engine_imgui::is_initialized()) {
        return make_error(be::Error::Category::InitFailed,
            "ImGui is not initialized. The editor requires a display with working ImGui.");
    }

    // Enable docking
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Enable docking layout persistence.
    // Before loading, validate the ini file: if it has window entries but no docking data,
    // it was saved by a buggy version (e.g. with double ImGui::Begin/End) and must be reset.
    {
        std::ifstream ini("buddd_editor.ini");
        bool has_docking = false;
        bool has_windows = false;
        std::string line;
        while (std::getline(ini, line)) {
            if (line.find("[Docking][Data]") != std::string::npos)
                has_docking = true;
            if (line.find("[Window][") != std::string::npos)
                has_windows = true;
        }
        ini.close();
        if (has_windows && !has_docking) {
            BUDDD_LOG_WARN("Editor: stale ini file detected (no docking data) — resetting");
            std::remove("buddd_editor.ini");
        }
    }
    ImGui::GetIO().IniFilename = "buddd_editor.ini";
    BUDDD_LOG_INFO("Editor: layout file: buddd_editor.ini");

    // ── Create menu bar ──
    auto menu_bar = std::make_unique<MenuBar>(command_stack_);
    menu_bar->set_on_about([this]() {
        show_about_ = true;
    });
    add_menu(std::move(menu_bar));

    // ── Register panels ──
    add_panel(std::make_unique<ScenePanel>());
    add_panel(std::make_unique<PropertiesPanel>());
    add_panel(std::make_unique<ConsolePanel>());
    add_panel(std::make_unique<ProjectPanel>());
    add_panel(std::make_unique<AssetsPanel>());

    // ── Register shortcuts ──
    shortcuts_.bind(be::KeyCode::Q, {.ctrl = true}, [](be::EngineContext const& ctx) {
        ctx.request_exit();
    });
    shortcuts_.bind(be::KeyCode::Z, {.ctrl = true}, [this](be::EngineContext const&) {
        [[maybe_unused]] auto _ = command_stack_.undo();
    });
    shortcuts_.bind(be::KeyCode::Z, {.ctrl = true, .shift = true}, [this](be::EngineContext const&) {
        [[maybe_unused]] auto _ = command_stack_.redo();
    });
    shortcuts_.bind(be::KeyCode::Y, {.ctrl = true}, [this](be::EngineContext const&) {
        [[maybe_unused]] auto _ = command_stack_.redo();
    });

    return {};
}

auto Editor::update(be::EngineContext const& ctx) -> void {
    if (!initialized_) {
        return;
    }

    // ═══════════════════════════════════════════════
    // Keyboard shortcuts (gated by WantCaptureKeyboard)
    // ═══════════════════════════════════════════════
    shortcuts_.process(ctx, ImGui::GetIO().WantCaptureKeyboard);

    // ═══════════════════════════════════════════════
    // Delegate to registered menus and panels
    // ═══════════════════════════════════════════════
    for (auto& menu : menus_) {
        menu->update(ctx);
    }
    for (auto& panel : panels_) {
        panel->update(ctx);
    }
}

auto Editor::add_menu(std::unique_ptr<EditorMenu> menu) -> void {
    menus_.push_back(std::move(menu));
}

auto Editor::add_panel(std::unique_ptr<EditorPanel> panel) -> void {
    panels_.push_back(std::move(panel));
}

auto Editor::draw_ui(be::EngineContext const& ctx) -> void {
    if (!initialized_) {
        return;
    }

    // ═══════════════════════════════════════════════
    // Phase 1: Overlays (menus) — drawn before dockspace
    // ═══════════════════════════════════════════════
    for (auto& menu : menus_) {
        menu->draw_ui(ctx);
    }

    // ═══════════════════════════════════════════════
    // Phase 2: Dockspace + default layout
    // ═══════════════════════════════════════════════
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    static bool first_layout = true;
    if (first_layout) {
        first_layout = false;

        // Check if a saved layout already exists (ini loaded by ImGui on NewFrame)
        ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace_id);
        // If the node has no children, no layout was loaded -> create default
        if (node && node->ChildNodes[0] == nullptr && node->ChildNodes[1] == nullptr) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

            // Split: right 25% for Properties
            ImGuiID dock_right;
            ImGuiID dock_main = dockspace_id;
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right, &dock_main);

            // Split bottom 25% (under center + right)
            ImGuiID dock_bottom;
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_main);

            // Split bottom area: left half for Console+Project (tabs), right half for Assets
            ImGuiID dock_bottom_left;
            ImGuiID dock_bottom_right;
            ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.5f, &dock_bottom_left, &dock_bottom_right);

            // Dock windows
            ImGui::DockBuilderDockWindow("Scene", dock_main);
            ImGui::DockBuilderDockWindow("Properties", dock_right);
            ImGui::DockBuilderDockWindow("Console", dock_bottom_left);
            ImGui::DockBuilderDockWindow("Project", dock_bottom_left);
            ImGui::DockBuilderDockWindow("Assets", dock_bottom_right);

            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    // ═══════════════════════════════════════════════
    // Phase 3: Dockable panels (inside dockspace)
    // ═══════════════════════════════════════════════
    for (auto& panel : panels_) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin(panel->title().data());
        panel->draw_ui(ctx);
        ImGui::End();
    }

    // ═══════════════════════════════════════════════
    // Phase 4: About popup (rendered every frame if show_about_ is true)
    // ═══════════════════════════════════════════════
    draw_about_popup(ctx);
}

auto Editor::draw_about_popup(be::EngineContext const& /*ctx*/) -> void {
    if (!show_about_) {
        return;
    }

    ImGui::OpenPopup("About Buddd Editor");

    // Modal popup
    if (ImGui::BeginPopupModal("About Buddd Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Buddd Engine v%s", be::version().data());

        ImGui::Separator();

        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
            show_about_ = false;
        }

        ImGui::EndPopup();
    } else {
        // Popup was dismissed by Escape or click-outside
        show_about_ = false;
    }
}

auto Editor::shutdown() -> void {
    initialized_ = false;
    engine_ = nullptr;
    window_ = nullptr;
}

} // namespace buddd::editor
