#pragma once

#include "error.h"
#include "vertex_format.h"

#include <cstddef>
#include <memory>
#include <span>

namespace buddd::engine {

class VertexBuffer {
public:
    virtual ~VertexBuffer() = default;

    virtual auto format() const noexcept -> const VertexFormat& = 0;

    VertexBuffer(const VertexBuffer&) = delete;
    auto operator=(const VertexBuffer&) -> VertexBuffer& = delete;
    VertexBuffer(VertexBuffer&&) = delete;
    auto operator=(VertexBuffer&&) -> VertexBuffer& = delete;

protected:
    VertexBuffer() = default;
};

} // namespace buddd::engine
