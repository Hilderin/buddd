# SPEC-019 — Asset Manager

## Status

`Draft`

## Problem

The Buddd Engine currently has no centralised system for loading, caching, or managing content assets (textures, materials, shaders). Each asset must be loaded manually via low-level APIs:

- **Textures** are created by loading PNG files via `Image::load()` then calling `RenderDevice::create_texture()` with hardcoded paths.
- **Materials** are assembled programmatically: shader source strings are embedded in C++ code, materials are created via `RenderDevice::create_material()`, and textures are attached manually via `set_texture()`.
- **Shader files** exist on disk (`source assets`) but are never loaded from disk — they are embedded as C++ string literals (e.g., `phong_shaders.h`).
- **No asset identity**: there is no way to reference an asset by a stable ID, no caching, no deduplication — calling code has full ownership and must manage shared_ptr weaving manually.
- **No hot-reload**: changing a texture file or shader file requires a full rebuild and restart.
- **No dependency tracking**: if a material references a texture, changing the texture file has no effect on the material cache.

As the engine grows beyond demos, manual asset management becomes a significant friction point. A dedicated Asset Manager system provides:

- A uniform, ID-based API for loading assets (`create<Texture>("textures/brick")`).
- YAML-based metadata files that describe assets declaratively.
- Lazy loading: assets are loaded on first access, not pre-scanned at startup.
- Automatic caching and deduplication: loading the same asset ID twice returns the same `shared_ptr`.
- Shader program deduplication by (vertex_path, fragment_path) pair — the GL program is shared via a reference-counted `ShaderProgram` wrapper, while each Material retains its own uniform/texture state.
- A dependency graph that tracks which source files each asset depends on.
- A FileWatcher subsystem (Linux inotify) that hot-reloads assets when their YAML files or source files change on disk.

## Goals

- **Asset type system**: Define an abstract `Asset` base class with a closed set of concrete asset types (`TextureAsset`, `MaterialAsset`) for V1.
- **ID-based loading**: Provide `AssetManager::create<T>(asset_id) -> Result<std::shared_ptr<T>>` where `asset_id` maps to a relative path from `assets/` without extension. Also provide explicit `create_material(id)` and `create_texture(id)` convenience methods.
- **YAML metadata**: Each asset is defined by a YAML file (`.yaml` extension) in the `assets/` directory tree, with `type`, `version`, and type-specific fields.
- **Lazy loading**: Assets are loaded from disk on the first `create<T>()` call. No filesystem scan at startup.
- **Caching and deduplication**: Loaded assets are cached by ID. Subsequent `create<T>()` calls return the cached instance. The cache persists for the lifetime of the `AssetManager`.
- **Texture assets**: YAML -> parse source image path -> `Image::load()` -> `RenderDevice::create_texture()` -> cache.
- **Material assets**: YAML -> parse shader paths (vertex + fragment) -> load shader source -> create `Shader` objects -> create `Material` via `create_material()` -> resolve texture references by ID -> call `set_texture()` -> set constant overrides -> cache.
- **Shader program deduplication**: Compiled shader programs are deduplicated by (vertex_path, fragment_path) pair. Two materials with the same shader files share one reference-counted `ShaderProgram` wrapper, but each Material retains its own uniform/texture state.
- **FileWatcher subsystem** (Linux, all build types): A dedicated thread monitors the `assets/` directory tree via inotify and pushes change events to a thread-safe queue. The main thread drains the queue each frame (before render) and triggers hot-reloads. The user must call `poll_file_events()` explicitly (e.g., in the app loop).
- **Hot-reload flows** (via explicit `poll_file_events()` call by the user, e.g., once per frame):
  - YAML file changes -> reload asset metadata -> re-resolve dependencies -> update cached asset.
  - Source image file changes -> reload texture GPU object.
  - Shader file changes -> recompile shader programs -> update all materials using them.
- **Dependency tracking**: Maintain a reverse dependency map so that when a source file changes, all dependent assets are correctly reloaded.
- **EngineService integration**: The `AssetManager` is owned by `EngineService` (as `unique_ptr`) and is accessible via `engine_service.assets()`.
- **CMake integration**: yaml-cpp is added via FetchContent, following the existing pattern for SDL3, GLM, stb, and Catch2.
- **Headless testability**: All asset loading logic is testable with the Headless backend.
- **Textured material loading demo**: A demo that loads a Material from YAML (which references a Texture) and renders with it, replacing the current hardcoded `TexturedCubeApp` pattern.

## Non-goals

- No MaterialInstance system (Unreal-style material inheritance) — deferred to V2.
- No material blueprints, node graphs, or visual shader editing.
- No runtime asset registry or asset browser GUI.
- No texture packing, atlasing, or compression in the asset pipeline.
- No automatic asset import from DCC tools (Blender, Maya, etc.).
- No non-YAML asset metadata formats (JSON, TOML, etc. — YAML only for V1).
- No startup-time asset scan or index file generation.
- No async background loading (all asset loading is synchronous on the calling thread).
- No Windows/macOS FileWatcher support (Linux inotify only for V1).
- No network-based asset loading (all assets are local filesystem only).
- No asset streaming, LOD systems, or partial loading.
- glTF/glb model loading is handled by `ModelAsset` (see `.specs/sprint-2026-06/gltf-model-loading/spec.md`) — this spec covers only TextureAsset and MaterialAsset.
- No `assets/` directory creation or management — the directory is expected to exist.
- No validation that referenced source files remain on disk across hot-reload cycles.
- No shader include directive support (`#include` in GLSL is not processed by the engine).

## Actors

| Actor | Description |
|---|---|
| Application developer | Loads assets by ID via `AssetManager::create<T>(id)`. Writes YAML files for assets. Benefits from caching and deduplication. |
| Engine developer | Maintains the AssetManager, FileWatcher, and hot-reload subsystems. Extends asset types in future versions. |
| Asset content creator | Creates and edits YAML asset files and their referenced source files (PNG images, `.vert`/`.frag` shader files) in the `assets/` directory. |
| Build system | CMake + Ninja — adds yaml-cpp as a FetchContent dependency. New `.h` and `.cpp` files in `src/engine/` subdirectories are picked up by existing `file(GLOB_RECURSE)`. |
| Test suite | Catch2 v3 tests that verify asset loading, caching, YAML parsing, error propagation, dependency tracking, and hot-reload event processing — all in headless mode. |

## Key entities

| Entity | Description |
|---|---|
| `Asset` | Abstract base class for all asset types. Pure virtual destructor. Non-copyable, non-movable. |
| `TextureAsset` | Concrete asset wrapping a `std::shared_ptr<Texture>`. Loaded from a YAML file with `type: Texture`. |
| `MaterialAsset` | Concrete asset wrapping a `std::shared_ptr<Material>`. Loaded from a YAML file with `type: Material`. References textures by ID and shaders by file path. |
| `AssetManager` | Core class. Owns the asset cache, dependency map, shader deduplication map, and the FileWatcher. Provides `create<T>(id)` template method plus `create_texture(id)` and `create_material(id)` convenience methods. Creates GPU resources (textures, shaders, materials) via a `RenderDevice&`. Owned by `EngineService`. |
| `ShaderProgramKey` | Key type for shader program deduplication: a pair of (vertex_path, fragment_path) strings. |
| `ShaderProgram` | Reference-counted wrapper around a compiled GL shader program (`GLuint`). Multiple `Material` objects share the same `ShaderProgram` instance (via `shared_ptr<ShaderProgram>`) when they use the same vertex+fragment shader pair. Each Material retains separate uniform and texture state. **Defined in `src/engine/render/shader_program.h`** (render layer) because it exposes `GLuint` — this respects CONST-001 (no graphics types outside render layer). The AssetManager includes it cross-submodule (same library, `buddd_engine`). |
| `FileWatcher` | Linux inotify-based directory monitor. Runs a dedicated thread. Pushes `FileEvent` structs to a thread-safe queue. |
| `FileEvent` | Struct with `path`, `event_type` (Created, Modified, Deleted), and `timestamp`. |
| `DependencyMap` | Internal graph tracking asset_id -> set of source file paths, and source file path -> set of dependent asset IDs. |

## User-visible behavior

### 1. Asset base class and type system

An abstract `Asset` base class in namespace `buddd::engine`:

```cpp
// src/engine/asset/asset.h
#pragma once

namespace buddd::engine {

class Asset {
public:
    virtual ~Asset() = default;

    Asset(const Asset&) = delete;
    auto operator=(const Asset&) -> Asset& = delete;
    Asset(Asset&&) = delete;
    auto operator=(Asset&&) -> Asset& = delete;

protected:
    Asset() = default;
};

} // namespace buddd::engine
```

Concrete asset types:

- `TextureAsset` (in `src/engine/asset/texture_asset.h`, private): wraps a `std::shared_ptr<Texture>`. Provides `texture() -> const std::shared_ptr<Texture>&` accessor.
- `MaterialAsset` (in `src/engine/asset/material_asset.h`, private): wraps a `std::shared_ptr<Material>`. Provides `material() -> const std::shared_ptr<Material>&` accessor.

