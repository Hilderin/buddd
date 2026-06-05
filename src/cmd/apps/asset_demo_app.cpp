#include "apps/asset_demo_app.h"

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
#include "render/texture.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/entity.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

namespace be = buddd::engine;

auto buddd::cmd::app::AssetDemoApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    // 1. Create AssetManager
    auto am = be::AssetManager::create(device, "assets");
    if (!am) {
        std::cerr << "FATAL: could not create AssetManager: "
                  << be::to_string(am.error()) << "\n";
        return std::unexpected(am.error());
    }

    // 2. Load material from YAML
    auto mat_asset = (*am)->create<be::MaterialAsset>("materials/demo_cube");
    if (!mat_asset) {
        std::cerr << "FATAL: could not load material: "
                  << be::to_string(mat_asset.error()) << "\n";
        return std::unexpected(mat_asset.error());
    }
    auto material = (*mat_asset)->material();

    // 3. Create World, Entity, Camera
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
        0.1f,
        100.0f
    );
    entity.add_component<be::CameraComponent>(camera);

    // 4. Create vertex buffer with texture coordinates
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

    // 5. Create model
    auto model = be::Model::create_indexed(
        device, format,
        std::as_bytes(std::span(vertices)),
        std::as_bytes(std::span(indices)),
        be::IndexType::Uint16, material
    );
    if (!model) {
        std::cerr << "FATAL: Failed to create textured cube model: "
                  << be::to_string(model.error()) << "\n";
        return std::unexpected(model.error());
    }

    // 6. Attach to entity via MeshRenderer
    entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*model)));

    // 7. Create RenderSystem
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);

    entity_ = std::make_unique<be::Entity>(std::move(entity));
    start_time_ = std::chrono::steady_clock::now();

    return {};
}

auto buddd::cmd::app::AssetDemoApp::render(be::RenderDevice&, int) -> void {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
    float angle = elapsed_seconds * 0.5f;

    entity_->transform().rotation =
        be::math::Quat::angle_axis(angle, be::math::Vec3::unit_y());

    render_system_->render_scene();
}
