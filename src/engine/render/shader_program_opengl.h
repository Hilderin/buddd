#pragma once

#include "render/shader_program.h"
#include "render/shader.h"
#include "error.h"

#include <SDL3/SDL_opengl.h>

#include <memory>
#include <string>
#include <utility>

namespace buddd::engine {

/// OpenGL implementation of ShaderProgram.
/// Wraps a GLuint program handle created via glCreateProgram/glLinkProgram.
class ShaderProgramOpenGL final : public ShaderProgram {
public:
    /// Creates a linked OpenGL shader program from vertex and fragment shaders.
    /// On link failure, returns a LinkingFailed error.
    /// On success, the shader objects are marked for deletion (glDeleteShader).
    [[nodiscard]] static auto create(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader
    ) -> Result<std::unique_ptr<ShaderProgramOpenGL>>;

    ~ShaderProgramOpenGL() override;

    auto handle() const -> uint32_t override { return static_cast<uint32_t>(program_); }
    auto is_valid() const noexcept -> bool override { return program_ != 0; }

    auto replace_handle(uint32_t new_handle) -> void override;
    auto release_handle() noexcept -> uint32_t override;

    ShaderProgramOpenGL(const ShaderProgramOpenGL&) = delete;
    auto operator=(const ShaderProgramOpenGL&) -> ShaderProgramOpenGL& = delete;
    ShaderProgramOpenGL(ShaderProgramOpenGL&&) = delete;
    auto operator=(ShaderProgramOpenGL&&) -> ShaderProgramOpenGL& = delete;

private:
    explicit ShaderProgramOpenGL(GLuint program);
    GLuint program_ = 0;
};

} // namespace buddd::engine
