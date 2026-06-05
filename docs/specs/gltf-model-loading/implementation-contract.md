# IMPL-022 — glTF Model Loading Implementation Contract

## Source spec

`docs/specs/gltf-model-loading/spec.md`

## Goal

Implement glTF/glb model loading in the Buddd Engine: a `ModelAsset` concrete asset type wrapping a `ModelNode` hierarchy tree, a `PbrMaterial` class (following `PhongMaterial`'s PIMPL + embedded shader pattern), a `ModelLoader` internal component (tinygltf-based glTF→engine conversion), hot-reload of glTF source files via in-place `ModelNode` tree replacement, and two demo apps (`gltf_demo_app`, `hot_reload_gltf_app`). All model loading logic is testable in headless mode with Catch2 v3.

## Non-goals

- Do NOT implement animation loading, playback, or skinning — deferred to V2.
- Do NOT import cameras or lights from glTF.
- Do NOT implement Draco/meshopt compression.
- Do NOT implement KHR_materials extensions beyond core metallic-roughness.
- Do NOT implement non-triangle primitive modes (POINTS, LINES, TRIANGLE_FAN, TRIANGLE_STRIP).
- Do NOT implement async/background model loading.
- Do NOT implement scene selection — only scene[0] or the first scene is loaded.
- Do NOT implement model editing or material reassignment after load.
- Do NOT implement non-gltf formats (OBJ, FBX, COLLADA, etc.).
- Do NOT implement in-memory model caching beyond the AssetManager cache.
- Do NOT implement model LOD.
- Do NOT implement automatic UV generation, tangent computation, or mesh optimisation.
- Do NOT modify existing `PhongMaterial`, `PhongShaders`, or any file under `src/engine/render/phong/`.
- Do NOT modify existing `Model` class (already multi-material via ADR-017).
- Do NOT modify `Asset::replace_root()` friend pattern on existing assets (`TextureAsset`, `MaterialAsset`).
- Do NOT modify existing demo apps (except `main.cpp` for new scene registration).
- Do NOT modify `RenderDevice`, `Texture`, `ShaderProgram`, or `Material` base classes.

## Relevant constitution rules

- **CONST-001**: No platform/graphics/windowing headers outside `src/engine/`. `ModelNode`, `PbrMaterial`, and `ModelAsset` must not leak GL/graphics types into public headers beyond the existing `Material`/`Texture` base class pattern. `tinygltf` types must NOT appear in public headers — they are internal to `ModelLoader`.
- **CONST-002**: All new code MUST have corresponding unit tests that pass.
- **CONST-003** (implied): All fallible APIs return `Result<T>` — matching ADR-001.

## Relevant ADRs

- **ADR-001** (Result/Error pattern): All new public APIs return `Result<T>`.
- **ADR-009** (test naming): New test file `model_asset_tests.cpp` uses the `_tests.cpp` suffix.
- **ADR-010** (no raw pointers): All referenced parameters use `T&` or `shared_ptr<T>`; no raw `T*` in public API.
- **ADR-012** (EngineService navigable graph): `AssetManager` is already accessible via `assets()`. New code uses this.
- **ADR-013** (Standard vertex format): glTF vertex data converts to the 72-byte `Vertex` struct from `vertex.h`, using `k_standard_vertex_format`.
- **ADR-017** (Multi-material Model): `Model::create_indexed()` is the only factory. glTF meshes use `SubMesh` + `vector<shared_ptr<Material>>`.
- **ADR-016** (yaml-cpp dependency): Already present. No new YAML dependency.
- **ADR-014** (CLI app system): New demo apps are `App` subclasses registered in `main.cpp`.

## Files to inspect

The Code Agent MUST read these files before editing to understand existing patterns, signatures, and conventions:

- `src/engine/error.h` — `Error` struct, `Error::Category` enum, `make_error()`, `Result<T>` alias
- `src/engine/asset/asset.h` — Abstract `Asset` base class
- `src/engine/asset/asset_manager.h` — `AssetManager` class, `create<T>()` template, `ShaderProgramKey`, `create_texture()`, `create_material()`
- `src/engine/asset/asset_manager.tpp` — `create<T>()` template implementation
- `src/engine/asset/asset_manager.cpp` — `load_texture()`, `load_material()`, `handle_yaml_change()`, `handle_source_change()`, `resolve_path()`, explicit instantiations, `poll_file_events()`
- `src/engine/render/model.h` — `Model` class, `SubMesh`, `Model::create_indexed()`
- `src/engine/render/model.cpp` — Model factory and draw implementation
- `src/engine/render/vertex.h` — Standard `Vertex` struct (72 bytes) and `k_standard_vertex_format`
- `src/engine/render/phong/phong_material.h` — `PhongMaterial` PIMPL pattern (model for `PbrMaterial`)
- `src/engine/render/phong/phong_material.cpp` — PhongMaterial implementation pattern
- `src/engine/render/phong/phong_shaders.h` — Embedded GLSL shader constants pattern
- `src/engine/render/material.h` — Abstract `Material` class (interface `PbrMaterial` implements)
- `src/engine/render/render_device.h` — `create_material(shared_ptr<ShaderProgram>)`, `create_texture(const Image&)`, `create_shader_program()`
- `src/engine/image/image.h` — `Image::create(const ImageBuffer&)` for embedded texture data, `Image::load(path)` for external files
- `src/engine/image/image_buffer.h` — `ImageBuffer` aggregate struct
- `src/engine/render/texture.h` — Abstract `Texture` class, `replace_gl_handle()`, `gl_handle()`, `release_gl_handle()`
- `src/engine/render/texture_headless.h` — `TextureHeadless` with `data()` accessor for pixel-level inspection
- `src/engine/render/texture_headless.cpp` — Headless texture implementation
- `src/engine/CMakeLists.txt` — FetchContent pattern for stb, yaml-cpp (model for tinygltf)
- `tests/asset_manager_tests.cpp` — Test patterns, `make_headless_engine()` usage
- `tests/model_tests.cpp` — Existing Model test patterns
- `src/cmd/apps/hot_reload_app.cpp` — Hot-reload demo pattern (for `hot_reload_gltf_app`)
- `src/cmd/apps/phong_app.h/.cpp` — Phong lighting demo with lights/camera/ECS (model for `gltf_demo_app`)
- `src/cmd/apps/asset_demo_app.h/.cpp` — AssetManager demo with YAML loading pattern
- `src/cmd/main.cpp` — Scene registration / command dispatch
- `docs/specs/asset-manager/implementation-contract.md` — Existing contract for reference (load_texture/load_material structure)

## Files allowed to change

### New files (must be created):

| File | Purpose |
|------|---------|
| `src/engine/render/model_node.h` | `ModelNode` struct — name, translation, rotation, scale, `std::optional<Model> model`, `vector<ModelNode> children`. Movable, non-copyable. |
| `src/engine/render/pbr/pbr_material.h` | `PbrMaterial` class — final `Material` subclass, PIMPL pattern, embedded GLSL PBR shaders. `set_data(PbrMaterialData)`, `data()`, all `Material` interface overrides, `known_uniform_names()`. Non-copyable, non-movable. |
| `src/engine/render/pbr/pbr_material.cpp` | PbrMaterial implementation — creates inner `Material` via device, delegates all set_uniform/set_texture/bind calls. |
| `src/engine/render/pbr/pbr_shaders.h` | Embedded GLSL PBR vertex + fragment shader constants (metallic-roughness, Cook-Torrance BRDF). Follows `phong_shaders.h` pattern exactly: `constexpr std::string_view k_pbr_vertex_shader_source` and `k_pbr_fragment_shader_source` in `namespace buddd::engine::detail`. |
| `src/engine/asset/model_asset.h` | `ModelAsset` final class — extends `Asset`, wraps `ModelNode` root. `root_node()` (const + mutable), `replace_root(new_root)` (private, friend `AssetManager`). Non-copyable, non-movable. |
| `src/engine/asset/model_asset.cpp` | ModelAsset implementation — constructor, accessors, `replace_root()`. |
| `src/engine/asset/model_loader.h` | `ModelLoader` internal component — static `load_model()` function that takes `tinygltf::Model`, `RenderDevice&`, and `scale` factor, returns `Result<ModelNode>`. All tinygltf types are internal to this file; they MUST NOT appear in any public header. |
| `src/engine/asset/model_loader.cpp` | ModelLoader implementation — vertex conversion, material creation, hierarchy building, texture loading. |
| `src/cmd/apps/gltf_demo_app.h` | `GltfDemoApp` — loads a glTF model from YAML, calls `add_model_to_world()` to create ECS entities, renders with ECS + RenderSystem. |
| `src/cmd/apps/gltf_demo_app.cpp` | GltfDemoApp implementation. |
| `src/cmd/apps/hot_reload_gltf_app.h` | `HotReloadGltfApp` — loads a glTF model, triggers hot-reload by programmatically modifying the source file, verifies update. |
| `src/cmd/apps/hot_reload_gltf_app.cpp` | HotReloadGltfApp implementation. |
| `tests/model_asset_tests.cpp` | All model asset unit tests (headless, Catch2 v3). |
| `assets/models/box/Box.gltf` + `.bin` | Khronos Box test model (minimal, ~few KB). |
| `assets/models/box/Box.yaml` | YAML metadata: `type: Model`, `version: 1`, `source: models/box/Box.gltf`. |
| `assets/models/damaged-helmet/DamagedHelmet.gltf` + `.bin` + textures | Khronos DamagedHelmet model with PBR textures. |
| `assets/models/damaged-helmet/DamagedHelmet.yaml` | YAML metadata. |
| `src/engine/render/model_utils.h` | `add_model_to_world()` free function — traverses `ModelNode` tree depth-first, creates `Entity` objects with `Transform` + `MeshRenderer` for each mesh node, preserves parent-child hierarchy via entity child-parent relationship. |

### Modified files:

| File | Change |
|------|--------|
| `src/engine/asset/asset_manager.tpp` | Extend `static_assert` to include `ModelAsset`. Add `else if constexpr` branch calling `load_model(id, yaml_path)`. |
| `src/engine/asset/asset_manager.h` | Add `#include "asset/model_asset.h"`. Add `create_model(id)` convenience method declaration. Add `load_model(id, yaml_path)` private method declaration. Add `load_model_internal(...)` or similar private helper for the actual glTF loading (the method that takes the parsed YAML and does the work). |
| `src/engine/asset/asset_manager.cpp` | Implement `load_model()`, `create_model()`. Add `ModelAsset` branch to `handle_yaml_change()` and `handle_source_change()`. Add explicit instantiation for `create<ModelAsset>`. |
| `src/engine/CMakeLists.txt` | Add tinygltf FetchContent block (after yaml-cpp). Add `tinygltf_SOURCE_DIR` to PRIVATE include directories. No link-time changes needed. |
| `src/cmd/main.cpp` | Register the new scene names (`gltf`, `hot-reload-gltf`) in the dispatch table. |
| `src/engine/error.h` | Add `InvalidFormat` to `Error::Category` enum. |

## Files forbidden to change

- `src/engine/render/phong/` — any files (existing Phong material system)
- `src/engine/render/vertex.h` — standard Vertex struct and format
- `src/engine/render/model.h` / `model.cpp` — existing Model class
- `src/engine/render/render_device.h`, `render_device_opengl.*`, `render_device_headless.*`
- `src/engine/render/texture.h`, `texture_opengl.*`, `texture_headless.*`
- `src/engine/render/material.h`, `material_opengl.*`, `material_headless.*`
- `src/engine/render/shader_program.h`, `shader_program_opengl.*`, `shader_program_headless.*`
- `src/engine/asset/asset.h` — abstract Asset base
- `src/engine/asset/texture_asset.h/.cpp` — existing texture asset
- `src/engine/asset/material_asset.h/.cpp` — existing material asset
- `src/engine/asset/file_watcher.*`, `dependency_map.*`, `asset_id.h`
- `src/engine/engine_service.h/.cpp` — no changes needed; `AssetManager` already accessible via `assets()`
- `tests/CMakeLists.txt` — new test file is auto-picked by glob
- `.clang-format`, `.clang-tidy`, `.gitignore`, or any CI/CD configuration files
- Any `Makefile`, `CMakePresets.json`, or build configuration outside `src/engine/CMakeLists.txt`

## Existing conventions to follow

- **Header guard**: `#pragma once` (no `#ifndef` guards).
- **Namespace**: `buddd::engine` for all engine code. Use `buddd::engine::detail` for internal helpers (like `phong_shaders.h`).
- **File naming**: `snake_case.h` / `snake_case.cpp`.
- **Include style**: `#include "relative/path/from/src/engine/file.h"` (no angle brackets for project headers).
- **Trailing return types**: `auto method() -> ReturnType`.
- **`[[nodiscard]]`** on all `Result<T>` factory methods and any method whose return value should not be ignored.
- **`noexcept`** on simple accessors / getters.
- **Forward declarations** instead of includes where possible in headers.
- **`std::cerr`** for logging (consistent with existing engine code).
- **Debug-only logging** guarded by `#ifndef NDEBUG`.
- **Tests**: Catch2 v3, `make_headless_engine()` helper, `REQUIRE`/`REQUIRE_FALSE`, `CHECK`, tags `[model][headless]`.
- **`file(GLOB_RECURSE)`** for source files in CMake — new files in `src/engine/asset/` and `src/engine/render/pbr/` are auto-discovered on re-configure.
- **PIMPL pattern**: `PbrMaterial` uses `struct Impl; std::unique_ptr<Impl> impl_;` — identical to `PhongMaterial`.
- **Non-copyable, non-movable**: Delete copy/move operations for resource-owning types. `ModelNode` IS movable (contains `std::optional<Model>` which is movable, and `vector<ModelNode>` which is movable).
- **`friend class AssetManager`** pattern: `ModelAsset::replace_root()` is private, with `friend class AssetManager;` declared in the class — new pattern for hot-reload (first asset type to use in-place tree replacement instead of GPU handle swap).
- **Embedded shader pattern**: Shader source strings are `constexpr std::string_view` constants in a detail header `pbr_shaders.h`, following `phong_shaders.h` exactly.
- **yaml-cpp exception safety**: ALL `YAML::LoadFile()` calls MUST be wrapped in try-catch, following the `parse_yaml_file()` helper pattern in `asset_manager.cpp`.
- **`ImageBuffer` for embedded textures**: Use `ImageBuffer{width, height, channels, data}` + `Image::create(buffer)` for glTF embedded texture data.
- **Model replacement pattern** (new): `Model` is movable. For hot-reload, the entire `ModelNode` tree is replaced via `replace_root()`. Old `Model` objects (within `std::optional<Model>`) are destroyed. External `shared_ptr` references to old `PbrMaterial` objects remain valid (materials are stored as `shared_ptr<Material>` in the `Model`'s materials vector, so even after replacement, old shared_ptrs still reference valid material objects — but the new tree has new material objects).

## Required implementation behavior

### 0. Error::Category extension

Before any model loading code is written, add `InvalidFormat` to the `Error::Category` enum in `src/engine/error.h`:

```cpp
enum class Category {
    // ... existing entries ...
    InvalidFormat,  // corrupt/invalid file format (e.g., malformed glTF)
};
```

All model loading code that detects corrupt or invalid glTF files must use `Error::Category::InvalidFormat`, while `Error::Category::IoFailed` is reserved for missing/unreadable files (file not found, permission denied).

### 1. ModelNode (`src/engine/render/model_node.h`)

```cpp
#pragma once

#include "render/model.h"

#include <optional>
#include <string>
#include <vector>

namespace buddd::engine {

struct ModelNode {
    std::string name;
    math::Vec3 translation{0.0f};
    math::Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // w, x, y, z
    math::Vec3 scale{1.0f};
    std::optional<Model> model;
    std::vector<ModelNode> children;

    ModelNode() = default;
    ModelNode(const ModelNode&) = delete;
    auto operator=(const ModelNode&) -> ModelNode& = delete;
    ModelNode(ModelNode&&) = default;
    auto operator=(ModelNode&&) -> ModelNode& = default;
    ~ModelNode() = default;
};

} // namespace buddd::engine
```

- `ModelNode` is a **movable** struct (not a class with virtual methods).
- `model` is `std::nullopt` when the node has no mesh.
- The quaternion constructor order is `(w, x, y, z)` matching GLM's `quat` constructor.

### 2. ModelAsset (`src/engine/asset/model_asset.h` / `.cpp`)

```cpp
#pragma once

#include "asset/asset.h"
#include "render/model_node.h"

#include <memory>

namespace buddd::engine {

class ModelAsset final : public Asset {
public:
    explicit ModelAsset(ModelNode root) noexcept;

    auto root_node() const noexcept -> const ModelNode&;
    auto root_node() noexcept -> ModelNode&;

    // In-place hot-reload support (friend AssetManager)
    auto replace_root(ModelNode new_root) -> void;

    ModelAsset(const ModelAsset&) = delete;
    auto operator=(const ModelAsset&) -> ModelAsset& = delete;
    ModelAsset(ModelAsset&&) = delete;
    auto operator=(ModelAsset&&) -> ModelAsset& = delete;

private:
    ModelNode root_;
    friend class AssetManager;
};

} // namespace buddd::engine
```

- `replace_root()` uses move assignment: `root_ = std::move(new_root);`
- This swaps the contents in-place, destroying the old tree. External `shared_ptr<Material>` references from the old tree remain valid (materials are ref-counted), but the model hierarchy is replaced.

### 3. PbrMaterialData (`src/engine/render/pbr/pbr_material.h`)

Defined in the same header as `PbrMaterial`:

```cpp
struct PbrMaterialData {
    math::Vec4 base_color_factor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic_factor{1.0f};
    float roughness_factor{1.0f};
    math::Vec3 emissive_factor{0.0f, 0.0f, 0.0f};
    bool double_sided{false};

    std::shared_ptr<Texture> base_color_texture;
    std::shared_ptr<Texture> metallic_roughness_texture;
    std::shared_ptr<Texture> normal_texture;
    std::shared_ptr<Texture> occlusion_texture;
    std::shared_ptr<Texture> emissive_texture;
};
```

- If a texture pointer is null, the shader uses the corresponding factor value.
- `double_sided` is stored but NOT applied to rendering in V1 (face culling is left at backend default). Document this limitation in a comment.

### 4. PbrMaterial (`src/engine/render/pbr/pbr_material.h` / `.cpp`)

Exactly follows the `PhongMaterial` pattern:

- Constructor takes `RenderDevice&` only.
- Creates vertex + fragment shaders from embedded GLSL strings in `pbr_shaders.h`.
- Creates a `ShaderProgram` via `device.create_shader_program()`.
- Creates an inner `Material` via `device.create_material(shader_program)`.
- All `set_uniform` / `set_texture` / `bind` / `has_uniform` / `has_texture` calls delegate to the inner `Material`.
- `set_data(const PbrMaterialData&)` calls `set_uniform` for each factor and `set_texture` for each texture slot. Texture slots with null `shared_ptr` are NOT bound (skip).
- `known_uniform_names()` returns a static vector containing all 26 uniform names from Appendix A of the spec (`u_mvp`, `u_model`, `u_normal_mat`, `u_camera_pos`, `u_light_count`, `u_light_positions_or_dir[8]`, `u_light_colours[8]`, `u_light_ranges[8]`, `u_light_spot_directions[8]`, `u_light_inner_cones[8]`, `u_light_outer_cones[8]`, `u_base_color_factor`, `u_metallic_factor`, `u_roughness_factor`, `u_emissive_factor`, `u_has_base_color_texture`, `u_has_metallic_roughness_texture`, `u_has_normal_texture`, `u_has_occlusion_texture`, `u_has_emissive_texture`). Note: `u_has_*` uniforms are NOT declared in the default Phong shader — they are PBR-specific.
- PIMPL: `struct Impl; std::unique_ptr<Impl> impl_;`
- Non-copyable, non-movable.
- Provides `inner_material()` const/mutable accessor (same as PhongMaterial) for test diagnostic access.
- Does NOT expose `set_camera_position`, `set_lights`, or `set_transforms` convenience setters — those are handled by `RenderSystem` generically (same as Phong). The `RenderSystem` already checks `has_uniform("u_model")` to detect lit materials and sets lighting uniforms automatically — PbrMaterial has these uniforms, so it works automatically.

### 5. PbrShaders (`src/engine/render/pbr/pbr_shaders.h`)

Contains two `constexpr std::string_view` constants:

```cpp
namespace buddd::engine::detail {

constexpr std::string_view k_pbr_vertex_shader_source = R"(#version 450 core
// ... spec Appendix A vertex shader ...
)";

constexpr std::string_view k_pbr_fragment_shader_source = R"(#version 450 core
// ... spec Appendix A fragment shader (Cook-Torrance BRDF) ...
)";

} // namespace buddd::engine::detail
```

- Copy exactly from spec Appendix A, lines 754–920.
- Ensure the `#define MAX_LIGHTS 8` is present in the fragment shader.
- `u_has_base_color_texture` etc. are `uniform float` (not int) — consistent with the spec.
- The `u_has_normal_texture` flag is declared but the normal map sampling code is reserved for future use (V1 uses `v_normal` directly). The uniform is declared for API consistency but no normal map sampling is implemented.

### 6. ModelLoader (`src/engine/asset/model_loader.h` / `.cpp`)

```cpp
#pragma once

#include "error.h"
#include "render/model_node.h"

#include <memory>

namespace buddd::engine {

class RenderDevice;

namespace detail {

struct ModelLoadResult {
    ModelNode root;
    std::vector<std::shared_ptr<Material>> materials;  // all created materials
};

/// Load a glTF model from a file path into a ModelNode hierarchy.
/// @param device     RenderDevice for GPU resource creation.
/// @param gltf_path  Absolute path to the .gltf or .glb file.
/// @param scale      Uniform scale applied to vertex positions.
[[nodiscard]] auto load_gltf_model(RenderDevice& device,
                                   const std::string& gltf_path,
                                   float scale) -> Result<ModelLoadResult>;

} // namespace detail
} // namespace buddd::engine
```

**Implementation behavior** (`model_loader.cpp`):

1. **Load glTF file** using `tinygltf::TinyGLTF::LoadASCIIFromFile()` (for `.gltf`) or `LoadBinaryFromFile()` (for `.glb`). The file extension determines the loader:
   - If the path ends with `.glb`, use `LoadBinaryFromFile()`.
   - Otherwise (`.gltf`), use `LoadASCIIFromFile()`.
   - On parse failure: return `make_error(Error::Category::InvalidFormat, "glTF parse error: <tinygltf error/warning message>")`.

2. **Determine scene**: If `model.default_scene > -1`, use `model.scenes[model.default_scene]`. Otherwise, use `model.scenes[0]`. If no scenes exist, all root-level nodes (`model.nodes` that are children of no scene) are treated as root-node children. Return empty `ModelNode` root with no children if there are no nodes at all.

3. **Build hierarchy**:
   - Create a root `ModelNode` with identity transform, empty name.
   - Recursively build child nodes from the scene's node indices (depth-first).
   - For each glTF node: extract TRS from `gltf_node.translation` (3 floats), `gltf_node.rotation` (4 floats: x, y, z, w — convert to engine Quat w, x, y, z), `gltf_node.scale` (3 floats).
   - If `gltf_node.mesh >= 0`: build a `Model` for that mesh and store in `node.model`.
   - If the mesh reference is invalid (out of bounds): warning logged, `model` stays nullopt.

4. **Build Model from glTF mesh**:
   - For each primitive in the mesh:
     - Verify `mode == 4` (TRIANGLES). If mode is unsupported (0, 1, 5, 6): log warning, skip primitive.
     - Read `POSITION` accessor: **required**. Convert float data to vertex positions, apply `scale` factor.
     - Read `NORMAL` accessor (optional): default to `(0, 0, 1)`.
     - Read `TEXCOORD_0` accessor (optional): default to `(0, 0)`.
     - Read `COLOR_0` accessor (optional): default to `(1, 1, 1, 1)`. If `VEC3`, expand to `VEC4` with alpha = 1.0.
     - Read `TANGENT` accessor (optional): default to `(0, 0, 0, 0)`.
     - Read indices from accessor: detect type (Uint16 = `5123`, Uint32 = `5125`). Error on unsupported type.
     - Assign material index: if primitive's material index is valid, use it; else use 0.
   - After processing all primitives: collect unique material indices, build combined vertex buffer and index buffer, create `SubMesh` per primitive with correct `material_index`.
   - Call `Model::create_indexed(device, k_standard_vertex_format, vertex_data, index_data, index_type, submeshes, materials)`.
   - If `create_indexed` fails: return error.

5. **Build PbrMaterial from glTF material**:
   - For each unique material in the glTF:
     - Read `pbrMetallicRoughness` fields.
     - Create `PbrMaterial` via `RenderDevice::create_material(shared_ptr<ShaderProgram>)` path (PbrMaterial wraps a shader program internally).
     - Create `PbrMaterialData`, populate factors.
     - Load each referenced texture:
       - Determine if texture image is embedded (buffer view) or external (URI).
       - **Embedded**: Extract raw pixel data from glTF buffer, construct `ImageBuffer{width, height, channels, data}`, call `Image::create(buffer)`, then `device.create_texture(image)`.
       - **External**: Resolve URI relative to glTF file directory. If the URI points to an external file, call `Image::load(resolved_path)`, then `device.create_texture(image)`.
       - If any texture load fails: use magenta fallback texture (1×1 RGBA8, values 255, 0, 255, 255), created once as a static and reused. Log warning via `std::cerr`.
     - Call `material->set_data(data)`.
     - Store `shared_ptr<PbrMaterial>` in the materials vector.

6. **Texture loading details**:
   - For each glTF texture with `texCoord > 0`: skip texture with a log warning (V1 only supports TEXCOORD_0).
   - For metallic-roughness texture: `G` channel = roughness, `B` channel = metallic.
   - For the magenta fallback: create a single `shared_ptr<Texture>` magenta 1×1 texture, reused across all PbrMaterial instances that need it. Store it as a function-local static in `model_loader.cpp`.
   - `Image::create(const ImageBuffer&)` flips rows vertically (bottom-left → top-left). glTF embedded textures may be top-left origin. This is acceptable — if the result appears flipped, note it but do not change the flipping logic (it's consistent with all other engine image loading).

7. **Handle `KHR_materials_pbrSpecularGlossiness`**: If detected, log warning, load material with default PBR metallic-roughness factors (white base color, metallic=0.0, roughness=1.0). Do NOT attempt to convert.

8. **Handle `alphaMode`**: If present and not `"OPAQUE"`, log warning `"alphaMode '<mode>' not supported in V1 — treating as opaque"`. Ignore `alphaCutoff`.

### 7. AssetManager changes

#### asset_manager.h changes:

- Add `#include "asset/model_asset.h"` (or forward-declare `ModelAsset` if only used in template — but since `load_model` returns `shared_ptr<ModelAsset>`, the full header is needed).
- Add convenience method:
  ```cpp
  [[nodiscard]] auto create_model(std::string_view id) -> Result<std::shared_ptr<ModelAsset>>;
  ```
- Add private method:
  ```cpp
  auto load_model(const std::string& id, const std::string& yaml_path) -> Result<std::shared_ptr<ModelAsset>>;
  ```

#### asset_manager.tpp changes:

Extend the `static_assert`:
```cpp
static_assert(std::is_same_v<T, TextureAsset> ||
              std::is_same_v<T, MaterialAsset> ||
              std::is_same_v<T, ModelAsset>,
    "AssetManager::create<T>() is only supported for TextureAsset, MaterialAsset, and ModelAsset");
```

Add the `if constexpr` branch:
```cpp
} else if constexpr (std::is_same_v<T, ModelAsset>) {
    return load_model(std::string(id), yaml_path);
}
```

#### asset_manager.cpp changes:

**`create_model()`:**
```cpp
auto AssetManager::create_model(std::string_view id) -> Result<std::shared_ptr<ModelAsset>> {
    return create<ModelAsset>(id);
}
```

**`load_model()`:**

Follows the same YAML validation pattern as `load_texture()` and `load_material()`:

1. Parse YAML via `parse_yaml_file(yaml_path)`. On failure, propagate error.
2. Validate `type == "Model"`. On mismatch, return `InvalidArgument`.
3. Validate `version == 1`. On mismatch, return `Unsupported`.
4. Read `source` field. If missing/empty, return `InvalidArgument`.
5. Resolve source path via `resolve_path()`.
6. Read `settings.scale` (float, default 1.0).
7. Call `detail::load_gltf_model(device_, make_full_path(source_path), scale)`.
8. On success: create `ModelAsset` wrapping `ModelLoadResult::root`.
9. Record materials from `ModelLoadResult::materials` (they are owned by the Model's materials vectors in the tree).
10. Record dependencies: YAML path + glTF source path.
    - Only the `.gltf`/`.glb` file is explicitly tracked as a dependency. The `.bin` file is NOT tracked separately (per spec Assumption A-11). Texture files are NOT tracked individually (they are owned by PbrMaterials, and the glTF source change triggers a full reload which re-loads all textures).
11. Cache in `cache_[id]`, return `shared_ptr<ModelAsset>`.

**`handle_yaml_change()` extension:**

Add a new branch after the existing `MaterialAsset` branch (before the closing `}` of the if-else chain):

```cpp
} else if (auto model_asset = std::dynamic_pointer_cast<ModelAsset>(cache_it->second)) {
    // Model YAML changed — reload entirely
    std::cerr << "[Asset] Hot-reload: " << asset_id << " (Model YAML changed)\n";
    auto result = load_model(asset_id, full_changed_path);
    if (!result) {
        std::cerr << "[Asset] Hot-reload: model reload failed: " << asset_id
                  << " \u2014 retaining old model (" << result.error().message << ")\n";
        return;
    }
    // The new asset is already cached by load_model. Remove the old one.
    // Note: load_model calls cache_[id] = asset, overwriting the old entry.
    // The old ModelNode tree is destroyed when the old shared_ptr goes out of scope.
    // Old shared_ptr<Material> references held by external code remain valid.
    std::cerr << "[Asset] Hot-reload: model reloaded: " << asset_id << "\n";
}
```

**`handle_source_change()` extension:**

Add a new branch for ModelAsset:

```cpp
} else if (auto model_asset = std::dynamic_pointer_cast<ModelAsset>(cache_it->second)) {
    // glTF source file changed — reload and replace in-place
    std::cerr << "[Asset] Hot-reload: " << asset_id << " (glTF source changed)\n";
    
    // Re-read YAML to get scale setting
    auto yaml_path = base_path_ + "/" + asset_id + ".yaml";
    auto yaml_result = parse_yaml_file(make_full_path(yaml_path));
    if (!yaml_result) {
        std::cerr << "[Asset] Hot-reload: YAML parse error for " << asset_id << "\n";
        return;
    }
    auto yaml = std::move(*yaml_result);
    float scale = 1.0f;
    try { scale = yaml["settings"]["scale"].as<float>(1.0f); } catch (...) {}
    
    // Reload the model
    auto result = detail::load_gltf_model(device_, make_full_path(changed_path), scale);
    if (!result) {
        std::cerr << "[Asset] Hot-reload: model reload failed: " << asset_id
                  << " \u2014 retaining old model\n";
        return;
    }
    
    // Replace in-place
    model_asset->replace_root(std::move(result->root));
    std::cerr << "[Asset] Hot-reload: model reloaded: " << asset_id << "\n";
}
```

Note: V1 does NOT update the dependency map during hot-reload of model source — the dependency is always the `.gltf`/`.glb` path from the original YAML. If the YAML source path changes, `handle_yaml_change` catches it.

**Explicit instantiation** at the bottom of `asset_manager.cpp`:
```cpp
template auto AssetManager::create<ModelAsset>(std::string_view id) -> Result<std::shared_ptr<ModelAsset>>;
```

### 8. CMake changes (`src/engine/CMakeLists.txt`)

Add after the yaml-cpp block, inside the FetchContent section:

```cmake
# ----- tinygltf (glTF 2.0 model loading, header-only) -----
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.10.0
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
              -DTINYGLTF_BUILD_LOADER_EXAMPLE=OFF
              -DTINYGLTF_BUILD_LOADER_TEST=OFF
)
FetchContent_MakeAvailable(tinygltf)
```

Add `tinygltf_SOURCE_DIR` to the PRIVATE include directories:
```cmake
target_include_directories(buddd_engine PRIVATE
    ${stb_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
    ${tinygltf_SOURCE_DIR}
)
```

No link-time change needed (tinygltf is header-only).

### 9. Model hot-reload in-place replacement

`ModelAsset::replace_root(ModelNode new_root)` uses move semantics:
```cpp
auto ModelAsset::replace_root(ModelNode new_root) -> void {
    root_ = std::move(new_root);
}
```

- The old `ModelNode` tree is destroyed as `new_root` moves into `root_`.
- Old `shared_ptr<Material>` objects (referenced by old `Model` objects in the tree) remain valid for any external shared_ptr holders, but the tree's `Model objects` are replaced.
- After hot-reload, callers MUST re-traverse the tree via `root_node()` to get fresh references.

### 10. Magenta fallback texture

Created once in `model_loader.cpp` as a function-local static:

```cpp
auto get_magenta_fallback_texture(RenderDevice& device) -> std::shared_ptr<Texture> {
    static std::shared_ptr<Texture> fallback;
    static bool created = false;
    if (!created) {
        auto buffer = ImageBuffer{1, 1, 4, {std::byte{255}, std::byte{0}, std::byte{255}, std::byte{255}}};
        auto image = Image::create(buffer);
        if (image) {
            auto tex = device.create_texture(*image);
            if (tex) {
                fallback = std::shared_ptr<Texture>(std::move(*tex));
            }
        }
        created = true;
    }
    return fallback;
}
```

### 11. Demo apps

**`gltf_demo_app`**:
- Loads a model via `assets().create<ModelAsset>("models/box/Box")` (or `models/damaged-helmet/DamagedHelmet`).
- Calls `add_model_to_world()` to traverse the `ModelNode` tree and create ECS entities with `Transform` + `MeshRenderer` for each mesh node.
- Uses `PhongApp`-style camera controls (orbit or free).
- 120-frame run for automated capture testing.
- Can be registered as `buddd run gltf`.
- This app serves as the canonical usage example for `add_model_to_world()`.

**`hot_reload_gltf_app`**:
- Loads a model from YAML.
- At frame 30, programmatically modifies the `.gltf` source file (e.g., swaps model variant, or uses a pre-prepared alternative `.gltf` file that gets copied over).
- Calls `assets().poll_file_events()` to trigger hot-reload.
- Renders for 60+ frames.
- Can be registered as `buddd run hot-reload-gltf`.

### 12. `add_model_to_world()` utility (`src/engine/render/model_utils.h`)

A free function in `namespace buddd::engine` that eliminates traversal boilerplate in demo apps and application code:

```cpp
#pragma once

#include "render/model_node.h"
#include "scene/entity.h"
#include "scene/world.h"

namespace buddd::engine {

/// Traverses a ModelNode tree depth-first and creates ECS entities for each
/// mesh node. Each entity gets a Transform (set from the node's TRS) and a
/// MeshRenderer (holding the node's shared_ptr<Model>). Parent-child
/// relationships in the entity hierarchy mirror the ModelNode tree.
///
/// @param world   The World (ECS container) to create entities in.
/// @param node    The root ModelNode to traverse.
/// @param parent  Optional parent Entity for hierarchy (Entity::none() = no parent).
/// @return The Entity created for `node`, or Entity::none() if the node has no mesh.
[[nodiscard]] auto add_model_to_world(
    World& world,
    const ModelNode& node,
    Entity parent = Entity::none()
) -> Entity;

} // namespace buddd::engine
```

**Implementation behavior**:

1. **Compute local transform**: Build a `math::Mat4` from the node's TRS:
   ```cpp
   math::Mat4::translate(node.translation) * node.rotation.to_mat4() * math::Mat4::scale(node.scale)
   ```
   Match the `Transform::local_matrix()` convention exactly.

2. **Entity creation**:
   - If `node.model.has_value()`:
     - Create an entity via `Entity::create(world)`.
     - Set the entity's `Transform` position, rotation, scale from `node.translation`, `node.rotation`, `node.scale`.
     - Add a `MeshRenderer` component via `entity.add_component<MeshRenderer>(std::make_shared<Model>(*node.model))`.
     - If `parent` is not `Entity::none()`, reparent via `entity.reparent(parent)` or use `parent.create_child()`.
   - If `!node.model.has_value()`: return `Entity::none()` (no entity created for this node).

3. **Recursive traversal**: For each child in `node.children`, call `add_model_to_world(world, child, entity_created_for_this_node)` passing the entity created for the current node as parent. This preserves the parent-child hierarchy from the `ModelNode` tree.

4. **Return value**: Return the entity created for the current node (if mesh existed), or `Entity::none()`.

5. **Edge cases**:
   - A node with no mesh and no children: returns `Entity::none()`, no entities created.
   - A node with no mesh but with children: returns `Entity::none()`, but children are still traversed with `Entity::none()` as parent (they become root entities).
   - A node with `node.model` but `node.model->submesh_count() == 0`: entity is still created with an empty MeshRenderer. This is valid but unusual.
   - Empty `node.children` vector: no recursion, just the node's own entity (if any).

6. **Inclusion**: The `gltf_demo_app` MUST use `add_model_to_world()` for the common case (loading a single model). This is the canonical usage example.

## Required tests

All tests go in `tests/model_asset_tests.cpp`. Use the `make_headless_engine()` helper pattern.

### Unit tests

| # | Test case | AC | Priority |
|---|-----------|----|----------|
| 1 | `ModelAsset stores and returns root ModelNode` | AC-001, AC-002 | P1 |
| 2 | `PbrMaterial created with known uniforms` | AC-003, AC-015 | P1 |
| 3 | `PbrMaterialData fields settable and readable via set_data()/data()` | AC-004 | P1 |
| 4 | `Load Khronos Box from YAML — success, vertex count 24, index count 36, 1 submesh` | AC-005, AC-007 | P1 |
| 5 | `Load Box twice returns cached instance (same address)` | AC-006 | P1 |
| 6 | `Load DamagedHelmet from YAML — success, PBR materials have textures` | AC-008 | P1 |
| 7 | `ModelNode tree reflects glTF hierarchy — root children match scene root nodes` | AC-009 | P1 |
| 8 | `Each ModelNode with a model has valid submeshes and materials` | AC-010 | P1 |
| 9 | `Missing POSITION attribute returns InvalidArgument` | AC-011 | P1 |
| 10 | `Corrupt glTF file returns InvalidFormat` | AC-012 | P1 |
| 11 | `Type mismatch: YAML type:Texture requested as ModelAsset returns InvalidArgument` | AC-013 | P1 |
| 12 | `YAML version:2 returns Unsupported` | AC-014 | P1 |
| 13 | `glTF material without textures — null texture slots, correct factors` | AC-016 | P2 |
| 14 | `Missing texture URI — magenta fallback used, warning logged` | AC-017 | P2 |
| 15 | `settings.scale: 2.0 doubles vertex positions vs scale 1.0` | AC-018 | P2 |
| 16 | `Transform-only node (no mesh) — model is nullopt, children preserved` | AC-019 | P3 |
| 17 | `Unsupported primitive mode (POINTS) — skipped with warning` | AC-020 | P3 |
| 18 | `Hot-reload: synthetic FileEvent triggers model reload (headless)` | AC-021 | P2 |
| 19 | `create<ModelAsset> compiles; create<int> fails at compile time` | AC-022 | P1 |
| 20 | `create_model(id) convenience method works` | AC-023 | P1 |
| 21 | `Uint32 indices supported` | AC-024 | P2 |
| 22 | `replace_root() is private — compile error when called from outside` | AC-025 | P1 |
| 23 | `doubleSided flag read from glTF` | AC-026 | P2 |
| 24 | `COLOR_0 VEC3 expanded to VEC4 with alpha=1.0` | AC-027 | P2 |
| 25 | `Missing NORMAL defaults to (0,0,1)` | AC-028 | P2 |
| 26 | `glTF material with KHR_materials_pbrSpecularGlossiness — loads with warning, default PBR factors` | Edge case (spec line 609) | P3 |
| 27 | `glTF with alphaMode:BLEND — loads as opaque with warning` | Edge case (spec line 610) | P3 |
| 28 | `glTF file with no meshes — root node with no model, no error` | Edge case (spec line 602) | P3 |
| 29 | `glTF file with no default scene — first scene used` | Edge case (spec line 601) | P3 |
| 30 | `Hot-reload: glTF fails to parse after source change — old model retained` | Edge case (spec line 615) | P2 |
| 31 | `add_model_to_world() creates entities for mesh nodes with correct transforms` | AC-007, AC-009 (traversal) | P1 |
| 32 | `add_model_to_world() returns Entity::none() for nodes without mesh` | AC-019 | P1 |
| 33 | `add_model_to_world() preserves parent-child hierarchy in entity tree` | AC-009 | P1 |

### E2E / Integration verification

- **`gltf_demo_app`**: `buddd run gltf` loads and renders a glTF model (Box or DamagedHelmet). The app must compile, link, and run without crash. Visual verification via `--capture`.
- **`hot_reload_gltf_app`**: `buddd run hot-reload-gltf` loads a model, modifies the source at frame 30, calls `poll_file_events()`, and renders the updated model. Verified via headless test (Test #18) but the app also compiles and runs as an integration check.
- **Headless test suite**: All test cases above pass with `ctest --preset debug` on a headless CI runner.

## Edge cases

All edge cases from the spec (lines 597–618) and error cases (lines 620–639) must be handled per the "Expected behaviour" column. The following edge cases require explicit handling in the implementation:

| Edge case | Required handling |
|-----------|-------------------|
| `create<ModelAsset>` with ID containing path traversal (`../`) | Allowed at filesystem level, no sanitisation for V1 (same as other asset types). |
| glTF file with no default scene | Use `model.scenes[0]`. If no scenes, use all root-level nodes as root children (no scene wrapper). |
| glTF file with no meshes | Returns `ModelAsset` with root `ModelNode` with no children having `model`. Valid — no error. |
| glTF file with multiple scenes | Only the default (or first) scene is loaded. Others are silently ignored. |
| glTF mesh with zero primitives | Node has `model == std::nullopt`. |
| glTF with external `.bin` file | tinygltf resolves the relative URI. If the `.bin` is missing, tinygltf parse fails → `IoFailed` error. |
| glTF with embedded data URI (base64) | Handled by tinygltf automatically. No additional handling. |
| glTF texture with unsupported image format | stb_image handles PNG, JPEG, BMP, GIF. Unsupported → magenta fallback. |
| Same glTF source referenced by two YAML files | Each is a separate cache entry (cache key is ID, not source). Both load independently. |
| YAML `settings.scale` is negative | Applied to vertex positions. Normals are renormalised after scaling. Valid (mirroring). |
| YAML `settings.scale` is zero | Collapses all vertices to origin. No error — valid but unusual. Log a warning. |
| Hot-reload of glTF source that fails to parse after valid load | Old model retained. Warning logged. Rendering continues with old model. |
| glTF with extremely large vertex count (>1M) | Synchronous load may cause frame hitch. No async in V1. Acceptable. |
| glTF material extension not supported (specular-glossiness) | Load with default PBR factors. Warning logged. |
| `poll_file_events()` during model creation | Not re-entrant. Undefined behaviour (same restriction as other asset types). |

## Security impact

- **tinygltf**: Header-only parser that reads from file paths or callbacks. Does not execute arbitrary code. Well-maintained library (widely used in the glTF ecosystem). No known security vulnerabilities at v2.10.0.
- **File paths**: YAML `source` paths are resolved against the filesystem. Path traversal (`../`) is possible but matches existing risk profile for all asset types.
- **No network access**: All asset files are local filesystem reads.
- **GPU resources**: All created through existing `RenderDevice` abstraction (CONST-001 compliant).
- **Magenta fallback**: Prevents rendering with uninitialised texture state.
- **Headless mode**: All loading logic works without GPU/display access (CI-safe).
- **No elevated privileges**: Regular file read permissions only.

## Data and migration impact

None. No existing data formats are changed. New YAML files (`type: Model`) are consumed but never written by the engine. New model asset files are created in `assets/models/`. No existing asset files are modified.

## API compatibility impact

- **`AssetManager`** gains `create_model()` convenience method and `load_model()` private method. `create<ModelAsset>(id)` and `create<TextureAsset>(id)` / `create<MaterialAsset>(id)` share the same template. No existing public API is removed or modified.
- **`AssetManager::create<T>()`** template now accepts `ModelAsset` in addition to `TextureAsset` and `MaterialAsset`. The `static_assert` is updated. This is backward-compatible as the set is additive.
- **`ModelAsset`** is a new concrete `Asset` subclass. No existing `Asset` subclasses are affected.
- **`PbrMaterial`** is a new `Material` subclass. It uses the existing `RenderDevice::create_material(shared_ptr<ShaderProgram>)` overload. No changes to `RenderDevice`.
- **`ModelNode`** is a new struct in `render/`. No existing types are modified.
- All new types are in new files. No existing public API is removed or modified.

## Documentation impact

- **`docs/specs/asset-manager/spec.md`**: Update the non-goals section (currently states "No glTF/glb model loading") to reflect that glTF loading is now supported. This is an existing spec that requires a manual update — note in warnings but do NOT modify it.
- **`docs/wiki/architecture/module-map.md`**: Must be updated by the wiki-agent to add entries for `ModelAsset`, `ModelNode`, `PbrMaterial`, `ModelLoader`, `pbr_shaders.h`, and the `pbr/` subdirectory. (Note only — do NOT modify.)
- **`docs/wiki/domain/glossary.md`**: Add terms: `ModelAsset`, `ModelNode`, `PbrMaterial`, `PbrMaterialData`. (Note only — do NOT modify.)
- **`docs/adr/`**: A new ADR should be created documenting the tinygltf dependency decision (FetchContent integration, rationale). This is out of scope for the implementation contract (handled by adr-agent).
- **README**: Consider adding a note about the new glTF model loading capability and tinygltf dependency. (Optional — not required for V1.)

## ADR impact

- A new ADR should be created documenting the tinygltf dependency decision (FetchContent integration, rationale, version pinning). This is out of scope for the implementation contract — the adr-agent handles it.
- The friend-`AssetManager` pattern for `ModelAsset::replace_root()` is a new architectural decision. If the reviewer deems it significant, a new ADR may be warranted, but the spec and coordination.md already document this as Decision #6. No new ADR is required from the implementation contract.

## Constitution impact

None. All new code complies with CONST-001 (no platform/graphics types leak outside src/engine/), CONST-002 (tests for all new code), and all existing ADRs.

## Done criteria

The implementation is complete when ALL of the following are verifiable:

- [ ] **DC-1**: `src/engine/render/model_node.h` exists with `ModelNode` struct matching the spec (name, translation, rotation, scale, optional<Model>, children). Movable, non-copyable.
- [ ] **DC-2**: `src/engine/render/pbr/pbr_material.h` exists with `PbrMaterial final : Material` class and `PbrMaterialData` struct. Follows `PhongMaterial` PIMPL pattern (embed shaders, delegate to inner Material).
- [ ] **DC-3**: `src/engine/render/pbr/pbr_material.cpp` exists. Creates shader program from embedded shaders. `set_data()` applies all factors and textures. All Material interface methods delegate to inner Material.
- [ ] **DC-4**: `src/engine/render/pbr/pbr_shaders.h` exists with `constexpr std::string_view` vertex and fragment shader source matching spec Appendix A. In `namespace buddd::engine::detail`.
- [ ] **DC-5**: `src/engine/asset/model_asset.h` exists with `ModelAsset final : Asset`. `root_node()` (const + mutable), `replace_root()` (private, friend AssetManager). Non-copyable, non-movable.
- [ ] **DC-6**: `src/engine/asset/model_asset.cpp` exists. Constructor takes ModelNode by value. `replace_root()` uses move assignment.
- [ ] **DC-7**: `src/engine/asset/model_loader.h` and `model_loader.cpp` exist with `detail::load_gltf_model()`. Converts glTF vertices to standard 72-byte Vertex, builds ModelNode tree with PbrMaterials, handles embedded/external textures with magenta fallback. Only used from `asset_manager.cpp` — tinygltf types never appear in any public header.
- [ ] **DC-8**: `src/engine/asset/asset_manager.h` declares `create_model()` and `load_model()`. Includes `model_asset.h`.
- [ ] **DC-9**: `src/engine/asset/asset_manager.tpp` `static_assert` includes `ModelAsset`. `if constexpr` branch calls `load_model()`.
- [ ] **DC-10**: `src/engine/asset/asset_manager.cpp`:
  - `create_model()` delegates to `create<ModelAsset>`.
  - `load_model()` validates YAML, delegates to `load_gltf_model()`, caches result.
  - `handle_yaml_change()` has `ModelAsset` branch (reload via `load_model()`).
  - `handle_source_change()` has `ModelAsset` branch (reload via `load_gltf_model()`, in-place `replace_root()`).
  - Explicit instantiation for `create<ModelAsset>` present.
- [ ] **DC-11**: `src/engine/CMakeLists.txt` has tinygltf FetchContent block. `tinygltf_SOURCE_DIR` in PRIVATE include directories.
- [ ] **DC-12**: `tests/model_asset_tests.cpp` exists with at least the following passing tests (headless mode):
  - [ ] Load Khronos Box — success, 24 vertices, 36 indices, 1 submesh.
  - [ ] Load Box twice returns cached instance (same address).
  - [ ] Load DamagedHelmet — success, PbrMaterial with textures.
  - [ ] ModelNode tree hierarchy matches glTF scene root.
  - [ ] Node with model has valid submeshes and materials (traversal).
  - [ ] Missing POSITION → InvalidArgument.
  - [ ] Corrupt glTF → InvalidFormat.
  - [ ] Type mismatch (YAML type:Texture, request ModelAsset) → InvalidArgument.
  - [ ] Unsupported version → Unsupported.
  - [ ] Factor-only material (no textures) — null texture slots, correct factor values.
  - [ ] Missing texture → magenta fallback.
  - [ ] Scale 2.0 doubles vertex positions.
  - [ ] Transform-only node — model is nullopt, children preserved.
  - [ ] Unsupported primitive mode skipped with warning.
  - [ ] Hot-reload: synthetic FileEvent triggers reload (headless).
  - [ ] `replace_root()` is private (compile check or friend test).
  - [ ] `create_model()` convenience method.
  - [ ] `doubleSided` flag read from glTF.
  - [ ] COLOR_0 VEC3 → VEC4 expansion.
  - [ ] Missing NORMAL → (0,0,1) default.
  - [ ] Uint32 indices supported.
- [ ] **DC-13**: All tests pass with `ctest --preset debug` in headless mode.
- [ ] **DC-14**: Build succeeds with no new warnings (excluding third-party code).
- [ ] **DC-15**: `assets/models/box/Box.gltf` + `.bin` + `Box.yaml` exist and contain valid Khronos Box model with `type: Model`.
- [ ] **DC-16**: `assets/models/damaged-helmet/DamagedHelmet.gltf` + `.bin` + textures + `DamagedHelmet.yaml` exist with valid content.
- [ ] **DC-17**: `src/cmd/apps/gltf_demo_app.h` and `.cpp` exist, compile, and run via `buddd run gltf` without crash for at least 1 frame.
- [ ] **DC-18**: `src/cmd/apps/hot_reload_gltf_app.h` and `.cpp` exist, compile, and run via `buddd run hot-reload-gltf` without crash.
- [ ] **DC-19**: `src/cmd/main.cpp` registers `gltf` and `hot-reload-gltf` scene names.
- [ ] **DC-20**: No tinygltf types appear in any public header (`asset_manager.h`, `model_asset.h`, `model_node.h`, `pbr_material.h`).
- [ ] **DC-21**: `src/engine/render/model_utils.h` exists with `add_model_to_world()` free function. The function traverses a `ModelNode` tree depth-first, creates `Entity` objects with `Transform` + `MeshRenderer` for each mesh node, and preserves parent-child hierarchy.
- [ ] **DC-22**: Test #31 (`add_model_to_world()` creates entities for mesh nodes) passes with correct transform values.
- [ ] **DC-23**: Test #32 (`add_model_to_world()` returns Entity::none() for nodes without mesh) passes.
- [ ] **DC-24**: Test #33 (`add_model_to_world()` preserves parent-child hierarchy) passes.
