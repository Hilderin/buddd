#include "apps/cube_app.h"
#include "demo/demo_helpers.h"

#include "math/math.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "render/render_device.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>

namespace be = buddd::engine;

auto buddd::cmd::app::CubeApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    auto cube = demo::setup_cube(device);
    cube_ = std::make_unique<demo::CubeResources>(std::move(cube));

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

    auto uniform_result = cube_->material->set_uniform("u_mvp", mvp);
    if (!uniform_result) {
        std::cerr << "Failed to set u_mvp uniform: "
                  << be::to_string(uniform_result.error()) << "\n";
    }
    cube_->model.draw(device);
}
