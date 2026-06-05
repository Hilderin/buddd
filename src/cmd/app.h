#pragma once

#include "app_config.h"

#include "error.h"

#include <string>

namespace buddd::engine { class RenderDevice; }

namespace buddd::cmd {

/// Window configuration returned by App::config().
struct AppConfig {
    std::string title = "Buddd Engine";
    int width = 1024;
    int height = 768;
};

/// Base class for all renderable applications.
/// Lifecycle: config() -> setup() -> render() x N -> shutdown().
class App {
public:
    virtual ~App() = default;

    /// Window configuration (title, dimensions). Called once before setup().
    [[nodiscard]] virtual auto config() const -> AppConfig = 0;

    /// Called once before the render loop.
    /// Return error to abort (loop skipped, shutdown() still called).
    [[nodiscard]] virtual auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> = 0;

    /// Called once per frame, between begin_frame() and end_frame().
    /// frame is 0-based (0 = first rendered frame).
    virtual auto render(buddd::engine::RenderDevice& device, int frame) -> void = 0;

    /// Called once after the render loop ends (normal or error).
    virtual auto shutdown() -> void {}

    /// Returns false if the app has requested the render loop to stop.
    [[nodiscard]] auto is_running() const noexcept -> bool { return running_; }

protected:
    /// Set to false to stop the render loop early (for interactive scenes).
    bool running_ = true;
};

/// Central render loop.
/// Creates Platform/Window/RenderDevice, runs render loop, saves captures.
///
/// Flow:
///   1. Get AppConfig from app.config()
///   2. Create Platform (SDL3 or Headless based on BUDDD_HAS_DISPLAY)
///   3. Create Window (using AppConfig.title/width/height)
///   4. Create RenderDevice
///   5. Call app.setup()
///   6. Render loop with RunningArgs.frame_limit and RunningArgs.captures
///   7. Call app.shutdown()
///   8. Return exit code
[[nodiscard]] auto run_app(App& app, const RunningArgs& args) -> int;

} // namespace buddd::cmd
