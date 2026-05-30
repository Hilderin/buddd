#include "index_buffer_headless.h"

namespace buddd::engine {

IndexBufferHeadless::IndexBufferHeadless(IndexType type, std::span<const std::byte> data)
    : type_(type), data_(data.begin(), data.end()) {}

auto IndexBufferHeadless::type() const noexcept -> IndexType {
    return type_;
}

} // namespace buddd::engine
