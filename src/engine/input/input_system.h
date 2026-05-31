#pragma once

#include "error.h"
#include "input/key_code.h"

#include <memory>
#include <utility>

namespace buddd::engine {

enum class Backend;  // Forward-declared from platform/platform.h

enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    X1,   // Forward / thumb button 1
    X2    // Back / thumb button 2
};

class InputSystem {
public:
    /// Creates an InputSystem with the given backend.
    /// Returns InputInitFailed for unknown backends (forward-compat).
    [[nodiscard]] static auto create(Backend backend) -> Result<std::unique_ptr<InputSystem>>;

    virtual ~InputSystem() = default;

    // ── Frame lifecycle ──
    /// Must be called once per frame before any queries.
    /// Copies current state to previous (for pressed/released detection),
    /// and resets accumulated frame values (mouse delta, wheel).
    /// Called automatically by the owning Platform's poll_events().
    virtual auto begin_frame() -> void = 0;

    // ── Keyboard state ──
    [[nodiscard]] virtual auto is_down(KeyCode key) const noexcept -> bool = 0;
    [[nodiscard]] virtual auto is_pressed(KeyCode key) const noexcept -> bool = 0;
    [[nodiscard]] virtual auto is_released(KeyCode key) const noexcept -> bool = 0;

    // ── Mouse state ──
    /// Absolute mouse position in window coordinates (origin top-left).
    [[nodiscard]] virtual auto mouse_position() const noexcept -> std::pair<float, float> = 0;
    /// Relative mouse motion accumulated since the last begin_frame() call.
    [[nodiscard]] virtual auto mouse_delta() const noexcept -> std::pair<float, float> = 0;
    /// Mouse wheel scroll accumulated since the last begin_frame() call.
    /// Positive Y = scroll up / away from user (natural scrolling).
    [[nodiscard]] virtual auto mouse_wheel() const noexcept -> std::pair<float, float> = 0;

    [[nodiscard]] virtual auto is_mouse_down(MouseButton button) const noexcept -> bool = 0;
    [[nodiscard]] virtual auto is_mouse_pressed(MouseButton button) const noexcept -> bool = 0;
    [[nodiscard]] virtual auto is_mouse_released(MouseButton button) const noexcept -> bool = 0;

    InputSystem(const InputSystem&) = delete;
    auto operator=(const InputSystem&) -> InputSystem& = delete;
    InputSystem(InputSystem&&) = delete;
    auto operator=(InputSystem&&) -> InputSystem& = delete;

protected:
    InputSystem() = default;
};

} // namespace buddd::engine
