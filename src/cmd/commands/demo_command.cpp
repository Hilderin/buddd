#include "demo_command.h"
#include "demo/triangle_demo.h"

#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

namespace {

/// Shared demo usage text constant.
inline constexpr std::string_view k_demo_usage =
    "Usage: buddd demo <demo>\n"
    "\n"
    "Available demos:\n"
    "  triangle     Run the triangle demo (120 frames)\n"
    "\n"
    "Demo names are case-sensitive.\n";

} // anonymous namespace

auto bc::DemoCommand::run(int argc, const char* const* argv) -> int {
    // No demo name provided
    if (argc < 3) {
        std::fwrite(k_demo_usage.data(), 1, k_demo_usage.size(), stderr);
        return EXIT_FAILURE;
    }

    const std::string_view demo_name{argv[2]};

    // Create platform, window, and render device
    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "Failed to create platform: "
                  << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Build full title: "Buddd Engine — Demo: <name>"
    // WindowConfig::title is std::string, so concatenation creates a temporary
    // that is copied into the config.
    auto window_title = std::string("Buddd Engine \u2014 Demo: ") + std::string(demo_name);
    auto window = (*platform)->create_window({
        .title = window_title,
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

    // Warn about unexpected extra arguments (argv[3] and beyond)
    if (argc > 3) {
        std::fprintf(stderr, "Warning: unexpected arguments after 'demo %s':",
                     argv[2]);
        for (int i = 3; i < argc; ++i) {
            std::fprintf(stderr, " %s", argv[i]);
        }
        std::fprintf(stderr, "\n");
    }

    // Dispatch to per-demo function using if/else chain
    // Pass argc - 2, argv + 2 so the demo receives argv[0] == demo name
    if (demo_name == "triangle") {
        return buddd::cmd::demo::run_triangle_demo(**platform, **device, argc - 2, argv + 2);
    }

    // Unknown demo name
    std::fprintf(stderr, "Unknown demo: '%s'\n\n", argv[2]);
    std::fwrite(k_demo_usage.data(), 1, k_demo_usage.size(), stderr);
    return EXIT_FAILURE;
}
