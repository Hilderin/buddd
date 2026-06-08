#pragma once

#include "error.h"

namespace buddd::engine { struct EngineContext; class EngineService; class Window; }

namespace buddd::editor {

/// Scaffold for the Buddd Editor.
/// Lifecycle: Editor() → setup(ctx) → draw_ui(ctx) x N → shutdown().
class Editor {
public:
    Editor();
    ~Editor();

    /// Store engine service and window references for later use.
    /// Called from EditorApp::setup(). Returns error if ImGui is not initialized.
    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void>;

    /// Draw the ImGui dockspace and any active editor panels.
    /// Called every frame from EditorApp::on_render() with fresh per-frame context.
    /// No-op if setup() was not called.
    auto draw_ui(buddd::engine::EngineContext const& ctx) -> void;

    /// Cleanup. Called from EditorApp::shutdown().
    auto shutdown() -> void;

private:
    bool initialized_ = false;
    buddd::engine::EngineService* engine_ = nullptr;
    buddd::engine::Window* window_ = nullptr;
};

} // namespace buddd::editor
