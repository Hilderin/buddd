# IMPL-017-001 — 2D Texture Support for the Render Pipeline

## Source spec

`.specs/sprint-2026-05/texture-support/spec.md` (SPEC-017, accepted)

## Goal

Add a 2D texture abstraction to the Buddd Engine render pipeline: define an abstract `Texture` class with `TextureOpenGL` (DSA-based GPU upload) and `TextureHeadless` (in-memory pixel storage) backends; add `create_texture(const Image&)` factory to `RenderDevice`; extend `Material` with `set_texture(name, shared_ptr<Texture>)` for named sampler binding; implement `Material::bind()` to activate the program, apply cached uniforms, and bind textures with automatic texture-unit assignment; fix the `glUseProgram` ordering bug by deferring all `glUniform*` calls to `bind()`; and provide a `textured-cube` demo using the scene graph.

## Non-goals

- No cubemaps, texture arrays, 3D textures, or buffer textures.
- No mipmap generation or mipmap filtering (`GL_LINEAR` for both min and mag).
- No texture compression (`GL_COMPRESSED_*` formats).
- No non-PNG image formats.
- No glTF/asset pipeline material loading.
- No texture atlasing, texture coordinate transforms, or texture transform matrices.
- No FBO/render-to-texture or `glReadPixels` into textures.
- No shader reflection — `set_texture` checks only uniform name existence, not GLSL type.
- No shared texture registry or deduplication.
- No per-frame texture unit caching optimisations.
- No dirty-tracking optimisation for uniform application — `bind()` applies all cached uniforms unconditionally.
- No dynamic texture updates after creation (`glTextureSubImage2D` only at create time).
- No anisotropic filtering or sampler objects.
- No thread safety guarantees for texture creation or binding.
- No editor integration or texture preview.
- No new dependencies beyond what the engine already uses (stb_image via existing `Image` class).

## Relevant constitution rules

- **CONST-001-architecture-boundaries**: No code outside `src/engine/` may include OpenGL, SDL3, or GLM headers. The new `texture.h` public header must expose no backend types. `TextureOpenGL` is a private header at `src/engine/render/texture_opengl.h`; `TextureHeadless` is a private header at `src/engine/render/texture_headless.h`.

## Relevant ADRs

- **ADR-001**: All fallible public API functions return `Result<T>`. `create_texture` returns `Result<std::unique_ptr<Texture>>`. `set_texture` returns `Result<void>`. Draw methods (`draw`, `draw_indexed`) remain `void` per ADR-003.
- **ADR-003**: Draw methods return `void` (precondition-based contract). New `bind()` method on `Material` is also `void` (it is a GPU state command, not fallible — headless backend implements it as no-op). This is consistent with ADR-003's principle for non-fallible rendering commands.

## Files to inspect

The Code Agent MUST read these files before making any changes:

1. `src/engine/render/material.h` — Abstract Material base class (add `set_texture`, `has_texture`, `bind` pure virtual methods)
2. `src/engine/render/material_opengl.h` — MaterialOpenGL header (add texture map, uniform cache, `bind()`, mutable unit counter)
3. `src/engine/render/material_opengl.cpp` — MaterialOpenGL implementation (rewrite `set_uniform` to defer, add `set_texture`, `bind()`)
4. `src/engine/render/material_headless.h` — MaterialHeadless header (add texture map, `get_texture`, `bind()`)
5. `src/engine/render/material_headless.cpp` — MaterialHeadless implementation (add `set_texture`, `has_texture`, `bind()`, `get_texture()`)
6. `src/engine/render/render_device.h` — Abstract RenderDevice (add `create_texture` pure virtual)
7. `src/engine/render/render_device_opengl.h` — RenderDeviceOpenGL header (add `create_texture` override)
8. `src/engine/render/render_device_opengl.cpp` — RenderDeviceOpenGL implementation (add `create_texture`, move `glUseProgram` into `material.bind()` calls in `draw()`/`draw_indexed()`)
9. `src/engine/render/render_device_headless.h` — RenderDeviceHeadless header (add `create_texture` override)
10. `src/engine/render/render_device_headless.cpp` — RenderDeviceHeadless implementation (add `create_texture`)
11. `src/engine/error.h` — Error categories (verify `ResourceCreationFailed` and `Unsupported` exist, add `TextureCreationFailed` or reuse existing)
12. `src/engine/image/image.h` — Image class (understand `width()`, `height()`, `channels()`, `data()` accessors)
13. `src/engine/render/vertex_format.h` — Vertex format types for demo vertex definition
14. `src/engine/render/model.h` + `src/engine/render/model.cpp` — Model class (understand `draw()` flow)
15. `src/engine/render/render_system.h` + `src/engine/render/render_system.cpp` — RenderSystem (understand `set_uniform` + `draw()` pattern that the `bind()` fix addresses)
16. `src/cmd/demo/cube_scene_demo.h` + `src/cmd/demo/cube_scene_demo.cpp` — Scene graph demo pattern to follow for `textured_cube_demo`
17. `src/cmd/demo/demo_helpers.h` — Demo helper types (CubeResources, setup_cube)
18. `src/cmd/commands/demo_command.cpp` — Demo command registration (must add `textured-cube` to the list)
19. `src/engine/engine_service.h` — EngineService (test setup pattern)
20. `tests/scene_rendering_tests.cpp` — Test patterns for headless tests
21. `tests/render_device_tests.cpp` — Test patterns

