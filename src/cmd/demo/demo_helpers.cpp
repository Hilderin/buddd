#include "demo_helpers.h"

#include "render/model.h"
#include "render/render_device.h"
#include "render/shader.h"
#include "render/material.h"
#include "render/vertex_buffer.h"
#include "render/vertex_format.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

namespace be = buddd::engine;
namespace bcd = buddd::cmd::demo;

auto bcd::setup_triangle(
    be::RenderDevice& device
) -> std::pair<
        std::unique_ptr<be::Material>,
        std::unique_ptr<be::VertexBuffer>>
{
    constexpr std::string_view vertex_source = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_color;
        out vec3 v_color;
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";

    constexpr std::string_view fragment_source = R"(
        #version 450 core
        in vec3 v_color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(v_color, 1.0);
        }
    )";

    auto vs = device.create_shader(be::ShaderType::Vertex, vertex_source);
    if (!vs) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(vs.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    auto fs = device.create_shader(be::ShaderType::Fragment, fragment_source);
    if (!fs) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(fs.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    auto material = device.create_material(std::move(*vs), std::move(*fs));
    if (!material) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(material.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    // Create vertex buffer: triangle with position (Float3) and color (Float3)
    struct Vertex { float x, y, z, r, g, b; };
    const Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f },  // top, red
        {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f },  // bottom-left, green
        { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f },  // bottom-right, blue
    };

    be::VertexFormat format;
    format.stride = sizeof(Vertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, static_cast<uint32_t>(offsetof(Vertex, x)), false},
        {1, be::VertexAttributeType::Float3, static_cast<uint32_t>(offsetof(Vertex, r)), false},
    };

    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vb = device.create_vertex_buffer(format, vertex_data);
    if (!vb) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(vb.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    return {std::move(*material), std::move(*vb)};
}

auto bcd::setup_cube(
    be::RenderDevice& device
) -> CubeResources
{
    // --- Vertex shader ---
    constexpr std::string_view k_cube_vs = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_color;
        out vec3 v_color;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";

    // --- Fragment shader ---
    constexpr std::string_view k_cube_fs = R"(
        #version 450 core
        in vec3 v_color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(v_color, 1.0);
        }
    )";

    // --- Create shaders ---
    auto vs = device.create_shader(be::ShaderType::Vertex, k_cube_vs);
    if (!vs) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(vs.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    auto fs = device.create_shader(be::ShaderType::Fragment, k_cube_fs);
    if (!fs) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(fs.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    // --- Create material ---
    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    if (!mat) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(mat.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    // Convert unique_ptr<Material> to shared_ptr<Material>
    std::shared_ptr<be::Material> shared_mat(std::move(*mat));

    // --- Vertex data: 24 vertices, stride 24 (Float3 position + Float3 color) ---
    struct CubeVertex { float px, py, pz, cr, cg, cb; };
    const CubeVertex vertices[] = {
        // +X face (right) - Red
        { 1.f, -1.f, -1.f,  1.f, 0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 0.f, 0.f },
        { 1.f,  1.f, -1.f,  1.f, 0.f, 0.f },
        // -X face (left) - Green
        {-1.f, -1.f, -1.f,  0.f, 1.f, 0.f },
        {-1.f, -1.f,  1.f,  0.f, 1.f, 0.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f, 0.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f, 0.f },
        // +Y face (top) - Blue
        {-1.f,  1.f,  1.f,  0.f, 0.f, 1.f },
        { 1.f,  1.f,  1.f,  0.f, 0.f, 1.f },
        { 1.f,  1.f, -1.f,  0.f, 0.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 0.f, 1.f },
        // -Y face (bottom) - Yellow
        {-1.f, -1.f, -1.f,  1.f, 1.f, 0.f },
        { 1.f, -1.f, -1.f,  1.f, 1.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 1.f, 0.f },
        {-1.f, -1.f,  1.f,  1.f, 1.f, 0.f },
        // +Z face (front) - Cyan
        {-1.f, -1.f,  1.f,  0.f, 1.f, 1.f },
        { 1.f, -1.f,  1.f,  0.f, 1.f, 1.f },
        { 1.f,  1.f,  1.f,  0.f, 1.f, 1.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f, 1.f },
        // -Z face (back) - Magenta
        { 1.f, -1.f, -1.f,  1.f, 0.f, 1.f },
        {-1.f, -1.f, -1.f,  1.f, 0.f, 1.f },
        {-1.f,  1.f, -1.f,  1.f, 0.f, 1.f },
        { 1.f,  1.f, -1.f,  1.f, 0.f, 1.f },
    };

    // --- Index data: 36 indices, Uint16, CCW winding ---
    const uint16_t indices[] = {
        // +X face
         0,  1,  2,   0,  2,  3,
        // -X face
         4,  5,  6,   4,  6,  7,
        // +Y face
         8,  9, 10,   8, 10, 11,
        // -Y face
        12, 13, 14,  12, 14, 15,
        // +Z face
        16, 17, 18,  16, 18, 19,
        // -Z face
        20, 21, 22,  20, 22, 23,
    };

    // --- Vertex format: stride=24, position at loc 0, color at loc 1 ---
    be::VertexFormat format;
    format.stride = sizeof(CubeVertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, 0, false},
        {1, be::VertexAttributeType::Float3,
            static_cast<uint32_t>(offsetof(CubeVertex, cr)), false},
    };

    auto vertex_data = std::as_bytes(std::span(vertices));
    auto index_data = std::as_bytes(std::span(indices));

    // --- Create indexed model ---
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16, shared_mat
    );
    if (!model) {
        std::fprintf(stderr, "FATAL: Failed to create cube model: %s\n",
                     be::to_string(model.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    return CubeResources{std::move(shared_mat), std::move(*model)};
}
