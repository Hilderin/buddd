#include "apps/gltf_demo_app.h"

#include "asset/asset_manager.h"
#include "asset/model_asset.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/model_utils.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/entity.h"
#include "platform/platform.h"
#include "window/window.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace be = buddd::engine;

buddd::cmd::app::GltfDemoApp::GltfDemoApp() = default;
buddd::cmd::app::GltfDemoApp::~GltfDemoApp() = default;

auto buddd::cmd::app::GltfDemoApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    // Create AssetManager with base path at project root/assets
    // (relative to CWD which should be project root)
    std::string base_path = "assets";
    auto am_result = be::AssetManager::create(device, base_path);
    if (!am_result) {
        std::cerr << "Failed to create AssetManager: "
                  << be::to_string(am_result.error()) << "\n";
        return std::unexpected(am_result.error());
    }
    asset_manager_ = std::move(*am_result);

    world_ = std::make_unique<be::World>();

    // ── Camera ──
    camera_entity_ = std::make_unique<be::Entity>(be::Entity::create(*world_));
    be::math::Camera camera;
    camera_entity_->add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity_->get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{0.0f, 1.0f, 3.0f});
    cam.set_orientation(be::math::Quat::from_euler(0.0f, be::math::radians(180.0f), 0.0f));
    cam.set_perspective(be::math::radians(55.0f),
                        static_cast<float>(config().width) / static_cast<float>(config().height),
                        0.1f, 100.0f);

    // ── Directional light (from above-right) ──
    {
        auto light_entity = be::Entity::create(*world_);
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
    auto model_asset = asset_manager_->create<be::ModelAsset>("models/box/Box");
    if (!model_asset) {
        std::cerr << "Failed to load model: "
                  << be::to_string(model_asset.error()) << "\n";
        // Return error to abort
        return std::unexpected(model_asset.error());
    }

    auto& root = (*model_asset)->root_node();
    be::add_model_to_world(*world_, root);

    // ── Render system ──
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);

    return {};
}

auto buddd::cmd::app::GltfDemoApp::render(be::RenderDevice& device, int frame) -> void {
    // Y-rotation animation
    auto& cam = camera_entity_->get_component<be::CameraComponent>()->camera();
    float angle = static_cast<float>(frame) * 0.02f;
    cam.set_position(be::math::Vec3{
        3.0f * std::sin(angle),
        1.0f,
        3.0f * std::cos(angle)
    });
    cam.look_at(be::math::Vec3{0.0f, 0.0f, 0.0f});

    render_system_->render_scene();
}
