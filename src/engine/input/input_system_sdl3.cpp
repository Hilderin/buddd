#include "input_system_sdl3.h"

#include "log/log.h"

BUDDD_LOG_TAG("Input:SDL3");

namespace buddd::engine {

namespace {

auto sdl_button_to_mouse_button(uint8_t sdl_button) -> MouseButton {
    switch (sdl_button) {
        case SDL_BUTTON_LEFT:   return MouseButton::Left;
        case SDL_BUTTON_RIGHT:  return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_X1:     return MouseButton::X1;
        case SDL_BUTTON_X2:     return MouseButton::X2;
        default:                return MouseButton::Left;  // Default to Left for unknown
    }
}

} // anonymous namespace

// ── Frame lifecycle ──

auto InputSystemSDL3::begin_frame() -> void {
    // Copy current→previous for all keys
    previous_keys_ = current_keys_;

    // Copy current→previous for all mouse buttons
    previous_mouse_buttons_ = current_mouse_buttons_;

    // Reset accumulated frame values
    delta_x_ = 0.0f;
    delta_y_ = 0.0f;
    wheel_x_ = 0.0f;
    wheel_y_ = 0.0f;

    // NOTE: mouse position (mouse_x_, mouse_y_) is NOT reset — it persists.
    // NOTE: current key/button state is NOT cleared — held keys stay held.
}

// ── Keyboard state ──

auto InputSystemSDL3::is_down(KeyCode key) const noexcept -> bool {
    auto idx = static_cast<size_t>(key);
    if (idx >= kKeyCount) return false;
    return current_keys_[idx];
}

auto InputSystemSDL3::is_pressed(KeyCode key) const noexcept -> bool {
    auto idx = static_cast<size_t>(key);
    if (idx >= kKeyCount) return false;
    return current_keys_[idx] && !previous_keys_[idx];
}

auto InputSystemSDL3::is_released(KeyCode key) const noexcept -> bool {
    auto idx = static_cast<size_t>(key);
    if (idx >= kKeyCount) return false;
    return !current_keys_[idx] && previous_keys_[idx];
}

// ── Mouse state ──

auto InputSystemSDL3::mouse_position() const noexcept -> std::pair<float, float> {
    return {mouse_x_, mouse_y_};
}

auto InputSystemSDL3::mouse_delta() const noexcept -> std::pair<float, float> {
    return {delta_x_, delta_y_};
}

auto InputSystemSDL3::mouse_wheel() const noexcept -> std::pair<float, float> {
    return {wheel_x_, wheel_y_};
}

auto InputSystemSDL3::is_mouse_down(MouseButton button) const noexcept -> bool {
    auto idx = static_cast<size_t>(button);
    if (idx >= kMouseButtonCount) return false;
    return current_mouse_buttons_[idx];
}

auto InputSystemSDL3::is_mouse_pressed(MouseButton button) const noexcept -> bool {
    auto idx = static_cast<size_t>(button);
    if (idx >= kMouseButtonCount) return false;
    return current_mouse_buttons_[idx] && !previous_mouse_buttons_[idx];
}

auto InputSystemSDL3::is_mouse_released(MouseButton button) const noexcept -> bool {
    auto idx = static_cast<size_t>(button);
    if (idx >= kMouseButtonCount) return false;
    return !current_mouse_buttons_[idx] && previous_mouse_buttons_[idx];
}

// ── SDL event processing ──

void InputSystemSDL3::on_sdl_event(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN: {
            auto scancode = event.key.scancode;
            if (scancode > SDL_SCANCODE_UNKNOWN && scancode < static_cast<SDL_Scancode>(KeyCode::_Count)) {
                auto idx = static_cast<size_t>(scancode);
                current_keys_[idx] = true;
            }
            else {
                BUDDD_LOG_DEBUG("InputSystemSDL3: unrecognised scancode {}", static_cast<int>(scancode));
            }
            break;
        }
        case SDL_EVENT_KEY_UP: {
            auto scancode = event.key.scancode;
            if (scancode > SDL_SCANCODE_UNKNOWN && scancode < static_cast<SDL_Scancode>(KeyCode::_Count)) {
                auto idx = static_cast<size_t>(scancode);
                current_keys_[idx] = false;
            }
            else {
                BUDDD_LOG_DEBUG("InputSystemSDL3: unrecognised scancode {}", static_cast<int>(scancode));
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            mouse_x_ = static_cast<float>(event.motion.x);
            mouse_y_ = static_cast<float>(event.motion.y);
            delta_x_ += event.motion.xrel;
            delta_y_ += event.motion.yrel;
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            auto button = sdl_button_to_mouse_button(event.button.button);
            auto idx = static_cast<size_t>(button);
            if (idx < kMouseButtonCount) {
                current_mouse_buttons_[idx] = true;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            auto button = sdl_button_to_mouse_button(event.button.button);
            auto idx = static_cast<size_t>(button);
            if (idx < kMouseButtonCount) {
                current_mouse_buttons_[idx] = false;
            }
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL: {
            wheel_x_ += event.wheel.x;
            wheel_y_ += event.wheel.y;
            break;
        }
        default:
            break;  // Ignore unhandled event types
    }
}

} // namespace buddd::engine
