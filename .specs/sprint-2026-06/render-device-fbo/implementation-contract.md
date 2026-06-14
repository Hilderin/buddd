# IMPL-2026-06-RDFBO - RenderDevice FBO / Render-to-Texture

## Source spec

- `.specs/sprint-2026-06/render-device-fbo/spec.md`

## Goal

Add offscreen rendering capability to the engine's `RenderDevice` abstraction by introducing a `FrameBuffer` class hierarchy (abstract base + OpenGL/Headless backends), new `RenderDevice` factory methods (`create_render_texture`, `create_frame_buffer`, `read_pixels(FrameBuffer&)` overload), and a `RenderSystem::render_scene(FrameBuffer&)` overload that renders into a specified FBO. All functionality is also available in the headless backend as safe no-ops.

## Non-goals

- No stencil attachment.
- No depth texture (renderbuffer only, not a texture).
- No multisampling (MSAA).
- No FBO pool or caching.
- No editor viewport panel (F-07 — separate feature).
- No `RenderDevice::begin_frame(FrameBuffer*)` or `begin_frame(FrameBuffer&)` overload.
- No changes to `begin_frame()` / `end_frame()` frame pipeline.
- No color attachment format customization (always RGBA8).
- No depth attachment format customization (always `GL_DEPTH_COMPONENT24`).
- No modification to existing `read_pixels()` (parameterless) — it remains unchanged.
- No modification to existing `render_scene()` (parameterless) — it remains unchanged.
- No changes to `app.cpp` main loop.

## Relevant ADRs

- **ADR-001** (`docs/adr/ADR-001-result-error-pattern.md`): All new fallible APIs must return `Result<T>`. The `Error` enum may need new categories (but existing `TextureCreationFailed`, `ResourceCreationFailed`, `InvalidArgument`, `Unsupported` are sufficient).
- **ADR-003** (`docs/adr/ADR-003-render-pipeline-architecture.md`): Draw methods return `void` (not `Result`). This is unaffected — new methods (create, resize, read_pixels) return `Result<T>` per ADR-001.

## Files to inspect

The Code Agent must read these files before editing (already read during contract authoring, but re-read for implementation):

- `src/engine/render/render_device.h` — understand abstract API, forward declarations, protected constructor pattern
- `src/engine/render/render_device_opengl.h` — understand OpenGL backend class shape
- `src/engine/render/render_device_opengl.cpp` — understand DSA texture creation pattern (`glCreateTextures` + `glTextureStorage2D`) and `read_pixels()` implementation
- `src/engine/render/render_device_headless.h` — understand headless backend class shape
- `src/engine/render/render_device_headless.cpp` — understand headless resource factory patterns, `read_pixels()` error return
- `src/engine/render/render_system.h` — understand current `render_scene()` signature
- `src/engine/render/render_system.cpp` — understand render loop logic that the FBO overload will wrap
- `src/engine/render/texture.h` — understand `Texture` abstract base (virtual `width()`, `height()`, `channels()`)
- `src/engine/render/texture_opengl.h` / `texture_opengl.cpp` — understand OpenGL texture wrapper (GLuint handle, destructor calls `glDeleteTextures`)
- `src/engine/render/texture_headless.h` / `texture_headless.cpp` — understand headless texture pattern (stores dimensions + pixel data)
- `src/engine/error.h` — understand `Result<T>`, `Error::Category`, `make_error()`
- `src/engine/image/image_buffer.h` — understand `ImageBuffer` aggregate struct
- `src/engine/render/material.h` — understand `Material::set_texture()` for FBO color texture usage
- `tests/engine/render_device_tests.cpp` — understand test conventions (`make_headless_engine()` helper, Catch2 macros, `[headless]` tags)
- `tests/CMakeLists.txt` — understand test discovery (auto-glob `*_tests.cpp`, conditional REGEX filter for display-dependent tests)

## Files allowed to change

The following files may be created or modified. Each entry specifies the exact changes.

### New files (create)

1. **`src/engine/render/frame_buffer.h`** — Abstract base class declaration.
2. **`src/engine/render/frame_buffer_opengl.h`** — OpenGL FBO implementation header.
3. **`src/engine/render/frame_buffer_opengl.cpp`** — OpenGL FBO implementation body.
4. **`src/engine/render/frame_buffer_headless.h`** — Headless FBO implementation header.
5. **`src/engine/render/frame_buffer_headless.cpp`** — Headless FBO implementation body.
6. **`tests/engine/render_device_fbo_test.cpp`** — Headless FBO unit tests (always compiled).
7. **`tests/engine/render_device_fbo_opengl_test.cpp`** — OpenGL FBO integration test (conditional on `BUDDD_HAS_DISPLAY`).

### Modified files (edit)

