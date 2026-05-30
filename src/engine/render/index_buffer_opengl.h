#pragma once

#include "index_buffer.h"

#include <SDL3/SDL_opengl.h>

namespace buddd::engine {

class IndexBufferOpenGL final : public IndexBuffer {
public:
    IndexBufferOpenGL(GLuint handle, IndexType type, size_t byte_size);
    ~IndexBufferOpenGL() override;

    auto type() const noexcept -> IndexType override;

    auto handle() const noexcept -> GLuint;
    auto index_type() const noexcept -> IndexType;
    auto byte_size() const noexcept -> size_t;

    IndexBufferOpenGL(const IndexBufferOpenGL&) = delete;
    auto operator=(const IndexBufferOpenGL&) -> IndexBufferOpenGL& = delete;
    IndexBufferOpenGL(IndexBufferOpenGL&&) = delete;
    auto operator=(IndexBufferOpenGL&&) -> IndexBufferOpenGL& = delete;

private:
    GLuint handle_;
    IndexType type_;
    size_t byte_size_;
};

} // namespace buddd::engine
