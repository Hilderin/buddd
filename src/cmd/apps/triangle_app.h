#pragma once

#include "app.h"

#include "render/material.h"
#include "render/vertex_buffer.h"

#include <memory>

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::app {

/// Coloured triangle: 120-frame render loop.
class TriangleApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 triangle", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

private:
    std::unique_ptr<buddd::engine::Material> material_;
    std::unique_ptr<buddd::engine::VertexBuffer> vb_;
};

} // namespace buddd::cmd::app
