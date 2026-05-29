#include "platform_sdl3.h"
#include "window/window_sdl3.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace buddd::engine {

PlatformSDL3::~PlatformSDL3() {
    SDL_Quit();
    std::cerr << "Platform shutdown (SDL3)\n";
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

    std::cerr << "Window created: " << config.width << "x" << config.height << "\n";
    return std::unique_ptr<Window>(new WindowSDL3(sdl_window, config.width, config.height));
}

} // namespace buddd::engine
