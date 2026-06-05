#include "asset/material_asset.h"

namespace buddd::engine {

MaterialAsset::MaterialAsset(std::shared_ptr<Material> material) noexcept
    : material_(std::move(material)) {}

auto MaterialAsset::material() const noexcept -> const std::shared_ptr<Material>& {
    return material_;
}

} // namespace buddd::engine
