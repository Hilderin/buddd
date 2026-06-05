#pragma once

#include "app.h"

#include <chrono>
#include <memory>
#include <vector>

#include "scene/entity.h"
#include "scene/world.h"
#include "render/render_system.h"

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// Phong lighting demo: interactive, 5 cubes + 5 lights.
class PhongApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 phong", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

private:
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::Entity> camera_entity_;
    std::unique_ptr<buddd::engine::Entity> pointA_entity_;
    std::unique_ptr<buddd::engine::Entity> pointB_entity_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
    std::chrono::steady_clock::time_point start_time_;
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    bool prev_right_click_ = false;
};

} // namespace buddd::cmd::app
