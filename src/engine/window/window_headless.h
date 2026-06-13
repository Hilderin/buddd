#pragma once

#include "window.h"

namespace buddd::engine {

class WindowHeadless final : public Window {
public:
    WindowHeadless(int width, int height, Platform& platform);
    ~WindowHeadless() override = default;

    auto width() const noexcept -> int override;
    auto height() const noexcept -> int override;
    auto native_handle() const noexcept -> void* override;

    auto on_resize(int w, int h) -> void override;

    auto set_title(std::string title) -> void override;

    auto set_mouse_capture(bool captured) -> void override;
    auto is_mouse_captured() const noexcept -> bool override;

    [[nodiscard]] auto position() const noexcept -> WindowPosition override;
    auto set_position(WindowPosition pos) -> void override;
    [[nodiscard]] auto state() const noexcept -> WindowState override;
    auto set_state(WindowState state) -> void override;
    auto resize(int width, int height) -> void override;

    WindowHeadless(const WindowHeadless&) = delete;
    auto operator=(const WindowHeadless&) -> WindowHeadless& = delete;
    WindowHeadless(WindowHeadless&&) = delete;
    auto operator=(WindowHeadless&&) -> WindowHeadless& = delete;

private:
    int width_;
    int height_;
};

} // namespace buddd::engine