8. **`src/engine/render/render_device.h`** — Add forward declarations for `FrameBuffer`; add three new pure virtual methods.
9. **`src/engine/render/render_device_opengl.h`** — Add three new `override` declarations.
10. **`src/engine/render/render_device_opengl.cpp`** — Add implementation of the three new methods; add includes.
11. **`src/engine/render/render_device_headless.h`** — Add three new `override` declarations.
12. **`src/engine/render/render_device_headless.cpp`** — Add implementation of the three new methods; add includes.
13. **`src/engine/render/render_system.h`** — Add `render_scene(FrameBuffer& target)` overload declaration.
14. **`src/engine/render/render_system.cpp`** — Add include for `frame_buffer.h` and `render_scene(FrameBuffer&)` implementation.

## Files forbidden to change

- `src/cmd/app.cpp` — no changes to the frame pipeline or main loop.
- Any file in `src/engine/scene/` — camera, world, entity, transform are unaffected.
- Any file in `src/engine/image/` — not directly affected.
- Any file in `src/engine/editor/` — editor is out of scope for this feature.
- `docs/adr/ADR-001.md` or `docs/adr/ADR-003.md` — no ADR changes needed.
- `tests/engine/scene_rendering_tests.cpp` — existing tests must pass unchanged.
- `tests/engine/render_device_tests.cpp` — existing tests must pass unchanged.

## Existing conventions to follow

1. **Abstract base + backends**: `FrameBuffer` is an abstract base class with `FrameBufferOpenGL` and `FrameBufferHeadless` concrete subclasses (same pattern as `Texture` / `TextureOpenGL` / `TextureHeadless`).
2. **Non-copyable, non-movable**: Follow the `= delete` pattern for copy and move operations from `Texture` / `RenderDevice`. `FrameBuffer` is stored in `std::unique_ptr<FrameBuffer>` (returned from factories).
3. **`Result<T>` for fallible APIs**: All new factory methods, `resize()`, and `read_pixels(FrameBuffer&)` return `Result<T>`. Use `make_error()` for error construction.
4. **Error messages**: Use the exact error messages from the spec error cases table (lines 177–185).
5. **Includes**: Use `"render/..."` style includes (relative to `src/engine/`), following the existing convention in `render_device.h` and `render_system.cpp`.
6. **Logging**: Use `BUDDD_LOG_TAG("Render:OpenGL")` / `BUDDD_LOG_TAG("Render:Headless")` at file scope. Use `BUDDD_LOG_INFO`, `BUDDD_LOG_DEBUG`, `BUDDD_LOG_TRACE`, `BUDDD_LOG_ERROR` with message strings matching the spec observability table (lines 196–205).
7. **Namespace**: All new code goes in `namespace buddd::engine` (the existing engine namespace).
8. **DSA OpenGL**: Use direct-state-access (DSA) GL functions (`glCreateTextures`, `glTextureStorage2D`, `glCreateRenderbuffers`, `glNamedRenderbufferStorage`, `glCreateFramebuffers`, `glNamedFramebufferTexture`, `glNamedFramebufferRenderbuffer`, `glCheckNamedFramebufferStatus`) for the FBO implementation, consistent with the existing texture creation pattern in `render_device_opengl.cpp`.
9. **Forward declarations**: Add forward declarations for `FrameBuffer` in `render_device.h` (before the `RenderDevice` class) following the existing pattern for `Texture`, `Shader`, `Material`, etc.
10. **Test tags**: Use `[render][fbo][headless]` for headless tests, `[render][fbo][opengl]` for the OpenGL integration test (matching existing tag style in `render_device_tests.cpp`).

## Required implementation behavior

### 1. FrameBuffer abstract base (`src/engine/render/frame_buffer.h`)

```cpp
#pragma once

#include "error.h"

#include <cstdint>
#include <memory>
#include <utility>

namespace buddd::engine {

class Texture;
class FrameBuffer;

class FrameBuffer {
public:
    virtual ~FrameBuffer();

    virtual auto bind() -> void = 0;
    virtual auto unbind() -> void = 0;
    virtual auto resize(uint32_t width, uint32_t height) -> Result<void> = 0;
    [[nodiscard]] virtual auto color_texture() const noexcept -> Texture& = 0;
    [[nodiscard]] virtual auto width() const noexcept -> uint32_t = 0;
    [[nodiscard]] virtual auto height() const noexcept -> uint32_t = 0;

    FrameBuffer(const FrameBuffer&) = delete;
    auto operator=(const FrameBuffer&) -> FrameBuffer& = delete;
    FrameBuffer(FrameBuffer&&) = delete;
    auto operator=(FrameBuffer&&) -> FrameBuffer& = delete;

protected:
    FrameBuffer() = default;
};

} // namespace buddd::engine
```

### 2. FrameBufferOpenGL (`src/engine/render/frame_buffer_opengl.h` and `.cpp`)

