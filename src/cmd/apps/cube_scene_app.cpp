#include "apps/cube_scene_app.h"
#include "demo/demo_helpers.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/entity.h"

#include <chrono>
#include <memory>
#include <utility>

namespace be = buddd::engine;

auto buddd::cmd::app::CubeSceneApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
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

    // MeshRenderer with cube
    auto cube = demo::setup_cube(device);
    entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(cube.model)));

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
