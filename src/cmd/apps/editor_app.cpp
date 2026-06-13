#include "apps/editor_app.h"

#include "editor.h"
#include "error.h"
#include "log/log.h"

BUDDD_LOG_TAG("EditorApp");

namespace be = buddd::engine;

buddd::cmd::app::EditorApp::EditorApp(std::optional<std::string> scene_path)
    : scene_path_(std::move(scene_path)) {}

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
    auto setup_result = editor_->setup(ctx);
    if (!setup_result)
        return setup_result;

    if (scene_path_) {
        BUDDD_LOG_INFO("Editor: opening scene: {}", *scene_path_);
        auto open_result = editor_->open_scene(*scene_path_);
        if (!open_result) {
            // Error is handled internally by Editor (error modal) — don't propagate
            BUDDD_LOG_WARN("Scene load failed: {}", be::to_string(open_result.error()));
        }
    }

    return setup_result;
}

auto buddd::cmd::app::EditorApp::on_render(be::EngineContext const& ctx) -> void {
    if (editor_) {
        editor_->draw_ui(ctx);
    }
}

auto buddd::cmd::app::EditorApp::update(be::EngineContext const& ctx) -> void {
    if (editor_) {
        editor_->update(ctx);
    }
}

auto buddd::cmd::app::EditorApp::shutdown() -> void {
    if (editor_) {
        editor_->shutdown();
        editor_.reset();
    }
}
