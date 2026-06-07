#pragma once

#include "app.h"

#include "render/material.h"
#include "render/model.h"

#include <memory>

namespace buddd::cmd::app {

/// Coloured triangle: 120-frame render loop.
class TriangleApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 triangle", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

private:
    buddd::engine::Model model_;
    std::shared_ptr<buddd::engine::Material> material_;
};

} // namespace buddd::cmd::app
