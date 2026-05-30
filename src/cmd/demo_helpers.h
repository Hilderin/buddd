#pragma once

#include "render/material.h"
#include "render/vertex_buffer.h"

#include <memory>
#include <utility>

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd {

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

} // namespace buddd::cmd
