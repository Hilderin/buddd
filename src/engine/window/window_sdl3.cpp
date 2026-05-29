#include "window_sdl3.h"

namespace buddd::engine {

WindowSDL3::WindowSDL3(SDL_Window* window, int width, int height)
    : window_(window), width_(width), height_(height) {}

WindowSDL3::~WindowSDL3() {
    SDL_DestroyWindow(window_);
}

auto WindowSDL3::width() const noexcept -> int {
    return width_;
}

auto WindowSDL3::height() const noexcept -> int {
    return height_;
}

auto WindowSDL3::native_handle() const noexcept -> void* {
    return static_cast<void*>(window_);
}

} // namespace buddd::engine
