#include "material_headless.h"
#include "render/glsl_util.h"

#include <optional>

#include "log/log.h"

BUDDD_LOG_TAG("Render:Headless");

namespace buddd::engine {

MaterialHeadless::MaterialHeadless(std::unordered_set<std::string> known_uniforms)
    : known_uniforms_(std::move(known_uniforms)) {}

auto MaterialHeadless::has_uniform(std::string_view name) const -> bool {
    auto norm_key = detail::normalize_uniform_name(name);
    return known_uniforms_.count(norm_key) > 0
        || uniform_values_.count(std::string(name)) > 0;
}

auto MaterialHeadless::set_uniform(std::string_view name, float value) -> Result<void> {
    auto exact_key = std::string(name);
    auto norm_key = detail::normalize_uniform_name(name);
    if (known_uniforms_.count(norm_key) == 0 && uniform_values_.count(exact_key) == 0) {
        BUDDD_LOG_ERROR("Uniform not found: {}", name);
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(norm_key);
    uniform_values_[exact_key] = value;
    BUDDD_LOG_DEBUG("Uniform set: {} (type=float)", name);
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, int32_t value) -> Result<void> {
    auto exact_key = std::string(name);
    auto norm_key = detail::normalize_uniform_name(name);
    if (known_uniforms_.count(norm_key) == 0 && uniform_values_.count(exact_key) == 0) {
        BUDDD_LOG_ERROR("Uniform not found: {}", name);
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(norm_key);
    uniform_values_[exact_key] = value;
    BUDDD_LOG_DEBUG("Uniform set: {} (type=int)", name);
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, bool value) -> Result<void> {
    auto exact_key = std::string(name);
    auto norm_key = detail::normalize_uniform_name(name);
    if (known_uniforms_.count(norm_key) == 0 && uniform_values_.count(exact_key) == 0) {
        BUDDD_LOG_ERROR("Uniform not found: {}", name);
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(norm_key);
    uniform_values_[exact_key] = value;
    BUDDD_LOG_DEBUG("Uniform set: {} (type=bool)", name);
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> {
    auto exact_key = std::string(name);
    auto norm_key = detail::normalize_uniform_name(name);
    if (known_uniforms_.count(norm_key) == 0 && uniform_values_.count(exact_key) == 0) {
        BUDDD_LOG_ERROR("Uniform not found: {}", name);
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(norm_key);
    uniform_values_[exact_key] = value;
    BUDDD_LOG_DEBUG("Uniform set: {} (type=Vec3)", name);
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> {
    auto exact_key = std::string(name);
    auto norm_key = detail::normalize_uniform_name(name);
    if (known_uniforms_.count(norm_key) == 0 && uniform_values_.count(exact_key) == 0) {
        BUDDD_LOG_ERROR("Uniform not found: {}", name);
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(norm_key);
    uniform_values_[exact_key] = value;
    BUDDD_LOG_DEBUG("Uniform set: {} (type=Vec4)", name);
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> {
    auto exact_key = std::string(name);
    auto norm_key = detail::normalize_uniform_name(name);
    if (known_uniforms_.count(norm_key) == 0 && uniform_values_.count(exact_key) == 0) {
        BUDDD_LOG_ERROR("Uniform not found: {}", name);
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(norm_key);
    uniform_values_[exact_key] = value;
    BUDDD_LOG_DEBUG("Uniform set: {} (type=Mat4)", name);
    return {};
}

auto MaterialHeadless::set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> {
    if (!texture) {
        return make_error(Error::Category::InvalidArgument,
            "Texture is null");
    }

    auto exact_key = std::string(name);
    auto norm_key = detail::normalize_uniform_name(name);
    if (known_uniforms_.count(norm_key) == 0 && uniform_values_.count(exact_key) == 0) {
        BUDDD_LOG_ERROR("Uniform not found: {}", name);
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }

    known_uniforms_.insert(norm_key);
    texture_values_[exact_key] = std::move(texture);
    BUDDD_LOG_DEBUG("Texture set (Headless): {}", name);
    return {};
}

auto MaterialHeadless::has_texture(std::string_view name) const -> bool {
    auto key = std::string(name);
    return known_uniforms_.count(key) > 0 || texture_values_.count(key) > 0;
}

auto MaterialHeadless::bind() const -> void {
    // No-op for headless backend
}

auto MaterialHeadless::get_texture(std::string_view name) const -> std::optional<std::shared_ptr<Texture>> {
    auto it = texture_values_.find(std::string(name));
    if (it == texture_values_.end()) {
        return std::nullopt;
    }
    return it->second;
}

auto MaterialHeadless::get_uniform_mat4(std::string_view name) const -> std::optional<math::Mat4> {
    auto key = std::string(name);
    auto it = uniform_values_.find(key);
    if (it == uniform_values_.end()) {
        // Try normalized key (strip array subscript)
        key = detail::normalize_uniform_name(name);
        it = uniform_values_.find(key);
        if (it == uniform_values_.end()) return std::nullopt;
    }
    if (!std::holds_alternative<math::Mat4>(it->second)) return std::nullopt;
    return std::get<math::Mat4>(it->second);
}

auto MaterialHeadless::get_uniform_vec3(std::string_view name) const -> std::optional<math::Vec3> {
    auto key = std::string(name);
    auto it = uniform_values_.find(key);
    if (it == uniform_values_.end()) {
        key = detail::normalize_uniform_name(name);
        it = uniform_values_.find(key);
        if (it == uniform_values_.end()) return std::nullopt;
    }
    if (!std::holds_alternative<math::Vec3>(it->second)) return std::nullopt;
    return std::get<math::Vec3>(it->second);
}

auto MaterialHeadless::get_uniform_vec4(std::string_view name) const -> std::optional<math::Vec4> {
    auto key = std::string(name);
    auto it = uniform_values_.find(key);
    if (it == uniform_values_.end()) {
        key = detail::normalize_uniform_name(name);
        it = uniform_values_.find(key);
        if (it == uniform_values_.end()) return std::nullopt;
    }
    if (!std::holds_alternative<math::Vec4>(it->second)) return std::nullopt;
    return std::get<math::Vec4>(it->second);
}

auto MaterialHeadless::get_uniform_float(std::string_view name) const -> std::optional<float> {
    auto key = std::string(name);
    auto it = uniform_values_.find(key);
    if (it == uniform_values_.end()) {
        key = detail::normalize_uniform_name(name);
        it = uniform_values_.find(key);
        if (it == uniform_values_.end()) return std::nullopt;
    }
    if (!std::holds_alternative<float>(it->second)) return std::nullopt;
    return std::get<float>(it->second);
}

auto MaterialHeadless::get_uniform_int(std::string_view name) const -> std::optional<int32_t> {
    auto key = std::string(name);
    auto it = uniform_values_.find(key);
    if (it == uniform_values_.end()) {
        key = detail::normalize_uniform_name(name);
        it = uniform_values_.find(key);
        if (it == uniform_values_.end()) return std::nullopt;
    }
    if (!std::holds_alternative<int32_t>(it->second)) return std::nullopt;
    return std::get<int32_t>(it->second);
}

} // namespace buddd::engine
