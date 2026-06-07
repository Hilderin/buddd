#pragma once

namespace buddd::engine {

class EngineService;
class Window;
class RenderDevice;
class World;
class RenderSystem;

struct EngineContext {
    EngineService& services;
    Window& window;
    RenderDevice& device;
    World& world;
    RenderSystem& render_system;
    float delta_time;
    int frame;           // 0-based, incremented by run_app()

    void request_exit() const { exit_requested_ = true; }
    [[nodiscard]] auto is_exit_requested() const -> bool { return exit_requested_; }

    mutable bool exit_requested_ = false;
};

} // namespace buddd::engine
