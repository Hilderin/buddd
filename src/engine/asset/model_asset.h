#pragma once

#include "asset/asset.h"
#include "render/model_node.h"

#include <memory>

namespace buddd::engine {

class ModelAsset final : public Asset {
public:
    explicit ModelAsset(ModelNode root) noexcept;

    auto root_node() const noexcept -> const ModelNode&;
    auto root_node() noexcept -> ModelNode&;

    ModelAsset(const ModelAsset&) = delete;
    auto operator=(const ModelAsset&) -> ModelAsset& = delete;
    ModelAsset(ModelAsset&&) = delete;
    auto operator=(ModelAsset&&) -> ModelAsset& = delete;

private:
    // In-place hot-reload support (friend AssetManager)
    auto replace_root(ModelNode new_root) -> void;

    ModelNode root_;
    friend class AssetManager;
};

} // namespace buddd::engine
