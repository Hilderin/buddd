#include "render/render_system.h"

#include "error.h"            // to_string()
#include "render/render_device.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "render/mesh_renderer.h"

#include <iostream>

namespace buddd::engine {

RenderSystem::RenderSystem(RenderDevice& device, World& world)
    : device_(&device), world_(&world) {}

auto RenderSystem::render() -> void {
    device_->begin_frame();

    auto cam_opt = world_->active_camera();
    if (!cam_opt.has_value()) {
        std::cerr << "RenderSystem: no active camera — rendering skipped\n";
        device_->end_frame();
        return;
    }
    auto& cam_comp = *cam_opt;  // optional<CameraComponent&> — operator* yields CameraComponent&
    auto vp = cam_comp.camera().view_projection_matrix();

    world_->each<MeshRenderer>([&](Entity entity, MeshRenderer& mr) -> bool {
        auto world_mat = entity.world_matrix();
        auto mvp = vp * world_mat;

        auto uniform_result = mr.model().material().set_uniform("u_mvp", mvp);
        if (!uniform_result) {
            std::cerr << "RenderSystem: set_uniform(u_mvp) failed for entity "
                      << entity.id().index << ": "
                      << to_string(uniform_result.error()) << "\n";
            return true;
        }

        mr.model().draw(*device_);
        return true;
    });

    device_->end_frame();
}

} // namespace buddd::engine
