#include "render/texture_opengl.h"

#include "log/log.h"

BUDDD_LOG_TAG("Render:OpenGL");

namespace buddd::engine {

TextureOpenGL::TextureOpenGL(GLuint handle, int width, int height, int channels) noexcept
    : texture_(handle)
    , width_(width)
    , height_(height)
    , channels_(channels)
{}

TextureOpenGL::~TextureOpenGL() {
    BUDDD_LOG_DEBUG("TextureOpenGL destroyed: {}", texture_);
    glDeleteTextures(1, &texture_);
}

auto TextureOpenGL::replace_gl_handle(uint32_t new_handle) -> void {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
    }
    texture_ = static_cast<GLuint>(new_handle);
}

auto TextureOpenGL::gl_handle() const noexcept -> uint32_t {
    return static_cast<uint32_t>(texture_);
}

auto TextureOpenGL::release_gl_handle() noexcept -> uint32_t {
    GLuint old = texture_;
    texture_ = 0;
    return static_cast<uint32_t>(old);
}

} // namespace buddd::engine
