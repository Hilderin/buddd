#include "apps/editor_app.h"

#include "editor.h"
#include "error.h"

namespace be = buddd::engine;

buddd::cmd::app::EditorApp::EditorApp() = default;

buddd::cmd::app::EditorApp::~EditorApp() = default;

auto buddd::cmd::app::EditorApp::config() const -> AppConfig {
    return {"Buddd Editor", 1280, 800};
}

auto buddd::cmd::app::EditorApp::setup(be::EngineContext const& ctx) -> be::Result<void> {
#ifndef BUDDD_HAS_DISPLAY
    return make_error(be::Error::Category::InitFailed,
        "editor requires a display (compiled with BUDDD_HAS_DISPLAY=OFF)");
#endif

    editor_ = std::make_unique<buddd::editor::Editor>();
    return editor_->setup(ctx);
}

auto buddd::cmd::app::EditorApp::on_render(be::EngineContext const& ctx) -> void {
    if (editor_) {
        editor_->draw_ui(ctx);
    }
}

auto buddd::cmd::app::EditorApp::shutdown() -> void {
    if (editor_) {
        editor_->shutdown();
        editor_.reset();
    }
}