**Header**:
```cpp
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
```

**Implementation details** (`.cpp`):

- **`create(w, h)`**:
  1. Validate `w > 0 && h > 0` → error `InvalidArgument`, "FrameBuffer dimensions must be positive".
  2. Create color texture via:
     - `glCreateTextures(GL_TEXTURE_2D, 1, &tex)`
     - `glTextureStorage2D(tex, 1, GL_RGBA8, w, h)`
     - `glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR)`
     - `glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR)`
     - `glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)`
     - `glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)`
     - Wrap in `std::unique_ptr<Texture>(new TextureOpenGL(tex, static_cast<int>(w), static_cast<int>(h), 4))`
  3. Create depth renderbuffer:
     - `glCreateRenderbuffers(1, &rbo)`
     - `glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, w, h)`
  4. Create FBO:
     - `glCreateFramebuffers(1, &fbo)`
     - `glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0)`
     - `glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo)`
  5. Check completeness: `GLenum status = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER)`
     - If not `GL_FRAMEBUFFER_COMPLETE` → clean up (delete texture, renderbuffer, framebuffer) and return error `ResourceCreationFailed`, "Framebuffer is incomplete (status: 0xXXXX)"
  6. Log: `BUDDD_LOG_INFO("FrameBuffer created ({}x{})", w, h)`
  7. Return `std::unique_ptr<FrameBuffer>(new FrameBufferOpenGL(w, h, fbo, rbo, std::move(color_tex)))`

- **Destructor**: Call `destroy_attachments()`. Log: `BUDDD_LOG_DEBUG("FrameBuffer destroyed")`.

- **`destroy_attachments()`**: Private helper. Deletes `color_texture_` (which in turn calls `glDeleteTextures`), `glDeleteRenderbuffers(1, &rbo_depth_)`, `glDeleteFramebuffers(1, &fbo_)`. Sets handles to 0 to prevent double-deletion.

- **`bind()`**:
  1. `glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo_)` — save current FBO
  2. `glGetIntegerv(GL_VIEWPORT, previous_viewport_)` — save current viewport
  3. `glBindFramebuffer(GL_FRAMEBUFFER, fbo_)`
  4. Set viewport: `glViewport(0, 0, static_cast<GLint>(width_), static_cast<GLint>(height_))`
  5. Log: `BUDDD_LOG_TRACE("FrameBuffer bound (id={})", fbo_)`

- **`unbind()`**:
  1. Restore viewport: `glViewport(previous_viewport_[0], previous_viewport_[1], previous_viewport_[2], previous_viewport_[3])`
  2. `glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previous_fbo_))`
  3. Log: `BUDDD_LOG_TRACE("Default framebuffer restored")`

- **`resize(w, h)`**:
  1. Validate `w > 0 && h > 0` → error `InvalidArgument`, "FrameBuffer dimensions must be positive"
  2. Save old width/height.
  3. Call `destroy_attachments()` (which deletes old texture, renderbuffer, but NOT the FBO handle itself — just detach and delete attachments).
  4. Create new texture, new renderbuffer, re-attach to existing FBO (same steps as create).
  5. Update `width_`, `height_`.
  6. Check completeness; if incomplete → error `ResourceCreationFailed`, "Framebuffer is incomplete after resize (status: 0xXXXX)". On error, FBO is left in invalid state.
  7. Log: `BUDDD_LOG_INFO("FrameBuffer resized ({}x{} -> {}x{})", old_w, old_h, w, h)`

- **`color_texture()`**: Return `*color_texture_` reference.

- **`width()` / `height()`**: Return the stored dimensions.

### 3. FrameBufferHeadless (`src/engine/render/frame_buffer_headless.h` and `.cpp`)

**Header**:
```cpp
#pragma once

#include "render/frame_buffer.h"
#include "render/texture_headless.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace buddd::engine {

class FrameBufferHeadless final : public FrameBuffer {
public:
    static auto create(uint32_t width, uint32_t height) -> Result<std::unique_ptr<FrameBuffer>>;

    ~FrameBufferHeadless() override = default;

    auto bind() -> void override;
    auto unbind() -> void override;
    auto resize(uint32_t width, uint32_t height) -> Result<void> override;
    auto color_texture() const noexcept -> Texture& override;
    auto width() const noexcept -> uint32_t override;
    auto height() const noexcept -> uint32_t override;

    FrameBufferHeadless(const FrameBufferHeadless&) = delete;
    auto operator=(const FrameBufferHeadless&) -> FrameBufferHeadless& = delete;
    FrameBufferHeadless(FrameBufferHeadless&&) = delete;
    auto operator=(FrameBufferHeadless&&) -> FrameBufferHeadless& = delete;

private:
    FrameBufferHeadless(uint32_t width, uint32_t height,
                        std::unique_ptr<Texture> color_tex);

    std::unique_ptr<Texture> color_texture_;
    uint32_t width_;
    uint32_t height_;
};

} // namespace buddd::engine
```

