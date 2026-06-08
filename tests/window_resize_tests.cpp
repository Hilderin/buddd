#include "window/window_headless.h"
#include "platform/platform_headless.h"
#include "engine_service.h"
#include "render/render_device.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("WindowHeadless::on_resize updates dimensions", "[window][headless][resize]") {
    auto platform = buddd::engine::Platform::create(buddd::engine::Backend::Headless);
    REQUIRE(platform.has_value());

    buddd::engine::WindowConfig cfg{"Test", 640, 480};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    REQUIRE(window.value()->width() == 640);
    REQUIRE(window.value()->height() == 480);

    window.value()->on_resize(800, 600);

    REQUIRE(window.value()->width() == 800);
    REQUIRE(window.value()->height() == 600);
}

TEST_CASE("RenderDeviceHeadless::size reflects on_resize", "[window][headless][resize][device]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{"Test", 640, 480});
    REQUIRE(engine.has_value());

    auto& eng = **engine;
    REQUIRE(eng.window().width() == 640);
    REQUIRE(eng.window().height() == 480);
    REQUIRE(eng.device().size() == std::pair{640, 480});

    eng.window().on_resize(800, 600);

    REQUIRE(eng.window().width() == 800);
    REQUIRE(eng.window().height() == 600);
    REQUIRE(eng.device().size() == std::pair{800, 600});
}

TEST_CASE("WindowHeadless::on_resize with boundary values", "[window][headless][resize][boundary]") {
    auto platform = buddd::engine::Platform::create(buddd::engine::Backend::Headless);
    REQUIRE(platform.has_value());

    buddd::engine::WindowConfig cfg{"Test", 320, 240};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    // Headless accepts any values (no clamping by design)
    window.value()->on_resize(320, 240);
    REQUIRE(window.value()->width() == 320);
    REQUIRE(window.value()->height() == 240);
}
