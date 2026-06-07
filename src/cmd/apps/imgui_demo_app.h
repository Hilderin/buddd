#pragma once

#include "app.h"

namespace buddd::cmd::app {

class ImguiDemoApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 ImGui Demo", 1280, 720};
    }

    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

private:
    bool show_demo_window_ = true;
};

} // namespace buddd::cmd::app
