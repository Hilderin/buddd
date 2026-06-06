#include "render/pbr/pbr_material.h"
#include "render/pbr/pbr_shaders.h"
#include "render/render_device.h"
#include "render/shader.h"
#include "render/material_headless.h"
#include "render/glsl_util.h"

#include <cstdlib>
#include <string>
#include <vector>

#include "log/log.h"

BUDDD_LOG_TAG("Render:Pbr");

namespace buddd::engine {

// ============================================================================
// Known uniform names for PBR shaders
// ============================================================================
static auto build_known_uniform_names() -> std::vector<std::string> {
    std::vector<std::string> names;
    // Vertex shader
    names.emplace_back("u_mvp");
    names.emplace_back("u_model");
    names.emplace_back("u_normal_mat");
    // Fragment shader
    names.emplace_back("u_camera_pos");
    names.emplace_back("u_light_count");
    names.emplace_back("u_light_positions_or_dir");
    names.emplace_back("u_light_colours");
    names.emplace_back("u_light_ranges");
    names.emplace_back("u_light_spot_directions");
    names.emplace_back("u_light_inner_cones");
    names.emplace_back("u_light_outer_cones");
    // PBR material parameters
    names.emplace_back("u_base_color_factor");
    names.emplace_back("u_metallic_factor");
    names.emplace_back("u_roughness_factor");
    names.emplace_back("u_emissive_factor");
    // PBR texture flags
    names.emplace_back("u_has_base_color_texture");
    names.emplace_back("u_has_metallic_roughness_texture");
    names.emplace_back("u_has_normal_texture");
    names.emplace_back("u_has_occlusion_texture");
    names.emplace_back("u_has_emissive_texture");
    return names;
}

auto PbrMaterial::known_uniform_names() -> const std::vector<std::string>& {
    static const std::vector<std::string> names = build_known_uniform_names();
    return names;
}

// ============================================================================
// Impl — stores the inner material and PbrMaterialData
// ============================================================================
struct PbrMaterial::Impl {
    std::unique_ptr<Material> inner;
    PbrMaterialData data;

    static auto create_material(RenderDevice& device) -> std::unique_ptr<Material> {
        // Create vertex and fragment shaders from embedded sources
        auto vs = device.create_shader(
            ShaderType::Vertex, detail::k_pbr_vertex_shader_source);
        if (!vs) {
            BUDDD_LOG_ERROR("FATAL: Failed to create PBR vertex shader: {}", to_string(vs.error()));
            std::exit(EXIT_FAILURE);
        }

        auto fs = device.create_shader(
            ShaderType::Fragment, detail::k_pbr_fragment_shader_source);
        if (!fs) {
            BUDDD_LOG_ERROR("FATAL: Failed to create PBR fragment shader: {}", to_string(fs.error()));
            std::exit(EXIT_FAILURE);
        }

        auto mat = device.create_material(
            std::move(*vs), std::move(*fs),
            std::span<const std::string>(known_uniform_names()));
        if (!mat) {
            BUDDD_LOG_ERROR("FATAL: Failed to create PBR material: {}", to_string(mat.error()));
            std::exit(EXIT_FAILURE);
        }

        return std::move(*mat);
    }
};

// ============================================================================
// Constructor / Destructor
// ============================================================================
PbrMaterial::PbrMaterial(RenderDevice& device)
    : impl_(std::make_unique<Impl>())
{
    impl_->inner = Impl::create_material(device);
}

PbrMaterial::~PbrMaterial() = default;

// ============================================================================
// set_data / data
// ============================================================================
auto PbrMaterial::set_data(const PbrMaterialData& data) -> void {
    impl_->data = data;

    // Set uniform factors
    (void)impl_->inner->set_uniform("u_base_color_factor", data.base_color_factor);
    (void)impl_->inner->set_uniform("u_metallic_factor", data.metallic_factor);
    (void)impl_->inner->set_uniform("u_roughness_factor", data.roughness_factor);
    (void)impl_->inner->set_uniform("u_emissive_factor", data.emissive_factor);

    // Set has-texture flags and bind textures
    auto set_texture_uniform = [&](const std::string& flag_name,
                                   const std::string& tex_name,
                                   const std::shared_ptr<Texture>& tex) {
        if (tex) {
            (void)impl_->inner->set_uniform(flag_name, 1.0f);
            (void)impl_->inner->set_texture(tex_name, tex);
        } else {
            (void)impl_->inner->set_uniform(flag_name, 0.0f);
        }
    };

    set_texture_uniform("u_has_base_color_texture", "u_base_color_texture",
                        data.base_color_texture);
    set_texture_uniform("u_has_metallic_roughness_texture", "u_metallic_roughness_texture",
                        data.metallic_roughness_texture);
    set_texture_uniform("u_has_normal_texture", "u_normal_texture",
                        data.normal_texture);
    set_texture_uniform("u_has_occlusion_texture", "u_occlusion_texture",
                        data.occlusion_texture);
    set_texture_uniform("u_has_emissive_texture", "u_emissive_texture",
                        data.emissive_texture);
}

auto PbrMaterial::data() const noexcept -> const PbrMaterialData& {
    return impl_->data;
}

// ============================================================================
// Material interface — delegate to inner material
// ============================================================================
auto PbrMaterial::set_uniform(std::string_view name, float value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PbrMaterial::set_uniform(std::string_view name, int32_t value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PbrMaterial::set_uniform(std::string_view name, bool value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PbrMaterial::set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PbrMaterial::set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PbrMaterial::set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PbrMaterial::has_uniform(std::string_view name) const -> bool {
    return impl_->inner->has_uniform(name);
}

auto PbrMaterial::set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> {
    return impl_->inner->set_texture(name, std::move(texture));
}

auto PbrMaterial::has_texture(std::string_view name) const -> bool {
    return impl_->inner->has_texture(name);
}

auto PbrMaterial::bind() const -> void {
    impl_->inner->bind();
}

auto PbrMaterial::inner_material() const noexcept -> const Material& {
    return *impl_->inner;
}

auto PbrMaterial::inner_material() noexcept -> Material& {
    return *impl_->inner;
}

} // namespace buddd::engine
