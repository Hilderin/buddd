#pragma once

#include "error.h"

#include <cstddef>
#include <memory>
#include <span>

namespace buddd::engine {

enum class IndexType {
    Uint16,
    Uint32
};

class IndexBuffer {
public:
    virtual ~IndexBuffer() = default;

    virtual auto type() const noexcept -> IndexType = 0;

    IndexBuffer(const IndexBuffer&) = delete;
    auto operator=(const IndexBuffer&) -> IndexBuffer& = delete;
    IndexBuffer(IndexBuffer&&) = delete;
    auto operator=(IndexBuffer&&) -> IndexBuffer& = delete;

protected:
    IndexBuffer() = default;
};

} // namespace buddd::engine
