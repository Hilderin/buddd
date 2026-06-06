#pragma once

#include "input/key_code.h"
#include "scene/component.h"
#include "scene/updatable.h"

#include <cstdint>

namespace buddd::engine {

class FreeCameraMovement : public Component, public Updatable {
public:
    /// @param initial_yaw  Initial yaw angle in radians (default 0).
    /// @param initial_pitch Initial pitch angle in radians (default 0).
    explicit FreeCameraMovement(float initial_yaw = 0.0f, float initial_pitch = 0.0f);

    // -- Configurable parameters (public per AC-005) --
    float move_speed = 5.0f;
    float mouse_sensitivity = 0.002f;
    float pitch_clamp_degrees = 89.0f;
    bool invert_yaw = false;
    bool invert_pitch = false;

    /// Called once per frame via Updatable dispatch.
    /// Uses ctx.services.platform().input_system() for input,
    /// ctx.window for window operations, and ctx.delta_time for frame timing.
    /// Calls ctx.request_exit() when ESC is pressed.
    /// @param ctx  Engine context for the current frame.
    auto update(const EngineContext& ctx) -> void override;

private:
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    bool prev_right_click_ = false;
    bool missing_camera_warned_ = false; // one-shot warning flag
};

} // namespace buddd::engine
