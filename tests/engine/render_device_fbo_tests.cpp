#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/frame_buffer.h"
#include "render/texture.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

// ---------------------------------------------------------------------------
// Headless FBO unit tests (tagged [render][fbo][headless])
// ---------------------------------------------------------------------------

TEST_CASE("FrameBuffer_Headless_CreateDestroy", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    auto fbo_result = device.create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();
    REQUIRE(fbo.width() == 64);
    REQUIRE(fbo.height() == 64);
    // Destructor runs when unique_ptr goes out of scope — no crash.
}

TEST_CASE("FrameBuffer_Headless_Resize", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    auto fbo_result = device.create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();

    REQUIRE(fbo.resize(128, 128));
    REQUIRE(fbo.width() == 128);
    REQUIRE(fbo.height() == 128);

    REQUIRE(fbo.resize(32, 32));
    REQUIRE(fbo.width() == 32);
    REQUIRE(fbo.height() == 32);
}

TEST_CASE("FrameBuffer_Headless_BindUnbind", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    auto fbo_result = device.create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();

    REQUIRE_NOTHROW(fbo.bind());
    REQUIRE_NOTHROW(fbo.unbind());
}

TEST_CASE("FrameBuffer_Headless_ReadPixelsFails", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    auto fbo_result = device.create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();

    auto result = device.read_pixels(fbo);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().category == buddd::engine::Error::Category::Unsupported);

    auto msg = result.error().message;
    REQUIRE(msg.find("not supported") != std::string::npos);
}

TEST_CASE("FrameBuffer_Headless_ZeroSize", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    // Zero width
    auto r1 = device.create_frame_buffer(0, 64);
    REQUIRE_FALSE(r1.has_value());
    REQUIRE(r1.error().category == buddd::engine::Error::Category::InvalidArgument);

    // Zero height
    auto r2 = device.create_frame_buffer(64, 0);
    REQUIRE_FALSE(r2.has_value());
    REQUIRE(r2.error().category == buddd::engine::Error::Category::InvalidArgument);
}

TEST_CASE("FrameBuffer_Headless_ResizeZero", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    auto fbo_result = device.create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();

    // Resize with zero width — must fail and leave dimensions unchanged
    auto r1 = fbo.resize(0, 128);
    REQUIRE_FALSE(r1.has_value());
    REQUIRE(r1.error().category == buddd::engine::Error::Category::InvalidArgument);
    REQUIRE(fbo.width() == 64);
    REQUIRE(fbo.height() == 64);

    // Resize with zero height — must fail and leave dimensions unchanged
    auto r2 = fbo.resize(128, 0);
    REQUIRE_FALSE(r2.has_value());
    REQUIRE(r2.error().category == buddd::engine::Error::Category::InvalidArgument);
    REQUIRE(fbo.width() == 64);
    REQUIRE(fbo.height() == 64);
}

TEST_CASE("FrameBuffer_Headless_ColorTexture", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    auto fbo_result = device.create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();

    REQUIRE(fbo.color_texture().width() == 64);
    REQUIRE(fbo.color_texture().height() == 64);
    REQUIRE(fbo.color_texture().channels() == 4);
}

TEST_CASE("RenderTexture_Headless_Create", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    auto tex_result = device.create_render_texture(64, 64);
    REQUIRE(tex_result.has_value());
    auto& tex = *tex_result.value();
    REQUIRE(tex.width() == 64);
    REQUIRE(tex.height() == 64);
    REQUIRE(tex.channels() == 4);
}

TEST_CASE("FrameBuffer_Headless_BindResizeUnbind", "[render][fbo][headless]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();

    auto fbo_result = device.create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();

    fbo.bind();                          // no-op, no crash
    REQUIRE(fbo.resize(128, 128));       // resize while bound
    fbo.unbind();                        // no-op, no crash
    REQUIRE(fbo.width() == 128);
    REQUIRE(fbo.height() == 128);
}
