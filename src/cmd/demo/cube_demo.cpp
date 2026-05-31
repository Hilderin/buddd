#include "demo/cube_demo.h"
#include "demo/demo_helpers.h"

#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/mat4.h"
#include "math/vec3.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace be = buddd::engine;

auto buddd::cmd::demo::run_cube_demo(
    be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int
{
    auto cube = setup_cube(device);

    // Camera setup
    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},     // eye
        be::math::Vec3{0.0f, 0.0f, 0.0f},     // centre
        be::math::Vec3::unit_y()               // up
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );

    // Render loop: ~120 frames at 60 FPS (~2 seconds)
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16);
    auto demo_start = std::chrono::steady_clock::now();

    std::cerr << "Demo started: cube (" << target_frames << " frames)\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!device.window().platform().poll_events()) {
            std::cerr << "Demo aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        // Compute elapsed time and rotation
        auto elapsed = std::chrono::steady_clock::now() - demo_start;
        float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
        float angle = elapsed_seconds * 0.5f;  // 0.5 rad/s around Y

        be::math::Mat4 model_matrix =
            be::math::Mat4::rotate(angle, be::math::Vec3::unit_y());
        be::math::Mat4 mvp =
            camera.projection_matrix() * camera.view_matrix() * model_matrix;

        device.begin_frame();

        // Set MVP uniform (face colours are in vertex attributes)
        auto uniform_result = cube.material->set_uniform("u_mvp", mvp);
        if (!uniform_result) {
            std::cerr << "Failed to set u_mvp uniform: "
                      << be::to_string(uniform_result.error()) << "\n";
        }

        // Single indexed draw call covering all 36 indices
        cube.model.draw(device);

        device.end_frame();

        // Frame rate limiting
        auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
        if (frame_elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_elapsed);
        }
    }

    std::cerr << "Demo complete: cube (" << target_frames << " frames rendered)\n";
    return EXIT_SUCCESS;
}
