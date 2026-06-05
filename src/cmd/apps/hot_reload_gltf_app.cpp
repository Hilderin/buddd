#include "apps/hot_reload_gltf_app.h"

#include "asset/asset_manager.h"
#include "asset/model_asset.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/model_node.h"
#include "render/mesh_renderer.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/entity.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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
    std::cerr << "[HotReload] " << desc << " (scale=" << scale << ")\n";
}

} // anonymous namespace

buddd::cmd::app::HotReloadGltfApp::HotReloadGltfApp() = default;
buddd::cmd::app::HotReloadGltfApp::~HotReloadGltfApp() = default;

auto buddd::cmd::app::HotReloadGltfApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    auto am_result = be::AssetManager::create(device, "assets");
    if (!am_result) return std::unexpected(am_result.error());
    asset_manager_ = std::move(*am_result);

    world_ = std::make_unique<be::World>();

    // Camera
    auto cam_entity = be::Entity::create(*world_);
    cam_entity.add_component<be::CameraComponent>(be::math::Camera{});
    auto& cam = cam_entity.get_component<be::CameraComponent>()->camera();
    cam.set_position({0.0f, 1.0f, 3.0f});
    cam.look_at({0.0f, 0.0f, 0.0f});
    cam.set_perspective(be::math::radians(55.0f),
                        static_cast<float>(config().width) / static_cast<float>(config().height),
                        0.1f, 100.0f);

    // Directional light
    {
        auto e = be::Entity::create(*world_);
        e.add_component<be::DirectionalLightComponent>(be::math::Vec3{1,1,1}, 1.5f);
        e.transform().rotation = be::math::Quat::from_euler(
            be::math::radians(-45.0f), be::math::radians(45.0f), 0.0f);
    }

    // Start with scale 1.0
    write_yaml(1.0f, "Box scale=1.0");
    reload_model();

    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);
    std::cerr << "[HotReload] Started \u2014 will scale to 2.0 at frame 30\n";
    return {};
}

auto buddd::cmd::app::HotReloadGltfApp::reload_model() -> void {
    // Destroy existing model entities
    for (auto& e : model_entities_) {
        if (e.id() != be::EntityId::none()) e.destroy();
    }
    model_entities_.clear();
    world_->flush_destroyed();

    // Clear cache so the next create() loads from the updated YAML
    asset_manager_->clear();

    auto result = asset_manager_->create<be::ModelAsset>(k_yaml_id);
    if (!result) {
        std::cerr << "[HotReload] Load failed: " << be::to_string(result.error()) << "\n";
        return;
    }

    create_entities((*result)->root_node());
    std::cerr << "[HotReload] Reloaded: " << model_entities_.size() << " entities\n";
}

auto buddd::cmd::app::HotReloadGltfApp::create_entities(be::ModelNode& node) -> void {
    for (auto& child : node.children) {
        if (child.model.has_value()) {
            auto e = be::Entity::create(*world_);
            e.transform().position = child.translation;
            e.transform().rotation = child.rotation;
            e.transform().scale = child.scale;
            auto model_ptr = std::make_shared<be::Model>(std::move(*child.model));
            e.add_component<be::MeshRenderer>(std::move(model_ptr));
            model_entities_.push_back(e);
        }
        create_entities(child);
    }
}

auto buddd::cmd::app::HotReloadGltfApp::on_frame_begin() -> void {
    if (frame_count_ == 30) {
        write_yaml(2.0f, "Box scale=2.0 (bigger)");
        // Write the YAML, then force a direct reload (bypass FileWatcher
        // timing issues — just clear and reload from the updated file).
        asset_manager_->poll_file_events();
        reload_model();
        std::cerr << "[HotReload] \u2192 Box is now 2x bigger (scale=2.0)\n";
    }

    ++frame_count_;
}

auto buddd::cmd::app::HotReloadGltfApp::render(be::RenderDevice& device, int frame) -> void {
    if (auto cam_opt = world_->active_camera()) {
        auto& cam = cam_opt->camera();
        float a = static_cast<float>(frame) * 0.02f;
        cam.set_position({3.0f * std::sin(a), 1.0f, 3.0f * std::cos(a)});
        cam.look_at({0.0f, 0.0f, 0.0f});
    }
    render_system_->render_scene();
}
