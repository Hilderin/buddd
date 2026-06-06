#pragma once

#include "app.h"

#include <memory>

namespace buddd::engine {
class RenderDevice;
class World;
class RenderSystem;
class Entity;
class AssetManager;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// glTF Helmet demo: loads the DamagedHelmet model with free-camera controls.
/// Camera is auto-updated via the Updatable system (FreeCameraMovement).
class GltfHelmetApp final : public App {
public:
    GltfHelmetApp();
    ~GltfHelmetApp() override;

    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 glTF Helmet", 1280, 720};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineService& engine)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

    auto world() noexcept -> buddd::engine::World* override { return world_.get(); }

private:
    std::unique_ptr<buddd::engine::AssetManager> asset_manager_;
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::Entity> camera_entity_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
};

} // namespace buddd::cmd::app