**Implementation details** (`.cpp`):

- **`create(w, h)`**:
  1. Validate `w > 0 && h > 0` → error `InvalidArgument`, "FrameBuffer dimensions must be positive".
  2. Create a `TextureHeadless` with zeroed pixel data (RGBA8, size `w * h * 4`, all bytes = 0). This is a storage-only no-op texture with no real data.
  3. Log: `BUDDD_LOG_INFO("FrameBuffer created ({}x{})", w, h)`
  4. Return `std::unique_ptr<FrameBuffer>(new FrameBufferHeadless(w, h, std::move(color_tex)))`

- **`bind()`**: No-op.

- **`unbind()`**: No-op.

- **`resize(w, h)`**:
  1. Validate `w > 0 && h > 0` → error `InvalidArgument`, "FrameBuffer dimensions must be positive".
  2. Log: `BUDDD_LOG_INFO("FrameBuffer resized ({}x{} -> {}x{})", width_, height_, w, h)`
  3. Update `width_`, `height_`.
  4. Create a new `TextureHeadless` with the new dimensions (zeroed data), store as `color_texture_`.
  5. Return success.

- **`color_texture()`**: Return `*color_texture_` reference.

- **`width()` / `height()`**: Return the stored dimensions.

### 4. RenderDevice changes

#### `src/engine/render/render_device.h`

Add forward declaration before `class RenderDevice`:
```cpp
class FrameBuffer;
```

Add three new pure virtual methods after `create_texture`:
```cpp
/// Creates a storage-only render texture (no initial CPU data).
/// Suitable as a color attachment for FrameBuffer.
/// The texture uses GL_LINEAR filtering and GL_CLAMP_TO_EDGE wrapping.
/// @return Error::TextureCreationFailed if the GPU cannot allocate the texture.
[[nodiscard]] virtual auto create_render_texture(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<Texture>> = 0;

/// Creates a complete FrameBuffer with a color attachment (RGBA8 texture)
/// and a depth attachment (D24 renderbuffer).
/// @param width  Must be > 0.
/// @param height Must be > 0.
/// @return Error::InvalidArgument if dimensions are zero.
/// @return Error::ResourceCreationFailed if the FBO completeness check fails.
[[nodiscard]] virtual auto create_frame_buffer(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<FrameBuffer>> = 0;

/// Reads pixel data from a custom FrameBuffer.
/// The returned ImageBuffer has bottom-left pixel origin (OpenGL convention).
/// @param fbo The FBO to read from.
/// @return Error::Unsupported in headless mode.
/// @return Error::ReadbackFailed if glReadPixels fails.
[[nodiscard]] virtual auto read_pixels(FrameBuffer& fbo)
    -> Result<ImageBuffer> = 0;
```

#### `src/engine/render/render_device_opengl.h`

Add three override declarations after `read_pixels()`:
```cpp
auto create_render_texture(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<Texture>> override;
auto create_frame_buffer(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<FrameBuffer>> override;
auto read_pixels(FrameBuffer& fbo)
    -> Result<ImageBuffer> override;
```

#### `src/engine/render/render_device_opengl.cpp`

Add includes:
```cpp
#include "render/frame_buffer_opengl.h"
```

Add implementations:

- **`create_render_texture(w, h)`**:
  1. Validate `w > 0 && h > 0` → error `InvalidArgument`, "Render texture dimensions must be positive".
  2. `glCreateTextures(GL_TEXTURE_2D, 1, &tex)`
  3. `glTextureStorage2D(tex, 1, GL_RGBA8, static_cast<GLsizei>(w), static_cast<GLsizei>(h))`
  4. Check GL error after `glTextureStorage2D`; if error → clean up and return `TextureCreationFailed`.
  5. `glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR)`
  6. `glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR)`
  7. `glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)`
  8. `glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)`
  9. Log: `BUDDD_LOG_INFO("Render texture created (OpenGL, {}x{})", w, h)`
  10. Return `std::unique_ptr<Texture>(new TextureOpenGL(tex, static_cast<int>(w), static_cast<int>(h), 4))`.

- **`create_frame_buffer(w, h)`**: Delegate to `FrameBufferOpenGL::create(w, h)`.

