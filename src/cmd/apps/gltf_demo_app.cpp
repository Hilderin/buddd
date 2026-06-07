#include "apps/gltf_demo_app.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "asset/model_asset.h"
#include "engine_context.h"
#include "engine_service.h"
#include "scene/world.h"
#include "render/render_device.h"
#include "render/model_utils.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/entity.h"
#include "platform/platform.h"
#include "window/window.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

BUDDD_LOG_TAG("GltfDemo");

namespace be = buddd::engine;

buddd::cmd::app::GltfDemoApp::GltfDemoApp() = default;
buddd::cmd::app::GltfDemoApp::~GltfDemoApp() = default;

auto buddd::cmd::app::GltfDemoApp::setup(be::EngineContext const& ctx)
    -> be::Result<void>
{
    auto& device = ctx.device;
    // AssetManager is available from EngineService via ctx.services.assets()
    std::string base_path = "assets";

    // ── Camera ──
    camera_entity_ = ctx.world.add_entity();
    be::math::Camera camera;
    camera_entity_.add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{0.0f, 1.0f, 3.0f});
    cam.set_orientation(be::math::Quat::from_euler(0.0f, be::math::radians(180.0f), 0.0f));
    cam.set_perspective(be::math::radians(55.0f),
                        static_cast<float>(config().width) / static_cast<float>(config().height),
                        0.1f, 100.0f);

    // ── Directional light (from above-right) ──
    {
        auto light_entity = ctx.world.add_entity();
        light_entity.add_component<be::DirectionalLightComponent>(
            be::math::Vec3{1.0f, 1.0f, 1.0f},  // white
            1.5f                                 // intensity
        );
        // -Z forward, so pitch=-45 (down), yaw=45 (right-forward)
        light_entity.transform().rotation =
            be::math::Quat::from_euler(be::math::radians(-45.0f),
                                        be::math::radians(45.0f), 0.0f);
    }

    // ── Load model ──
    auto model_asset = ctx.services.assets().create<be::ModelAsset>("models/box/Box");
    if (!model_asset) {
        BUDDD_LOG_ERROR("Failed to load model: {}",
                        be::to_string(model_asset.error()));
        return make_error(model_asset);
    }

    auto& root = (*model_asset)->root_node();
    be::add_model_to_world(ctx.world, root);

    return {};
}

auto buddd::cmd::app::GltfDemoApp::on_frame_begin(be::EngineContext const& ctx) -> void {
    // Y-rotation animation
    auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();
    float angle = static_cast<float>(ctx.frame) * 0.02f;
    cam.set_position(be::math::Vec3{
        3.0f * std::sin(angle),
        1.0f,
        3.0f * std::cos(angle)
    });
    cam.look_at(be::math::Vec3{0.0f, 0.0f, 0.0f});
}
