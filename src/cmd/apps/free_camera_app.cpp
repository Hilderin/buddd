#include "apps/free_camera_app.h"

#include "engine_context.h"
#include "scene/world.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "render/primitives.h"
#include "render/render_device.h"
#include "render/mesh_renderer.h"
#include "render/shader.h"
#include "scene/camera_component.h"
#include "scene/free_camera_movement.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

namespace be = buddd::engine;

auto buddd::cmd::app::FreeCameraApp::setup(be::EngineContext const& ctx)
    -> be::Result<void>
{
    auto& device = ctx.device;

    // Camera entity
    camera_entity_ = ctx.world.add_entity();
    be::math::Camera camera;
    camera_entity_.add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{0.0f, 2.0f, 5.0f});
    cam.set_orientation(be::math::Quat::from_euler(0.0f, 0.0f, 0.0f));
    cam.set_perspective(be::math::radians(60.0f),
                        static_cast<float>(config().width) / static_cast<float>(config().height),
                        0.1f, 100.0f);

    // FreeCameraMovement (default yaw=0, pitch=0 matches identity orientation)
    camera_entity_.add_component<be::FreeCameraMovement>();

    // --- Create cube material ---
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
    if (!vs) return make_error(vs);
    auto fs = device.create_shader(be::ShaderType::Fragment, k_fs);
    if (!fs) return make_error(fs);
    auto mat = device.create_material(std::move(*vs), std::move(*fs), {"u_mvp"});
    if (!mat) return make_error(mat);
    auto shared_mat = std::shared_ptr<be::Material>(std::move(*mat));

    // --- Create cube via primitive helper ---
    auto cube_result = be::create_cube(device, shared_mat);
    if (!cube_result) return make_error(cube_result);
    auto cube_entity = ctx.world.add_entity();
    cube_entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*cube_result)));
    cube_entity_ = cube_entity;

    return {};
}
