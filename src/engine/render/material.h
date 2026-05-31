#pragma once

#include "error.h"
#include "shader.h"

#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace buddd::engine {

class Texture;

class Material {
public:
    virtual ~Material() = default;

    virtual auto set_uniform(std::string_view name, float value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, int32_t value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, bool value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> = 0;

    virtual auto has_uniform(std::string_view name) const -> bool = 0;

    virtual auto set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> = 0;
    virtual auto has_texture(std::string_view name) const -> bool = 0;

    /// Applies all pending material state: activates the shader program
    /// (if applicable), applies cached uniforms, binds textures.
    /// Called by RenderDevice::draw() at the start of each draw call.
    virtual auto bind() const -> void = 0;

    Material(const Material&) = delete;
    auto operator=(const Material&) -> Material& = delete;
    Material(Material&&) = delete;
    auto operator=(Material&&) -> Material& = delete;

protected:
    Material() = default;
};

} // namespace buddd::engine
