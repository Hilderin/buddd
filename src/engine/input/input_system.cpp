#include "input_system.h"
#include "input_system_sdl3.h"
#include "input_system_headless.h"

#include "platform/platform.h"   // for Backend enum definition

#include <iostream>

namespace buddd::engine {

auto InputSystem::create(Backend backend) -> Result<std::unique_ptr<InputSystem>> {
    switch (backend) {
        case Backend::SDL3: {
            std::cerr << "InputSystem backend: SDL3\n";
            return std::unique_ptr<InputSystem>(new InputSystemSDL3());
        }
        case Backend::Headless: {
            std::cerr << "InputSystem backend: Headless\n";
            return std::unique_ptr<InputSystem>(new InputSystemHeadless());
        }
    }
    return make_error(Error::Category::InputInitFailed, "Unknown backend");
}

} // namespace buddd::engine
