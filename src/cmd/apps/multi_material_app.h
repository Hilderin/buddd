#pragma once
#include "app.h"
#include "math/camera.h"
#include "render/model.h"
#include <chrono>

namespace buddd::cmd::app {

class MultiMaterialApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine — multi-material", 1024, 768};
    }
    [[nodiscard]] auto setup(engine::EngineContext const& ctx) -> engine::Result<void> override;
    auto on_render(engine::EngineContext const& ctx) -> void override;
private:
    engine::Model model_;
    engine::math::Camera camera_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
