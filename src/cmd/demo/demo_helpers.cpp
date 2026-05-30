#include "demo_helpers.h"

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
