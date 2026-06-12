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
#include "scene/scene_loader.h"
#include "scene/scene_saver.h"
#include "imgui/engine_imgui.h"
#include "input/key_code.h"
#include "input/input_system.h"
#include "log/log.h"
#include "platform/platform.h"
#include "version.h"
#include "window/window.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <ImGuiFileDialog.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
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
    menu_bar->set_on_new_scene([this]() {
        if (dirty_) {
            pending_op_ = PendingOp::NewScene;
        } else {
            new_scene();
        }
    });
    menu_bar->set_on_open_scene([this]() {
        if (dirty_) {
            pending_op_ = PendingOp::OpenScene;
        } else {
            show_file_dialog_ = true;
            file_dialog_action_ = "Open";
        }
    });
    menu_bar->set_on_save_scene([this]() {
        auto result = save_scene();
        if (!result) {
            show_file_dialog_ = true;
            file_dialog_action_ = "SaveAs";
        }
    });
    menu_bar->set_on_save_scene_as([this]() {
        show_file_dialog_ = true;
        file_dialog_action_ = "SaveAs";
    });
    menu_bar->set_on_quit([this](be::EngineContext const& ctx) {
        if (dirty_) {
            pending_op_ = PendingOp::Quit;
        } else {
            ctx.request_exit();
        }
    });
    add_menu(std::move(menu_bar));

    // ── Register panels ──
    add_panel(std::make_unique<ScenePanel>());
    add_panel(std::make_unique<PropertiesPanel>());
    add_panel(std::make_unique<ConsolePanel>());
    add_panel(std::make_unique<ProjectPanel>());
    add_panel(std::make_unique<AssetsPanel>());

    // ── Register shortcuts ──
    shortcuts_.bind(be::KeyCode::Q, {.ctrl = true}, [this](be::EngineContext const& ctx) {
        if (dirty_) {
            pending_op_ = PendingOp::Quit;
        } else {
            ctx.request_exit();
        }
    });
    shortcuts_.bind(be::KeyCode::N, {.ctrl = true}, [this](be::EngineContext const&) {
        if (dirty_) {
            pending_op_ = PendingOp::NewScene;
        } else {
            new_scene();
        }
    });
    shortcuts_.bind(be::KeyCode::O, {.ctrl = true}, [this](be::EngineContext const&) {
        if (dirty_) {
            pending_op_ = PendingOp::OpenScene;
        } else {
            show_file_dialog_ = true;
            file_dialog_action_ = "Open";
        }
    });
    shortcuts_.bind(be::KeyCode::S, {.ctrl = true}, [this](be::EngineContext const&) {
        auto result = save_scene();
        if (!result) {
            show_file_dialog_ = true;
            file_dialog_action_ = "SaveAs";
        }
    });
    shortcuts_.bind(be::KeyCode::S, {.ctrl = true, .shift = true}, [this](be::EngineContext const&) {
        show_file_dialog_ = true;
        file_dialog_action_ = "SaveAs";
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

    // ── Set initial window title ──
    update_window_title();

    // ── Register close-request handler for OS close button ──
    ctx.services.platform().set_on_close_request([this]() -> bool {
        if (!dirty_) {
            return true;  // clean scene: allow close
        }
        pending_op_ = PendingOp::Quit;
        return false;  // cancel close — will re-request exit after user resolves
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

    // ═══════════════════════════════════════════════
    // Phase 5: Save-prompt state machine
    // ═══════════════════════════════════════════════
    draw_pending_op_modal(ctx);

    // ═══════════════════════════════════════════════
    // Phase 6: File dialog (ImGuiFileDialog)
    // ═══════════════════════════════════════════════
    draw_file_dialog();

    // ═══════════════════════════════════════════════
    // Phase 7: Error modals
    // ═══════════════════════════════════════════════
    draw_error_modals();
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

// ── Scene management: dirty state ──

auto Editor::mark_dirty() -> void {
    dirty_ = true;
    update_window_title();
    BUDDD_LOG_DEBUG("Scene marked dirty");
}

auto Editor::clear_dirty() -> void {
    dirty_ = false;
    update_window_title();
    BUDDD_LOG_DEBUG("Scene dirty cleared");
}

auto Editor::is_dirty() const noexcept -> bool {
    return dirty_;
}

auto Editor::current_file_path() const noexcept -> const std::optional<std::string>& {
    return current_file_path_;
}

// ── Window title ──

auto Editor::build_title_string() const -> std::string {
    std::string filename;
    if (current_file_path_.has_value()) {
        filename = std::filesystem::path(*current_file_path_).filename().string();
    } else {
        filename = "Untitled";
    }
    if (dirty_) {
        filename += '*';
    }
    return filename + " \u2014 Buddd Editor";
}

auto Editor::update_window_title() -> void {
    if (!window_) return;
    window_->set_title(build_title_string());
}

// ── Scene management: operations ──

auto Editor::new_scene() -> void {
    world_ = std::make_unique<be::World>();
    current_file_path_ = std::nullopt;
    dirty_ = false;
    update_window_title();
    BUDDD_LOG_INFO("New scene created");
}

auto Editor::open_scene(const std::string& path) -> be::Result<void> {
    if (!engine_) {
        return make_error(be::Error::Category::InvalidArgument,
            "Editor not initialized");
    }

    // Save current World aside so we can restore on failure
    auto saved_world = std::move(world_);
    world_ = std::make_unique<be::World>();

    auto& registry = engine_->registry();
    auto& assets = engine_->assets();
    be::SceneLoader loader(*world_, registry, assets);
    auto result = loader.load_from_file(path);

    if (result.has_value()) {
        current_file_path_ = path;
        dirty_ = false;
        update_window_title();
        BUDDD_LOG_INFO("Scene loaded: {}", path);
        return {};
    } else {
        // Restore the saved World on failure
        world_ = std::move(saved_world);
        BUDDD_LOG_WARN("Scene load failed: {}", result.error().message);
        return make_error(result.error());
    }
}

auto Editor::save_scene() -> be::Result<void> {
    if (!engine_) {
        return make_error(be::Error::Category::InvalidArgument,
            "Editor not initialized");
    }

    // Clean scene with a file path: no-op
    if (!dirty_ && current_file_path_.has_value()) {
        return {};
    }

    if (!current_file_path_.has_value()) {
        return make_error(be::Error::Category::InvalidArgument,
            "No file path set \u2014 use save_scene_as instead");
    }

    auto& registry = engine_->registry();
    auto& assets = engine_->assets();
    be::SceneSaver saver(*world_, registry, assets);
    auto result = saver.save_to_file(*current_file_path_);

    if (result.has_value()) {
        dirty_ = false;
        update_window_title();
        BUDDD_LOG_INFO("Scene saved: {}", *current_file_path_);
        return {};
    } else {
        BUDDD_LOG_WARN("Scene save failed: {}", result.error().message);
        return make_error(result.error());
    }
}

auto Editor::save_scene_as(const std::string& path) -> be::Result<void> {
    if (!engine_) {
        return make_error(be::Error::Category::InvalidArgument,
            "Editor not initialized");
    }

    auto& registry = engine_->registry();
    auto& assets = engine_->assets();
    be::SceneSaver saver(*world_, registry, assets);
    auto result = saver.save_to_file(path);

    if (result.has_value()) {
        current_file_path_ = path;
        dirty_ = false;
        update_window_title();
        BUDDD_LOG_INFO("Scene saved: {}", path);
        return {};
    } else {
        BUDDD_LOG_WARN("Scene save failed: {}", result.error().message);
        return make_error(result.error());
    }
}

// ── Error modals ──

auto Editor::show_error_modal(const std::string& title, const std::string& message) -> void {
    error_modal_title_ = title;
    error_modal_message_ = message;
    show_error_modal_ = true;
}

auto Editor::draw_error_modals() -> void {
    if (!show_error_modal_) return;

    ImGui::OpenPopup(error_modal_title_.c_str());
    if (ImGui::BeginPopupModal(error_modal_title_.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", error_modal_message_.c_str());
        if (ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
            show_error_modal_ = false;
        }
        ImGui::EndPopup();
    } else {
        // Dismissed by Escape or click-outside
        show_error_modal_ = false;
    }
}

// ── Save-prompt modal ──

auto Editor::draw_save_prompt_modal() -> SavePromptResult {
    SavePromptResult result = SavePromptResult::Cancel;

    ImGui::OpenPopup("Save Changes");
    if (ImGui::BeginPopupModal("Save Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string scene_name = "Untitled";
        if (current_file_path_.has_value()) {
            scene_name = std::filesystem::path(*current_file_path_).filename().string();
        }
        ImGui::Text("Save changes to %s?", scene_name.c_str());

        if (ImGui::Button("Save")) {
            ImGui::CloseCurrentPopup();
            result = SavePromptResult::Save;
        }
        ImGui::SameLine();
        if (ImGui::Button("Don't Save")) {
            ImGui::CloseCurrentPopup();
            result = SavePromptResult::Discard;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            result = SavePromptResult::Cancel;
        }

        ImGui::EndPopup();
    } else {
        // Dismissed by Escape or click-outside → Cancel
        result = SavePromptResult::Cancel;
    }

    return result;
}

// ── Pending op state machine ──

auto Editor::draw_pending_op_modal(be::EngineContext const& ctx) -> void {
    if (pending_op_ == PendingOp::None) {
        return;
    }

    if (!dirty_) {
        // Clean scene — execute pending op immediately
        execute_pending_op(ctx);
        pending_op_ = PendingOp::None;
        return;
    }

    // Dirty scene — show save-prompt modal
    auto result = draw_save_prompt_modal();
    switch (result) {
        case SavePromptResult::Save: {
            auto save_result = save_scene();
            if (save_result.has_value()) {
                // Save succeeded — proceed
                if (pending_op_ == PendingOp::OpenScene) {
                    show_file_dialog_ = true;
                    file_dialog_action_ = "Open";
                } else {
                    execute_pending_op(ctx);
                }
                pending_op_ = PendingOp::None;
            } else if (!current_file_path_.has_value()) {
                // Untitled: redirect to Save As dialog
                show_file_dialog_ = true;
                file_dialog_action_ = "SaveAs";
                pending_op_ = PendingOp::None;
            } else {
                // Save failed (disk full, permissions, etc.)
                show_error_modal("Save Error", save_result.error().message);
                pending_op_ = PendingOp::None;
            }
            break;
        }
        case SavePromptResult::Discard: {
            if (pending_op_ == PendingOp::OpenScene) {
                show_file_dialog_ = true;
                file_dialog_action_ = "Open";
            } else {
                execute_pending_op(ctx);
            }
            pending_op_ = PendingOp::None;
            break;
        }
        case SavePromptResult::Cancel: {
            pending_op_ = PendingOp::None;
            BUDDD_LOG_INFO("Save prompt cancelled");
            break;
        }
    }
}

auto Editor::execute_pending_op(be::EngineContext const& ctx) -> void {
    switch (pending_op_) {
        case PendingOp::NewScene:
            new_scene();
            break;
        case PendingOp::OpenScene:
            if (pending_file_path_.has_value()) {
                auto result = open_scene(pending_file_path_.value());
                if (!result) {
                    show_error_modal("Load Error", result.error().message);
                }
            }
            break;
        case PendingOp::Quit:
            ctx.request_exit();
            break;
        case PendingOp::None:
            break;
    }
}

auto Editor::handle_dirty_before_op(be::EngineContext const& ctx, PendingOp op) -> bool {
    if (!dirty_) {
        return true;
    }
    pending_op_ = op;
    return false;
}

// ── File dialog ──

auto Editor::draw_file_dialog() -> void {
    // Open the dialog on the frame where show_file_dialog_ is first set
    if (show_file_dialog_) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        config.filePathName = "";
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_None;
        ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
            "Choose File", "\\.yaml", config);
        show_file_dialog_ = false;  // Reset — dialog is now managed by ImGuiFileDialog
    }

    // Display the dialog every frame while it is open
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();
            if (!file_path.empty()) {
                if (file_dialog_action_ == "Open") {
                    auto result = open_scene(file_path);
                    if (!result) {
                        show_error_modal("Load Error", result.error().message);
                    }
                } else if (file_dialog_action_ == "SaveAs") {
                    auto result = save_scene_as(file_path);
                    if (!result) {
                        show_error_modal("Save Error", result.error().message);
                    }
                }
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

} // namespace buddd::editor
