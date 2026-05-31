#pragma once

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "render/vertex_format.h"

#include <cstddef>

namespace buddd::engine {

/// Standard vertex format used by all meshes.
/// Fields not used by a particular shader can be left as zero-initialized
/// — the GPU safely ignores unbound attribute locations.
struct Vertex {
    math::Vec3 position;     // offset 0   (12B)  location 0
    math::Vec4 color;        // offset 12  (16B)  location 1
    math::Vec3 normal;       // offset 28  (12B)  location 2
    math::Vec2 texcoord;     // offset 40  (8B)   location 3
    math::Vec4 tangent;      // offset 48  (16B)  location 4  (reserved)
    math::Vec2 texcoord2;    // offset 64  (8B)   location 5  (reserved)
};
static_assert(sizeof(Vertex) == 72, "Vertex must be 72 bytes");

/// Vertex format descriptor for the standard Vertex.
inline VertexFormat k_standard_vertex_format = [] {
    VertexFormat fmt;
    fmt.stride = sizeof(Vertex);
    fmt.attributes = {
        {0, VertexAttributeType::Float3, static_cast<uint32_t>(offsetof(Vertex, position)),  false},
        {1, VertexAttributeType::Float4, static_cast<uint32_t>(offsetof(Vertex, color)),     false},
        {2, VertexAttributeType::Float3, static_cast<uint32_t>(offsetof(Vertex, normal)),    false},
        {3, VertexAttributeType::Float2, static_cast<uint32_t>(offsetof(Vertex, texcoord)),  false},
        {4, VertexAttributeType::Float4, static_cast<uint32_t>(offsetof(Vertex, tangent)),   false},
        {5, VertexAttributeType::Float2, static_cast<uint32_t>(offsetof(Vertex, texcoord2)), false},
    };
    return fmt;
}();

} // namespace buddd::engine
