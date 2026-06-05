# SPEC-017 — 2D Texture Support for the Render Pipeline

## Status

`Draft`

## Problem

The Buddd Engine render pipeline supports shaders, materials, vertex buffers, index buffers, and models — but has no way to load, create, or bind 2D textures on the GPU. This means:

- **No image data on the GPU**: PNG images can be loaded via `Image::load()` but there is no `Texture` abstraction, no `create_texture` factory on `RenderDevice`, and no OpenGL texture object lifecycle management.
- **No named texture binding on materials**: The `Material` API provides `set_uniform(name, value)` for scalar/matrix types but has no `set_texture(name, texture)` equivalent. Textures cannot be mapped to `uniform sampler2D` variables in shaders.
- **No sampler binding infrastructure**: There is no mechanism to assign texture units, bind textures to units, or set `sampler2D` uniforms to the correct unit index. Every frame would require manual OpenGL boilerplate.
- **No texture demo**: The existing cube demo uses per-vertex colours. There is no example of loading a PNG, creating a texture, and applying it to geometry.
- **`glUseProgram` ordering bug**: The `RenderSystem` sets uniforms via `set_uniform("u_mvp", mvp)` *before* `glUseProgram(program)` is called inside `draw()`, so `glUniform*` targets the wrong or no program. This bug must be fixed as part of adding texture support, because texture binding also depends on correct program activation ordering.

Without textures, the engine cannot represent surface appearance beyond vertex colours — a fundamental capability for any 3D engine.

## Goals

- **Texture abstraction**: Define an abstract `Texture` class representing a 2D image on the GPU, with concrete `TextureOpenGL` (wraps `GLuint`) and `TextureHeadless` (stores pixel data in memory) backends.
- **Texture creation from `Image`**: `RenderDevice::create_texture(const Image&)` creates a texture from a loaded image. Internally uploads to the GPU (OpenGL) or stores in memory (Headless).
- **Named texture binding on `Material`**: Add `set_texture(std::string_view name, std::shared_ptr<Texture>)` to the abstract `Material` API, consistent with the existing `set_uniform` API and compatible with `uniform sampler2D` declarations in GLSL.
- **Automatic texture unit management**: The `Material` implementation assigns texture units automatically and binds them before draw (via `glActiveTexture` + `glBindTexture` + `glUniform1i` for each sampler).
- **Fix `glUseProgram` ordering**: Introduce a `bind()` virtual method on `Material` that: (1) activates the shader program (`glUseProgram`), (2) applies all pending uniforms, (3) binds textures and sets sampler uniforms. The `RenderDevice::draw()` methods call `material.bind()` as their first operation, fixing the existing uniform-ordering bug.
- **PNG loading integration**: Load textures from PNG files via the existing `Image::load(path)`, then pass the `Image` directly to `create_texture`.
- **Channels support**: Support 1-channel (R), 3-channel (RGB), and 4-channel (RGBA) textures. All images are uploaded with appropriate OpenGL formats (`GL_RED`, `GL_RGB`, `GL_RGBA`).
- **Textured demo**: A new demo (`buddd demo textured-cube`) that loads a PNG texture from disk, applies it to a cube, and renders it with proper UV coordinates.

## Non-goals

- No cubemaps, texture arrays, 3D textures, or any texture type beyond `GL_TEXTURE_2D`.
- No mipmap generation or mipmap filtering (minification uses `GL_LINEAR`).
- No texture compression (no `GL_COMPRESSED_*` formats).
- No non-PNG image formats (PNG only, via existing `Image::load`).
- No `glTF`/asset material loading — textures are created programmatically from `Image` objects.
- No texture atlasing or texture coordinate transforms in the engine.
- No render-to-texture or framebuffer objects (FBO).
- No multiple texture units beyond what the sampler binding requires.
- No editor integration or texture preview.
- No deferred/optimised texture streaming — textures are fully uploaded at creation time.
- No shared texture registry or deduplication — textures are owned via `shared_ptr` and can be shared by convention.
- No texture parameter configuration beyond default linear filtering and clamp-to-edge wrapping (these are reasonable defaults for v1).
- No per-frame performance optimisation of texture binding (e.g., texture unit caching across draw calls).

## Actors

| Actor | Description |
|---|---|
| Engine developer | Creates textures, extends material API, writes rendering systems that use textures. Depends on abstract `Texture` and `Material::set_texture`. |
| Application developer | Loads PNG images, creates textures, assigns them to materials by name matching `uniform sampler2D` in GLSL shaders. Writes textured demos. |
| Build system | CMake + Ninja — new `.h` and `.cpp` files in `src/engine/render/` and `src/cmd/demo/` are picked up automatically by `file(GLOB_RECURSE)`. |
| Test suite | Catch2 v3 tests that verify texture creation, `set_texture`, headless texture state tracking, and the fix for the `glUseProgram` ordering bug — all in headless mode. |

## User-visible behavior

### 1. Texture class hierarchy

An abstract `Texture` class is defined in `src/engine/render/texture.h`:

