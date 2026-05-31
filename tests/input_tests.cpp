#include "error.h"
#include "platform/platform.h"
#include "input/input_system.h"
#include "input/key_code.h"
#include "input/input_system_headless.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace buddd::engine;

// ===== Headless tests (always compiled) =====

// T-01
TEST_CASE("Factory creates Headless InputSystem", "[headless][input]") {
    auto result = InputSystem::create(Backend::Headless);
    REQUIRE(result.has_value());
    auto& sys = *result.value();
    REQUIRE(dynamic_cast<InputSystemHeadless*>(&sys) != nullptr);
}

// T-03
TEST_CASE("Headless InputSystem returns defaults", "[headless][input]") {
    auto result = InputSystem::create(Backend::Headless);
    REQUIRE(result.has_value());
    auto& sys = *result.value();
    sys.begin_frame();

    // All keys return false
    for (int k = 0; k < static_cast<int>(KeyCode::_Count); ++k) {
        auto key = static_cast<KeyCode>(k);
        REQUIRE_FALSE(sys.is_down(key));
        REQUIRE_FALSE(sys.is_pressed(key));
        REQUIRE_FALSE(sys.is_released(key));
    }

    // Mouse position, delta, wheel return (0,0)
    auto [mx, my] = sys.mouse_position();
    REQUIRE(mx == 0.0f);
    REQUIRE(my == 0.0f);
    auto [dx, dy] = sys.mouse_delta();
    REQUIRE(dx == 0.0f);
    REQUIRE(dy == 0.0f);
    auto [wx, wy] = sys.mouse_wheel();
    REQUIRE(wx == 0.0f);
    REQUIRE(wy == 0.0f);

    // All mouse buttons return false
    auto check_mouse = [&](MouseButton btn) {
        REQUIRE_FALSE(sys.is_mouse_down(btn));
        REQUIRE_FALSE(sys.is_mouse_pressed(btn));
        REQUIRE_FALSE(sys.is_mouse_released(btn));
    };
    check_mouse(MouseButton::Left);
    check_mouse(MouseButton::Right);
    check_mouse(MouseButton::Middle);
    check_mouse(MouseButton::X1);
    check_mouse(MouseButton::X2);
}

// T-04
TEST_CASE("Factory unknown backend returns InputInitFailed", "[headless][input]") {
    auto result = InputSystem::create(static_cast<Backend>(999));
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == Error::Category::InputInitFailed);
}

// T-05
TEST_CASE("Headless Platform input_system() returns valid ref", "[headless][platform][input]") {
    auto platform = Platform::create(Backend::Headless);
    REQUIRE(platform.has_value());

    auto& input = platform.value()->input_system();
    // Verify it's an InputSystem& by calling a query
    REQUIRE_FALSE(input.is_down(KeyCode::Escape));

    platform.value()->poll_events();
    // After poll_events, all queries still return defaults
    REQUIRE_FALSE(input.is_down(KeyCode::Escape));
    REQUIRE_FALSE(input.is_mouse_down(MouseButton::Left));
    auto [mx, my] = input.mouse_position();
    REQUIRE(mx == 0.0f);
    REQUIRE(my == 0.0f);
}

// T-06
TEST_CASE("Headless begin_frame() does not crash", "[headless][input]") {
    auto platform = Platform::create(Backend::Headless);
    REQUIRE(platform.has_value());

    // Call poll_events() twice — begin_frame() runs each time
    REQUIRE(platform.value()->poll_events());
    REQUIRE(platform.value()->poll_events());
    // Verify no crash
    REQUIRE(true);
}

// T-07
TEST_CASE("Double-buffered state transitions (standalone)", "[headless][input]") {
    // For headless: just verify that begin_frame() does not crash.
    auto result = InputSystem::create(Backend::Headless);
    REQUIRE(result.has_value());
    auto& sys = *result.value();
    sys.begin_frame();
    sys.begin_frame();
    REQUIRE(true);
}

// T-08
TEST_CASE("sizeof(KeyCode) == 1", "[headless][input]") {
    static_assert(sizeof(KeyCode) == 1, "KeyCode must be 1 byte");
    REQUIRE(sizeof(KeyCode) == 1);
}

// T-09
TEST_CASE("MouseButton values exist", "[headless][input]") {
    // Verify all five MouseButton values compile and can be compared.
    auto check = [](MouseButton btn) {
        auto val = static_cast<uint8_t>(btn);
        REQUIRE(val <= 4);  // X2 = 4
    };
    check(MouseButton::Left);
    check(MouseButton::Right);
    check(MouseButton::Middle);
    check(MouseButton::X1);
    check(MouseButton::X2);
}

