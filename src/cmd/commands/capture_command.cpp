#include "commands/capture_command.h"
#include "capture/cube_capture.h"

#include "image/image.h"
#include "image/image_buffer.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <string_view>

namespace be = buddd::engine;
namespace bc = buddd::cmd;

namespace {

inline constexpr std::string_view k_capture_usage =
    "Usage: buddd capture <scenario> [--frame N] [output_path]\n"
    "\n"
    "Available scenarios:\n"
    "  cube    Capture a frame of the rotating cube demo\n"
    "\n"
    "Options:\n"
    "  --frame N   Render N frames and capture the Nth frame (default: 1)\n"
    "\n"
    "Scenario names are case-sensitive.\n";

/// Returns true if the given scenario name is valid.
auto is_valid_scenario(std::string_view name) -> bool {
    return name == "cube";
}

/// Generates a default output path: /tmp/buddd_capture_<scenario>_<timestamp>.png
auto default_output_path(std::string_view scenario) -> std::string {
    auto now = std::time(nullptr);
    return "/tmp/buddd_capture_" + std::string(scenario) + "_"
         + std::to_string(static_cast<long>(now)) + ".png";
}

/// Parses --frame N from argv[start..end] and returns the frame count.
/// Returns 1 if --frame is not specified.
/// On error (invalid N), prints to stderr and returns -1.
auto parse_frame_count(int argc, const char* const* argv, int start) -> int {
    for (int i = start; i < argc; ++i) {
        if (argv[i] == std::string_view("--frame")) {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Error: --frame requires a number (e.g., --frame 60)\n");
                return -1;
            }
            char* end = nullptr;
            long n = std::strtol(argv[i + 1], &end, 10);
            if (end == argv[i + 1] || *end != '\0' || n < 1) {
                std::fprintf(stderr, "Error: --frame requires a positive integer, got '%s'\n", argv[i + 1]);
                return -1;
            }
            return static_cast<int>(n);
        }
    }
    return 1; // default
}

/// Returns the output path from argv, or empty string if not specified.
/// Skips --frame N pairs.
auto parse_output_path(int argc, const char* const* argv, int start) -> std::string {
    for (int i = start; i < argc; ++i) {
        if (argv[i] == std::string_view("--frame")) {
            ++i; // skip the value
            continue;
        }
        // First non-option argument is the output path
        return argv[i];
    }
    return {}; // not specified
}

} // anonymous namespace

auto bc::CaptureCommand::run(int argc, const char* const* argv) -> int {
    // No scenario provided
    if (argc < 3) {
        std::fwrite(k_capture_usage.data(), 1, k_capture_usage.size(), stderr);
        return EXIT_FAILURE;
    }

    const std::string_view scenario{argv[2]};

    // Validate scenario BEFORE creating any resources (fails fast on CI/headless)
    if (!is_valid_scenario(scenario)) {
        std::fprintf(stderr, "Unknown capture scenario: '%s'\n\n", argv[2]);
        std::fwrite(k_capture_usage.data(), 1, k_capture_usage.size(), stderr);
        return EXIT_FAILURE;
    }

    // Parse --frame count (scan argv[3..])
    int num_frames = parse_frame_count(argc, argv, 3);
    if (num_frames < 1) {
        return EXIT_FAILURE; // parse_frame_count already printed the error
    }

    // Parse optional output path (scan argv[3..], skip --frame N)
    std::string output_path = parse_output_path(argc, argv, 3);
    if (output_path.empty()) {
        output_path = default_output_path(scenario);
    }

    // Observability
    std::fprintf(stderr, "Capturing: %s (%d frame(s))\n", scenario.data(), num_frames);

    // Create platform (SDL3 unconditionally — headless capture is unsupported)
    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "Failed to create platform: "
                  << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Build window title
    auto window_title = std::string("Buddd Engine \u2014 Capture: ") + std::string(scenario);
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

    // Dispatch to scenario function
    be::Result<be::ImageBuffer> readback_result = be::make_error(
        be::Error::Category::Unknown, "Unknown scenario");
    if (scenario == "cube") {
        readback_result = buddd::cmd::capture::capture_cube_scene(
            **platform, **device, 800, 600, num_frames);
    }

    if (!readback_result) {
        std::cerr << "Capture failed: " << be::to_string(readback_result.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Convert raw buffer to Image (row-flip + validation)
    auto image = be::Image::create(*readback_result);
    if (!image) {
        std::cerr << "Image processing failed: " << be::to_string(image.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Write PNG
    auto save_result = image->save(output_path);
    if (!save_result) {
        std::cerr << "Failed to write image: " << be::to_string(save_result.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Observability: capture succeeded
    std::printf("Captured: %s\n", output_path.c_str());
    return EXIT_SUCCESS;
}
