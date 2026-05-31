#pragma once

#include "material.h"
#include "render/texture.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace buddd::engine {

class MaterialHeadless final : public Material {
public:
    explicit MaterialHeadless(std::unordered_set<std::string> known_uniforms);
    ~MaterialHeadless() override = default;

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

    /// Returns the last-set Mat4 value for the given uniform name, or
    /// std::nullopt if the uniform has not been set or is not of Mat4 type.
    auto get_uniform_mat4(std::string_view name) const -> std::optional<math::Mat4>;

    /// Returns the last-set Vec3 value for the given uniform name, or
    /// std::nullopt if the uniform has not been set or is not of Vec3 type.
    auto get_uniform_vec3(std::string_view name) const -> std::optional<math::Vec3>;

    /// Returns the last-set Vec4 value for the given uniform name, or
    /// std::nullopt if the uniform has not been set or is not of Vec4 type.
    auto get_uniform_vec4(std::string_view name) const -> std::optional<math::Vec4>;

    /// Returns the last-set float value for the given uniform name, or
    /// std::nullopt if the uniform has not been set or is not of float type.
    auto get_uniform_float(std::string_view name) const -> std::optional<float>;

    /// Returns the last-set int32_t value for the given uniform name, or
    /// std::nullopt if the uniform has not been set or is not of int32_t type.
    auto get_uniform_int(std::string_view name) const -> std::optional<int32_t>;

    /// Returns the texture for the given name, or std::nullopt if not set.
    auto get_texture(std::string_view name) const -> std::optional<std::shared_ptr<Texture>>;

    MaterialHeadless(const MaterialHeadless&) = delete;
    auto operator=(const MaterialHeadless&) -> MaterialHeadless& = delete;
    MaterialHeadless(MaterialHeadless&&) = delete;
    auto operator=(MaterialHeadless&&) -> MaterialHeadless& = delete;

private:
    std::unordered_set<std::string> known_uniforms_;
    std::unordered_map<std::string, std::variant<float, int32_t, bool, math::Vec3, math::Vec4, math::Mat4>> uniform_values_;
    std::unordered_map<std::string, std::shared_ptr<Texture>> texture_values_;
};

} // namespace buddd::engine
