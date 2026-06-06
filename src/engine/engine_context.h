#pragma once

namespace buddd::engine {

class EngineService;
class Window;

struct EngineContext {
    EngineService& services;
    Window& window;
    float delta_time;

    void request_exit() const { exit_requested_ = true; }
    [[nodiscard]] auto is_exit_requested() const -> bool { return exit_requested_; }

    mutable bool exit_requested_ = false;
};

} // namespace buddd::engine
