#include "render/frame_buffer_headless.h"

#include "log/log.h"

BUDDD_LOG_TAG("Render:Headless");

namespace buddd::engine {

// ============================================================================
// Factory / Constructor
// ============================================================================

FrameBufferHeadless::FrameBufferHeadless(uint32_t width, uint32_t height,
                                         std::unique_ptr<Texture> color_tex)
    : color_texture_(std::move(color_tex))
    , width_(width)
    , height_(height)
{}

auto FrameBufferHeadless::create(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<FrameBuffer>>
{
    // 1. Validate dimensions
    if (width == 0 || height == 0) {
        return make_error(Error::Category::InvalidArgument,
            "FrameBuffer dimensions must be positive");
    }

    // 2. Create a headless texture with zeroed pixel data
    std::vector<std::byte> data(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, std::byte{0});
    auto color_tex = std::unique_ptr<Texture>(
        new TextureHeadless(static_cast<int>(width), static_cast<int>(height), 4, std::move(data)));

    // 3. Log
    BUDDD_LOG_INFO("FrameBuffer created ({}x{})", width, height);

    // 4. Return
    return std::unique_ptr<FrameBuffer>(
        new FrameBufferHeadless(width, height, std::move(color_tex)));
}

// ============================================================================
// Bind / Unbind (no-ops)
// ============================================================================

auto FrameBufferHeadless::bind() -> void {
    // No-op in headless mode
}

auto FrameBufferHeadless::unbind() -> void {
    // No-op in headless mode
}

// ============================================================================
// Resize
// ============================================================================

auto FrameBufferHeadless::resize(uint32_t width, uint32_t height) -> Result<void> {
    if (width == 0 || height == 0) {
        return make_error(Error::Category::InvalidArgument,
            "FrameBuffer dimensions must be positive");
    }

    BUDDD_LOG_INFO("FrameBuffer resized ({}x{} -> {}x{})", width_, height_, width, height);

    width_ = width;
    height_ = height;

    // Create a new texture with the new dimensions
    std::vector<std::byte> data(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, std::byte{0});
    color_texture_ = std::unique_ptr<Texture>(
        new TextureHeadless(static_cast<int>(width), static_cast<int>(height), 4, std::move(data)));

    return {};
}

// ============================================================================
// Accessors
// ============================================================================

auto FrameBufferHeadless::color_texture() const noexcept -> Texture& {
    return *color_texture_;
}

auto FrameBufferHeadless::width() const noexcept -> uint32_t {
    return width_;
}

auto FrameBufferHeadless::height() const noexcept -> uint32_t {
    return height_;
}

} // namespace buddd::engine
