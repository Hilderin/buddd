#include "demo/free_camera_demo.h"
#include "demo/demo_helpers.h"

#include "input/input_system.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "scene/world.h"
#include "scene/camera_component.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

namespace be = buddd::engine;

auto buddd::cmd::demo::run_free_camera_demo(
    be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int
{
    std::cerr << "Demo started: free-camera (interactive)\n";

    // ── ECS setup ──
    be::World world;

    auto camera_entity = be::Entity::create(world);
    be::math::Camera camera;
    camera_entity.add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity.get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{0.0f, 2.0f, 5.0f});
    cam.set_orientation(be::math::Quat::from_euler(0.0f, 0.0f, 0.0f));
    cam.set_perspective(be::math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // ── Cube setup ──
    auto cube = setup_cube(device);
    auto cube_entity = be::Entity::create(world);
    cube_entity.add_component<be::MeshRenderer>(std::make_shared<be::Model>(std::move(cube.model)));

    // ── Render system ──
    be::RenderSystem render_system(device, world);

    // ── Camera state ──
    float yaw = 0.0f;
    float pitch = 0.0f;
    constexpr float k_move_speed = 5.0f;
    constexpr float k_mouse_sensitivity = 0.002f;
    constexpr float k_pitch_clamp = 89.0f;
    constexpr auto frame_duration = std::chrono::milliseconds(16);

    bool prev_right_click_{false};

    auto& input = device.window().platform().input_system();

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

        // ── Mouse capture (right-click) ──
        bool curr_right_click = input.is_mouse_down(be::MouseButton::Right);
        if (curr_right_click && !prev_right_click_) {
            // Right-click pressed — capture mouse
            device.window().set_mouse_capture(true);
            std::cerr << "Mouse captured (right-click)\n";
        }
        if (!curr_right_click && prev_right_click_) {
            // Right-click released — release mouse
            device.window().set_mouse_capture(false);
            std::cerr << "Mouse released (right-click)\n";
        }
        prev_right_click_ = curr_right_click;

        bool mouse_captured = device.window().is_mouse_captured();

        // ── Mouse look (only while captured) ──
        if (mouse_captured) {
            auto [dx, dy] = input.mouse_delta();
            yaw -= dx * k_mouse_sensitivity;
            pitch += -dy * k_mouse_sensitivity;
            pitch = std::clamp(pitch, be::math::radians(-k_pitch_clamp),
                                        be::math::radians(k_pitch_clamp));

            cam.set_orientation(be::math::Quat::from_euler(pitch, yaw, 0.0f));
        }

        // ── Keyboard movement (only while captured) ──
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

        // ── Render ──
        render_system.render();

        // ── Frame-rate limiting ──
        auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
        if (frame_elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_elapsed);
        }
    }

    std::cerr << "Demo complete: free-camera (interactive)\n";
    return EXIT_SUCCESS;
}
