#include "render/primitives.h"
#include "render/render_device.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace be = buddd::engine;

// ============================================================================
// Vertex format: Float3 position (loc 0) + Float3 colour (loc 1), stride 24
// ============================================================================
namespace {

/// Returns the vertex format used by all primitives: Float3 position + Float3 colour.
auto primitive_format() -> be::VertexFormat {
    return be::VertexFormat{
        24,
        {
            {0, be::VertexAttributeType::Float3, 0, false},
            {1, be::VertexAttributeType::Float3, static_cast<uint32_t>(sizeof(float) * 3), false},
        }
    };
}

} // anonymous namespace

// ============================================================================
// create_cube
// ============================================================================
auto be::create_cube(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>
{
    // 24 vertices: position (Float3) + colour (Float3) = 6 floats each
    constexpr float verts[144] = {
        // +X face (right) - Red
         1.f, -1.f, -1.f,  1.f, 0.f, 0.f,
         1.f, -1.f,  1.f,  1.f, 0.f, 0.f,
         1.f,  1.f,  1.f,  1.f, 0.f, 0.f,
         1.f,  1.f, -1.f,  1.f, 0.f, 0.f,
        // -X face (left) - Green
        -1.f, -1.f, -1.f,  0.f, 1.f, 0.f,
        -1.f, -1.f,  1.f,  0.f, 1.f, 0.f,
        -1.f,  1.f,  1.f,  0.f, 1.f, 0.f,
        -1.f,  1.f, -1.f,  0.f, 1.f, 0.f,
        // +Y face (top) - Blue
        -1.f,  1.f,  1.f,  0.f, 0.f, 1.f,
         1.f,  1.f,  1.f,  0.f, 0.f, 1.f,
         1.f,  1.f, -1.f,  0.f, 0.f, 1.f,
        -1.f,  1.f, -1.f,  0.f, 0.f, 1.f,
        // -Y face (bottom) - Yellow
        -1.f, -1.f, -1.f,  1.f, 1.f, 0.f,
         1.f, -1.f, -1.f,  1.f, 1.f, 0.f,
         1.f, -1.f,  1.f,  1.f, 1.f, 0.f,
        -1.f, -1.f,  1.f,  1.f, 1.f, 0.f,
        // +Z face (front) - Cyan
        -1.f, -1.f,  1.f,  0.f, 1.f, 1.f,
         1.f, -1.f,  1.f,  0.f, 1.f, 1.f,
         1.f,  1.f,  1.f,  0.f, 1.f, 1.f,
        -1.f,  1.f,  1.f,  0.f, 1.f, 1.f,
        // -Z face (back) - Magenta
         1.f, -1.f, -1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, -1.f,  1.f, 0.f, 1.f,
        -1.f,  1.f, -1.f,  1.f, 0.f, 1.f,
         1.f,  1.f, -1.f,  1.f, 0.f, 1.f,
    };

    constexpr uint16_t idxs[36] = {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23,
    };

    auto vertex_bytes = std::as_bytes(std::span(verts));
    auto index_bytes = std::as_bytes(std::span(idxs));

    return Model::create_indexed(
        device,
        primitive_format(),
        vertex_bytes,
        index_bytes,
        IndexType::Uint16,
        { SubMesh{0, 36, 0} },
        { std::move(material) }
    );
}

// ============================================================================
// create_triangle
// ============================================================================
auto be::create_triangle(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>
{
    constexpr float verts[18] = {
        // top, red
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
        // bottom-left, green
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
        // bottom-right, blue
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
    };

    constexpr uint16_t idxs[3] = { 0, 1, 2 };

    auto vertex_bytes = std::as_bytes(std::span(verts));
    auto index_bytes = std::as_bytes(std::span(idxs));

    return Model::create_indexed(
        device,
        primitive_format(),
        vertex_bytes,
        index_bytes,
        IndexType::Uint16,
        { SubMesh{0, 3, 0} },
        { std::move(material) }
    );
}

// ============================================================================
// create_quad
// ============================================================================
auto be::create_quad(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>
{
    constexpr float verts[24] = {
        // bottom-left, red
        -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
        // bottom-right, green
         0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
        // top-right, blue
         0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
        // top-left, yellow
        -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,
    };

    constexpr uint16_t idxs[6] = { 0, 1, 2,  0, 2, 3 };

    auto vertex_bytes = std::as_bytes(std::span(verts));
    auto index_bytes = std::as_bytes(std::span(idxs));

    return Model::create_indexed(
        device,
        primitive_format(),
        vertex_bytes,
        index_bytes,
        IndexType::Uint16,
        { SubMesh{0, 6, 0} },
        { std::move(material) }
    );
}
