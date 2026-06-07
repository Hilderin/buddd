#include "scene/free_camera_movement.h"
#include "engine_context.h"
#include "engine_service.h"
#include "log/log.h"
#include "math/math.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "scene/camera_component.h"
#include "scene/entity.h"
#include "input/input_system.h"
#include "window/window.h"
#include "platform/platform.h"

#include <algorithm> // std::clamp

BUDDD_LOG_TAG("Scene:FreeCamera");

namespace buddd::engine {

FreeCameraMovement::FreeCameraMovement(float initial_yaw, float initial_pitch)
    : yaw_(initial_yaw), pitch_(initial_pitch)
{
}

void FreeCameraMovement::update(const EngineContext& ctx) {
    auto& input = ctx.services.platform().input_system();
    auto& window = ctx.window;
    float dt = ctx.delta_time;

    // ── ESC to exit (checked first, before mouse capture) ──
    if (input.is_down(KeyCode::Escape)) {
        ctx.request_exit();
        return;
    }

    // ── Get CameraComponent on the same entity ──
    auto cam_opt = entity().get_component<CameraComponent>();
    if (!cam_opt) {
        if (!missing_camera_warned_) {
            BUDDD_LOG_WARN("FreeCameraMovement: entity has no CameraComponent — movement is a no-op");
            missing_camera_warned_ = true;
        }
        return; // no-op, don't crash
    }
    // ── Mouse capture (right-click toggle, edge-detected) ──
    bool curr_right_click = input.is_mouse_down(MouseButton::Right);
    if (curr_right_click && !prev_right_click_) {
        window.set_mouse_capture(true);
    }
    if (!curr_right_click && prev_right_click_) {
        window.set_mouse_capture(false);
    }
    prev_right_click_ = curr_right_click;

    bool mouse_captured = window.is_mouse_captured();
    if (!mouse_captured) {
        return; // no movement when mouse not captured
    }

    // ── Mouse look ──
    auto [dx, dy] = input.mouse_delta();
    float yaw_sign = invert_yaw ? 1.0f : -1.0f;
    float pitch_sign = invert_pitch ? -1.0f : 1.0f;
    yaw_ += dx * mouse_sensitivity * yaw_sign;
    pitch_ += -dy * mouse_sensitivity * pitch_sign;

    float pitch_clamp_rad = math::radians(pitch_clamp_degrees);
    pitch_ = std::clamp(pitch_, -pitch_clamp_rad, pitch_clamp_rad);

    entity().transform().rotation = math::Quat::from_euler(pitch_, yaw_, 0.0f);

    // ── Keyboard movement ──
    constexpr float k_epsilon = 1.0e-6f;
    math::Vec3 forward = entity().transform().rotation * math::Vec3{0.0f, 0.0f, -1.0f};
    forward.y = 0.0f;
    if (forward.length_squared() > k_epsilon) {
        forward.normalize();
    }

    math::Vec3 right = entity().transform().rotation * math::Vec3{1.0f, 0.0f, 0.0f};
    math::Vec3 movement{0.0f, 0.0f, 0.0f};

    if (input.is_down(KeyCode::W))           { movement += forward; }
    if (input.is_down(KeyCode::S))           { movement -= forward; }
    if (input.is_down(KeyCode::D))           { movement += right; }
    if (input.is_down(KeyCode::A))           { movement -= right; }
    if (input.is_down(KeyCode::E))           { movement += math::Vec3::unit_y(); }
    if (input.is_down(KeyCode::Q))           { movement -= math::Vec3::unit_y(); }

    entity().transform().position = entity().transform().position + movement * move_speed * dt;
}

} // namespace buddd::engine
