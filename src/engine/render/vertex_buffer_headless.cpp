#include "vertex_buffer_headless.h"

namespace buddd::engine {

VertexBufferHeadless::VertexBufferHeadless(VertexFormat format, std::span<const std::byte> data)
    : format_(std::move(format)), data_(data.begin(), data.end()) {}

auto VertexBufferHeadless::format() const noexcept -> const VertexFormat& {
    return format_;
}

} // namespace buddd::engine
