#include "render/render_system.h"

#include "error.h"            // to_string()
#include "render/render_device.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "render/mesh_renderer.h"
#include "scene/directional_light_component.h"
#include "scene/point_light_component.h"
#include "scene/spot_light_component.h"
#include "render/light_data.h"

#include <array>
#include <cmath>

#include "log/log.h"

BUDDD_LOG_TAG("Render");

namespace buddd::engine {

RenderSystem::RenderSystem(RenderDevice& device, World& world)
    : device_(&device), world_(&world) {}

auto RenderSystem::render() -> void {
    device_->begin_frame();
    render_scene();
    device_->end_frame();
}

auto RenderSystem::render_scene() -> void {
    auto cam_opt = world_->active_camera();
    if (!cam_opt.has_value()) {
        BUDDD_LOG_TRACE("RenderSystem: no active camera — rendering skipped");
        return;
    }
    auto& cam_comp = *cam_opt;
    auto vp = cam_comp.view_projection_matrix();
    auto camera_pos = cam_comp.entity().transform().position;

    // --- 1. Collect lights ---
    std::array<detail::LightData, detail::k_max_lights> light_data{};
    int light_count = 0;

    // Directional lights (type 0)
    world_->each<DirectionalLightComponent>([&](Entity entity, DirectionalLightComponent& lc) -> bool {
        if (light_count >= detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto forward = math::Vec4{0.0f, 0.0f, -1.0f, 0.0f};
        auto dir_v4 = world_mat * forward;
        math::Vec3 dir = {dir_v4.x, dir_v4.y, dir_v4.z};
        dir.normalize();
        auto& ld = light_data[light_count];
        ld.position_or_dir = {dir.x, dir.y, dir.z, 0.0f};
        ld.color = {lc.color().x * lc.intensity(),
                     lc.color().y * lc.intensity(),
                     lc.color().z * lc.intensity(), 1.0f};
        ld.range = 0.0f;
        ld.spot_direction = {0.0f, 0.0f, 0.0f, 0.0f};
        ld.inner_cone_cos = 1.0f;
        ld.outer_cone_cos = 1.0f;
        ++light_count;
        return true;
    });

    // Point lights (type 1)
    world_->each<PointLightComponent>([&](Entity entity, PointLightComponent& lc) -> bool {
        if (light_count >= detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto pos_v4 = world_mat * math::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
        auto& ld = light_data[light_count];
        ld.position_or_dir = {pos_v4.x, pos_v4.y, pos_v4.z, 1.0f};
        ld.color = {lc.color().x * lc.intensity(),
                     lc.color().y * lc.intensity(),
                     lc.color().z * lc.intensity(), 1.0f};
        ld.range = lc.range();
        ld.spot_direction = {0.0f, 0.0f, 0.0f, 0.0f};
        ld.inner_cone_cos = 1.0f;
        ld.outer_cone_cos = 1.0f;
        ++light_count;
        return true;
    });

    // Spot lights (type 2)
    world_->each<SpotLightComponent>([&](Entity entity, SpotLightComponent& lc) -> bool {
        if (light_count >= detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto pos_v4 = world_mat * math::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
        auto forward = math::Vec4{0.0f, 0.0f, -1.0f, 0.0f};
        auto dir_v4 = world_mat * forward;
        math::Vec3 dir = {dir_v4.x, dir_v4.y, dir_v4.z};
        dir.normalize();
        auto& ld = light_data[light_count];
        ld.position_or_dir = {pos_v4.x, pos_v4.y, pos_v4.z, 2.0f};
        ld.color = {lc.color().x * lc.intensity(),
                     lc.color().y * lc.intensity(),
                     lc.color().z * lc.intensity(), 1.0f};
        ld.range = lc.range();
        ld.spot_direction = {dir.x, dir.y, dir.z, 0.0f};
        ld.inner_cone_cos = std::cos(lc.inner_angle());
        ld.outer_cone_cos = std::cos(lc.outer_angle());
        ++light_count;
        return true;
    });

    if (light_count > 0) {
        BUDDD_LOG_DEBUG("RenderSystem: collected {} lights", light_count);
    }

    // --- 2. Iterate MeshRenderers ---
    world_->each<MeshRenderer>([&](Entity entity, MeshRenderer& mr) -> bool {
        auto world_mat = entity.world_matrix();
        auto mvp = vp * world_mat;
        auto& mats = mr.model().materials();
        if (mats.empty()) return true;
        auto& material = *mats[0];

        // Always set u_mvp (backward compat)
        auto r = material.set_uniform("u_mvp", mvp);
        if (!r) {
            BUDDD_LOG_ERROR("RenderSystem: set_uniform(u_mvp) failed for entity {}: {}", entity.id().index, to_string(r.error()));
            return true;
        }

        // Check if this material supports lighting (has u_model uniform)
        if (material.has_uniform("u_model")) {
            (void)material.set_uniform("u_model", world_mat);
            auto normal_mat = world_mat.inverse().transpose();
            (void)material.set_uniform("u_normal_mat", normal_mat);
            (void)material.set_uniform("u_camera_pos", camera_pos);

            (void)material.set_uniform("u_light_count", light_count);
            for (int i = 0; i < light_count; ++i) {
                auto const& ld = light_data[i];
                (void)material.set_uniform(
                    "u_light_positions_or_dir[" + std::to_string(i) + "]", ld.position_or_dir);
                (void)material.set_uniform(
                    "u_light_colors[" + std::to_string(i) + "]", ld.color);
                (void)material.set_uniform(
                    "u_light_ranges[" + std::to_string(i) + "]", ld.range);
                (void)material.set_uniform(
                    "u_light_spot_directions[" + std::to_string(i) + "]", ld.spot_direction);
                (void)material.set_uniform(
                    "u_light_inner_cones[" + std::to_string(i) + "]", ld.inner_cone_cos);
                (void)material.set_uniform(
                    "u_light_outer_cones[" + std::to_string(i) + "]", ld.outer_cone_cos);
            }

            // Material defaults are NOT set here — they are provided by GLSL shader defaults
            // (u_material_ambient = 0.1, u_material_specular = 1.0, 
            //  u_material_shininess = 32.0, u_material_diffuse_tint = 1.0).
            // The demo or application sets custom values during material creation.
        }

        mr.model().draw(*device_);
        return true;
    });
}

} // namespace buddd::engine
