#include "test_command.h"
#include "demo_helpers.h"

#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/primitive_topology.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

auto bc::TestCommand::run(int argc, const char* const* argv) -> int {
    // Warn about unexpected extra arguments
    if (argc > 2) {
        std::fprintf(stderr, "Warning: unexpected arguments after 'test':");
        for (int i = 2; i < argc; ++i) {
            std::fprintf(stderr, " %s", argv[i]);
        }
        std::fprintf(stderr, "\n");
    }

    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "Failed to create platform: "
                  << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto window = (*platform)->create_window({
        .title = "Buddd Engine \u2014 Render Test",
        .width = 800,
        .height = 600
    });
    if (!window) {
        std::cerr << "Failed to create window: "
                  << be::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto device = be::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "Failed to create render device: "
                  << be::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto [material, vb] = bc::setup_triangle(**device);

    // Render loop: ~120 frames at 60 FPS (~2 seconds)
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16); // ~60 FPS

    std::cerr << "Render test started: " << target_frames << " frames\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!(*platform)->poll_events()) {
            std::cerr << "Render test aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        (*device)->begin_frame();
        (*device)->draw(
            be::PrimitiveTopology::Triangles,
            *vb, *material, 3);
        (*device)->end_frame();

        // Frame rate limiting
        auto elapsed = std::chrono::steady_clock::now() - frame_start;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }

    std::cerr << "Render test complete: " << target_frames << " frames rendered\n";
    return EXIT_SUCCESS;
}
