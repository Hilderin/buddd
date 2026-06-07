#pragma once

#include "app.h"

#include "scene/entity.h"

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

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

private:
    buddd::engine::Entity camera_entity_;
};

} // namespace buddd::cmd::app
