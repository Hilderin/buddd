#include "error.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>

using namespace buddd::engine;

// ---------------------------------------------------------------------------
// Headless backend tests (always runnable, no display required)
// ---------------------------------------------------------------------------

// T-01
TEST_CASE("Platform::create(Headless) succeeds", "[headless][platform]") {
    auto result = Platform::create(Backend::Headless);
    REQUIRE(result.has_value());
}

// T-02
TEST_CASE("Headless Platform creates Window with valid config", "[headless][window]") {
    auto platform = Platform::create(Backend::Headless);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());
}

// T-03
TEST_CASE("Headless Window creates RenderDevice", "[headless][render]") {
    auto platform = Platform::create(Backend::Headless);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());
}

// T-04
TEST_CASE("Headless frame cycle completes", "[headless][render]") {
    auto platform = Platform::create(Backend::Headless);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    // Frame cycle must complete without error
    device.value()->begin_frame();
    device.value()->end_frame();
    REQUIRE(true);
}

// T-05
TEST_CASE("Headless RenderDevice::size() returns correct dimensions", "[headless][render]") {
    auto platform = Platform::create(Backend::Headless);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    auto [w, h] = device.value()->size();
    REQUIRE(w == 800);
    REQUIRE(h == 600);
}

// T-06
TEST_CASE("Headless Window::native_handle() returns nullptr", "[headless][window]") {
    auto platform = Platform::create(Backend::Headless);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    REQUIRE(window.value()->native_handle() == nullptr);
}

// T-07
TEST_CASE("WindowConfig negative dimensions return error", "[headless][window]") {
    auto platform = Platform::create(Backend::Headless);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "Bad", .width = -1, .height = 100});
    REQUIRE_FALSE(window.has_value());
    REQUIRE(window.error().category == Error::Category::WindowCreationFailed);
}

// T-08
TEST_CASE("Error struct construction and to_string", "[headless][error]") {
    auto err = Error{Error::Category::InitFailed, 42, "test"};
    REQUIRE(buddd::engine::to_string(err) == "InitFailed: test (code 42)");
}

// T-09
TEST_CASE("make_error helper compiles and returns correct category", "[headless][error]") {
    auto result = make_error(Error::Category::WindowCreationFailed, "test");
    REQUIRE(result.error().category == Error::Category::WindowCreationFailed);
    REQUIRE(result.error().message == "test");
    // code should default to 0
    REQUIRE(result.error().code == 0);
}

// T-10
TEST_CASE("make_error with explicit code", "[headless][error]") {
    auto result = make_error(Error::Category::InitFailed, "msg", 42);
    REQUIRE(result.error().code == 42);
}

// T-11
TEST_CASE("Result<T> compiles with unique_ptr", "[headless][error]") {
    auto func = []() -> Result<std::unique_ptr<int>> {
        return std::unique_ptr<int>(new int(42));
    };
    auto result = func();
    REQUIRE(result.has_value());
    REQUIRE(*result.value() == 42);
}

// ---------------------------------------------------------------------------
// SDL3 backend tests (conditional or deferred)
// ---------------------------------------------------------------------------

// T-12 — Compilation check: enum values exist
TEST_CASE("Backend enum values exist", "[sdl3][platform]") {
    auto backend = Backend::SDL3;
    backend = Backend::Headless;
    REQUIRE(true); // If it compiles, it passes
}


