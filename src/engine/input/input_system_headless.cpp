#include "input_system_headless.h"

namespace buddd::engine {

auto InputSystemHeadless::begin_frame() -> void {
    // No-op: no state to update in headless mode.
}

auto InputSystemHeadless::is_down(KeyCode) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::is_pressed(KeyCode) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::is_released(KeyCode) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::mouse_position() const noexcept -> std::pair<float, float> {
    return {0.0f, 0.0f};
}

auto InputSystemHeadless::mouse_delta() const noexcept -> std::pair<float, float> {
    return {0.0f, 0.0f};
}

auto InputSystemHeadless::mouse_wheel() const noexcept -> std::pair<float, float> {
    return {0.0f, 0.0f};
}

auto InputSystemHeadless::is_mouse_down(MouseButton) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::is_mouse_pressed(MouseButton) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::is_mouse_released(MouseButton) const noexcept -> bool {
    return false;
}

} // namespace buddd::engine