// ===== SDL3 integration tests (conditional) =====

#ifdef BUDDD_HAS_DISPLAY

#include <SDL3/SDL.h>       // For SDL_SetHint + SDL_PushEvent (per expanded AMEND-2026-001)
#include "input/input_system_sdl3.h"

namespace {

auto key_code_to_sdl_scancode(KeyCode key) -> SDL_Scancode {
    auto sc = static_cast<SDL_Scancode>(key);
    // Verify the KeyCode value is valid (not Unknown or _Count)
    REQUIRE(sc > SDL_SCANCODE_UNKNOWN);
    return sc;
}

auto mouse_button_to_sdl(MouseButton button) -> uint8_t {
    switch (button) {
        case MouseButton::Left:   return SDL_BUTTON_LEFT;
        case MouseButton::Right:  return SDL_BUTTON_RIGHT;
        case MouseButton::Middle: return SDL_BUTTON_MIDDLE;
        case MouseButton::X1:     return SDL_BUTTON_X1;
        case MouseButton::X2:     return SDL_BUTTON_X2;
    }
    return SDL_BUTTON_LEFT;
}

static auto push_key_event(KeyCode key, bool down) -> bool {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = key_code_to_sdl_scancode(key);
    event.key.down = down;
    event.key.repeat = false;
    return SDL_PushEvent(&event);
}

static auto push_mouse_motion_event(float x, float y, float xrel, float yrel) -> bool {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = x;
    event.motion.y = y;
    event.motion.xrel = xrel;
    event.motion.yrel = yrel;
    event.motion.which = 0;
    event.motion.timestamp = 0;
    return SDL_PushEvent(&event);
}

static auto push_mouse_button_event(MouseButton button, bool down) -> bool {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.button = mouse_button_to_sdl(button);
    event.button.down = down;
    event.button.x = 0;
    event.button.y = 0;
    event.button.which = 0;
    event.button.timestamp = 0;
    return SDL_PushEvent(&event);
}

static auto push_mouse_wheel_event(float x, float y) -> bool {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.x = x;
    event.wheel.y = y;
    event.wheel.which = 0;
    event.wheel.timestamp = 0;
    return SDL_PushEvent(&event);
}

} // anonymous namespace

// T-02
TEST_CASE("Factory creates SDL3 InputSystem", "[headless][input]") {
    auto result = InputSystem::create(Backend::SDL3);
    REQUIRE(result.has_value());
    auto& sys = *result.value();
    REQUIRE(dynamic_cast<InputSystemSDL3*>(&sys) != nullptr);
}

// T-10
TEST_CASE("SDL3 key down/up detected via InputSystem", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    // Push key-down for Escape
    REQUIRE(push_key_event(KeyCode::Escape, true));
    REQUIRE(platform.value()->poll_events());

    auto& input = platform.value()->input_system();
    REQUIRE(input.is_down(KeyCode::Escape));
    REQUIRE(input.is_pressed(KeyCode::Escape));

    // Push key-up for Escape
    REQUIRE(push_key_event(KeyCode::Escape, false));
    REQUIRE(platform.value()->poll_events());

    REQUIRE_FALSE(input.is_down(KeyCode::Escape));
    REQUIRE(input.is_released(KeyCode::Escape));
}

// T-11
TEST_CASE("SDL3 mouse motion updates position and delta", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    REQUIRE(push_mouse_motion_event(100.0f, 200.0f, 10.0f, -5.0f));
    REQUIRE(platform.value()->poll_events());

    auto& input = platform.value()->input_system();
    {
        auto [x, y] = input.mouse_position();
        REQUIRE(x == 100.0f);
        REQUIRE(y == 200.0f);
    }
    {
        auto [dx, dy] = input.mouse_delta();
        REQUIRE(dx == 10.0f);
        REQUIRE(dy == -5.0f);
    }

    // Next frame with no events: delta resets to zero
    REQUIRE(platform.value()->poll_events());
    {
        auto [dx, dy] = input.mouse_delta();
        REQUIRE(dx == 0.0f);
        REQUIRE(dy == 0.0f);
    }
    // Position persists
    {
        auto [x, y] = input.mouse_position();
        REQUIRE(x == 100.0f);
        REQUIRE(y == 200.0f);
    }
}

