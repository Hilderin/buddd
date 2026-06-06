#pragma once

#include "app.h"

#include "scene/entity.h"
#include "scene/world.h"
#include "render/render_system.h"

#include <chrono>
#include <memory>

namespace buddd::engine {
class EngineService;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// Interactive free camera scene: WASD + mouse look, ESC to exit.
class FreeCameraApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 free-camera", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineService& engine)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

    auto world() noexcept -> buddd::engine::World* override { return world_.get(); }

private:
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
    std::unique_ptr<buddd::engine::Entity> cube_entity_;
    buddd::engine::Entity camera_entity_;
};

} // namespace buddd::cmd::app
