#pragma once

#include "material.h"

#include <SDL3/SDL_opengl.h>

#include <string>
#include <unordered_map>

namespace buddd::engine {

class MaterialOpenGL final : public Material {
public:
    explicit MaterialOpenGL(GLuint program);
    ~MaterialOpenGL() override;

    auto set_uniform(std::string_view name, float value) -> Result<void> override;
    auto set_uniform(std::string_view name, int32_t value) -> Result<void> override;
    auto set_uniform(std::string_view name, bool value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> override;

    auto has_uniform(std::string_view name) const -> bool override;

    auto program() const noexcept -> GLuint;

    MaterialOpenGL(const MaterialOpenGL&) = delete;
    auto operator=(const MaterialOpenGL&) -> MaterialOpenGL& = delete;
    MaterialOpenGL(MaterialOpenGL&&) = delete;
    auto operator=(MaterialOpenGL&&) -> MaterialOpenGL& = delete;

private:
    auto get_uniform_location(std::string_view name) -> Result<GLint>;

    GLuint program_;
    mutable std::unordered_map<std::string, GLint> location_cache_;
};

} // namespace buddd::engine
