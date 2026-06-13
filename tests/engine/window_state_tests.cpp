#include "window/window.h"
#include "window/window_headless.h"
#include "window/window_utils.h"
#include "platform/platform.h"

#include <catch2/catch_test_macros.hpp>

namespace be = buddd::engine;

TEST_CASE("AC-001: WindowState enum values exist", "[math][window]") {
    // Compile-time check: these must compile
    auto normal  = be::WindowState::Normal;
    auto maxi    = be::WindowState::Maximized;
    auto mini    = be::WindowState::Minimized;
    REQUIRE(static_cast<int>(normal) == 0);
    REQUIRE(static_cast<int>(maxi)   == 1);
    REQUIRE(static_cast<int>(mini)   == 2);
}

TEST_CASE("AC-019: WindowHeadless::position() returns {0,0}", "[math][window][headless]") {
    auto platform = be::Platform::create(be::Backend::Headless);
    REQUIRE(platform.has_value());

    be::WindowConfig cfg{"Test", 640, 480};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    auto pos = window.value()->position();
    REQUIRE(pos.x == 0);
    REQUIRE(pos.y == 0);
}

TEST_CASE("AC-019: WindowHeadless::state() returns Normal", "[math][window][headless]") {
    auto platform = be::Platform::create(be::Backend::Headless);
    REQUIRE(platform.has_value());

    be::WindowConfig cfg{"Test", 640, 480};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    REQUIRE(window.value()->state() == be::WindowState::Normal);
}

TEST_CASE("AC-019: WindowHeadless::set_position is no-op", "[math][window][headless]") {
    auto platform = be::Platform::create(be::Backend::Headless);
    REQUIRE(platform.has_value());

    be::WindowConfig cfg{"Test", 640, 480};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    // set_position should not crash and position should remain {0,0}
    window.value()->set_position({100, 200});
    auto pos = window.value()->position();
    REQUIRE(pos.x == 0);
    REQUIRE(pos.y == 0);
}

TEST_CASE("AC-019: WindowHeadless::set_state is no-op", "[math][window][headless]") {
    auto platform = be::Platform::create(be::Backend::Headless);
    REQUIRE(platform.has_value());

    be::WindowConfig cfg{"Test", 640, 480};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    // set_state should not crash and state should remain Normal
    window.value()->set_state(be::WindowState::Maximized);
    REQUIRE(window.value()->state() == be::WindowState::Normal);

    window.value()->set_state(be::WindowState::Minimized);
    REQUIRE(window.value()->state() == be::WindowState::Normal);
}

TEST_CASE("AC-019: WindowHeadless::resize updates dimensions", "[math][window][headless]") {
    auto platform = be::Platform::create(be::Backend::Headless);
    REQUIRE(platform.has_value());

    be::WindowConfig cfg{"Test", 640, 480};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    window.value()->resize(800, 600);
    REQUIRE(window.value()->width() == 800);
    REQUIRE(window.value()->height() == 600);
}

TEST_CASE("AC-019: WindowState string round-trip for Normal", "[math][window][state_string]") {
    auto str = be::window_state_to_string(be::WindowState::Normal);
    REQUIRE(str == "normal");
    auto parsed = be::parse_window_state(str);
    REQUIRE(parsed == be::WindowState::Normal);
}

TEST_CASE("AC-019: WindowState string round-trip for Maximized", "[math][window][state_string]") {
    auto str = be::window_state_to_string(be::WindowState::Maximized);
    REQUIRE(str == "maximized");
    auto parsed = be::parse_window_state(str);
    REQUIRE(parsed == be::WindowState::Maximized);
}

TEST_CASE("AC-019: WindowState string round-trip for Minimized", "[math][window][state_string]") {
    auto str = be::window_state_to_string(be::WindowState::Minimized);
    REQUIRE(str == "minimized");
    auto parsed = be::parse_window_state(str);
    REQUIRE(parsed == be::WindowState::Minimized);
}

TEST_CASE("AC-013: parse_window_state with unknown string returns Normal", "[math][window][state_string]") {
    REQUIRE(be::parse_window_state("fullscreen") == be::WindowState::Normal);
    REQUIRE(be::parse_window_state("") == be::WindowState::Normal);
    REQUIRE(be::parse_window_state("garbage") == be::WindowState::Normal);
    REQUIRE(be::parse_window_state("FULLSCREEN") == be::WindowState::Normal);
    REQUIRE(be::parse_window_state("normal") == be::WindowState::Normal); // sanity check: known string works
}

TEST_CASE("AC-018: WindowHeadless::resize immediate cache update", "[math][window][headless]") {
    auto platform = be::Platform::create(be::Backend::Headless);
    REQUIRE(platform.has_value());

    be::WindowConfig cfg{"Test", 1280, 800};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    window.value()->resize(800, 600);
    REQUIRE(window.value()->width() == 800);
    REQUIRE(window.value()->height() == 600);

    // Verify immediate update (no event processing needed)
    window.value()->resize(1024, 768);
    REQUIRE(window.value()->width() == 1024);
    REQUIRE(window.value()->height() == 768);
}
