#pragma once

#include "app.h"

#include "scene/entity.h"
#include "scene/world.h"
#include "render/render_system.h"

#include <memory>

namespace buddd::engine {
class AssetManager;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// Hot-reload test app.
///
/// - Creates an AssetManager that loads a material from YAML
/// - At frame 30, swaps the source texture file on disk and triggers
///   poll_file_events() to verify the hot-reload pipeline works
/// - Run with dual captures to compare before/after:
///   buddd run hot-reload --frame 60 --capture 30:/tmp/before.png --capture 60:/tmp/after.png
class HotReloadApp final : public App {
public:
    HotReloadApp();
    ~HotReloadApp() override;

    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 hot-reload test", 1024, 768};
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
};

} // namespace buddd::cmd::app
