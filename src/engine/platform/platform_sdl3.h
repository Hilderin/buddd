#pragma once

#include "platform.h"
#include "input/input_system_sdl3.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <unordered_map>

namespace buddd::engine {

class PlatformSDL3 final : public Platform {
public:
    ~PlatformSDL3() override;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;
    auto poll_events() -> bool override;
    auto input_system() -> InputSystem& override;
    auto register_window(SDL_WindowID id, Window* window) -> void;
    auto unregister_window(SDL_WindowID id) -> void;
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
    std::unordered_map<SDL_WindowID, Window*> window_map_;
};

} // namespace buddd::engine