### 2. AssetManager class API

The AssetManager lives in `src/engine/asset/asset_manager.h`:

```cpp
// src/engine/asset/asset_manager.h
#pragma once

#include "error.h"
#include "asset/asset.h"

#include <memory>
#include <string>
#include <string_view>

namespace buddd::engine {

class RenderDevice;

class AssetManager {
public:
    [[nodiscard]] static auto create(RenderDevice& device, std::string_view base_path)
        -> Result<std::unique_ptr<AssetManager>>;

    ~AssetManager();

    /// Loads (or retrieves from cache) the asset with the given ID,
    /// creating GPU resources (textures, shaders, materials) via the RenderDevice.
    /// The ID is the relative path from `base_path` without the `.yaml` extension.
    /// E.g. "textures/brick" -> loads `<base_path>/textures/brick.yaml`.
    /// Returns an error if the type `T` does not match the `type` field in the YAML,
    /// if the YAML file cannot be parsed, or if GPU resource creation fails.
    template<typename T>
    [[nodiscard]] auto create(std::string_view id) -> Result<std::shared_ptr<T>>;

    /// Convenience: loads a TextureAsset by ID. Equivalent to create<TextureAsset>(id).
    [[nodiscard]] auto create_texture(std::string_view id) -> Result<std::shared_ptr<TextureAsset>>;

    /// Convenience: loads a MaterialAsset by ID. Equivalent to create<MaterialAsset>(id).
    [[nodiscard]] auto create_material(std::string_view id) -> Result<std::shared_ptr<MaterialAsset>>;

    /// Removes all cached assets and resets dependency tracking.
    auto clear() -> void;

    /// Returns the base path (the `assets/` directory).
    auto base_path() const noexcept -> std::string_view;

    /// Polls the FileWatcher queue and processes any pending hot-reload events.
    /// Must be called explicitly by the user (e.g., once per frame in the app loop).
    /// Does nothing if the FileWatcher is a no-op (headless/non-Linux).
    auto poll_file_events() -> void;

    /// Enables or disables the FileWatcher. In headless mode or on non-Linux platforms,
    /// the FileWatcher is a no-op regardless of this setting.
    auto set_file_watcher_enabled(bool enabled) -> void;

    AssetManager(const AssetManager&) = delete;
    auto operator=(const AssetManager&) -> AssetManager& = delete;
    AssetManager(AssetManager&&) = delete;
    auto operator=(AssetManager&&) -> AssetManager& = delete;

private:
    AssetManager(RenderDevice& device, std::string base_path);
    // Internal state: cache_ (unordered_map<string, shared_ptr<Asset>>),
    // shader_programs_ (unordered_map<ShaderProgramKey, shared_ptr<ShaderProgram>>),
    // dependency_map_, file_watcher_ (unique_ptr), etc.
};

} // namespace buddd::engine
```

#### Template `create<T>()` implementation detail

The template method is defined inline in the header (or a `.tpp` file included at the bottom of the header). It:

1. Computes the full YAML path from the ID: `<base_path>/<id>.yaml`.
2. Checks the in-memory cache: if an asset with this ID is already cached, validates that its type matches `T` (via `dynamic_cast<T*>` or a stored `std::type_info*`), returns the cached instance or an error on type mismatch.
3. If not cached: reads and parses the YAML file using yaml-cpp.
4. Validates the `type` field matches `T`:
   - For `TextureAsset` -> expects `type: Texture`.
   - For `MaterialAsset` -> expects `type: Material`.
5. Validates the `version` field — if present and not `1`, returns `Unsupported` error.
6. Constructs the concrete asset from the parsed YAML, creating GPU resources via `RenderDevice`:
   - `TextureAsset`: reads `source` path, loads the image via `Image::load()`, creates a GPU texture via `RenderDevice::create_texture()`.
   - `MaterialAsset`: reads `shaders.vertex` / `shaders.fragment` paths, loads shader source, creates/deduplicates the shader program (sharing only a reference-counted `ShaderProgram` wrapper, not the `Material` object), creates a fresh `Material` via `RenderDevice::create_material()`, resolves each texture reference via recursive `create<TextureAsset>()`, calls `set_texture()`, applies constant overrides via `set_uniform()`.
7. Stores the loaded asset in the cache.
8. Records dependency edges in the dependency map: asset -> YAML file + all source files it references.
9. Returns a `shared_ptr<T>`.

#### create<TextureAsset>() pseudo-code

```cpp
// Pseudo-code for create<TextureAsset>
auto AssetManager::create<TextureAsset>(std::string_view id) -> Result<std::shared_ptr<TextureAsset>> {
    auto yaml_path = base_path_ + "/" + id + ".yaml";

    // 1. Check cache
    if (auto it = cache_.find(std::string(id)); it != cache_.end()) {
        auto* tex = dynamic_cast<TextureAsset*>(it->second.get());
        if (!tex) return make_error(Error::Category::InvalidArgument, "Cached asset type mismatch");
        return shared_ptr<TextureAsset>(it->second, tex);
    }

    // 2. Parse YAML (with exception safety — yaml-cpp exceptions must not escape)
    YAML::Node yaml;
    try {
        yaml = YAML::LoadFile(yaml_path);
    } catch (const YAML::Exception& e) {
        return make_error(Error::Category::IoFailed,
            "YAML parse error in " + std::string(yaml_path) + ": " + e.what());
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "Unexpected error parsing " + std::string(yaml_path) + ": " + e.what());
    }

    // 3. Validate type
    auto type = yaml["type"].as<std::string>("");
    if (type != "Texture") return make_error(Error::Category::InvalidArgument,
        "Expected type 'Texture', got '" + type + "'");

    // 4. Validate version
    auto version = yaml["version"].as<int>(1);
    if (version != 1) return make_error(Error::Category::Unsupported,
        "Unsupported Texture version: " + std::to_string(version));

    // 5. Parse source image path
    auto source = yaml["source"].as<std::string>("");
    if (source.empty()) return make_error(Error::Category::InvalidArgument, "Texture 'source' is required");
    auto source_path = resolve_path(source);  // relative to CWD or absolute

    // 6. Load image and create texture
    auto image = Image::load(source_path);
    if (!image) return std::unexpected(image.error());

    auto texture = device_.create_texture(*image);
    if (!texture) return std::unexpected(texture.error());

    // Convert unique_ptr<Texture> to shared_ptr<Texture>
    std::shared_ptr<Texture> shared_tex(std::move(*texture));

    // 7. Texture settings from YAML (wrap_s, wrap_t, min_filter, mag_filter, generate_mipmaps)
    //    are PARSED AND VALIDATED but NOT APPLIED in V1.
    //    GPU texture creation uses current defaults (linear filtering, clamp-to-edge wrapping).
    //    The YAML schema includes these fields for forward compatibility.

    // 8. Create asset wrapper
    auto asset = std::make_shared<TextureAsset>(std::move(shared_tex));

    // 9. Cache and track dependencies
    cache_[std::string(id)] = asset;
    dependency_map_.add_dependency(std::string(id), yaml_path);
    dependency_map_.add_dependency(std::string(id), source_path);

    return asset;
}
```

#### create<MaterialAsset>() pseudo-code

