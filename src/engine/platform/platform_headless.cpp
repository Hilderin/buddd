#include "platform_headless.h"
#include "window/window_headless.h"

#include <iostream>

namespace buddd::engine {

auto PlatformHeadless::poll_events() -> bool {
    input_system_.begin_frame();   // Begin the input frame
    return true;                   // Headless: never quits.
}

auto PlatformHeadless::input_system() -> InputSystem& {
    return input_system_;
}

auto PlatformHeadless::delta_time() const noexcept -> float {
    return 1.0f / 60.0f;
}

auto PlatformHeadless::create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> {
    if (config.width <= 0 || config.height <= 0) {
        return make_error(Error::Category::WindowCreationFailed, "Invalid window dimensions");
    }

    std::cerr << "Window created (Headless): " << config.width << "x" << config.height << "\n";
    return std::unique_ptr<Window>(new WindowHeadless(config.width, config.height));
}

} // namespace buddd::engine
