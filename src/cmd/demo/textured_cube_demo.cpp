#include "demo/textured_cube_demo.h"

#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "render/texture.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "image/image.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

namespace be = buddd::engine;

auto buddd::cmd::demo::run_textured_cube_demo(
    be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int
{
    // 1. Load texture
    auto image_result = be::Image::load("assets/brick.png");
    if (!image_result) {
        std::cerr << "FATAL: could not load assets/brick.png: "
                  << be::to_string(image_result.error()) << "\n";
        return EXIT_FAILURE;
    }

    // 2. Create texture
    auto texture_result = device.create_texture(*image_result);
    if (!texture_result) {
        std::cerr << "FATAL: could not create texture: "
                  << be::to_string(texture_result.error()) << "\n";
        return EXIT_FAILURE;
    }

    // 3. Wrap in shared_ptr
    std::shared_ptr<be::Texture> texture(std::move(*texture_result));

    // 4. Create World, Entity, Camera
    be::World world;
    auto entity = be::Entity::create(world);

    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );
    entity.add_component<be::CameraComponent>(camera);

    // 5. Create vertex buffer with texture coordinates
    // Vertex format: position (Float3, loc 0) + texcoord (Float2, loc 1), stride = 20 bytes
    struct TexturedCubeVertex {
        float px, py, pz;
        float tx, ty;
    };

    const TexturedCubeVertex vertices[] = {
        // +X face (right)
        { 1.f, -1.f, -1.f,  0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 1.f },
        { 1.f,  1.f, -1.f,  0.f, 1.f },
        // -X face (left)
        {-1.f, -1.f, -1.f,  0.f, 0.f },
        {-1.f, -1.f,  1.f,  1.f, 0.f },
        {-1.f,  1.f,  1.f,  1.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f },
        // +Y face (top)
        {-1.f,  1.f,  1.f,  0.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f, -1.f,  1.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f },
        // -Y face (bottom)
        {-1.f, -1.f, -1.f,  0.f, 0.f },
        { 1.f, -1.f, -1.f,  1.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 1.f },
        {-1.f, -1.f,  1.f,  0.f, 1.f },
        // +Z face (front)
        {-1.f, -1.f,  1.f,  0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 1.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f },
        // -Z face (back)
        { 1.f, -1.f, -1.f,  0.f, 0.f },
        {-1.f, -1.f, -1.f,  1.f, 0.f },
        {-1.f,  1.f, -1.f,  1.f, 1.f },
        { 1.f,  1.f, -1.f,  0.f, 1.f },
    };

    const uint16_t indices[] = {
        // +X face
         0,  1,  2,   0,  2,  3,
        // -X face
         4,  5,  6,   4,  6,  7,
        // +Y face
         8,  9, 10,   8, 10, 11,
        // -Y face
        12, 13, 14,  12, 14, 15,
        // +Z face
        16, 17, 18,  16, 18, 19,
        // -Z face
        20, 21, 22,  20, 22, 23,
    };

    be::VertexFormat format;
    format.stride = sizeof(TexturedCubeVertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, 0, false},
        {1, be::VertexAttributeType::Float2,
            static_cast<uint32_t>(offsetof(TexturedCubeVertex, tx)), false},
    };

    // 6. Create shaders
    constexpr std::string_view k_vertex_source = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec2 a_texcoord;
        out vec2 v_texcoord;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
            v_texcoord = a_texcoord;
        }
    )";

    constexpr std::string_view k_fragment_source = R"(
        #version 450 core
        in vec2 v_texcoord;
        out vec4 frag_color;
        uniform sampler2D u_tex;
        void main() {
            frag_color = texture(u_tex, v_texcoord);
        }
    )";

    auto vs = device.create_shader(be::ShaderType::Vertex, k_vertex_source);
    if (!vs) {
        std::cerr << "FATAL: " << be::to_string(vs.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto fs = device.create_shader(be::ShaderType::Fragment, k_fragment_source);
    if (!fs) {
        std::cerr << "FATAL: " << be::to_string(fs.error()) << "\n";
        return EXIT_FAILURE;
    }

    // 7. Create material
    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    if (!mat) {
        std::cerr << "FATAL: " << be::to_string(mat.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Convert to shared_ptr
    std::shared_ptr<be::Material> shared_mat(std::move(*mat));

    // 8. Set texture on material
    auto tex_result = shared_mat->set_texture("u_tex", texture);
    if (!tex_result) {
        std::cerr << "FATAL: set_texture failed: "
                  << be::to_string(tex_result.error()) << "\n";
        return EXIT_FAILURE;
    }

    // 9. Create model
    auto model = be::Model::create_indexed(
        device, format,
        std::as_bytes(std::span(vertices)),
        std::as_bytes(std::span(indices)),
        be::IndexType::Uint16, shared_mat
    );
    if (!model) {
        std::cerr << "FATAL: Failed to create textured cube model: "
                  << be::to_string(model.error()) << "\n";
        return EXIT_FAILURE;
    }

    // 10. Attach to entity via MeshRenderer
    entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*model)));

    // 11. Create RenderSystem
    be::RenderSystem render_system(device, world);

    // 12. Render loop: 120 frames at ~60 FPS
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16);
    auto demo_start = std::chrono::steady_clock::now();

    std::cerr << "Demo started: textured-cube (" << target_frames << " frames)\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!device.window().platform().poll_events()) {
            std::cerr << "Demo aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        // Rotate entity around Y axis
        auto elapsed = std::chrono::steady_clock::now() - demo_start;
        float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
        float angle = elapsed_seconds * 0.5f;
        entity.transform().rotation =
            be::math::Quat::angle_axis(angle, be::math::Vec3::unit_y());

        render_system.render();

        // Frame rate limiting
        auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
        if (frame_elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_elapsed);
        }
    }

    std::cerr << "Demo complete: textured-cube ("
              << target_frames << " frames rendered)\n";
    return EXIT_SUCCESS;
}
