#include "window_sdl3.h"
#include "platform/platform_sdl3.h"

namespace buddd::engine {

WindowSDL3::WindowSDL3(SDL_Window* window, int width, int height, Platform& platform)
    : Window(platform), window_(window), width_(width), height_(height) {}

WindowSDL3::~WindowSDL3() {
    SDL_WindowID id = SDL_GetWindowID(window_);
    if (id != 0) {
        // Safe static_cast: WindowSDL3 is only ever created by PlatformSDL3::create_window()
        static_cast<PlatformSDL3&>(platform_).unregister_window(id);
    }
    SDL_DestroyWindow(window_);
}

auto WindowSDL3::width() const noexcept -> int {
    return width_;
}

auto WindowSDL3::height() const noexcept -> int {
    return height_;
}

auto WindowSDL3::on_resize(int w, int h) -> void {
    width_  = w;
    height_ = h;
}

auto WindowSDL3::native_handle() const noexcept -> void* {
    return static_cast<void*>(window_);
}

auto WindowSDL3::set_title(std::string title) -> void {
    SDL_SetWindowTitle(window_, title.c_str());
}

auto WindowSDL3::set_mouse_capture(bool captured) -> void {
    SDL_SetWindowRelativeMouseMode(window_, captured);
    captured_ = captured;
    // NOTE: cached `captured_` may desync from actual SDL relative mouse mode
    // on window focus loss (EC-012). SDL auto-releases relative mode on focus
    // loss, but `captured_` stays `true`. The demo recovers when user releases
    // and re-presses right-click. A future fix could listen for
    // SDL_EVENT_WINDOW_FOCUS_LOST to reset `captured_`.
}

auto WindowSDL3::is_mouse_captured() const noexcept -> bool {
    return captured_;
}

auto WindowSDL3::position() const noexcept -> WindowPosition {
    int x, y;
    SDL_GetWindowPosition(window_, &x, &y);
    return {x, y};
}

auto WindowSDL3::set_position(WindowPosition pos) -> void {
    SDL_SetWindowPosition(window_, pos.x, pos.y);
}

auto WindowSDL3::state() const noexcept -> WindowState {
    SDL_WindowFlags flags = SDL_GetWindowFlags(window_);
    if (flags & SDL_WINDOW_MAXIMIZED) return WindowState::Maximized;
    if (flags & SDL_WINDOW_MINIMIZED) return WindowState::Minimized;
    return WindowState::Normal;
}

auto WindowSDL3::set_state(WindowState state) -> void {
    switch (state) {
        case WindowState::Normal:    SDL_RestoreWindow(window_);   break;
        case WindowState::Maximized: SDL_MaximizeWindow(window_);  break;
        case WindowState::Minimized: SDL_MinimizeWindow(window_);  break;
    }
}

auto WindowSDL3::resize(int width, int height) -> void {
    SDL_SetWindowSize(window_, width, height);
    width_  = width;
    height_ = height;
}

} // namespace buddd::engine
