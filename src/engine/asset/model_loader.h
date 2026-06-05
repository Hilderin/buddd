#pragma once

#include "error.h"
#include "render/model_node.h"

#include <memory>
#include <string>
#include <vector>

namespace buddd::engine {

class RenderDevice;

namespace detail {

struct ModelLoadResult {
    ModelNode root;
    std::vector<std::shared_ptr<Material>> materials;  // all created materials
};

/// Load a glTF model from a file path into a ModelNode hierarchy.
/// @param device     RenderDevice for GPU resource creation.
/// @param gltf_path  Absolute path to the .gltf or .glb file.
/// @param scale      Uniform scale applied to vertex positions.
[[nodiscard]] auto load_gltf_model(RenderDevice& device,
                                   const std::string& gltf_path,
                                   float scale) -> Result<ModelLoadResult>;

} // namespace detail
} // namespace buddd::engine
