#pragma once

#include "app.h"

#include "scene/entity.h"
#include "scene/world.h"
#include "render/render_system.h"

#include <chrono>
#include <memory>

namespace buddd::engine {
class AssetManager;
} // namespace buddd::engine

namespace buddd::cmd::app {

class AssetDemoApp final : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 asset-demo", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

private:
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
    std::unique_ptr<buddd::engine::Entity> entity_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
