#include "apps/hot_reload_app.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "asset/material_asset.h"
#include "asset/texture_asset.h"
#include "image/image.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "render/model.h"
#include "render/texture.h"
#include "scene/camera_component.h"
#include "scene/entity.h"
#include "scene/transform.h"
#include "scene/world.h"

#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

BUDDD_LOG_TAG("HotReload");

namespace be = buddd::engine;

buddd::cmd::app::HotReloadApp::HotReloadApp() = default;
buddd::cmd::app::HotReloadApp::~HotReloadApp() = default;

auto buddd::cmd::app::HotReloadApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    // 1. Initialize live texture as copy of texture A (red checker)
    std::filesystem::copy(
        "assets/textures/hot_reload_a.png",
        "assets/textures/hot_reload_live.png",
        std::filesystem::copy_options::overwrite_existing);
    BUDDD_LOG_INFO("Initial texture: hot_reload_a.png -> hot_reload_live.png");

    // 2. Create AssetManager
    auto am = be::AssetManager::create(device, "assets");
    if (!am) {
        BUDDD_LOG_ERROR("FATAL: could not create AssetManager: {}",
                        be::to_string(am.error()));
        return std::unexpected(am.error());
    }

    // 3. Load material from YAML (will load hot_reload_live.png = texture A)
    auto mat_asset = (*am)->create<be::MaterialAsset>("materials/hot_reload_test");
    if (!mat_asset) {
        BUDDD_LOG_ERROR("FATAL: could not load material: {}",
                        be::to_string(mat_asset.error()));
        return std::unexpected(mat_asset.error());
    }
    auto material = (*mat_asset)->material();
    asset_manager_ = std::move(*am);

    // 4. Create World + Entity + Camera
    world_ = std::make_unique<be::World>();
    auto entity = be::Entity::create(*world_);

    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(config().width) / static_cast<float>(config().height),
        0.1f, 100.0f
    );
    entity.add_component<be::CameraComponent>(camera);

    // 5. Create cube mesh with UVs
    struct TexturedCubeVertex {
        float px, py, pz;
        float tx, ty;
    };

    const TexturedCubeVertex vertices[] = {
        { 1.f, -1.f, -1.f,  0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 1.f },
        { 1.f,  1.f, -1.f,  0.f, 1.f },
        {-1.f, -1.f, -1.f,  0.f, 0.f },
        {-1.f, -1.f,  1.f,  1.f, 0.f },
        {-1.f,  1.f,  1.f,  1.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f },
        {-1.f,  1.f,  1.f,  0.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f, -1.f,  1.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f },
        {-1.f, -1.f, -1.f,  0.f, 0.f },
        { 1.f, -1.f, -1.f,  1.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 1.f },
        {-1.f, -1.f,  1.f,  0.f, 1.f },
        {-1.f, -1.f,  1.f,  0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 1.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f },
        { 1.f, -1.f, -1.f,  0.f, 0.f },
        {-1.f, -1.f, -1.f,  1.f, 0.f },
        {-1.f,  1.f, -1.f,  1.f, 1.f },
        { 1.f,  1.f, -1.f,  0.f, 1.f },
    };

    const uint16_t indices[] = {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23,
    };

    be::VertexFormat format;
    format.stride = sizeof(TexturedCubeVertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, 0, false},
        {1, be::VertexAttributeType::Float2,
            static_cast<uint32_t>(offsetof(TexturedCubeVertex, tx)), false},
    };

    auto model = be::Model::create_indexed(
        device, format,
        std::as_bytes(std::span(vertices)),
        std::as_bytes(std::span(indices)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 36, 0} },
        { material }
    );
    if (!model) {
        BUDDD_LOG_ERROR("FATAL: could not create cube model");
        return std::unexpected(model.error());
    }

    entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*model)));

    // 6. Render system
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);
    entity_ = std::make_unique<be::Entity>(std::move(entity));

    BUDDD_LOG_INFO("Setup complete. Will swap texture at frame 30.");
    return {};
}

auto buddd::cmd::app::HotReloadApp::on_frame_begin() -> void {
    asset_manager_->poll_file_events();
}

auto buddd::cmd::app::HotReloadApp::render(be::RenderDevice& /*device*/, int frame) -> void {
    // At frame 30: swap texture B over the live file.
    // on_frame_begin() will call poll_file_events() before render.
    if (frame == 30) {
        BUDDD_LOG_INFO("Frame 30: swapping texture...");
        std::filesystem::copy(
            "assets/textures/hot_reload_b.png",
            "assets/textures/hot_reload_live.png",
            std::filesystem::copy_options::overwrite_existing);
        // Note: on_frame_begin() already called before this render(),
        // so poll_file_events() will be called NEXT frame.
        // We call it manually now to trigger immediate reload.
        asset_manager_->poll_file_events();
        BUDDD_LOG_INFO("Texture swapped and poll_file_events() called.");
    }

    // Rotate cube
    float angle_deg = static_cast<float>(frame) * 3.0f;
    float angle_rad = be::math::radians(angle_deg);
    entity_->transform().rotation = be::math::Quat::angle_axis(angle_rad, be::math::Vec3::unit_y());

    render_system_->render_scene();
}
