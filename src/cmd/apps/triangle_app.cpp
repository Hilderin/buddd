#include "apps/triangle_app.h"
#include "demo/demo_helpers.h"

#include "render/render_device.h"
#include "render/primitive_topology.h"

namespace be = buddd::engine;

auto buddd::cmd::app::TriangleApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    auto [material, vb] = demo::setup_triangle(device);
    material_ = std::move(material);
    vb_ = std::move(vb);
    return {};
}

auto buddd::cmd::app::TriangleApp::render(be::RenderDevice& device, int) -> void {
    device.draw(
        be::PrimitiveTopology::Triangles,
        *vb_, *material_, 3);
}
