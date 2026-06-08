#include "window_headless.h"

namespace buddd::engine {

WindowHeadless::WindowHeadless(int width, int height, Platform& platform)
    : Window(platform), width_(width), height_(height) {}

auto WindowHeadless::width() const noexcept -> int {
    return width_;
}

auto WindowHeadless::height() const noexcept -> int {
    return height_;
}

auto WindowHeadless::on_resize(int w, int h) -> void {
    width_  = w;
    height_ = h;
}

auto WindowHeadless::native_handle() const noexcept -> void* {
    return nullptr;
}

auto WindowHeadless::set_mouse_capture(bool /*captured*/) -> void {
    // no-op
}

auto WindowHeadless::is_mouse_captured() const noexcept -> bool {
    return false;
}

} // namespace buddd::engine
