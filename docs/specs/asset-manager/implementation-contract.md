# IMPL-019 — Asset Manager Implementation Contract

## Source spec

`docs/specs/asset-manager/spec.md`

## Goal

Implement a centralized Asset Manager system (SPEC-019) for the Buddd Engine: an `AssetManager` class owned by `EngineService` that provides ID-based, lazy-loaded access to `TextureAsset` and `MaterialAsset` types via YAML metadata files. The system includes shader program deduplication, a `FileWatcher` (Linux inotify) for hot-reload of YAML, image, and shader source files, a dependency graph for reverse-dependency tracking, and a `ShaderProgram` wrapper for shared GL program handles. All loading logic is testable in headless mode.

## Non-goals

- Do NOT implement MaterialInstance, material blueprints, node graphs, or visual shader editing.
- Do NOT implement non-YAML asset metadata formats (JSON, TOML).
- Do NOT implement startup-time directory scanning or asset index files.
- Do NOT implement async/background asset loading.
- Do NOT implement asset streaming, LOD, or partial loading.
- Do NOT implement network-based asset loading.
- Do NOT implement Windows/macOS FileWatcher backends (Linux inotify only; NullFileWatcher for others).
- Do NOT implement glTF/glb model loading.
- Do NOT implement shader `#include` preprocessing.
- Do NOT implement texture atlas generation, compression, or mipmap generation (`generate_mipmaps` field is parsed but NOT applied).
- Do NOT implement asset browser GUI or editor integration.
- Do NOT implement file event debouncing/deduplication.
- Do NOT modify existing `create_material(unique_ptr<Shader>, unique_ptr<Shader>, span<const string>)` signature on RenderDevice.
- Do NOT modify existing demo apps (`TexturedCubeApp`, etc.) — the new demo is a separate file.
- Do NOT modify `PhongMaterial` or any shader files under `src/engine/render/phong/`.

## Relevant constitution rules

- **CONST-001**: No platform/graphics/windowing headers outside `src/engine/`. `ShaderProgram` exposes `GLuint` and MUST live in `src/engine/render/`. `swap_handle()` on `Texture` uses `uint32_t` to avoid GLuint leak into the base class. yaml-cpp types must NOT appear in public headers.
- **CONST-002**: All new code MUST have corresponding unit tests that pass.
- **ADR-001**: All fallible APIs return `Result<T>` (not exceptions). yaml-cpp exceptions MUST be caught and converted to `Result` errors.
- **ADR-009**: New test file must be named `asset_manager_tests.cpp` (plural `_tests.cpp` suffix).
- **ADR-012**: `EngineService` member declaration order matters — `asset_manager_` must be declared AFTER `device_` to ensure correct destruction order. New accessor `assets() -> AssetManager&`.

## Relevant ADRs

- ADR-001 (Result/Error pattern) — all new APIs return `Result<T>`.
- ADR-009 (test naming) — `asset_manager_tests.cpp`.
- ADR-010 (no raw pointers) — all back-references use `T&`.
- ADR-012 (EngineService navigable graph) — `asset_manager_` member declaration order, `assets()` accessor.

## Files to inspect

The Code Agent MUST read these files before editing to understand the existing patterns, signatures, and conventions:

- `src/engine/error.h` — `Error` struct, `Error::Category` enum, `make_error()`, `Result<T>` alias
- `src/engine/engine_service.h` — current EngineService API, member declaration order
- `src/engine/engine_service.cpp` — `create()` factory implementation pattern
- `src/engine/render/render_device.h` — existing `create_material` signature, `create_texture` signature
- `src/engine/render/render_device_opengl.h` + `.cpp` — OpenGL implementations
- `src/engine/render/render_device_headless.h` + `.cpp` — Headless implementations
- `src/engine/render/texture.h` — abstract `Texture` class
- `src/engine/render/texture_opengl.h` + `.cpp` — concrete OpenGL texture
- `src/engine/render/texture_headless.h` + `.cpp` — concrete Headless texture
- `src/engine/render/material.h` — abstract `Material` class
- `src/engine/render/material_opengl.h` + `.cpp` — OpenGL material (note `bind()` uses `program_`)
- `src/engine/render/material_headless.h` + `.cpp` — Headless material
- `src/engine/render/shader.h` — abstract `Shader` + `ShaderType` enum
- `src/engine/render/shader_opengl.h` — concrete `ShaderOpenGL` with `handle()`
- `src/engine/render/shader_headless.h` — concrete `ShaderHeadless` with `source()`
- `src/engine/render/glsl_util.h` — `extract_uniform_names()` helper
- `src/engine/image/image.h` — `Image::load(path)` signature
- `src/engine/CMakeLists.txt` — FetchContent pattern for dependencies, `file(GLOB_RECURSE)` for sources
- `tests/texture_tests.cpp` — test patterns, `make_headless_engine()` helper
- `tests/CMakeLists.txt` — test build configuration, glob pattern

## Files allowed to change

### New files (must be created):

| File | Purpose |
|------|---------|
| `src/engine/asset/asset.h` | Abstract `Asset` base class. |
| `src/engine/asset/asset_id.h` | (Optional) Asset ID utilities, path resolution helper. |
| `src/engine/asset/asset_manager.h` | `AssetManager` class declaration + `ShaderProgramKey` struct + `create<T>()` template declaration. |
| `src/engine/asset/asset_manager.tpp` | `create<T>()` template implementation, included at bottom of `asset_manager.h`. |
| `src/engine/asset/asset_manager.cpp` | Non-template parts of `AssetManager` + explicit instantiations for `create<TextureAsset>` and `create<MaterialAsset>`. |
| `src/engine/asset/texture_asset.h` | `TextureAsset` concrete class (wraps `shared_ptr<Texture>`). |
| `src/engine/asset/texture_asset.cpp` | `TextureAsset` implementation (if any non-trivial logic). |
| `src/engine/asset/material_asset.h` | `MaterialAsset` concrete class (wraps `shared_ptr<Material>`). |
| `src/engine/asset/material_asset.cpp` | `MaterialAsset` implementation (if any non-trivial logic). |
| `src/engine/asset/file_watcher.h` | `FileWatcher` abstract base + `NullFileWatcher` + `FileEvent` struct + `FileEventType` enum. |
| `src/engine/asset/file_watcher.cpp` | `FileWatcher::create()` factory and `~FileWatcher()` destructor implementations. |
| `src/engine/asset/file_watcher_inotify.h` | `InotifyFileWatcher` concrete class (Linux). |
| `src/engine/asset/file_watcher_inotify.cpp` | Inotify implementation (Linux only, `#ifdef __linux__`). |
| `src/engine/asset/dependency_map.h` | `DependencyMap` class (bidirectional asset↔source tracking). |
| `src/engine/asset/dependency_map.cpp` | `DependencyMap` implementation. |
| `src/engine/render/shader_program.h` | `ShaderProgram` class declaration (GLuint wrapper, render layer, CONST-001 compliant). |
| `src/engine/render/shader_program.cpp` | `ShaderProgram` implementation (OpenGL linking + Headless compatible stub). |
| `tests/asset_manager_tests.cpp` | All asset manager unit tests. |

### New demo app and sample asset files (must be created):

| File | Purpose |
|------|---------|
| `src/cmd/apps/asset_demo_app.h` | `AssetDemoApp` — demo that uses `AssetManager` to load a Material from YAML and render a textured cube. |
| `src/cmd/apps/asset_demo_app.cpp` | `AssetDemoApp` implementation — `setup()` creates an `AssetManager` via `AssetManager::create(device, "assets")`, then calls `asset_manager_->create<MaterialAsset>("materials/demo_cube")`, creates cube mesh, sets up render loop. |
| `src/cmd/commands/demo_command.h` | Declaration for the demo command handler, following the `help_command.h`/`version_command.h` pattern with a `DemoCommand` class and a `run()` method. |
| `src/cmd/commands/demo_command.cpp` | Top-level `buddd demo asset-demo` command handler. Parses args, creates `AssetDemoApp`, runs it for 120 frames. |
| `assets/textures/demo_brick.yaml` | Texture metadata — points to `assets/textures/demo_brick.png`. Settings: repeat wrapping, linear filtering. |
| `assets/materials/demo_cube.yaml` | Material metadata — references `assets/shaders/demo.vert`, `assets/shaders/demo.frag`, and texture `textures/demo_brick`. |
| `assets/shaders/demo.vert` | Vertex shader with UV passthrough (`#version 450 core`, `a_position`, `a_texcoord`, `u_mvp`). |
| `assets/shaders/demo.frag` | Fragment shader sampling `u_tex` texture (`#version 450 core`, `v_texcoord`, `sampler2D u_tex`). |
| `assets/textures/demo_brick.png` | Small PNG texture (e.g. 2×2 or 4×4 checker pattern) used by the demo. Must be a valid PNG file. |

