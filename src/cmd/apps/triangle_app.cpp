#include "apps/triangle_app.h"

#include "engine_context.h"
#include "render/primitives.h"
#include "render/render_device.h"
#include "render/shader.h"

#include <memory>
#include <string_view>

namespace be = buddd::engine;

auto buddd::cmd::app::TriangleApp::setup(be::EngineContext const& ctx)
    -> be::Result<void>
{
    auto& device = ctx.device;

    // --- Create material ---
    constexpr std::string_view k_vs = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_color;
        out vec3 v_color;
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";

    constexpr std::string_view k_fs = R"(
        #version 450 core
        in vec3 v_color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(v_color, 1.0);
        }
    )";

    auto vs = device.create_shader(be::ShaderType::Vertex, k_vs);
    if (!vs) return make_error(vs);
    auto fs = device.create_shader(be::ShaderType::Fragment, k_fs);
    if (!fs) return make_error(fs);
    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    if (!mat) return make_error(mat);
    material_ = std::shared_ptr<be::Material>(std::move(*mat));

    // --- Create triangle model ---
    auto tri_result = be::create_triangle(device, material_);
    if (!tri_result) return make_error(tri_result);
    model_ = std::move(*tri_result);

    return {};
}

auto buddd::cmd::app::TriangleApp::on_render(be::EngineContext const& ctx) -> void {
    model_.draw(ctx.device);
}
