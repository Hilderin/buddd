#include "apps/hot_reload_gltf_app.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "asset/model_asset.h"
#include "engine_context.h"
#include "engine_service.h"
#include "scene/world.h"
#include "render/render_device.h"
#include "render/model_node.h"
#include "render/mesh_renderer.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/entity.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

BUDDD_LOG_TAG("HotReload");

namespace be = buddd::engine;
namespace fs = std::filesystem;

namespace {

const auto k_yaml_path = fs::path("assets/models/hot-reload/live.yaml");
const auto k_yaml_id = std::string("models/hot-reload/live");

auto write_yaml(float scale, const std::string& desc) -> void {
    std::ofstream file(k_yaml_path);
    file << "type: Model\nversion: 1\n"
         << "source: models/box/BoxTextured.gltf\n"
         << "settings:\n  scale: " << scale << "\n";
    file.close();
    auto now = fs::file_time_type::clock::now();
    fs::last_write_time(k_yaml_path, now);
    BUDDD_LOG_INFO("{} (scale={})", desc, scale);
}

} // anonymous namespace

buddd::cmd::app::HotReloadGltfApp::HotReloadGltfApp() = default;
buddd::cmd::app::HotReloadGltfApp::~HotReloadGltfApp() = default;

auto buddd::cmd::app::HotReloadGltfApp::setup(be::EngineContext const& ctx)
    -> be::Result<void>
{
    auto& device = ctx.device;
    // Store pointer to the shared AssetManager
    asset_manager_ = &ctx.services.assets();

    // Camera — position/rotation from Transform, projection from CameraComponent
    auto cam_entity = ctx.world.add_entity();
    cam_entity.transform().position = {0.0f, 1.0f, 3.0f};
    auto& cam_comp = cam_entity.add_component<be::CameraComponent>();
    cam_comp.look_at({0.0f, 0.0f, 0.0f});
    cam_comp.set_perspective(be::math::radians(55.0f),
                             static_cast<float>(config().width) / static_cast<float>(config().height),
                             0.1f, 100.0f);
    camera_entity_ = cam_entity;

    // Directional light
    {
        auto e = ctx.world.add_entity();
        e.add_component<be::DirectionalLightComponent>(be::math::Vec3{1,1,1}, 1.5f);
        e.transform().rotation = be::math::Quat::from_euler(
            be::math::radians(-45.0f), be::math::radians(45.0f), 0.0f);
    }

    // Start with scale 1.0
    write_yaml(1.0f, "Box scale=1.0");
    reload_model(ctx.world);

    BUDDD_LOG_INFO("Started \u2014 will scale to 2.0 at frame 30");
    return {};
}

auto buddd::cmd::app::HotReloadGltfApp::reload_model(be::World& world) -> void {
    // Destroy existing model entities
    for (auto& e : model_entities_) {
        if (e.id() != be::EntityId::none()) e.destroy();
    }
    model_entities_.clear();
    world.flush_destroyed();

    // Clear cache so the next create() loads from the updated YAML
    asset_manager_->clear();

    auto result = asset_manager_->create<be::ModelAsset>(k_yaml_id);
    if (!result) {
        BUDDD_LOG_ERROR("Load failed: {}", be::to_string(result.error()));
        return;
    }

    create_entities((*result)->root_node(), world);
    BUDDD_LOG_INFO("Reloaded: {} entities", model_entities_.size());
}

auto buddd::cmd::app::HotReloadGltfApp::create_entities(be::ModelNode& node, be::World& world) -> void {
    for (auto& child : node.children) {
        if (child.model) {
            auto e = world.add_entity();
            e.transform().position = child.translation;
            e.transform().rotation = child.rotation;
            e.transform().scale = child.scale;
            e.add_component<be::MeshRenderer>(child.model);
            model_entities_.push_back(e);
        }
        create_entities(child, world);
    }
}

auto buddd::cmd::app::HotReloadGltfApp::on_frame_begin(be::EngineContext const& ctx) -> void {
    if (ctx.frame == 30) {
        write_yaml(2.0f, "Box scale=2.0 (bigger)");
        // Write the YAML, then force a direct reload (bypass FileWatcher
        // timing issues — just clear and reload from the updated file).
        asset_manager_->poll_file_events();
        reload_model(ctx.world);
        BUDDD_LOG_INFO("\u2192 Box is now 2x bigger (scale=2.0)");
    }

    // Camera animation
    if (auto cam_opt = ctx.world.active_camera()) {
        auto& cc = *cam_opt;
        float a = static_cast<float>(ctx.frame) * 0.02f;
        cc.entity().transform().position = {3.0f * std::sin(a), 1.0f, 3.0f * std::cos(a)};
        cc.look_at({0.0f, 0.0f, 0.0f});
    }
}
