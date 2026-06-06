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

/// glTF model loading demo: loads a glTF model from YAML via AssetManager
/// and renders it using add_model_to_world() with a free camera.
class GltfDemoApp final : public App {
public:
    GltfDemoApp();
    ~GltfDemoApp() override;

    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 glTF Demo", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineService& engine)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

private:
    std::unique_ptr<buddd::engine::AssetManager> asset_manager_;
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::Entity> camera_entity_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
};

} // namespace buddd::cmd::app
