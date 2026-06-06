#include "render_device.h"
#include "render_device_opengl.h"
#include "render_device_headless.h"
#include "window/window.h"

#include <SDL3/SDL.h>

#include "log/log.h"

BUDDD_LOG_TAG("Render");

namespace buddd::engine {

auto RenderDevice::create(Window& window) -> Result<std::unique_ptr<RenderDevice>> {
    auto* native = window.native_handle();

    if (native == nullptr) {
        BUDDD_LOG_INFO("Render device created (Headless)");
        return std::unique_ptr<RenderDevice>(
            new RenderDeviceHeadless(window));
    }

    // SDL3/OpenGL backend
    auto* sdl_window = static_cast<SDL_Window*>(native);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    // Request a 24-bit depth buffer for correct 3D occlusion.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    BUDDD_LOG_DEBUG("Depth buffer requested: 24-bit");

    auto* gl_context = SDL_GL_CreateContext(sdl_window);
    if (gl_context == nullptr) {
        return make_error(Error::Category::RenderDeviceCreationFailed,
            "SDL_GL_CreateContext failed: " + std::string(SDL_GetError()));
    }

    SDL_GL_MakeCurrent(sdl_window, gl_context);

    BUDDD_LOG_INFO("Render device created (OpenGL 4.5 Core)");
    return std::unique_ptr<RenderDevice>(
        new RenderDeviceOpenGL(window, sdl_window, gl_context));
}

} // namespace buddd::engine
