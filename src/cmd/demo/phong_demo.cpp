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
#include <vector>

namespace be = buddd::engine;

// ============================================================================
// Procedural texture helpers
// ============================================================================

static auto make_checkerboard_texture(be::RenderDevice& device) -> std::shared_ptr<be::Texture> {
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

static auto make_solid_texture(be::RenderDevice& device, uint8_t r, uint8_t g, uint8_t b)
    -> std::shared_ptr<be::Texture>
{
    constexpr int SIZE = 1;
    constexpr int CHANNELS = 3;
    std::vector<std::byte> pixels(static_cast<size_t>(SIZE) * SIZE * CHANNELS);
    pixels[0] = static_cast<std::byte>(r);
    pixels[1] = static_cast<std::byte>(g);
    pixels[2] = static_cast<std::byte>(b);

    be::ImageBuffer buf;
    buf.width = SIZE;
    buf.height = SIZE;
    buf.channels = CHANNELS;
    buf.data = std::move(pixels);

    auto image = be::Image::create(buf);
    if (!image) {
        std::cerr << "FATAL: Failed to create solid image: "
                  << be::to_string(image.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    auto tex = device.create_texture(*image);
    if (!tex) {
        std::cerr << "FATAL: Failed to create solid texture: "
                  << be::to_string(tex.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    return std::shared_ptr<be::Texture>(std::move(*tex));
}

// ============================================================================
// Cube model factory
// ============================================================================

static auto create_phong_cube(be::RenderDevice& device,
                              std::shared_ptr<be::PhongMaterial> material)
    -> be::Model
{
    using be::Vertex;
    // 24 vertices: position + normal + texcoord per face
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
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
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

// ============================================================================
// Material characteristics for each cube
// ============================================================================

struct CubeSpec {
    const char*  name;
    be::math::Vec3 position;
    be::math::Vec3 diffuse_tint;
    be::math::Vec3 specular;
    float         shininess;
    bool          textured;     // true = checkerboard, false = solid colour via tint
};

static const CubeSpec k_cubes[] = {
    //         name        position     diffuse             specular         shininess  textured
    {  "Red Metallic",    {-5.f, 0, 0}, {0.95f, 0.15f, 0.10f}, {1.0f, 0.9f, 0.7f}, 128.f,  false },
    {  "Blue Glossy",     {-2.5f, 0, 0},{0.10f, 0.30f, 0.95f}, {1.0f, 1.0f, 1.0f}, 64.f,   false },
    {  "Textured Cube",   {0.f, 0, 0},  {1.0f, 1.0f, 1.0f},    {1.0f, 1.0f, 1.0f}, 32.f,   true  },
    {  "Green Matte",     {2.5f, 0, 0}, {0.15f, 0.85f, 0.25f}, {0.15f, 0.15f, 0.15f}, 4.f,  false },
    {  "Pearl White",     {5.f, 0, 0},  {1.0f, 0.88f, 0.70f},  {0.9f, 0.9f, 1.0f},  8.f,   false },
};

// ============================================================================
// Main demo
// ============================================================================

auto buddd::cmd::demo::run_phong_demo(
    be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int
{
    std::cerr << "\n=== Phong Lighting Demo ===\n"
              << "Right-click to capture mouse | WASD + Space/Ctrl to move | ESC to exit\n"
              << "Scene: " << std::size(k_cubes) << " cubes, 5 lights\n\n";

    // ── ECS ──
    be::World world;

    // ── Camera ──
    auto camera_entity = be::Entity::create(world);
    be::math::Camera camera;
    camera_entity.add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity.get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{6.0f, 3.5f, 8.0f});
    cam.set_orientation(be::math::Quat::from_euler(be::math::radians(-18.0f),
                                                    be::math::radians(35.0f), 0.0f));
    cam.set_perspective(be::math::radians(55.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // ── Textures ──
    auto checkerboard_tex = make_checkerboard_texture(device);
    auto white_tex = make_solid_texture(device, 255, 255, 255);

    std::cerr << "Created textures: checkerboard + solid white\n";

    // ── Create cubes ──
    for (const auto& spec : k_cubes) {
        auto mat = std::make_shared<be::PhongMaterial>(device);

        // Set material properties (persist across frames — RenderSystem no longer overwrites them)
        auto res_spec = mat->set_uniform("u_material_specular",
            be::math::Vec4{spec.specular.x, spec.specular.y, spec.specular.z, 1.0f});
        if (!res_spec) {
            std::cerr << "Warning: set_uniform(u_material_specular) failed: "
                      << be::to_string(res_spec.error()) << "\n";
        }

        auto res_shiny = mat->set_uniform("u_material_shininess", spec.shininess);
        if (!res_shiny) {
            std::cerr << "Warning: set_uniform(u_material_shininess) failed: "
                      << be::to_string(res_shiny.error()) << "\n";
        }

        auto res_tint = mat->set_uniform("u_material_diffuse_tint",
            be::math::Vec4{spec.diffuse_tint.x, spec.diffuse_tint.y, spec.diffuse_tint.z, 1.0f});
        if (!res_tint) {
            std::cerr << "Warning: set_uniform(u_material_diffuse_tint) failed: "
                      << be::to_string(res_tint.error()) << "\n";
        }

        // Assign texture
        auto tex = spec.textured ? checkerboard_tex : white_tex;
        auto res_tex = mat->set_texture("u_diffuse_texture", tex);
        if (!res_tex) {
            std::cerr << "Warning: set_texture failed: "
                      << be::to_string(res_tex.error()) << "\n";
        }

        // Create model and entity
        auto model = create_phong_cube(device, mat);
        auto entity = be::Entity::create(world);
        entity.add_component<be::MeshRenderer>(
            std::make_shared<be::Model>(std::move(model)));
        entity.transform().position = spec.position;

        std::cerr << "  Created cube: " << spec.name
                  << "  at (" << spec.position.x << ", "
                  << spec.position.y << ", " << spec.position.z << ")"
                  << "  shininess=" << spec.shininess << "\n";
    }

    // ── Directional fill light ──
    auto fill = be::Entity::create(world);
    fill.add_component<be::DirectionalLightComponent>(
        be::math::Vec3{0.6f, 0.6f, 0.8f}, 0.35f);
    fill.transform().rotation =
        be::math::Quat::from_euler(be::math::radians(-35.0f),
                                    be::math::radians(50.0f), 0.0f);
    std::cerr << "  Directional light: cool fill, intensity=0.35\n";

    // ── Point light A (warm orange, orbiting) ──
    auto pointA_entity = be::Entity::create(world);
    pointA_entity.add_component<be::PointLightComponent>(
        be::math::Vec3{1.0f, 0.4f, 0.1f}, 1.8f, 12.0f);
    std::cerr << "  Point light A: warm orange, orbiting\n";

    // ── Point light B (cool blue, orbiting, out of phase) ──
    auto pointB_entity = be::Entity::create(world);
    pointB_entity.add_component<be::PointLightComponent>(
        be::math::Vec3{0.1f, 0.3f, 1.0f}, 1.6f, 12.0f);
    std::cerr << "  Point light B: cool blue, orbiting\n";

    // ── Point light C (static purple, above center) ──
    auto pointC_entity = be::Entity::create(world);
    pointC_entity.add_component<be::PointLightComponent>(
        be::math::Vec3{0.6f, 0.2f, 0.8f}, 0.7f, 8.0f);
    pointC_entity.transform().position = be::math::Vec3{0.0f, 3.5f, 0.0f};
    std::cerr << "  Point light C: purple, static at (0, 3.5, 0)\n";

    // ── Spot light (bright warm, from above aiming at origin) ──
    auto spot_entity = be::Entity::create(world);
    spot_entity.add_component<be::SpotLightComponent>(
        be::math::Vec3{1.0f, 0.95f, 0.85f}, 2.5f, 14.0f,
        be::math::radians(18.0f), be::math::radians(35.0f));
    spot_entity.transform().position = be::math::Vec3{0.0f, 6.0f, -3.0f};
    // Rotate to point toward origin (pitch down ~30°)
    spot_entity.transform().rotation =
        be::math::Quat::from_euler(be::math::radians(30.0f), 0.0f, 0.0f);
    std::cerr << "  Spot light: warm white, from (0, 6, -3) aiming at origin\n";

    // ── Render system ──
    be::RenderSystem render_system(device, world);

    // ── Camera state ──
    float yaw = be::math::radians(35.0f);
    float pitch = be::math::radians(-18.0f);
    constexpr float k_move_speed = 5.0f;
    constexpr float k_mouse_sensitivity = 0.002f;
    constexpr float k_pitch_clamp = 89.0f;
    constexpr auto frame_duration = std::chrono::milliseconds(16);

    bool prev_right_click = false;

    auto& input = device.window().platform().input_system();
    auto demo_start = std::chrono::steady_clock::now();

    std::cerr << "\nDemo ready — " << std::size(k_cubes) << " cubes, 5 lights.\n"
              << "Right-click to begin exploring!\n\n";

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
        float elapsed = std::chrono::duration<float>(now - demo_start).count();

        // ── Mouse capture (right-click toggle) ──
        bool curr_right_click = input.is_mouse_down(be::MouseButton::Right);
        if (curr_right_click && !prev_right_click) {
            device.window().set_mouse_capture(true);
        }
        if (!curr_right_click && prev_right_click) {
            device.window().set_mouse_capture(false);
        }
        prev_right_click = curr_right_click;

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

        // ── Update orbiting lights ──
        float t = elapsed;
        float orbit_r = 6.0f;
        float orbit_y = 2.5f;

        // Point A: warm orange, large horizontal orbit
        pointA_entity.transform().position = be::math::Vec3{
            orbit_r * std::cos(t * 0.8f),
            orbit_y + 0.8f * std::sin(t * 1.2f),
            orbit_r * std::sin(t * 0.8f)
        };

        // Point B: cool blue, smaller orbit, 90° out of phase, different height
        pointB_entity.transform().position = be::math::Vec3{
            orbit_r * 0.7f * std::cos(t * 0.6f + 1.57f),
            orbit_y - 0.5f + 1.2f * std::sin(t * 0.9f + 0.5f),
            orbit_r * 0.7f * std::sin(t * 0.6f + 1.57f)
        };

        // ── Render ──
        render_system.render();

        // ── Frame-rate limiting ──
        auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
        if (frame_elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_elapsed);
        }
    }

    std::cerr << "\nDemo complete: phong (interactive)\n";
    return EXIT_SUCCESS;
}
