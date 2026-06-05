#pragma once

#include "asset/asset.h"
#include "render/material.h"

#include <memory>

namespace buddd::engine {

class MaterialAsset final : public Asset {
public:
    explicit MaterialAsset(std::shared_ptr<Material> material) noexcept;

    auto material() const noexcept -> const std::shared_ptr<Material>&;

    MaterialAsset(const MaterialAsset&) = delete;
    auto operator=(const MaterialAsset&) -> MaterialAsset& = delete;
    MaterialAsset(MaterialAsset&&) = delete;
    auto operator=(MaterialAsset&&) -> MaterialAsset& = delete;

private:
    std::shared_ptr<Material> material_;
};

} // namespace buddd::engine
