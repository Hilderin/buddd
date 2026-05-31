#include "render/texture_opengl.h"

#include <iostream>

namespace buddd::engine {

TextureOpenGL::TextureOpenGL(GLuint handle, int width, int height, int channels) noexcept
    : texture_(handle)
    , width_(width)
    , height_(height)
    , channels_(channels)
{}

TextureOpenGL::~TextureOpenGL() {
#ifndef NDEBUG
    std::cerr << "TextureOpenGL destroyed: " << texture_ << "\n";
#endif
    glDeleteTextures(1, &texture_);
}

} // namespace buddd::engine
