#define GL_GLEXT_PROTOTYPES
#include "material_opengl.h"
#include "render/texture_opengl.h"

#include "log/log.h"

BUDDD_LOG_TAG("Render:OpenGL");

namespace buddd::engine {

MaterialOpenGL::MaterialOpenGL(GLuint program)
    : program_(program) {}

MaterialOpenGL::MaterialOpenGL(std::shared_ptr<ShaderProgram> program)
    : program_(program ? static_cast<GLuint>(program->handle()) : 0)
    , shader_program_(std::move(program)) {}

MaterialOpenGL::~MaterialOpenGL() {
    if (!shader_program_) {
        glDeleteProgram(program_);
    }
    // If shader_program_ is set, the ShaderProgram owns the GL handle
}

auto MaterialOpenGL::program() const noexcept -> GLuint {
    if (shader_program_) {
        return static_cast<GLuint>(shader_program_->handle());
    }
    return program_;
}

auto MaterialOpenGL::active_program() const noexcept -> GLuint {
    return program();
}

auto MaterialOpenGL::get_uniform_location(std::string_view name) -> Result<GLint> {
    GLuint prog = active_program();
    if (prog == 0) {
        return make_error(Error::Category::InvalidArgument,
            "No active shader program");
    }

    // Check cache first
    auto cache_it = location_cache_.find(std::string(name));
    if (cache_it != location_cache_.end()) {
        if (cache_it->second == -1) {
            return make_error(Error::Category::UniformNotFound,
                "Uniform '" + std::string(name) + "' not found");
        }
        return cache_it->second;
    }

    GLint location = glGetUniformLocation(prog, name.data());
    location_cache_[std::string(name)] = location;

    if (location == -1) {
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    return location;
}

// ============================================================================
// set_uniform overloads — cache values, do NOT call glUniform* immediately
// ============================================================================

auto MaterialOpenGL::set_uniform(std::string_view name, float value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return make_error(loc);
    uniform_cache_[std::string(name)] = value;
    BUDDD_LOG_DEBUG("Uniform cached: {} (type=float)", name);
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, int32_t value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return make_error(loc);
    uniform_cache_[std::string(name)] = value;
    BUDDD_LOG_DEBUG("Uniform cached: {} (type=int)", name);
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, bool value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return make_error(loc);
    uniform_cache_[std::string(name)] = value;
    BUDDD_LOG_DEBUG("Uniform cached: {} (type=bool)", name);
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return make_error(loc);
    uniform_cache_[std::string(name)] = value;
    BUDDD_LOG_DEBUG("Uniform cached: {} (type=Vec3)", name);
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return make_error(loc);
    uniform_cache_[std::string(name)] = value;
    BUDDD_LOG_DEBUG("Uniform cached: {} (type=Vec4)", name);
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return make_error(loc);
    uniform_cache_[std::string(name)] = value;
    BUDDD_LOG_DEBUG("Uniform cached: {} (type=Mat4)", name);
    return {};
}

// ============================================================================
// has_uniform
// ============================================================================

auto MaterialOpenGL::has_uniform(std::string_view name) const -> bool {
    GLuint prog = active_program();
    if (prog == 0) return false;
    GLint location = glGetUniformLocation(prog, name.data());
    return location != -1;
}

// ============================================================================
// Texture support
// ============================================================================

auto MaterialOpenGL::set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> {
    if (!texture) {
        return make_error(Error::Category::InvalidArgument,
            "Texture is null");
    }

    auto loc = get_uniform_location(name);
    if (!loc) return make_error(loc);

    texture_map_[std::string(name)] = std::move(texture);
    BUDDD_LOG_DEBUG("Texture set: {}", name);
    return {};
}

auto MaterialOpenGL::has_texture(std::string_view name) const -> bool {
    GLuint prog = active_program();
    if (prog == 0) return false;
    GLint location = glGetUniformLocation(prog, name.data());
    return location != -1;
}

// ============================================================================
// bind() — activate program, apply cached uniforms, bind textures
// ============================================================================

auto MaterialOpenGL::bind() const -> void {
    // 1. Activate the shader program
    GLuint prog = active_program();
    if (prog == 0) return;
    glUseProgram(prog);
    BUDDD_LOG_DEBUG("Material bind: program {}", prog);

    // 2. Apply cached uniforms
    for (const auto& [name, value] : uniform_cache_) {
        GLint loc = glGetUniformLocation(prog, name.c_str());
        if (loc == -1) continue;  // uniform was removed or program changed — skip
        std::visit([loc](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, float>)          glUniform1f(loc, v);
            else if constexpr (std::is_same_v<T, int32_t>)   glUniform1i(loc, v);
            else if constexpr (std::is_same_v<T, bool>)      glUniform1i(loc, v ? 1 : 0);
            else if constexpr (std::is_same_v<T, math::Vec3>) glUniform3fv(loc, 1, &v.x);
            else if constexpr (std::is_same_v<T, math::Vec4>) glUniform4fv(loc, 1, &v.x);
            else if constexpr (std::is_same_v<T, math::Mat4>) glUniformMatrix4fv(loc, 1, GL_FALSE, &v[0].x);
        }, value);
    }

    // 3. Bind textures
    for (const auto& [name, texture] : texture_map_) {
        int unit = next_unit_++;
        glActiveTexture(GL_TEXTURE0 + unit);
        auto* gl_tex = static_cast<TextureOpenGL*>(texture.get());
        glBindTexture(GL_TEXTURE_2D, gl_tex->handle());
        GLint loc = glGetUniformLocation(prog, name.c_str());
        if (loc != -1) {
            glUniform1i(loc, unit);
        }
        BUDDD_LOG_DEBUG("Bind texture: {} (unit={})", name, unit);
    }

    // 4. Reset unit counter for next bind() call
    next_unit_ = 0;
}

} // namespace buddd::engine
