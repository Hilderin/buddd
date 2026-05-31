#include "capture/phong_capture.h"
#include "demo/demo_helpers.h"

#include "platform/platform.h"
#include "render/render_device.h"
#include "render/mesh_renderer.h"
#include "render/texture.h"
#include "render/vertex.h"
#include "render/light_data.h"
#include "render/phong/phong_material.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/directional_light_component.h"
#include "scene/point_light_component.h"
#include "scene/spot_light_component.h"
#include "image/image.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/quat.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace be = buddd::engine;

// ============================================================================
// Procedural texture helpers (duplicated from phong_demo.cpp for capture use)
// ============================================================================

static auto make_checkerboard_texture(be::RenderDevice& device) -> std::shared_ptr<be::Texture> {
    constexpr int SIZE = 8;
    constexpr int CHANNELS = 3;
    std::vector<std::byte> pixels(static_cast<size_t>(SIZE) * SIZE * CHANNELS);

    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            bool white = ((x + y) % 2) == 0;
            uint8_t val = white ? 255 : 0;
            size_t offset = static_cast<size_t>(y * SIZE + x) * CHANNELS;
            pixels[offset + 0] = static_cast<std::byte>(val);
            pixels[offset + 1] = static_cast<std::byte>(val);
            pixels[offset + 2] = static_cast<std::byte>(val);
        }
    }

    be::ImageBuffer buf;
    buf.width = SIZE;
    buf.height = SIZE;
    buf.channels = CHANNELS;
    buf.data = std::move(pixels);

    auto image = be::Image::create(buf);
    if (!image) {
        std::cerr << "FATAL: Failed to create checkerboard image: "
                  << be::to_string(image.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    auto tex = device.create_texture(*image);
    if (!tex) {
        std::cerr << "FATAL: Failed to create checkerboard texture: "
                  << be::to_string(tex.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    return std::shared_ptr<be::Texture>(std::move(*tex));
}

static auto make_solid_texture(be::RenderDevice& device, uint8_t r, uint8_t g, uint8_t b)
    -> std::shared_ptr<be::Texture>
{
    constexpr int SIZE = 1;
    constexpr int CHANNELS = 3;
    std::vector<std::byte> pixels(static_cast<size_t>(SIZE) * SIZE * CHANNELS);
    pixels[0] = static_cast<std::byte>(r);
    pixels[1] = static_cast<std::byte>(g);
    pixels[2] = static_cast<std::byte>(b);

    be::ImageBuffer buf;
    buf.width = SIZE;
    buf.height = SIZE;
    buf.channels = CHANNELS;
    buf.data = std::move(pixels);

    auto image = be::Image::create(buf);
    if (!image) {
        std::cerr << "FATAL: Failed to create solid image: "
                  << be::to_string(image.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    auto tex = device.create_texture(*image);
    if (!tex) {
        std::cerr << "FATAL: Failed to create solid texture: "
                  << be::to_string(tex.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    return std::shared_ptr<be::Texture>(std::move(*tex));
}

// ============================================================================
// Cube model factory (duplicated from phong_demo.cpp)
// ============================================================================

static auto create_phong_cube(be::RenderDevice& device,
                              std::shared_ptr<be::PhongMaterial> material)
    -> be::Model
{
    using be::Vertex;
    // 24 vertices: position + normal + texcoord per face
    const Vertex vertices[] = {
        // +X face (right): normal (1,0,0)
        {{ 1.f, -1.f, -1.f}, {}, { 1.f, 0.f, 0.f}, {0.f, 0.f}, {}, {}},
        {{ 1.f, -1.f,  1.f}, {}, { 1.f, 0.f, 0.f}, {1.f, 0.f}, {}, {}},
        {{ 1.f,  1.f,  1.f}, {}, { 1.f, 0.f, 0.f}, {1.f, 1.f}, {}, {}},
        {{ 1.f,  1.f, -1.f}, {}, { 1.f, 0.f, 0.f}, {0.f, 1.f}, {}, {}},
        // -X face (left): normal (-1,0,0)
        {{-1.f, -1.f, -1.f}, {}, {-1.f, 0.f, 0.f}, {0.f, 0.f}, {}, {}},
        {{-1.f, -1.f,  1.f}, {}, {-1.f, 0.f, 0.f}, {1.f, 0.f}, {}, {}},
        {{-1.f,  1.f,  1.f}, {}, {-1.f, 0.f, 0.f}, {1.f, 1.f}, {}, {}},
        {{-1.f,  1.f, -1.f}, {}, {-1.f, 0.f, 0.f}, {0.f, 1.f}, {}, {}},
        // +Y face (top): normal (0,1,0)
        {{-1.f,  1.f,  1.f}, {}, {0.f, 1.f, 0.f}, {0.f, 0.f}, {}, {}},
        {{ 1.f,  1.f,  1.f}, {}, {0.f, 1.f, 0.f}, {1.f, 0.f}, {}, {}},
        {{ 1.f,  1.f, -1.f}, {}, {0.f, 1.f, 0.f}, {1.f, 1.f}, {}, {}},
        {{-1.f,  1.f, -1.f}, {}, {0.f, 1.f, 0.f}, {0.f, 1.f}, {}, {}},
        // -Y face (bottom): normal (0,-1,0)
        {{-1.f, -1.f, -1.f}, {}, {0.f, -1.f, 0.f}, {0.f, 0.f}, {}, {}},
        {{ 1.f, -1.f, -1.f}, {}, {0.f, -1.f, 0.f}, {1.f, 0.f}, {}, {}},
        {{ 1.f, -1.f,  1.f}, {}, {0.f, -1.f, 0.f}, {1.f, 1.f}, {}, {}},
        {{-1.f, -1.f,  1.f}, {}, {0.f, -1.f, 0.f}, {0.f, 1.f}, {}, {}},
        // +Z face (front): normal (0,0,1)
        {{-1.f, -1.f,  1.f}, {}, {0.f, 0.f, 1.f}, {0.f, 0.f}, {}, {}},
        {{ 1.f, -1.f,  1.f}, {}, {0.f, 0.f, 1.f}, {1.f, 0.f}, {}, {}},
        {{ 1.f,  1.f,  1.f}, {}, {0.f, 0.f, 1.f}, {1.f, 1.f}, {}, {}},
        {{-1.f,  1.f,  1.f}, {}, {0.f, 0.f, 1.f}, {0.f, 1.f}, {}, {}},
        // -Z face (back): normal (0,0,-1)
        {{ 1.f, -1.f, -1.f}, {}, {0.f, 0.f, -1.f}, {0.f, 0.f}, {}, {}},
        {{-1.f, -1.f, -1.f}, {}, {0.f, 0.f, -1.f}, {1.f, 0.f}, {}, {}},
        {{-1.f,  1.f, -1.f}, {}, {0.f, 0.f, -1.f}, {1.f, 1.f}, {}, {}},
        {{ 1.f,  1.f, -1.f}, {}, {0.f, 0.f, -1.f}, {0.f, 1.f}, {}, {}},
    };

    const uint16_t indices[] = {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23,
    };

    auto model = be::Model::create_indexed(
        device, be::k_standard_vertex_format,
        std::as_bytes(std::span(vertices)),
        std::as_bytes(std::span(indices)),
        be::IndexType::Uint16,
        std::move(material));
    if (!model) {
        std::cerr << "FATAL: Failed to create phong cube model: "
                  << be::to_string(model.error()) << "\n";
        std::exit(EXIT_FAILURE);
    }

    return std::move(*model);
}

// ============================================================================
// Material characteristics for each cube (same as phong_demo)
// ============================================================================

struct CubeSpec {
    const char*  name;
    be::math::Vec3 position;
    be::math::Vec3 diffuse_tint;
    be::math::Vec3 specular;
    float         shininess;
    bool          textured;     // true = checkerboard, false = solid colour via tint
};

static const CubeSpec k_cubes[] = {
    //         name        position     diffuse             specular         shininess  textured
    {  "Red Metallic",    {-5.f, 0, 0}, {0.95f, 0.15f, 0.10f}, {1.0f, 0.9f, 0.7f}, 128.f,  false },
    {  "Blue Glossy",     {-2.5f, 0, 0},{0.10f, 0.30f, 0.95f}, {1.0f, 1.0f, 1.0f}, 64.f,   false },
    {  "Textured Cube",   {0.f, 0, 0},  {1.0f, 1.0f, 1.0f},    {1.0f, 1.0f, 1.0f}, 32.f,   true  },
    {  "Green Matte",     {2.5f, 0, 0}, {0.15f, 0.85f, 0.25f}, {0.15f, 0.15f, 0.15f}, 4.f,  false },
    {  "Pearl White",     {5.f, 0, 0},  {1.0f, 0.88f, 0.70f},  {0.9f, 0.9f, 1.0f},  8.f,   false },
};

// ============================================================================
// Manual render: replicates the logic of RenderSystem::render() but allows
// read_pixels before end_frame() for the capture frame.
// ============================================================================

static auto render_and_capture_frame(
    be::RenderDevice& device,
    be::World& world,
    bool capture_this_frame
) -> be::Result<be::ImageBuffer> {
    device.begin_frame();

    // -- Active camera --
    auto cam_opt = world.active_camera();
    if (!cam_opt.has_value()) {
        std::cerr << "phong_capture: no active camera\n";
        device.end_frame();
        return be::make_error(be::Error::Category::Unknown, "no active camera");
    }
    auto& cam_comp = *cam_opt;
    auto vp = cam_comp.camera().view_projection_matrix();
    auto camera_pos = cam_comp.camera().position();

    // -- Collect lights --
    std::array<be::detail::LightData, be::detail::k_max_lights> light_data{};
    int light_count = 0;

    // Directional lights (type 0)
    world.each<be::DirectionalLightComponent>([&](be::Entity entity, be::DirectionalLightComponent& lc) -> bool {
        if (light_count >= be::detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto forward = be::math::Vec4{0.0f, 0.0f, -1.0f, 0.0f};
        auto dir_v4 = world_mat * forward;
        be::math::Vec3 dir = {dir_v4.x, dir_v4.y, dir_v4.z};
        dir.normalize();
        auto& ld = light_data[light_count];
        ld.position_or_dir = {dir.x, dir.y, dir.z, 0.0f};
        ld.colour = {lc.colour().x * lc.intensity(),
                     lc.colour().y * lc.intensity(),
                     lc.colour().z * lc.intensity(), 1.0f};
        ld.range = 0.0f;
        ld.spot_direction = {0.0f, 0.0f, 0.0f, 0.0f};
        ld.inner_cone_cos = 1.0f;
        ld.outer_cone_cos = 1.0f;
        ++light_count;
        return true;
    });

    // Point lights (type 1)
    world.each<be::PointLightComponent>([&](be::Entity entity, be::PointLightComponent& lc) -> bool {
        if (light_count >= be::detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto pos_v4 = world_mat * be::math::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
        auto& ld = light_data[light_count];
        ld.position_or_dir = {pos_v4.x, pos_v4.y, pos_v4.z, 1.0f};
        ld.colour = {lc.colour().x * lc.intensity(),
                     lc.colour().y * lc.intensity(),
                     lc.colour().z * lc.intensity(), 1.0f};
        ld.range = lc.range();
        ld.spot_direction = {0.0f, 0.0f, 0.0f, 0.0f};
        ld.inner_cone_cos = 1.0f;
        ld.outer_cone_cos = 1.0f;
        ++light_count;
        return true;
    });

    // Spot lights (type 2)
    world.each<be::SpotLightComponent>([&](be::Entity entity, be::SpotLightComponent& lc) -> bool {
        if (light_count >= be::detail::k_max_lights) return false;
        auto world_mat = entity.world_matrix();
        auto pos_v4 = world_mat * be::math::Vec4{0.0f, 0.0f, 0.0f, 1.0f};
        auto forward = be::math::Vec4{0.0f, 0.0f, -1.0f, 0.0f};
        auto dir_v4 = world_mat * forward;
        be::math::Vec3 dir = {dir_v4.x, dir_v4.y, dir_v4.z};
        dir.normalize();
        auto& ld = light_data[light_count];
        ld.position_or_dir = {pos_v4.x, pos_v4.y, pos_v4.z, 2.0f};
        ld.colour = {lc.colour().x * lc.intensity(),
                     lc.colour().y * lc.intensity(),
                     lc.colour().z * lc.intensity(), 1.0f};
        ld.range = lc.range();
        ld.spot_direction = {dir.x, dir.y, dir.z, 0.0f};
        ld.inner_cone_cos = std::cos(lc.inner_angle());
        ld.outer_cone_cos = std::cos(lc.outer_angle());
        ++light_count;
        return true;
    });

    be::Result<be::ImageBuffer> last_buffer =
        be::make_error(be::Error::Category::Unknown, "no frame captured");

    // -- Iterate MeshRenderers --
    world.each<be::MeshRenderer>([&](be::Entity entity, be::MeshRenderer& mr) -> bool {
        auto world_mat = entity.world_matrix();
        auto mvp = vp * world_mat;
        auto& material = mr.model().material();

        auto r = material.set_uniform("u_mvp", mvp);
        if (!r) {
            return true; // skip this entity, continue iteration
        }

        if (material.has_uniform("u_model")) {
            // Ignore set_uniform return values in the render path — same pattern
            // as RenderSystem::render(). Uniforms may legitimately fail to set
            // on some materials, and the draw proceeds regardless.
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
                    "u_light_colours[" + std::to_string(i) + "]", ld.colour);
                (void)material.set_uniform(
                    "u_light_ranges[" + std::to_string(i) + "]", ld.range);
                (void)material.set_uniform(
                    "u_light_spot_directions[" + std::to_string(i) + "]", ld.spot_direction);
                (void)material.set_uniform(
                    "u_light_inner_cones[" + std::to_string(i) + "]", ld.inner_cone_cos);
                (void)material.set_uniform(
                    "u_light_outer_cones[" + std::to_string(i) + "]", ld.outer_cone_cos);
            }
        }

        mr.model().draw(device);
        return true;
    });

    // -- Capture before end_frame (back buffer still has rendered content) --
    if (capture_this_frame) {
        last_buffer = device.read_pixels();
        if (!last_buffer) {
            device.end_frame();
            return std::unexpected(last_buffer.error());
        }
    }

    device.end_frame();
    return last_buffer;
}

// ============================================================================
// Public capture function
// ============================================================================

auto buddd::cmd::capture::capture_phong_scene(
    be::Platform& platform,
    be::RenderDevice& device,
    int window_w,
    int window_h,
    int num_frames
) -> be::Result<be::ImageBuffer>
{
    std::cerr << "phong_capture: setting up scene...\n";

    // ── ECS ──
    be::World world;

    // ── Camera ──
    auto camera_entity = be::Entity::create(world);
    be::math::Camera camera;
    camera_entity.add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity.get_component<be::CameraComponent>()->camera();
    // Fixed camera at the same starting position as the interactive demo
    cam.set_position(be::math::Vec3{6.0f, 3.5f, 8.0f});
    cam.set_orientation(be::math::Quat::from_euler(be::math::radians(-18.0f),
                                                   be::math::radians(35.0f), 0.0f));
    cam.set_perspective(be::math::radians(55.0f),
                        static_cast<float>(window_w) / static_cast<float>(window_h),
                        0.1f, 100.0f);

    // ── Textures ──
    auto checkerboard_tex = make_checkerboard_texture(device);
    auto white_tex = make_solid_texture(device, 255, 255, 255);

    std::cerr << "phong_capture: created textures\n";

    // ── Create cubes ──
    for (const auto& spec : k_cubes) {
        auto mat = std::make_shared<be::PhongMaterial>(device);

        auto res_spec = mat->set_uniform("u_material_specular",
            be::math::Vec4{spec.specular.x, spec.specular.y, spec.specular.z, 1.0f});
        if (!res_spec) {
            std::cerr << "Warning: set_uniform(u_material_specular) failed: "
                      << be::to_string(res_spec.error()) << "\n";
        }

        auto res_shiny = mat->set_uniform("u_material_shininess", spec.shininess);
        if (!res_shiny) {
            std::cerr << "Warning: set_uniform(u_material_shininess) failed: "
                      << be::to_string(res_shiny.error()) << "\n";
        }

        auto res_tint = mat->set_uniform("u_material_diffuse_tint",
            be::math::Vec4{spec.diffuse_tint.x, spec.diffuse_tint.y, spec.diffuse_tint.z, 1.0f});
        if (!res_tint) {
            std::cerr << "Warning: set_uniform(u_material_diffuse_tint) failed: "
                      << be::to_string(res_tint.error()) << "\n";
        }

        auto tex = spec.textured ? checkerboard_tex : white_tex;
        auto res_tex = mat->set_texture("u_diffuse_texture", tex);
        if (!res_tex) {
            std::cerr << "Warning: set_texture failed: "
                      << be::to_string(res_tex.error()) << "\n";
        }

        auto model = create_phong_cube(device, mat);
        auto entity = be::Entity::create(world);
        entity.add_component<be::MeshRenderer>(
            std::make_shared<be::Model>(std::move(model)));
        entity.transform().position = spec.position;

        std::cerr << "phong_capture: created cube '" << spec.name << "' at ("
                  << spec.position.x << ", " << spec.position.y << ", " << spec.position.z << ")\n";
    }

    // ── Directional fill light ──
    auto fill = be::Entity::create(world);
    fill.add_component<be::DirectionalLightComponent>(
        be::math::Vec3{0.6f, 0.6f, 0.8f}, 0.35f);
    fill.transform().rotation =
        be::math::Quat::from_euler(be::math::radians(-35.0f),
                                    be::math::radians(50.0f), 0.0f);

    // ── Point light A (warm orange, may orbit if num_frames > 1) ──
    auto pointA_entity = be::Entity::create(world);
    pointA_entity.add_component<be::PointLightComponent>(
        be::math::Vec3{1.0f, 0.4f, 0.1f}, 1.8f, 12.0f);
    // Initial position at t=0 (same as interactive demo)
    pointA_entity.transform().position = be::math::Vec3{6.0f, 2.5f, 0.0f};

    // ── Point light B (cool blue, may orbit if num_frames > 1) ──
    auto pointB_entity = be::Entity::create(world);
    pointB_entity.add_component<be::PointLightComponent>(
        be::math::Vec3{0.1f, 0.3f, 1.0f}, 1.6f, 12.0f);
    // Initial position at t=0 (90° out of phase)
    pointB_entity.transform().position = be::math::Vec3{0.0f, 2.0f, 4.2f};

    // ── Point light C (static purple, above center) ──
    auto pointC_entity = be::Entity::create(world);
    pointC_entity.add_component<be::PointLightComponent>(
        be::math::Vec3{0.6f, 0.2f, 0.8f}, 0.7f, 8.0f);
    pointC_entity.transform().position = be::math::Vec3{0.0f, 3.5f, 0.0f};

    // ── Spot light (bright warm, from above aiming at origin) ──
    auto spot_entity = be::Entity::create(world);
    spot_entity.add_component<be::SpotLightComponent>(
        be::math::Vec3{1.0f, 0.95f, 0.85f}, 2.5f, 14.0f,
        be::math::radians(18.0f), be::math::radians(35.0f));
    spot_entity.transform().position = be::math::Vec3{0.0f, 6.0f, -3.0f};
    spot_entity.transform().rotation =
        be::math::Quat::from_euler(be::math::radians(30.0f), 0.0f, 0.0f);

    std::cerr << "phong_capture: scene setup complete (" << std::size(k_cubes)
              << " cubes, 5 lights)\n";

    // ── Render loop ──
    platform.poll_events();

    // Render at least 2 frames (driver quirk workaround, same as cube_capture)
    int effective_frames = (num_frames < 2) ? 2 : num_frames;
    constexpr auto frame_duration = std::chrono::milliseconds(16); // ~60 FPS

    be::Result<be::ImageBuffer> last_buffer =
        be::make_error(be::Error::Category::Unknown, "no frame captured");

    auto demo_start = std::chrono::steady_clock::now();

    for (int frame = 0; frame < effective_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        platform.poll_events();

        // Animate orbiting lights (same motion as the interactive demo),
        // so --frame N produces different light positions.
        auto now = std::chrono::steady_clock::now();
        float t = std::chrono::duration<float>(now - demo_start).count();
        float orbit_r = 6.0f;
        float orbit_y = 2.5f;

        pointA_entity.transform().position = be::math::Vec3{
            orbit_r * std::cos(t * 0.8f),
            orbit_y + 0.8f * std::sin(t * 1.2f),
            orbit_r * std::sin(t * 0.8f)
        };

        pointB_entity.transform().position = be::math::Vec3{
            orbit_r * 0.7f * std::cos(t * 0.6f + 1.57f),
            orbit_y - 0.5f + 1.2f * std::sin(t * 0.9f + 0.5f),
            orbit_r * 0.7f * std::sin(t * 0.6f + 1.57f)
        };

        bool is_last_frame = (frame == effective_frames - 1);
        auto result = render_and_capture_frame(device, world, is_last_frame);

        if (is_last_frame) {
            if (!result) {
                return result; // error already set
            }
            last_buffer = std::move(result);
        }

        // Frame rate limiting
        if (effective_frames > 1) {
            auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
            if (frame_elapsed < frame_duration) {
                std::this_thread::sleep_for(frame_duration - frame_elapsed);
            }
        }
    }

    std::cerr << "phong_capture: capture complete\n";
    return std::move(*last_buffer);
}