## Files allowed to change

### New files to create

1. `src/engine/render/texture.h` — Abstract `Texture` class (public API)
2. `src/engine/render/texture_opengl.h` — `TextureOpenGL` (private header, no include outside `src/engine/render/`)
3. `src/engine/render/texture_opengl.cpp` — `TextureOpenGL` implementation
4. `src/engine/render/texture_headless.h` — `TextureHeadless` (private header)
5. `src/engine/render/texture_headless.cpp` — `TextureHeadless` implementation
6. `src/cmd/demo/textured_cube_demo.h` — Textured cube demo header
7. `src/cmd/demo/textured_cube_demo.cpp` — Textured cube demo implementation
8. `tests/texture_tests.cpp` — All texture-related tests

### Existing files to modify

1. `src/engine/render/render_device.h` — Add `create_texture(const Image&) -> Result<std::unique_ptr<Texture>>` pure virtual; add `#include "image/image.h"` or forward-declare `Image`.
2. `src/engine/render/render_device_opengl.h` — Add `create_texture` override declaration.
3. `src/engine/render/render_device_opengl.cpp` — Add `create_texture` implementation with OpenGL DSA upload; change `draw()` and `draw_indexed()` to call `mat.bind()` as first operation (replacing direct `glUseProgram` calls).
4. `src/engine/render/render_device_headless.h` — Add `create_texture` override declaration.
5. `src/engine/render/render_device_headless.cpp` — Add `create_texture` implementation returning `TextureHeadless`; change `draw()` and `draw_indexed()` to call `material.bind()` as first operation.
6. `src/engine/render/material.h` — Add pure virtual `set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void>`, `has_texture(std::string_view name) const -> bool`, `bind() const -> void`; add forward declaration `class Texture;`.
7. `src/engine/render/material_opengl.h` — Add `set_texture`, `has_texture`, `bind` overrides; add `uniform_cache_` member (`std::unordered_map<std::string, std::variant<float, int32_t, bool, math::Vec3, math::Vec4, math::Mat4>>`); add `texture_map_` member (`std::unordered_map<std::string, std::shared_ptr<Texture>>`); add `mutable int next_unit_{0}` for texture unit management.
8. `src/engine/render/material_opengl.cpp` — Rewrite all `set_uniform` overloads to cache values in `uniform_cache_` instead of calling `glUniform*` immediately (must still validate uniform existence via `get_uniform_location()` and return `UniformNotFound` error if location is -1); implement `set_texture` (store in `texture_map_`, validate name via `get_uniform_location()`); implement `has_texture` (check via `glGetUniformLocation`); implement `bind()` (sequence: `glUseProgram`, apply cached uniforms, bind textures with unit assignment, reset `next_unit_`).
9. `src/engine/render/material_headless.h` — Add `set_texture`, `has_texture`, `bind` overrides; add `get_texture(std::string_view name) const -> std::optional<std::shared_ptr<Texture>>`; add `texture_values_` member (`std::unordered_map<std::string, std::shared_ptr<Texture>>`).
10. `src/engine/render/material_headless.cpp` — Implement `set_texture` (store in `texture_values_`, add name to `known_uniforms_`); implement `has_texture` (check `known_uniforms_` or `texture_values_`); implement `bind()` as no-op; implement `get_texture()`.
11. `src/cmd/commands/demo_command.cpp` — Add `#include "demo/textured_cube_demo.h"`; add `"textured-cube"` to the validation list (line ~57); add dispatch branch for `"textured-cube"` before the existing `else` branch.
12. `src/engine/error.h` — Add `TextureCreationFailed` to `Error::Category` enum and its `to_string` switch, OR reuse `ResourceCreationFailed` for OpenGL texture creation failures. Pick one and be consistent.

### Files NOT requiring changes (auto-discovered by CMake GLOB_RECURSE)
- `src/engine/render/CMakeLists.txt` — does not exist; handled by `src/engine/CMakeLists.txt` GLOB_RECURSE.
- `src/cmd/CMakeLists.txt` — uses GLOB_RECURSE; no edit needed.
- `tests/CMakeLists.txt` — uses GLOB_RECURSE for `*_tests.cpp`; no edit needed.

## Files forbidden to change

- Any file outside `src/engine/render/`, `src/cmd/`, or `tests/` unless explicitly listed above.
- `docs/` files (except writing the contract and updating coordination.md).
- `src/engine/render/shader.h`, `src/engine/render/shader_opengl.h/.cpp`, `src/engine/render/shader_headless.h/.cpp`.
- `src/engine/render/vertex_buffer.h` and backends.
- `src/engine/render/index_buffer.h` and backends.
- `src/engine/render/primitive_topology.h`.
- `src/engine/render/mesh_renderer.h/.cpp`.
- `src/engine/render/vertex_format.h`.
- `src/engine/render/model.h/.cpp` — unchanged except that its `draw()` now triggers `material.bind()` inside `RenderDevice::draw()`, which is transparent to Model.
- `src/engine/scene/` files.
- `src/engine/image/image.h` — already provides the needed API. Do not add methods or modify its include list.
- `src/engine/image/image_buffer.h`.
- `src/engine/platform/`, `src/engine/window/` files.
- `src/cmd/demo/cube_demo.*`, `cube_scene_demo.*`, `free_camera_demo.*`, `triangle_demo.*`, `demo_helpers.*`.
- `tests/*.cpp` except the new `texture_tests.cpp`.