- **`read_pixels(FrameBuffer& fbo)`**:
  1. Get `w = fbo.width()`, `h = fbo.height()`.
  2. Set up `ImageBuffer` with those dimensions, channels = 4.
  3. Save current FBO binding (belt + suspenders; `FrameBuffer::bind()` already does this but we need the current state).
  4. `glBindFramebuffer(GL_FRAMEBUFFER, ...)` — get the FBO handle. Cast the `FrameBuffer&` to `FrameBufferOpenGL&` via `dynamic_cast` (or `static_cast` if the design guarantees it). Access `fbo_` handle. (Alternative: the FBO is already bound because `FrameBuffer::bind()` was called. We can just `glReadBuffer(GL_COLOR_ATTACHMENT0)`, `glReadPixels`, and the current bound FBO will be the right one.)
  5. `glReadBuffer(GL_COLOR_ATTACHMENT0)`
  6. `glPixelStorei(GL_PACK_ALIGNMENT, 1)`
  7. `glReadPixels(0, 0, static_cast<GLint>(w), static_cast<GLint>(h), GL_RGBA, GL_UNSIGNED_BYTE, buffer.data.data())`
  8. Check GL error → `ReadbackFailed` if error.
  9. Do NOT unbind the FBO — the caller is responsible for bind/unbind pairing.
  10. Return `buffer`.

  Note: If the FBO is not currently bound, the readback will read from whatever is bound (likely the default framebuffer). The caller must call `fbo.bind()` before `read_pixels(fbo)`. This is documented via the `fbo` parameter requirement. However, for convenience, the implementation should bind the FBO itself during `read_pixels` and restore the previous binding.

  **Decision**: The implementation of `read_pixels(FrameBuffer& fbo)` should:
  1. Save current FBO binding.
  2. Bind `fbo` (using `glBindFramebuffer` after downcasting to get the GL handle).
  3. Perform the readback.
  4. Restore the previous FBO binding.

  This makes `read_pixels(FrameBuffer&)` self-contained: the caller does not need to have the FBO bound beforehand. This is the most ergonomic design.

  Implementation: `auto& gl_fbo = static_cast<FrameBufferOpenGL&>(fbo);`, then `glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo)`, `glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo.handle())`, read, `glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo)`.

  **Add accessor**: `FrameBufferOpenGL` needs a `handle() -> GLuint` method (returning `fbo_`). Add it to the header.

#### `src/engine/render/render_device_headless.h`

Add three override declarations after `read_pixels()`:
```cpp
auto create_render_texture(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<Texture>> override;
auto create_frame_buffer(uint32_t width, uint32_t height)
    -> Result<std::unique_ptr<FrameBuffer>> override;
auto read_pixels(FrameBuffer& fbo)
    -> Result<ImageBuffer> override;
```

#### `src/engine/render/render_device_headless.cpp`

Add includes:
```cpp
#include "render/frame_buffer_headless.h"
```

Add implementations:

- **`create_render_texture(w, h)`**:
  1. Validate `w > 0 && h > 0` → error `InvalidArgument`, "Render texture dimensions must be positive".
  2. Create a `TextureHeadless` with zeroed pixel data (size `w * h * 4`, all bytes = 0).
  3. Log: `BUDDD_LOG_INFO("Render texture created (Headless, {}x{})", w, h)`
  4. Return `std::unique_ptr<Texture>(new TextureHeadless(static_cast<int>(w), static_cast<int>(h), 4, std::move(data)))`

- **`create_frame_buffer(w, h)`**: Delegate to `FrameBufferHeadless::create(w, h)`.

- **`read_pixels(FrameBuffer&)`**: Return `make_error(Error::Category::Unsupported, "read_pixels with FBO is not supported in headless mode")`.

### 5. RenderSystem changes

#### `src/engine/render/render_system.h`

Add forward declaration before `class RenderSystem`:
```cpp
class FrameBuffer;
```

Add overload after `render_scene()`:
```cpp
/// Renders the scene into the specified FBO.
/// Binds the FBO before rendering, unbinds it after.
/// Behaviour is undefined if called from within a render_scene() call.
/// @param target The FBO to render into.
auto render_scene(FrameBuffer& target) -> void;
```

#### `src/engine/render/render_system.cpp`

Add include:
```cpp
#include "render/frame_buffer.h"
```

Add implementation:
```cpp
auto RenderSystem::render_scene(FrameBuffer& target) -> void {
    target.bind();
    render_scene();      // delegates to the parameterless overload
    target.unbind();
}
```

Note: No changes to the existing `render_scene()` body. No changes to `render()`.

### 6. OpenGL FBO handle accessor

Add to `frame_buffer_opengl.h`:
```cpp
auto handle() const noexcept -> GLuint { return fbo_; }
```

This is needed by `RenderDeviceOpenGL::read_pixels(FrameBuffer&)` to get the GL handle.

## Required tests

### Unit tests (headless, always run) — `tests/engine/render_device_fbo_test.cpp`

Using the existing convention of a `make_headless_engine()` helper.

All tests are tagged `[render][fbo][headless]`.

