#include "apps/cube_scene_app.h"

#include "engine_service.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "render/primitives.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "render/shader.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/entity.h"

#include <chrono>
#include <memory>
#include <string_view>
#include <utility>

namespace be = buddd::engine;

auto buddd::cmd::app::CubeSceneApp::setup(be::EngineService& engine)
    -> be::Result<void>
{
    auto& device = engine.device();

    world_ = std::make_unique<be::World>();

    // Create entity
    auto entity = be::Entity::create(*world_);

    // Camera component
    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(config().width) / static_cast<float>(config().height),
        0.1f,
        100.0f
    );
    entity.add_component<be::CameraComponent>(camera);

    // --- Create material with u_mvp ---
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
    auto shared_mat = std::shared_ptr<be::Material>(std::move(*mat));

    // --- Create cube and attach via MeshRenderer ---
    auto cube_result = be::create_cube(device, shared_mat);
    if (!cube_result) return std::unexpected(cube_result.error());
    entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*cube_result)));

    // RenderSystem
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);

    // Store entity (Entity is copyable, it's a handle)
    entity_ = std::make_unique<be::Entity>(std::move(entity));

    start_time_ = std::chrono::steady_clock::now();

    return {};
}

auto buddd::cmd::app::CubeSceneApp::render(be::RenderDevice&, int) -> void {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
    float angle = elapsed_seconds * 0.5f;

    entity_->transform().rotation =
        be::math::Quat::angle_axis(angle, be::math::Vec3::unit_y());

    render_system_->render_scene();
}