### Modified files:

| File | Change |
|------|--------|
| `src/engine/render/render_device.h` | Add `virtual auto create_material(std::shared_ptr<ShaderProgram> program) -> Result<std::unique_ptr<Material>> = 0;` overload. See Required implementation behavior for `replace_gl_handle` / `release_gl_handle` texture changes. |
| `src/engine/render/render_device_opengl.h` | Declare new `create_material(shared_ptr<ShaderProgram>)` override. |
| `src/engine/render/render_device_opengl.cpp` | Implement new overload: extract `GLuint` from `ShaderProgram`, create `MaterialOpenGL` that holds `shared_ptr<ShaderProgram>` for hot-reload. |
| `src/engine/render/render_device_headless.h` | Declare new `create_material(shared_ptr<ShaderProgram>)` override. |
| `src/engine/render/render_device_headless.cpp` | Implement new overload: create `MaterialHeadless` with empty known_uniforms (or extract from shader sources). |
| `src/engine/render/texture.h` | Add `virtual auto replace_gl_handle(uint32_t new_handle) -> void` with default no-op. Add `virtual auto gl_handle() const noexcept -> uint32_t` returning 0. Add `virtual auto release_gl_handle() noexcept -> uint32_t` returning 0. |
| `src/engine/render/texture_opengl.h` + `.cpp` | Override `replace_gl_handle` (swap handle, delete old), `gl_handle` (return `texture_` as `uint32_t`), and `release_gl_handle` (extract handle, clear internal, return old). |
| `src/engine/render/texture_headless.h` + `.cpp` | No override needed (inherits no-op defaults). |
| `src/engine/render/material_opengl.h` + `.cpp` | Add new constructor `MaterialOpenGL(std::shared_ptr<ShaderProgram> program)`. Modify `bind()` to dereference `shader_program_` if set, else use `program_`. Keep existing `MaterialOpenGL(GLuint)` unchanged. |
| `src/engine/engine_service.h` | Add `#include <memory>` for `unique_ptr` (already present). Add forward declaration `class AssetManager;`. Add `std::unique_ptr<AssetManager> asset_manager_;` member (declared AFTER `device_`). Add `auto assets() noexcept -> AssetManager&;` accessor. |
| `src/engine/engine_service.cpp` | Add `#include "asset/asset_manager.h"`. In `create()` static method, after creating the `RenderDevice`, construct `AssetManager` via `AssetManager::create(engine_service.device(), "assets")`. Implement `assets()` accessor. |
| `src/engine/CMakeLists.txt` | Add yaml-cpp FetchContent block (after stb block, before `find_package(OpenGL)`). Add yaml-cpp include dir to PRIVATE includes. Add yaml-cpp to PRIVATE link libraries. |
| `src/cmd/main.cpp` | Add `#include "commands/demo_command.h"` and register the top-level `buddd demo` command (dispatched before the `run` command handler).

### Test asset files:

Create the following test assets under `tests/assets/`:

| File | Content |
|------|---------|
| `tests/assets/textures/test_texture.yaml` | `type: Texture`, `version: 1`, `source: tests/assets/textures/test_image.png` |
| `tests/assets/textures/test_image.png` | Small valid PNG (e.g., 2x2 RGBA, generated programmatically at test time or committed as a 1x1 red pixel PNG). |
| `tests/assets/textures/missing_source.yaml` | `type: Texture`, `version: 1`, no `source` field. |
| `tests/assets/textures/type_material.yaml` | `type: Material`, `version: 1`, no shader fields (triggers type mismatch). |
| `tests/assets/textures/unsupported_version.yaml` | `type: Texture`, `version: 2`, `source: tests/assets/textures/test_image.png`. |
| `tests/assets/materials/test_material.yaml` | `type: Material`, `version: 1`, `shaders: { vertex: tests/assets/shaders/test.vert, fragment: tests/assets/shaders/test.frag }`, `textures: { albedo: textures/test_texture }`, `constants: { roughness: 0.5 }`. |
| `tests/assets/shaders/test.vert` | Minimal vertex shader (e.g., `#version 450 core\nlayout(location=0) in vec3 pos;\nvoid main(){gl_Position=vec4(pos,1);}`). |
| `tests/assets/shaders/test.frag` | Minimal fragment shader (e.g., `#version 450 core\nout vec4 color;\nuniform float roughness;\nuniform sampler2D albedo;\nvoid main(){color=vec4(roughness);}`). |
| `tests/assets/shaders/compile_error.vert` | Vertex shader containing `#error` to trigger compilation failure. |

## Files forbidden to change

- `src/cmd/apps/` — existing demo app files must NOT be modified (new files `src/cmd/apps/asset_demo_app.{h,cpp}` are permitted).
- `src/cmd/commands/` — existing command files must NOT be modified (new files `src/cmd/commands/demo_command.{h,cpp}` are permitted).
- `src/cmd/app.h`, `src/cmd/app.cpp`, `src/cmd/app_config.h` — must NOT be modified.
- `src/engine/render/phong/` — any files (existing Phong material system)
- `docs/` — any existing files (except the `asset-manager/` spec directory files being created/updated)
- `tests/test_helpers.h` — existing test helpers
- `tests/CMakeLists.txt` — existing test CMake (new file is auto-picked by glob)
- `.clang-format`, `.clang-tidy`, `.gitignore`, or any CI/CD configuration files
- Any `Makefile`, `CMakePresets.json`, or build configuration outside `src/engine/CMakeLists.txt`

## Existing conventions to follow

- **Header guard**: `#pragma once` (no `#ifndef` guards).
- **Namespace**: `buddd::engine` for all engine code. Nested namespaces for details.
- **File naming**: `snake_case.h` / `snake_case.cpp`.
- **Include style**: `#include "relative/path/from/src/engine/file.h"` (no angle brackets for project headers).
- **Trailing return types**: `auto method() -> ReturnType`.
- **Result pattern**: `Result<T> = std::expected<T, Error>`. Factory pattern: `static auto create(...) -> Result<std::unique_ptr<ClassName>>`.
- **Non-copyable, non-movable**: Delete copy/move operations for resource-owning types. `ShaderProgram` is movable (for emplacement in `shared_ptr`).
- **`[[nodiscard]]`** on all `Result<T>` factory methods and any method whose return value should not be ignored.
- **`noexcept`** on simple accessors / getters.
- **Forward declarations** instead of includes where possible in headers.
- **`std::cerr`** for logging (consistent with existing engine code).
- **Debug-only logging** guarded by `#ifndef NDEBUG`.
- **Tests**: Catch2 v3, `make_headless_engine()` helper, `REQUIRE`/`REQUIRE_FALSE`, tag `[asset][headless]`.
- **`file(GLOB_RECURSE)`** for source files in CMake — new files in `src/engine/asset/` are auto-discovered on re-configure.
- **Destruction order**: `AssetManager` holds `RenderDevice&`, so `asset_manager_` must be declared AFTER `device_` in `EngineService`.

## Required implementation behavior

### 1. ShaderProgram (`src/engine/render/shader_program.h` / `.cpp`)