**Test 1: `FrameBuffer_Headless_CreateDestroy`**
- Create a headless engine.
- Call `device.create_frame_buffer(64, 64)`.
- REQUIRE result has_value().
- REQUIRE the returned FrameBuffer is non-null.
- REQUIRE `fbo->width() == 64`, `fbo->height() == 64`.
- Destructor runs when unique_ptr goes out of scope — no crash.
- Verifies AC-006.

**Test 2: `FrameBuffer_Headless_Resize`**
- Create `device.create_frame_buffer(64, 64)`.
- REQUIRE `fbo->resize(128, 128)` succeeds.
- REQUIRE `fbo->width() == 128`, `fbo->height() == 128`.
- REQUIRE `fbo->resize(32, 32)` succeeds.
- REQUIRE `fbo->width() == 32`, `fbo->height() == 32`.
- Verifies AC-006.

**Test 3: `FrameBuffer_Headless_BindUnbind`**
- Create `device.create_frame_buffer(64, 64)`.
- REQUIRE_NOTHROW `fbo->bind()`.
- REQUIRE_NOTHROW `fbo->unbind()`.
- Verifies AC-006 (bind/unbind don't crash).

**Test 4: `FrameBuffer_Headless_ReadPixelsFails`**
- Create `device.create_frame_buffer(64, 64)`.
- Call `device.read_pixels(*fbo)`.
- REQUIRE_FALSE result has_value().
- REQUIRE error category == `Error::Category::Unsupported`.
- Error message contains "not supported".
- Verifies AC-005.

**Test 5: `FrameBuffer_Headless_ZeroSize`**
- Call `device.create_frame_buffer(0, 64)` → REQUIRE_FALSE has_value(), category `InvalidArgument`.
- Call `device.create_frame_buffer(64, 0)` → REQUIRE_FALSE has_value(), category `InvalidArgument`.
- Verifies AC-007.

**Test 6: `FrameBuffer_Headless_ResizeZero`**
- Create `device.create_frame_buffer(64, 64)`.
- REQUIRE_FALSE `fbo->resize(0, 128)` has_value(), category `InvalidArgument`.
- REQUIRE `fbo->width() == 64 && fbo->height() == 64` (unchanged).
- REQUIRE_FALSE `fbo->resize(128, 0)` has_value(), category `InvalidArgument`.
- REQUIRE `fbo->width() == 64 && fbo->height() == 64` (unchanged).
- Verifies edge case from spec lines 165–166.

**Test 7: `FrameBuffer_Headless_ColorTexture`**
- Create `device.create_frame_buffer(64, 64)`.
- REQUIRE `fbo->color_texture().width() == 64`.
- REQUIRE `fbo->color_texture().height() == 64`.
- REQUIRE `fbo->color_texture().channels() == 4`.
- Verifies color texture accessor works.

**Test 8: `RenderTexture_Headless_Create`**
- Create a headless engine.
- Call `device.create_render_texture(64, 64)`.
- REQUIRE result has_value().
- REQUIRE the returned Texture is non-null.
- REQUIRE `tex->width() == 64`, `tex->height() == 64`, `tex->channels() == 4`.
- Verifies the standalone `create_render_texture` API works (addresses critic warning).
- Verifies Goal #2.

**Test 9: `FrameBuffer_Headless_BindResizeUnbind`**
- Create `device.create_frame_buffer(64, 64)`.
- `fbo->bind()` — no-op, no crash.
- REQUIRE `fbo->resize(128, 128)` succeeds.
- `fbo->unbind()` — no-op, no crash.
- REQUIRE `fbo->width() == 128`, `fbo->height() == 128`.
- Verifies the bind → resize → unbind edge case (spec line 170).

### OpenGL integration test (conditional on `BUDDD_HAS_DISPLAY`) — `tests/engine/render_device_fbo_opengl_test.cpp`

The test file must be compiled only when `BUDDD_HAS_DISPLAY` is defined. Since the CMakeLists.txt already filters out `sdl3_backend_test.cpp` with a REGEX, the new file name must match a different pattern. Option: the CMakeLists.txt currently filters `.*sdl3_backend_test\\.cpp$`. The new file is `render_device_fbo_opengl_test.cpp` — this will NOT be filtered by the existing REGEX. The test should use `#ifdef BUDDD_HAS_DISPLAY` at compile-time to skip on headless, OR we add an additional filter for `opengl_test.cpp`.

**Decision**: Use a compile-time `#ifdef BUDDD_HAS_DISPLAY` guard around the test body and `REQUIRE` that `BUDDD_HAS_DISPLAY` is defined when compiling this file. This way the test file is always compiled but the test is a no-op when display is not available. The CMakeLists.txt adds `target_compile_definitions(buddd_tests PRIVATE BUDDD_HAS_DISPLAY)` when display is available, so the guard works naturally.

All tests are tagged `[render][fbo][opengl]`.

**Test: `FrameBuffer_OpenGL_RenderAndReadback`**
This test verifies AC-001, AC-002, AC-003, AC-004, AC-008.

```
#ifdef BUDDD_HAS_DISPLAY
TEST_CASE("FrameBuffer_OpenGL_RenderAndReadback", "[render][fbo][opengl]") {
    // 1. Create engine with SDL3 backend
    auto engine = EngineService::create(
        Backend::SDL3,
        WindowConfig{.title = "FBO Test", .width = 800, .height = 600});
    REQUIRE(engine.has_value());
    auto& device = engine.value()->device();
    
    // 2. Create a 64x64 FBO
    auto fbo_result = device.create_frame_buffer(64, 64);
    REQUIRE(fbo_result.has_value());
    auto& fbo = *fbo_result.value();
    REQUIRE(fbo.width() == 64);
    REQUIRE(fbo.height() == 64);
    
    // 3. Verify AC-003: unbind() restores the default framebuffer
    GLint default_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &default_fbo);
    fbo.bind();
    GLint bound_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound_fbo);
    REQUIRE(bound_fbo != default_fbo);   // FBO is now bound
    fbo.unbind();
    GLint after_unbind = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &after_unbind);
    REQUIRE(after_unbind == default_fbo); // default framebuffer restored
    
    // 4. Set up a simple scene with a camera and a colored quad/triangle
    //    (follow existing patterns from scene_rendering_tests.cpp)
    
    // ... create world, camera, mesh renderer with a colored material ...
    // ... camera looks at the mesh ...
    
    // 5. Render scene into the FBO
    RenderSystem render_system(device, world);
    render_system.render_scene(fbo);
    
    // 6. Read back pixels
    auto pixels_result = device.read_pixels(fbo);
    REQUIRE(pixels_result.has_value());
    auto& buffer = *pixels_result;
    REQUIRE(buffer.width == 64);
    REQUIRE(buffer.height == 64);
    REQUIRE(buffer.channels == 4);
    REQUIRE(buffer.data.size() == static_cast<size_t>(64 * 64 * 4));
    
    // 7. Verify some pixels have non-zero values (the scene was rendered)
    //    We don't check exact values (different GPUs/drivers may vary slightly),
    //    but we verify that the readback data is non-trivial.
    bool has_non_black_pixel = false;
    for (size_t i = 0; i < buffer.data.size(); i += 4) {
        auto r = static_cast<unsigned char>(buffer.data[i]);
        auto g = static_cast<unsigned char>(buffer.data[i + 1]);
        auto b = static_cast<unsigned char>(buffer.data[i + 2]);
        if (r > 0 || g > 0 || b > 0) {
            has_non_black_pixel = true;
            break;
        }
    }
    REQUIRE(has_non_black_pixel);
}
#else
TEST_CASE("FrameBuffer_OpenGL_RenderAndReadback - SKIPPED (no display)", "[render][fbo][opengl]") {
    // This test requires a display. In headless CI it's a no-op placeholder.
    SUCCEED("Skipped: BUDDD_HAS_DISPLAY not defined");
}
#endif
```

The test world setup (step 4) must create:
- A `World`
- An entity with `CameraComponent` (perspective, 64x64 aspect ratio, positioned at origin looking down -Z)
- A colored full-screen quad or triangle MeshRenderer covering the view frustum

Simplest approach: Create a single triangle that covers the viewport with a material using a solid color fragment shader (e.g., `vec4(0.2, 0.4, 0.6, 1.0)`). Place the camera so the triangle fills the view. The vertex shader can use a simple `u_mvp` uniform (standard).

## Edge cases

All edge cases from the spec must be handled:

1. **Zero width or height** in `create_frame_buffer(0, h)` / `(w, 0)` → error `InvalidArgument`.
2. **Zero width or height** in `resize(0, h)` / `(w, 0)` → error `InvalidArgument`, FBO unchanged (width/height stay at previous values).
3. **Very large dimensions**: If `glTextureStorage2D` fails (e.g., exceeds `GL_MAX_TEXTURE_SIZE`), return `TextureCreationFailed` for `create_render_texture` and `ResourceCreationFailed` for `create_frame_buffer`.
4. **Resize to same dimensions**: Should succeed, recreating attachments at the same size.
5. **Double resize**: `resize(A)` then `resize(B)` → second succeeds, FBO at size B.
6. **Bind → resize → unbind**: Resizing while bound is allowed (implementation clears attachments and recreates; the FBO handle remains valid). After resize, `bind()` must be called again (the FBO changed).
7. **Destroy after resize**: Destructor cleans up current attachments only — the `destroy_attachments()` helper handles the current state correctly.
8. **Multiple FBOs**: Each is independent. Bind/unbind stack is per-context (OpenGL handles this via `glGetIntegerv(GL_FRAMEBUFFER_BINDING, ...)` save/restore).
9. **`color_texture()` after destruction**: Undefined behavior (use-after-free). Not checked at runtime — callers must ensure `FrameBuffer` outlives texture usage. Documented in spec.
10. **FBO incomplete**: `create_frame_buffer` and `resize()` check completeness via `glCheckNamedFramebufferStatus` and return error on failure.
11. **Headless all operations**: bind/unbind/resize are no-ops, no crash. `read_pixels(FrameBuffer&)` returns `Unsupported` error.

## Security impact

- No new security concerns. Input validation (dimensions > 0) prevents degenerate allocations. All GL operations are driver-validated. No file I/O or network access.
- FBO resize with zero dimensions is validated and rejected with `InvalidArgument`, preventing potential driver-level issues.

## Data and migration impact

None. No new data formats, schema changes, or migrations. Pixel readback returns `ImageBuffer` (existing type). No on-disk state changes.

## API compatibility impact

- **Backward compatible**: All existing public API is unchanged. `render_scene()` (parameterless), `read_pixels()` (parameterless), `create_texture(const Image&)` are unchanged.
- **Additive**: Three new pure virtual methods on `RenderDevice` base class. Any existing custom `RenderDevice` subclass (outside this codebase) will fail to compile until it implements the new methods — but since this is an engine-internal API, no external subclasses exist.
- `RenderSystem::render_scene(FrameBuffer&)` is additive — existing callers of the parameterless overload are unaffected.
- `FrameBuffer` is a new abstract base class. No existing code references it.

## Documentation impact

- **README**: None.
- **Wiki pages**: After implementation, the wiki-agent will update `docs/wiki/architecture/overview.md` and `docs/wiki/architecture/data-flow.md` to describe FrameBuffer usage.
- **Other specs**: None (editor viewport spec F-07 will reference this feature).

## ADR impact

No existing ADRs need modification. No new ADR is required. The implementation follows established patterns (ADR-001 for error handling, ADR-003 for the RenderDevice abstraction pattern, no new architectural decisions).

## Done criteria

The Code Agent must satisfy all of the following before reporting completion:

- [ ] **DC-001**: `src/engine/render/frame_buffer.h` exists with the abstract `FrameBuffer` class as specified.
- [ ] **DC-002**: `src/engine/render/frame_buffer_opengl.h` and `.cpp` exist with the `FrameBufferOpenGL` implementation using DSA GL calls. `bind()` saves and sets viewport; `unbind()` restores previous viewport.
- [ ] **DC-003**: `src/engine/render/frame_buffer_headless.h` and `.cpp` exist with the `FrameBufferHeadless` no-op implementation.
- [ ] **DC-004**: `src/engine/render/render_device.h` has the three new pure virtual methods (`create_render_texture`, `create_frame_buffer`, `read_pixels(FrameBuffer&)`) and forward declaration for `FrameBuffer`.
- [ ] **DC-005**: `src/engine/render/render_device_opengl.h` and `.cpp` implement all three new methods. `create_render_texture` creates DSA textures with `GL_RGBA8`, `GL_LINEAR`, `GL_CLAMP_TO_EDGE`. `create_frame_buffer` delegates to `FrameBufferOpenGL::create()`. `read_pixels(FrameBuffer&)` binds the FBO, reads pixels, restores previous binding.
- [ ] **DC-006**: `src/engine/render/render_device_headless.h` and `.cpp` implement all three new methods as safe no-ops/error-returners. `read_pixels(FrameBuffer&)` returns `Unsupported` error.
- [ ] **DC-007**: `src/engine/render/render_system.h` has the `render_scene(FrameBuffer& target)` overload declaration.
- [ ] **DC-008**: `src/engine/render/render_system.cpp` implements `render_scene(FrameBuffer&)` as: `target.bind(); render_scene(); target.unbind();`
- [ ] **DC-009**: `tests/engine/render_device_fbo_test.cpp` exists with all 9 headless test cases, all passing.
- [ ] **DC-010**: `tests/engine/render_device_fbo_opengl_test.cpp` exists with the OpenGL integration test guarded by `#ifdef BUDDD_HAS_DISPLAY`.
- [ ] **DC-011**: All OpenGL integration test assertions (create FBO, verify AC-003 bind/unbind restoration via glGetIntegerv, render scene, read back pixels, verify non-black) pass when `BUDDD_HAS_DISPLAY=ON`.
- [ ] **DC-012**: Zero new compiler warnings: build with `-Wall -Wextra -Wpedantic` (or equivalent) produces no new warnings.
- [ ] **DC-013**: All existing tests pass (`ctest --preset debug` shows no regressions).
- [ ] **DC-014**: Logging follows spec observability table: `INFO` for FBO creation/destruction/resize/render texture creation, `DEBUG` for destruction, `TRACE` for bind/unbind, `ERROR` for incomplete FBO.
