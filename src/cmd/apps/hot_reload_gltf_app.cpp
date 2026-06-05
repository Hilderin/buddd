#include "apps/hot_reload_gltf_app.h"

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
#include <filesystem>
#include <fstream>
#include <iostream>

namespace be = buddd::engine;
namespace fs = std::filesystem;

buddd::cmd::app::HotReloadGltfApp::HotReloadGltfApp() = default;
buddd::cmd::app::HotReloadGltfApp::~HotReloadGltfApp() = default;

auto buddd::cmd::app::HotReloadGltfApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    std::string base_path = "assets";
    auto am_result = be::AssetManager::create(device, base_path);
    if (!am_result) {
        return std::unexpected(am_result.error());
    }
    asset_manager_ = std::move(*am_result);

    world_ = std::make_unique<be::World>();

    // ── Camera ──
    auto camera_entity = be::Entity::create(*world_);
    be::math::Camera camera;
    camera_entity.add_component<be::CameraComponent>(camera);
    auto& cam = camera_entity.get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{0.0f, 1.0f, 3.0f});
    cam.look_at(be::math::Vec3{0.0f, 0.0f, 0.0f});
    cam.set_perspective(be::math::radians(55.0f),
                        static_cast<float>(config().width) / static_cast<float>(config().height),
                        0.1f, 100.0f);

    // ── Directional light (from above-right) ──
    {
        auto light_entity = be::Entity::create(*world_);
        light_entity.add_component<be::DirectionalLightComponent>(
            be::math::Vec3{1.0f, 1.0f, 1.0f}, 1.5f);
        light_entity.transform().rotation =
            be::math::Quat::from_euler(be::math::radians(-45.0f),
                                        be::math::radians(45.0f), 0.0f);
    }

    // ── Load model ──
    auto model_asset = asset_manager_->create<be::ModelAsset>("models/box/Box");
    if (!model_asset) {
        std::cerr << "Failed to load model: "
                  << be::to_string(model_asset.error()) << "\n";
        return std::unexpected(model_asset.error());
    }

    auto& root = (*model_asset)->root_node();
    be::add_model_to_world(*world_, root);

    // ── Render system ──
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);

    return {};
}

auto buddd::cmd::app::HotReloadGltfApp::on_frame_begin() -> void {
    // Touch the glTF source file at frame 0 to trigger a hot-reload
    // via the FileWatcher. The hot-reload replaces the ModelNode tree
    // in the ModelAsset cache. Entities created by add_model_to_world()
    // before the hot-reload still hold the previous model (Model is
    // move-only, consumed during traversal). After hot-reload, the
    // updated tree is available for new entities — existing entities
    // must be re-created to see the new model.
    if (!reload_triggered_ && frame_count_++ == 0) {
        reload_triggered_ = true;

        auto gltf_path = fs::path("assets/models/box/BoxTextured.gltf");
        if (fs::exists(gltf_path)) {
            auto now = fs::file_time_type::clock::now();
            fs::last_write_time(gltf_path, now);
            std::cerr << "[HotReloadGltf] Touched: " << gltf_path << "\n";

            asset_manager_->poll_file_events();
        }
    }
}

auto buddd::cmd::app::HotReloadGltfApp::render(be::RenderDevice& device, int frame) -> void {
    // Y-rotation animation so model is visible
    if (auto cam_opt = world_->active_camera()) {
        auto& cam = cam_opt->camera();
        float angle = static_cast<float>(frame) * 0.02f;
        cam.set_position(be::math::Vec3{
            3.0f * std::sin(angle),
            1.0f,
            3.0f * std::cos(angle)
        });
        cam.look_at(be::math::Vec3{0.0f, 0.0f, 0.0f});
    }

    render_system_->render_scene();
}
