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

    auto set_mouse_capture(bool captured) -> void override;
    auto is_mouse_captured() const noexcept -> bool override;

    WindowHeadless(const WindowHeadless&) = delete;
    auto operator=(const WindowHeadless&) -> WindowHeadless& = delete;
    WindowHeadless(WindowHeadless&&) = delete;
    auto operator=(WindowHeadless&&) -> WindowHeadless& = delete;

private:
    int width_;
    int height_;
};

} // namespace buddd::engine
