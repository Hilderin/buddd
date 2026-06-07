#pragma once

#include "app.h"

#include "render/material.h"
#include "render/model.h"
#include "scene/camera_component.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <chrono>
#include <memory>

namespace buddd::cmd::app {

/// Rotating cube: 120-frame render loop with manual camera/MVP/draw calls.
class CubeApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 cube", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

private:
    buddd::engine::Model model_;
    std::shared_ptr<buddd::engine::Material> material_;
    buddd::engine::Entity camera_entity_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
