#include "image/image.h"
#include "image/image_buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>

namespace buddd::engine {

// ============================================================================
// Image::create(const ImageBuffer& buffer)
// ============================================================================

auto Image::create(const ImageBuffer& buffer) -> Result<Image> {
    // Validate width, height, channels are positive
    if (buffer.width <= 0 || buffer.height <= 0 || buffer.channels <= 0) {
        return make_error(Error::Category::InvalidArgument,
            "ImageBuffer dimensions must be positive");
    }

    // Validate data size matches dimensions
    auto expected_size = static_cast<size_t>(buffer.width)
                       * static_cast<size_t>(buffer.height)
                       * static_cast<size_t>(buffer.channels);
    if (buffer.data.size() != expected_size) {
        return make_error(Error::Category::InvalidArgument,
            "ImageBuffer data size does not match dimensions");
    }

    // Allocate data_ and flip rows vertically (bottom-left → top-left)
    Image image;
    image.width_ = buffer.width;
    image.height_ = buffer.height;
    image.channels_ = buffer.channels;
    image.data_.resize(expected_size);

    const size_t row_bytes = static_cast<size_t>(buffer.width)
                           * static_cast<size_t>(buffer.channels);

    for (int r = 0; r < buffer.height; ++r) {
        // Row r in buffer (0 = bottom) becomes row (height - 1 - r) in Image
        const auto* src = buffer.data.data() + r * row_bytes;
        auto* dst = image.data_.data() + (buffer.height - 1 - r) * row_bytes;
        std::copy(src, src + row_bytes, dst);
    }

    return image;
}

// ============================================================================
// Image::load(std::string_view path)
// ============================================================================

auto Image::load(std::string_view path) -> Result<Image> {
    int w = 0, h = 0, channels_in_file = 0;

    unsigned char* decoded = stbi_load(path.data(), &w, &h, &channels_in_file, 0);
    if (decoded == nullptr) {
        const char* reason = stbi_failure_reason();
        std::string msg = "Failed to load image: ";
        msg += (reason != nullptr) ? reason : "unknown error";
        return make_error(Error::Category::IoFailed, std::move(msg));
    }

    // stb_image loads with top-left origin — no row flip needed
    Image image;
    image.width_ = w;
    image.height_ = h;
    image.channels_ = channels_in_file;

    auto data_size = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(channels_in_file);
    image.data_.resize(data_size);
    std::memcpy(image.data_.data(), decoded, data_size);

    stbi_image_free(decoded);

    return image;
}

// ============================================================================
// Image::save(std::string_view path) const
// ============================================================================

auto Image::save(std::string_view path) const -> Result<void> {
    int stride_in_bytes = width_ * channels_;
    int result = stbi_write_png(
        path.data(),
        width_,
        height_,
        channels_,
        data_.data(),
        stride_in_bytes
    );

    if (result == 0) {
        return make_error(Error::Category::IoFailed,
            "Failed to write image: " + std::string(path));
    }

    return Result<void>{};
}

// ============================================================================
// Accessors
// ============================================================================

auto Image::width() const noexcept -> int { return width_; }
auto Image::height() const noexcept -> int { return height_; }
auto Image::channels() const noexcept -> int { return channels_; }
auto Image::data() const noexcept -> const std::vector<std::byte>& { return data_; }

} // namespace buddd::engine
