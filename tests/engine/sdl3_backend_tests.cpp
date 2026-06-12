#ifdef BUDDD_HAS_DISPLAY

#include <SDL3/SDL.h>     // For SDL_SetHint (CONST-001 exception, per AMEND-2026-001)
#include "error.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>

using namespace buddd::engine;

// Corresponds to AC-003
TEST_CASE("Platform::create(SDL3) succeeds with offscreen driver", "[sdl3][platform]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());
}

// Corresponds to AC-004
TEST_CASE("SDL3 Platform creates Window with valid config", "[sdl3][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());
}

// Corresponds to AC-007
TEST_CASE("SDL3 Window::native_handle() returns non-null", "[sdl3][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    REQUIRE(window.value()->native_handle() != nullptr);
}

// Corresponds to AC-008
TEST_CASE("SDL3 Window dimensions match config", "[sdl3][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    REQUIRE(window.value()->width() == 800);
    REQUIRE(window.value()->height() == 600);
}

// Corresponds to AC-005, AC-009
TEST_CASE("SDL3 RenderDevice creation", "[sdl3][render]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    auto [w, h] = device.value()->size();
    REQUIRE(w == 800);
    REQUIRE(h == 600);
}

// Corresponds to AC-006
TEST_CASE("SDL3 frame cycle completes", "[sdl3][render]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    device.value()->begin_frame();
    device.value()->end_frame();
    REQUIRE(true);
}

#endif // BUDDD_HAS_DISPLAY
