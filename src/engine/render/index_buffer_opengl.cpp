#define GL_GLEXT_PROTOTYPES
#include "index_buffer_opengl.h"

namespace buddd::engine {

IndexBufferOpenGL::IndexBufferOpenGL(GLuint handle, IndexType type, size_t byte_size)
    : handle_(handle), type_(type), byte_size_(byte_size) {}

IndexBufferOpenGL::~IndexBufferOpenGL() {
    glDeleteBuffers(1, &handle_);
}

auto IndexBufferOpenGL::type() const noexcept -> IndexType {
    return type_;
}

auto IndexBufferOpenGL::handle() const noexcept -> GLuint {
    return handle_;
}

auto IndexBufferOpenGL::index_type() const noexcept -> IndexType {
    return type_;
}

auto IndexBufferOpenGL::byte_size() const noexcept -> size_t {
    return byte_size_;
}

} // namespace buddd::engine
