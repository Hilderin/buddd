#pragma once

#include "app.h"

#include "scene/entity.h"

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

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_frame_begin(buddd::engine::EngineContext const& ctx) -> void override;

    auto on_render(buddd::engine::EngineContext const&) -> void override {}

private:
    buddd::engine::Entity camera_entity_;
};

} // namespace buddd::cmd::app
