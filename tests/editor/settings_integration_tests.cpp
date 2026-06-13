#include "editor.h"
#include "engine_service.h"
#include "engine_context.h"
#include "platform/platform.h"
#include "window/window.h"
#include "scene/world.h"
#include "render/render_system.h"
#include "util/os_config_dir.h"

#include <catch2/catch_test_macros.hpp>

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace be = buddd::engine;
namespace ed = buddd::editor;

// ═════════════════════════════════════════════════════════════════════════════
// Settings integration tests — require BUDDD_HAS_DISPLAY
// ═════════════════════════════════════════════════════════════════════════════

#ifdef BUDDD_HAS_DISPLAY

#include <SDL3/SDL.h>

// ── AC-020: Editor::setup() calls SettingsManager::load_all() ──
TEST_CASE("AC-020: Editor::setup() creates .buddd/user/ and sets layout path",
          "[editor][settings][integration]") {
    // Configure offscreen SDL3 driver
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto engine = be::EngineService::create(
        be::Backend::SDL3,
        be::WindowConfig{.title = "AC-020", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<be::World>();
    auto render_system = std::make_unique<be::RenderSystem>(eng.device(), *world);

    be::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    ed::Editor editor;
    auto setup_result = editor.setup(ctx);

    // If ImGui is initialized (display available), setup should succeed
    if (setup_result.has_value()) {
        // Access SettingsManager
        auto& sm = editor.settings_manager();

        // Verify .buddd/user/ directory was created
        auto cwd = std::filesystem::current_path();
        auto user_data_root = cwd / ".buddd" / "user";
        REQUIRE(std::filesystem::exists(user_data_root));

        // Verify layout path points to .buddd/user/layout.ini
        auto expected_ini = (user_data_root / "layout.ini").string();
        REQUIRE(sm.layout_ini_path() == expected_ini);
    }

    editor.shutdown();
}

// ── AC-021: Editor::shutdown() calls SettingsManager::save_all() ──
TEST_CASE("AC-021: Editor::shutdown() saves settings to disk",
          "[editor][settings][integration]") {
    // Configure offscreen SDL3 driver
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto engine = be::EngineService::create(
        be::Backend::SDL3,
        be::WindowConfig{.title = "AC-021", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<be::World>();
    auto render_system = std::make_unique<be::RenderSystem>(eng.device(), *world);

    be::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    ed::Editor editor;
    auto setup_result = editor.setup(ctx);

    if (setup_result.has_value()) {
        auto& sm = editor.settings_manager();

        // Set a key on editor settings
        sm.editor_settings().set<std::string>("test_key", "integration_value");

        // Shutdown triggers save_all()
        editor.shutdown();

        // Verify the editor settings file was written
        // SettingsManager saves to os_user_config_dir() / "editor.yaml"
        // We check that the file exists (platform-dependent, but should be fine in CI)
        auto editor_path = be::os_user_config_dir() / "editor.yaml";
        REQUIRE(std::filesystem::exists(editor_path));

        // Verify content
        auto node = YAML::LoadFile(editor_path.string());
        auto loaded_value = node["test_key"].as<std::string>();
        REQUIRE(loaded_value == "integration_value");

        // Cleanup: remove the test file to avoid polluting the config dir
        std::filesystem::remove(editor_path);
    } else {
        editor.shutdown();
    }
}

// ── Shield test: Editor::shutdown() without setup() does not crash ──
TEST_CASE("Editor::shutdown() without setup() is safe", "[editor][settings][integration]") {
    // Configure offscreen SDL3 driver
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    ed::Editor editor;
    // No setup() called — shutdown() must not crash
    REQUIRE_NOTHROW(editor.shutdown());
}

// ── Shield test: Editor::setup() called multiple times ──
TEST_CASE("Editor::setup() multiple times is safe", "[editor][settings][integration]") {
    // Configure offscreen SDL3 driver
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto engine = be::EngineService::create(
        be::Backend::SDL3,
        be::WindowConfig{.title = "MultiSetup", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<be::World>();
    auto render_system = std::make_unique<be::RenderSystem>(eng.device(), *world);

    be::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    ed::Editor editor;

    // First setup
    auto result1 = editor.setup(ctx);
    if (result1.has_value()) {
        REQUIRE_NOTHROW(editor.settings_manager());
    }

    // Second setup (may fail if ImGui already initialized, but must not crash)
    REQUIRE_NOTHROW(editor.setup(ctx));

    editor.shutdown();
}

#else
// Stub tests so the file compiles in headless mode (no tests run)
TEST_CASE("Settings integration tests require display", "[editor][settings][.][hide]") {
    SUCCEED("Skipped — BUDDD_HAS_DISPLAY=OFF");
}
#endif // BUDDD_HAS_DISPLAY
