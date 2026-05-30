#define GL_GLEXT_PROTOTYPES
#include "vertex_buffer_opengl.h"

namespace buddd::engine {

VertexBufferOpenGL::VertexBufferOpenGL(GLuint vao, GLuint vbo, VertexFormat format, size_t byte_size)
    : vao_(vao), vbo_(vbo), format_(std::move(format)), byte_size_(byte_size) {}

VertexBufferOpenGL::~VertexBufferOpenGL() {
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

auto VertexBufferOpenGL::format() const noexcept -> const VertexFormat& {
    return format_;
}

auto VertexBufferOpenGL::vao() const noexcept -> GLuint {
    return vao_;
}

auto VertexBufferOpenGL::vbo() const noexcept -> GLuint {
    return vbo_;
}

} // namespace buddd::engine
