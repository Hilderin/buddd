#include "asset/model_asset.h"

namespace buddd::engine {

ModelAsset::ModelAsset(ModelNode root) noexcept
    : root_(std::move(root)) {}

auto ModelAsset::root_node() const noexcept -> const ModelNode& {
    return root_;
}

auto ModelAsset::root_node() noexcept -> ModelNode& {
    return root_;
}

auto ModelAsset::replace_root(ModelNode new_root) -> void {
    root_ = std::move(new_root);
}

} // namespace buddd::engine
