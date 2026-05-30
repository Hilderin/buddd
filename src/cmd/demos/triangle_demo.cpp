#include "triangle_demo.h"
#include "demos/demo_helpers.h"

#include "platform/platform.h"
#include "render/render_device.h"
#include "render/primitive_topology.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

namespace be = buddd::engine;

auto buddd::cmd::demo::run_triangle_demo(
    be::Platform& platform, be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int {

    auto [material, vb] = buddd::cmd::demo::setup_triangle(device);

    // Render loop: ~120 frames at 60 FPS (~2 seconds)
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16); // ~60 FPS

    std::cerr << "Demo started: triangle (" << target_frames << " frames)\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!platform.poll_events()) {
            std::cerr << "Demo aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        device.begin_frame();
        device.draw(
            be::PrimitiveTopology::Triangles,
            *vb, *material, 3);
        device.end_frame();

        // Frame rate limiting
        auto elapsed = std::chrono::steady_clock::now() - frame_start;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }

    std::cerr << "Demo complete: triangle (" << target_frames << " frames rendered)\n";
    return EXIT_SUCCESS;
}
