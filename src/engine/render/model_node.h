#pragma once

#include "render/model.h"
#include "math/vec3.h"
#include "math/quat.h"

#include <optional>
#include <string>
#include <vector>

namespace buddd::engine {

struct ModelNode {
    std::string name;
    math::Vec3 translation{0.0f, 0.0f, 0.0f};
    math::Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // w, x, y, z
    math::Vec3 scale{1.0f, 1.0f, 1.0f};
    std::optional<Model> model;
    std::vector<ModelNode> children;

    ModelNode() = default;
    ModelNode(const ModelNode&) = delete;
    auto operator=(const ModelNode&) -> ModelNode& = delete;
    ModelNode(ModelNode&&) = default;
    auto operator=(ModelNode&&) -> ModelNode& = default;
    ~ModelNode() = default;
};

} // namespace buddd::engine