## Existing conventions to follow

1. **Namespace**: All code in `src/engine/` uses `namespace buddd::engine`. Demo code uses `namespace buddd::cmd::demo`.
2. **Include style**: Engine files use `#include "relative/path.h"` with no `src/` prefix. E.g., `#include "render/texture.h"`.
3. **Include order**: Corresponding header first, then standard library, then project headers. See `material_opengl.cpp` for reference.
4. **Error returns**: Use `make_error(Error::Category::..., "message")` returning `std::unexpected<Error>`. Use `[[nodiscard]]` on factory methods.
5. **Logging**: Debug-only logging via `#ifndef NDEBUG` / `std::cerr`. Error logging always on via `std::cerr`.
6. **Copy/move**: Abstract base classes are non-copyable, non-movable (delete copy and move ops). Concrete classes follow the same pattern.
7. **Factory methods**: Return `Result<std::unique_ptr<T>>` for owned resources.
8. **Shared ownership**: Use `std::shared_ptr<Material>` in Model. For textures in Material, use `std::shared_ptr<Texture>`.
9. **Forward declarations**: Prefer forward declarations in headers over includes where possible.
10. **Demo registration pattern**: Add `#include` in `demo_command.cpp`, validate name string, dispatch to function.
11. **Test patterns**: Use `EngineService::create(Backend::Headless, ...)` to get headless device. Use Catch2 `REQUIRE` macros.

## Required implementation behavior

### 1. Abstract Texture class (`src/engine/render/texture.h`)

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

- Must not include any external library headers (no `<SDL3/`, no `<GL/`, no `glm/`, no `stb_`).
- Must not expose any backend types.
- All three query methods are `auto`-returning `int`, `noexcept`, pure virtual.

### 2. TextureOpenGL (`src/engine/render/texture_opengl.h` and `.cpp`)

- Private header (only included by `render_device_opengl.cpp` and `material_opengl.cpp`).
- Includes `<SDL3/SDL_opengl.h>` and `"render/texture.h"`.
- Constructor takes `GLuint texture_handle, int width, int height, int channels`.
- Provides `handle() const noexcept -> GLuint` for `material_opengl.cpp` to use during `bind()`.
- Destructor calls `glDeleteTextures(1, &texture_)`.
- Member: `GLuint texture_{0}`.
- Non-copyable, non-movable.

```cpp
// TextureOpenGL interface (header):
class TextureOpenGL final : public Texture {
public:
    TextureOpenGL(GLuint handle, int width, int height, int channels) noexcept;
    ~TextureOpenGL() override;

    auto width() const noexcept -> int override { return width_; }
    auto height() const noexcept -> int override { return height_; }
    auto channels() const noexcept -> int override { return channels_; }

    auto handle() const noexcept -> GLuint { return texture_; }

    // non-copyable, non-movable
private:
    GLuint texture_;
    int width_;
    int height_;
    int channels_;
};
```

### 3. TextureHeadless (`src/engine/render/texture_headless.h` and `.cpp`)

- Private header (only included by `render_device_headless.cpp` and `material_headless.cpp`).
- Must NOT include `<SDL3/` or `<GL/` headers.
- Constructor takes width, height, channels, and a copy of pixel data.
- Stores `int width_`, `int height_`, `int channels_`, `std::vector<std::byte> data_`.

```cpp
class TextureHeadless final : public Texture {
public:
    TextureHeadless(int width, int height, int channels, std::vector<std::byte> data) noexcept;
    auto width() const noexcept -> int override;
    auto height() const noexcept -> int override;
    auto channels() const noexcept -> int override;
    // getters for test verification exposed as needed
    // non-copyable, non-movable
private:
    int width_;
    int height_;
    int channels_;
    std::vector<std::byte> data_;
};
```

### 4. RenderDevice::create_texture

Add to `render_device.h`:

```cpp
#include "image/image.h"  // or forward declare class Image;

// In class RenderDevice:
[[nodiscard]] virtual auto create_texture(const Image& image)
    -> Result<std::unique_ptr<Texture>> = 0;
```

- Signature is `const Image& image` (not `Image&` — the spec code block is authoritative over the prose).
- Return type is `Result<std::unique_ptr<Texture>>` (consistent with all other factories).

### 5. RenderDeviceOpenGL::create_texture

Validation (in order):

1. If `image.width() <= 0 || image.height() <= 0`: return `InvalidArgument`.
2. If `image.channels() != 1 && image.channels() != 3 && image.channels() != 4`: return `Unsupported`.
3. If `image.data().empty()`: return `InvalidArgument`.
4. If `image.data().size() != static_cast<size_t>(image.width() * image.height() * image.channels())`: return `InvalidArgument`.

If validation passes, create the OpenGL texture with DSA:

