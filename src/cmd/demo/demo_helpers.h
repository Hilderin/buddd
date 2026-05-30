#pragma once

#include "render/material.h"
#include "render/model.h"
#include "render/vertex_buffer.h"

#include <memory>
#include <utility>

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

/// Creates a coloured triangle by compiling shaders and creating a material +
/// vertex buffer.
///
/// @param device  The render device to create resources from.
/// @return        A pair of (material, vertex_buffer).
///
/// On failure, prints a FATAL error to stderr and calls std::exit(EXIT_FAILURE).
auto setup_triangle(buddd::engine::RenderDevice& device)
    -> std::pair<
        std::unique_ptr<buddd::engine::Material>,
        std::unique_ptr<buddd::engine::VertexBuffer>>;

struct CubeResources {
    std::shared_ptr<buddd::engine::Material> material;
    buddd::engine::Model model;
};

/// Creates a CubeResources for a unit cube (2×2×2, centred at origin):
/// - 24 vertices (Float3 position + Float3 color per vertex, stride 24)
/// - 36 indices (Uint16, 6 per face, CCW winding)
/// - Material with a_position (loc 0), a_color (loc 1), u_mvp (Mat4) uniform
///
/// Face colors are encoded in vertex data:
///   +X (right):  Red    (1,0,0)  -> vertices  0-3
///   -X (left):   Green  (0,1,0)  -> vertices  4-7
///   +Y (top):    Blue   (0,0,1)  -> vertices  8-11
///   -Y (bottom): Yellow (1,1,0)  -> vertices 12-15
///   +Z (front):  Cyan   (0,1,1)  -> vertices 16-19
///   -Z (back):   Magenta(1,0,1)  -> vertices 20-23
///
/// On failure, prints a FATAL error to stderr and calls std::exit(EXIT_FAILURE),
/// consistent with setup_triangle().
auto setup_cube(buddd::engine::RenderDevice& device) -> CubeResources;

} // namespace buddd::cmd::demo
