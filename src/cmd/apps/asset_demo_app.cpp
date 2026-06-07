#include "apps/asset_demo_app.h"

#include "log/log.h"

#include "asset/asset_manager.h"
#include "asset/material_asset.h"
#include "asset/texture_asset.h"
#include "engine_context.h"
#include "engine_service.h"
#include "scene/world.h"
#include "image/image.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "render/render_device.h"
#include "render/mesh_renderer.h"
#include "render/texture.h"
#include "scene/camera_component.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

BUDDD_LOG_TAG("AssetDemo");

namespace be = buddd::engine;

buddd::cmd::app::AssetDemoApp::AssetDemoApp() = default;
buddd::cmd::app::AssetDemoApp::~AssetDemoApp() = default;

auto buddd::cmd::app::AssetDemoApp::setup(be::EngineContext const& ctx)
    -> be::Result<void>
{
    auto& device = ctx.device;

    // 1. Load material from YAML using shared AssetManager
    auto mat_asset = ctx.services.assets().create<be::MaterialAsset>("materials/demo_cube");
    if (!mat_asset) {
        BUDDD_LOG_ERROR("FATAL: could not load material: {}",
                        be::to_string(mat_asset.error()));
        return make_error(mat_asset);
    }
    auto material = (*mat_asset)->material();

    // 2. Create Entity, Camera
    auto entity = ctx.world.add_entity();

    entity.transform().position = be::math::Vec3{3.0f, 2.0f, 3.0f};
    auto& cam_comp = entity.add_component<be::CameraComponent>();
    cam_comp.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    cam_comp.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(config().width) / static_cast<float>(config().height),
        0.1f,
        100.0f
    );

    // 3. Create vertex buffer with texture coordinates
    struct TexturedCubeVertex {
        float px, py, pz;
        float tx, ty;
    };

    const TexturedCubeVertex vertices[] = {
        // +X face (right)
        { 1.f, -1.f, -1.f,  0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 1.f },
        { 1.f,  1.f, -1.f,  0.f, 1.f },
        // -X face (left)
        {-1.f, -1.f, -1.f,  0.f, 0.f },
        {-1.f, -1.f,  1.f,  1.f, 0.f },
        {-1.f,  1.f,  1.f,  1.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f },
        // +Y face (top)
        {-1.f,  1.f,  1.f,  0.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f, -1.f,  1.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f },
        // -Y face (bottom)
        {-1.f, -1.f, -1.f,  0.f, 0.f },
        { 1.f, -1.f, -1.f,  1.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 1.f },
        {-1.f, -1.f,  1.f,  0.f, 1.f },
        // +Z face (front)
        {-1.f, -1.f,  1.f,  0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 1.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f },
        // -Z face (back)
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

    // 4. Create model
    auto model = be::Model::create_indexed(
        device, format,
        std::as_bytes(std::span(vertices)),
        std::as_bytes(std::span(indices)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 36, 0} },
        { material }
    );
    if (!model) {
        BUDDD_LOG_ERROR("FATAL: Failed to create textured cube model: {}",
                        be::to_string(model.error()));
        return make_error(model);
    }

    // 5. Attach to entity via MeshRenderer
    entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*model)));

    entity_ = entity;
    start_time_ = std::chrono::steady_clock::now();

    return {};
}

auto buddd::cmd::app::AssetDemoApp::on_frame_begin(be::EngineContext const& ctx) -> void {
    // Update rotation
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
    float angle = elapsed_seconds * 0.5f;

    entity_.transform().rotation =
        be::math::Quat::angle_axis(angle, be::math::Vec3::unit_y());
}
