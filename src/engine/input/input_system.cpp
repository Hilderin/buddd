#include "input_system.h"
#include "input_system_sdl3.h"
#include "input_system_headless.h"

#include "log/log.h"
#include "platform/platform.h"   // for Backend enum definition

BUDDD_LOG_TAG("Input");

namespace buddd::engine {

auto InputSystem::create(Backend backend) -> Result<std::unique_ptr<InputSystem>> {
    switch (backend) {
        case Backend::SDL3: {
            BUDDD_LOG_INFO("InputSystem backend: SDL3");
            return std::unique_ptr<InputSystem>(new InputSystemSDL3());
        }
        case Backend::Headless: {
            BUDDD_LOG_INFO("InputSystem backend: Headless");
            return std::unique_ptr<InputSystem>(new InputSystemHeadless());
        }
    }
    return make_error(Error::Category::InputInitFailed, "Unknown backend");
}

} // namespace buddd::engine
