#pragma once

#include "app.h"

#include <chrono>

#include "scene/entity.h"

namespace buddd::cmd::app {

/// Phong lighting demo: interactive, 5 cubes + 5 lights.
class PhongApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 phong", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_frame_begin(buddd::engine::EngineContext const& ctx) -> void override;

    auto on_render(buddd::engine::EngineContext const&) -> void override {}

private:
    buddd::engine::Entity camera_entity_;
    buddd::engine::Entity pointA_entity_;
    buddd::engine::Entity pointB_entity_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
