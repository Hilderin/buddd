#include "asset/texture_asset.h"

namespace buddd::engine {

TextureAsset::TextureAsset(std::shared_ptr<Texture> texture) noexcept
    : texture_(std::move(texture)) {}

auto TextureAsset::texture() const noexcept -> const std::shared_ptr<Texture>& {
    return texture_;
}

} // namespace buddd::engine
