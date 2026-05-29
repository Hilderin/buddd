#include "render_device_opengl.h"

#include <GL/gl.h>

namespace buddd::engine {

RenderDeviceOpenGL::RenderDeviceOpenGL(SDL_Window* window, SDL_GLContext context)
    : window_(window), context_(context) {}

RenderDeviceOpenGL::~RenderDeviceOpenGL() {
    SDL_GL_DestroyContext(context_);
}

auto RenderDeviceOpenGL::begin_frame() -> void {
    glClear(GL_COLOR_BUFFER_BIT);
}

auto RenderDeviceOpenGL::end_frame() -> void {
    SDL_GL_SwapWindow(window_);
}

auto RenderDeviceOpenGL::size() const noexcept -> std::pair<int, int> {
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);
    return {w, h};
}

} // namespace buddd::engine
