#include "material_headless.h"

#include <iostream>

namespace buddd::engine {

MaterialHeadless::MaterialHeadless(std::unordered_set<std::string> known_uniforms)
    : known_uniforms_(std::move(known_uniforms)) {}

auto MaterialHeadless::has_uniform(std::string_view name) const -> bool {
    return known_uniforms_.count(std::string(name)) > 0
        || uniform_values_.count(std::string(name)) > 0;
}

auto MaterialHeadless::set_uniform(std::string_view name, float value) -> Result<void> {
    auto key = std::string(name);
    if (known_uniforms_.count(key) == 0 && uniform_values_.count(key) == 0) {
        std::cerr << "Uniform not found: " << name << "\n";
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(key);
    uniform_values_[key] = value;
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=float)\n";
#endif
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, int32_t value) -> Result<void> {
    auto key = std::string(name);
    if (known_uniforms_.count(key) == 0 && uniform_values_.count(key) == 0) {
        std::cerr << "Uniform not found: " << name << "\n";
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(key);
    uniform_values_[key] = value;
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=int)\n";
#endif
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, bool value) -> Result<void> {
    auto key = std::string(name);
    if (known_uniforms_.count(key) == 0 && uniform_values_.count(key) == 0) {
        std::cerr << "Uniform not found: " << name << "\n";
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(key);
    uniform_values_[key] = value;
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=bool)\n";
#endif
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> {
    auto key = std::string(name);
    if (known_uniforms_.count(key) == 0 && uniform_values_.count(key) == 0) {
        std::cerr << "Uniform not found: " << name << "\n";
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(key);
    uniform_values_[key] = value;
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Vec3)\n";
#endif
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> {
    auto key = std::string(name);
    if (known_uniforms_.count(key) == 0 && uniform_values_.count(key) == 0) {
        std::cerr << "Uniform not found: " << name << "\n";
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(key);
    uniform_values_[key] = value;
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Vec4)\n";
#endif
    return {};
}

auto MaterialHeadless::set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> {
    auto key = std::string(name);
    if (known_uniforms_.count(key) == 0 && uniform_values_.count(key) == 0) {
        std::cerr << "Uniform not found: " << name << "\n";
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(key);
    uniform_values_[key] = value;
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Mat4)\n";
#endif
    return {};
}

} // namespace buddd::engine
