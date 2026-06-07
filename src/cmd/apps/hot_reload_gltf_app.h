#pragma once

#include "app.h"

#include "asset/asset_manager.h"
#include "render/model_node.h"
#include "scene/entity.h"
#include "scene/world.h"

#include <vector>

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

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_frame_begin(buddd::engine::EngineContext const& ctx) -> void override;

    auto on_render(buddd::engine::EngineContext const&) -> void override {}

private:
    /// Reloads the model from the live YAML and creates corresponding entities.
    /// Destroys any existing model entities first.
    auto reload_model(buddd::engine::World& world) -> void;

    /// Recursively creates entities for mesh nodes in the model tree.
    auto create_entities(buddd::engine::ModelNode& node, buddd::engine::World& world) -> void;

    buddd::engine::Entity camera_entity_;
    buddd::engine::AssetManager* asset_manager_ = nullptr;
    std::vector<buddd::engine::Entity> model_entities_;
};

} // namespace buddd::cmd::app
