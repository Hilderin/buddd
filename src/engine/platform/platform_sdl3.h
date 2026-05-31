#pragma once

#include "platform.h"
#include "input/input_system_sdl3.h"

#include <cstdint>

namespace buddd::engine {

class PlatformSDL3 final : public Platform {
public:
    ~PlatformSDL3() override;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;
    auto poll_events() -> bool override;
    auto input_system() -> InputSystem& override;
    [[nodiscard]] auto delta_time() const noexcept -> float override;

    PlatformSDL3(const PlatformSDL3&) = delete;
    auto operator=(const PlatformSDL3&) -> PlatformSDL3& = delete;
    PlatformSDL3(PlatformSDL3&&) = delete;
    auto operator=(PlatformSDL3&&) -> PlatformSDL3& = delete;

private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformSDL3() = default;

    InputSystemSDL3 input_system_;
    float delta_time_{1.0f / 60.0f};
    uint64_t last_frame_ticks_{0};
};

} // namespace buddd::engine
