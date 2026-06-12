#pragma once

#include "error.h"

#include "command_stack.h"
#include "editor_menu.h"
#include "editor_panel.h"
#include "scene/world.h"
#include "shortcut_registry.h"

#include <memory>
#include <optional>
#include <vector>

namespace buddd::engine { struct EngineContext; class EngineService; class Window; }

namespace buddd::editor {

/// Result of the save-prompt modal (Save / Don't Save / Cancel).
enum class SavePromptResult { Save, Discard, Cancel };

/// Pending operation that triggered a save-prompt (New, Open, Quit).
enum class PendingOp { None, NewScene, OpenScene, Quit };

/// Scaffold for the Buddd Editor.
/// Lifecycle: Editor() -> setup(ctx) -> update(ctx) x N -> draw_ui(ctx) x N -> shutdown().
class Editor {
public:
    Editor();
    ~Editor();

    /// Store engine service and window references for later use.
    /// Called from EditorApp::setup(). Returns error if ImGui is not initialized.
    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void>;

    /// Process editor logic: keyboard shortcuts, state updates.
    /// Called every frame from EditorApp::update(), after world->update_updatables().
    auto update(buddd::engine::EngineContext const& ctx) -> void;

    /// Register a menu overlay (takes ownership).
    auto add_menu(std::unique_ptr<EditorMenu> menu) -> void;

    /// Register a dockable panel (takes ownership).
    auto add_panel(std::unique_ptr<EditorPanel> panel) -> void;

    /// Draw the ImGui UI: menu bar, dockable panels, popups.
    /// Called every frame from EditorApp::on_render() with fresh per-frame context.
    /// No-op if setup() was not called.
    auto draw_ui(buddd::engine::EngineContext const& ctx) -> void;

    /// Cleanup. Called from EditorApp::shutdown().
    auto shutdown() -> void;

    /// Returns a reference to the editor's World.
    /// Always valid — created in the constructor, destroyed in the destructor.
    /// Safe to call at any point during the Editor's lifetime.
    [[nodiscard]] auto world() -> buddd::engine::World&;

    // ── Scene management operations ──

    /// Mark the current scene as having unsaved changes.
    auto mark_dirty() -> void;

    /// Clear the dirty flag (called after a successful save).
    auto clear_dirty() -> void;

    /// Returns true if the scene has unsaved changes.
    [[nodiscard]] auto is_dirty() const noexcept -> bool;

    /// Returns the current file path, or std::nullopt if untitled.
    [[nodiscard]] auto current_file_path() const noexcept -> const std::optional<std::string>&;

    /// Create a new untitled scene, replacing the current World.
    auto new_scene() -> void;

    /// Load a scene from the given file path into the Editor's World.
    [[nodiscard]] auto open_scene(const std::string& path) -> buddd::engine::Result<void>;

    /// Save the current scene to the current file path.
    /// If untitled (no file path), returns an error — caller must use save_scene_as() instead.
    [[nodiscard]] auto save_scene() -> buddd::engine::Result<void>;

    /// Save the current scene to the given file path.
    [[nodiscard]] auto save_scene_as(const std::string& path) -> buddd::engine::Result<void>;

    /// Returns the formatted window title string (e.g., "scene.yaml* — Buddd Editor").
    /// Public for testing.
    [[nodiscard]] auto build_title_string() const -> std::string;

private:

    // ── About popup ──
    auto draw_about_popup(buddd::engine::EngineContext const& ctx) -> void;

    // ── Scene management helpers ──
    auto update_window_title() -> void;
    auto draw_save_prompt_modal() -> SavePromptResult;
    auto show_error_modal(const std::string& title, const std::string& message) -> void;
    auto draw_error_modals() -> void;
    auto draw_pending_op_modal(buddd::engine::EngineContext const& ctx) -> void;
    auto draw_file_dialog() -> void;
    auto execute_pending_op(buddd::engine::EngineContext const& ctx) -> void;
    [[nodiscard]] auto handle_dirty_before_op(buddd::engine::EngineContext const& ctx, PendingOp op) -> bool;

    // ── State ──
    bool initialized_ = false;
    buddd::engine::EngineService* engine_ = nullptr;
    buddd::engine::Window* window_ = nullptr;

    // Command system
    CommandStack command_stack_;

    // Shortcut registry
    ShortcutRegistry shortcuts_;

    // Registered overlays (drawn before dockspace via menus_ iteration in draw_ui)
    std::vector<std::unique_ptr<EditorMenu>> menus_;

    // Registered panels (drawn inside dockspace via panels_ iteration in draw_ui)
    std::vector<std::unique_ptr<EditorPanel>> panels_;

    // Panel state flags
    bool show_about_ = false;

    // ── Scene management state ──
    bool dirty_ = false;
    std::optional<std::string> current_file_path_;

    // ── Pending operation (multi-frame save-prompt) ──
    PendingOp pending_op_ = PendingOp::None;
    std::optional<std::string> pending_file_path_;

    // ── File dialog state ──
    bool show_file_dialog_ = false;
    std::string file_dialog_action_;

    // ── Error modal state ──
    std::string error_modal_title_;
    std::string error_modal_message_;
    bool show_error_modal_ = false;

    // ── Save-prompt modal state ──
    bool show_save_prompt_modal_ = false;
    SavePromptResult save_prompt_result_ = SavePromptResult::Cancel;

    // Editor's own World (separate from ctx.world)
    std::unique_ptr<buddd::engine::World> world_;
};

} // namespace buddd::editor
