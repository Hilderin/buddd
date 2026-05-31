#pragma once

#include "render/material.h"
#include "render/light_data.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::engine {

class RenderDevice;

class PhongMaterial final : public Material {
public:
    /// Creates a PhongMaterial with embedded Phong vertex/fragment shaders.
    /// @param device          RenderDevice used to create shader program.
    /// @param known_uniforms  Additional known uniform names beyond the standard Phong set.
    explicit PhongMaterial(RenderDevice& device,
                           std::span<const std::string> known_uniforms = {});

    ~PhongMaterial() override;

    PhongMaterial(const PhongMaterial&) = delete;
    auto operator=(const PhongMaterial&) -> PhongMaterial& = delete;
    PhongMaterial(PhongMaterial&&) = delete;
    auto operator=(PhongMaterial&&) -> PhongMaterial& = delete;

    // -- Material interface --
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

    // -- Convenience setters --
    auto set_camera_position(const math::Vec3& position) -> void;
    auto set_lights(const detail::LightData* lights, int count) -> void;
    auto set_transforms(const math::Mat4& model, const math::Mat4& view_projection) -> void;

    /// Returns the list of known uniform names declared in the Phong shaders.
    static auto known_uniform_names() -> const std::vector<std::string>&;

    /// Returns the inner Material (e.g., MaterialHeadless) for diagnostic access.
    /// Used by tests to downcast to MaterialHeadless.
    auto inner_material() const noexcept -> const Material&;
    auto inner_material() noexcept -> Material&;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace buddd::engine
