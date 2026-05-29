#pragma once

#include "window.h"

#include <SDL3/SDL.h>

namespace buddd::engine {

class WindowSDL3 final : public Window {
public:
    WindowSDL3(SDL_Window* window, int width, int height);
    ~WindowSDL3() override;

    auto width() const noexcept -> int override;
    auto height() const noexcept -> int override;
    auto native_handle() const noexcept -> void* override;

    WindowSDL3(const WindowSDL3&) = delete;
    auto operator=(const WindowSDL3&) -> WindowSDL3& = delete;
    WindowSDL3(WindowSDL3&&) = delete;
    auto operator=(WindowSDL3&&) -> WindowSDL3& = delete;

private:
    SDL_Window* window_;
    int width_;
    int height_;
};

} // namespace buddd::engine