```cpp
#pragma once

#include "error.h"
#include "render/shader.h"

#include <SDL3/SDL_opengl.h>  // GLuint

#include <memory>
#include <string>
#include <utility>

namespace buddd::engine {

class ShaderProgram {
public:
    [[nodiscard]] static auto create(std::unique_ptr<Shader> vertex_shader,
                                     std::unique_ptr<Shader> fragment_shader)
        -> Result<ShaderProgram>;

    ~ShaderProgram();

    auto handle() const noexcept -> GLuint;
    auto is_valid() const noexcept -> bool;

    /// Hot-reload support: replaces the internal GPU handle with a new one.
    /// The old handle is deleted via glDeleteProgram before assignment.
    /// In headless mode this is a no-op (handle_ is always 0).
    auto replace_handle(GLuint new_handle) -> void;

    /// Releases ownership of the internal GPU handle, setting it to 0.
    /// Returns the previous handle value. Used to prevent double-deletion
    /// when transferring ownership (similar to Texture::release_gl_handle).
    auto release_handle() noexcept -> GLuint;

    ShaderProgram(const ShaderProgram&) = delete;
    auto operator=(const ShaderProgram&) -> ShaderProgram& = delete;
    ShaderProgram(ShaderProgram&&) noexcept;
    auto operator=(ShaderProgram&&) noexcept -> ShaderProgram&;

    // Test-only accessor for hot-reload pipeline verification (spec AC-023).
    // Returns an opaque ID that changes on each successful recompilation.
#ifdef BUDDD_TESTING
    auto testing_handle() const noexcept -> uint64_t;
#endif

private:
    ShaderProgram(/* implementation-defined */);

    // OpenGL mode (BUDDD_HAS_DISPLAY): stores linked GLuint.
    // Headless mode: stores a generation counter, handle_ is a sentinel.
    GLuint handle_ = 0;
#ifndef BUDDD_HAS_DISPLAY
    uint64_t generation_ = 0;
    std::string vs_source_;
    std::string fs_source_;
#endif
};

} // namespace buddd::engine
```

**Implementation behavior:**

- **OpenGL mode** (`BUDDD_HAS_DISPLAY`):
  - Extract `GLuint` from `ShaderOpenGL` via `static_cast` + `handle()`.
  - Call `glCreateProgram()`, `glAttachShader`, `glLinkProgram`.
  - On link failure: return `make_error(Error::Category::LinkingFailed, log)`.
  - On success: mark shaders for deletion via `glDeleteShader` (they stay alive until program is deleted).
  - `replace_handle(GLuint new_handle)`: calls `glDeleteProgram(handle_)` (if non-zero), assigns `handle_ = new_handle`.
  - `release_handle() -> GLuint`: stores `handle_` in local, sets `handle_ = 0`, returns stored value.
  - Move constructor: transfers `handle_`, sets source handle to 0. The source `ShaderProgram` has `handle_ = 0` after move, so its destructor is a no-op.
  - Move assignment: deletes current `handle_` via `glDeleteProgram` (if non-zero), then transfers `handle_` from source, sets source `handle_ = 0`.

