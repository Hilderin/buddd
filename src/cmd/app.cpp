#include "app.h"
#include "app_config.h"

#include "log/log.h"

#include "engine_service.h"
#include "engine_context.h"
#include "image/image.h"
#include "image/image_buffer.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "scene/world.h"
#include "scene/updatable.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

BUDDD_LOG_TAG("App");

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

    // 2. Create EngineService (creates Platform + Window + RenderDevice + AssetManager)
    auto engine = be::EngineService::create(k_app_backend, {cfg.title, cfg.width, cfg.height});
    if (!engine) {
        BUDDD_LOG_ERROR("FATAL: {}", be::to_string(engine.error()));
        return EXIT_FAILURE;
    }
    auto& eng = **engine;

    BUDDD_LOG_INFO("Window opened: {}x{}", eng.window().width(), eng.window().height());

    // 3. Setup app
    auto setup_result = app.setup(eng);
    if (!setup_result) {
        BUDDD_LOG_ERROR("{}", be::to_string(setup_result.error()));
        app.shutdown();
        return EXIT_FAILURE;
    }

    // 4. Print start message
    bool has_limit = args.frame_limit > 0;
    if (has_limit) {
        BUDDD_LOG_INFO("Scene started: {} ({} frames)", cfg.title, args.frame_limit);
    } else {
        BUDDD_LOG_INFO("Scene started: {} (interactive)", cfg.title);
    }

    // 5. Render loop
    bool any_capture_success = false;
    bool any_capture_failure = false;
    int frame = 0;
    bool aborted_by_user = false;

    while (true) {
        // Frame limit check
        if (has_limit && frame >= args.frame_limit)
            break;

        // Event polling
        if (!eng.platform().poll_events()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user");
            break;
        }

        // App requested stop
        if (!app.is_running()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user (frame {})", frame + 1);
            break;
        }

        // Begin frame
        eng.device().begin_frame();

        // Frame start hook (hot-reload polling, etc.)
        app.on_frame_begin();

        // ── Updatable auto-dispatch ──
        if (auto* app_world = app.world()) {
            be::EngineContext ctx{eng, eng.window(), eng.platform().delta_time()};
            app_world->update_updatables(ctx);
            if (ctx.is_exit_requested()) {
                app.set_running(false);
            }
        }

        // Render
        app.render(eng.device(), frame);

        // Capture: read_pixels BEFORE end_frame()
        bool did_read_pixels = false;
        be::Result<be::ImageBuffer> pixel_buffer =
            be::make_error(be::Error::Category::Unknown, "no capture needed");

        for (const auto& spec : args.captures) {
            int effective_frame = spec.effective_frame();
            if (effective_frame == frame + 1) {
                if (!did_read_pixels) {
                    pixel_buffer = eng.device().read_pixels();
                    did_read_pixels = true;
                }
                if (pixel_buffer) {
                    auto image = be::Image::create(*pixel_buffer);
                    if (image) {
                        auto save_result = image->save(spec.path);
                        if (save_result) {
                            any_capture_success = true;
                            BUDDD_LOG_INFO("Captured: {}", spec.path);
                        } else {
                            any_capture_failure = true;
                            BUDDD_LOG_ERROR("{}", be::to_string(save_result.error()));
                        }
                    } else {
                        any_capture_failure = true;
                        BUDDD_LOG_ERROR("{}", be::to_string(image.error()));
                    }
                } else {
                    any_capture_failure = true;
                    BUDDD_LOG_ERROR("{}", be::to_string(pixel_buffer.error()));
                }
            }
        }

        // End frame
        eng.device().end_frame();

        ++frame;
    }

    // 6. Print completion or abort
    if (!aborted_by_user) {
        BUDDD_LOG_INFO("Scene complete: {} ({} frames rendered)", cfg.title, frame);
    }

    // 7. Shutdown
    app.shutdown();

    BUDDD_LOG_INFO("Window closed, shutting down.");

    // 8. Exit code based on capture success
    bool has_captures = !args.captures.empty();
    if (has_captures && !any_capture_success && any_capture_failure)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