```cpp
GLuint tex;
glCreateTextures(GL_TEXTURE_2D, 1, &tex);

GLenum internal_format = (image.channels() == 4) ? GL_RGBA8 :
                         (image.channels() == 3) ? GL_RGB8 : GL_R8;

GLenum format = (image.channels() == 4) ? GL_RGBA :
                (image.channels() == 3) ? GL_RGB : GL_RED;

glTextureStorage2D(tex, 1, internal_format, image.width(), image.height());
glTextureSubImage2D(tex, 0, 0, 0, image.width(), image.height(), format, GL_UNSIGNED_BYTE, image.data().data());

glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
```

After creation, check `glGetError()`. If an error occurred, `glDeleteTextures(1, &tex)` and return `ResourceCreationFailed` (or new category `TextureCreationFailed`).

Return:

```cpp
return std::unique_ptr<Texture>(new TextureOpenGL(tex, image.width(), image.height(), image.channels()));
```

Log creation to stderr: `"Texture created (OpenGL, WxH, N channels)\n"`.

### 6. RenderDeviceHeadless::create_texture

Same validation as OpenGL backend (identical checks). On success:

```cpp
return std::unique_ptr<Texture>(
    new TextureHeadless(image.width(), image.height(), image.channels(),
                        image.data()));
```

### 7. Material changes

**Add to `material.h`:**

```cpp
// Forward declarations at top of file:
class Texture;

// Inside class Material:
virtual auto set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> = 0;
virtual auto has_texture(std::string_view name) const -> bool = 0;

/// Applies all pending material state: activates the shader program
/// (if applicable), applies cached uniforms, binds textures.
/// Called by RenderDevice::draw() at the start of each draw call.
virtual auto bind() const -> void = 0;
```

**MaterialOpenGL changes:**

Add members to `material_opengl.h`:

```cpp
#include "render/texture.h"  // for Texture

// Inside class MaterialOpenGL, private:
std::unordered_map<std::string, std::variant<float, int32_t, bool, math::Vec3, math::Vec4, math::Mat4>> uniform_cache_;
std::unordered_map<std::string, std::shared_ptr<Texture>> texture_map_;
mutable int next_unit_{0};
```

`set_uniform` behaviour (ALL overloads, OpenGL backend):
- Look up uniform location via `get_uniform_location(name)` (same as before).
- If location is -1: return `UniformNotFound` error.
- If location is valid: store value in `uniform_cache_[std::string(name)]` and return success.
- Do NOT call any `glUniform*` function in `set_uniform`.

`set_texture`:
- If `texture` is `nullptr`: return `InvalidArgument`.
- Look up uniform location via `get_uniform_location(name)`.
- If location is -1: return `UniformNotFound`.
- Store `texture` in `texture_map_[std::string(name)]`. Return success.

`has_texture`:
- Check via `glGetUniformLocation(program_, name.data()) != -1` (same as `has_uniform`). Return `true` if location is valid, `false` otherwise.
- Do NOT check the `texture_map_`.

`bind()` const:
```cpp
auto MaterialOpenGL::bind() const -> void {
    // 1. Activate the shader program
    glUseProgram(program_);
#ifndef NDEBUG
    std::cerr << "Material bind: program " << program_ << "\n";
#endif

    // 2. Apply cached uniforms
    for (const auto& [name, value] : uniform_cache_) {
        GLint loc = glGetUniformLocation(program_, name.c_str());
        if (loc == -1) continue;  // uniform was removed or program changed — skip
        std::visit([loc](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, float>)          glUniform1f(loc, v);
            else if constexpr (std::is_same_v<T, int32_t>)   glUniform1i(loc, v);
            else if constexpr (std::is_same_v<T, bool>)      glUniform1i(loc, v ? 1 : 0);
            else if constexpr (std::is_same_v<T, math::Vec3>) glUniform3fv(loc, 1, &v.x);
            else if constexpr (std::is_same_v<T, math::Vec4>) glUniform4fv(loc, 1, &v.x);
            else if constexpr (std::is_same_v<T, math::Mat4>) glUniformMatrix4fv(loc, 1, GL_FALSE, &v[0].x);
        }, value);
    }

    // 3. Bind textures
    for (const auto& [name, texture] : texture_map_) {
        int unit = next_unit_++;
        glActiveTexture(GL_TEXTURE0 + unit);
        auto* gl_tex = static_cast<TextureOpenGL*>(texture.get());
        glBindTexture(GL_TEXTURE_2D, gl_tex->handle());
        GLint loc = glGetUniformLocation(program_, name.c_str());
        if (loc != -1) {
            glUniform1i(loc, unit);
        }
#ifndef NDEBUG
        std::cerr << "Bind texture: " << name << " (unit=" << unit << ")\n";
#endif
    }

    // 4. Reset unit counter for next bind() call
    next_unit_ = 0;
}
```

