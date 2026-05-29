#include "version.h"
#include "error.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

auto main(int argc, char* argv[]) -> int {
    if (argc == 2 && std::string_view{argv[1]} == "--version") {
        std::printf("buddd %s\n", buddd::engine::version().data());
        return 0;
    }

    std::printf("Buddd Engine v%s\n", buddd::engine::version().data());

    using namespace buddd::engine;

    // Create platform (SDL3 backend)
    auto platform = Platform::create(Backend::SDL3);
    if (!platform) {
        std::fprintf(stderr, "FATAL: %s\n", to_string(platform.error()).c_str());
        return EXIT_FAILURE;
    }

    // Create window
    auto window = (*platform)->create_window({"Buddd Engine", 1024, 768});
    if (!window) {
        std::fprintf(stderr, "FATAL: %s\n", to_string(window.error()).c_str());
        return EXIT_FAILURE;
    }

    // Create render device
    auto device = RenderDevice::create(**window);
    if (!device) {
        std::fprintf(stderr, "FATAL: %s\n", to_string(device.error()).c_str());
        return EXIT_FAILURE;
    }

    std::printf("Window opened: %dx%d\n", (*window)->width(), (*window)->height());

    // Simple render loop (fixed frame count since input handling is not yet implemented)
    constexpr int frame_count = 120;
    for (int i = 0; i < frame_count; ++i) {
        (*device)->begin_frame();
        // TODO: actual rendering commands go here
        (*device)->end_frame();
    }

    std::printf("Rendered %d frames, shutting down.\n", frame_count);

    // Resources clean up automatically when platform, window, device go out of scope
    return 0;
}
