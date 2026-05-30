#include "demo/cube_scene_demo.h"
#include "demo/demo_helpers.h"

#include "platform/platform.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "scene/world.h"
#include "scene/camera_component.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace be = buddd::engine;

auto buddd::cmd::demo::run_cube_scene_demo(
    be::Platform& platform, be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int
{
    // 1. Create a World (scene container)
    be::World world;

    // 2. Create a single entity that will be both the camera and the renderable cube
    auto entity = be::Entity::create(world);

    // 3. Attach CameraComponent (with same camera params as before)
    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},
        be::math::Vec3{0.0f, 0.0f, 0.0f},
        be::math::Vec3::unit_y()
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );
    entity.add_component<be::CameraComponent>(camera);

    // 4. Attach MeshRenderer with the cube model
    auto cube = setup_cube(device);
    // cube.material is deliberately discarded — Model already holds a
    // shared_ptr<Material> internally (accessed via Model::material()).
    entity.add_component<be::MeshRenderer>(std::make_shared<be::Model>(std::move(cube.model)));

    // 5. Create the RenderSystem (bridges World + RenderDevice)
    be::RenderSystem render_system(device, world);

    // 6. Render loop: ~120 frames at 60 FPS
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16);
    auto demo_start = std::chrono::steady_clock::now();

    std::cerr << "Demo started: cube (scene-based, " << target_frames << " frames)\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!platform.poll_events()) {
            std::cerr << "Demo aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        // Update the entity's rotation each frame
        auto elapsed = std::chrono::steady_clock::now() - demo_start;
        float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
        float angle = elapsed_seconds * 0.5f;
        entity.transform().rotation =
            be::math::Quat::angle_axis(angle, be::math::Vec3::unit_y());

        // Let RenderSystem handle all rendering
        render_system.render();

        // Frame rate limiting
        auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
        if (frame_elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_elapsed);
        }
    }

    std::cerr << "Demo complete: cube (scene-based, " << target_frames << " frames rendered)\n";
    return EXIT_SUCCESS;
}
