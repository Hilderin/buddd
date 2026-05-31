#pragma once

#include "render/texture.h"

#include <cstddef>
#include <vector>

namespace buddd::engine {

class TextureHeadless final : public Texture {
public:
    TextureHeadless(int width, int height, int channels, std::vector<std::byte> data) noexcept;
    ~TextureHeadless() override = default;

    auto width() const noexcept -> int override { return width_; }
    auto height() const noexcept -> int override { return height_; }
    auto channels() const noexcept -> int override { return channels_; }

    /// Returns a const reference to the stored pixel data.
    auto data() const noexcept -> const std::vector<std::byte>& { return data_; }

    TextureHeadless(const TextureHeadless&) = delete;
    auto operator=(const TextureHeadless&) -> TextureHeadless& = delete;
    TextureHeadless(TextureHeadless&&) = delete;
    auto operator=(TextureHeadless&&) -> TextureHeadless& = delete;

private:
    int width_;
    int height_;
    int channels_;
    std::vector<std::byte> data_;
};

} // namespace buddd::engine
