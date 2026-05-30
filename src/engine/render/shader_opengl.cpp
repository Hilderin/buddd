#define GL_GLEXT_PROTOTYPES
#include "shader_opengl.h"

namespace buddd::engine {

ShaderOpenGL::ShaderOpenGL(GLuint handle, ShaderType type)
    : handle_(handle), type_(type) {}

ShaderOpenGL::~ShaderOpenGL() {
    glDeleteShader(handle_);
}

auto ShaderOpenGL::type() const noexcept -> ShaderType {
    return type_;
}

auto ShaderOpenGL::handle() const noexcept -> GLuint {
    return handle_;
}

} // namespace buddd::engine
