#pragma once

#include "shader.h"

#include <SDL3/SDL_opengl.h>

namespace buddd::engine {

class ShaderOpenGL final : public Shader {
public:
    ShaderOpenGL(GLuint handle, ShaderType type);
    ~ShaderOpenGL() override;

    auto type() const noexcept -> ShaderType override;

    auto handle() const noexcept -> GLuint;

    ShaderOpenGL(const ShaderOpenGL&) = delete;
    auto operator=(const ShaderOpenGL&) -> ShaderOpenGL& = delete;
    ShaderOpenGL(ShaderOpenGL&&) = delete;
    auto operator=(ShaderOpenGL&&) -> ShaderOpenGL& = delete;

private:
    GLuint handle_;
    ShaderType type_;
};

} // namespace buddd::engine
