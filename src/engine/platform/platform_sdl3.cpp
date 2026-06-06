#include "platform_sdl3.h"
#include "window/window_sdl3.h"

#include "log/log.h"

#include <SDL3/SDL.h>

BUDDD_LOG_TAG("Platform:SDL3");

namespace buddd::engine {

PlatformSDL3::~PlatformSDL3() {
    SDL_Quit();
    BUDDD_LOG_INFO("Platform shutdown (SDL3)");
}

auto PlatformSDL3::poll_events() -> bool {
    // Compute delta time from SDL_GetTicks
    Uint64 now = SDL_GetTicks();
    if (last_frame_ticks_ != 0) {
        delta_time_ = static_cast<float>(now - last_frame_ticks_) / 1000.0f;
    } else {
        delta_time_ = 1.0f / 60.0f;
    }
    last_frame_ticks_ = now;

    // 1. Begin the input frame (copies current→previous, resets delta/wheel)
    input_system_.begin_frame();

    // 2. Process all pending SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
        // Route non-quit events to the input system
        input_system_.on_sdl_event(event);
    }
    return true;
}

auto PlatformSDL3::input_system() -> InputSystem& {
    return input_system_;
}

auto PlatformSDL3::delta_time() const noexcept -> float {
    return delta_time_;
}

auto PlatformSDL3::create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> {
    if (config.width <= 0 || config.height <= 0) {
        return make_error(Error::Category::WindowCreationFailed, "Invalid window dimensions");
    }

    auto* sdl_window = SDL_CreateWindow(
        config.title.c_str(),
        config.width,
        config.height,
        SDL_WINDOW_OPENGL
    );

    if (sdl_window == nullptr) {
        return make_error(Error::Category::WindowCreationFailed,
            "SDL_CreateWindow failed: " + std::string(SDL_GetError()));
    }

    BUDDD_LOG_INFO("Window created: {}x{}", config.width, config.height);
    return std::unique_ptr<Window>(new WindowSDL3(sdl_window, config.width, config.height, *this));
}

} // namespace buddd::engine
