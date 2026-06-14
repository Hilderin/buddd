#pragma once

#include "input_system.h"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>

namespace buddd::engine {

class InputSystemSDL3 final : public InputSystem {
public:
    ~InputSystemSDL3() override = default;

    /// Store the SDL_Window pointer for use by set_mouse_position().
    /// Called by PlatformSDL3::create_window() after window creation.
    void set_sdl_window(SDL_Window* window);

    // ── InputSystem overrides ──
    auto begin_frame() -> void override;

    [[nodiscard]] auto is_down(KeyCode key) const noexcept -> bool override;
    [[nodiscard]] auto is_pressed(KeyCode key) const noexcept -> bool override;
    [[nodiscard]] auto is_released(KeyCode key) const noexcept -> bool override;

    [[nodiscard]] auto mouse_position() const noexcept -> std::pair<float, float> override;
    [[nodiscard]] auto mouse_delta() const noexcept -> std::pair<float, float> override;
    [[nodiscard]] auto mouse_wheel() const noexcept -> std::pair<float, float> override;

    [[nodiscard]] auto is_mouse_down(MouseButton button) const noexcept -> bool override;
    [[nodiscard]] auto is_mouse_pressed(MouseButton button) const noexcept -> bool override;
    [[nodiscard]] auto is_mouse_released(MouseButton button) const noexcept -> bool override;

    auto set_mouse_position(int x, int y) -> void override;

    InputSystemSDL3(const InputSystemSDL3&) = delete;
    auto operator=(const InputSystemSDL3&) -> InputSystemSDL3& = delete;
    InputSystemSDL3(InputSystemSDL3&&) = delete;
    auto operator=(InputSystemSDL3&&) -> InputSystemSDL3& = delete;

private:
    friend class PlatformSDL3;
    friend auto InputSystem::create(Backend) -> Result<std::unique_ptr<InputSystem>>;
    InputSystemSDL3() = default;

    /// Called by PlatformSDL3::poll_events() for each non-quit SDL event.
    /// Processes keyboard, mouse-motion, mouse-button, and mouse-wheel events.
    /// Not part of the abstract InputSystem interface.
    void on_sdl_event(const SDL_Event& event);

    SDL_Window* sdl_window_{nullptr};

    // ── State (double-buffered) ──
    static constexpr size_t kKeyCount = static_cast<size_t>(KeyCode::_Count);
    static constexpr size_t kMouseButtonCount = 5;

    std::array<bool, kKeyCount> current_keys_{};
    std::array<bool, kKeyCount> previous_keys_{};

    std::array<bool, kMouseButtonCount> current_mouse_buttons_{};
    std::array<bool, kMouseButtonCount> previous_mouse_buttons_{};

    float mouse_x_{0.0f};
    float mouse_y_{0.0f};
    float delta_x_{0.0f};
    float delta_y_{0.0f};
    float wheel_x_{0.0f};
    float wheel_y_{0.0f};
};

} // namespace buddd::engine
