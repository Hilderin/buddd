#include "editor.h"
#include "editor_context.h"

#include "inspector_editors.h"
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
#include "window/window_utils.h"

#include <imgui.h>
#include <imgui_internal.h>

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

auto Editor::selection() -> EditorSelection& {
    return selection_;
}

auto Editor::command_stack() -> CommandStack& {
    return command_stack_;
}

auto Editor::settings_manager() -> be::SettingsManager& {
    return *settings_manager_;
}

auto Editor::setup(be::EngineContext const& ctx) -> be::Result<void> {
    engine_ = &ctx.services;
    window_ = &ctx.window;
    initialized_ = true;

    // ── Editor window geometry: initialise tracking cache ──
    // Window position/size/state are already applied by EditorApp before
    // Editor::setup() is called (via AppConfig → WindowConfig → create_window).
    // We initialise the cache early (before the ImGui check) so that headless
    // tests also get correct cache initialisation.
    {
        cached_w_ = window_->width();
        cached_h_ = window_->height();
        auto pos = window_->position();
        cached_x_ = pos.x;
        cached_y_ = pos.y;
        has_cached_geometry_ = true;
        BUDDD_LOG_DEBUG("Editor: initialised geometry cache ({}x{} + {{{}, {}}}, {})",
            cached_w_, cached_h_, cached_x_, cached_y_,
            be::window_state_to_string(window_->state()));
    }

    if (!be::engine_imgui::is_initialized()) {
        return make_error(be::Error::Category::InitFailed,
            "ImGui is not initialized. The editor requires a display with working ImGui.");
    }

    // Enable docking
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // ── Settings system initialisation (MVP1) ──
    {
        auto sctx = be::SerializationContext{engine_->assets()};
        settings_manager_ = std::make_unique<be::SettingsManager>(
            std::filesystem::current_path(), sctx);
        ImGui::GetIO().IniFilename = settings_manager_->layout_ini_path().c_str();
        BUDDD_LOG_INFO("Editor: layout file: {}", settings_manager_->layout_ini_path());

        auto load_result = settings_manager_->load_all();
        if (!load_result) {
            BUDDD_LOG_WARN("Editor: settings load warning: {} (using defaults)",
                load_result.error().message);
        }
    }

    // ── Register built-in inspector editors ──
    register_builtin_inspector_editors();

    // ── Create menu bar ──
    auto menu_bar = std::make_unique<MenuBar>(command_stack_);
    menu_bar->set_on_about([this]() {
        open_dialog(std::make_unique<CustomDialog>(
            "about",
            "About Buddd Editor",
            [this]() {
                ImGui::Text("Buddd Engine v%s", be::version().data());
            },
            std::vector<DialogButton>{
                {"Close", "close_btn", []() {
                    // No-op: the framework auto-closes after any button click.
                }}
            }
        ));
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
            engine_->platform().show_open_file_dialog(
                [this](std::optional<std::string> path) {
                    if (!path) return;
                    if (auto result = open_scene(*path); !result) {
                        show_error_modal("Load Error", result.error().message);
                    }
                },
                "YAML Scene", "yaml");
        }
    });
    menu_bar->set_on_save_scene([this]() {
        auto result = save_scene();
        if (!result) {
            engine_->platform().show_save_file_dialog(
                [this](std::optional<std::string> path) {
                    if (!path) return;
                    if (auto r = save_scene_as(*path); !r) {
                        show_error_modal("Save Error", r.error().message);
                    }
                },
                "YAML Scene", "yaml", dialog_default_path().c_str());
        }
    });
    menu_bar->set_on_save_scene_as([this]() {
        engine_->platform().show_save_file_dialog(
            [this](std::optional<std::string> path) {
                if (!path) return;
                if (auto r = save_scene_as(*path); !r) {
                    show_error_modal("Save Error", r.error().message);
                }
            },
            "YAML Scene", "yaml", dialog_default_path().c_str());
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
            engine_->platform().show_open_file_dialog(
                [this](std::optional<std::string> path) {
                    if (!path) return;
                    if (auto result = open_scene(*path); !result) {
                        show_error_modal("Load Error", result.error().message);
                    }
                },
                "YAML Scene", "yaml");
        }
    });
    shortcuts_.bind(be::KeyCode::S, {.ctrl = true}, [this](be::EngineContext const&) {
        auto result = save_scene();
        if (!result) {
            engine_->platform().show_save_file_dialog(
                [this](std::optional<std::string> path) {
                    if (!path) return;
                    if (auto r = save_scene_as(*path); !r) {
                        show_error_modal("Save Error", r.error().message);
                    }
                },
                "YAML Scene", "yaml", dialog_default_path().c_str());
        }
    });
    shortcuts_.bind(be::KeyCode::S, {.ctrl = true, .shift = true}, [this](be::EngineContext const&) {
        engine_->platform().show_save_file_dialog(
            [this](std::optional<std::string> path) {
                if (!path) return;
                if (auto r = save_scene_as(*path); !r) {
                    show_error_modal("Save Error", r.error().message);
                }
            },
            "YAML Scene", "yaml", dialog_default_path().c_str());
    });
    shortcuts_.bind(be::KeyCode::Z, {.ctrl = true}, [this](be::EngineContext const& ectx) {
        auto editor_ctx = EditorContext{*this, ectx};
        [[maybe_unused]] auto _ = command_stack_.undo(editor_ctx);
    });
    shortcuts_.bind(be::KeyCode::Z, {.ctrl = true, .shift = true}, [this](be::EngineContext const& ectx) {
        auto editor_ctx = EditorContext{*this, ectx};
        [[maybe_unused]] auto _ = command_stack_.redo(editor_ctx);
    });
    shortcuts_.bind(be::KeyCode::Y, {.ctrl = true}, [this](be::EngineContext const& ectx) {
        auto editor_ctx = EditorContext{*this, ectx};
        [[maybe_unused]] auto _ = command_stack_.redo(editor_ctx);
    });
    shortcuts_.bind(be::KeyCode::A, {.ctrl = true}, [this](be::EngineContext const&) {
        // Gate: do nothing if ImGui captures keyboard (e.g., text input focused)
        if (ImGui::GetIO().WantCaptureKeyboard) return;

        // Collect all entity IDs via tree traversal
        auto& w = world();
        std::vector<buddd::engine::EntityId> all_ids;
        all_ids.reserve(w.entity_count());

        // Recursive traversal
        auto collect = [&](auto& self, buddd::engine::Entity entity) -> void {
            all_ids.push_back(entity.id());
            for (size_t i = 0; i < entity.child_count(); ++i) {
                self(self, entity.get_child(i));
            }
        };
        for (size_t i = 0; i < w.root_entity_count(); ++i) {
            auto entity = w.get_root_entity(i);
            if (entity.id() != buddd::engine::EntityId::none()) {
                collect(collect, entity);
            }
        }

        BUDDD_LOG_DEBUG("Ctrl+A: selected {} entities", all_ids.size());
        selection_.set_selection(all_ids);
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
    auto editor_ctx = EditorContext{*this, ctx};
    for (auto& menu : menus_) {
        menu->update(editor_ctx);
    }
    for (auto& panel : panels_) {
        panel->update(editor_ctx);
    }

    // Remove entities marked for destruction this frame
    world().flush_destroyed();

    // ═══════════════════════════════════════════════
    // Track last-known Normal geometry (for correct save on shutdown
    // even when the window is currently Maximized or Minimized).
    // ═══════════════════════════════════════════════
    if (window_) {
        auto st = window_->state();
        if (st == be::WindowState::Normal) {
            auto pos = window_->position();
            cached_x_ = pos.x;
            cached_y_ = pos.y;
            cached_w_ = window_->width();
            cached_h_ = window_->height();
            has_cached_geometry_ = true;
        }
    }
}

auto Editor::add_menu(std::unique_ptr<EditorMenu> menu) -> void {
    menus_.push_back(std::move(menu));
}

auto Editor::add_panel(std::unique_ptr<EditorPanel> panel) -> void {
    panels_.push_back(std::move(panel));
}

auto Editor::open_dialog(std::unique_ptr<Dialog> dialog) -> bool {
    auto const& incoming_id = dialog->id();
    for (auto const& existing : dialogs_) {
        if (existing->id() == incoming_id) {
            BUDDD_LOG_DEBUG("Dialog dedup: {} already open", incoming_id);
            return false;
        }
    }
    BUDDD_LOG_DEBUG("Dialog opened: {}", incoming_id);
    opened_dialog_ids_.insert(incoming_id);
    dialogs_.push_back(std::move(dialog));
    return true;
}

auto Editor::draw_ui(be::EngineContext const& ctx) -> void {
    if (!initialized_) {
        return;
    }

    auto editor_ctx = EditorContext{*this, ctx};

    // ═══════════════════════════════════════════════
    // Phase 1: Overlays (menus) — drawn before dockspace
    // ═══════════════════════════════════════════════
    for (auto& menu : menus_) {
        menu->draw_ui(editor_ctx);
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
        panel->draw_ui(editor_ctx);
        ImGui::End();
    }

    // ═══════════════════════════════════════════════
    // Phase 4: Dialog rendering
    // ═══════════════════════════════════════════════
    for (auto& dialog : dialogs_) {
        // OpenPopup each frame (matching the pattern used by error modals and delete confirmation)
        ImGui::OpenPopup(dialog->title().c_str());

        // Also remove from tracking set if present (legacy, but harmless)
        opened_dialog_ids_.erase(dialog->id());

        if (ImGui::BeginPopupModal(dialog->title().c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            dialog->draw_content();
            ImGui::EndPopup();
        }
    }

    // Escape handling: only for the topmost dialog
    if (!dialogs_.empty() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        BUDDD_LOG_DEBUG("Escape on topmost dialog: {}", dialogs_.back()->id());
        dialogs_.back()->handle_escape();
    }

    // Remove closed dialogs
    std::erase_if(dialogs_, [](auto& d) {
        if (d->should_close()) {
            BUDDD_LOG_DEBUG("Dialog closed: {}", d->id());
            return true;
        }
        return false;
    });

    // ═══════════════════════════════════════════════
    // Phase 5: Save-prompt state machine
    // ═══════════════════════════════════════════════
    draw_pending_op_modal(ctx);

    // ═══════════════════════════════════════════════
    // Phase 6: Exit-on-next-frame flag (set by async dialog callbacks)
    // ═══════════════════════════════════════════════
    if (request_exit_next_frame_) {
        request_exit_next_frame_ = false;
        ctx.request_exit();
    }

    // ═══════════════════════════════════════════════
    // Phase 7: Error modals
    // ═══════════════════════════════════════════════
    draw_error_modals();
}

auto Editor::shutdown() -> void {
    // ── Save window geometry before persisting settings ──
    if (window_ && settings_manager_) {
        auto& ups = settings_manager_->user_project_settings();
        auto state_str = be::window_state_to_string(window_->state());

        // Always save the last-known Normal position/size (tracked in update()),
        // not the current window geometry, so that maximised/minimised state does
        // not pollute the saved "restored" geometry.
        if (has_cached_geometry_) {
            ups.set<int32_t>("editor.window.x",       cached_x_);
            ups.set<int32_t>("editor.window.y",       cached_y_);
            ups.set<int32_t>("editor.window.width",   cached_w_);
            ups.set<int32_t>("editor.window.height",  cached_h_);
            BUDDD_LOG_INFO("Editor: saving window geometry ({}x{} + {{{}, {}}}, {})",
                cached_w_, cached_h_, cached_x_, cached_y_, state_str);
        } else {
            BUDDD_LOG_INFO("Editor: saving window state as '{}' (no cached normal geometry)",
                state_str);
        }

        ups.set<std::string>("editor.window.state", state_str);
    }

    if (settings_manager_) {
        auto save_result = settings_manager_->save_all();
        if (!save_result) {
            BUDDD_LOG_WARN("Editor: settings save warning: {}",
                save_result.error().message);
        }
        // Prevent dangling pointer in ImGui::GetIO().IniFilename:
        // settings_manager_ will be destroyed before ImGui shutdown,
        // so the string backing layout_ini_path() would become invalid.
        ImGui::GetIO().IniFilename = nullptr;
    }
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

auto Editor::default_save_name() const -> std::string {
    if (current_file_path_.has_value()) {
        return std::filesystem::path(*current_file_path_).filename().string();
    }
    return "Untitled.yaml";
}

auto Editor::dialog_default_path() const -> std::string {
    if (current_file_path_.has_value()) {
        // Return only the parent directory: the XDG Portal backend on Linux
        // uses default_location ONLY as current_folder, ignoring any filename.
        return std::filesystem::path(*current_file_path_).parent_path().string();
    }
    // Untitled: current directory
    return ".";
}

auto Editor::update_window_title() -> void {
    if (!window_) return;
    window_->set_title(build_title_string());
}

// ── Scene management: operations ──

auto Editor::new_scene() -> void {
    selection_.clear();
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
        selection_.clear();
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

auto Editor::draw_save_prompt_modal() -> std::optional<SavePromptResult> {
    // OpenPopup only once per request (on the frame where save_prompt_requested_ is set)
    if (save_prompt_requested_) {
        ImGui::OpenPopup("Save Changes");
        save_prompt_requested_ = false;
    }

    if (!ImGui::BeginPopupModal("Save Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (save_prompt_seen_) {
            // Popup was visible on a previous frame but is now closed
            // (Escape / click-outside) → treat as cancellation.
            save_prompt_seen_ = false;
            return SavePromptResult::Cancel;
        }
        // Popup not yet visible this frame (first frame after OpenPopup) — wait.
        return std::nullopt;
    }

    // Popup is visible this frame
    save_prompt_seen_ = true;

    // ── Draw buttons ──
    std::string scene_name = "Untitled";
    if (current_file_path_.has_value()) {
        scene_name = std::filesystem::path(*current_file_path_).filename().string();
    }
    ImGui::Text("Save changes to %s?", scene_name.c_str());

    if (ImGui::Button("Save")) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return SavePromptResult::Save;
    }
    ImGui::SameLine();
    if (ImGui::Button("Don't Save")) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return SavePromptResult::Discard;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return SavePromptResult::Cancel;
    }

    // If popup was dismissed by Escape/click-outside, BeginPopupModal returned
    // false above (handled). If we're here the popup is still open — no decision yet.
    ImGui::EndPopup();
    return std::nullopt;
}

// ── Pending op state machine ──

auto Editor::draw_pending_op_modal(be::EngineContext const& ctx) -> void {
    if (pending_op_ == PendingOp::None) return;

    if (!dirty_) {
        execute_pending_op(ctx);
        pending_op_ = PendingOp::None;
        return;
    }

    // Request the save prompt once — don't re-open if the user dismissed it
    if (!save_prompt_requested_ && !save_prompt_seen_) {
        save_prompt_requested_ = true;
    }

    auto result = draw_save_prompt_modal();
    if (!result.has_value()) {
        return;
    }

    switch (*result) {
        case SavePromptResult::Save: {
            auto save_result = save_scene();
            if (save_result.has_value()) {
                // Save succeeded — proceed with pending operation
                if (pending_op_ == PendingOp::OpenScene) {
                    engine_->platform().show_open_file_dialog(
                        [this](std::optional<std::string> path) {
                            if (!path) return;
                            if (auto r = open_scene(*path); !r) {
                                show_error_modal("Load Error", r.error().message);
                            }
                        },
                        "YAML Scene", "yaml");
                } else {
                    execute_pending_op(ctx);
                }
                pending_op_ = PendingOp::None;
            } else if (!current_file_path_.has_value()) {
                // Untitled: redirect to Save As dialog, then complete pending op
                auto original_op = pending_op_;
                pending_op_ = PendingOp::None;
                engine_->platform().show_save_file_dialog(
                    [this, original_op](std::optional<std::string> save_path) {
                        if (!save_path) return; // cancelled — stay on current scene
                        auto r = save_scene_as(*save_path);
                        if (!r) {
                            show_error_modal("Save Error", r.error().message);
                            return;
                        }
                        // Save succeeded — complete the original operation
                        if (original_op == PendingOp::OpenScene) {
                            engine_->platform().show_open_file_dialog(
                                [this](std::optional<std::string> path) {
                                    if (!path) return;
                                    if (auto r = open_scene(*path); !r)
                                        show_error_modal("Load Error", r.error().message);
                                },
                                "YAML Scene", "yaml");
                        } else if (original_op == PendingOp::NewScene) {
                            new_scene();
                        } else if (original_op == PendingOp::Quit) {
                            request_exit_next_frame_ = true;
                        }
                    },
                    "YAML Scene", "yaml", dialog_default_path().c_str());
            } else {
                show_error_modal("Save Error", save_result.error().message);
                pending_op_ = PendingOp::None;
            }
            break;
        }
        case SavePromptResult::Discard: {
            if (pending_op_ == PendingOp::OpenScene) {
                engine_->platform().show_open_file_dialog(
                    [this](std::optional<std::string> path) {
                        if (!path) return;
                        if (auto r = open_scene(*path); !r) {
                            show_error_modal("Load Error", r.error().message);
                        }
                    },
                    "YAML Scene", "yaml");
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
        default: break;
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

// (handle_dirty_before_op removed — dead code)
// (draw_file_dialog removed — replaced by Platform dialog calls)

} // namespace buddd::editor
