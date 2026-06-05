#pragma once

#include "render/material.h"
#include "render/model.h"

#include <memory>

namespace buddd::engine {

/// Creates a unit cube (2×2×2, centred at origin).
/// 24 vertices (Float3 position + Float3 colour), 36 indices (Uint16).
/// One submesh. The provided material is stored at materials()[0].
[[nodiscard]] auto create_cube(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

/// Creates a coloured right triangle.
/// 3 vertices (Float3 position + Float3 colour), 3 indices (Uint16).
/// One submesh.
[[nodiscard]] auto create_triangle(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

/// Creates a unit quad (2 triangles) in the XY plane.
/// 4 vertices (Float3 position + Float3 colour), 6 indices (Uint16).
/// One submesh.
[[nodiscard]] auto create_quad(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

} // namespace buddd::engine
