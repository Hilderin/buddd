#pragma once

#include <cstddef>
#include <vector>

namespace buddd::engine {

/// Raw pixel data read back from the GPU framebuffer.
/// Bottom-left origin (OpenGL convention). No methods — pure aggregate.
struct ImageBuffer {
    int width = 0;
    int height = 0;
    int channels = 0;            // 4 for RGBA framebuffer reads
    std::vector<std::byte> data; // raw pixels, size = width * height * channels
};

} // namespace buddd::engine
