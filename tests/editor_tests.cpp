#include "editor.h"
#include "engine_service.h"
#include "engine_context.h"
#include "platform/platform.h"
#include "window/window.h"
#include "scene/world.h"
#include "render/render_system.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>

TEST_CASE("Editor can be constructed, set up, and shut down headlessly", "[editor]") {
    // Create a headless engine
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Editor Test", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<buddd::engine::World>();
    auto render_system = std::make_unique<buddd::engine::RenderSystem>(eng.device(), *world);

    buddd::engine::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    // Construct Editor
    buddd::editor::Editor editor;

    // Setup will fail because ImGui is not initialized in headless mode — must not crash
    auto result = editor.setup(ctx);
    // Both success and failure are valid outcomes; we only verify no crash.

    // Shutdown must be safe and idempotent
    editor.shutdown();
    editor.shutdown();  // second call must be a no-op
}