```cpp
#pragma once

#include <cstddef>
#include <memory>

namespace buddd::engine {

class Texture {
public:
    virtual ~Texture() = default;

    virtual auto width() const noexcept -> int = 0;
    virtual auto height() const noexcept -> int = 0;
    virtual auto channels() const noexcept -> int = 0;

    Texture(const Texture&) = delete;
    auto operator=(const Texture&) -> Texture& = delete;
    Texture(Texture&&) = delete;
    auto operator=(Texture&&) -> Texture& = delete;

protected:
    Texture() = default;
};

} // namespace buddd::engine
```

Concrete backends:

- **`TextureOpenGL`** (`src/engine/render/texture_opengl.h`, private): wraps a `GLuint texture_` created via `glCreateTextures(GL_TEXTURE_2D, ...)`. Uploads pixel data via `glTextureStorage2D` + `glTextureSubImage2D` (DSA). Sets filtering to `GL_LINEAR` for both min and mag, wrapping to `GL_CLAMP_TO_EDGE` for both S and T. The destructor calls `glDeleteTextures`. Supports channels: 1 → `GL_RED`, 3 → `GL_RGB`, 4 → `GL_RGBA`.
- **`TextureHeadless`** (`src/engine/render/texture_headless.h`, private): stores width, height, channels, and a copy of the pixel data in a `std::vector<std::byte>`. All methods return stored values.

### 2. RenderDevice::create_texture

`RenderDevice` gains a new pure virtual factory method:

```cpp
[[nodiscard]] virtual auto create_texture(const Image& image)
    -> Result<std::unique_ptr<Texture>> = 0;
```

- Takes an `Image&` reference (which provides width, height, channels, and raw pixel data).
- Returns `Result<std::unique_ptr<Texture>>` — the caller owns the texture and can wrap it in `shared_ptr` when assigning to a material. This is consistent with all other `RenderDevice` factory methods.
- Returns an error if the image has zero width, zero height, empty data, or unsupported channel count.
- In the OpenGL backend: creates a `TextureOpenGL`, uploads to GPU, sets filtering/wrapping parameters.
- In the Headless backend: creates a `TextureHeadless`, stores pixel data in memory.

The OpenGL upload follows this sequence:

```cpp
GLuint tex;
glCreateTextures(GL_TEXTURE_2D, 1, &tex);

GLenum internal_format = (image.channels() == 4) ? GL_RGBA8 :
                         (image.channels() == 3) ? GL_RGB8 :
                         (image.channels() == 1) ? GL_R8 : GL_RGBA8; // fallback

GLenum format = (image.channels() == 4) ? GL_RGBA :
                (image.channels() == 3) ? GL_RGB :
                (image.channels() == 1) ? GL_RED : GL_RGBA;

glTextureStorage2D(tex, 1, internal_format, image.width(), image.height());
glTextureSubImage2D(tex, 0, 0, 0, image.width(), image.height(), format, GL_UNSIGNED_BYTE, image.data().data());

glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

### 3. Material API extension — set_texture

The abstract `Material` class gains new pure virtual methods:

```cpp
virtual auto set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> = 0;
virtual auto has_texture(std::string_view name) const -> bool = 0;
```

- `set_texture(name, texture)` maps a texture to a named sampler uniform in the shader. The `name` should correspond to a `uniform sampler2D name;` in GLSL.
- `texture` is `std::shared_ptr<Texture>` — ownership is shared. Passing `nullptr` for the texture pointer returns an error (`Error::Category::InvalidArgument`).
- Calling `set_texture` with a name that does not correspond to a known uniform location returns an error (`Error::Category::UniformNotFound`), consistent with `set_uniform` error semantics. The spec does not mandate GLSL type checking — `set_texture` checks only whether the uniform name exists (same as `has_uniform`).
- `has_texture(name)` returns `true` if a texture with that name has been set on this material.
- A material can hold 0 to N textures (one per unique name).

**OpenGL backend** (`MaterialOpenGL`):
- `set_texture` stores the `shared_ptr<Texture>` in an internal map (`std::unordered_map<std::string, std::shared_ptr<Texture>>`).
- The `has_texture(name)` check validates the name against the shader's uniform locations (same mechanism as `has_uniform` — checks `glGetUniformLocation != -1`), not against previously-set textures. This matches the `has_uniform` semantics: it checks if the uniform *exists in the shader*, not whether it has been set.

**Headless backend** (`MaterialHeadless`):
- `set_texture` stores the `shared_ptr<Texture>` in an internal `std::unordered_map<std::string, std::shared_ptr<Texture>>` and adds the name to `known_uniforms_`.
- `has_texture` checks `known_uniforms_` OR `texture_values_` for the name (same pattern as `has_uniform`).
- Provides a `get_texture(name)` public accessor returning `std::optional<std::shared_ptr<Texture>>` for test verification.

### 4. Material bind() — fixing the glUseProgram ordering bug

A new pure virtual method is added to the abstract `Material`:

```cpp
/// Applies all pending material state: activates the shader program,
/// sets cached uniforms, binds textures and sets sampler uniforms.
/// Called by RenderDevice::draw() at the start of every draw call.
/// The caller must ensure this is called within begin_frame()/end_frame().
virtual auto bind() const -> void = 0;
```

**OpenGL backend** (`MaterialOpenGL::bind()`):

```
1. glUseProgram(program_)
2. For each (name, value) in uniform_cache_:
      call glUniform*(location, value)  // location from cached getUniformLocation