```cpp
// Pseudo-code for create<MaterialAsset>
auto AssetManager::create<MaterialAsset>(std::string_view id) -> Result<std::shared_ptr<MaterialAsset>> {
    auto yaml_path = base_path_ + "/" + id + ".yaml";

    // 1. Check cache (same as Texture)
    // 2. Parse YAML (with exception safety — yaml-cpp exceptions must not escape)
    YAML::Node yaml;
    try {
        yaml = YAML::LoadFile(yaml_path);
    } catch (const YAML::Exception& e) {
        return make_error(Error::Category::IoFailed,
            "YAML parse error in " + std::string(yaml_path) + ": " + e.what());
    } catch (const std::exception& e) {
        return make_error(Error::Category::IoFailed,
            "Unexpected error parsing " + std::string(yaml_path) + ": " + e.what());
    }
    // 3. Validate type == "Material"
    // 4. Validate version == 1

    // 5. Load shader sources
    auto vert_path = resolve_path(yaml["shaders"]["vertex"].as<std::string>(""));
    auto frag_path = resolve_path(yaml["shaders"]["fragment"].as<std::string>(""));
    if (vert_path.empty() || frag_path.empty()) return error(...);

    auto vert_source = read_file(vert_path);
    auto frag_source = read_file(frag_path);
    if (!vert_source || !frag_source) return error(...);

    // 6. Deduplicate shader program by (vert_path, frag_path)
    //    Share ONLY the compiled GL program (via reference-counted ShaderProgram wrapper),
    //    NOT the Material object. Each Material has its own uniform/texture state.
    auto program_key = ShaderProgramKey{vert_path, frag_path};
    std::shared_ptr<ShaderProgram> shader_program;
    auto program_iter = shader_programs_.find(program_key);
    if (program_iter != shader_programs_.end()) {
        shader_program = program_iter->second;  // increment ref count on ShaderProgram
    } else {
        // Compile new shader program
        auto vs = device_.create_shader(ShaderType::Vertex, *vert_source);
        if (!vs) return std::unexpected(vs.error());
        auto fs = device_.create_shader(ShaderType::Fragment, *frag_source);
        if (!fs) return std::unexpected(fs.error());

        auto program = ShaderProgram::create(std::move(*vs), std::move(*fs));
        if (!program) return std::unexpected(program.error());

        shader_program = std::make_shared<ShaderProgram>(std::move(*program));
        shader_programs_[program_key] = shader_program;
    }

    // 7. Create a fresh Material for THIS asset (each material has its own uniform/texture state)
    auto material = device_.create_material(shader_program);
    if (!material) return std::unexpected(material.error());
    auto shared_material = std::shared_ptr<Material>(std::move(*material));

    // 8. Resolve texture references
    auto textures_node = yaml["textures"];
    for (auto it = textures_node.begin(); it != textures_node.end(); ++it) {
        auto tex_name = it->first.as<std::string>();
        auto tex_id = it->second.as<std::string>();
        auto tex_asset = create<TextureAsset>(tex_id);
        if (!tex_asset) return std::unexpected(tex_asset.error());

        auto set_tex_result = shared_material->set_texture(tex_name, (*tex_asset)->texture());
        if (!set_tex_result) { /* log warning, continue */ }
    }

    // 9. Apply constant overrides
    auto constants_node = yaml["constants"];
    for (auto it = constants_node.begin(); it != constants_node.end(); ++it) {
        auto name = it->first.as<std::string>();
        auto value = it->second;
        if (value.IsScalar()) {
            auto set_result = shared_material->set_uniform(name, value.as<float>());
            if (!set_result) { /* log warning, continue */ }
        }
        // Future: support Vec3/Color constants
    }

    // 10. Create asset wrapper
    auto asset = std::make_shared<MaterialAsset>(std::move(shared_material));

    // 11. Cache and track dependencies
    cache_[std::string(id)] = asset;
    dependency_map_.add_dependency(std::string(id), yaml_path);
    dependency_map_.add_dependency(std::string(id), vert_path);
    dependency_map_.add_dependency(std::string(id), frag_path);
    // Texture dependencies are tracked indirectly — the texture asset tracks its own source files.

    return asset;
}
```

### 3. YAML schemas

#### Texture

File: `assets/textures/<name>.yaml`

```yaml
type: Texture
version: 1
source: /absolute/or/relative/path/to/image.png
settings:
  wrap_s: repeat           # repeat, clamp, mirrored_repeat
  wrap_t: repeat
  min_filter: linear       # nearest, linear, nearest_mipmap_linear, linear_mipmap_linear
  mag_filter: linear
  generate_mipmaps: true
```

Fields:
- `type` (string, required): must be `"Texture"`.
- `version` (integer, optional, default 1): format version.
- `source` (string, required): path to the image file. If relative, resolved against the working directory (the project root). If absolute, used as-is. The same source file can be referenced by multiple texture assets.
- `settings` (map, optional): texture parameter overrides.
  - `wrap_s` / `wrap_t` (string, optional, default per backend): one of `"repeat"`, `"clamp"`, `"mirrored_repeat"`.
  - `min_filter` / `mag_filter` (string, optional, default per backend): one of `"nearest"`, `"linear"`, `"nearest_mipmap_linear"`, `"linear_mipmap_linear"` (mag_filter only supports `"nearest"` and `"linear"`).
  - `generate_mipmaps` (bool, optional, default `false`).

#### Material

File: `assets/materials/<name>.yaml`

```yaml
type: Material
version: 1
shaders:
  vertex: shaders/filename.vert       # relative to CWD or absolute
  fragment: shaders/filename.frag
textures:
  albedo: textures/brick              # reference by asset ID
  normal: textures/brick_normal
constants:
  roughness: 0.5                      # float override, mapped to set_uniform
  metallic: 0.1
```

Fields:
- `type` (string, required): must be `"Material"`.
- `version` (integer, optional, default 1): format version.
- `shaders` (map, required):
  - `vertex` (string, required): path to vertex shader source file.
  - `fragment` (string, required): path to fragment shader source file.
- `textures` (map, optional): key is the sampler uniform name (e.g., `"albedo"`), value is an asset ID (e.g., `"textures/brick"`) that is resolved via `AssetManager::create<TextureAsset>()`. The ID refers to a `.yaml` file in the asset tree, not directly to a PNG.
- `constants` (map, optional): key is the uniform name, value is a scalar float. Applied via `set_uniform(name, float_value)`.

### 4. Asset loading data flow

```
User code:
    assets().create<TextureAsset>("textures/brick")
                              │
                              ▼
          ┌─────────────────────────────────┐
          │  AssetManager::create<T>(id)    │
          │                                 │
          │  1. Lookup in cache_            │
          │     ├── found? → return cached  │
          │     └── not found? → continue   │
          │                                 │
          │  2. Compute path:               │
          │     base_path_ + "/" + id       │
          │     + ".yaml"                   │
          │                                 │
          │  3. Parse YAML (yaml-cpp)       │
          │     ├── fail? → error(IoFailed) │
          │     └── success → validate      │
          │         type field matches T    │
          │                                 │
          │  4. Build asset:                │
          │     ├── Texture: load image,    │
          │     │   create_texture, cache   │
          │     └── Material: load shaders, │
          │         dedup program, load     │
          │         textures, apply consts  │
          │                                 │
          │  5. Record dependencies:        │
          │     YAML + all source files     │
          │                                 │
          │  6. Store in cache_, return     │
          └─────────────────────────────────┘
                              │
                              ▼
                    shared_ptr<TextureAsset>
                    (or MaterialAsset)
```

### 5. Shader program deduplication

Two Materials with the same (vertex_path, fragment_path) share **only** the compiled GL program, not the Material object. Each Material has its own uniform/texture state.

A `ShaderProgram` wrapper is reference-counted via `shared_ptr<ShaderProgram>`:

```cpp
struct ShaderProgramKey {
    std::string vertex_path;
    std::string fragment_path;

    auto operator==(const ShaderProgramKey&) const -> bool = default;
};

// Specialise std::hash for ShaderProgramKey
template<>
struct std::hash<ShaderProgramKey> {
    auto operator()(const ShaderProgramKey& k) const noexcept -> size_t {
        return std::hash<std::string>{}(k.vertex_path)
             ^ (std::hash<std::string>{}(k.fragment_path) << 1);
    }
};
```

The deduplication map:

```cpp
std::unordered_map<ShaderProgramKey, std::shared_ptr<ShaderProgram>> shader_programs_;
```

`ShaderProgram` is a reference-counted wrapper around a compiled GL program (`GLuint`), defined in `src/engine/render/shader_program.h` (render layer — CONST-001):

```cpp
// src/engine/render/shader_program.h
namespace buddd::engine {

class ShaderProgram {
public:
    [[nodiscard]] static auto create(std::unique_ptr<Shader> vs, std::unique_ptr<Shader> fs)
        -> Result<ShaderProgram>;

    ~ShaderProgram();

    auto handle() const noexcept -> GLuint;

    ShaderProgram(const ShaderProgram&) = delete;
    auto operator=(const ShaderProgram&) -> ShaderProgram& = delete;
    ShaderProgram(ShaderProgram&&) noexcept;
    auto operator=(ShaderProgram&&) noexcept -> ShaderProgram&;

private:
    ShaderProgram(GLuint program) noexcept;
    GLuint program_ = 0;
};

} // namespace buddd::engine
```

When creating a MaterialAsset:
1. If `shader_programs_` contains an entry for (vert_path, frag_path): retrieve the existing `shared_ptr<ShaderProgram>` (increment ref count).
2. Otherwise: compile both shaders, create a `ShaderProgram` via `ShaderProgram::create()`, store in `shader_programs_` as `shared_ptr<ShaderProgram>`.
3. Create a **fresh** `Material` via the NEW `RenderDevice::create_material(shared_ptr<ShaderProgram>)` overload — each Material gets its own Material object with its own uniform/texture state, sharing only the underlying GL program handle.

**RenderDevice API change**: A new overload is added. The existing `create_material(unique_ptr<Shader>, unique_ptr<Shader>, span<const string>)` is unchanged — callers that compile shaders directly (e.g., `PhongMaterial`) continue to work as before.

```cpp
// NEW overload — accepts a pre-compiled, shared ShaderProgram:
[[nodiscard]] virtual auto create_material(std::shared_ptr<ShaderProgram> program)
    -> Result<std::unique_ptr<Material>> = 0;

// Existing overload — UNCHANGED:
[[nodiscard]] virtual auto create_material(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader,
    std::span<const std::string> known_uniforms = {}
) -> Result<std::unique_ptr<Material>> = 0;
```

