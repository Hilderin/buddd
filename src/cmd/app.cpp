#include "app.h"
#include "app_config.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "engine_service.h"
#include "engine_context.h"
#include "image/image.h"
#include "image/image_buffer.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"
#include "render/render_system.h"
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

    // 3. Create World + RenderSystem (always, unconditionally)
    auto world = std::make_unique<be::World>();
    auto render_system = std::make_unique<be::RenderSystem>(eng.device(), *world);

    // 4. Setup app
    be::EngineContext setup_ctx{
        eng, eng.window(), eng.device(), *world, *render_system,
        eng.platform().delta_time(), 0
    };
    auto setup_result = app.setup(setup_ctx);
    if (!setup_result) {
        BUDDD_LOG_ERROR("{}", be::to_string(setup_result.error()));
        app.shutdown();
        return EXIT_FAILURE;
    }

    // 5. Print start message
    bool has_limit = args.frame_limit > 0;
    if (has_limit) {
        BUDDD_LOG_INFO("Scene started: {} ({} frames)", cfg.title, args.frame_limit);
    } else {
        BUDDD_LOG_INFO("Scene started: {} (interactive)", cfg.title);
    }

    // 6. Render loop
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

        // Poll asset file events (hot-reload, file watching)
        eng.assets().poll_file_events();

        // Begin frame
        eng.device().begin_frame();

        // Construct per-frame EngineContext
        be::EngineContext ctx{
            eng, eng.window(), eng.device(), *world, *render_system,
            eng.platform().delta_time(), frame
        };

        // Frame start hook (hot-reload polling, transform updates, etc.)
        app.on_frame_begin(ctx);

        // Exit check after on_frame_begin
        if (ctx.is_exit_requested()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user (frame {})", frame + 1);
            eng.device().end_frame();  // Must end frame before breaking
            break;
        }

        // ── Updatable auto-dispatch ──
        world->update_updatables(ctx);
        if (ctx.is_exit_requested()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user (frame {})", frame + 1);
            eng.device().end_frame();
            break;
        }

        // ── Automatic scene render ──
        render_system->render_scene();

        // ── Custom rendering (optional, default no-op) ──
        app.on_render(ctx);

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

    // 7. Print completion or abort
    if (!aborted_by_user) {
        BUDDD_LOG_INFO("Scene complete: {} ({} frames rendered)", cfg.title, frame);
    }

    // 8. Shutdown
    app.shutdown();

    BUDDD_LOG_INFO("Window closed, shutting down.");

    // 9. Exit code based on capture success
    bool has_captures = !args.captures.empty();
    if (has_captures && !any_capture_success && any_capture_failure)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
