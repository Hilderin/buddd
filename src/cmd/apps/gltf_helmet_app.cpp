#include "apps/gltf_helmet_app.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "asset/model_asset.h"
#include "engine_context.h"
#include "engine_service.h"
#include "scene/world.h"
#include "render/render_device.h"
#include "render/model_utils.h"
#include "scene/free_camera_movement.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"

#include <cstdlib>
#include <memory>

BUDDD_LOG_TAG("GltfHelmet");

namespace be = buddd::engine;

buddd::cmd::app::GltfHelmetApp::GltfHelmetApp() = default;
buddd::cmd::app::GltfHelmetApp::~GltfHelmetApp() = default;

auto buddd::cmd::app::GltfHelmetApp::setup(be::EngineContext const& ctx)
    -> be::Result<void>
{
    auto& device = ctx.device;

    // ── Camera — position/rotation from Transform, projection from CameraComponent ──
    camera_entity_ = ctx.world.add_entity();
    camera_entity_.transform().position = be::math::Vec3{0.0f, 1.5f, 3.0f};
    // Pitch to look at origin from (0, 1.5, 3):
    //   direction = (0, 0, 0) - (0, 1.5, 3) = (0, -1.5, -3)
    //   pitch = asin(-1.5 / sqrt(1.5^2 + 3^2)) ≈ -0.4636 rad
    camera_entity_.transform().rotation = be::math::Quat::from_euler(-0.4636f, 0.0f, 0.0f);
    auto& cam_comp = camera_entity_.add_component<be::CameraComponent>();
    cam_comp.set_perspective(be::math::radians(55.0f),
                             static_cast<float>(config().width) / static_cast<float>(config().height),
                             0.1f, 100.0f);

    // FreeCameraMovement on the camera entity
    camera_entity_.add_component<be::FreeCameraMovement>(0.0f, -0.4636f);

    // ── Directional light (white, intensity 1.5, pitch=-45°, yaw=45°) ──
    {
        auto light_entity = ctx.world.add_entity();
        light_entity.add_component<be::DirectionalLightComponent>(
            be::math::Color{1.0f, 1.0f, 1.0f},  // white
            1.5f                                 // intensity
        );
        light_entity.transform().rotation =
            be::math::Quat::from_euler(be::math::radians(-45.0f),
                                        be::math::radians(45.0f), 0.0f);
    }

    // ── Load DamagedHelmet ──
    auto model_asset = ctx.services.assets().create<be::ModelAsset>(
        "models/damaged-helmet/DamagedHelmet");
    if (!model_asset) {
        BUDDD_LOG_ERROR("Failed to load DamagedHelmet model: {}",
                        be::to_string(model_asset.error()));
        return make_error(model_asset);
    }

    auto& root = (*model_asset)->root_node();
    be::add_model_to_world(ctx.world, root);

    return {};
}