3. For each (name, texture) in texture_cache_:
      a. unit = next_available_unit_++
      b. glActiveTexture(GL_TEXTURE0 + unit)
      c. glBindTexture(GL_TEXTURE_2D, static_cast<TextureOpenGL*>(texture.get())->handle())
      d. glUniform1i(get_uniform_location(name), unit)
4. Reset next_available_unit_ to 0
```

- `bind()` is declared `const` because it only modifies GPU state, not the material object. The internal `next_available_unit_` counter is marked `mutable` to allow this.
- Texture units are assigned sequentially per `bind()` call, starting from unit 0.
- The `next_available_unit_` counter is reset to 0 after each `bind()` call, so the next draw call starts fresh.
- This is a per-material, per-draw-call assignment. If a material has 2 textures, they get units 0 and 1. If another material with 1 texture is drawn next, that texture gets unit 0.

**Headless backend** (`MaterialHeadless::bind()`):
- No-op (`const`). The headless backend does not interact with GPU state.

**Integration into RenderDevice::draw():**

Both `RenderDeviceOpenGL::draw()` and `RenderDeviceOpenGL::draw_indexed()` call `material.bind()` as their first operation, before setting up vertex state and issuing the draw command:

```cpp
auto RenderDeviceOpenGL::draw(...) -> void {
    auto& mat = static_cast<const MaterialOpenGL&>(material);
    auto& vb = static_cast<const VertexBufferOpenGL&>(vertices);

    mat.bind();                          // NEW: glUseProgram + uniforms + textures
    glBindVertexArray(vb.vao());
    glDrawArrays(...);
}
```

This means:
- The existing `cube_demo.cpp` pattern (`set_uniform("u_mvp", mvp)` followed by `model.draw(device)`) is automatically fixed — `set_uniform` caches the value, and `draw()` → `bind()` → `glUseProgram` → `glUniform*` applies it to the correct program.
- The `RenderSystem::render()` pattern is also fixed by the same mechanism.
- **`set_uniform` no longer calls `glUniform*` immediately.** All current `set_uniform` calls cache the value in an internal `std::unordered_map<std::string, UniformValue>` variant map. The actual `glUniform*` call is deferred to `bind()`. This is a behavioural change but does not break any existing caller — all callers follow the `set_uniform` → `draw` pattern.

**Uniform value storage for deferred application:**

`MaterialOpenGL` gains a variant-based uniform value cache:

```cpp
// New member in MaterialOpenGL:
std::unordered_map<std::string, std::variant<float, int32_t, bool, math::Vec3, math::Vec4, math::Mat4>> uniform_cache_;
```

Each `set_uniform` overload stores the value in `uniform_cache_` (keyed by name). The `get_uniform_location` caching remains unchanged. The `bind()` method iterates `uniform_cache_` and issues `glUniform*` calls.

**Important**: `has_uniform` continues to check via `glGetUniformLocation` (not the cache), so its semantics are unchanged.

### 5. RenderDevice factory method signature changes

`RenderDevice` gains:

```cpp
[[nodiscard]] virtual auto create_texture(const Image& image)
    -> Result<std::unique_ptr<Texture>> = 0;
```

### 6. RenderSystem integration

The `RenderSystem` does not need changes for basic texture support. Textures are set on materials at creation time (before the render loop), and the `set_texture` / `set_uniform` / `bind()` flow handles everything:

```cpp
// Demo flow:
auto image = Image::load("brick.png");           // existing API, returns Result<Image>
auto texture = device.create_texture(*image);    // new API: returns Result<unique_ptr<Texture>>
material->set_texture("u_tex",                   // new API: Material takes shared_ptr<Texture>
    std::shared_ptr<Texture>(std::move(*texture)));  // wrap unique_ptr -> shared_ptr
// ... then render loop as before:
//   material->set_uniform("u_mvp", mvp);  // caches value
//   model.draw(device);                    // draw() → bind() → glUseProgram + apply all
```

### 7. Texture data flow

```
PNG file on disk
    ↓ Image::load(path)         [src/engine/image/ — stb_image]
Image (width, height, channels, pixel data)
    ↓ RenderDevice::create_texture(image)
        OpenGL:              Headless:
        glCreateTextures     Store copy of pixel data
        glTextureStorage2D   Store width/height/channels
        glTextureSubImage2D
        glTextureParameteri
    ↓
unique_ptr<Texture>
    ↓ (wrap in shared_ptr) std::shared_ptr<Texture>(std::move(unique_ptr))
shared_ptr<Texture>
    ↓ Material::set_texture(name, texture)
