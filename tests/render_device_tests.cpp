#include "render/render_device_headless.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <string>

// ---------------------------------------------------------------------------
// Headless render device tests (tagged [render][headless])
// ---------------------------------------------------------------------------

TEST_CASE("Headless read_pixels returns Unsupported error", "[render][headless]") {
    buddd::engine::RenderDeviceHeadless device(800, 600);

    auto result = device.read_pixels();

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == buddd::engine::Error::Category::Unsupported);

    // Check that the error message contains "not supported"
    auto msg = result.error().message;
    REQUIRE(msg.find("not supported") != std::string::npos);
}
