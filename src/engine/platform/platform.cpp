#include "platform.h"
#include "platform_sdl3.h"
#include "platform_headless.h"

#include "log/log.h"

#include <SDL3/SDL.h>

BUDDD_LOG_TAG("Platform");

namespace buddd::engine {

auto Platform::create(Backend backend) -> Result<std::unique_ptr<Platform>> {
    switch (backend) {
        case Backend::SDL3: {
            if (!SDL_Init(SDL_INIT_VIDEO)) {
                BUDDD_LOG_ERROR("Platform init failed: SDL_Init failed: {}",
                    SDL_GetError());
                return make_error(Error::Category::InitFailed,
                    "SDL_Init failed: " + std::string(SDL_GetError()));
            }
            BUDDD_LOG_INFO("Platform backend: SDL3");
            BUDDD_LOG_INFO("Platform initialized");
            return std::unique_ptr<Platform>(new PlatformSDL3());
        }
        case Backend::Headless: {
            BUDDD_LOG_INFO("Platform backend: Headless");
            BUDDD_LOG_INFO("Platform initialized");
            return std::unique_ptr<Platform>(new PlatformHeadless());
        }
    }
    return make_error(Error::Category::Unsupported, "Unknown backend");
}

} // namespace buddd::engine