Key points about `bind()`:
- Uses `glGetUniformLocation` directly (not the cached location) to handle cases where the shader program changes (program_ is const for the material's lifetime, but this keeps it safe).
- The uniform cache lookup uses `std::visit` to dispatch to the correct `glUniform*` call.
- Texture unit assignment starts at 0 each `bind()` call, resets to 0 at end.
- `std::visit` requires C++26 support (which the project targets) or a local polyfill. Alternatively, use if-else chain with `std::holds_alternative`. Prefer `std::visit` if the compiler supports it; otherwise use explicit `if constexpr` with template matching.

**MaterialHeadless changes:**

Add members to `material_headless.h`:

```cpp
#include "render/texture.h"

// Inside class MaterialHeadless, private:
std::unordered_map<std::string, std::shared_ptr<Texture>> texture_values_;

// Public:
auto set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> override;
auto has_texture(std::string_view name) const -> bool override;
auto bind() const -> void override;

/// Returns the texture for the given name, or std::nullopt if not set.
auto get_texture(std::string_view name) const -> std::optional<std::shared_ptr<Texture>>;
```

`set_texture`:
- If `texture` is `nullptr`: return `InvalidArgument`.
- If name not in `known_uniforms_` AND not in `uniform_values_`: return `UniformNotFound` (same pattern as `set_uniform`).
- Add name to `known_uniforms_`.
- Store `texture` in `texture_values_[std::string(name)]`. Return success.

`has_texture`:
- Return `known_uniforms_.count(std::string(name)) > 0 || texture_values_.count(std::string(name)) > 0`.

`bind()`: No-op.

`get_texture`:
- Look up `texture_values_`. If found, return the `shared_ptr<Texture>`. Otherwise return `std::nullopt`.

### 8. RenderDevice::draw() / draw_indexed() changes

**RenderDeviceOpenGL:**

Replace current `glUseProgram(mat.program())` at the start of `draw()` and `draw_indexed()` with `mat.bind()`.

```cpp
auto RenderDeviceOpenGL::draw(...) -> void {
    auto& mat = static_cast<const MaterialOpenGL&>(material);
    auto& vb = static_cast<const VertexBufferOpenGL&>(vertices);

    mat.bind();                          // NEW: glUseProgram + uniforms + textures
    glBindVertexArray(vb.vao());
    glDrawArrays(...);
}
```

Same pattern for `draw_indexed()`.

The `static_cast<const MaterialOpenGL&>(material)` is safe because only `MaterialOpenGL` objects are created by this backend's `create_material`.

**RenderDeviceHeadless:**

In `draw()` and `draw_indexed()`, call `material.bind()` as the first operation (it will be a no-op, but ensures consistent contract):

```cpp
auto RenderDeviceHeadless::draw(...) -> void {
    material.bind();  // no-op for headless
    ++draw_call_count_;
    // ...
}
```

Since the headless `draw()` currently ignores the material parameter, the cast is not needed. Simply call `material.bind()` directly on the `const Material&` reference.

### 9. Texture unit management rules

- `next_unit_` starts at 0.
- Each call to `bind()` assigns units sequentially: first texture gets unit 0, second gets unit 1, etc.
- `next_unit_` is reset to 0 at the end of `bind()`.
- `next_unit_` is `mutable int` to allow `bind()` to be `const`.
- No upper-bound check on `next_unit_` (OpenGL 4.5 guarantees at least 48 combined texture units).
- If a material has no textures, no `glActiveTexture`/`glBindTexture`/`glUniform1i` calls occur.

### 10. Textured cube demo

**Header** (`src/cmd/demo/textured_cube_demo.h`):

```cpp
#pragma once

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

[[nodiscard]] auto run_textured_cube_demo(buddd::engine::RenderDevice& device,
                                          int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo
```

**Implementation** (`src/cmd/demo/textured_cube_demo.cpp`):

Follow the exact pattern of `cube_scene_demo.cpp`:

1. **Load texture**: `auto image_result = Image::load("assets/brick.png");` If it fails, print `FATAL: could not load assets/brick.png: <error>` and `return EXIT_FAILURE`.
2. **Create texture**: `auto texture_result = device.create_texture(*image_result);` If it fails, print FATAL and exit.
3. **Wrap in shared_ptr**: `std::shared_ptr<Texture> texture(std::move(*texture_result));`
4. **Create World, Entity, Camera** — same as cube_scene_demo.
5. **Create vertex buffer** with texture coordinates:
   - Vertex format: position (Float3, loc 0) + texcoord (Float2, loc 1), stride = 20 bytes.
   - 24 vertices with standard per-face UV mapping (each face covers [0,1]×[0,1]).
   - 36 indices (Uint16), same winding as existing cube.
6. **Create shaders**:
   - Vertex shader: `#version 450 core`, takes `a_position` (loc 0) and `a_texcoord` (loc 1), passes UV to fragment via `v_texcoord`, transforms with `u_mvp`.
   - Fragment shader: `#version 450 core`, has `uniform sampler2D u_tex;`, samples with `texture(u_tex, v_texcoord)`, outputs to `frag_color`.
7. **Create material**: `device.create_material(vertex_shader, fragment_shader)`.
8. **Set texture**: `(*material)->set_texture("u_tex", texture);`
9. **Create Model** with the shared material and vertex buffer.
10. **Attach to entity** via `MeshRenderer`.
11. **Create RenderSystem**.
12. **Render loop**: 120 frames at ~60 FPS, rotate entity around Y axis at `elapsed_seconds * 0.5f` radians.
13. **Return EXIT_SUCCESS** after loop.

The demo must verify the texture file exists at `assets/brick.png` relative to the working directory. On CI headless mode, the demo will not run (it requires a display), so the command registration should still allow it for display builds, but the demo itself will not be tested by `demo_tests.cpp` in headless mode — that's acceptable.

### 11. Demo command registration

In `demo_command.cpp`:

1. Add `#include "demo/textured_cube_demo.h"` after existing demo includes.
2. Add `"textured-cube"` to the validation string check (line ~57):
   ```cpp
   if (demo_name != "triangle" && demo_name != "cube" && demo_name != "cube-scene"
       && demo_name != "free-camera" && demo_name != "textured-cube") {
   ```
3. Add dispatch branch before the `else`:
   ```cpp
   } else if (demo_name == "textured-cube") {
       return buddd::cmd::demo::run_textured_cube_demo(**device, argc - 2, argv + 2);
   ```
4. Add `"  textured-cube  Run the textured cube demo (120 frames, UV-mapped brick texture)\n"` to `k_demo_usage`.

### 12. Error::Category

Add `TextureCreationFailed` to the enum in `error.h` (insert before `Unknown` in alphabetical position after `ReadbackFailed`):

```cpp
TextureCreationFailed,
```

Add corresponding case to `to_string`:
```cpp
case Error::Category::TextureCreationFailed: category_str = "TextureCreationFailed"; break;
```

Alternatively, reuse `ResourceCreationFailed`. The contract requires consistency — pick one approach. **Recommended**: add `TextureCreationFailed` as a new distinct category for texture-specific errors (consistent with the existing pattern of fine-grained categories like `ShaderCompilationFailed`, `LinkingFailed`, `ReadbackFailed`).

### 13. Uniform caching backward compatibility

The behavioural change from immediate `glUniform*` to deferred `bind()` must NOT break any existing caller. Verify that:

- `RenderSystem::render()` calls `set_uniform("u_mvp", mvp)` then `model.draw(device)` — this works because `bind()` is called at the top of `draw()`.
- `cube_demo` and `triangle_demo` follow the same pattern via `Model::draw()`.
- `free_camera_demo` also follows the same pattern.

No existing code reads back uniform state between `set_uniform` and `draw()`.

## Required tests

All tests in `tests/texture_tests.cpp`. Use `EngineService::create(Backend::Headless, ...)` for headless tests. Tag tests with `[texture][headless]`.

### Test 1: TextureHeadless stores correct dimensions and channels (AC-003, AC-006)
- **Setup**: Create headless engine, manually construct an `Image` (or use `Image::create()` from an `ImageBuffer`) with known 4-channel RGBA data (e.g., 2×2 checkerboard).
- **Action**: Call `device.create_texture(image)`.
- **Assert**: Result has value. `texture->width() == 2`, `texture->height() == 2`, `texture->channels() == 4`.

### Test 2: TextureHeadless stores pixel data correctly (AC-006)
- **Setup**: Same as Test 1. Construct image with known RGBA pixel values.
- **Action**: Call `device.create_texture(image)`.
- **Assert**: Dynamic cast to `TextureHeadless*` succeeds. Pixel data matches. (Or use `get_texture()` through a material in a later test — but a direct cast in test code is acceptable since this is a test.)

### Test 3: set_texture and get_texture on Headless material (AC-008, AC-009)
- **Setup**: Create headless engine. Create a material with `known_uniforms` containing `"u_tex"`. Create a texture.
- **Action**: `material->set_texture("u_tex", texture)`, then `auto retrieved = headless_material->get_texture("u_tex")`.
- **Assert**: `set_texture` succeeds. `get_texture` returns the same `shared_ptr<Texture>` (compare raw pointer or use `get()`).

### Test 4: has_texture returns true for valid uniform name (AC-008)
- **Setup**: As above.
- **Action**: `material->has_texture("u_tex")` and `material->has_texture("nonexistent")`.
- **Assert**: `has_texture("u_tex") == true` (the name is known). `has_texture("nonexistent") == false`.

### Test 5: bind() is no-op on Headless (AC-012)
- **Setup**: Headless material with a texture set.
- **Action**: Call `material->bind()`.
- **Assert**: No crash. No exception. State unchanged (verify via `get_texture` that the texture is still set).

### Test 6: set_texture with null shared_ptr returns InvalidArgument (AC-016)
- **Setup**: Headless material with known uniform `"u_tex"`.
- **Action**: `material->set_texture("u_tex", nullptr)`.
- **Assert**: Returns error with `Category::InvalidArgument`.

### Test 7: set_texture with unknown name returns UniformNotFound (Story 2)
- **Setup**: Headless material with known uniforms `{}` (empty).
- **Action**: `material->set_texture("u_nonexistent", some_texture)`.
- **Assert**: Returns error with `Category::UniformNotFound`.

### Test 8: create_texture with zero width returns InvalidArgument (AC-017)
- **Setup**: Create an `Image` (or `ImageBuffer` then `Image::create`) with zero width, positive height, valid data.
- **Action**: `device.create_texture(image)`.
- **Assert**: Returns error with `Category::InvalidArgument`.

### Test 9: create_texture with zero height returns InvalidArgument (AC-017)
- **Setup**: Create an `Image` with positive width, zero height.
- **Action**: `device.create_texture(image)`.
- **Assert**: Returns error with `Category::InvalidArgument`.

### Test 10: create_texture with empty data returns InvalidArgument (AC-017)
- **Setup**: Create an `Image` with positive dimensions but empty data vector.
- **Action**: `device.create_texture(image)`.
- **Assert**: Returns error with `Category::InvalidArgument`.

### Test 11: create_texture with 2 channels returns Unsupported (AC-018)
- **Setup**: Create an `Image` with 2 channels. (May not be possible via `Image::create` — may need to construct Image directly if `Image::create` validates. If `Image::create` rejects 2 channels, use a test-specific Image construction.)
- **Action**: `device.create_texture(image)`.
- **Assert**: Returns error with `Category::Unsupported`.

### Test 12: create_texture with >4 channels returns Unsupported (AC-018)
- **Setup**: Same pattern with 5 channels.
- **Action** / **Assert**: Returns error with `Category::Unsupported`.

### Test 13: set_uniform still validates existence (AC-015)
- **Setup**: Headless material with no known uniforms.
- **Action**: `material->set_uniform("u_nonexistent", 1.0f)`.
- **Assert**: Returns `UniformNotFound`.

### Test 14: Texture header exposes no backend types (AC-021)
- **Static check**: Grep `texture.h` for `SDL_`, `gl`, `GL_`, `GLAD`, `stb_` — must not match. This is a compile-time / code-review check.

### Test 15: TextureOpenGL destructor calls glDeleteTextures (AC-002)
- **Code review**: Verify the destructor body has `glDeleteTextures(1, &texture_)`.

### Test 16: TextureHeadless has no SDL3/GL includes (AC-003)
- **Code review**: Grep `texture_headless.h` and `texture_headless.cpp` for `SDL3` or `GL/` — must not match.

### Test 17 (OpenGL-only, conditional): OpenGL texture creation sequence (AC-005)
- **Setup**: Requires display (`BUDDD_HAS_DISPLAY`). Create engine with `Backend::SDL3`, render device with offscreen context.
- **Action**: Load or create a small 4-channel RGBA image. Call `device.create_texture(image)`.
- **Assert**: Returns valid texture. No GL error (use `glGetError`). This test can be in `sdl3_backend_tests.cpp` or guarded with `#ifdef BUDDD_HAS_DISPLAY`.

## Edge cases

All edge cases from the spec (section "Edge cases") must be handled by the implementation:

| Edge case | Required behaviour |
|---|---|
| 1-channel (grayscale) image | `GL_R8` internal format, `GL_RED` data format. |
| 3-channel (RGB) image | `GL_RGB8` / `GL_RGB`. |
| 4-channel (RGBA) image | `GL_RGBA8` / `GL_RGBA`. |
| 2-channel image | Error `Unsupported`. |
| >4 channels | Error `Unsupported`. |
| `set_texture` on a `uniform float` name | Succeeds (name exists). `bind()` will call `glUniform1i` on a float uniform — defined behaviour. |
| `set_texture` called after `bind()` | Stored in cache, applied on next `bind()`. Legal. |
| Same texture set twice on same material (same name) | Second call overwrites first. No error. |
| Same `shared_ptr<Texture>` on multiple materials | Legal. Each material binds independently during its own `bind()`. |
| `bind()` with no textures | No texture operations. Program activated + cached uniforms applied. |
| `bind()` with no cached uniforms | No `glUniform*` calls. Program activated + textures bound. |
| `bind()` called twice consecutively same material | Re-activates program, re-applies uniforms, re-binds textures (correct but slightly wasteful). |
| `Image` passed to `create_texture` has moved-from/destroyed image | Not the engine's responsibility — caller must ensure `Image` outlives the call. The backend copies data during creation. |
| `Image` with `data().size()` mismatch | Returns `InvalidArgument`. |
| Negative width or height in `Image` | Returns `InvalidArgument` (positive check catches both zero and negative). |

## Error categories

| Error case | Category |
|---|---|
| `create_texture` with zero/negative width or height | `InvalidArgument` |
| `create_texture` with empty data | `InvalidArgument` |
| `create_texture` with unsupported channel count | `Unsupported` |
| `create_texture` with data size mismatch | `InvalidArgument` |
| `create_texture` OpenGL upload failure (glGetError) | `TextureCreationFailed` (or `ResourceCreationFailed`) |
| `set_texture` with null `shared_ptr<Texture>` | `InvalidArgument` |
| `set_texture` with name not found in shader | `UniformNotFound` |
| `set_uniform` with name not found in shader | `UniformNotFound` (unchanged behaviour) |
| PNG file not found | `IoFailed` (from `Image::load()`, not modified) |

## Security impact

- No elevated privileges required.
- No network access.
- The `Image` class already handles PNG decoding (stb_image) and performs bounds checking.
- The new `create_texture` validates dimensions and data size to prevent invalid memory access.
- Architecture boundary (CONST-001) is maintained — backend types are invisible outside `src/engine/`.

## Data and migration impact

None. No schema, no persistent data, no migrations.

## API compatibility impact

- `MaterialOpenGL::set_uniform` changes from immediate `glUniform*` to deferred caching. This is backward-compatible for all existing callers because all callers follow the `set_uniform` → `draw()` sequence. No caller depends on immediate GL state side effects.
- `RenderDevice::draw()` signature is unchanged (still takes `const Material&`). Behaviour changes: now calls `material.bind()` internally instead of directly calling `glUseProgram`. This is invisible to callers.
- New virtual methods added to `Material` (`set_texture`, `has_texture`, `bind`) — all existing concrete subclasses (`MaterialOpenGL`, `MaterialHeadless`) must implement them.
- New virtual method added to `RenderDevice` (`create_texture`) — all existing concrete subclasses must implement it.

## Documentation impact

None (demo is self-documenting via its source code and the usage text in `demo_command.cpp`).

## ADR impact

None. The contract follows existing ADRs (ADR-001 for `Result<T>`, ADR-003 for `void` draw methods). No new ADR needed.

## Constitution impact

None. CONST-001 is respected by keeping backend types in private headers.

## Done criteria

Each criterion is verifiable by code inspection, compilation, or test execution.

- [ ] **DC-1**: `src/engine/render/texture.h` exists. `Texture` is abstract, has virtual destructor, pure virtual `width()`/`height()`/`channels()` returning `int`, is non-copyable and non-movable, has no external library types in its public interface. Verified by `grep -E '(SDL_|gl[A-Z]|GL_|GLAD|stb_)' src/engine/render/texture.h` returning no matches.
- [ ] **DC-2**: `src/engine/render/texture_opengl.h` exists, is a private header, implements `Texture`. Destructor calls `glDeleteTextures`. Verified by reading the destructor body.
- [ ] **DC-3**: `src/engine/render/texture_headless.h` exists, implements `Texture`, stores pixel data in `std::vector<std::byte>`. No SDL3 or GL includes. Verified by `grep -E '(SDL3|GL/)'` returning no matches.
- [ ] **DC-4**: `render_device.h` has `create_texture(const Image&) -> Result<std::unique_ptr<Texture>>` as a pure virtual method.
- [ ] **DC-5**: `RenderDeviceOpenGL::create_texture` creates `GL_TEXTURE_2D` using DSA (`glCreateTextures`, `glTextureStorage2D`, `glTextureSubImage2D`), sets `GL_LINEAR` min/mag, `GL_CLAMP_TO_EDGE` wrapping. Supports 1, 3, 4 channel images with correct format mapping. Returns `unique_ptr<TextureOpenGL>`. Verified by reading the implementation.
- [ ] **DC-6**: `RenderDeviceHeadless::create_texture` validates dimensions/channels/data (same validation as OpenGL), returns `unique_ptr<TextureHeadless>`. Verified by reading the implementation.
- [ ] **DC-7**: `material.h` has pure virtual `set_texture(std::string_view, std::shared_ptr<Texture>) -> Result<void>`, `has_texture(std::string_view) -> bool`, `bind() const -> void`. Verified by reading `material.h`.
- [ ] **DC-8**: `MaterialOpenGL::set_texture` stores texture in `texture_map_` after validating name exists via `get_uniform_location()`. `MaterialOpenGL::has_texture` checks `glGetUniformLocation`. `MaterialOpenGL::bind()` implements the full sequence: `glUseProgram`, cached uniforms, texture binding with unit management, unit counter reset. Verified by reading `material_opengl.cpp`.
- [ ] **DC-9**: `MaterialOpenGL::set_uniform` no longer calls `glUniform*` immediately — it caches the value. Still validates uniform existence and returns `UniformNotFound` if location is -1. Verified by reading each `set_uniform` overload.
- [ ] **DC-10**: `MaterialHeadless::set_texture` stores in `texture_values_`, adds to `known_uniforms_`. `MaterialHeadless::has_texture` checks known_uniforms_ or texture_values_. `MaterialHeadless::bind()` is no-op. `MaterialHeadless::get_texture(name)` returns `std::optional<std::shared_ptr<Texture>>`. Verified by reading `material_headless.cpp`.
- [ ] **DC-11**: `RenderDeviceOpenGL::draw()` and `draw_indexed()` call `mat.bind()` as first operation, replacing direct `glUseProgram` calls. Verified by reading `render_device_opengl.cpp`.
- [ ] **DC-12**: `RenderDeviceHeadless::draw()` and `draw_indexed()` call `material.bind()` as first operation (no-op). Verified by reading `render_device_headless.cpp`.
- [ ] **DC-13**: `Error::Category` has `TextureCreationFailed` (or `ResourceCreationFailed` is reused) with corresponding `to_string` entry. Verified by reading `error.h`.
- [ ] **DC-14**: All tests in `tests/texture_tests.cpp` pass with headless backend: `ctest --preset debug` or running `buddd_tests [texture]` shows all texture tests passing.
- [ ] **DC-15**: `"textured-cube"` is registered in `demo_command.cpp` (validation list, usage text, and dispatch branch). The demo compiles and can be launched (visual check only).
- [ ] **DC-16**: `src/cmd/demo/textured_cube_demo.cpp` implements the 120-frame scene-graph demo as specified: loads `assets/brick.png`, creates texture, creates cube with UV coordinates, applies texture via `set_texture`, renders with rotating cube.
- [ ] **DC-17**: The `texture.h` public header does not expose any backend-specific types. Verified by `grep -E '(SDL_|gl[A-Z]|GL_|GLAD|stb_)' src/engine/render/texture.h` returning no matches.
- [ ] **DC-18**: All existing tests that use materials (e.g., `scene_rendering_tests`, `render_device_tests`) still pass — confirming the deferred uniform caching does not break existing code.
