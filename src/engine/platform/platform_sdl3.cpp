#include "platform_sdl3.h"
#include "window/window_sdl3.h"

#include "imgui/engine_imgui.h"

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
        // Handle window resize / maximize / restore events
        if (event.type == SDL_EVENT_WINDOW_RESIZED
         || event.type == SDL_EVENT_WINDOW_MAXIMIZED
         || event.type == SDL_EVENT_WINDOW_RESTORED)
        {
            SDL_WindowID window_id = event.window.windowID;
            auto it = window_map_.find(window_id);
            if (it != window_map_.end()) {
                Window* win = it->second;
                if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                    int w = event.window.data1;
                    int h = event.window.data2;
                    BUDDD_LOG_DEBUG("Window resize: {}x{} (windowID={})", w, h, +window_id);
                    win->on_resize(w, h);
                } else {
                    // MAXIMIZED or RESTORED: query current size from SDL
                    SDL_Window* sdl_win = static_cast<SDL_Window*>(win->native_handle());
                    int w, h;
                    SDL_GetWindowSize(sdl_win, &w, &h);
                    BUDDD_LOG_DEBUG("Window resize (maximize/restore): {}x{} (windowID={})", w, h, +window_id);
                    win->on_resize(w, h);
                }
            }
        }

        // Route non-quit events to the input system
        input_system_.on_sdl_event(event);
        (void)engine_imgui::on_sdl_event(event);
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
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (sdl_window == nullptr) {
        return make_error(Error::Category::WindowCreationFailed,
            "SDL_CreateWindow failed: " + std::string(SDL_GetError()));
    }

    if (!SDL_SetWindowMinimumSize(sdl_window, 320, 240)) {
        BUDDD_LOG_WARN("SDL_SetWindowMinimumSize failed: {}", SDL_GetError());
    }

    auto win = std::unique_ptr<Window>(new WindowSDL3(sdl_window, config.width, config.height, *this));
    SDL_WindowID win_id = SDL_GetWindowID(sdl_window);
    register_window(win_id, win.get());
    BUDDD_LOG_INFO("Window created (resizable): {}x{} (windowID={})", config.width, config.height, +win_id);
    return win;
}

auto PlatformSDL3::register_window(SDL_WindowID id, Window* window) -> void {
    window_map_[id] = window;
}

auto PlatformSDL3::unregister_window(SDL_WindowID id) -> void {
    window_map_.erase(id);
}

} // namespace buddd::engine