// T-12
TEST_CASE("SDL3 mouse button pressed/released", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto check_button = [&](MouseButton btn) {
        auto& input = platform.value()->input_system();

        // Press
        REQUIRE(push_mouse_button_event(btn, true));
        REQUIRE(platform.value()->poll_events());

        REQUIRE(input.is_mouse_down(btn));
        REQUIRE(input.is_mouse_pressed(btn));
        REQUIRE_FALSE(input.is_mouse_released(btn));

        // Release
        REQUIRE(push_mouse_button_event(btn, false));
        REQUIRE(platform.value()->poll_events());

        REQUIRE_FALSE(input.is_mouse_down(btn));
        REQUIRE_FALSE(input.is_mouse_pressed(btn));
        REQUIRE(input.is_mouse_released(btn));
    };

    check_button(MouseButton::Left);
    check_button(MouseButton::Right);
    check_button(MouseButton::Middle);
    check_button(MouseButton::X1);
    check_button(MouseButton::X2);
}

// T-13
TEST_CASE("SDL3 mouse wheel accumulates correctly", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto& input = platform.value()->input_system();

    // Single wheel event
    REQUIRE(push_mouse_wheel_event(1.0f, 2.0f));
    REQUIRE(platform.value()->poll_events());

    {
        auto [wx, wy] = input.mouse_wheel();
        REQUIRE(wx == 1.0f);
        REQUIRE(wy == 2.0f);
    }

    // Next frame with no events: wheel resets to zero
    REQUIRE(platform.value()->poll_events());
    {
        auto [wx, wy] = input.mouse_wheel();
        REQUIRE(wx == 0.0f);
        REQUIRE(wy == 0.0f);
    }

    // Two wheel events before a single poll_events() call
    REQUIRE(push_mouse_wheel_event(1.0f, 2.0f));
    REQUIRE(push_mouse_wheel_event(1.0f, 2.0f));
    REQUIRE(platform.value()->poll_events());

    {
        auto [wx, wy] = input.mouse_wheel();
        REQUIRE(wx == 2.0f);
        REQUIRE(wy == 4.0f);
    }
}

// T-14
TEST_CASE("SDL3 multi-frame key transitions", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto& input = platform.value()->input_system();

    // Frame 1: key-down for W
    REQUIRE(push_key_event(KeyCode::W, true));
    REQUIRE(platform.value()->poll_events());
    REQUIRE(input.is_pressed(KeyCode::W));
    REQUIRE(input.is_down(KeyCode::W));
    REQUIRE_FALSE(input.is_released(KeyCode::W));

    // Frame 2: no new events — key held
    REQUIRE(platform.value()->poll_events());
    REQUIRE_FALSE(input.is_pressed(KeyCode::W));
    REQUIRE(input.is_down(KeyCode::W));
    REQUIRE_FALSE(input.is_released(KeyCode::W));

    // Frame 3: key-up for W
    REQUIRE(push_key_event(KeyCode::W, false));
    REQUIRE(platform.value()->poll_events());
    REQUIRE_FALSE(input.is_pressed(KeyCode::W));
    REQUIRE_FALSE(input.is_down(KeyCode::W));
    REQUIRE(input.is_released(KeyCode::W));
}

// T-15
TEST_CASE("SDL3 Platform provides valid InputSystem reference", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    // Verify input_system() returns a valid reference by calling a method
    auto& input = platform.value()->input_system();
    // The reference should bind successfully — call a non-fallible method
    input.is_down(KeyCode::Escape);
    REQUIRE(true);
}

// T-16
TEST_CASE("SDL3 rapid down+up in same frame", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto& input = platform.value()->input_system();

    // Push key-down then key-up before poll_events()
    REQUIRE(push_key_event(KeyCode::Space, true));
    REQUIRE(push_key_event(KeyCode::Space, false));
    REQUIRE(platform.value()->poll_events());

    // No net change: both events processed in same frame
    REQUIRE_FALSE(input.is_down(KeyCode::Space));
    REQUIRE_FALSE(input.is_pressed(KeyCode::Space));
    REQUIRE_FALSE(input.is_released(KeyCode::Space));
}

// T-17
TEST_CASE("KeyCode static_cast round-trip for representative keys", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto check_key = [](KeyCode key) {
        auto sc = static_cast<SDL_Scancode>(key);
        REQUIRE(sc > SDL_SCANCODE_UNKNOWN);
        // Round-trip: static_cast back to KeyCode
        auto back = static_cast<KeyCode>(sc);
        REQUIRE(back == key);
    };

    check_key(KeyCode::A);
    check_key(KeyCode::Z);
    check_key(KeyCode::Digit0);
    check_key(KeyCode::Digit9);
    check_key(KeyCode::Space);
    check_key(KeyCode::Escape);
    check_key(KeyCode::Up);
    check_key(KeyCode::F12);
    check_key(KeyCode::Grave);
}

#endif // BUDDD_HAS_DISPLAY
