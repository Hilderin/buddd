#pragma once

#include "material.h"

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

    /// Returns the last-set Mat4 value for the given uniform name, or
    /// std::nullopt if the uniform has not been set or is not of Mat4 type.
    auto get_uniform_mat4(std::string_view name) const -> std::optional<math::Mat4>;

    MaterialHeadless(const MaterialHeadless&) = delete;
    auto operator=(const MaterialHeadless&) -> MaterialHeadless& = delete;
    MaterialHeadless(MaterialHeadless&&) = delete;
    auto operator=(MaterialHeadless&&) -> MaterialHeadless& = delete;

private:
    std::unordered_set<std::string> known_uniforms_;
    std::unordered_map<std::string, std::variant<float, int32_t, bool, math::Vec3, math::Vec4, math::Mat4>> uniform_values_;
};

} // namespace buddd::engine
