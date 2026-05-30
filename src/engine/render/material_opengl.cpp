#define GL_GLEXT_PROTOTYPES
#include "material_opengl.h"

#include <iostream>

namespace buddd::engine {

MaterialOpenGL::MaterialOpenGL(GLuint program)
    : program_(program) {}

MaterialOpenGL::~MaterialOpenGL() {
    glDeleteProgram(program_);
}

auto MaterialOpenGL::program() const noexcept -> GLuint {
    return program_;
}

auto MaterialOpenGL::get_uniform_location(std::string_view name) -> Result<GLint> {
    auto it = location_cache_.find(std::string(name));
    if (it != location_cache_.end()) {
        if (it->second == -1) {
            return make_error(Error::Category::UniformNotFound,
                "Uniform '" + std::string(name) + "' not found");
        }
        return it->second;
    }

    GLint location = glGetUniformLocation(program_, name.data());
    location_cache_[std::string(name)] = location;

    if (location == -1) {
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    return location;
}

auto MaterialOpenGL::set_uniform(std::string_view name, float value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform1f(*loc, value);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=float)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, int32_t value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform1i(*loc, value);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=int)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, bool value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform1i(*loc, value ? 1 : 0);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=bool)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform3fv(*loc, 1, &value.x);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Vec3)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform4fv(*loc, 1, &value.x);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Vec4)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    // GLM/Mat4 is column-major; GL_FALSE means "don't transpose"
    glUniformMatrix4fv(*loc, 1, GL_FALSE, &value[0].x);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Mat4)\n";
#endif
    return {};
}

auto MaterialOpenGL::has_uniform(std::string_view name) const -> bool {
    GLint location = glGetUniformLocation(program_, name.data());
    return location != -1;
}

} // namespace buddd::engine
