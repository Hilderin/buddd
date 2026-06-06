#include "render/phong/phong_material.h"
#include "render/phong/phong_shaders.h"
#include "render/render_device.h"
#include "render/shader.h"
#include "render/material_headless.h"
#include "render/glsl_util.h"

#include <cstdlib>
#include <sstream>

#include "log/log.h"

BUDDD_LOG_TAG("Render:Phong");
#include <string>
#include <vector>

namespace be = buddd::engine;

namespace buddd::engine {

// ============================================================================
// Known uniform names for Phong shaders
// ============================================================================
static auto build_known_uniform_names() -> std::vector<std::string> {
    std::vector<std::string> names;
    // Vertex shader
    names.emplace_back("u_mvp");
    names.emplace_back("u_model");
    names.emplace_back("u_normal_mat");
    // Fragment shader
    names.emplace_back("u_light_count");
    names.emplace_back("u_light_positions_or_dir");
    names.emplace_back("u_light_colours");
    names.emplace_back("u_light_ranges");
    names.emplace_back("u_light_spot_directions");
    names.emplace_back("u_light_inner_cones");
    names.emplace_back("u_light_outer_cones");
    names.emplace_back("u_camera_pos");
    names.emplace_back("u_material_ambient");
    names.emplace_back("u_material_specular");
    names.emplace_back("u_material_shininess");
    names.emplace_back("u_diffuse_texture");
    names.emplace_back("u_material_diffuse_tint");
    return names;
}

auto PhongMaterial::known_uniform_names() -> const std::vector<std::string>& {
    static const std::vector<std::string> names = build_known_uniform_names();
    return names;
}

// ============================================================================
// Impl — stores the inner material
// ============================================================================
struct PhongMaterial::Impl {
    std::unique_ptr<Material> inner;

    static auto create_material(RenderDevice& device,
                                std::span<const std::string> extra_uniforms)
        -> std::unique_ptr<Material>
    {
        // Create vertex and fragment shaders from embedded sources
        auto vs = device.create_shader(
            ShaderType::Vertex, detail::k_phong_vertex_shader_source);
        if (!vs) {
            BUDDD_LOG_ERROR("FATAL: Failed to create Phong vertex shader: {}", to_string(vs.error()));
            std::exit(EXIT_FAILURE);
        }

        auto fs = device.create_shader(
            ShaderType::Fragment, detail::k_phong_fragment_shader_source);
        if (!fs) {
            BUDDD_LOG_ERROR("FATAL: Failed to create Phong fragment shader: {}", to_string(fs.error()));
            std::exit(EXIT_FAILURE);
        }

        // Build combined known_uniforms list
        auto base_names = known_uniform_names();
        std::vector<std::string> all_uniforms;
        all_uniforms.reserve(base_names.size() + extra_uniforms.size());
        all_uniforms.insert(all_uniforms.end(), base_names.begin(), base_names.end());
        all_uniforms.insert(all_uniforms.end(), extra_uniforms.begin(), extra_uniforms.end());

        auto mat = device.create_material(
            std::move(*vs), std::move(*fs),
            std::span<const std::string>(all_uniforms));
        if (!mat) {
            BUDDD_LOG_ERROR("FATAL: Failed to create Phong material: {}", to_string(mat.error()));
            std::exit(EXIT_FAILURE);
        }

        return std::move(*mat);
    }
};

// ============================================================================
// Constructor / Destructor
// ============================================================================
PhongMaterial::PhongMaterial(RenderDevice& device,
                             std::span<const std::string> known_uniforms)
    : impl_(std::make_unique<Impl>())
{
    impl_->inner = Impl::create_material(device, known_uniforms);
}

PhongMaterial::~PhongMaterial() = default;

// ============================================================================
// Material interface — delegate to inner material
// ============================================================================
auto PhongMaterial::set_uniform(std::string_view name, float value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PhongMaterial::set_uniform(std::string_view name, int32_t value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PhongMaterial::set_uniform(std::string_view name, bool value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PhongMaterial::set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PhongMaterial::set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PhongMaterial::set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> {
    return impl_->inner->set_uniform(name, value);
}

auto PhongMaterial::has_uniform(std::string_view name) const -> bool {
    return impl_->inner->has_uniform(name);
}

auto PhongMaterial::set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> {
    return impl_->inner->set_texture(name, std::move(texture));
}

auto PhongMaterial::has_texture(std::string_view name) const -> bool {
    return impl_->inner->has_texture(name);
}

auto PhongMaterial::bind() const -> void {
    impl_->inner->bind();
}

auto PhongMaterial::inner_material() const noexcept -> const Material& {
    return *impl_->inner;
}

auto PhongMaterial::inner_material() noexcept -> Material& {
    return *impl_->inner;
}

// ============================================================================
// Convenience setters
// ============================================================================
auto PhongMaterial::set_camera_position(const math::Vec3& position) -> void {
    auto r = impl_->inner->set_uniform("u_camera_pos", position);
    if (!r) {
        BUDDD_LOG_WARN("PhongMaterial: set_uniform(u_camera_pos) failed: {}", to_string(r.error()));
    }
}

auto PhongMaterial::set_lights(const detail::LightData* lights, int count) -> void {
    auto r = impl_->inner->set_uniform("u_light_count", count);
    if (!r) {
        BUDDD_LOG_WARN("PhongMaterial: set_uniform(u_light_count) failed");
        return;
    }

    for (int i = 0; i < count; ++i) {
        auto const& ld = lights[i];
        std::string idx = "[" + std::to_string(i) + "]";

        (void)impl_->inner->set_uniform("u_light_positions_or_dir" + idx, ld.position_or_dir);
        (void)impl_->inner->set_uniform("u_light_colours" + idx, ld.colour);
        (void)impl_->inner->set_uniform("u_light_ranges" + idx, ld.range);
        (void)impl_->inner->set_uniform("u_light_spot_directions" + idx, ld.spot_direction);
        (void)impl_->inner->set_uniform("u_light_inner_cones" + idx, ld.inner_cone_cos);
        (void)impl_->inner->set_uniform("u_light_outer_cones" + idx, ld.outer_cone_cos);
    }
}

auto PhongMaterial::set_transforms(const math::Mat4& model, const math::Mat4& view_projection) -> void {
    (void)impl_->inner->set_uniform("u_model", model);
    auto mvp = view_projection * model;
    (void)impl_->inner->set_uniform("u_mvp", mvp);
    auto normal_mat = model.inverse().transpose();
    (void)impl_->inner->set_uniform("u_normal_mat", normal_mat);
}

} // namespace buddd::engine
