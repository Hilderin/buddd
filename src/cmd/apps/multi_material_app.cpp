#include "apps/multi_material_app.h"

#include "log/log.h"

#include "engine_context.h"
#include "math/math.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "render/render_device.h"
#include "render/shader.h"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>

BUDDD_LOG_TAG("MultiMaterial");

namespace be = buddd::engine;

namespace {

struct ColoredVertex {
    float px, py, pz;
    float cx, cy, cz;
};

constexpr ColoredVertex k_vertices[24] = {
    // +X (red)
    { 1,-1,-1, 1,0,0 }, { 1, 1,-1, 1,0,0 }, { 1, 1, 1, 1,0,0 }, { 1,-1, 1, 1,0,0 },
    // -X (green)
    {-1,-1, 1, 0,1,0 }, {-1, 1, 1, 0,1,0 }, {-1, 1,-1, 0,1,0 }, {-1,-1,-1, 0,1,0 },
    // +Y (blue)
    {-1, 1, 1, 0,0,1 }, { 1, 1, 1, 0,0,1 }, { 1, 1,-1, 0,0,1 }, {-1, 1,-1, 0,0,1 },
    // -Y (yellow)
    {-1,-1,-1, 1,1,0 }, { 1,-1,-1, 1,1,0 }, { 1,-1, 1, 1,1,0 }, {-1,-1, 1, 1,1,0 },
    // +Z (cyan)
    { 1,-1, 1, 0,1,1 }, { 1, 1, 1, 0,1,1 }, {-1, 1, 1, 0,1,1 }, {-1,-1, 1, 0,1,1 },
    // -Z (magenta)
    {-1,-1,-1, 1,0,1 }, {-1, 1,-1, 1,0,1 }, { 1, 1,-1, 1,0,1 }, { 1,-1,-1, 1,0,1 },
};

constexpr uint16_t k_indices[36] = {
     0, 1, 2,  0, 2, 3,   4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11,  12,13,14, 12,14,15,
    16,17,18, 16,18,19,  20,21,22, 20,22,23,
};

// Vertex format: Float3 position + Float3 colour, stride 24
auto multi_mat_format() -> be::VertexFormat {
    return be::VertexFormat{
        24,
        {
            {0, be::VertexAttributeType::Float3, 0, false},
            {1, be::VertexAttributeType::Float3, static_cast<uint32_t>(sizeof(float) * 3), false},
        }
    };
}

} // anonymous namespace

auto buddd::cmd::app::MultiMaterialApp::setup(be::EngineContext const& ctx)
    -> be::Result<void>
{
    auto& device = ctx.device;
    // Create 3 materials (red, green, blue)
    auto red_vs = device.create_shader(be::ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }
    )");
    if (!red_vs) return make_error(red_vs);

    auto red_fs = device.create_shader(be::ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() { frag_color = vec4(1.0, 0.0, 0.0, 1.0); }
    )");
    if (!red_fs) return make_error(red_fs);

    auto red_mat = device.create_material(std::move(*red_vs), std::move(*red_fs), {"u_mvp"});

    auto green_vs = device.create_shader(be::ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }
    )");
    if (!green_vs) return make_error(green_vs);

    auto green_fs = device.create_shader(be::ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() { frag_color = vec4(0.0, 1.0, 0.0, 1.0); }
    )");
    if (!green_fs) return make_error(green_fs);

    auto green_mat = device.create_material(std::move(*green_vs), std::move(*green_fs), {"u_mvp"});

    auto blue_vs = device.create_shader(be::ShaderType::Vertex, R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        uniform mat4 u_mvp;
        void main() { gl_Position = u_mvp * vec4(a_position, 1.0); }
    )");
    if (!blue_vs) return make_error(blue_vs);

    auto blue_fs = device.create_shader(be::ShaderType::Fragment, R"(
        #version 450 core
        out vec4 frag_color;
        void main() { frag_color = vec4(0.0, 0.0, 1.0, 1.0); }
    )");
    if (!blue_fs) return make_error(blue_fs);

    auto blue_mat = device.create_material(std::move(*blue_vs), std::move(*blue_fs), {"u_mvp"});

    // Shared vertex/index data as bytes
    auto vertex_bytes = std::as_bytes(std::span{k_vertices});
    auto index_bytes = std::as_bytes(std::span{k_indices});

    // Convert unique_ptr<Material> to shared_ptr<Material>
    auto red_shared = std::shared_ptr<be::Material>(std::move(*red_mat));
    auto green_shared = std::shared_ptr<be::Material>(std::move(*green_mat));
    auto blue_shared = std::shared_ptr<be::Material>(std::move(*blue_mat));

    // 3 submeshes: 12 indices each (2 faces per submesh)
    auto model_result = be::Model::create_indexed(
        device, multi_mat_format(), vertex_bytes, index_bytes, be::IndexType::Uint16,
        {
            {0, 12, 0},   // first 2 faces → red
            {12, 12, 1},  // next 2 faces → green
            {24, 12, 2},  // last 2 faces → blue
        },
        {red_shared, green_shared, blue_shared}
    );
    if (!model_result) return make_error(model_result);
    model_ = std::move(*model_result);

    camera_.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    camera_.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(config().width) / static_cast<float>(config().height),
        0.1f, 100.0f
    );

    start_time_ = std::chrono::steady_clock::now();
    return {};
}

auto buddd::cmd::app::MultiMaterialApp::on_render(be::EngineContext const& ctx) -> void {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
    float angle = elapsed_seconds * 0.5f;

    be::math::Mat4 mvp = camera_.projection_matrix()
        * camera_.view_matrix()
        * be::math::Mat4::rotate(angle, be::math::Vec3::unit_y());

    // Set u_mvp on all materials
    for (auto& mat : model_.materials()) {
        if (mat) {
            auto r = mat->set_uniform("u_mvp", mvp);
            if (!r) {
                BUDDD_LOG_ERROR("u_mvp set failed: {}", be::to_string(r.error()));
            }
        }
    }

    model_.draw(ctx.device);
}
