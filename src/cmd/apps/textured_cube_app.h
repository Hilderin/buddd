#pragma once

#include "app.h"

#include "scene/entity.h"

#include <chrono>

namespace buddd::cmd::app {

/// Textured cube with UV-mapped brick texture: 120-frame render loop.
class TexturedCubeApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 textured-cube", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_frame_begin(buddd::engine::EngineContext const& ctx) -> void override;

    auto on_render(buddd::engine::EngineContext const&) -> void override {}

private:
    buddd::engine::Entity entity_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
