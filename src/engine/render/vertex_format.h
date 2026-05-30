#pragma once

#include <cstdint>
#include <vector>

namespace buddd::engine {

enum class VertexAttributeType {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UByte,
    UByte4,
    UByte4Norm
};

struct VertexAttribute {
    uint32_t location;
    VertexAttributeType type;
    uint32_t offset;
    bool normalized{false};
};

struct VertexFormat {
    uint32_t stride;
    std::vector<VertexAttribute> attributes;
};

} // namespace buddd::engine
