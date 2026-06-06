#define GL_GLEXT_PROTOTYPES
#include "render/shader_program_opengl.h"
#include "render/shader_opengl.h"

#include <string>

#include "log/log.h"

BUDDD_LOG_TAG("Render:OpenGL");

namespace buddd::engine {

ShaderProgramOpenGL::ShaderProgramOpenGL(GLuint program)
    : program_(program) {}

ShaderProgramOpenGL::~ShaderProgramOpenGL() {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

auto ShaderProgramOpenGL::create(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader
) -> Result<std::unique_ptr<ShaderProgramOpenGL>>
{
    if (!vertex_shader || !fragment_shader) {
        return make_error(Error::Category::InvalidArgument,
            "Null shader passed to ShaderProgramOpenGL::create");
    }

    auto* vs = static_cast<ShaderOpenGL*>(vertex_shader.get());
    auto* fs = static_cast<ShaderOpenGL*>(fragment_shader.get());

    GLuint program = glCreateProgram();
    glAttachShader(program, vs->handle());
    glAttachShader(program, fs->handle());
    glLinkProgram(program);

    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);

    if (link_status != GL_TRUE) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(log_length, '\0');
        glGetProgramInfoLog(program, log_length, nullptr, log.data());
        glDeleteProgram(program);

        return make_error(Error::Category::LinkingFailed, std::move(log));
    }

    // Mark shaders for deletion; they stay alive until program is deleted.
    glDeleteShader(vs->handle());
    glDeleteShader(fs->handle());

    BUDDD_LOG_DEBUG("Shader program linked (OpenGL, id={})", program);

    return std::unique_ptr<ShaderProgramOpenGL>(new ShaderProgramOpenGL(program));
}

auto ShaderProgramOpenGL::replace_handle(uint32_t new_handle) -> void {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
    program_ = static_cast<GLuint>(new_handle);
}

auto ShaderProgramOpenGL::release_handle() noexcept -> uint32_t {
    GLuint old = program_;
    program_ = 0;
    return static_cast<uint32_t>(old);
}

} // namespace buddd::engine
