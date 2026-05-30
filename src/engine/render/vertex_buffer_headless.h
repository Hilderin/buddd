#pragma once

#include "vertex_buffer.h"

#include <cstddef>
#include <vector>

namespace buddd::engine {

class VertexBufferHeadless final : public VertexBuffer {
public:
    VertexBufferHeadless(VertexFormat format, std::span<const std::byte> data);
    ~VertexBufferHeadless() override = default;

    auto format() const noexcept -> const VertexFormat& override;

    VertexBufferHeadless(const VertexBufferHeadless&) = delete;
    auto operator=(const VertexBufferHeadless&) -> VertexBufferHeadless& = delete;
    VertexBufferHeadless(VertexBufferHeadless&&) = delete;
    auto operator=(VertexBufferHeadless&&) -> VertexBufferHeadless& = delete;

private:
    VertexFormat format_;
    std::vector<std::byte> data_;
};

} // namespace buddd::engine
