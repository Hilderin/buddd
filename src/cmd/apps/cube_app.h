#pragma once

#include "app.h"

#include "demo/demo_helpers.h"
#include "math/camera.h"

#include <chrono>
#include <memory>

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// Rotating cube: 120-frame render loop with manual camera/MVP/draw calls.
class CubeApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 cube", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

private:
    std::unique_ptr<demo::CubeResources> cube_;
    buddd::engine::math::Camera camera_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
