#include "app.h"
#include "app_config.h"

#include "image/image.h"
#include "image/image_buffer.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace be = buddd::engine;

// Select backend based on display availability
constexpr auto k_app_backend = [] {
#ifdef BUDDD_HAS_DISPLAY
    return be::Backend::SDL3;
#else
    return be::Backend::Headless;
#endif
}();

auto buddd::cmd::run_app(App& app, const RunningArgs& args) -> int {
    // 1. Get AppConfig
    auto cfg = app.config();

    // 2. Already have backend constant

    // 3. Create platform
    auto platform = be::Platform::create(k_app_backend);
    if (!platform) {
        std::cerr << "FATAL: " << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    // 4. Create window
    auto window = (*platform)->create_window({
        .title = cfg.title,
        .width = cfg.width,
        .height = cfg.height
    });
    if (!window) {
        std::cerr << "FATAL: " << be::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    std::printf("Window opened: %dx%d\n", (*window)->width(), (*window)->height());

    // 5. Create render device
    auto device = be::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "FATAL: " << be::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    // 6. Setup app
    auto setup_result = app.setup(**device);
    if (!setup_result) {
        std::cerr << be::to_string(setup_result.error()) << "\n";
        app.shutdown();
        return EXIT_FAILURE;
    }

    // 7. Print start message
    bool has_limit = args.frame_limit > 0;
    if (has_limit) {
        std::fprintf(stderr, "Scene started: %s (%d frames)\n",
                     cfg.title.c_str(), args.frame_limit);
    } else {
        std::fprintf(stderr, "Scene started: %s (interactive)\n",
                     cfg.title.c_str());
    }

    // 8. Render loop
    bool any_capture_success = false;
    bool any_capture_failure = false;
    int frame = 0;
    bool aborted_by_user = false;

    while (true) {
        // Frame limit check
        if (has_limit && frame >= args.frame_limit)
            break;

        // Event polling
        if (!(*platform)->poll_events()) {
            aborted_by_user = true;
            std::fprintf(stderr, "Scene aborted by user\n");
            break;
        }

        // App requested stop
        if (!app.is_running()) {
            aborted_by_user = true;
            std::fprintf(stderr, "Scene aborted by user (frame %d)\n", frame + 1);
            break;
        }

        // Begin frame
        (*device)->begin_frame();

        // Frame start hook (hot-reload polling, etc.)
        app.on_frame_begin();

        // Render
        app.render(**device, frame);

        // Capture: read_pixels BEFORE end_frame()
        bool did_read_pixels = false;
        be::Result<be::ImageBuffer> pixel_buffer =
            be::make_error(be::Error::Category::Unknown, "no capture needed");

        for (const auto& spec : args.captures) {
            // Driver quirk: minimum capture frame is 2
            int effective_frame = (spec.frame < 2) ? 2 : spec.frame;
            if (effective_frame == frame + 1) {
                if (!did_read_pixels) {
                    pixel_buffer = (*device)->read_pixels();
                    did_read_pixels = true;
                }
                if (pixel_buffer) {
                    auto image = be::Image::create(*pixel_buffer);
                    if (image) {
                        auto save_result = image->save(spec.path);
                        if (save_result) {
                            any_capture_success = true;
                            std::printf("Captured: %s\n", spec.path.c_str());
                        } else {
                            any_capture_failure = true;
                            std::cerr << be::to_string(save_result.error()) << "\n";
                        }
                    } else {
                        any_capture_failure = true;
                        std::cerr << be::to_string(image.error()) << "\n";
                    }
                } else {
                    any_capture_failure = true;
                    std::cerr << be::to_string(pixel_buffer.error()) << "\n";
                }
            }
        }

        // End frame
        (*device)->end_frame();

        ++frame;
    }

    // 9. Print completion or abort
    if (!aborted_by_user) {
        std::fprintf(stderr, "Scene complete: %s (%d frames rendered)\n",
                     cfg.title.c_str(), frame);
    }

    // 10. Shutdown
    app.shutdown();

    std::printf("Window closed, shutting down.\n");

    // 11. Exit code based on capture success
    bool has_captures = !args.captures.empty();
    if (has_captures && !any_capture_success && any_capture_failure)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
