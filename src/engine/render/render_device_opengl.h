#pragma once

#include "render_device.h"

#include <SDL3/SDL.h>

namespace buddd::engine {

class RenderDeviceOpenGL final : public RenderDevice {
public:
    RenderDeviceOpenGL(SDL_Window* window, SDL_GLContext context);
    ~RenderDeviceOpenGL() override;

    auto begin_frame() -> void override;
    auto end_frame() -> void override;
    auto size() const noexcept -> std::pair<int, int> override;

    RenderDeviceOpenGL(const RenderDeviceOpenGL&) = delete;
    auto operator=(const RenderDeviceOpenGL&) -> RenderDeviceOpenGL& = delete;
    RenderDeviceOpenGL(RenderDeviceOpenGL&&) = delete;
    auto operator=(RenderDeviceOpenGL&&) -> RenderDeviceOpenGL& = delete;

private:
    SDL_Window* window_;
    SDL_GLContext context_;
};

} // namespace buddd::engine
