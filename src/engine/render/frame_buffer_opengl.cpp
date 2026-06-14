#define GL_GLEXT_PROTOTYPES
#include "render/frame_buffer_opengl.h"

#include "log/log.h"

#include <cstdio>
#include <string>

BUDDD_LOG_TAG("Render:OpenGL");

namespace buddd::engine {

namespace {

/// Formats a GLenum as "0x" followed by 4 lowercase hex digits.
auto to_hex_string(GLenum value) -> std::string {
    char buf[12];
    std::snprintf(buf, sizeof(buf), "0x%04x", static_cast<unsigned int>(value));
    return std::string(buf);
}

} // anonymous namespace

// ============================================================================
// Factory / Constructor / Destructor
// ============================================================================

FrameBufferOpenGL::FrameBufferOpenGL(uint32_t width, uint32_t height,
                                     GLuint fbo, GLuint rbo_depth,
                                     std::unique_ptr<Texture> color_tex)
    : fbo_(fbo)
    , rbo_depth_(rbo_depth)
    , color_texture_(std::move(color_tex))
    , width_(width)
    , height_(height)
{}

auto FrameBufferOpenGL::create(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<FrameBuffer>>
{
    // 1. Validate dimensions
    if (width == 0 || height == 0) {
        return make_error(Error::Category::InvalidArgument,
            "FrameBuffer dimensions must be positive");
    }

    // 2. Create color texture
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    auto color_tex = std::unique_ptr<Texture>(
        new TextureOpenGL(tex, static_cast<int>(width), static_cast<int>(height), 4));

    // 3. Create depth renderbuffer
    GLuint rbo;
    glCreateRenderbuffers(1, &rbo);
    glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

    // 4. Create FBO and attach
    GLuint fbo;
    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
    glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // 5. Check completeness
    GLenum status = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        // Clean up
        glDeleteTextures(1, &tex);
        glDeleteRenderbuffers(1, &rbo);
        glDeleteFramebuffers(1, &fbo);
        return make_error(Error::Category::ResourceCreationFailed,
            "Framebuffer is incomplete (status: " + to_hex_string(status) + ")");
    }

    // 6. Log
    BUDDD_LOG_INFO("FrameBuffer created ({}x{})", width, height);

    // 7. Return
    return std::unique_ptr<FrameBuffer>(
        new FrameBufferOpenGL(width, height, fbo, rbo, std::move(color_tex)));
}

FrameBufferOpenGL::~FrameBufferOpenGL() {
    destroy_attachments();
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    BUDDD_LOG_DEBUG("FrameBuffer destroyed");
}

// ============================================================================
// Attachments management
// ============================================================================

void FrameBufferOpenGL::destroy_attachments() noexcept {
    // Delete color texture (TextureOpenGL destructor calls glDeleteTextures)
    color_texture_.reset();

    // Delete depth renderbuffer
    if (rbo_depth_ != 0) {
        glDeleteRenderbuffers(1, &rbo_depth_);
        rbo_depth_ = 0;
    }
}

// ============================================================================
// Bind / Unbind
// ============================================================================

auto FrameBufferOpenGL::bind() -> void {
    // Save current FBO binding
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo_);
    // Save current viewport
    glGetIntegerv(GL_VIEWPORT, previous_viewport_);

    // Bind this FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    // Set viewport to FBO dimensions
    glViewport(0, 0, static_cast<GLint>(width_), static_cast<GLint>(height_));

    BUDDD_LOG_TRACE("FrameBuffer bound (id={})", fbo_);
}

auto FrameBufferOpenGL::unbind() -> void {
    // Restore previous viewport
    glViewport(previous_viewport_[0], previous_viewport_[1],
               previous_viewport_[2], previous_viewport_[3]);
    // Restore previous FBO binding
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previous_fbo_));

    BUDDD_LOG_TRACE("Default framebuffer restored");
}

// ============================================================================
// Resize
// ============================================================================

auto FrameBufferOpenGL::resize(uint32_t width, uint32_t height) -> Result<void> {
    if (width == 0 || height == 0) {
        return make_error(Error::Category::InvalidArgument,
            "FrameBuffer dimensions must be positive");
    }

    uint32_t old_w = width_;
    uint32_t old_h = height_;

    // Destroy existing attachments (but keep the FBO handle itself)
    destroy_attachments();

    // Create new color texture
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    color_texture_ = std::unique_ptr<Texture>(
        new TextureOpenGL(tex, static_cast<int>(width), static_cast<int>(height), 4));

    // Create new depth renderbuffer
    GLuint rbo;
    glCreateRenderbuffers(1, &rbo);
    glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    rbo_depth_ = rbo;

    // Attach to existing FBO
    glNamedFramebufferTexture(fbo_, GL_COLOR_ATTACHMENT0, tex, 0);
    glNamedFramebufferRenderbuffer(fbo_, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Update stored dimensions
    width_ = width;
    height_ = height;

    // Check completeness
    GLenum status = glCheckNamedFramebufferStatus(fbo_, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        return make_error(Error::Category::ResourceCreationFailed,
            "Framebuffer is incomplete after resize (status: " + to_hex_string(status) + ")");
    }

    BUDDD_LOG_INFO("FrameBuffer resized ({}x{} -> {}x{})", old_w, old_h, width, height);
    return {};
}

// ============================================================================
// Accessors
// ============================================================================

auto FrameBufferOpenGL::color_texture() const noexcept -> Texture& {
    return *color_texture_;
}

auto FrameBufferOpenGL::width() const noexcept -> uint32_t {
    return width_;
}

auto FrameBufferOpenGL::height() const noexcept -> uint32_t {
    return height_;
}

} // namespace buddd::engine
