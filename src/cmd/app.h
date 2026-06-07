#pragma once

#include "app_config.h"

#include "error.h"

#include <string>

namespace buddd::engine { class EngineContext; }

namespace buddd::cmd {

/// Window configuration returned by App::config().
struct AppConfig {
    std::string title = "Buddd Engine";
    int width = 1024;
    int height = 768;
};

/// Base class for all renderable applications.
/// Lifecycle: config() -> setup() -> on_frame_begin() x N -> on_render() x N -> shutdown().
class App {
public:
    virtual ~App() = default;

    /// Window configuration (title, dimensions). Called once before setup().
    [[nodiscard]] virtual auto config() const -> AppConfig = 0;

    /// Called once before the render loop.
    /// Return error to abort (loop skipped, shutdown() still called).
    [[nodiscard]] virtual auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> = 0;

    /// Called once per frame, before render_scene(), between begin_frame() and end_frame().
    /// Default no-op. Override for per-frame tasks like hot-reload polling or transform updates.
    virtual auto on_frame_begin(buddd::engine::EngineContext const& ctx) -> void {}

    /// Called once per frame after render_scene().
    /// Replaces render(RenderDevice&, int). Default no-op.
    virtual auto on_render(buddd::engine::EngineContext const& ctx) -> void {}

    /// Called once after the render loop ends (normal or error).
    virtual auto shutdown() -> void {}
};

/// Central render loop.
/// Creates Platform/Window/RenderDevice, runs render loop, saves captures.
///
/// Flow:
///   1. Get AppConfig from app.config()
///   2. Create Platform (SDL3 or Headless based on BUDDD_HAS_DISPLAY)
///   3. Create Window (using AppConfig.title/width/height)
///   4. Create RenderDevice
///   5. Create World + RenderSystem (always, unconditionally)
///   6. Call app.setup(ctx) where ctx contains all per-frame references
///   7. Render loop with RunningArgs.frame_limit and RunningArgs.captures
///   8. Call app.shutdown()
///   9. Return exit code
[[nodiscard]] auto run_app(App& app, const RunningArgs& args) -> int;

} // namespace buddd::cmd
