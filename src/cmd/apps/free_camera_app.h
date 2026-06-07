#pragma once

#include "app.h"

#include "scene/entity.h"

namespace buddd::cmd::app {

/// Interactive free camera scene: WASD + mouse look, ESC to exit.
class FreeCameraApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 free-camera", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

private:
    buddd::engine::Entity cube_entity_;
    buddd::engine::Entity camera_entity_;
};

} // namespace buddd::cmd::app
