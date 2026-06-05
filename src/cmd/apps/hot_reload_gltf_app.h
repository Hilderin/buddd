#pragma once

#include "app.h"

#include <memory>

namespace buddd::engine {
class AssetManager;
class World;
class RenderSystem;
class Entity;
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// Hot-reload test app for glTF models.
///
/// Loads a model from YAML. At frame 30, copies a different model file over
/// the source, triggers poll_file_events(), and verifies the model updates.
/// Renders for 60+ frames.
class HotReloadGltfApp final : public App {
public:
    HotReloadGltfApp();
    ~HotReloadGltfApp() override;

    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 hot-reload glTF test", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> override;

    auto on_frame_begin() -> void override;
    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

private:
    std::unique_ptr<buddd::engine::AssetManager> asset_manager_;
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
    std::unique_ptr<buddd::engine::Entity> entity_;
    int frame_count_ = 0;
    bool reload_triggered_ = false;
};

} // namespace buddd::cmd::app
