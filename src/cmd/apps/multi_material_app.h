#pragma once
#include "app.h"
#include "math/camera.h"
#include "render/model.h"
#include <chrono>
#include <memory>
#include <vector>

namespace buddd::engine { class EngineService; }
namespace buddd::cmd::app {

class MultiMaterialApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine — multi-material", 1024, 768};
    }
    [[nodiscard]] auto setup(engine::EngineService& engine) -> engine::Result<void> override;
    auto render(engine::RenderDevice& device, int frame) -> void override;
private:
    engine::Model model_;
    engine::math::Camera camera_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