Material stores shared_ptr<Texture> keyed by name
    ↓ (during draw) Material::bind()
OpenGL:
  glUseProgram(program)
  for each cached uniform: glUniform*(loc, value)
  for each texture:
    glActiveTexture(GL_TEXTURE0 + unit)
    glBindTexture(GL_TEXTURE_2D, handle)
    glUniform1i(sampler_loc, unit)
  glDrawArrays / glDrawElements
```

### 8. Textured cube demo

A new demo `buddd demo textured-cube` is added alongside the existing demos:

```
src/cmd/demo/textured_cube_demo.h
src/cmd/demo/textured_cube_demo.cpp
```

The demo:
1. Loads a PNG texture file from a known path (e.g., `assets/brick.png` or a built-in fallback).
2. Creates a cube Model with UV coordinates (Float2 at layout location 2) in addition to position (Float3, loc 0) and colour (Float3, loc 1) or replacing colour.
3. Creates a Material with a vertex shader that passes UVs through and a fragment shader that samples `u_tex`.
4. Sets the texture on the material via `material->set_texture("u_tex", texture)`.
5. Uses the scene graph (`World` + `Entity` + `MeshRenderer` + `RenderSystem`) — creates a cube entity, attaches a `MeshRenderer` with the material and mesh, and lets the `RenderSystem` drive rendering.
6. Runs a 120-frame render loop with rotation on the Y axis, calling `world.update()` each frame (same pattern as the existing scene-graph demos).

The demo expects the texture file at a hardcoded relative path `assets/brick.png`. If the file cannot be loaded, the demo prints a fatal error and exits.

The UV data for the cube uses standard per-face UV mapping (covering [0,1] × [0,1] per face).

The demo uses a minimal vertex format: position + texcoord only (the existing cube already tests normals elsewhere):

```cpp
struct TexturedCubeVertex {
    float px, py, pz;   // position (Float3, loc 0)
    float u, v;         // texcoord (Float2, loc 1)
};
```

With 24 vertices and 36 indices (same as the existing cube, but with UVs per vertex).

## User stories

### Story 1 — Create a texture from a loaded PNG image (Priority: P1)

As an application developer, I want to load a PNG file from disk, create a GPU texture from it, and use that texture in a material, so that my 3D objects can display images.

**Given** a valid PNG file at `assets/brick.png`

**When** I call:
```
auto image = Image::load("assets/brick.png");
auto texture = device.create_texture(*image);
```

**Then** a `unique_ptr<Texture>` is returned with width/height/channels matching the PNG file. In the OpenGL backend, a `GL_TEXTURE_2D` object is created on the GPU with the correct pixel data.

**Given** the headless backend

**When** I call the same sequence

**Then** a `unique_ptr<TextureHeadless>` is returned with the pixel data stored in memory, matching the loaded image dimensions and content.

### Story 2 — Assign a texture to a material by name (Priority: P1)

As an application developer, I want to assign a texture to a named sampler uniform in my shader, so that the shader can sample it during rendering.

**Given** a `Material` with a GLSL declaration `uniform sampler2D u_tex;` and a `shared_ptr<Texture>`

**When** I call `material->set_texture("u_tex", texture)`

**Then** the call succeeds. When `bind()` is called during `draw()`, the texture is bound to a texture unit and `u_tex` sampler is set to that unit.

**Given** the same material without a `uniform sampler2D u_tex;` declaration (i.e., no uniform named `"u_nonexistent"`)

**When** I call `material->set_texture("u_nonexistent", texture)`

**Then** the call returns an error (`Error::Category::UniformNotFound`), matching `set_uniform` error semantics.

### Story 3 — Fix the glUseProgram ordering (Priority: P1)

As an engine developer, I want `set_uniform` and `set_texture` to work correctly regardless of the current GL program state, so that the RenderSystem can set uniforms without worrying about program activation order.

**Given** a render loop that calls `material->set_uniform("u_mvp", mvp)` and then `model.draw(device)`

**When** the draw call executes

**Then** the uniform `u_mvp` is correctly applied to the shader program owned by that material. The OpenGL backend calls `glUseProgram` before `glUniform*`, ensuring the uniform targets the correct program.

### Story 4 — Render a textured object (Priority: P1)

As an application developer, I want to render a cube with a texture applied, using UV coordinates per vertex and a `sampler2D` in the fragment shader.

**Given** a textured cube demo with a valid texture, vertex data including UVs, a material with `u_tex` sampler, and a shader that samples the texture

**When** the render loop runs

**Then** the cube appears on screen with the texture mapped onto each face according to the per-vertex UV coordinates.

### Story 5 — Texture creation errors are propagated (Priority: P2)

As an engine developer, I want to receive a meaningful error when texture creation fails (invalid image, unsupported format), so that I can diagnose and fix the issue.

**Given** an `Image` with zero width

**When** I call `device.create_texture(bad_image)`

**Then** the factory returns an error with `Error::Category::InvalidArgument`.

**Given** an `Image` with unsupported channel count (2 channels, or more than 4)

**When** I call `device.create_texture(bad_image)`

**Then** the factory returns an error with `Error::Category::Unsupported` (or `InvalidArgument`) indicating the channel count is not supported.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | An abstract `Texture` class exists in `src/engine/render/texture.h` in namespace `buddd::engine` with a virtual destructor and pure virtual `width()`, `height()`, `channels()` returning `int`. Non-copyable, non-movable. | File compiles; class is abstract with at least three pure virtual methods; destructor is `virtual`; copy/move operations are deleted. |
| AC-002 | Concrete `TextureOpenGL` class exists in a private header, implementing `Texture`. Wraps a `GLuint` texture handle. Destructor calls `glDeleteTextures`. | File compiles; `TextureOpenGL` is a valid concrete class. Grep confirms destructor calls `glDeleteTextures`. |
| AC-003 | Concrete `TextureHeadless` class exists in a private header, implementing `Texture`. Stores width, height, channels, and pixel data in a `std::vector<std::byte>`. Does not include `<SDL3/` or `<GL/` headers. | File compiles; `TextureHeadless` returns correct dimensions from stored values. Grep of headless file returns no SDL3 or GL includes. |
| AC-004 | `RenderDevice` gains pure virtual factory method `create_texture(const Image&) -> Result<std::unique_ptr<Texture>>`. | `render_device.h` compiles with the new method; signature matches the spec. |
| AC-005 | `RenderDeviceOpenGL::create_texture` creates a `GL_TEXTURE_2D` with `glCreateTextures`, uploads data via `glTextureStorage2D` + `glTextureSubImage2D`, sets `GL_LINEAR` min/mag filters and `GL_CLAMP_TO_EDGE` wrapping. Supports 1, 3, and 4 channel images. Extracts pixel data from `Image::data()`. Returns `unique_ptr<Texture>`. | Code review confirms the OpenGL texture creation sequence. Unit test: create texture from 4-channel RGBA image loaded via `Image::load()`, verify no GL error. |
| AC-006 | `RenderDeviceHeadless::create_texture` creates a `TextureHeadless` storing the image data in memory. Returns `unique_ptr<Texture>`. | Unit test: create a test `Image` with known dimensions and data, pass to `create_texture`, verify `texture->width()`, `height()`, `channels()` match. |
| AC-007 | `Material` gains pure virtual methods: `set_texture(std::string_view name, std::shared_ptr<Texture>) -> Result<void>` and `has_texture(std::string_view name) -> bool`. | `material.h` compiles with the two new methods. |
| AC-008 | `MaterialOpenGL::set_texture` stores the texture in an internal map. `MaterialOpenGL::has_texture` returns `true` for names corresponding to valid GL uniform locations (same as `has_uniform` — type is not validated). | Unit test (headless equivalent): create material with known uniforms including `u_tex`, call `set_texture("u_tex", tex)`, verify success. Call `has_texture("u_tex")` — returns `true` (the name is in known_uniforms). |
| AC-009 | `MaterialHeadless::set_texture` stores the texture in an internal map. `MaterialHeadless::has_texture` returns `true` for names that are known uniformly or were set via `set_texture`. `MaterialHeadless` provides `get_texture(name) -> std::optional<std::shared_ptr<Texture>>` for test verification. | Unit test: create material, set texture, verify `get_texture` returns the stored shared_ptr. |
| AC-010 | `Material` gains a pure virtual `bind() const -> void` method. | `material.h` compiles with `bind()` as a pure virtual const method. |
| AC-011 | `MaterialOpenGL::bind()` (const) calls `glUseProgram(program_)`, then applies all cached uniforms (via `glUniform*`), then binds all textures (assigns units sequentially, calls `glActiveTexture` + `glBindTexture` + `glUniform1i` for each sampler). Resets the mutable unit counter to 0 after binding. | Code review confirms the binding sequence. |
| AC-012 | `MaterialHeadless::bind()` is a const no-op. | Unit test: call `bind()` on headless material — no crash, no state change. |
| AC-013 | `MaterialOpenGL::set_uniform` no longer calls `glUniform*` immediately. Instead it caches the value. The actual `glUniform*` call is deferred to `bind()`. | Code review: `set_uniform` stores value in `uniform_cache_` and does not call any `glUniform*` function. `bind()` iterates the cache and calls `glUniform*`. |
| AC-014 | `RenderDeviceOpenGL::draw()` and `draw_indexed()` call `material.bind()` as their first operation, before any vertex/index buffer binding or draw command. | Code review confirms `material.bind()` is called at the top of both `draw()` and `draw_indexed()`. |
| AC-015 | `MaterialOpenGL::set_uniform` with a name not found in the shader still returns `UniformNotFound` error (location lookup still happens for validation). | Unit test (headless): set_uniform with unknown name returns `UniformNotFound`. |
| AC-016 | `MaterialOpenGL::set_texture` with a null `shared_ptr<Texture>` returns `Error::Category::InvalidArgument`. | Unit test (headless): set_texture with `nullptr` returns `InvalidArgument`. |
| AC-017 | `create_texture` with zero-width, zero-height, or empty data returns `Error::Category::InvalidArgument`. | Unit test (headless): construct an `Image` with zero width/height, call `create_texture` → error. Construct an `Image` with empty data → error. |
| AC-018 | `create_texture` with unsupported channel count (2, or >4) returns an error. | Unit test (headless): construct an `Image` with 2 channels, call `create_texture` → error. |
| AC-019 | A textured cube demo compiles and runs under the OpenGL backend. The demo loads a PNG file, creates a texture, creates a cube with UV coordinates, and renders it. | The demo `buddd demo textured-cube` runs for 120 frames without crashing. Visual output shows a textured rotating cube. |
| AC-020 | Headless backend tests cover: texture creation, `set_texture`/`has_texture`, `bind()` no-op, and error paths. All pass without a GPU. | `ctest --preset debug` passes all texture tests in headless mode. |
| AC-021 | All four new abstract headers (`texture.h`) expose no external library types in their public interface. | `grep -E '(SDL_|gl[A-Z]|GL_|GLAD|stb_)' src/engine/render/texture.h` returns no matches. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An application developer can load a PNG, create a texture, assign it to a material, and draw a textured mesh in fewer than 20 lines of C++ (excluding shader source and boilerplate). | Count lines in a minimal textured demo program. |
| SC-002 | All texture-related functionality is testable in headless mode without a GPU or display server. | `ctest --preset debug` on a headless CI runner passes all texture tests. |
| SC-003 | The `glUseProgram` ordering bug is fixed: after implementation, any call to `set_uniform(name, value)` followed by `model.draw(device)` correctly applies uniforms to the owning material's program. | OpenGL backend: `render_device_tests` with offscreen OpenGL context verifies uniform values via a test shader that writes the uniform value to a colour output (readback via `read_pixels`). Headless: `MaterialHeadless` stores the value; `bind()` is a no-op; the value is retrievable via `get_uniform_mat4()`. |
| SC-004 | The `Texture` abstraction adds no link-time dependency to `buddd_engine` beyond what the render pipeline already requires. | Build succeeds with `BUDDD_HAS_DISPLAY=OFF`; headless-only texture tests pass. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| `create_texture` with a 1-channel (grayscale) image | OpenGL backend creates `GL_R8` texture, uploads with `GL_RED` format. Texture appears grayscale when sampled (all RGB components read from R channel). |
| `create_texture` with a 3-channel (RGB) image | OpenGL backend creates `GL_RGB8` texture, uploads with `GL_RGB` format. |
| `create_texture` with a 4-channel (RGBA) image | OpenGL backend creates `GL_RGBA8` texture, uploads with `GL_RGBA` format. Alpha channel is available in shader. |
| `create_texture` with a 2-channel image (e.g., `GL_RG`) | Error returned — 2-channel textures are not supported in v1. |
| `create_texture` with channel count > 4 | Error returned — unsupported format. |
| `set_texture` with a name that exists as a `uniform float` (not `sampler2D`) in the shader | The `set_texture` call succeeds (the name exists as a uniform location — type is not validated). The subsequent `bind()` call will call `glUniform1i(sampler_location, unit)` on a float uniform. In OpenGL this is defined behaviour (the float uniform gets set to an integer). Same policy as `set_uniform` type mismatches. |
| `set_texture` called after `bind()` (i.e., during a frame, before the next draw call) | The texture is stored in the cache and will be bound on the next `bind()` call (next draw call). Legal usage. |
| Same texture set twice on the same material (same name) | The second call overwrites the first. No error. |
| Same texture `shared_ptr` used on multiple materials | Legal and expected — `shared_ptr` allows shared ownership. Each material binds the texture independently during its own `bind()` call. |
| `bind()` called when no textures are set | No texture operations occur. The program is activated and cached uniforms are applied. Normal case for non-textured materials. |
| `bind()` called when no uniforms have been set (empty uniform cache) | No `glUniform*` calls occur. Program is activated, textures are bound, draw proceeds. Normal. |
| Material with both uniforms and textures | Uniforms are applied first, then textures (order within each group is implementation-defined but deterministic). All state is ready for the draw command. |
| `has_texture(name)` on a name that does not exist in the shader | Returns `false` (headless: name not in known_uniforms; OpenGL: `glGetUniformLocation` returns -1). |
| `bind()` called outside `begin_frame()`/`end_frame()` | Undefined behaviour at the abstract level. The OpenGL backend will call `glUseProgram` which is valid outside a frame, but the draw call that follows would be outside the frame boundary. The spec does not mandate guarding. |
| Multiple draw calls with different materials (each with different textures) each frame | Each `draw()` calls `bind()` on its material, activating the correct program and binding the correct textures. Texture units are reassigned per `bind()` call. Legal and expected. |
| `bind()` called twice consecutively (e.g., two draw calls with the same material) | The second `bind()` re-activates the program, re-applies uniforms, and re-binds textures. This is correct (uniform values may have changed between draw calls) but slightly wasteful. Callers should batch when possible, but correctness is guaranteed. |
| Texture created from an `Image` object that has been moved-from or destroyed | `create_texture` receives a `const Image&` reference. The caller must ensure the `Image` outlives the `create_texture` call. Internally, the backend copies the pixel data during creation, so the texture does not hold a reference to the `Image`. |
| `Image::data()` returns empty but width/height are positive | `create_texture` validates: if `data().size() != static_cast<size_t>(width() * height() * channels())`, return `InvalidArgument`. |
| `Image` with negative width or height | Defined as `int` but physically nonsensical. `create_texture` validates `width() > 0 && height() > 0`, returns `InvalidArgument` for zero or negative. |

## Error cases

| Case | Expected behaviour |
|---|---|
| `create_texture` with zero width/height | `make_error(Error::Category::InvalidArgument, "Image dimensions must be positive")` |
| `create_texture` with empty data vector | `make_error(Error::Category::InvalidArgument, "Image data is empty")` |
| `create_texture` with unsupported channel count (2, or >4) | `make_error(Error::Category::Unsupported, "Unsupported channel count: N")` |
| `create_texture` with data size mismatch (image.data().size() != image.width() * image.height() * image.channels()) | `make_error(Error::Category::InvalidArgument, "Image data size mismatch")` |
| `set_texture` with null `shared_ptr<Texture>` | `make_error(Error::Category::InvalidArgument, "Texture pointer is null")` |
| `set_texture` with name not found in shader | `make_error(Error::Category::UniformNotFound, "Uniform 'name' not found")` |
| `set_texture(/set_uniform)` on a moved-from or destroyed material | Undefined behaviour (existing precondition). |
| OpenGL texture creation fails (e.g., out of GPU memory) | `create_texture` returns `make_error(Error::Category::ResourceCreationFailed, ...)`. Backend should detect GL errors after `glCreateTextures`. |
| PNG file not found at the given path | Handled by `Image::load()` which returns `Error::Category::IoFailed`. The `Image::load` error propagates to the caller before `create_texture` is attempted. |
| `bind()` called on a `MaterialOpenGL` whose shader program has been deleted | Undefined behaviour (the program is owned by the material and deleted only in its destructor; calling `bind()` after destruction is UB). |

## Permissions and security

- No elevated privileges required to create textures or load PNG files.
- Texture data is loaded from disk by the application via `Image::load()` — the engine does not initiate file I/O for textures outside that API.
- No network access, secrets, or credentials are involved.
- The headless backend requires no GPU or display access, maintaining CI safety.
- The architecture boundary (CONST-001) remains in full effect: no code outside `src/engine/` may include OpenGL, SDL3, or GLM headers. The new `texture.h` exposes no backend types.
- OpenGL backend implementations live inside `src/engine/render/` and are invisible to external consumers.
- PNG parsing via stb_image is already used and trusted by the existing `Image` class. No new image parsing dependencies are introduced.

## Observability

All observability uses `std::cerr` consistent with the project pattern.

| Signal | Source |
|---|---|
| Texture creation success/failure | `std::cerr << "Texture created (" << w << "x" << h << ", " << channels << " channels)\n"` or `std::cerr << "Texture creation failed: " << error << "\n"` |
| Texture binding during `bind()` | `std::cerr << "Bind texture: " << name << " (unit=" << unit << ")\n"` (debug builds only) |
| Texture unit assignments | `std::cerr << "Active texture unit " << unit << " for " << name << "\n"` (debug builds only) |
| `set_texture` success | `std::cerr << "Texture set: " << name << "\n"` (debug builds only) |
| `set_texture` error (name not found) | `std::cerr << "Texture not found: " << name << "\n"` on error return |
| `bind()` — program activation | `std::cerr << "Material bind: program " << program << "\n"` (debug builds only) |
| `bind()` — uniform application | Existing uniform log messages (debug builds only) |

## Out of scope

- Cubemaps, texture arrays, 3D textures, buffer textures.
- Mipmap generation, mipmap filtering, or any `glGenerateMipmap` call.
- Texture compression (`GL_COMPRESSED_RGBA`, `GL_COMPRESSED_RGB_S3TC`, `ASTC`, `ETC2`, etc.).
- Non-PNG image formats (JPEG, BMP, TGA, etc. — use stb_image's other decoders externally or in a future spec).
- glTF material import or asset pipeline integration.
- Framebuffer objects (FBO), render-to-texture, or `glReadPixels` into textures.
- Texture atlasing, texture coordinate manipulation, or texture transform matrices.
- Deduplicated texture registry or reference-counted texture manager.
- Anisotropic filtering, mipmap LOD bias, or sampler objects (`GLSampler`).
- Per-fragment or per-pixel texture lookup beyond basic `sampler2D`.
- Shader reflection for automatic sampler discovery (uniform names are manual).
- Thread-safe texture creation or concurrent texture binding.
- Dynamic texture updates (e.g., `glTextureSubImage2D` after creation for video textures).
- Editor integration, texture preview, or drag-and-drop texture assignment.
- Performance optimisation of texture unit allocation (e.g., caching unit assignments across draw calls).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `Image` class provides public accessors: `width()`, `height()`, `channels()` returning `int`, and `data()` returning `const std::vector<std::byte>&`. These are used by `create_texture` to extract pixel data. |
| A-02 | The OpenGL backend uses DSA (Direct State Access) APIs: `glCreateTextures`, `glTextureStorage2D`, `glTextureSubImage2D`, `glTextureParameteri`. These require OpenGL 4.5 Core, which the project already uses (see SPEC-005 A-01). |
| A-03 | GLSL version is `#version 450 core`. Sampler uniforms are declared as `uniform sampler2D u_name;` in fragment shaders. |
| A-04 | Maximum supported texture dimensions are implementation-defined (OpenGL `GL_MAX_TEXTURE_SIZE`). The engine does not validate texture size against this limit before creation. If the image exceeds the GPU's capability, `glTextureStorage2D` will generate a `GL_INVALID_VALUE` error which the backend may detect and propagate. |
| A-05 | Texture unit count is assumed to be at least 16 (GL spec minimum for GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is 48 for OpenGL 4.5). The engine does not track or limit the number of textures per material. |
| A-06 | `Error::Category` may gain new values: `TextureCreationFailed` (or reuse `ResourceCreationFailed` if already present). The choice is left to the implementation. |
| A-07 | New files to be created:
- `src/engine/render/texture.h` — abstract Texture
- `src/engine/render/texture_opengl.h` — TextureOpenGL
- `src/engine/render/texture_opengl.cpp`
- `src/engine/render/texture_headless.h` — TextureHeadless
- `src/engine/render/texture_headless.cpp`
- `src/cmd/demo/textured_cube_demo.h`
- `src/cmd/demo/textured_cube_demo.cpp`
Modified files:
- `src/engine/render/render_device.h` — add `create_texture`
- `src/engine/render/render_device_opengl.h/cpp` — implement `create_texture`
- `src/engine/render/render_device_headless.h/cpp` — implement `create_texture`
- `src/engine/render/material.h` — add `set_texture`, `has_texture`, `bind`
- `src/engine/render/material_opengl.h/cpp` — implement `set_texture`, `has_texture`, `bind` with deferred uniform application
- `src/engine/render/material_headless.h/cpp` — implement `set_texture`, `has_texture`, `bind` (no-op) + `get_texture`
- `src/engine/render/render_device_opengl.cpp` — call `material.bind()` in `draw()`/`draw_indexed()`
- `src/cmd/commands/demo_command.cpp` — register `"textured-cube"` command |
| A-08 | The `MaterialOpenGL` gains a `bool dirty_` flag or uses presence in `uniform_cache_` to track which uniforms need applying during `bind()`. For v1, `bind()` applies all cached uniforms unconditionally (no dirty tracking). This is simpler and correct. |
| A-09 | Texture unit management: `MaterialOpenGL::bind()` assigns units starting from 0 for the first texture, 1 for the second, etc., and resets to 0 after `bind()` returns. This means every `bind()` call reassigns units. This is correct for per-draw-call binding but means that if two materials are drawn with the same texture in the same frame, that texture is bound twice (potentially to different units). This is acceptable for v1. |
| A-10 | The `set_uniform` caching change is backward-compatible for all existing callers. All existing callers (RenderSystem, cube_demo, triangle_demo) follow the `set_uniform` → `draw()` sequence. There are no callers that depend on immediate `glUniform*` side effects (e.g., reading back uniform state between `set_uniform` and `draw()`). |
| A-11 | The `Image::load()` method returns `Result<Image>`. The `Image` class provides `width()`, `height()`, `channels()`, and `data()` accessors that `create_texture` uses to read pixel data. |
| A-12 | Tests for SPEC-017 acceptance criteria live in a new file `tests/texture_tests.cpp`, following the project convention. |
| A-13 | The textured cube demo uses the scene graph (`World` + `Entity` + `MeshRenderer` + `RenderSystem`) pattern for consistency with the scene-graph demo style. |

## Open questions

| ID | Question | Resolution |
|---|---|---|---|
| Q-01 | The existing `Image` class stores data in `std::vector<std::byte>`. Should `create_texture` accept `const Image&` or `const ImageBuffer&`? | **Resolved**: `create_texture` accepts `const Image&`. The `Image` class provides `width()`, `height()`, `channels()`, and `data()` accessors. |
| Q-02 | Should the textured cube demo use the scene graph or a standalone pattern? | **Resolved**: The demo uses the scene graph (`World` + `Entity` + `MeshRenderer` + `RenderSystem`). |
| Q-03 | The `set_uniform` change from immediate `glUniform*` to deferred caching — are there any existing tests or code that depend on the immediate behaviour? | **Resolved**: No existing code depends on immediate `glUniform*` behaviour. The deferred `bind()` approach is confirmed. |
