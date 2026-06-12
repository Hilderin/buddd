#pragma once

#include "error.h"

#include "command_stack.h"
#include "editor_menu.h"
#include "editor_panel.h"
#include "scene/world.h"
#include "shortcut_registry.h"

#include <memory>
#include <vector>

namespace buddd::engine { struct EngineContext; class EngineService; class Window; }

namespace buddd::editor {

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

private:
    // ── About popup ──
    auto draw_about_popup(buddd::engine::EngineContext const& ctx) -> void;

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

    // Editor's own World (separate from ctx.world)
    std::unique_ptr<buddd::engine::World> world_;
};

} // namespace buddd::editor
