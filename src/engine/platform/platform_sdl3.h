#pragma once

#include "platform.h"
#include "input/input_system_sdl3.h"

namespace buddd::engine {

class PlatformSDL3 final : public Platform {
public:
    ~PlatformSDL3() override;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;
    auto poll_events() -> bool override;
    auto input_system() -> InputSystem& override;

    PlatformSDL3(const PlatformSDL3&) = delete;
    auto operator=(const PlatformSDL3&) -> PlatformSDL3& = delete;
    PlatformSDL3(PlatformSDL3&&) = delete;
    auto operator=(PlatformSDL3&&) -> PlatformSDL3& = delete;

private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformSDL3() = default;

    InputSystemSDL3 input_system_;
};

} // namespace buddd::engine
