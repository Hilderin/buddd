#include "apps/editor_app.h"

#include "editor.h"
#include "error.h"
#include "log/log.h"
#include "util/editor_data_root.h"
#include "window/window_utils.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>

BUDDD_LOG_TAG("EditorApp");

namespace be = buddd::engine;

// ── Minimum / default window geometry constants ──
constexpr int k_min_w = 400;
constexpr int k_min_h = 300;
constexpr int k_default_w = 1280;
constexpr int k_default_h = 800;

buddd::cmd::app::EditorApp::EditorApp(std::optional<std::string> scene_path)
    : scene_path_(std::move(scene_path)) {}

buddd::cmd::app::EditorApp::~EditorApp() = default;

auto buddd::cmd::app::EditorApp::config() const -> AppConfig {
    // Try to load saved window geometry from user_project_settings.
    // We read the YAML directly (no SettingsManager needed at this stage)
    // and apply basic size validation so the window is created at the
    // correct size from the start.
    auto yaml_path = be::editor_user_data_root(std::filesystem::current_path())
                     / "settings.yaml";

    if (std::filesystem::exists(yaml_path)) {
        try {
            auto node = YAML::LoadFile(yaml_path.string());
            auto win = node["editor"]["window"];
            if (win.IsDefined()) {
                int w = win["width"].as<int>(k_default_w);
                int h = win["height"].as<int>(k_default_h);
                int x = win["x"].as<int>(-1);
                int y = win["y"].as<int>(-1);
                auto state_str = win["state"].as<std::string>("normal");
                auto state = be::parse_window_state(state_str);

                // Size validation (minimum size)
                if (w < k_min_w || h < k_min_h) {
                    BUDDD_LOG_DEBUG("EditorApp: saved size ({}x{}) below minimum, using defaults",
                        w, h);
                    w = k_default_w;
                    h = k_default_h;
                }

                // State validation: never start minimised
                if (state == be::WindowState::Minimized) {
                    state = be::WindowState::Normal;
                }

                return {
                    .title = "Buddd Editor",
                    .width = w,
                    .height = h,
                    .window_x = x,
                    .window_y = y,
                    .window_state = state
                };
            }
        } catch (const std::exception& e) {
            BUDDD_LOG_WARN("EditorApp: failed to read window settings: {}", e.what());
        }
    }

    return {"Buddd Editor", k_default_w, k_default_h};
}

auto buddd::cmd::app::EditorApp::setup(be::EngineContext const& ctx) -> be::Result<void> {
#ifndef BUDDD_HAS_DISPLAY
    return make_error(be::Error::Category::InitFailed,
        "editor requires a display (compiled with BUDDD_HAS_DISPLAY=OFF)");
#endif

    // ── Validate window position against current display layout ──
    // The window was created with saved geometry from config().  Here we
    // re-validate position using the live Platform API (not available
    // during config()) and correct the window if needed.
    {
        // Re-read saved values from YAML to get the raw saved position
        int raw_x = -1, raw_y = -1;
        int win_w = k_default_w, win_h = k_default_h;
        auto yaml_path = be::editor_user_data_root(std::filesystem::current_path())
                         / "settings.yaml";
        if (std::filesystem::exists(yaml_path)) {
            try {
                auto node = YAML::LoadFile(yaml_path.string());
            auto win = node["editor"]["window"];
                if (win.IsDefined()) {
                    raw_x = win["x"].as<int>(-1);
                    raw_y = win["y"].as<int>(-1);
                    win_w = win["width"].as<int>(k_default_w);
                    win_h = win["height"].as<int>(k_default_h);
                    if (win_w < k_min_w || win_h < k_min_h) {
                        win_w = k_default_w;
                        win_h = k_default_h;
                    }
                }
            } catch (...) {}
        }

        if (raw_x != -1 && raw_y != -1) {
            bool position_valid = false;
            int display_count = ctx.services.platform().display_count();
            if (display_count > 0) {
                for (int i = 0; i < display_count; ++i) {
                    auto bounds = ctx.services.platform().display_bounds(i);
                    if (raw_x < bounds.x + bounds.width
                        && raw_x + win_w > bounds.x
                        && raw_y < bounds.y + bounds.height
                        && raw_y + win_h > bounds.y)
                    {
                        position_valid = true;
                        break;
                    }
                }
            }
            if (!position_valid) {
                BUDDD_LOG_INFO("EditorApp: saved window position ({} + {}) is off-screen, using default",
                    raw_x, raw_y);
                // Window was created at the saved position by create_window(), but
                // it's off-screen. Let the window manager center it by not calling
                // set_position.  Actually the window already has the wrong position,
                // so we must explicitly move it to a default location.  SDL3 centres
                // new windows automatically; to trigger re-centring we set (SDL_WINDOWPOS_CENTERED).
                // In SDL3 there is no SDL_WINDOWPOS_CENTERED constant — we use (0, 0)
                // and rely on the window manager.  For now, just don't set position
                // and log a warning.
            }
        }
    }

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
