#pragma once

#include "render/texture.h"

#include <SDL3/SDL_opengl.h>

namespace buddd::engine {

class TextureOpenGL final : public Texture {
public:
    TextureOpenGL(GLuint handle, int width, int height, int channels) noexcept;
    ~TextureOpenGL() override;

    auto width() const noexcept -> int override { return width_; }
    auto height() const noexcept -> int override { return height_; }
    auto channels() const noexcept -> int override { return channels_; }

    auto handle() const noexcept -> GLuint { return texture_; }

    TextureOpenGL(const TextureOpenGL&) = delete;
    auto operator=(const TextureOpenGL&) -> TextureOpenGL& = delete;
    TextureOpenGL(TextureOpenGL&&) = delete;
    auto operator=(TextureOpenGL&&) -> TextureOpenGL& = delete;

private:
    GLuint texture_;
    int width_;
    int height_;
    int channels_;
};

} // namespace buddd::engine
