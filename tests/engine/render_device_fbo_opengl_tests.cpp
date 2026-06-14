#ifdef BUDDD_HAS_DISPLAY

#include <SDL3/SDL.h>     // For SDL_SetHint
#include <SDL3/SDL_opengl.h>

#include "error.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/frame_buffer.h"
#include "render/texture.h"
#include "render/shader.h"
#include "render/primitive_topology.h"
#include "render/vertex_format.h"
#include "render/material.h"
#include "render/vertex_buffer.h"
#include "render/index_buffer.h"
#include "render/model.h"
#include "render/mesh_renderer.h"
#include "render/render_system.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/entity.h"
#include "math/math.h"
#include "math/mat4.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>

using namespace buddd::engine;

// ---------------------------------------------------------------------------
// OpenGL FBO integration test (tagged [render][fbo][opengl])
// ---------------------------------------------------------------------------

TEST_CASE("FrameBuffer_OpenGL_RenderAndReadback", "[render][fbo][opengl]") {
    // Use offscreen SDL driver to avoid needing a real display
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    // 1. Create platform, window, device
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "FBO Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    // 2. Create a 64x64 FBO
    auto fbo_result = device.value()->create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();
    REQUIRE(fbo.width() == 64);
    REQUIRE(fbo.height() == 64);

    // 3. Verify AC-003: bind() sets FBO, unbind() restores default framebuffer
    GLint default_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &default_fbo);
    fbo.bind();
    GLint bound_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound_fbo);
    REQUIRE(bound_fbo != default_fbo);   // FBO is now bound
    fbo.unbind();
    GLint after_unbind = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &after_unbind);
    REQUIRE(after_unbind == default_fbo); // default framebuffer restored

    // 4. Create a simple colored material (solid blue-ish: vec4(0.2, 0.4, 0.6, 1.0))
    constexpr std::string_view vs_src = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
        }
    )";
    constexpr std::string_view fs_src = R"(
        #version 450 core
        out vec4 frag_color;
        void main() {
            frag_color = vec4(0.2, 0.4, 0.6, 1.0);
        }
    )";

    auto vs = device.value()->create_shader(ShaderType::Vertex, vs_src);
    REQUIRE(vs.has_value());
    auto fs = device.value()->create_shader(ShaderType::Fragment, fs_src);
    REQUIRE(fs.has_value());
    auto mat = device.value()->create_material(std::move(*vs), std::move(*fs),
        std::span<const std::string>{});
    REQUIRE(mat.has_value());

    // 5. Create a full-coverage triangle (covers the entire FBO)
    //    Vertices in NDC (-1 to 1): a single triangle covering the viewport.
    const float triangle_vertices[] = {
        -1.0f, -1.0f, 0.0f,
         3.0f, -1.0f, 0.0f,
        -1.0f,  3.0f, 0.0f,
    };

    VertexFormat format;
    format.stride = 3 * sizeof(float);
    format.attributes.push_back({0, VertexAttributeType::Float3, 0, false});

    auto vertex_data = std::as_bytes(std::span<const float>(triangle_vertices));
    auto vb = device.value()->create_vertex_buffer(format, vertex_data);
    REQUIRE(vb.has_value());

    // Set MVP to identity (we're in clip space directly)
    auto identity = math::Mat4::identity();
    [[maybe_unused]] auto result = mat.value()->set_uniform("u_mvp", identity);

    // 6. Render into the FBO
    device.value()->begin_frame();
    fbo.bind();
    // Clear the FBO to ensure deterministic initial state
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    device.value()->draw(PrimitiveTopology::Triangles,
                         *vb.value(), *mat.value(), 3);
    fbo.unbind();
    device.value()->end_frame();

    // 7. Read back pixels from FBO
    auto pixels_result = device.value()->read_pixels(fbo);
    REQUIRE(pixels_result.has_value());
    auto& buffer = *pixels_result;
    REQUIRE(buffer.width == 64);
    REQUIRE(buffer.height == 64);
    REQUIRE(buffer.channels == 4);
    REQUIRE(buffer.data.size() == static_cast<size_t>(64 * 64 * 4));

    // 8. Verify some pixels have non-zero values (the scene was rendered)
    //    The triangle covers the entire viewport, so every pixel should be
    //    colored (0.2, 0.4, 0.6) at full alpha.
    bool has_non_black_pixel = false;
    for (size_t i = 0; i < buffer.data.size(); i += 4) {
        auto r = static_cast<unsigned char>(buffer.data[i]);
        auto g = static_cast<unsigned char>(buffer.data[i + 1]);
        auto b = static_cast<unsigned char>(buffer.data[i + 2]);
        if (r > 0 || g > 0 || b > 0) {
            has_non_black_pixel = true;
            break;
        }
    }
    REQUIRE(has_non_black_pixel);

    // Also verify that pixels near the center have non-zero values
    // Center pixel at (32, 32) — in bottom-left origin, row-major.
    size_t center_idx = (static_cast<size_t>(32) * 64 + 32) * 4;
    if (center_idx + 3 < buffer.data.size()) {
        auto r = static_cast<unsigned char>(buffer.data[center_idx]);
        auto g = static_cast<unsigned char>(buffer.data[center_idx + 1]);
        auto b = static_cast<unsigned char>(buffer.data[center_idx + 2]);
        CHECK(r > 0);
        CHECK(g > 0);
        CHECK(b > 0);
    }
}

