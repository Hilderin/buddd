#pragma once

#include "input_system.h"

namespace buddd::engine {

class InputSystemHeadless final : public InputSystem {
public:
    ~InputSystemHeadless() override = default;

    auto begin_frame() -> void override;

    [[nodiscard]] auto is_down(KeyCode) const noexcept -> bool override;
    [[nodiscard]] auto is_pressed(KeyCode) const noexcept -> bool override;
    [[nodiscard]] auto is_released(KeyCode) const noexcept -> bool override;

    [[nodiscard]] auto mouse_position() const noexcept -> std::pair<float, float> override;
    [[nodiscard]] auto mouse_delta() const noexcept -> std::pair<float, float> override;
    [[nodiscard]] auto mouse_wheel() const noexcept -> std::pair<float, float> override;

    [[nodiscard]] auto is_mouse_down(MouseButton) const noexcept -> bool override;
    [[nodiscard]] auto is_mouse_pressed(MouseButton) const noexcept -> bool override;
    [[nodiscard]] auto is_mouse_released(MouseButton) const noexcept -> bool override;

    auto set_mouse_position(int x, int y) -> void override;

    InputSystemHeadless(const InputSystemHeadless&) = delete;
    auto operator=(const InputSystemHeadless&) -> InputSystemHeadless& = delete;
    InputSystemHeadless(InputSystemHeadless&&) = delete;
    auto operator=(InputSystemHeadless&&) -> InputSystemHeadless& = delete;

private:
    friend class PlatformHeadless;
    friend auto InputSystem::create(Backend) -> Result<std::unique_ptr<InputSystem>>;
    InputSystemHeadless() = default;
};

} // namespace buddd::engine
