#pragma once

#include "error.h"

#include <cstdint>
#include <memory>
#include <utility>

namespace buddd::engine {

class Texture;

class FrameBuffer {
public:
    virtual ~FrameBuffer() = default;

    virtual auto bind() -> void = 0;
    virtual auto unbind() -> void = 0;
    virtual auto resize(uint32_t width, uint32_t height) -> Result<void> = 0;
    [[nodiscard]] virtual auto color_texture() const noexcept -> Texture& = 0;
    [[nodiscard]] virtual auto width() const noexcept -> uint32_t = 0;
    [[nodiscard]] virtual auto height() const noexcept -> uint32_t = 0;

    FrameBuffer(const FrameBuffer&) = delete;
    auto operator=(const FrameBuffer&) -> FrameBuffer& = delete;
    FrameBuffer(FrameBuffer&&) = delete;
    auto operator=(FrameBuffer&&) -> FrameBuffer& = delete;

protected:
    FrameBuffer() = default;
};

} // namespace buddd::engine
