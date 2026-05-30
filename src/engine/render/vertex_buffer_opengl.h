#pragma once

#include "vertex_buffer.h"

#include <SDL3/SDL_opengl.h>

namespace buddd::engine {

class VertexBufferOpenGL final : public VertexBuffer {
public:
    VertexBufferOpenGL(GLuint vao, GLuint vbo, VertexFormat format, size_t byte_size);
    ~VertexBufferOpenGL() override;

    auto format() const noexcept -> const VertexFormat& override;

    auto vao() const noexcept -> GLuint;
    auto vbo() const noexcept -> GLuint;

    VertexBufferOpenGL(const VertexBufferOpenGL&) = delete;
    auto operator=(const VertexBufferOpenGL&) -> VertexBufferOpenGL& = delete;
    VertexBufferOpenGL(VertexBufferOpenGL&&) = delete;
    auto operator=(VertexBufferOpenGL&&) -> VertexBufferOpenGL& = delete;

private:
    GLuint vao_;
    GLuint vbo_;
    VertexFormat format_;
    size_t byte_size_;
};

} // namespace buddd::engine
