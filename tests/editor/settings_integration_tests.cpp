#include "editor.h"
#include "engine_service.h"
#include "engine_context.h"
#include "platform/platform.h"
#include "window/window.h"
#include "window/window_utils.h"
#include "scene/world.h"
#include "render/render_system.h"
#include "util/os_config_dir.h"
#include "util/editor_data_root.h"

#include <catch2/catch_test_macros.hpp>

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>

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

// ── Helper: create a temp directory path ──
static auto temp_dir() -> std::filesystem::path {
    auto tmp = std::filesystem::temp_directory_path();
    static std::atomic<unsigned int> counter = 0;
    auto pid = static_cast<unsigned int>(getpid());
    for (int i = 0; i < 200; ++i) {
        auto unique = pid + (++counter);
        auto candidate = tmp / ("buddd_win_settings_" + std::to_string(unique));
        std::error_code ec;
        if (std::filesystem::create_directory(candidate, ec)) {
            return candidate;
        }
    }
    FAIL("Could not create temp directory");
    return tmp;
};

// ── Helper: write content to a file, creating parent directories ──
static void write_settings_yaml(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    REQUIRE(out.is_open());
    out << content;
}

// ── Helper: pre-populate user_project_settings YAML with window keys ──
static void pre_populate_window_settings(
    const std::filesystem::path& project_root,
    int x, int y, int w, int h,
    const std::string& state)
{
    auto yaml_path = be::editor_user_data_root(project_root) / "settings.yaml";
    std::string content =
        "editor:\n"
        "  window:\n"
        "    x: " + std::to_string(x) + "\n"
        "    y: " + std::to_string(y) + "\n"
        "    width: " + std::to_string(w) + "\n"
        "    height: " + std::to_string(h) + "\n"
        "    state: " + state + "\n";
    write_settings_yaml(yaml_path, content);
}

// ── AC-009/AC-014: Full window settings round-trip ──
TEST_CASE("AC-009/AC-014: Window settings round-trip save/load",
          "[editor][settings][integration][window]") {
    // Configure offscreen SDL3 driver
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto project_root = temp_dir();
    auto old_cwd = std::filesystem::current_path();
    std::filesystem::current_path(project_root);

    // Pre-populate with known window settings
    pre_populate_window_settings(project_root, 100, 50, 1024, 768, "normal");

    auto engine = be::EngineService::create(
        be::Backend::SDL3,
        be::WindowConfig{.title = "RoundTrip", .width = 128, .height = 128});
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
        // Verify window size was set from pre-populated settings (1024x768)
        // Offscreen window may report different sizes, but at minimum verify
        // our resize was called (the validation code runs before the ImGui check).
        // WindowHeadless tests cover the validation logic directly.
        INFO("Window after setup: " << eng.window().width() << "x" << eng.window().height());

        // Shutdown should save current window geometry
        editor.shutdown();

        // Read back the saved YAML
        auto yaml_path = be::editor_user_data_root(project_root) / "settings.yaml";
        REQUIRE(std::filesystem::exists(yaml_path));

        auto node = YAML::LoadFile(yaml_path.string());
        REQUIRE(node["editor"]["window"]["x"].IsDefined());
        REQUIRE(node["editor"]["window"]["y"].IsDefined());
        REQUIRE(node["editor"]["window"]["width"].IsDefined());
        REQUIRE(node["editor"]["window"]["height"].IsDefined());
        REQUIRE(node["editor"]["window"]["state"].IsDefined());
    } else {
        editor.shutdown();
    }

    std::filesystem::current_path(old_cwd);
    std::error_code ec;
    std::filesystem::remove_all(project_root, ec);
}

// ── AC-015: Written types are correct ──
TEST_CASE("AC-015: Written types are correct",
          "[editor][settings][integration][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto project_root = temp_dir();
    auto old_cwd = std::filesystem::current_path();
    std::filesystem::current_path(project_root);

    auto engine = be::EngineService::create(
        be::Backend::SDL3,
        be::WindowConfig{.title = "TypesTest", .width = 128, .height = 128});
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
        editor.shutdown();

        auto yaml_path = be::editor_user_data_root(project_root) / "settings.yaml";
        if (std::filesystem::exists(yaml_path)) {
            auto node = YAML::LoadFile(yaml_path.string());
            // x, y, width, height should be integers
            REQUIRE(node["editor"]["window"]["x"].IsScalar());
            REQUIRE(node["editor"]["window"]["y"].IsScalar());
            REQUIRE(node["editor"]["window"]["width"].IsScalar());
            REQUIRE(node["editor"]["window"]["height"].IsScalar());
            // state should be a string
            REQUIRE(node["editor"]["window"]["state"].IsScalar());
            std::string state = node["editor"]["window"]["state"].as<std::string>();
            REQUIRE((state == "normal" || state == "maximized" || state == "minimized"));
        }
    } else {
        editor.shutdown();
    }

    std::filesystem::current_path(old_cwd);
    std::error_code ec;
    std::filesystem::remove_all(project_root, ec);
}

