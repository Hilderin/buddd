#pragma once

#include "error.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace buddd::engine {

struct ImageBuffer; // forward declaration — full definition in image_buffer.h

class Image {
public:
    /// Creates an Image from a raw framebuffer ImageBuffer.
    /// Flips rows vertically (bottom-left → top-left).
    /// Validates dimensions and channels are positive.
    [[nodiscard]] static auto create(const ImageBuffer& buffer) -> Result<Image>;

    /// Loads a PNG image from disk using stb_image.
    [[nodiscard]] static auto load(std::string_view path) -> Result<Image>;

    /// Writes the image to disk as a PNG file using stb_image_write.
    [[nodiscard]] auto save(std::string_view path) const -> Result<void>;

    /// Accessors
    auto width() const noexcept -> int;
    auto height() const noexcept -> int;
    auto channels() const noexcept -> int;
    auto data() const noexcept -> const std::vector<std::byte>&;

    // Non-copyable, movable
    Image(const Image&) = delete;
    auto operator=(const Image&) -> Image& = delete;
    Image(Image&&) noexcept = default;
    auto operator=(Image&&) noexcept -> Image& = default;

    ~Image() = default;

private:
    Image() = default;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    std::vector<std::byte> data_;
};

} // namespace buddd::engine
