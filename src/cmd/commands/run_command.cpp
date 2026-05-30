#include "run_command.h"

#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

auto bc::RunCommand::run([[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int {
    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "FATAL: " << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto window = (*platform)->create_window({
        .title = "Buddd Engine",
        .width = 1024,
        .height = 768
    });
    if (!window) {
        std::cerr << "FATAL: " << be::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    std::printf("Window opened: %dx%d\n", (*window)->width(), (*window)->height());

    auto device = be::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "FATAL: " << be::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Render loop: runs until the window is closed by the user
    // Each frame clears the framebuffer (begin_frame does the clear in the
    // OpenGL backend via glClear) with no draw calls.
    while ((*platform)->poll_events()) {
        (*device)->begin_frame();
        (*device)->end_frame();
    }

    std::printf("Window closed, shutting down.\n");
    return EXIT_SUCCESS;
}