Both `RenderDeviceOpenGL` and `RenderDeviceHeadless` implement the new overload. The OpenGL backend extracts the GL program handle from the `ShaderProgram` and creates a `MaterialOpenGL` backed by that handle. The Headless backend stores the program reference and enables uniform/texture state tracking without linking real shaders.

When a shader file changes (hot-reload):
- The shared `ShaderProgram` is recompiled once (the `shared_ptr<ShaderProgram>` in `shader_programs_` is replaced with a new instance).
- All Materials using it automatically retrieve the new GL handle from the `ShaderProgram` during their next `bind()` call, because they hold the `shared_ptr<ShaderProgram>` (not a cached `GLuint`).

This means two material assets with identical shader files share the same compiled GL program but remain independent for uniforms and texture bindings.

### 6. Dependency map

The dependency map tracks two directions:

```cpp
class DependencyMap {
public:
    // asset_id -> list of source file paths
    auto add_dependency(std::string_view asset_id, std::string_view source_path) -> void;
    auto get_dependencies(std::string_view asset_id) const -> std::span<const std::string>;

    // source file path -> list of asset_ids that depend on it
    auto get_dependents(std::string_view source_path) const -> std::span<const std::string>;

    // Remove all entries for a given asset (on hot-reload)
    auto remove_asset(std::string_view asset_id) -> void;

    // Clear all
    auto clear() -> void;

private:
    // Internally a std::unordered_map<std::string, std::vector<std::string>>
    // in each direction (forward and reverse).
    // Implementation detail — not exposed in public API.
};
```

The dependency map is internal to AssetManager and is not exposed in any public header.

### 7. FileWatcher subsystem

#### FileWatcher class

```cpp
// src/engine/asset/file_watcher.h (private)
namespace buddd::engine {

enum class FileEventType {
    Created,
    Modified,
    Deleted
};

struct FileEvent {
    std::string path;        // Absolute path to the changed file
    FileEventType type;
};

class FileWatcher {
public:
    [[nodiscard]] static auto create(std::string_view watch_path)
        -> Result<std::unique_ptr<FileWatcher>>;

    virtual ~FileWatcher();

    /// Returns all pending file events since the last call.
    /// Thread-safe. Must be called from the main thread.
    virtual auto poll_events() -> std::vector<FileEvent> = 0;

    /// Starts the watcher thread. Called after construction.
    virtual auto start() -> void = 0;

    /// Stops the watcher thread.
    virtual auto stop() -> void = 0;

    FileWatcher(const FileWatcher&) = delete;
    auto operator=(const FileWatcher&) -> FileWatcher& = delete;
    FileWatcher(FileWatcher&&) = delete;
    auto operator=(FileWatcher&&) -> FileWatcher& = delete;

private:
    // Linux inotify implementation details:
    // - int inotify_fd_ (created via inotify_init1(IN_NONBLOCK | IN_CLOEXEC))
    // - int watch_fd_ (added via inotify_add_watch with IN_CREATE|IN_MODIFY|IN_DELETE|IN_MOVED_TO|IN_MOVED_FROM)
    // - std::thread watcher_thread_ (runs the read loop)
    // - Thread-safe event queue (e.g., std::mutex + std::queue<FileEvent> or
    //   moodycamel::ReaderWriterQueue if available)
    // - std::atomic<bool> running_ for graceful shutdown
};
```

#### FileWatcher behaviour

- **Availability**: Always available on Linux (all build types). On non-Linux platforms, a `NullFileWatcher` (no-op) is used.
- **Construction**: `FileWatcher::create(watch_path)` opens an inotify file descriptor and adds a watch on the `watch_path` directory recursively. On non-Linux platforms, `create()` returns `Unsupported` error.
- **Start**: Spawns a dedicated thread that reads from the inotify fd in a loop (blocking read with timeout for wake-on-shutdown). Thread stores events in a `std::queue<FileEvent>` protected by `std::mutex`.
- **Poll**: Main thread calls `poll_events()` which locks the mutex, swaps the queue into a local vector, and returns it.
- **Stop**: Sets `running_` to false, writes to a self-pipe (or uses `inotify_rm_watch` + close) to unblock the read, then joins the thread.
- **Destruction**: Calls `stop()` if running, closes the inotify fd.
- **User responsibility**: The user must call `AssetManager::poll_file_events()` explicitly and periodically (e.g., once per frame in the app loop) for hot-reload to work. This is NOT automatic — the render system and engine service do not call it implicitly.

#### Headless/noop FileWatcher

On non-Linux platforms or when the backend is Headless, a no-op `NullFileWatcher` is used instead:

```cpp
class NullFileWatcher final : public FileWatcher {
public:
    auto poll_events() -> std::vector<FileEvent> override { return {}; }
    auto start() -> void override {}
    auto stop() -> void override {}
};
```

The `AssetManager` detects the platform at compile time (`#ifdef __linux__`) and instantiates the appropriate FileWatcher. The FileWatcher is started after construction.

### 8. Hot-reload flows

#### Flow 1: YAML file change detected

```
FileWatcher detects MODIFY on textures/brick.yaml
        │
        ▼
Main thread calls AssetManager::poll_file_events()
        │
        ▼
For each FileEvent:
  1. Check if path matches a cached asset's YAML dependency:
     - Look up reversed_deps_[path] -> set<asset_id>
  2. For each dependent asset_id:
     a. Remove old asset from cache
     b. Remove old dependency entries
      c. Reload asset via create<T>(asset_id):
        - Re-parse YAML
        - Re-resolve all source files
        - Create new GPU resources
        - Update cache
  3. If the YAML change affects a source path that other assets also depend on,
     those dependents are NOT automatically re-triggered (they will be picked
     up if their own source files change). Only the directly changed file's
     dependents are reloaded.
```

#### Flow 2: Source image file change detected

```
FileWatcher detects MODIFY on assets/brick.png
        │
        ▼
poll_file_events() -> lookup reversed_deps_["assets/brick.png"]
        │
        ▼
For each dependent asset_id (all TextureAssets referencing this PNG):
   1. Reload the image: Image::load(path)
   2. Create a new GPU texture: device.create_texture(*image)
   3. SWAP the internal GL handle in the EXISTING Texture object
      (the Texture object is mutated in-place — see design below)
   4. All Materials holding shared_ptr<Texture> automatically see the
      new GPU data on the next bind() call
        │
        ▼
Design (mutable Texture handle swap):
  Instead of replacing the shared_ptr<Texture> inside TextureAsset (which would
  not propagate to Materials that already hold shared_ptr<Texture>), we MUTATE
  the existing Texture OBJECT in-place:
  
  1. Create a new GPU texture: auto new_tex = device_.create_texture(*image);
  2. Swap the internal GLuint handle inside the EXISTING Texture object
     (the Texture class gains a `swap_handle(GLuint new_handle)` method,
      available only to the AssetManager via friendship or internal detail header).
  3. The old GPU handle is deleted.
  4. Every Material that holds a shared_ptr<Texture> to this Texture object
     will use the new handle on the next bind() call — no material iteration,
     no bind-time resolution, no shared_ptr replacement needed.

  This requires:
  - The concrete Texture class (e.g., TextureOpenGL) to support handle swapping.
  - A `swap_handle()` method (private, friended to AssetManager or internal).
  - The Texture object identity (shared_ptr address) never changes.
```
```

#### Flow 3: Shader file change detected

```
FileWatcher detects MODIFY on assets/shaders/phong.vert
        │
        ▼
poll_file_events() -> lookup reversed_deps_
        │
        ▼
For each dependent asset_id (all MaterialAssets using this shader):
   1. Re-read the vertex and fragment source files
   2. Recompile both shaders
   3. Re-link the shader program into a new ShaderProgram
   4. Replace the existing shared_ptr<ShaderProgram> in the shader_programs_ map
      for the (vert_path, frag_path) key
   5. Since the ShaderProgram is shared via shared_ptr<ShaderProgram>,
      all MaterialAssets using it automatically see the new compiled program
      (the shared_ptr points to the new ShaderProgram instance)
   6. No need to recreate Material objects or re-apply uniforms/textures —
      the Material retains its state and simply uses the new GL program handle

Note: Shader recompilation may fail (e.g., syntax error during hot-reload).
In that case, the old shader program and old Material are retained —
hot-reload failure does not destroy the currently running program.
A warning is logged to std::cerr.
```

### 9. EngineService integration

The `EngineService` class gains an `AssetManager` member (accessible via `assets()`) and a new `asset_manager_` member variable:

```cpp
// New private member in EngineService (declared after device_ to ensure correct destruction order):
std::unique_ptr<AssetManager> asset_manager_;
```

New public accessor:

```cpp
auto assets() noexcept -> AssetManager&;
```

The existing `device()` accessor (already present in `EngineService`) returns the `RenderDevice&` needed for AssetManager construction.

Construction in `EngineService::create()`:

```cpp
// After RenderDevice is created:
auto asset_mgr = AssetManager::create(engine_service.device(), "assets");
if (!asset_mgr) {
    return std::unexpected(asset_mgr.error());
}