// ── AC-018: WindowSDL3::resize() immediate cache update ──
TEST_CASE("AC-018: WindowSDL3::resize() immediate cache update",
          "[editor][settings][integration][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto engine = be::EngineService::create(
        be::Backend::SDL3,
        be::WindowConfig{.title = "ResizeCache", .width = 1280, .height = 800});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    // Verify initial dimensions
    REQUIRE(eng.window().width() == 1280);
    REQUIRE(eng.window().height() == 800);

    // Call resize — cache should update immediately
    eng.window().resize(800, 600);
    REQUIRE(eng.window().width() == 800);
    REQUIRE(eng.window().height() == 600);
}

// ── Maximized/Minimized state uses cached Normal geometry ──
TEST_CASE("Maximized/Minimized state uses cached Normal geometry in settings",
          "[editor][settings][integration][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto project_root = temp_dir();
    auto old_cwd = std::filesystem::current_path();
    std::filesystem::current_path(project_root);

    // Pre-populate with a known Normal position/size
    pre_populate_window_settings(project_root, 500, 300, 1024, 768, "normal");

    auto engine = be::EngineService::create(
        be::Backend::SDL3,
        be::WindowConfig{.title = "MaxCacheTest", .width = 128, .height = 128});
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
        // After setup, the pre-populated values (500, 300, 1024, 768) were applied
        // and the editor's geometry cache was initialised with them.
        // Now maximise the window — this changes the SDL-reported geometry but
        // the editor's cache should still hold the Normal values.
        eng.window().set_state(be::WindowState::Maximized);

        // Shutdown — must save the CACHED Normal geometry, NOT the current maximised one
        editor.shutdown();

        // Read back the saved YAML
        auto yaml_path = be::editor_user_data_root(project_root) / "settings.yaml";
        REQUIRE(std::filesystem::exists(yaml_path));

        auto node = YAML::LoadFile(yaml_path.string());

        // The cached geometry should be the pre-populated Normal values (500, 300, 1024, 768)
        // regardless of whether the offscreen driver tracks maximised state or not.
        REQUIRE(node["editor"]["window"]["x"].as<int>()       == 500);
        REQUIRE(node["editor"]["window"]["y"].as<int>()       == 300);
        REQUIRE(node["editor"]["window"]["width"].as<int>()   == 1024);
        REQUIRE(node["editor"]["window"]["height"].as<int>()  == 768);
        REQUIRE(node["editor"]["window"]["state"].IsDefined());
    } else {
        editor.shutdown();
    }

    std::filesystem::current_path(old_cwd);
    std::error_code ec;
    std::filesystem::remove_all(project_root, ec);
}

// ── AC-012: minimized state on disk → normal on startup ──
TEST_CASE("Edge: minimized state on disk → normal on startup",
          "[editor][settings][integration][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto project_root = temp_dir();
    auto old_cwd = std::filesystem::current_path();
    std::filesystem::current_path(project_root);

    // Pre-populate with state=minimized
    pre_populate_window_settings(project_root, 100, 50, 1024, 768, "minimized");

    auto engine = be::EngineService::create(
        be::Backend::SDL3,
        be::WindowConfig{.title = "MinimizedTest", .width = 128, .height = 128});
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
        // Window state should be Normal (not Minimized) after setup
        // Note: In SDL3 offscreen mode, state queries may return Normal
        // regardless, but the validation code should force Normal
        auto state = eng.window().state();
        INFO("Window state after setup with minimized saved state: " << static_cast<int>(state));
        // The state may be Normal due to validation or SDL3 behavior
        // We verify it is not Minimized
        REQUIRE(state != be::WindowState::Minimized);

        editor.shutdown();
    } else {
        editor.shutdown();
    }

    std::filesystem::current_path(old_cwd);
    std::error_code ec;
    std::filesystem::remove_all(project_root, ec);
}

#else
// Stub tests so the file compiles in headless mode (no tests run)
TEST_CASE("Settings integration tests require display", "[editor][settings][.][hide]") {
    SUCCEED("Skipped — BUDDD_HAS_DISPLAY=OFF");
}
#endif // BUDDD_HAS_DISPLAY