// ---------------------------------------------------------------------------
// AC-007: create_frame_buffer with zero dimensions returns error on OpenGL
// ---------------------------------------------------------------------------
TEST_CASE("FrameBuffer_OpenGL_ZeroSize", "[render][fbo][opengl]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());
    auto window = platform.value()->create_window(
        {.title = "FBO Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());
    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    // Zero width
    auto r1 = device.value()->create_frame_buffer(0, 64);
    REQUIRE_FALSE(r1.has_value());
    REQUIRE(r1.error().category == Error::Category::InvalidArgument);

    // Zero height
    auto r2 = device.value()->create_frame_buffer(64, 0);
    REQUIRE_FALSE(r2.has_value());
    REQUIRE(r2.error().category == Error::Category::InvalidArgument);
}

// ---------------------------------------------------------------------------
// AC-004: FrameBuffer resize on OpenGL backend
// ---------------------------------------------------------------------------
TEST_CASE("FrameBuffer_OpenGL_Resize", "[render][fbo][opengl]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());
    auto window = platform.value()->create_window(
        {.title = "FBO Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());
    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    // Create 64x64 FBO
    auto fbo_result = device.value()->create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();
    REQUIRE(fbo.width() == 64);
    REQUIRE(fbo.height() == 64);

    // Resize to 128x128
    auto resize_result = fbo.resize(128, 128);
    REQUIRE(resize_result.has_value());
    REQUIRE(fbo.width() == 128);
    REQUIRE(fbo.height() == 128);
    REQUIRE(fbo.color_texture().width() == 128);
    REQUIRE(fbo.color_texture().height() == 128);

    // Resize to same dimensions
    auto same_result = fbo.resize(128, 128);
    REQUIRE(same_result.has_value());
    REQUIRE(fbo.width() == 128);
    REQUIRE(fbo.height() == 128);

    // Double resize: 128x128 -> 64x64 -> 32x32
    REQUIRE(fbo.resize(64, 64).has_value());
    REQUIRE(fbo.resize(32, 32).has_value());
    REQUIRE(fbo.width() == 32);
    REQUIRE(fbo.height() == 32);

    // Verify completeness by rendering into the resized FBO
    fbo.bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fbo.unbind();
    // No crash — FBO is complete enough to clear into.

    // Bind -> resize -> unbind edge case
    fbo.bind();
    REQUIRE(fbo.resize(64, 64).has_value());
    fbo.unbind();
    REQUIRE(fbo.width() == 64);
    REQUIRE(fbo.height() == 64);
}

// ---------------------------------------------------------------------------
// AC-008: RenderSystem::render_scene(FrameBuffer&) overload
// ---------------------------------------------------------------------------
TEST_CASE("FrameBuffer_OpenGL_RenderSceneOverload", "[render][fbo][opengl]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());
    auto window = platform.value()->create_window(
        {.title = "FBO Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());
    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    // Create a 64x64 FBO
    auto fbo_result = device.value()->create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();

    // Create a simple World with a camera and a mesh
    World world;

    // Create a perspective camera looking at a colored triangle
    auto camera_entity = world.add_entity();
    camera_entity.add_component<CameraComponent>(
        math::radians(60.0f),
        static_cast<float>(fbo.width()) / static_cast<float>(fbo.height()),
        0.1f, 100.0f);

    // Create a material with a solid color shader
    constexpr std::string_view vs_src = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
        }
    )";
    constexpr std::string_view fs_src = R"(
        #version 450 core
        out vec4 frag_color;
        void main() {
            frag_color = vec4(0.3, 0.5, 0.7, 1.0);
        }
    )";

    auto vs = device.value()->create_shader(ShaderType::Vertex, vs_src);
    REQUIRE(vs.has_value());
    auto fs = device.value()->create_shader(ShaderType::Fragment, fs_src);
    REQUIRE(fs.has_value());
    auto mat = device.value()->create_material(std::move(*vs), std::move(*fs),
        std::span<const std::string>{"u_mvp"});
    REQUIRE(mat.has_value());

    // Create a large triangle at z=-5 that fills the camera's 60-degree FOV
    // The camera is at origin looking down -Z, so the triangle must be
    // within the frustum (0.1 to 100 units) and cover the entire viewport.
    const float verts[] = { -6.0f, -6.0f, -5.0f, 12.0f, -6.0f, -5.0f, -6.0f, 12.0f, -5.0f };
    const uint16_t idxs[] = { 0, 1, 2 };

    VertexFormat fmt;
    fmt.stride = 3 * sizeof(float);
    fmt.attributes.push_back({0, VertexAttributeType::Float3, 0, false});

    auto model = Model::create_indexed(
        *device.value(), fmt,
        std::as_bytes(std::span(verts)),
        std::as_bytes(std::span(idxs)),
        IndexType::Uint16,
        { SubMesh{0, 3, 0} },
        { std::shared_ptr<Material>(std::move(*mat)) });
    REQUIRE(model.has_value());

    // Attach the model to an entity
    auto mesh_entity = world.add_entity();
    mesh_entity.add_component<MeshRenderer>(std::make_shared<Model>(std::move(*model)));

    // Render the scene into the FBO using the new overload
    auto& rd = *device.value();
    rd.begin_frame();
    {
        RenderSystem render_system(rd, world);
        render_system.render_scene(fbo);
    }
    rd.end_frame();

    // Read back pixels from FBO
    auto pixels_result = device.value()->read_pixels(fbo);
    REQUIRE(pixels_result.has_value());
    auto& buffer = *pixels_result;
    REQUIRE(buffer.width == 64);
    REQUIRE(buffer.height == 64);
    REQUIRE(buffer.channels == 4);
    REQUIRE(buffer.data.size() == static_cast<size_t>(64 * 64 * 4));

    // Verify the scene was rendered (non-black pixels)
    bool has_non_black_pixel = false;
    for (size_t i = 0; i < buffer.data.size(); i += 4) {
        auto r = static_cast<unsigned char>(buffer.data[i]);
        auto g = static_cast<unsigned char>(buffer.data[i + 1]);
        auto b = static_cast<unsigned char>(buffer.data[i + 2]);
        if (r > 0 || g > 0 || b > 0) {
            has_non_black_pixel = true;
            break;
        }
    }
    REQUIRE(has_non_black_pixel);
}

#else

TEST_CASE("FrameBuffer_OpenGL_RenderAndReadback - SKIPPED (no display)", "[render][fbo][opengl]") {
    SUCCEED("Skipped: BUDDD_HAS_DISPLAY not defined");
}

TEST_CASE("FrameBuffer_OpenGL_ZeroSize - SKIPPED (no display)", "[render][fbo][opengl]") {
    SUCCEED("Skipped: BUDDD_HAS_DISPLAY not defined");
}

TEST_CASE("FrameBuffer_OpenGL_Resize - SKIPPED (no display)", "[render][fbo][opengl]") {
    SUCCEED("Skipped: BUDDD_HAS_DISPLAY not defined");
}

TEST_CASE("FrameBuffer_OpenGL_RenderSceneOverload - SKIPPED (no display)", "[render][fbo][opengl]") {
    SUCCEED("Skipped: BUDDD_HAS_DISPLAY not defined");
}

#endif
