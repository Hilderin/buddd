#pragma once

#include "app.h"

#include <memory>
#include <vector>

namespace buddd::engine {
class AssetManager;
class World;
class RenderSystem;
class Entity;
class RenderDevice;
class ModelNode;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// Hot-reload demo for glTF models.
///
/// Starts by rendering BoxTextured. At frame 30, the YAML source is rewritten
/// to point to DamagedHelmet (a visually distinct PBR model). At frame 70,
/// it swaps back to BoxTextured. The hot-reload is triggered via the
/// FileWatcher + poll_file_events mechanism.
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
    /// Reloads the model from the live YAML and creates corresponding entities.
    /// Destroys any existing model entities first.
    auto reload_model() -> void;

    /// Recursively creates entities for mesh nodes in the model tree.
    auto create_entities(buddd::engine::ModelNode& node) -> void;

    std::unique_ptr<buddd::engine::AssetManager> asset_manager_;
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
    std::vector<buddd::engine::Entity> model_entities_;
    int frame_count_ = 0;
};

} // namespace buddd::cmd::app
