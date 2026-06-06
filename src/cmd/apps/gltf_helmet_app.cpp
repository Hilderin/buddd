#include "apps/gltf_helmet_app.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "asset/model_asset.h"
#include "engine_service.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/model_utils.h"
#include "scene/free_camera_movement.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"

#include <cstdlib>
#include <memory>

BUDDD_LOG_TAG("GltfHelmet");

namespace be = buddd::engine;

buddd::cmd::app::GltfHelmetApp::GltfHelmetApp() = default;
buddd::cmd::app::GltfHelmetApp::~GltfHelmetApp() = default;

auto buddd::cmd::app::GltfHelmetApp::setup(be::EngineService& engine)
    -> be::Result<void>
{
    auto& device = engine.device();
    // AssetManager
    std::string base_path = "assets";
    auto am_result = be::AssetManager::create(device, base_path);
    if (!am_result) {
        BUDDD_LOG_ERROR("Failed to create AssetManager: {}",
                        be::to_string(am_result.error()));
        return std::unexpected(am_result.error());
    }
    asset_manager_ = std::move(*am_result);

    world_ = std::make_unique<be::World>();

    // ── Camera ──
    camera_entity_ = std::make_unique<be::Entity>(be::Entity::create(*world_));
    be::math::Camera camera;
    camera_entity_->add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity_->get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{0.0f, 1.5f, 3.0f});
    // Pitch to look at origin from (0, 1.5, 3):
    //   direction = (0, 0, 0) - (0, 1.5, 3) = (0, -1.5, -3)
    //   pitch = asin(-1.5 / sqrt(1.5^2 + 3^2)) ≈ -0.4636 rad
    cam.set_orientation(be::math::Quat::from_euler(-0.4636f, 0.0f, 0.0f));
    cam.set_perspective(be::math::radians(55.0f),
                        static_cast<float>(config().width) / static_cast<float>(config().height),
                        0.1f, 100.0f);

    // FreeCameraMovement on the camera entity
    camera_entity_->add_component<be::FreeCameraMovement>(0.0f, -0.4636f);

    // ── Directional light (white, intensity 1.5, pitch=-45°, yaw=45°) ──
    {
        auto light_entity = be::Entity::create(*world_);
        light_entity.add_component<be::DirectionalLightComponent>(
            be::math::Vec3{1.0f, 1.0f, 1.0f},  // white
            1.5f                                 // intensity
        );
        light_entity.transform().rotation =
            be::math::Quat::from_euler(be::math::radians(-45.0f),
                                        be::math::radians(45.0f), 0.0f);
    }

    // ── Load DamagedHelmet ──
    auto model_asset = asset_manager_->create<be::ModelAsset>(
        "models/damaged-helmet/DamagedHelmet");
    if (!model_asset) {
        BUDDD_LOG_ERROR("Failed to load DamagedHelmet model: {}",
                        be::to_string(model_asset.error()));
        return std::unexpected(model_asset.error());
    }

    auto& root = (*model_asset)->root_node();
    be::add_model_to_world(*world_, root);

    // ── Render system ──
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);

    return {};
}

auto buddd::cmd::app::GltfHelmetApp::render(be::RenderDevice&, int) -> void {
    // Camera is auto-updated via World::update_updatables() in run_app()
    render_system_->render_scene();
}