// Inside EngineService private constructor after device_ is set:
asset_manager_ = std::move(*asset_mgr);
```

The `AssetManager` construction takes a `RenderDevice&` and the base path. The FileWatcher is started after construction.

**The `EngineService` does NOT call `poll_file_events()` automatically.** Hot-reload is driven by explicit user calls. The user must call `engine.assets().poll_file_events()` periodically (e.g., once per frame in the app loop) for hot-reload to work. Calling `poll_file_events()` on a no-op FileWatcher (headless/non-Linux) is safe and returns immediately.

### 10. Thread safety

| Method | Thread-safe? | Notes |
|---|---|---|
| `create<T>(id)` | No | Must be called from the main/render thread. Reads/writes internal cache and dependency map. Not re-entrant. |
| `create_texture(id)` | No | Same as `create<T>`. |
| `create_material(id)` | No | Same as `create<T>`. |
| `clear()` | No | Modifies cache and dependency map. Must NOT be called concurrently with `create<T>()`, `poll_file_events()`, or `set_file_watcher_enabled()`. Safe to call while the FileWatcher thread is running (the FileWatcher thread only writes to the event queue; `clear()` does not touch the watcher). |
| `poll_file_events()` | No | Reads and drains the FileWatcher event queue (which IS thread-safe) and mutates asset cache. Must be called from the main thread only. Not re-entrant — calling recursively or from multiple threads is undefined behaviour. |
| `set_file_watcher_enabled(bool)` | No | Modifies FileWatcher state. Must NOT be called concurrently with `poll_file_events()`. Safe to call from main thread while FileWatcher thread is running (the flag is `std::atomic<bool>`). |
| `base_path()` | Yes | Immutable after construction. Read-only, always safe. |

All AssetManager methods that mutate state assume single-threaded access from the main thread. The only cross-thread interaction is the FileWatcher thread writing `FileEvent` structs to a thread-safe queue (protected by `std::mutex`), which the main thread drains via `poll_file_events()`.

### 11. CMake changes

Add yaml-cpp via FetchContent in `src/engine/CMakeLists.txt`:

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

Then link it as PRIVATE (public headers do not expose yaml-cpp types):

```cmake
target_link_libraries(buddd_engine PUBLIC
    SDL3::SDL3
    OpenGL::GL
    glm::glm
)
target_link_libraries(buddd_engine PRIVATE
    yaml-cpp
)
```

And add include directory:

```cmake
target_include_directories(buddd_engine PRIVATE
    ${stb_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
)
```

An ADR (e.g., ADR-015) should be created to document the yaml-cpp dependency decision, following the project's ADR process.

### 12. Headless test files

New test file: `tests/asset_manager_tests.cpp` with the following test structure:

- `make_headless_engine()` helper (same pattern as `texture_tests.cpp`)
- Test asset files (YAML, PNG, shader sources) are versioned in the repo at `tests/assets/` directory
- Tests cover: loading TextureAsset, loading MaterialAsset, caching, type mismatch, missing YAML, missing source files, texture settings parsing, shader deduplication, dependency tracking, hot-reload event processing (by constructing synthetic FileEvent objects and calling the internal handler via a test hook or making `poll_file_events` testable).

## User stories

### Story 1 — Load a texture by ID (Priority: P1)

As an application developer, I want to load a texture by its asset ID (`"textures/brick"`) and receive a `shared_ptr<TextureAsset>` that I can use with any material, without manually loading PNG files or creating GPU textures.

**Given** a valid YAML file at `assets/textures/brick.yaml` with `type: Texture` pointing to a valid PNG

**When** I call `engine.assets().create<TextureAsset>("textures/brick")`

**Then** a `Result<std::shared_ptr<TextureAsset>>` is returned with the inner texture matching the PNG dimensions and content, and a GPU texture has been created via the RenderDevice.

**Given** the same call is made a second time

**When** I call `create<TextureAsset>("textures/brick")` again

**Then** the cached instance is returned (same pointer address as the first call).

### Story 2 — Load a material by ID (Priority: P1)

As an application developer, I want to load a material by its asset ID (`"materials/wall"`) and receive a `shared_ptr<MaterialAsset>`, with its shader program compiled, its textures resolved, and its constants applied.

**Given** a valid YAML file at `assets/materials/wall.yaml` with:
- `type: Material`
- `shaders.vertex: shaders/phong.vert`
- `shaders.fragment: shaders/phong.frag`
- `textures.albedo: textures/brick`
- `constants.roughness: 0.5`

and the referenced shader files and texture asset exist

**When** I call `engine.assets().create<MaterialAsset>("materials/wall")`

**Then** a `Result<std::shared_ptr<MaterialAsset>>` is returned, the inner material has the shader program compiled, `albedo` texture is set to the brick texture, and `roughness` uniform is set to `0.5`.

### Story 3 — Shader program deduplication (Priority: P1)

As an engine developer, I want two material assets with identical shader files to share the same compiled GL program, to minimise GPU state changes, while keeping their own uniform/texture state.

**Given** two material YAML files `materials/wall.yaml` and `materials/floor.yaml` that both reference `shaders/phong.vert` and `shaders/phong.frag`

**When** both materials are loaded via `create<MaterialAsset>()`

**Then** the two materials share the same `shared_ptr<ShaderProgram>` (same compiled GL program), but each has its own `Material` object with independent uniforms and texture bindings.

### Story 4 — Error on type mismatch (Priority: P1)

As an application developer, I want to receive a clear error if I request a `MaterialAsset` when the YAML declares `type: Texture`.

**Given** a YAML file at `assets/textures/brick.yaml` with `type: Texture`

**When** I call `engine.assets().create<MaterialAsset>("textures/brick")`

**Then** an error is returned with `Error::Category::InvalidArgument` and a message indicating the type mismatch.

### Story 5 — YAML file hot-reload (Priority: P2)

As an asset content creator, I want changes to a texture's YAML file (e.g., changing the `source` path) to be picked up automatically while the engine is running, so that I can iterate on assets without restarting.

**Given** a running engine with a loaded texture asset `"textures/brick"`

**When** the YAML file `assets/textures/brick.yaml` is modified on disk, and `poll_file_events()` is called on the main thread

**Then** the texture asset is reloaded: the old PNG is replaced by the new one, and all materials using this texture see the updated texture on the next frame.

### Story 6 — Shader file hot-reload with error recovery (Priority: P2)

As a shader author, I want to edit a shader file and see the visual result without restarting, and if I introduce a syntax error, the old shader remains active.

**Given** a running engine with a loaded material asset referencing `shaders/phong.vert`

**When** `shaders/phong.vert` is modified on disk (valid change), then `poll_file_events()` is called

**Then** the shader is recompiled, the material's program is updated, and the new visual result appears on the next frame.

**Given** a subsequent edit introduces a syntax error in the same file

**When** `poll_file_events()` detects the change and recompilation fails

**Then** the old shader program is retained, a warning is logged to `std::cerr`, and rendering continues with the old program.

### Story 7 — FileWatcher is a no-op in headless / non-Linux mode (Priority: P2)

As a test author, I want the FileWatcher to do nothing when the engine is in headless mode or on non-Linux platforms, to keep tests deterministic.

**Given** the headless backend (or a non-Linux platform)

**When** `AssetManager` is constructed

**Then** the internal FileWatcher is a `NullFileWatcher` — `poll_file_events()` returns an empty vector at every call, and no thread is spawned.

### Story 8 — Asset caching survives multiple get calls (Priority: P2)

As an application developer, I want repeated calls to `create<T>(id)` to return the same instance, so that materials sharing textures don't create duplicate GPU resources.

**Given** a loaded texture asset `"textures/brick"`

**When** `create<TextureAsset>("textures/brick")` is called 10 times

**Then** all 10 calls return the same `shared_ptr<TextureAsset>` (same address).

### Story 9 — Clear cache resets everything (Priority: P3)

As an engine developer, I want to programmatically clear the asset cache, so that I can force a full reload of all assets (e.g., when switching scenes in the editor).

**Given** several loaded assets cached in the AssetManager

**When** `clear()` is called, then `create<T>(id)` is called for a previously loaded asset

**Then** the asset is freshly loaded from disk (new address, new GPU resources).

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | Abstract `Asset` base class exists in `src/engine/asset/asset.h` with virtual destructor and deleted copy/move operations. | File compiles; class is abstract with virtual destructor; copy/move are deleted. |
| AC-002 | `TextureAsset` concrete class exists in a private header, wrapping `std::shared_ptr<Texture>`. Provides `texture() -> const std::shared_ptr<Texture>&` accessor. | File compiles; `TextureAsset` is final concrete class; accessor returns stored shared_ptr. |
| AC-003 | `MaterialAsset` concrete class exists in a private header, wrapping `std::shared_ptr<Material>`. Provides `material() -> const std::shared_ptr<Material>&` accessor. | File compiles; `MaterialAsset` is final concrete class; accessor returns stored shared_ptr. |
| AC-004 | `AssetManager` class exists in `src/engine/asset/asset_manager.h` with `static create(RenderDevice&, string_view) -> Result<unique_ptr>` factory, template `create<T>(id) -> Result<shared_ptr<T>>`, convenience `create_texture(id)` and `create_material(id)`, `clear()`, `base_path()`, `poll_file_events()`, and `set_file_watcher_enabled(bool)`. | Header compiles; all methods present; template `create<T>` is declared. |
| AC-005 | `AssetManager::create<TextureAsset>(id)` with a valid YAML file (`type: Texture`, valid `source` image path) returns `Result<shared_ptr<TextureAsset>>`. The inner texture has dimensions and channels matching the source image. A GPU texture has been created via the RenderDevice. | Unit test: create YAML + PNG in `tests/assets/`, call `create<TextureAsset>("test_texture")`, verify width/height/channels match and GPU texture handle is valid. |
| AC-006 | `AssetManager::create<MaterialAsset>(id)` with a valid YAML file (`type: Material`, valid shader paths, valid texture references) returns `Result<shared_ptr<MaterialAsset>>`. The inner material has the shader program compiled, textures set, and constants applied. GPU resources (shaders, textures) are created via the RenderDevice. | Unit test: create YAMLs + shader files + texture YAML in `tests/assets/`, call `create<MaterialAsset>("test_material")`, verify material exists, textures are set, constants are set, GPU resources are valid. |
| AC-007 | Calling `create<T>(id)` twice returns the same cached instance (same `shared_ptr` address). | Unit test: two calls return `shared_ptr` with same `get()` address. |
| AC-008 | Calling `create<MaterialAsset>` with a YAML declaring `type: Texture` returns `InvalidArgument` error. | Unit test: YAML with `type: Texture`, request `MaterialAsset` -> error category `InvalidArgument`. |
| AC-009 | Calling `create<TextureAsset>` with a YAML declaring `type: Material` returns `InvalidArgument` error. | Unit test: YAML with `type: Material`, request `TextureAsset` -> error category `InvalidArgument`. |
| AC-010 | Calling `create<T>(id)` with a YAML file that has `version: 2` (unsupported) returns `Unsupported` error. | Unit test: YAML with `version: 2`, request asset -> error category `Unsupported`. |
| AC-011 | Calling `create<T>(id)` with a nonexistent YAML file path returns `IoFailed` error. | Unit test: pass an ID that maps to a file that does not exist -> error category `IoFailed`. |
| AC-012 | `create<TextureAsset>` with a missing `source` field returns `InvalidArgument` error. | Unit test: YAML has `type: Texture` but no `source` -> error. |
| AC-013 | `create<MaterialAsset>` with missing shader paths returns an appropriate error. | Unit test: YAML missing `shaders.vertex` -> error. |
| AC-014 | Shader program deduplication: two `MaterialAsset`s with the same (vertex_path, fragment_path) share the same `shared_ptr<ShaderProgram>` (same compiled GL program), but have separate `Material` objects with independent uniforms/textures. | Unit test: load two material assets with same shader refs, check `ShaderProgram` pointer equality and `Material` pointer inequality. |
| AC-015 | `AssetManager::clear()` removes all cached assets. Subsequent `create<T>(id)` loads fresh. | Unit test: load asset, `clear()`, load again -> different `shared_ptr` address. |
| AC-016 | `poll_file_events()` is safe to call every frame. Returns empty vector when no events. | Unit test: call `poll_file_events()` 10 times in a row on headless -> no crash, returns empty vector each time. |
| AC-017 | On non-Linux platforms or in headless mode, `FileWatcher` is a no-op `NullFileWatcher`. `poll_file_events()` returns empty without a watcher thread. | Unit test: verify that in headless mode, `poll_file_events()` returns empty vector each call and no thread is spawned. |
| AC-018 | Texture settings from YAML (`wrap_s`, `wrap_t`, `min_filter`, `mag_filter`, `generate_mipmaps`) are parsed and validated but NOT applied in V1. GPU texture creation uses current defaults (linear filtering, clamp-to-edge wrapping). | Unit test: parse YAML with settings, verify no crash and settings are stored; no GL-side verification required since settings are not applied. |
| AC-019 | `EngineService` gains `assets() -> AssetManager&` accessor (the existing `device()` accessor is unchanged and provides `RenderDevice&`). | Unit test: create `EngineService`, call `engine.device()` and `engine.assets()`, both return non-null references. |
| AC-020 | `EngineService::create()` fails gracefully if `AssetManager::create()` fails. | Unit test: engineer a failure condition (e.g., invalid base path) -> `create()` returns error. |
| AC-021 | yaml-cpp is a link-time dependency of `buddd_engine`. | Build succeeds; `#include <yaml-cpp/yaml.h>` compiles in `src/engine/` code. |
| AC-023 | Hot-reload of a shader file triggers the reload pipeline: FileEvent → shader source re-read → program recompiled. (Headless test — verifies the pipeline, not GPU state.) | Unit test (headless): inject synthetic FileEvent for a shader file, call `poll_file_events()`, verify that `ShaderProgram`'s internal handle or state changed (via `#ifdef BUDDD_TESTING` test-only accessor: `ShaderProgram::testing_handle()` returns an opaque ID that changes on recompilation). |
| AC-024 | Shader recompilation failure during hot-reload does not destroy the old program (headless). | Unit test (headless): inject a `FileEvent` for a shader that will fail to compile (e.g., contains `#error`), verify the `ShaderProgram` handle or state is unchanged from before the event. Warning is logged to `std::cerr`. |
| AC-025 | (OpenGL, guarded by `BUDDD_HAS_DISPLAY`) Hot-reload of a shader file changes the GPU program handle. | Integration test (OpenGL only): load a material, capture GL program handle (`ShaderProgram::handle()`), inject synthetic `FileEvent`, call `poll_file_events()`, verify the handle has changed to a new valid GLuint. Guarded by `#ifdef BUDDD_HAS_DISPLAY`. |
| AC-026 | Dependency tracking approach: `AssetManager` exposes a test-only read-only view of the dependency map via `#ifdef BUDDD_TESTING` accessor (`get_dependency_map() const`). `TextureAsset` tracks the source image path as a dependency. | Unit test: load texture, query dependency map via test accessor, confirm `source` path is recorded. |
| AC-027 | `MaterialAsset` tracks YAML path, vertex shader path, and fragment shader path as dependencies. | Unit test: load material, query dependency map via `#ifdef BUDDD_TESTING` accessor, confirm all three paths are recorded. |
| AC-028 | Asset manager source files are in a new `src/engine/asset/` directory. | `src/engine/asset/asset.h`, `asset_manager.h`, `asset_manager.cpp`, `texture_asset.h`, `texture_asset.cpp`, `material_asset.h`, `material_asset.cpp`, `file_watcher.h`, `file_watcher.cpp` exist. |
| AC-029 | All public headers in `src/engine/asset/` expose no external library types (no yaml-cpp, no SDL3, no OpenGL). yaml-cpp is a PRIVATE link dependency. | Grep of public headers for `yaml_cpp|yaml-cpp|SDL_|gl[A-Z]|GL_` returns no matches. CMake shows yaml-cpp linked as PRIVATE. |
| AC-030 | The FileWatcher thread uses proper synchronisation and does not cause data races. | Thread sanitizer (TSAN) passes during FileWatcher tests. |
| AC-031 | yaml-cpp is fetched in Release mode, with tests/tools/install disabled. | CMake configuration log shows yaml-cpp fetched with `-DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_CPP_INSTALL=OFF`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An application developer can render a textured cube by loading a material asset from YAML in under 10 lines of C++ (excluding boilerplate). | Count lines in a minimal demo program. |
| SC-002 | All asset loading logic is testable in headless mode without a GPU or display server. | `ctest --preset debug` on a headless CI runner passes all asset manager tests. |
| SC-003 | Loading the same asset N times returns the same cached instance in O(1) amortised time after the first load. | Unit test verifies pointer identity. |
| SC-004 | Hot-reload of a texture file is reflected in the rendered output within 1 frame of `poll_file_events()`. | Integration test: load texture, modify source, poll, verify GPU texture data changed. |
| SC-005 | The `AssetManager` adds no measurable overhead to the frame loop when no file events are pending. | `poll_file_events()` returns in microsecond range when queue is empty. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| `create<T>(id)` with an ID containing path traversal sequences (`../`) | The path is resolved relative to `base_path_`. Path traversal is allowed at the filesystem level but discouraged. The spec does not mandate sanitisation for V1. |
| Same YAML file accessed via different ID strings that resolve to the same path | The cache uses the ID string as the key. Different IDs with the same resolved path will be loaded twice. The spec considers this an application error — IDs must be stable. |
| Texture `source` path is absolute vs relative | Both are supported. Relative paths are resolved from the working directory (project root). |
| Material references a texture asset ID that does not exist | `create<TextureAsset>` returns an error, which propagates as an error from `create<MaterialAsset>`. The material is not cached. |
| Material references a texture asset ID that is valid but has not been loaded yet | The recursive call to `create<TextureAsset>` triggers lazy loading of the texture. Dependencies are resolved transitively. |
| Two materials reference the same texture asset ID | Both material loads call `create<TextureAsset>` — the texture is loaded once and cached. Both materials receive the same `shared_ptr<Texture>`. |
| Material `constants` value is a non-numeric YAML value (e.g., `true`, `"hello"`) | The YAML value is parsed as a scalar. If `as<float>()` throws, the constant is skipped with a warning logged. The material is still loaded. |
| YAML file has extra unknown fields | Unknown fields are silently ignored (yaml-cpp default behaviour). |
| YAML file has no `type` field | `type` defaults to empty string. The type validation check fails, returning `InvalidArgument`. |
| `create<T>(id)` called before FileWatcher is started | The FileWatcher start/stop does not affect asset loading. Loading works regardless of watcher state. |
| `poll_file_events()` called on non-Linux platform | The `NullFileWatcher` always returns an empty vector. No thread is spawned. |
| `poll_file_events()` called while a hot-reload is in progress | `poll_file_events()` drains the queue and processes each event sequentially. Events that arrive during processing are picked up on the next call. The method is not re-entrant (must be called from main thread only). |
| Multiple rapid file changes on the same file | Each change produces a separate `FileEvent`. Processing each event is redundant for the same file — V1 processes them all. Future optimisation: debounce file events by path within a small time window. |
| File deletion event for a source file | Deletion events for source files are noted in the log but do not trigger reload. If the file does not exist when polled, the reload will fail with `IoFailed`, and a warning is logged. The asset remains in its current state. |
| File deletion event for a YAML file | Same as source file deletion — the cached asset remains valid. No automatic cache eviction on YAML deletion for V1. |
| `create<T>(id)` with `T` being an unspecialised type (not `TextureAsset` or `MaterialAsset`) | Compile-time error: the template `create<T>` is only defined for `TextureAsset` and `MaterialAsset` via explicit specialisation (or `static_assert` with a dependent-false trick). |
| Circular dependency between assets (e.g., Material A references Texture B, whose YAML somehow references Material A) | The YAML schemas for V1 do not support cross-type references in both directions. Texture YAML cannot reference a material. Circular dependency through the `create<T>()` chain is impossible in V1. |
| The `assets/` directory does not exist | `AssetManager::create()` returns `IoFailed` error. `EngineService::create()` propagates the error. |
| Shader source file is empty | Creating a `Shader` from an empty source string returns a compilation error (`ShaderCompilationFailed`). This propagates from `create<MaterialAsset>`. |
| Texture image file has zero bytes | `Image::load()` returns `IoFailed`. |
| yaml-cpp YAML parse error (syntax error in YAML) | yaml-cpp throws an exception. The AssetManager must catch exceptions from yaml-cpp and convert them to `Result` errors with `Error::Category::IoFailed`. This is a critical safety requirement — YAML parsing exceptions must not escape the engine boundary. |
| `void AssetManager::poll_file_events()` with no pending events | Returns immediately with no work done. This is the common case for most frames. |
| Texture with `generate_mipmaps: true` and `min_filter: linear` (not mipmap variant) | Texture is created with mipmaps generated, but min filter remains `linear`. The spec does not mandate auto-upgrade of min filter when mipmaps are enabled. The YAML author is responsible for setting both fields consistently. |
| FileWatcher `inotify` watch descriptor limit reached | `inotify_add_watch` fails. The FileWatcher logs a warning and operates with reduced coverage (missing some subdirectories). |

## Error cases

| Case | Expected behaviour |
|---|---|---|
| YAML file not found | `create<T>(id)` returns `make_error(Error::Category::IoFailed, "YAML file not found: <path>")` |
| YAML syntax error (yaml-cpp exception) | Exception caught, returns `make_error(Error::Category::IoFailed, "YAML parse error: <what>")` |
| YAML `type` field missing or empty | `make_error(Error::Category::InvalidArgument, "Missing or empty 'type' field in <path>")` |
| YAML `type` does not match requested `T` | `make_error(Error::Category::InvalidArgument, "Expected type '<expected>', got '<actual>'")` |
| YAML `version` is set to a value > 1 | `make_error(Error::Category::Unsupported, "Unsupported version: N for '<type>'")` |
| Texture YAML missing `source` field | `make_error(Error::Category::InvalidArgument, "Texture 'source' field is required")` |
| Texture source image file not found | Propagates from `Image::load()`: `IoFailed` with message from stb_image |
| Texture source image has unsupported channel count (2 or >4) | `create_texture()` returns `Unsupported` or `InvalidArgument` |
| Material YAML missing `shaders` field | `make_error(Error::Category::InvalidArgument, "Material 'shaders' field is required")` |
| Material YAML missing `shaders.vertex` or `shaders.fragment` | `make_error(Error::Category::InvalidArgument, "Material shaders 'vertex' and 'fragment' are required")` |
| Shader vertex/fragment source file not found | `make_error(Error::Category::IoFailed, "Shader source not found: <path>")` |
| Shader compilation fails | Propagates from `create_shader()`: `ShaderCompilationFailed` with compilation log |
| Shader program linking fails | Propagates from `create_material()`: `LinkingFailed` with link log |
| Texture reference in material YAML points to a nonexistent asset ID | Recursive `create<TextureAsset>()` returns an error, which propagates. The material is not cached. |
| Texture reference in material YAML has a `type: Material` mismatch | Recursive `create<TextureAsset>()` returns `InvalidArgument` type mismatch, which propagates. |
| Constant override name does not match any uniform in the shader | `set_uniform()` returns `UniformNotFound`. The material is still loaded (the constant is skipped with a warning). No error propagated to the caller. |
| `AssetManager::create()` called with empty `base_path` | Returns `InvalidArgument` error. |
| `create<T>(id)` called with empty `id` string | Returns `InvalidArgument` error. |
| Hot-reload shader recompilation fails | Old shader program retained. Warning logged to `std::cerr`. No error propagated to the user. |
| Hot-reload texture reload fails (file not found, invalid image) | Old texture retained. Warning logged to `std::cerr`. No error propagated to the user. |
| FileWatcher thread fails to start | FileWatcher logs a warning and operates in a degraded mode (no hot-reload). Asset loading is unaffected. |
| `inotify_add_watch()` fails (too many watches) | FileWatcher logs a warning. The specific subdirectory or file may not be monitored. Existing loaded assets continue to work. |

## Permissions and security

- No elevated privileges are required to read YAML, image, or shader files.
- Asset files are read from the local filesystem only. No network access.
- The AssetManager does not execute arbitrary code — YAML is parsed structurally, and only the expected fields are read. However, yaml-cpp supports YAML tags and custom types by default. **The engine must disable yaml-cpp's node parsing of custom types/tags to avoid potential security issues** (e.g., by not enabling `YAML::Node::SetStyle` or similar). For V1, the risk is minimal (local files only), but as a best practice, YAML parsing is restricted to basic scalar, map, and sequence types.
- File paths from YAML are resolved against the working directory. Path traversal (`../../../etc/passwd`) is possible but is an already-existing risk (the application reads any file it has access to). No additional sandboxing is added for V1.
- The FileWatcher thread uses only inotify (no exec, no network). The thread is joined cleanly on shutdown.
- All asset GPU resources are managed through the existing `RenderDevice` abstraction, following CONST-001 (architecture boundary).
- Headless mode requires no GPU or display access, maintaining CI safety.

## Observability

All observability uses `std::cerr` consistent with the project pattern.

| Signal | Source |
|---|---|
| Asset loaded (success) | `std::cerr << "[Asset] Loaded: " << id << " (" << type << ")\n"` |
| Asset loaded from cache | `std::cerr << "[Asset] Cache hit: " << id << "\n"` (debug builds only) |
| Asset load failure | `std::cerr << "[Asset] Load failed: " << id << " - " << error.message << "\n"` |
| YAML parse error | `std::cerr << "[Asset] YAML error: " << path << " - " << what << "\n"` |
| YAML type mismatch | `std::cerr << "[Asset] Type mismatch: " << id << " (expected " << expected << ", got " << actual << ")\n"` |
| Texture created from asset | `std::cerr << "[Asset] Texture created: " << id << " (" << w << "x" << h << ", " << ch << "ch)\n"` (debug builds only) |
| Material created from asset | `std::cerr << "[Asset] Material created: " << id << " (" << vert << ", " << frag << ")\n"` (debug builds only) |
| Shader program deduplicated (cache hit) | `std::cerr << "[Asset] Shader program cache hit: (" << vert << ", " << frag << ")\n"` (debug builds only) |
| Shader program compiled (new) | `std::cerr << "[Asset] Shader program compiled: (" << vert << ", " << frag << ")\n"` (debug builds only) |
| Hot-reload: YAML file changed | `std::cerr << "[Asset] Hot-reload: " << id << " (YAML changed)\n"` |
| Hot-reload: source image file changed | `std::cerr << "[Asset] Hot-reload: " << id << " (source image changed)\n"` |
| Hot-reload: shader file changed | `std::cerr << "[Asset] Hot-reload: " << id << " (shader changed)\n"` |
| Hot-reload: shader recompilation failed | `std::cerr << "[Asset] Hot-reload: shader recompilation failed for " << id << " - retaining old program\n"` |
| Hot-reload: texture reload failed | `std::cerr << "[Asset] Hot-reload: texture reload failed for " << id << " - retaining old texture\n"` |
| FileWatcher thread started | `std::cerr << "[FileWatcher] Monitoring: " << watch_path << "\n"` (debug builds only) |
| FileWatcher thread stopped | `std::cerr << "[FileWatcher] Stopped\n"` (debug builds only) |
| FileWatcher inotify_add_watch failure | `std::cerr << "[FileWatcher] Failed to watch: " << path << " - " << strerror(errno) << "\n"` |
| Asset cache cleared | `std::cerr << "[Asset] Cache cleared (" << count << " assets)\n"` |
| Constant override skipped (uniform not found) | `std::cerr << "[Asset] Constant '" << name << "' not found in material " << id << "\n"` (debug builds only) |

## Out of scope

- MaterialInstance (Unreal-style material inheritance) — deferred to V2.
- Non-YAML asset metadata formats (JSON, TOML, etc.).
- Startup-time directory scan for discovering assets (lazy loading only).
- Async/non-blocking asset loading.
- Asset streaming, LOD, or partial loading.
- Network-based asset serving.
- Asset editing, preview, or management GUI.
- glTF/glb model loading (handled by `ModelAsset` in `.specs/sprint-2026-06/gltf-model-loading/`).
- Windows (ReadDirectoryChangesW) or macOS (FSEvents) FileWatcher — Linux inotify only for V1.
- Cross-platform FileWatcher abstraction with fallback polling — V1 is Linux-only with NullFileWatcher for others.
- YAML schema validation beyond basic `type` and `version` checks.
- Shader include processing (`#include` directives in GLSL).
- Texture atlas generation or packing.
- Automatic generation of mipmaps on the GPU (`glGenerateMipmap`) — the `generate_mipmaps` setting is parsed and stored but actual mipmap generation is a future concern (the OpenGL backend for texture already uses DSA without mipmaps; the spec merely reserves the field in YAML).
- Texture compression format handling.
- Asset hot-unloading or reference counting beyond `shared_ptr` lifecycle.
- File event debouncing or deduplication across multiple rapid changes.
- Image reload on the GPU without recreating the texture object (V1 recreates the texture; future versions may use `glTextureSubImage2D` for in-place updates).
- Editor integration, drag-and-drop asset import, or any GUI.
- `assets/` directory creation or migration — the directory must exist.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | yaml-cpp version 0.8.0 is available via GitHub and compatible with C++26 and the project's CMake setup. |
| A-02 | The `assets/` directory exists at the project root (the working directory when the engine binary runs). |
| A-03 | Asset YAML files use `.yaml` extension. Files with `.yml` extension are not supported for V1. |
| A-04 | `Image::load()` can load PNG files via stb_image. The texture `source` path in YAML must point to a PNG file (matching the existing project capability). |
| A-05 | The `Image` class provides public `width()`, `height()`, `channels()`, and `data()` accessors used by `create_texture()` (already true — see SPEC-017). |
| A-06 | The `RenderDevice::create_material()` method accepts a `std::span<const std::string>` for known uniform names (already true — see `render_device.h`). |
| A-07 | Shader source files use `.vert` and `.frag` extensions by convention. The engine reads them as plain text. |
| A-08 | yaml-cpp exceptions thrown during parsing are caught and converted to `Result::Error`. This is a requirement, not an assumption. |
| A-09 | The FileWatcher thread is started after `AssetManager` construction and stopped before destruction. The construction/destruction is on the main thread. |
| A-10 | `poll_file_events()` is called from the main thread only. It is not thread-safe to call from multiple threads. |
| A-11 | Shader compilation/linking errors during hot-reload do not crash the engine — the old program remains in use. |
| A-12 | Texture settings (`wrap_s`, `wrap_t`, `min_filter`, `mag_filter`, `generate_mipmaps`) are parsed and validated from YAML but NOT applied in V1. GPU texture creation uses current defaults (linear filtering, clamp-to-edge wrapping). The YAML schema includes these fields for forward compatibility. |
| A-13 | `EngineService` member declaration order guarantees `asset_manager_` is destroyed before `device_` (since `AssetManager` holds a `RenderDevice&`). The `asset_manager_` is declared after `device_` in the class body. |
| A-14 | New files to be created: |
| | - `src/engine/asset/asset.h` — abstract `Asset` base |
| | - `src/engine/asset/asset_manager.h` — `AssetManager` class |
| | - `src/engine/asset/asset_manager.cpp` — implementation |
| | - `src/engine/asset/texture_asset.h` — `TextureAsset` |
| | - `src/engine/asset/texture_asset.cpp` |
| | - `src/engine/asset/material_asset.h` — `MaterialAsset` |
| | - `src/engine/asset/material_asset.cpp` |
| | - `src/engine/render/shader_program.h` — `ShaderProgram` wrapper (render layer, CONST-001 compliant) |
| | - `src/engine/render/shader_program.cpp` |
| | - `src/engine/asset/file_watcher.h` — `FileWatcher`/`NullFileWatcher` abstract + concrete |
| | - `src/engine/asset/file_watcher.cpp` — Linux inotify implementation |
| | - `src/engine/asset/dependency_map.h` — `DependencyMap` |
| | - `src/engine/asset/dependency_map.cpp` |
| | - `tests/asset_manager_tests.cpp` |
| | Modified files: |
| | - `src/engine/engine_service.h` — add `AssetManager` member and `assets()` accessor |
| | - `src/engine/engine_service.cpp` — construct AssetManager, implement `assets()` |
| | - `src/engine/render/render_device.h` — add new `create_material(shared_ptr<ShaderProgram>)` overload |
| | - `src/engine/render/render_device_opengl.h` and `.cpp` — implement the new overload |
| | - `src/engine/render/render_device_headless.h` and `.cpp` — implement the new overload |
| | - `src/engine/CMakeLists.txt` — add yaml-cpp FetchContent and link |
| A-15 | The FileWatcher is implemented as an abstract base class with two concrete implementations: `InotifyFileWatcher` (Linux) and `NullFileWatcher` (all other platforms). The backend selection is done at compile time via `#ifdef __linux__`. The FileWatcher is available in all build types on Linux (not just debug). |
| A-16 | Shader program deduplication stores `shared_ptr<ShaderProgram>` (reference-counted wrapper around a compiled GL program) in the map. Each Material gets its own `Material` object with independent uniforms and texture bindings, sharing only the underlying GL program handle. When a shader file changes (hot-reload), the shared `ShaderProgram` is recompiled once and all Materials using it automatically see the new program. |

## Open questions

All previously open questions have been resolved. See coordination.md for the full resolution log.

| ID | Question | Resolution |
|---|---|---|
| Q-01 | Should deduplicated shader programs share the full Material object or just the GL program? | **Resolved**: Share only the GL program via a reference-counted `ShaderProgram` wrapper (`shared_ptr<ShaderProgram>`). Each Material has its own uniform/texture state. |
| Q-02 | Should yaml-cpp be PUBLIC or PRIVATE? | **Resolved**: PRIVATE. Public headers do not expose yaml-cpp types. |
| Q-03 | Where should `poll_file_events()` be called? | **Resolved**: Explicit call by the user (e.g., once per frame in the app loop). Not automatic. |
| Q-04 | Should YAML texture settings be applied in V1? | **Resolved**: Parsed and validated in V1 but NOT applied. YAML schema includes them for forward compatibility. GPU texture creation uses current defaults. |
| Q-05 | Should AssetManager hold RenderDevice& or EngineService&? | **Resolved**: `RenderDevice&` passed at construction, obtained from `EngineService::device()`. |
| Q-06 | How to distribute test asset files? | **Resolved**: Versioned in `tests/assets/` directory in the repo. |
| Q-07 | Should FileWatcher be debug-only or always available? | **Resolved**: Always available on Linux (all build types). NullFileWatcher on non-Linux. |

## Out of scope (additional)

- MaterialInstance (V2).
- Asset discovery/scanning at startup.
- Non-Linux FileWatcher backends.
- Async loading.
- Asset compression or packing.
- Shader `#include` processing.
- glTF model loading (now supported via `ModelAsset`, see `.specs/sprint-2026-06/gltf-model-loading/`).
- Editor asset browser.
