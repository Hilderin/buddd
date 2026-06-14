#pragma once

#include "render/frame_buffer.h"
#include "render/texture_opengl.h"

#include <SDL3/SDL_opengl.h>

#include <cstdint>
#include <memory>

namespace buddd::engine {

class FrameBufferOpenGL final : public FrameBuffer {
public:
    /// Creates the FBO with color and depth attachments at the given size.
    /// Calls glCheckFramebufferStatus and returns error if incomplete.
    static auto create(uint32_t width, uint32_t height) -> Result<std::unique_ptr<FrameBuffer>>;

    ~FrameBufferOpenGL() override;

    auto bind() -> void override;
    auto unbind() -> void override;
    auto resize(uint32_t width, uint32_t height) -> Result<void> override;
    auto color_texture() const noexcept -> Texture& override;
    auto width() const noexcept -> uint32_t override;
    auto height() const noexcept -> uint32_t override;

    /// Returns the underlying GL framebuffer handle.
    auto handle() const noexcept -> GLuint { return fbo_; }

    FrameBufferOpenGL(const FrameBufferOpenGL&) = delete;
    auto operator=(const FrameBufferOpenGL&) -> FrameBufferOpenGL& = delete;
    FrameBufferOpenGL(FrameBufferOpenGL&&) = delete;
    auto operator=(FrameBufferOpenGL&&) -> FrameBufferOpenGL& = delete;

private:
    // Private constructor — use create().
    FrameBufferOpenGL(uint32_t width, uint32_t height,
                      GLuint fbo, GLuint rbo_depth,
                      std::unique_ptr<Texture> color_tex);

    void destroy_attachments() noexcept;

    GLuint fbo_;
    GLuint rbo_depth_;
    std::unique_ptr<Texture> color_texture_;
    uint32_t width_;
    uint32_t height_;
    GLint previous_fbo_ = 0;          // saved by bind(), restored by unbind()
    GLint previous_viewport_[4] = {}; // saved by bind(), restored by unbind()
};

} // namespace buddd::engine
