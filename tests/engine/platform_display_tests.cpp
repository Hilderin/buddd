#include "platform/platform.h"
#include "window/window.h"

#include <catch2/catch_test_macros.hpp>

namespace be = buddd::engine;

TEST_CASE("AC-008: PlatformHeadless::display_count() returns 0", "[headless][platform][display]") {
    auto platform = be::Platform::create(be::Backend::Headless);
    REQUIRE(platform.has_value());

    int count = (*platform)->display_count();
    REQUIRE(count == 0);
}

TEST_CASE("AC-008: PlatformHeadless::display_bounds(0) returns zero", "[headless][platform][display]") {
    auto platform = be::Platform::create(be::Backend::Headless);
    REQUIRE(platform.has_value());

    auto bounds = (*platform)->display_bounds(0);
    REQUIRE(bounds.x == 0);
    REQUIRE(bounds.y == 0);
    REQUIRE(bounds.width == 0);
    REQUIRE(bounds.height == 0);
}

// ── SDL3 offscreen display tests (require BUDDD_HAS_DISPLAY) ──
#ifdef BUDDD_HAS_DISPLAY
#include <SDL3/SDL.h>

TEST_CASE("AC-007: PlatformSDL3 display_count and display_bounds (offscreen)", "[sdl3][platform][display]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto platform = be::Platform::create(be::Backend::SDL3);
    REQUIRE(platform.has_value());

    int count = (*platform)->display_count();
    // Offscreen driver typically reports 1 display
    REQUIRE(count > 0);

    auto bounds = (*platform)->display_bounds(0);
    // Offscreen display should have valid bounds (even if 0,0,0,0 in some implementations)
    // At minimum, bounds should not be negative
    REQUIRE(bounds.x >= 0);
    REQUIRE(bounds.y >= 0);
}

TEST_CASE("AC-007: PlatformSDL3::display_bounds(-1) returns zero bounds", "[sdl3][platform][display]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto platform = be::Platform::create(be::Backend::SDL3);
    REQUIRE(platform.has_value());

    auto bounds = (*platform)->display_bounds(-1);
    REQUIRE(bounds.x == 0);
    REQUIRE(bounds.y == 0);
    REQUIRE(bounds.width == 0);
    REQUIRE(bounds.height == 0);
}

TEST_CASE("AC-007: PlatformSDL3::display_bounds(999) returns zero bounds", "[sdl3][platform][display]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto platform = be::Platform::create(be::Backend::SDL3);
    REQUIRE(platform.has_value());

    auto bounds = (*platform)->display_bounds(999);
    REQUIRE(bounds.x == 0);
    REQUIRE(bounds.y == 0);
    REQUIRE(bounds.width == 0);
    REQUIRE(bounds.height == 0);
}

#endif // BUDDD_HAS_DISPLAY
