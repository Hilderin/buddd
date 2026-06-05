#pragma once

#include "asset/asset.h"
#include "render/texture.h"

#include <memory>

namespace buddd::engine {

class TextureAsset final : public Asset {
public:
    explicit TextureAsset(std::shared_ptr<Texture> texture) noexcept;

    auto texture() const noexcept -> const std::shared_ptr<Texture>&;

    TextureAsset(const TextureAsset&) = delete;
    auto operator=(const TextureAsset&) -> TextureAsset& = delete;
    TextureAsset(TextureAsset&&) = delete;
    auto operator=(TextureAsset&&) -> TextureAsset& = delete;

private:
    std::shared_ptr<Texture> texture_;
};

} // namespace buddd::engine
