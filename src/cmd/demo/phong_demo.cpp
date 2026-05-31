#include "demo/phong_demo.h"

#include "input/input_system.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "render/texture.h"
#include "render/vertex.h"
#include "render/light_data.h"
#include "render/phong/phong_material.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/point_light_component.h"
#include "scene/spot_light_component.h"
#include "image/image.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

namespace be = buddd::engine;

static auto make_checkerboard_texture(be::RenderDevice& device) -> std::shared_ptr<be::Texture> {
    // Generate a procedural 8x8 checkerboard texture (3 channels, RGB)
    constexpr int SIZE = 8;
    constexpr int CHANNELS = 3;
    std::vector<std::byte> pixels(static_cast<size_t>(SIZE) * SIZE * CHANNELS);

    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            bool white = ((x + y) % 2) == 0;
            uint8_t val = white ? 255 : 0;
            size_t offset = static_cast<size_t>(y * SIZE + x) * CHANNELS;
            pixels[offset + 0] = static_cast<std::byte>(val);
            pixels[offset + 1] = static_cast<std::byte>(val);
            pixels[offset + 2] = static_cast<std::byte>(val);
        }
    }

    // Note: ImageBuffer uses bottom-left origin, but our checkerboard is symmetric
    // so flipping is irrelevant.
    be::ImageBuffer buf;
    buf.width = SIZE;
    buf.height = SIZE;
    buf.channels = CHANNELS;
    buf.data = std::move(pixels);

    auto image = be::Image::create(buf);
    if (!image) {
        std::cerr << "FATAL: Failed to create checkerboard image: "
                  << be::to_string(image.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    auto tex = device.create_texture(*image);
    if (!tex) {
        std::cerr << "FATAL: Failed to create checkerboard texture: "
                  << be::to_string(tex.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    return std::shared_ptr<be::Texture>(std::move(*tex));
}

// Create vertex data for a textured cube with normals and texcoords
// 24 vertices, 36 indices — matches the cube pattern from demo_helpers
static auto create_phong_cube_model(be::RenderDevice& device,
                                    std::shared_ptr<be::PhongMaterial> material)
    -> be::Model
{
    using be::Vertex;
    // 24 vertices: position + normal + texcoord for each face
    const Vertex vertices[] = {
        // +X face (right): normal (1,0,0)
        {{ 1.f, -1.f, -1.f}, {}, { 1.f, 0.f, 0.f}, {0.f, 0.f}, {}, {}},
        {{ 1.f, -1.f,  1.f}, {}, { 1.f, 0.f, 0.f}, {1.f, 0.f}, {}, {}},
        {{ 1.f,  1.f,  1.f}, {}, { 1.f, 0.f, 0.f}, {1.f, 1.f}, {}, {}},
        {{ 1.f,  1.f, -1.f}, {}, { 1.f, 0.f, 0.f}, {0.f, 1.f}, {}, {}},
        // -X face (left): normal (-1,0,0)
        {{-1.f, -1.f, -1.f}, {}, {-1.f, 0.f, 0.f}, {0.f, 0.f}, {}, {}},
        {{-1.f, -1.f,  1.f}, {}, {-1.f, 0.f, 0.f}, {1.f, 0.f}, {}, {}},
        {{-1.f,  1.f,  1.f}, {}, {-1.f, 0.f, 0.f}, {1.f, 1.f}, {}, {}},
        {{-1.f,  1.f, -1.f}, {}, {-1.f, 0.f, 0.f}, {0.f, 1.f}, {}, {}},
        // +Y face (top): normal (0,1,0)
        {{-1.f,  1.f,  1.f}, {}, {0.f, 1.f, 0.f}, {0.f, 0.f}, {}, {}},
        {{ 1.f,  1.f,  1.f}, {}, {0.f, 1.f, 0.f}, {1.f, 0.f}, {}, {}},
        {{ 1.f,  1.f, -1.f}, {}, {0.f, 1.f, 0.f}, {1.f, 1.f}, {}, {}},
        {{-1.f,  1.f, -1.f}, {}, {0.f, 1.f, 0.f}, {0.f, 1.f}, {}, {}},
        // -Y face (bottom): normal (0,-1,0)
        {{-1.f, -1.f, -1.f}, {}, {0.f, -1.f, 0.f}, {0.f, 0.f}, {}, {}},
        {{ 1.f, -1.f, -1.f}, {}, {0.f, -1.f, 0.f}, {1.f, 0.f}, {}, {}},
        {{ 1.f, -1.f,  1.f}, {}, {0.f, -1.f, 0.f}, {1.f, 1.f}, {}, {}},
        {{-1.f, -1.f,  1.f}, {}, {0.f, -1.f, 0.f}, {0.f, 1.f}, {}, {}},
        // +Z face (front): normal (0,0,1)
        {{-1.f, -1.f,  1.f}, {}, {0.f, 0.f, 1.f}, {0.f, 0.f}, {}, {}},
        {{ 1.f, -1.f,  1.f}, {}, {0.f, 0.f, 1.f}, {1.f, 0.f}, {}, {}},
        {{ 1.f,  1.f,  1.f}, {}, {0.f, 0.f, 1.f}, {1.f, 1.f}, {}, {}},
        {{-1.f,  1.f,  1.f}, {}, {0.f, 0.f, 1.f}, {0.f, 1.f}, {}, {}},
        // -Z face (back): normal (0,0,-1)
        {{ 1.f, -1.f, -1.f}, {}, {0.f, 0.f, -1.f}, {0.f, 0.f}, {}, {}},
        {{-1.f, -1.f, -1.f}, {}, {0.f, 0.f, -1.f}, {1.f, 0.f}, {}, {}},
        {{-1.f,  1.f, -1.f}, {}, {0.f, 0.f, -1.f}, {1.f, 1.f}, {}, {}},
        {{ 1.f,  1.f, -1.f}, {}, {0.f, 0.f, -1.f}, {0.f, 1.f}, {}, {}},
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

    auto model = be::Model::create_indexed(
        device, be::k_standard_vertex_format,
        std::as_bytes(std::span(vertices)),
        std::as_bytes(std::span(indices)),
        be::IndexType::Uint16,
        std::move(material));
    if (!model) {
        std::cerr << "FATAL: Failed to create phong cube model: "
                  << be::to_string(model.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    return std::move(*model);
}

auto buddd::cmd::demo::run_phong_demo(
    be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int
{
    std::cerr << "Demo started: phong (interactive)\n";

    // ── ECS setup ──
    be::World world;

    auto camera_entity = be::Entity::create(world);
    be::math::Camera camera;
    camera_entity.add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity.get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{3.0f, 2.0f, 3.0f});
    cam.set_orientation(be::math::Quat::from_euler(0.0f, 0.0f, 0.0f));
    cam.set_perspective(be::math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // ── Texture ──
    std::shared_ptr<be::Texture> texture;
    auto image_result = be::Image::load("assets/brick.png");
    if (image_result) {
        auto tex_result = device.create_texture(*image_result);
        if (tex_result) {
            texture = std::shared_ptr<be::Texture>(std::move(*tex_result));
            std::cerr << "Phong demo: loaded assets/brick.png\n";
        }
    }
    if (!texture) {
        std::cerr << "Phong demo: using procedural checkerboard texture\n";
        texture = make_checkerboard_texture(device);
    }

    // ── Phong cube ──
    auto phong_mat = std::make_shared<be::PhongMaterial>(device);
    auto tex_result = phong_mat->set_texture("u_diffuse_texture", texture);
    if (!tex_result) {
        std::cerr << "FATAL: set_texture failed: "
                  << be::to_string(tex_result.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    auto cube_model = create_phong_cube_model(device, phong_mat);
    auto cube_entity = be::Entity::create(world);
    cube_entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(cube_model)));

    // ── Point light (orbiting) ──
    auto light_entity = be::Entity::create(world);
    light_entity.add_component<be::PointLightComponent>(
        be::math::Vec3{1.0f, 1.0f, 1.0f}, 1.5f, 8.0f);
    light_entity.transform().position = be::math::Vec3{2.0f, 2.0f, 2.0f};

    // ── Directional light (fill) ──
    auto fill_entity = be::Entity::create(world);
    fill_entity.add_component<be::DirectionalLightComponent>(
        be::math::Vec3{0.5f, 0.5f, 0.7f}, 0.5f);
    // Rotate ~45° around Y, ~30° down
    fill_entity.transform().rotation =
        be::math::Quat::from_euler(be::math::radians(-30.0f),
                                    be::math::radians(45.0f), 0.0f);

    // ── Render system ──
    be::RenderSystem render_system(device, world);

    // ── Camera state ──
    float yaw = 0.0f;
    float pitch = 0.0f;
    constexpr float k_move_speed = 5.0f;
    constexpr float k_mouse_sensitivity = 0.002f;
    constexpr float k_pitch_clamp = 89.0f;
    constexpr auto frame_duration = std::chrono::milliseconds(16);

    bool prev_right_click_ = false;

    auto& input = device.window().platform().input_system();
    auto demo_start = std::chrono::steady_clock::now();

    // ── Interactive loop ──
    while (true) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!device.window().platform().poll_events()) {
            std::cerr << "Demo aborted by user\n";
            return EXIT_SUCCESS;
        }

        if (input.is_down(be::KeyCode::Escape)) {
            break;
        }

        float dt = device.window().platform().delta_time();
        auto now = std::chrono::steady_clock::now();
        float elapsed_seconds = std::chrono::duration<float>(now - demo_start).count();

        // ── Mouse capture (right-click) ──
        bool curr_right_click = input.is_mouse_down(be::MouseButton::Right);
        if (curr_right_click && !prev_right_click_) {
            device.window().set_mouse_capture(true);
            std::cerr << "Mouse captured (right-click)\n";
        }
        if (!curr_right_click && prev_right_click_) {
            device.window().set_mouse_capture(false);
            std::cerr << "Mouse released (right-click)\n";
        }
        prev_right_click_ = curr_right_click;

        bool mouse_captured = device.window().is_mouse_captured();

        // ── Mouse look ──
        if (mouse_captured) {
            auto [dx, dy] = input.mouse_delta();
            yaw -= dx * k_mouse_sensitivity;
            pitch += -dy * k_mouse_sensitivity;
            pitch = std::clamp(pitch, be::math::radians(-k_pitch_clamp),
                                        be::math::radians(k_pitch_clamp));

            cam.set_orientation(be::math::Quat::from_euler(pitch, yaw, 0.0f));
        }

        // ── Keyboard movement ──
        if (mouse_captured) {
            be::math::Vec3 forward = cam.orientation() * be::math::Vec3{0.0f, 0.0f, -1.0f};
            forward.y = 0.0f;
            if (forward.length_squared() > be::math::epsilon) {
                forward.normalize();
            }

            be::math::Vec3 right = cam.orientation() * be::math::Vec3{1.0f, 0.0f, 0.0f};
            be::math::Vec3 movement{0.0f, 0.0f, 0.0f};

            if (input.is_down(be::KeyCode::W))          { movement += forward; }
            if (input.is_down(be::KeyCode::S))          { movement -= forward; }
            if (input.is_down(be::KeyCode::D))          { movement += right; }
            if (input.is_down(be::KeyCode::A))          { movement -= right; }
            if (input.is_down(be::KeyCode::Space))      { movement += be::math::Vec3::unit_y(); }
            if (input.is_down(be::KeyCode::ControlLeft))  { movement -= be::math::Vec3::unit_y(); }
            if (input.is_down(be::KeyCode::ControlRight)) { movement -= be::math::Vec3::unit_y(); }

            cam.set_position(cam.position() + movement * k_move_speed * dt);
        }

        // ── Update orbiting light ──
        float t = elapsed_seconds;
        light_entity.transform().position = be::math::Vec3{
            2.0f * std::cos(t),
            2.0f * std::sin(t) + 1.0f,
            2.0f * std::sin(t * 0.7f)
        };

        // ── Render ──
        render_system.render();

        // ── Frame-rate limiting ──
        auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
        if (frame_elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_elapsed);
        }
    }

    std::cerr << "Demo complete: phong (interactive)\n";
    return EXIT_SUCCESS;
}
