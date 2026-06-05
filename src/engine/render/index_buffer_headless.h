#pragma once

#include "index_buffer.h"

#include <cstddef>
#include <vector>

namespace buddd::engine {

class IndexBufferHeadless final : public IndexBuffer {
public:
    IndexBufferHeadless(IndexType type, std::span<const std::byte> data);
    ~IndexBufferHeadless() override = default;

    auto type() const noexcept -> IndexType override;

    /// Returns a const reference to the stored index data.
    auto data() const noexcept -> const std::vector<std::byte>& { return data_; }

    IndexBufferHeadless(const IndexBufferHeadless&) = delete;
    auto operator=(const IndexBufferHeadless&) -> IndexBufferHeadless& = delete;
    IndexBufferHeadless(IndexBufferHeadless&&) = delete;
    auto operator=(IndexBufferHeadless&&) -> IndexBufferHeadless& = delete;

private:
    IndexType type_;
    std::vector<std::byte> data_;
};

} // namespace buddd::engine