- **Headless mode** (no `BUDDD_HAS_DISPLAY`):
  - Accept `ShaderHeadless` objects, extract source strings.
  - Simulate linking: check vertex outputs vs fragment inputs (matching logic from `render_device_headless.cpp`'s `extract_variable_names`).
  - On simulated link failure: return `make_error(Error::Category::LinkingFailed)`.
  - On success: store source strings, assign a generation counter (incrementing static).
  - `handle()` returns `0` (no real GL handle).
  - `testing_handle()` returns `generation_`.
  - `replace_handle(GLuint)`: no-op (ignores argument, handle_ is always 0).
  - `release_handle() -> GLuint`: returns 0, sets `handle_ = 0` (no-op).
  - Move constructor: transfers `handle_` (0), `generation_`, `vs_source_`, `fs_source_`; source is left in valid empty state.
  - Move assignment: deletes old state, then transfers `handle_`, `generation_`, `vs_source_`, `fs_source_` from source; source is left in valid empty state.

### 2. Texture base class changes (`src/engine/render/texture.h`)

Add to the abstract `Texture` class:

```cpp
    /// Hot-reload support: replaces the underlying GPU handle.
    /// new_handle is a backend-specific handle (GLuint cast to uint32_t).
    /// Default implementation is no-op (for headless).
    virtual auto replace_gl_handle(uint32_t new_handle) -> void;

    /// Returns the backend-specific handle (GLuint cast to uint32_t).
    /// Default returns 0.
    virtual auto gl_handle() const noexcept -> uint32_t;

    /// Releases ownership of the underlying GPU handle, clearing it to 0.
    /// Returns the previous handle value. Used to prevent double-deletion
    /// when the temporary texture's destructor would otherwise delete the
    /// handle that was just swapped into the existing texture.
    /// Default returns 0 (no-op for headless).
    virtual auto release_gl_handle() noexcept -> uint32_t;
```

- `TextureOpenGL` overrides all three. `replace_gl_handle` deletes old `texture_` with `glDeleteTextures`, assigns `texture_ = static_cast<GLuint>(new_handle)`. `gl_handle` returns `static_cast<uint32_t>(texture_)`. `release_gl_handle` stores `texture_` in local, sets `texture_ = 0`, returns stored value.
- `TextureHeadless` does NOT override (inherits no-op defaults).

### 3. MaterialOpenGL changes (`src/engine/render/material_opengl.h` / `.cpp`)

- Add a second constructor: `MaterialOpenGL(std::shared_ptr<ShaderProgram> program)`.
- Add private member: `std::shared_ptr<ShaderProgram> shader_program_;`.
- Modify `bind()`:
  - If `shader_program_` is set: use `shader_program_->handle()` for `glUseProgram`.
  - Else: use existing `program_` for `glUseProgram`.
- Keep existing `MaterialOpenGL(GLuint)` constructor unchanged.
- The `program()` accessor returns `shader_program_->handle()` if `shader_program_` is non-null, else returns `program_`.

### 4. RenderDevice new overload (`render_device.h`)

```cpp
    /// NEW overload: creates a Material backed by a pre-compiled, shared ShaderProgram.
    /// The Material holds a shared_ptr<ShaderProgram> so that hot-reload of the
    /// shader source is reflected automatically at bind() time.
    /// The Material has its own uniform/texture state independent of other Materials
    /// sharing the same ShaderProgram.
    [[nodiscard]] virtual auto create_material(std::shared_ptr<ShaderProgram> program)
        -> Result<std::unique_ptr<Material>> = 0;
```

**OpenGL implementation:**
```cpp
auto RenderDeviceOpenGL::create_material(std::shared_ptr<ShaderProgram> program)
    -> Result<std::unique_ptr<Material>>
{
    if (!program || !program->is_valid()) {
        return make_error(Error::Category::InvalidArgument, "Invalid ShaderProgram");
    }
    std::cerr << "Material created (from ShaderProgram)\n";
    return std::unique_ptr<Material>(new MaterialOpenGL(std::move(program)));
}
```

**Headless implementation:**
```cpp
auto RenderDeviceHeadless::create_material(std::shared_ptr<ShaderProgram> program)
    -> Result<std::unique_ptr<Material>>
{
    if (!program || !program->is_valid()) {
        return make_error(Error::Category::InvalidArgument, "Invalid ShaderProgram");
    }
    // Headless: create MaterialHeadless with no known uniforms.
    // Uniform/texture state is tracked via set_uniform/set_texture calls.
    ++material_count_;
    std::cerr << "Material created (Headless, from ShaderProgram)\n";
    return std::unique_ptr<Material>(
        new MaterialHeadless(std::unordered_set<std::string>{}));
}
```

### 5. Asset base class (`src/engine/asset/asset.h`)

Exactly as the spec defines it — abstract base, non-copyable, non-movable, virtual destructor, protected default constructor.

### 6. TextureAsset (`src/engine/asset/texture_asset.h` / `.cpp`)

```cpp
#pragma once

#include "asset/asset.h"
#include "render/texture.h"

#include <memory>

namespace buddd::engine {

class TextureAsset final : public Asset {
public:
    explicit TextureAsset(std::shared_ptr<Texture> texture) noexcept;
    
    auto texture() const noexcept -> const std::shared_ptr<Texture>&;

    TextureAsset(const TextureAsset&) = delete;
    auto operator=(const TextureAsset&) -> TextureAsset& = delete;
    TextureAsset(TextureAsset&&) = delete;
    auto operator=(TextureAsset&&) -> TextureAsset& = delete;

private:
    std::shared_ptr<Texture> texture_;
};

} // namespace buddd::engine
```

### 7. MaterialAsset (`src/engine/asset/material_asset.h` / `.cpp`)

```cpp
#pragma once

#include "asset/asset.h"
#include "render/material.h"

#include <memory>

namespace buddd::engine {

class MaterialAsset final : public Asset {
public:
    explicit MaterialAsset(std::shared_ptr<Material> material) noexcept;
    
    auto material() const noexcept -> const std::shared_ptr<Material>&;

    MaterialAsset(const MaterialAsset&) = delete;
    auto operator=(const MaterialAsset&) -> MaterialAsset& = delete;
    MaterialAsset(MaterialAsset&&) = delete;
    auto operator=(MaterialAsset&&) -> MaterialAsset& = delete;

private:
    std::shared_ptr<Material> material_;
};

} // namespace buddd::engine
```

### 8. ShaderProgramKey

Defined in `asset_manager.h` (since it's an implementation detail of the deduplication map):

```cpp
struct ShaderProgramKey {
    std::string vertex_path;
    std::string fragment_path;

    auto operator==(const ShaderProgramKey&) const -> bool = default;
};

template<>
struct std::hash<ShaderProgramKey> {
    auto operator()(const ShaderProgramKey& k) const noexcept -> size_t {
        return std::hash<std::string>{}(k.vertex_path)
             ^ (std::hash<std::string>{}(k.fragment_path) << 1);
    }
};
```

### 9. DependencyMap (`src/engine/asset/dependency_map.h` / `.cpp`)

```cpp
#pragma once

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace buddd::engine {

class DependencyMap {
public:
    auto add_dependency(std::string_view asset_id, std::string_view source_path) -> void;
    auto get_dependencies(std::string_view asset_id) const -> std::span<const std::string>;
    auto get_dependents(std::string_view source_path) const -> std::span<const std::string>;
    auto remove_asset(std::string_view asset_id) -> void;
    auto clear() -> void;

    DependencyMap() = default;
    DependencyMap(const DependencyMap&) = delete;
    auto operator=(const DependencyMap&) -> DependencyMap& = delete;
    DependencyMap(DependencyMap&&) = default;
    auto operator=(DependencyMap&&) -> DependencyMap& = default;

private:
    std::unordered_map<std::string, std::vector<std::string>> forward_;   // asset_id → [source_paths]
    std::unordered_map<std::string, std::vector<std::string>> reverse_;   // source_path → [asset_ids]
};

} // namespace buddd::engine
```

### 10. FileWatcher (`src/engine/asset/file_watcher.h`)

```cpp
#pragma once

#include "error.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::engine {

enum class FileEventType {
    Created,
    Modified,
    Deleted
};

struct FileEvent {
    std::string path;
    FileEventType type;
};

class FileWatcher {
public:
    [[nodiscard]] static auto create(std::string_view watch_path)
        -> Result<std::unique_ptr<FileWatcher>>;

    virtual ~FileWatcher();

    virtual auto poll_events() -> std::vector<FileEvent> = 0;
    virtual auto start() -> void = 0;
    virtual auto stop() -> void = 0;

    FileWatcher(const FileWatcher&) = delete;
    auto operator=(const FileWatcher&) -> FileWatcher& = delete;
    FileWatcher(FileWatcher&&) = delete;
    auto operator=(FileWatcher&&) -> FileWatcher& = delete;

protected:
    FileWatcher() = default;
};

class NullFileWatcher final : public FileWatcher {
public:
    auto poll_events() -> std::vector<FileEvent> override { return {}; }
    auto start() -> void override {}
    auto stop() -> void override {}
};

} // namespace buddd::engine
```

### 11. InotifyFileWatcher (`src/engine/asset/file_watcher_inotify.h` / `.cpp`)

- Implemented only under `#ifdef __linux__`.
- Constructor opens inotify fd via `inotify_init1(IN_NONBLOCK | IN_CLOEXEC)`.
- `start()` spawns a `std::thread` that reads from inotify fd in a loop. Stores events in a `std::queue<FileEvent>` protected by `std::mutex`. Uses blocking read with a configurable timeout (or uses `ppoll` on the fd with a timeout for wake-on-shutdown).
- `stop()` sets `std::atomic<bool> running_` to false, writes a byte to a self-pipe (or uses `inotify_rm_watch` + close) to unblock the read thread, joins the thread.
- `poll_events()` locks mutex, swaps queue into local vector, returns it.
- `create()` static: on Linux, attempts to create an `InotifyFileWatcher`. If `inotify_init1` fails, returns `Unsupported` error. On non-Linux, returns `Unsupported`.
- Recursive directory watching: uses `inotify_add_watch` on each subdirectory. Currently watches only the top-level directory (recursive watching is a future enhancement; for V1, only direct children of `watch_path` are monitored). If `inotify_add_watch` fails for a subdirectory, a warning is logged and coverage is reduced for that subdirectory.

### 12. AssetManager (`src/engine/asset/asset_manager.h` / `.tpp` / `.cpp`)

```cpp
#pragma once

#include "error.h"
#include "asset/asset.h"
#include "asset/dependency_map.h"
#include "asset/texture_asset.h"
#include "asset/material_asset.h"
#include "render/shader_program.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace buddd::engine {

class RenderDevice;
class FileWatcher;

// ShaderProgramKey (defined as described above)

class AssetManager {
public:
    [[nodiscard]] static auto create(RenderDevice& device, std::string_view base_path)
        -> Result<std::unique_ptr<AssetManager>>;

    ~AssetManager();

    template<typename T>
    [[nodiscard]] auto create(std::string_view id) -> Result<std::shared_ptr<T>>;

    [[nodiscard]] auto create_texture(std::string_view id) -> Result<std::shared_ptr<TextureAsset>>;
    [[nodiscard]] auto create_material(std::string_view id) -> Result<std::shared_ptr<MaterialAsset>>;

    auto clear() -> void;
    auto base_path() const noexcept -> std::string_view;
    auto poll_file_events() -> void;
    auto set_file_watcher_enabled(bool enabled) -> void;

    // Test-only accessors (under BUDDD_TESTING only, never in release builds).
#ifdef BUDDD_TESTING
    auto get_dependency_map() const -> const DependencyMap&;
    auto testing_shader_programs() const noexcept
        -> const std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>>&;
    void testing_inject_file_event(const FileEvent& event);
#endif

    AssetManager(const AssetManager&) = delete;
    auto operator=(const AssetManager&) -> AssetManager& = delete;
    AssetManager(AssetManager&&) = delete;
    auto operator=(AssetManager&&) -> AssetManager& = delete;

private:
    AssetManager(RenderDevice& device, std::string base_path);

    // Resolve a path from YAML. If absolute, return as-is.
    // If relative, resolve against the working directory.
    // This is a simple helper that checks the first character for '/'.
    static auto resolve_path(std::string_view path) -> std::string;

    // Read entire file into string. Returns IoFailed on error.
    static auto read_file(const std::string& path) -> Result<std::string>;

    // Internal: load a TextureAsset (non-template version).
    auto load_texture(const std::string& id, const std::string& yaml_path) -> Result<std::shared_ptr<TextureAsset>>;

    // Internal: load a MaterialAsset (non-template version).
    auto load_material(const std::string& id, const std::string& yaml_path) -> Result<std::shared_ptr<MaterialAsset>>;

    // Hot-reload handlers.
    auto handle_yaml_change(const std::string& changed_path) -> void;
    auto handle_source_change(const std::string& changed_path) -> void;

    RenderDevice& device_;
    std::string base_path_;

    // Cache: asset_id -> shared_ptr<Asset>
    std::unordered_map<std::string, std::shared_ptr<Asset>> cache_;

    // Shader program deduplication map
    std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>> shader_programs_;

    // Dependency tracking (bidirectional)
    DependencyMap dependency_map_;

    // File watcher (inotify on Linux, NullFileWatcher otherwise)
    std::unique_ptr<FileWatcher> file_watcher_;
    bool file_watcher_enabled_ = true;
};

} // namespace buddd::engine

#include "asset_manager.tpp"  // Template implementations
```

**`asset_manager.tpp`** — contains the `create<T>()` template method:

```cpp
template<typename T>
auto AssetManager::create(std::string_view id) -> Result<std::shared_ptr<T>> {
    // Compile-time guard: T must be TextureAsset or MaterialAsset
    static_assert(std::is_same_v<T, TextureAsset> || std::is_same_v<T, MaterialAsset>,
        "AssetManager::create<T>() is only supported for TextureAsset and MaterialAsset");

    // 1. Validate id is not empty
    if (id.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Asset ID must not be empty");
    }

    // 2. Check cache
    auto it = cache_.find(std::string(id));
    if (it != cache_.end()) {
        auto* ptr = dynamic_cast<T*>(it->second.get());
        if (!ptr) {
            return make_error(Error::Category::InvalidArgument,
                "Cached asset type mismatch for '" + std::string(id) + "'");
        }
#ifndef NDEBUG
        std::cerr << "[Asset] Cache hit: " << id << "\n";
#endif
        return std::shared_ptr<T>(it->second, ptr);
    }

    // 3. Compute YAML path
    auto yaml_path = base_path_ + "/" + std::string(id) + ".yaml";

    // 4. Delegate to type-specific loader
    if constexpr (std::is_same_v<T, TextureAsset>) {
        return load_texture(std::string(id), yaml_path);
    } else if constexpr (std::is_same_v<T, MaterialAsset>) {
        return load_material(std::string(id), yaml_path);
    }
}
```

**`asset_manager.cpp`** key behaviors:

**`create()` factory:**
```cpp
auto AssetManager::create(RenderDevice& device, std::string_view base_path)
    -> Result<std::unique_ptr<AssetManager>>
{
    if (base_path.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "AssetManager base path must not be empty");
    }

    // Verify the base path directory exists (basic check)
    // (optional — can be omitted for V1; loading will fail on first access)

    auto mgr = std::unique_ptr<AssetManager>(
        new AssetManager(device, std::string(base_path)));

    // Create and start FileWatcher
#ifdef __linux__
    if (auto watcher = FileWatcher::create(mgr->base_path_)) {
        mgr->file_watcher_ = std::move(*watcher);
        mgr->file_watcher_->start();
#ifndef NDEBUG
        std::cerr << "[FileWatcher] Monitoring: " << mgr->base_path_ << "\n";
#endif
    } else {
        std::cerr << "[FileWatcher] Failed to create: "
                  << watcher.error().message << " — falling back to NullFileWatcher\n";
        mgr->file_watcher_ = std::make_unique<NullFileWatcher>();
    }
#else
    mgr->file_watcher_ = std::make_unique<NullFileWatcher>();
#endif

    return mgr;
}
```

**`load_texture()` — implements spec pseudo-code:**
1. Parse YAML via `YAML::LoadFile(yaml_path)` with try-catch (exception safety).
2. Validate `type` field equals `"Texture"` (else `InvalidArgument`).
3. Validate `version` field (present and `== 1`, or absent/default to 1; else `Unsupported`).
4. Read `source` field. If missing/empty, return `InvalidArgument`.
5. Resolve source path via `resolve_path()`.
6. `Image::load(source_path)` — propagate error.
7. `device_.create_texture(*image)` — propagate error.
8. Convert `unique_ptr<Texture>` to `shared_ptr<Texture>`.
9. Parse `settings` fields (wrap_s, wrap_t, min_filter, mag_filter, generate_mipmaps) — validate enums but DO NOT apply (spec A-12).
10. Create `TextureAsset` wrapper.
11. Cache in `cache_[id]`, record dependency on yaml_path and source_path.
12. Log success (debug builds only).
13. Return `shared_ptr<TextureAsset>`.

**`load_material()` — implements spec pseudo-code:**
1. Parse YAML with exception safety.
2. Validate `type == "Material"`.
3. Validate `version == 1`.
4. Read `shaders.vertex` and `shaders.fragment`. If either missing, return `InvalidArgument`.
5. Resolve shader paths.
6. Read shader source files via `read_file()`. Propagate `IoFailed`.
7. Deduplicate shader program by `(vert_path, frag_path)`:
   - Check `shader_programs_` map.
   - If found: retrieve existing `shared_ptr<ShaderProgram>`.
   - If not found: create shaders via `device_.create_shader()`, create `ShaderProgram` via `ShaderProgram::create()`, store in map.
8. Create material via `device_.create_material(shader_program)`.
9. Parse `textures` map: for each entry, call `create<TextureAsset>(tex_id)` recursively, call `material->set_texture(slot, texture)`.
10. Parse `constants` map: for each entry, try `material->set_uniform(name, value.as<float>())`. If `as<float>()` throws or `set_uniform` fails, log warning and continue.
11. Create `MaterialAsset` wrapper.
12. Cache, record dependencies (yaml_path, vert_path, frag_path).
13. Return `shared_ptr<MaterialAsset>`.

**Explicit instantiations** at the bottom of `asset_manager.cpp`:
```cpp
template auto AssetManager::create<TextureAsset>(std::string_view id) -> Result<std::shared_ptr<TextureAsset>>;
template auto AssetManager::create<MaterialAsset>(std::string_view id) -> Result<std::shared_ptr<MaterialAsset>>;
```

**`poll_file_events()` flow:**
1. If `file_watcher_` is null or disabled, return immediately.
2. Call `file_watcher_->poll_events()` to get `std::vector<FileEvent>`.
3. For each `FileEvent`:
   a. Look up `dependency_map_.get_dependents(event.path)`.
   b. If no dependents, skip (no assets track this file).
   c. For each dependent `asset_id`:
      - Determine if it's a YAML change or a source file change:
        - If `event.path` ends with `.yaml`: it's a YAML change → call `handle_yaml_change(asset_id)`.
        - Else: it's a source change (image or shader) → call `handle_source_change(asset_id, event.path)`.

**`handle_yaml_change(asset_id)`:**
1. Log `"[Asset] Hot-reload: " + id + " (YAML changed)"`.
2. Determine type of old cached asset (TextureAsset or MaterialAsset) to know which loader to call.
3. Try to load new asset: if old asset was `TextureAsset`, call `load_texture(id, yaml_path)`. If `MaterialAsset`, call `load_material(id, yaml_path)`.
4. On success: remove old `cache_[asset_id]` entry, remove old dependency entries via `dependency_map_.remove_asset(asset_id)`, insert new asset into cache, add new dependency entries.
5. On failure: log warning and retain old asset in cache (no change).

**`handle_source_change(asset_id, changed_path)`:**
1. Look up current type of cached asset:
   - If `TextureAsset` (image source changed):
     - Log `"[Asset] Hot-reload: " + id + " (source image changed)"`.
     - Reload image via `Image::load(changed_path)`. On failure: log warning, retain old texture.
     - Create new GPU texture via `device_.create_texture(*new_image)`. On failure: log warning, retain old texture.
     - Extract GLuint handle from new texture via `new_tex->gl_handle()`.
     - Call `existing_texture->replace_gl_handle(new_handle)` to swap handle in-place.
     - Call `new_tex->release_gl_handle()` to clear the temporary texture's internal
       handle, preventing double-deletion when the temporary `unique_ptr<Texture>`
       goes out of scope and its destructor fires.
   - If `MaterialAsset` (shader source changed):
     - Log `"[Asset] Hot-reload: " + id + " (shader changed)"`.
     - Re-read vertex and fragment source files.
     - Create new shader objects via `device_.create_shader()`.
     - Create new `ShaderProgram` via `ShaderProgram::create()`. On failure: log warning, retain old program.
     - Mutate the existing ShaderProgram **in-place** via move assignment:
       `*existing_program = std::move(*new_program);`
       The move assignment operator deletes the old GL handle (or increments the
       generation counter in headless mode) and transfers the new handle/state
       from the temporary. The source ShaderProgram is left with `handle_ = 0`
       so its destructor is a no-op.
     - No need to update individual Materials or the `shader_programs_` map.
       Materials hold `shared_ptr<ShaderProgram>` to the same object — the
       object's internal state has been mutated, so all holders see the new
       handle at bind time automatically.

**`clear()`:**
1. Clear `cache_`, `shader_programs_`, `dependency_map_`.
2. Log `"[Asset] Cache cleared (" + count + " assets)"`.

### 13. EngineService changes (`engine_service.h` / `.cpp`)

**`engine_service.h` changes:**
- Add forward declaration: `class AssetManager;` (inside namespace).
- Add member AFTER `device_`: `std::unique_ptr<AssetManager> asset_manager_;`
- Add accessor: `auto assets() noexcept -> AssetManager&;`

**`engine_service.cpp` changes:**
- Add `#include "asset/asset_manager.h"`.
- In `EngineService::create()`:
  ```cpp
  // After device is created, create AssetManager
  auto asset_mgr = AssetManager::create(engine_service.device(), "assets");
  if (!asset_mgr) {
      return std::unexpected(asset_mgr.error());
  }
  engine_service.asset_manager_ = std::move(*asset_mgr);
  ```
  The `engine_service` object already exists at this point (created via `new EngineService(...)` before the `AssetManager` is constructed). So the AssetManager is assigned AFTER the EngineService is constructed.

  Implementation approach: construct the EngineService first, then assign `asset_manager_` via member pointer:
  ```cpp
  auto engine = std::unique_ptr<EngineService>(
      new EngineService(std::move(*platform), std::move(*window), std::move(*device)));
  
  auto asset_mgr = AssetManager::create(engine->device(), "assets");
  if (!asset_mgr) return std::unexpected(asset_mgr.error());
  engine->asset_manager_ = std::move(*asset_mgr);
  
  return engine;
  ```

- Implement accessor: `auto EngineService::assets() noexcept -> AssetManager& { return *asset_manager_; }`

### 14. CMake changes (`src/engine/CMakeLists.txt`)

Add after the stb FetchContent block:

```cmake
# ----- yaml-cpp (YAML parsing for asset metadata) -----
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
              -DYAML_CPP_BUILD_TESTS=OFF
              -DYAML_CPP_BUILD_TOOLS=OFF
              -DYAML_CPP_INSTALL=OFF
)
FetchContent_MakeAvailable(yaml-cpp)
```

Add include directory to PRIVATE:
```cmake
target_include_directories(buddd_engine PRIVATE
    ${stb_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
)
```

Add link library as PRIVATE:
```cmake
target_link_libraries(buddd_engine PRIVATE
    yaml-cpp
)
```

### 15. YAML parsing error handling

All `YAML::LoadFile()` calls MUST be wrapped in try-catch:

```cpp
YAML::Node yaml;
try {
    yaml = YAML::LoadFile(yaml_path);
} catch (const YAML::Exception& e) {
    return make_error(Error::Category::IoFailed,
        "YAML parse error in " + yaml_path + ": " + e.what());
} catch (const std::exception& e) {
    return make_error(Error::Category::IoFailed,
        "Unexpected error parsing " + yaml_path + ": " + e.what());
}
```

### 16. Path resolution

`resolve_path()` behavior:
- If path starts with `/`, treat as absolute, return as-is.
- Otherwise, treat as relative to the working directory. For simplicity, return the path as-is (relative paths are resolved by the OS relative to CWD).

### 17. file_watcher.cpp implementation details

`FileWatcher::create()` factory and `~FileWatcher()` destructor go in `src/engine/asset/file_watcher.cpp`:

```cpp
// file_watcher.cpp
#include "asset/file_watcher.h"
#include "asset/file_watcher_inotify.h"  // for InotifyFileWatcher

namespace buddd::engine {

FileWatcher::~FileWatcher() = default;

auto FileWatcher::create(std::string_view watch_path)
    -> Result<std::unique_ptr<FileWatcher>>
{
    if (watch_path.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "FileWatcher watch path must not be empty");
    }

#ifdef __linux__
    auto watcher = std::make_unique<InotifyFileWatcher>(watch_path);
    // InotifyFileWatcher constructor opens inotify fd; if it fails,
    // the factory returns an error. The caller falls back to NullFileWatcher.
    return std::unique_ptr<FileWatcher>(std::move(watcher));
#else
    return make_error(Error::Category::Unsupported,
        "FileWatcher is only supported on Linux");
#endif
}

} // namespace buddd::engine
```

The factory does NOT start the watcher thread — the caller (AssetManager::create) calls `start()` separately after construction. On non-Linux platforms, the factory returns `Unsupported` and the caller falls back to `NullFileWatcher`.

### 18. file_watcher_inotify.cpp implementation details

- `InotifyFileWatcher` class inherits `FileWatcher`.
- Private members:
  - `int inotify_fd_{-1}`
  - `int watch_fd_{-1}`
  - `std::thread watcher_thread_`
  - `std::mutex queue_mutex_`
  - `std::queue<FileEvent> event_queue_`
  - `std::atomic<bool> running_{false}`
  - `std::string watch_path_`
- `start()`:
  - Sets `running_ = true`.
  - Spawns thread: reads from `inotify_fd_` in a loop via `read()` (blocking). On each event, parses `inotify_event`, constructs `FileEvent`, pushes to queue under lock. If `running_` becomes false, stops reading.
- `stop()`:
  - Sets `running_ = false`.
  - Writes to a self-pipe (or closes the inotify fd) to unblock the `read()` in the thread.
  - Joins the thread.
- Watches the `watch_path_` directory with `IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_TO`.
- `poll_events()`: locks mutex, swaps queue contents into local vector, returns vector.
- `~InotifyFileWatcher()`: calls `stop()` if running, closes `inotify_fd_`.

### 19. Header inclusion and yaml-cpp privacy

- yaml-cpp includes (`#include <yaml-cpp/yaml.h>`) MUST only appear in `.cpp` files (specifically `asset_manager.cpp`). Public headers (`asset_manager.h`, `texture_asset.h`, etc.) MUST NOT include yaml-cpp.
- Forward declarations and opaque types (never yaml-cpp types) are used in headers.
- This is enforced by the spec's AC-029 requirement.

### 20. Testing include note

- `asset_manager.h` forward-declares `FileWatcher` but the `testing_inject_file_event(const FileEvent&)` method under `BUDDD_TESTING` requires the full `FileEvent` struct definition from `file_watcher.h`. Either:
  - Include `"asset/file_watcher.h"` unconditionally in `asset_manager.h` (FileWatcher is always part of the system), or
  - Include it only inside the `#ifdef BUDDD_TESTING` block.
  - The first option (unconditional include) is preferred for simplicity.

### 21. Demo app and sample assets

**Sample asset YAML files:**

`assets/textures/demo_brick.yaml`:
```yaml
type: Texture
version: 1
source: assets/textures/demo_brick.png
settings:
  wrap_s: repeat
  wrap_t: repeat
  min_filter: linear
  mag_filter: linear
```

`assets/materials/demo_cube.yaml`:
```yaml
type: Material
version: 1
shaders:
  vertex: assets/shaders/demo.vert
  fragment: assets/shaders/demo.frag
textures:
  u_tex: textures/demo_brick
```

**`assets/shaders/demo.vert`:**
```glsl
#version 450 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord;
out vec2 v_texcoord;
uniform mat4 u_mvp;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_texcoord = a_texcoord;
}
```

**`assets/shaders/demo.frag`:**
```glsl
#version 450 core
in vec2 v_texcoord;
out vec4 frag_color;
uniform sampler2D u_tex;
void main() {
    frag_color = texture(u_tex, v_texcoord);
}
```

**`assets/textures/demo_brick.png`:** A small valid PNG file (e.g. 2×2 or 4×4 checker in RGBA). May be generated programmatically or created with any image tool. Must be loadable by `Image::load()`.

**`src/cmd/apps/asset_demo_app.h`:**
```cpp
#pragma once

#include "app.h"

#include <memory>

namespace buddd::engine {
class AssetManager;
} // namespace buddd::engine

namespace buddd::cmd::app {

class AssetDemoApp final : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 asset-demo", 1024, 768};
    }

    [[nodiscard]] auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> override;

    auto render(buddd::engine::RenderDevice& device, int frame) -> void override;

private:
    std::unique_ptr<buddd::engine::World> world_;
    std::unique_ptr<buddd::engine::RenderSystem> render_system_;
    std::unique_ptr<buddd::engine::Entity> entity_;
    std::shared_ptr<buddd::engine::Material> material_;
    std::unique_ptr<buddd::engine::AssetManager> asset_manager_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace buddd::cmd::app
```

**`src/cmd/apps/asset_demo_app.cpp` — `setup()` flow:**
1. Create an `AssetManager` via `AssetManager::create(device, "assets")`. If creation fails, propagate the error.
2. Store the created `AssetManager` in `asset_manager_` member.
3. Load material YAML: `TRY(asset_manager_->create<MaterialAsset>("materials/demo_cube"))` → get `shared_ptr<Material>` via `->material()`.
4. Create cube mesh with UV coordinates (follow `TexturedCubeApp` pattern: `TexturedCubeVertex` struct with `px,py,pz,tx,ty`; 24 vertices, 36 indices).
5. Create `World`, `Entity`, `Camera` (perspective 60° FOV, look-at from `(3,2,3)` to origin).
6. Create vertex buffer via `device.create_vertex_buffer()` or `Model::create_indexed()`.
7. Assign material to model.
8. Attach `MeshRenderer` to entity.
9. Create `RenderSystem`.
10. Store `start_time_` for rotation animation.

**`src/cmd/apps/asset_demo_app.cpp` — `render()` flow:**
1. Compute elapsed time.
2. Rotate entity around Y axis at 0.5 rad/s.
3. Call `render_system_->render_scene()`.

**`src/cmd/commands/demo_command.h`:**
```cpp
#pragma once

namespace buddd::cmd {

class DemoCommand {
public:
    /// Parses the subcommand ("asset-demo") and runs the AssetDemoApp.
    /// Follows the existing help_command.h / version_command.h pattern.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
```

**`src/cmd/commands/demo_command.cpp`:**
- Parse subcommand: `buddd demo asset-demo`.
- Create `AssetDemoApp` instance.
- Call `bc::run_app(app, args)` with `--frame 120` (or equivalent non-interactive 120-frame run).
- Pattern: similar to how `main.cpp` registers scenes under the `run` command, but this is a standalone top-level `demo` command for simplicity.

## Required tests

All tests go in `tests/asset_manager_tests.cpp`. Use the `make_headless_engine()` helper pattern from `tests/texture_tests.cpp`.

### Test grouping by AC traceability:

| # | Test case | AC | Priority |
|---|-----------|----|----------|
| 1 | `TextureAsset stores and returns texture` | AC-002 | P1 |
| 2 | `MaterialAsset stores and returns material` | AC-003 | P1 |
| 3 | `AssetManager::create returns error for empty base_path` | AC-011 (variant) | P1 |
| 4 | `create<TextureAsset> with valid YAML loads texture` | AC-005 | P1 |
| 5 | `create<TextureAsset> twice returns cached instance` | AC-007 | P1 |
| 6 | `create<MaterialAsset> with valid YAML loads material` | AC-006 | P1 |
| 7 | `create<MaterialAsset> with type:Texture YAML returns InvalidArgument` | AC-008 | P1 |
| 8 | `create<TextureAsset> with type:Material YAML returns InvalidArgument` | AC-009 | P1 |
| 9 | `create<T> with unsupported version returns Unsupported` | AC-010 | P1 |
| 10 | `create<T> with nonexistent YAML returns IoFailed` | AC-011 | P1 |
| 11 | `create<TextureAsset> with missing source field returns InvalidArgument` | AC-012 | P1 |
| 12 | `create<MaterialAsset> with missing shaders field returns error` | AC-013 | P1 |
| 13 | `Shader deduplication: two materials share ShaderProgram` | AC-014 | P1 |
| 14 | `clear() removes all cached assets` | AC-015 | P2 |
| 15 | `poll_file_events() safe to call multiple times on headless` | AC-016 | P2 |
| 16 | `FileWatcher is NullFileWatcher on headless` | AC-017 | P2 |
| 17 | `Texture settings parsed but not applied` | AC-018 | P2 |
| 18 | `EngineService has assets() accessor` | AC-019 | P1 |
| 19 | `AssetManager::create failure propagates through EngineService` | AC-020 | P1 |
| 20 | `yaml-cpp compiles and links` | AC-021 | P1 |
| 21 | `Hot-reload pipeline: shader FileEvent triggers recompile (headless)` | AC-023 | P2 |
| 22 | `Shader recompilation failure retains old program` | AC-024 | P2 |
| 23 | `Dependency map tracks texture source path` | AC-026 | P2 |
| 24 | `Dependency map tracks material YAML + shader paths` | AC-027 | P2 |
| 25 | `Empty-ID returns InvalidArgument` | Edge case | P2 |
| 26 | `Shader hot-reload changes GPU handle (OpenGL only, requires BUDDD_HAS_DISPLAY)` | AC-025 | P3 |
| 27 | `Circular dependency between assets` | AC-014 (dedup already covers) | P3 |
| 28 | `AssetManager demo app compiles and runs. Loads a Material from YAML, renders a textured cube.` | AC-032 | P1 |

### Test implementation patterns:

**Test 4** (load texture):
```cpp
TEST_CASE("create<TextureAsset> with valid YAML loads texture", "[asset][headless]") {
    auto engine = make_headless_engine();
    auto& assets = engine->assets();
    // Use a pre-created YAML test asset at a path relative to CWD
    // The test runner's CWD is the build directory; test assets are in
    // tests/assets/ relative to the project root. Use a helper to compute
    // the path or copy test assets to the build directory.
    // For headless tests, the YAML files must be accessible.
    
    auto tex = assets.create<TextureAsset>("textures/test_texture");
    REQUIRE(tex.has_value());
    REQUIRE((*tex)->texture() != nullptr);
    REQUIRE((*tex)->texture()->width() > 0);
    REQUIRE((*tex)->texture()->height() > 0);
}
```

**Test 13** (shader dedup):
```cpp
TEST_CASE("Shader deduplication: two materials share ShaderProgram", "[asset][headless]") {
    auto engine = make_headless_engine();
    auto& assets = engine->assets();
    
    auto mat1 = assets.create<MaterialAsset>("materials/test_material");
    REQUIRE(mat1.has_value());
    
    // Create second material referencing the same shader pair
    auto mat2 = assets.create<MaterialAsset>("materials/test_material_same_shaders");
    REQUIRE(mat2.has_value());
    
    // Verify deduplication via AssetManager's testing_shader_programs() accessor.
    // The map key is (vertex_path, fragment_path). With two materials sharing
    // the same shader pair, the map should contain exactly 1 ShaderProgram.
    const auto& programs = assets.testing_shader_programs();
    REQUIRE(programs.size() == 1);
    
    // The single ShaderProgram should be valid
    auto it = programs.begin();
    REQUIRE(it->second != nullptr);
    REQUIRE(it->second->is_valid());
}
```

**Test 13b** (shader dedup via material accessor):
A secondary test can also verify pointer equality by having `MaterialHeadless` optionally store the `shared_ptr<ShaderProgram>` passed to its constructor. If `Material::testing_shader_program()` returns non-null for headless materials, the test checks that both materials return the same pointer. This is optional — the `testing_shader_programs()` map accessor above is the primary verification path.

**Test 21** (hot-reload pipeline):
```cpp
TEST_CASE("Hot-reload pipeline: shader FileEvent triggers recompile", "[asset][headless]") {
    auto engine = make_headless_engine();
    auto& assets = engine->assets();
    
    auto mat = assets.create<MaterialAsset>("materials/test_material");
    REQUIRE(mat.has_value());
    
    // Record the old generation counter from the ShaderProgram
    const auto& programs_before = assets.testing_shader_programs();
    REQUIRE(programs_before.size() == 1);
    auto old_gen = programs_before.begin()->second->testing_handle();
    
    // Inject a synthetic FileEvent for the shader source file
    // The path must match what the dependency map tracks for test_material.
    auto shader_path = std::string(CWD) + "/tests/assets/shaders/test.vert";
    assets.testing_inject_file_event({shader_path, FileEventType::Modified});
    assets.poll_file_events();
    
    // Verify the ShaderProgram's generation changed
    const auto& programs_after = assets.testing_shader_programs();
    REQUIRE(programs_after.size() == 1);
    auto new_gen = programs_after.begin()->second->testing_handle();
    REQUIRE(new_gen != old_gen);
}
```

Note: This test relies on the `testing_inject_file_event()` and `testing_shader_programs()` accessors on `AssetManager`, both gated by `#ifdef BUDDD_TESTING`. The test must also define a constant `CWD` pointing to the project root, or use a helper to resolve test asset paths. Follow the path resolution pattern from the existing texture tests.

## Edge cases

All edge cases from the spec (lines 1012-1039) must be handled per the "Expected behaviour" column. The implementation contract requires the following edge cases to have explicit handling in the code:

| Edge case | Required handling |
|-----------|-------------------|
| Empty asset ID | `create<T>("")` returns `InvalidArgument` before any disk access. |
| Path traversal in ID (`../`) | Allowed at filesystem level. No sanitisation for V1. The path is simply `<base_path>/<id>.yaml`. |
| Same YAML file via different IDs | Cache uses ID as key. Different IDs → two cache entries, two loads. Documented as application error. |
| Texture source path absolute vs relative | Both supported. Relative resolved from CWD. |
| Material references nonexistent texture ID | Recursive `create<TextureAsset>` fails → error propagates → material not cached. |
| Material references unloaded-but-valid texture ID | `create<TextureAsset>` triggers lazy load. Dependencies resolved transitively. |
| Two materials same texture ID | Both call `create<TextureAsset>` → texture loaded once, cached, both get same `shared_ptr<Texture>`. |
| Non-numeric constant value | `as<float>()` may throw. Caught, warning logged, constant skipped. Material still loaded. |
| Unknown YAML fields | Silently ignored (yaml-cpp default). |
| Missing `type` field | Defaults to empty string. Validation fails → `InvalidArgument`. |
| `create<T>` before watcher started | Loading works regardless of watcher state. |
| `poll_file_events()` called on non-Linux | `NullFileWatcher` returns empty vector. No thread. |
| Multiple rapid file same file changes | Each produces a separate event. V1 processes them all (no debounce). |
| File deletion event | Logged. No automatic cache eviction. If file missing when reload attempted → `IoFailed` warning, old asset retained. |
| `create<T>` with unspecialised type | `static_assert` fires at compile time. |
| `assets/` directory does not exist | `AssetManager::create()` must handle gracefully — either check existence at construction or let first load fail with `IoFailed`. V1: let first load fail. |
| Empty shader source | `create_shader` returns `ShaderCompilationFailed` → propagates from `load_material`. |
| YAML parse error (syntax) | Exception caught → `IoFailed` error. |
| `poll_file_events()` with no events | Returns immediately. |
| `inotify_add_watch` fails | Warning logged. Subdirectory not monitored. |

## Security impact

- yaml-cpp exception safety: ALL `YAML::LoadFile()` calls MUST be wrapped in try-catch to prevent yaml-cpp exceptions from escaping the engine boundary.
- No network access — all file I/O is local.
- Path traversal (`../`) is allowed at the filesystem level (matching existing risk profile).
- No custom YAML tag parsing: yaml-cpp is used in default mode (scalar/map/sequence only).
- All GPU resource creation goes through `RenderDevice` abstraction (CONST-001).
- FileWatcher uses only inotify (no exec, no network). Thread joins cleanly on shutdown.
- Headless mode requires no GPU access (CI-safe).

## Data and migration impact

None. No existing data formats are changed. New YAML files are consumed but never written by the engine.

## API compatibility impact

- **`RenderDevice::create_material()`** gains a new overload `(shared_ptr<ShaderProgram>)`. Existing overload is UNCHANGED — all existing callers continue to compile and work.
- **`EngineService`** gains `assets() -> AssetManager&` accessor. Existing `device()` is unchanged.
- **`Texture`** base class gains three new virtual methods with default implementations: `replace_gl_handle(uint32_t)`, `gl_handle()`, and `release_gl_handle()`. Existing `Texture` subclasses not overriding these get the defaults (no-op), preserving ABI compatibility for classes outside the known set.
- All new types are in new files — no existing public API is removed or modified.
- yaml-cpp is a PRIVATE link dependency — no public headers expose yaml-cpp types. No consumer-facing API change.

## Documentation impact

- `docs/specs/asset-manager/implementation-contract.md` — this file.
- `tests/assets/` directory — new test asset files.
- If ADR-015 (yaml-cpp dependency) is created, note that in `docs/adr/`.
- Existing README or wiki pages that describe the engine's dependency list should mention yaml-cpp as a FetchContent dependency.

## ADR impact

- A new ADR (ADR-015 or similar) should be created documenting the yaml-cpp dependency decision, following the project's ADR process. However, this is out of scope for this implementation contract — the spec mentions it as a note (spec line 846). The implementation-contract agent does NOT create ADRs; the adr-agent handles that. This contract simply notes that an ADR is expected.

## Constitution impact

None. All new code complies with CONST-001 (no platform/graphics types leak outside `src/engine/`), CONST-002 (tests for all new code), and all existing ADRs.

## Done criteria

The implementation is complete when ALL of the following are verifiable:

- [ ] **DC-1**: `src/engine/asset/asset.h` exists with abstract `Asset` base class (virtual destructor, deleted copy/move, protected default constructor).
- [ ] **DC-2**: `src/engine/asset/texture_asset.h` exists with `TextureAsset final : Asset`, `texture() -> const shared_ptr<Texture>&` accessor.
- [ ] **DC-3**: `src/engine/asset/material_asset.h` exists with `MaterialAsset final : Asset`, `material() -> const shared_ptr<Material>&` accessor.
- [ ] **DC-4**: `src/engine/asset/asset_manager.h` exists with ALL methods declared per the spec (create factory, template create<T>, create_texture, create_material, clear, base_path, poll_file_events, set_file_watcher_enabled). Template body in `asset_manager.tpp` included at bottom of header.
- [ ] **DC-5**: `src/engine/asset/asset_manager.cpp` exists with explicit instantiations for `create<TextureAsset>` and `create<MaterialAsset>`. YAML parsing uses try-catch.
- [ ] **DC-6**: `src/engine/render/shader_program.h` / `.cpp` exist with `ShaderProgram` class. OpenGL linking in display mode; headless stub in headless mode. `replace_handle(GLuint)`, `release_handle()`, and `testing_handle()` (under `BUDDD_TESTING`) are present. Move assignment deletes old handle and clears source handle.
- [ ] **DC-7**: `src/engine/render/render_device.h` has new `create_material(shared_ptr<ShaderProgram>)` pure virtual overload.
- [ ] **DC-8**: `RenderDeviceOpenGL` and `RenderDeviceHeadless` both implement the new overload.
- [ ] **DC-9**: `MaterialOpenGL` has a new constructor taking `shared_ptr<ShaderProgram>`, stores it, and dereferences it at `bind()` time. Existing `MaterialOpenGL(GLuint)` constructor unchanged.
- [ ] **DC-10**: `src/engine/render/texture.h` has `replace_gl_handle(uint32_t)`, `gl_handle()`, and `release_gl_handle()` virtual methods. `TextureOpenGL` overrides all three; `TextureHeadless` does not (inherits no-op defaults).
- [ ] **DC-11**: `src/engine/asset/file_watcher.h` exists with `FileWatcher` abstract base and `NullFileWatcher`. `FileEvent` and `FileEventType` defined. `file_watcher.cpp` exists with `FileWatcher::create()` factory and `~FileWatcher()` destructor implementations.
- [ ] **DC-12**: `src/engine/asset/file_watcher_inotify.h` / `.cpp` exist with `InotifyFileWatcher` (guarded by `__linux__`). Thread-safe event queue, proper start/stop.
- [ ] **DC-13**: `src/engine/asset/dependency_map.h` / `.cpp` exist with bidirectional dependency tracking.
- [ ] **DC-14**: `EngineService` has `assets() -> AssetManager&` accessor. `asset_manager_` member declared AFTER `device_`. AssetManager constructed in `EngineService::create()` after RenderDevice.
- [ ] **DC-15**: yaml-cpp added via FetchContent in `src/engine/CMakeLists.txt`. Linked PRIVATE.
- [ ] **DC-16**: No yaml-cpp includes in any public header in `src/engine/asset/`.
- [ ] **DC-17**: `tests/asset_manager_tests.cpp` exists with at least the following passing tests:
  - [ ] Loading a `TextureAsset` from valid YAML.
  - [ ] Loading a `TextureAsset` twice returns cached instance (same address).
  - [ ] Loading a `MaterialAsset` from valid YAML with shaders + textures + constants.
  - [ ] Type mismatch returns `InvalidArgument`.
  - [ ] Unsupported version returns `Unsupported`.
  - [ ] Missing YAML returns `IoFailed`.
  - [ ] Missing source/shaders fields return `InvalidArgument`.
  - [ ] Shader program deduplication (two materials share same `ShaderProgram`).
  - [ ] `clear()` reloads fresh.
  - [ ] `poll_file_events()` safe to call multiple times on headless.
  - [ ] `EngineService::assets()` returns non-null reference.
  - [ ] Hot-reload pipeline: synthetic FileEvent triggers ShaderProgram handle change (headless).
  - [ ] Shader recompilation failure retains old program (headless).
  - [ ] Dependency map correctly tracks source paths for textures and materials.
- [ ] **DC-18**: All tests pass with `ctest --preset debug` (or equivalent CMake test runner) in headless mode.
- [ ] **DC-19**: Build succeeds with no warnings (excluding yaml-cpp warnings, which may be treated as external).
- [ ] **DC-20**: Test asset files exist at `tests/assets/textures/`, `tests/assets/materials/`, `tests/assets/shaders/` with minimal valid content for testing.
- [ ] **DC-21**: Demo app files `src/cmd/apps/asset_demo_app.h`, `src/cmd/apps/asset_demo_app.cpp`, `src/cmd/commands/demo_command.h`, and `src/cmd/commands/demo_command.cpp` exist and compile.
- [ ] **DC-22**: Sample asset files `assets/textures/demo_brick.yaml`, `assets/materials/demo_cube.yaml`, `assets/shaders/demo.vert`, `assets/shaders/demo.frag`, and `assets/textures/demo_brick.png` exist with correct content.
- [ ] **DC-23**: `buddd demo asset-demo` runs without crash for 120 frames (or equivalently, the `AssetDemoApp` can be instantiated and renders at least one frame without error).
