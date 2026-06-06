#pragma once

#include "app.h"

#include "render/material.h"
#include "render/model.h"

#include <memory>

namespace buddd::engine {
class EngineService;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// Coloured triangle: 120-frame render loop.
class TriangleApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 triangle", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineService& engine)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

private:
    buddd::engine::Model model_;
    std::shared_ptr<buddd::engine::Material> material_;
};

} // namespace buddd::cmd::app
