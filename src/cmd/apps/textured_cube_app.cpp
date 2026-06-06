#include "apps/textured_cube_app.h"

#include "log/log.h"

#include "engine_service.h"
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

BUDDD_LOG_TAG("TexturedCube");

namespace be = buddd::engine;

auto buddd::cmd::app::TexturedCubeApp::setup(be::EngineService& engine)
    -> be::Result<void>
{
    auto& device = engine.device();
    // 1. Load texture
    auto image_result = be::Image::load("assets/brick.png");
    if (!image_result) {
        BUDDD_LOG_ERROR("FATAL: could not load assets/brick.png: {}",
                        be::to_string(image_result.error()));
        return std::unexpected(image_result.error());
    }

    // 2. Create texture
    auto texture_result = device.create_texture(*image_result);
    if (!texture_result) {
        BUDDD_LOG_ERROR("FATAL: could not create texture: {}",
                        be::to_string(texture_result.error()));
        return std::unexpected(texture_result.error());
    }
    std::shared_ptr<be::Texture> texture(std::move(*texture_result));

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

    // 5. Create shaders
    constexpr std::string_view k_vertex_source = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec2 a_texcoord;
        out vec2 v_texcoord;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
            v_texcoord = a_texcoord;
        }
    )";

    constexpr std::string_view k_fragment_source = R"(
        #version 450 core
        in vec2 v_texcoord;
        out vec4 frag_color;
        uniform sampler2D u_tex;
        void main() {
            frag_color = texture(u_tex, v_texcoord);
        }
    )";

    auto vs = device.create_shader(be::ShaderType::Vertex, k_vertex_source);
    if (!vs) {
        BUDDD_LOG_ERROR("FATAL: {}", be::to_string(vs.error()));
        return std::unexpected(vs.error());
    }

    auto fs = device.create_shader(be::ShaderType::Fragment, k_fragment_source);
    if (!fs) {
        BUDDD_LOG_ERROR("FATAL: {}", be::to_string(fs.error()));
        return std::unexpected(fs.error());
    }

    // 6. Create material
    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    if (!mat) {
        BUDDD_LOG_ERROR("FATAL: {}", be::to_string(mat.error()));
        return std::unexpected(mat.error());
    }
    std::shared_ptr<be::Material> shared_mat(std::move(*mat));

    // 7. Set texture on material
    auto tex_result = shared_mat->set_texture("u_tex", texture);
    if (!tex_result) {
        BUDDD_LOG_ERROR("FATAL: set_texture failed: {}",
                        be::to_string(tex_result.error()));
        return std::unexpected(tex_result.error());
    }

    // 8. Create model
    auto model = be::Model::create_indexed(
        device, format,
        std::as_bytes(std::span(vertices)),
        std::as_bytes(std::span(indices)),
        be::IndexType::Uint16,
        { be::SubMesh{0, 36, 0} },
        { shared_mat }
    );
    if (!model) {
        BUDDD_LOG_ERROR("FATAL: Failed to create textured cube model: {}",
                        be::to_string(model.error()));
        return std::unexpected(model.error());
    }

    // 9. Attach to entity via MeshRenderer
    entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*model)));

    // 10. Create RenderSystem
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);

    entity_ = std::make_unique<be::Entity>(std::move(entity));
    start_time_ = std::chrono::steady_clock::now();

    return {};
}

auto buddd::cmd::app::TexturedCubeApp::render(be::RenderDevice&, int) -> void {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
    float angle = elapsed_seconds * 0.5f;

    entity_->transform().rotation =
        be::math::Quat::angle_axis(angle, be::math::Vec3::unit_y());

    render_system_->render_scene();
}
