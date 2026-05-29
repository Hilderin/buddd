#include "window_headless.h"

namespace buddd::engine {

WindowHeadless::WindowHeadless(int width, int height)
    : width_(width), height_(height) {}

auto WindowHeadless::width() const noexcept -> int {
    return width_;
}

auto WindowHeadless::height() const noexcept -> int {
    return height_;
}

auto WindowHeadless::native_handle() const noexcept -> void* {
    return nullptr;
}

} // namespace buddd::engine
