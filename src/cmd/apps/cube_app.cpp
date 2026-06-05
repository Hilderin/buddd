#include "apps/cube_app.h"

#include "math/math.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "render/primitives.h"
#include "render/render_device.h"
#include "render/shader.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string_view>

namespace be = buddd::engine;

auto buddd::cmd::app::CubeApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    // --- Create material ---
    constexpr std::string_view k_vs = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_color;
        out vec3 v_color;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
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
    if (!vs) return std::unexpected(vs.error());
    auto fs = device.create_shader(be::ShaderType::Fragment, k_fs);
    if (!fs) return std::unexpected(fs.error());
    auto mat = device.create_material(std::move(*vs), std::move(*fs), {"u_mvp"});
    if (!mat) return std::unexpected(mat.error());
    material_ = std::shared_ptr<be::Material>(std::move(*mat));

    // --- Create cube model ---
    auto cube_result = be::create_cube(device, material_);
    if (!cube_result) return std::unexpected(cube_result.error());
    model_ = std::move(*cube_result);

    camera_.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    camera_.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(config().width) / static_cast<float>(config().height),
        0.1f,
        100.0f
    );

    start_time_ = std::chrono::steady_clock::now();

    return {};
}

auto buddd::cmd::app::CubeApp::render(be::RenderDevice& device, int) -> void {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
    float angle = elapsed_seconds * 0.5f;

    be::math::Mat4 model_matrix =
        be::math::Mat4::rotate(angle, be::math::Vec3::unit_y());
    be::math::Mat4 mvp =
        camera_.projection_matrix() * camera_.view_matrix() * model_matrix;

    auto uniform_result = material_->set_uniform("u_mvp", mvp);
    if (!uniform_result) {
        std::cerr << "Failed to set u_mvp uniform: "
                  << be::to_string(uniform_result.error()) << "\n";
    }
    model_.draw(device);
}
