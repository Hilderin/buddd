#pragma once

#include "render/frame_buffer.h"
#include "render/texture_headless.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace buddd::engine {

class FrameBufferHeadless final : public FrameBuffer {
public:
    static auto create(uint32_t width, uint32_t height) -> Result<std::unique_ptr<FrameBuffer>>;

    ~FrameBufferHeadless() override = default;

    auto bind() -> void override;
    auto unbind() -> void override;
    auto resize(uint32_t width, uint32_t height) -> Result<void> override;
    auto color_texture() const noexcept -> Texture& override;
    auto width() const noexcept -> uint32_t override;
    auto height() const noexcept -> uint32_t override;

    FrameBufferHeadless(const FrameBufferHeadless&) = delete;
    auto operator=(const FrameBufferHeadless&) -> FrameBufferHeadless& = delete;
    FrameBufferHeadless(FrameBufferHeadless&&) = delete;
    auto operator=(FrameBufferHeadless&&) -> FrameBufferHeadless& = delete;

private:
    FrameBufferHeadless(uint32_t width, uint32_t height,
                        std::unique_ptr<Texture> color_tex);

    std::unique_ptr<Texture> color_texture_;
    uint32_t width_;
    uint32_t height_;
};

} // namespace buddd::engine
