#include "platform.h"
#include "platform_sdl3.h"
#include "platform_headless.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace buddd::engine {

auto Platform::create(Backend backend) -> Result<std::unique_ptr<Platform>> {
    switch (backend) {
        case Backend::SDL3: {
            if (!SDL_Init(SDL_INIT_VIDEO)) {
                std::cerr << "Platform init failed: SDL_Init failed: "
                          << SDL_GetError() << "\n";
                return make_error(Error::Category::InitFailed,
                    "SDL_Init failed: " + std::string(SDL_GetError()));
            }
            std::cerr << "Platform backend: SDL3\n";
            std::cerr << "Platform initialized\n";
            return std::unique_ptr<Platform>(new PlatformSDL3());
        }
        case Backend::Headless: {
            std::cerr << "Platform backend: Headless\n";
            std::cerr << "Platform initialized\n";
            return std::unique_ptr<Platform>(new PlatformHeadless());
        }
    }
    return make_error(Error::Category::Unsupported, "Unknown backend");
}

} // namespace buddd::engine
