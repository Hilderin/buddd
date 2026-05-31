#pragma once

#include "material.h"
#include "render/texture.h"

#include <SDL3/SDL_opengl.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

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

    auto set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> override;
    auto has_texture(std::string_view name) const -> bool override;
    auto bind() const -> void override;

    auto program() const noexcept -> GLuint;

    MaterialOpenGL(const MaterialOpenGL&) = delete;
    auto operator=(const MaterialOpenGL&) -> MaterialOpenGL& = delete;
    MaterialOpenGL(MaterialOpenGL&&) = delete;
    auto operator=(MaterialOpenGL&&) -> MaterialOpenGL& = delete;

private:
    auto get_uniform_location(std::string_view name) -> Result<GLint>;

    GLuint program_;
    mutable std::unordered_map<std::string, GLint> location_cache_;

    std::unordered_map<std::string, std::variant<float, int32_t, bool, math::Vec3, math::Vec4, math::Mat4>> uniform_cache_;
    std::unordered_map<std::string, std::shared_ptr<Texture>> texture_map_;
    mutable int next_unit_{0};
};

} // namespace buddd::engine
