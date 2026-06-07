#include "apps/hot_reload_app.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "asset/material_asset.h"
#include "asset/texture_asset.h"
#include "engine_context.h"
#include "engine_service.h"
#include "scene/world.h"
#include "image/image.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "render/render_device.h"
#include "render/mesh_renderer.h"
#include "render/model.h"
#include "render/texture.h"
#include "scene/camera_component.h"
#include "scene/transform.h"

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

auto buddd::cmd::app::HotReloadApp::setup(be::EngineContext const& ctx)
    -> be::Result<void>
{
    auto& device = ctx.device;
    // 1. Initialize live texture as copy of texture A (red checker)
    std::filesystem::copy(
        "assets/textures/hot_reload_a.png",
        "assets/textures/hot_reload_live.png",
        std::filesystem::copy_options::overwrite_existing);
    BUDDD_LOG_INFO("Initial texture: hot_reload_a.png -> hot_reload_live.png");

    // 2. Load material from YAML using shared AssetManager (will load hot_reload_live.png = texture A)
    auto mat_asset = ctx.services.assets().create<be::MaterialAsset>("materials/hot_reload_test");
    if (!mat_asset) {
        BUDDD_LOG_ERROR("FATAL: could not load material: {}",
                        be::to_string(mat_asset.error()));
        return make_error(mat_asset);
    }
    auto material = (*mat_asset)->material();

    // 3. Create Entity + Camera
    auto entity = ctx.world.add_entity();

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

    // 4. Create cube mesh with UVs
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
        return make_error(model);
    }

    entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*model)));

    entity_ = entity;

    BUDDD_LOG_INFO("Setup complete. Will swap texture at frame 30.");
    return {};
}

auto buddd::cmd::app::HotReloadApp::on_frame_begin(be::EngineContext const& ctx) -> void {
    // At frame 30: swap texture B over the live file.
    if (ctx.frame == 30) {
        BUDDD_LOG_INFO("Frame 30: swapping texture...");
        std::filesystem::copy(
            "assets/textures/hot_reload_b.png",
            "assets/textures/hot_reload_live.png",
            std::filesystem::copy_options::overwrite_existing);
        ctx.services.assets().poll_file_events();  // immediate reload
        BUDDD_LOG_INFO("Texture swapped and poll_file_events() called.");
    }

    // Rotate cube
    float angle_deg = static_cast<float>(ctx.frame) * 3.0f;
    float angle_rad = be::math::radians(angle_deg);
    entity_.transform().rotation = be::math::Quat::angle_axis(angle_rad, be::math::Vec3::unit_y());
}
