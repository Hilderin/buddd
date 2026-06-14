#pragma once

#include "math/color.h"
#include "render/material.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::engine {

class RenderDevice;
class Texture;

struct PbrMaterialData {
    math::Color base_color_factor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic_factor{1.0f};
    float roughness_factor{1.0f};
    math::Vec3 emissive_factor{0.0f, 0.0f, 0.0f};
    bool double_sided{false}; // NOTE: stored but NOT applied to rendering in V1
                              // (face culling left at backend default)

    std::shared_ptr<Texture> base_color_texture;
    std::shared_ptr<Texture> metallic_roughness_texture;
    std::shared_ptr<Texture> normal_texture;
    std::shared_ptr<Texture> occlusion_texture;
    std::shared_ptr<Texture> emissive_texture;
};

class PbrMaterial final : public Material {
public:
    explicit PbrMaterial(RenderDevice& device);

    auto set_data(const PbrMaterialData& data) -> void;
    auto data() const noexcept -> const PbrMaterialData&;

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

    static auto known_uniform_names() -> const std::vector<std::string>&;

    /// Returns the inner Material for diagnostic access (tests).
    auto inner_material() const noexcept -> const Material&;
    auto inner_material() noexcept -> Material&;

    ~PbrMaterial() override;

    PbrMaterial(const PbrMaterial&) = delete;
    auto operator=(const PbrMaterial&) -> PbrMaterial& = delete;
    PbrMaterial(PbrMaterial&&) = delete;
    auto operator=(PbrMaterial&&) -> PbrMaterial& = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace buddd::engine
