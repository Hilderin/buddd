#include "capture/cube_capture.h"
#include "demo/demo_helpers.h"

#include "platform/platform.h"
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

auto buddd::cmd::capture::capture_cube_scene(
    be::Platform& platform,
    be::RenderDevice& device,
    int window_w,
    int window_h,
    int num_frames
) -> be::Result<be::ImageBuffer>
{
    // Setup cube resources (reuses setup_cube from demo_helpers)
    auto cube = buddd::cmd::demo::setup_cube(device);

    // Camera setup: (0, 0, 3) looking at origin — front-facing reference view.
    // This differs from the cube demo (3,2,3) to produce a deterministic,
    // axis-aligned capture for visual verification.
    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{0.0f, 0.0f, 3.0f},   // eye
        be::math::Vec3{0.0f, 0.0f, 0.0f},   // centre
        be::math::Vec3::unit_y()              // up
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(window_w) / static_cast<float>(window_h),
        0.1f,
        100.0f
    );

    // Poll events once to allow the window manager to map the window
    platform.poll_events();

    // Render at least 2 frames. Frame 1 has a driver quirk where
    // glReadPixels(GL_BACK) returns the clear color instead of rendered
    // content on the very first frame after window creation. Frame 2+ works.
    int effective_frames = (num_frames < 2) ? 2 : num_frames;

    // Render loop
    auto demo_start = std::chrono::steady_clock::now();
    constexpr auto frame_duration = std::chrono::milliseconds(16); // ~60 FPS

    be::Result<be::ImageBuffer> last_buffer =
        be::make_error(be::Error::Category::Unknown, "no frame captured");

    for (int frame = 0; frame < effective_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        platform.poll_events();

        // Compute rotation: 0.5 rad/s around Y (same as cube_demo)
        auto elapsed = std::chrono::steady_clock::now() - demo_start;
        float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
        float angle = elapsed_seconds * 0.5f;

        be::math::Mat4 model_matrix =
            be::math::Mat4::rotate(angle, be::math::Vec3::unit_y());
        be::math::Mat4 mvp =
            camera.projection_matrix() * camera.view_matrix() * model_matrix;

        device.begin_frame();

        // Set MVP uniform
        auto uniform_result = cube.material->set_uniform("u_mvp", mvp);
        if (!uniform_result) {
            std::cerr << "Failed to set u_mvp uniform: "
                      << be::to_string(uniform_result.error()) << "\n";
        }

        // Draw the cube
        cube.model.draw(device);

        // Capture on the requested frame (last frame). Must call read_pixels
        // BEFORE end_frame() to read from the back buffer before the swap.
        if (frame == effective_frames - 1) {
            last_buffer = device.read_pixels();
            if (!last_buffer) {
                device.end_frame();
                return std::unexpected(last_buffer.error());
            }
        }

        device.end_frame();

        // Frame rate limiting (skip for single-frame captures)
        if (effective_frames > 1) {
            auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
            if (frame_elapsed < frame_duration) {
                std::this_thread::sleep_for(frame_duration - frame_elapsed);
            }
        }
    }

    return std::move(*last_buffer);
}
