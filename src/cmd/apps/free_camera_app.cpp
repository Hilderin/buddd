#include "apps/free_camera_app.h"

#include "input/input_system.h"
#include "math/camera.h"
#include "math/math.h"
#include "math/vec3.h"
#include "math/quat.h"
#include "platform/platform.h"
#include "render/primitives.h"
#include "render/render_device.h"
#include "render/render_system.h"
#include "render/mesh_renderer.h"
#include "render/shader.h"
#include "scene/world.h"
#include "scene/camera_component.h"
#include "scene/entity.h"
#include "window/window.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

namespace be = buddd::engine;

auto buddd::cmd::app::FreeCameraApp::setup(be::RenderDevice& device)
    -> be::Result<void>
{
    world_ = std::make_unique<be::World>();

    // Camera entity
    camera_entity_ = be::Entity::create(*world_);
    be::math::Camera camera;
    camera_entity_.add_component<be::CameraComponent>(camera);

    auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();
    cam.set_position(be::math::Vec3{0.0f, 2.0f, 5.0f});
    cam.set_orientation(be::math::Quat::from_euler(0.0f, 0.0f, 0.0f));
    cam.set_perspective(be::math::radians(60.0f),
                        static_cast<float>(config().width) / static_cast<float>(config().height),
                        0.1f, 100.0f);

    // --- Create cube material ---
    constexpr std::string_view k_vs = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_color;
        out vec3 v_color;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";

    constexpr std::string_view k_fs = R"(
        #version 450 core
        in vec3 v_color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(v_color, 1.0);
        }
    )";

    auto vs = device.create_shader(be::ShaderType::Vertex, k_vs);
    if (!vs) return std::unexpected(vs.error());
    auto fs = device.create_shader(be::ShaderType::Fragment, k_fs);
    if (!fs) return std::unexpected(fs.error());
    auto mat = device.create_material(std::move(*vs), std::move(*fs), {"u_mvp"});
    if (!mat) return std::unexpected(mat.error());
    auto shared_mat = std::shared_ptr<be::Material>(std::move(*mat));

    // --- Create cube via primitive helper ---
    auto cube_result = be::create_cube(device, shared_mat);
    if (!cube_result) return std::unexpected(cube_result.error());
    auto cube_entity = be::Entity::create(*world_);
    cube_entity.add_component<be::MeshRenderer>(
        std::make_shared<be::Model>(std::move(*cube_result)));
    cube_entity_ = std::make_unique<be::Entity>(std::move(cube_entity));

    // RenderSystem
    render_system_ = std::make_unique<be::RenderSystem>(device, *world_);

    return {};
}

auto buddd::cmd::app::FreeCameraApp::render(be::RenderDevice& device, int) -> void {
    auto& input = device.window().platform().input_system();
    auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();

    float dt = device.window().platform().delta_time();

    // ── Mouse capture (right-click toggle) ──
    bool curr_right_click = input.is_mouse_down(be::MouseButton::Right);
    if (curr_right_click && !prev_right_click_) {
        device.window().set_mouse_capture(true);
    }
    if (!curr_right_click && prev_right_click_) {
        device.window().set_mouse_capture(false);
    }
    prev_right_click_ = curr_right_click;

    // ── ESC to exit ──
    if (input.is_down(be::KeyCode::Escape)) {
        running_ = false;
        return;
    }

    bool mouse_captured = device.window().is_mouse_captured();

    // ── Mouse look ──
    if (mouse_captured) {
        auto [dx, dy] = input.mouse_delta();
        constexpr float k_mouse_sensitivity = 0.002f;
        yaw_ -= dx * k_mouse_sensitivity;
        pitch_ += -dy * k_mouse_sensitivity;
        constexpr float k_pitch_clamp = 89.0f;
        pitch_ = std::clamp(pitch_, be::math::radians(-k_pitch_clamp),
                            be::math::radians(k_pitch_clamp));
        cam.set_orientation(be::math::Quat::from_euler(pitch_, yaw_, 0.0f));
    }

    // ── Keyboard movement ──
    if (mouse_captured) {
        constexpr float k_move_speed = 5.0f;
        be::math::Vec3 forward = cam.orientation() * be::math::Vec3{0.0f, 0.0f, -1.0f};
        forward.y = 0.0f;
        if (forward.length_squared() > be::math::epsilon) {
            forward.normalize();
        }

        be::math::Vec3 right = cam.orientation() * be::math::Vec3{1.0f, 0.0f, 0.0f};
        be::math::Vec3 movement{0.0f, 0.0f, 0.0f};

        if (input.is_down(be::KeyCode::W))          { movement += forward; }
        if (input.is_down(be::KeyCode::S))          { movement -= forward; }
        if (input.is_down(be::KeyCode::D))          { movement += right; }
        if (input.is_down(be::KeyCode::A))          { movement -= right; }
        if (input.is_down(be::KeyCode::Space))      { movement += be::math::Vec3::unit_y(); }
        if (input.is_down(be::KeyCode::ControlLeft))  { movement -= be::math::Vec3::unit_y(); }
        if (input.is_down(be::KeyCode::ControlRight)) { movement -= be::math::Vec3::unit_y(); }

        cam.set_position(cam.position() + movement * k_move_speed * dt);
    }

    // ── Render ──
    render_system_->render_scene();
}
