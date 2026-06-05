# SPEC-NNNN — glTF Model Loading

## Status

`Draft`

## Problem

The Buddd Engine currently has no way to load 3D models from standard file formats. Models are created programmatically — hardcoded vertex/index buffers in C++ — which is impractical for real-world content:

- **No file-based model loading**: glTF/glb files (the industry standard for 3D asset exchange) cannot be loaded.
- **No PBR material pipeline**: glTF 2.0 defines a physically based shading model (metallic-roughness) with base colour, metallic, roughness, normal, and occlusion maps. The engine has no PBR material type.
- **No hierarchy support**: glTF models contain node hierarchies with local transforms. The engine has no model hierarchy — every model is a flat collection of submeshes.
- **No model hot-reload**: Unlike textures and materials (which the AssetManager already handles via YAML metadata), there is no way to modify a model file and see changes without restarting.
- **No model asset type**: The AssetManager supports `TextureAsset` and `MaterialAsset` but has no concept of a `ModelAsset`.

Extending the AssetManager with glTF model loading and a PbrMaterial type closes this gap, enabling the engine to load real-world 3D content.

## Goals

- **glTF/glb file loading**: Load glTF 2.0 files (both `.gltf` JSON + binary and `.glb` binary formats) into engine geometry, materials, and node hierarchy.
- **ModelAsset type**: A new `ModelAsset` concrete asset type wrapping a `ModelNode` tree, loadable via `AssetManager::create<ModelAsset>(id)`.
- **ModelNode hierarchy**: Each glTF node becomes a `ModelNode` with a local transform (translation, rotation, scale) and an optional `Model` (if the node has a mesh). The tree preserves the glTF parent-child structure.
- **PbrMaterial type**: A new `PbrMaterial` class (like `PhongMaterial`) with embedded GLSL PBR shaders implementing the glTF 2.0 metallic-roughness shading model. Created automatically during glTF loading.
- **YAML metadata**: Model assets defined by a YAML file (`type: Model`) in the `assets/` directory, referencing a `.gltf` or `.glb` source file.
- **Hot-reload**: Modifying the `.gltf`/`.glb` source file triggers model rebuild via the existing `poll_file_events()` mechanism (in-place GPU resource swap).
- **PBR texture loading**: glTF textures (base colour, metallic-roughness, normal, occlusion) are loaded directly from image data (embedded or external) via `Image::load` + `RenderDevice::create_texture` and stored as `shared_ptr<Texture>` in the `PbrMaterial`. Not cached as `TextureAsset`.
- **Missing texture fallback**: A 1×1 magenta texture is used when a glTF model references a texture image that cannot be loaded.
- **Test models committed**: Khronos Box (minimal) and DamagedHelmet (PBR with textures) models are committed in `assets/models/box/` and `assets/models/damaged-helmet/`.
- **Utility function**: `add_model_to_world()` free function traverses the `ModelNode` tree and creates ECS entities with `MeshRenderer` + `TransformComponent`, eliminating boilerplate in demo apps.
- **Demo apps**: `gltf_demo_app` renders a glTF model with camera controls (using the utility). `hot_reload_gltf_app` validates the hot-reload flow.
- **Headless testability**: All model loading logic is testable with the Headless backend.

## Non-goals

- No animations (skeletal, vertex, or morph target) — deferred to V2.
- No cameras or lights imported from glTF — deferred to V2.
- No Draco mesh compression or EXT_meshopt_compression — deferred to V2.
- No KHR_materials_variants, KHR_materials_transmission, KHR_materials_volume, or other glTF material extensions — V1 implements only the core metallic-roughness model.
- No skinning or bone weights.
- No node animation or interpolation.
- No scene-level import — the first scene (`scene[0]`) is loaded automatically.
- No model editing or material assignment after load (immutable ModelNode tree).
- No async/background model loading.
- No non-gltf formats (OBJ, FBX, COLLADA, etc.).
- No in-memory model caching beyond the AssetManager cache.
- No model LOD (Level-of-Detail).
- No automatic UV generation, tangent computation, or mesh optimisation — glTF tangents are optional.

## Actors

| Actor | Description |
|---|---|
| Application developer | Loads models by ID via `AssetManager::create<ModelAsset>(id)`. Traverses the `ModelNode` tree. Creates ECS entities for each node. Renders via existing `MeshRenderer` pattern. |
| Engine developer | Maintains the PbrMaterial, ModelNode, ModelAsset, tinygltf integration, and hot-reload for model files. |
| Asset content creator | Creates and edits glTF/glb files in the `assets/models/` directory. Writes YAML metadata files (`type: Model`). |
| Build system | CMake + Ninja — adds tinygltf as a FetchContent dependency. New source files in `src/engine/` are picked up by existing `file(GLOB_RECURSE)`. |
| Test suite | Catch2 v3 tests that verify glTF loading, vertex conversion, hierarchy building, material creation, error cases, and hot-reload — all in headless mode. |

## Key entities

### ModelNode

Represents a single node in the glTF node hierarchy. Contains a local transform (TRS: translation, rotation, scale) and an optional `Model` if the glTF node referenced a mesh.

```cpp
namespace buddd::engine {

struct ModelNode {
    std::string name;                              ///< Node name from glTF (may be empty)
    math::Vec3 translation{0.0f};                  ///< Local translation
    math::Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  ///< Local rotation (quaternion)
    math::Vec3 scale{1.0f};                        ///< Local scale
    std::optional<Model> model;                     ///< Mesh data (absent if node has no mesh)
    std::vector<ModelNode> children;               ///< Child nodes (may be empty)
};

} // namespace buddd::engine
```

- `ModelNode` is a plain data struct — publicly constructible, movable, no virtual methods.
- `model` is `std::nullopt` when the glTF node references no mesh (or references a mesh with no primitives).
- The `math::Quat` type already exists in the engine's math library (GLM-based).

### ModelAsset

A concrete `Asset` subclass wrapping a `ModelNode` root:

```cpp
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

- `root_node()` provides const and mutable access to the root node of the hierarchy.
- `replace_root()` is used during hot-reload to swap the entire hierarchy in-place. It is a private method accessible only to `AssetManager` (via friendship).

### PbrMaterial

A new material class following the `PhongMaterial` pattern — embedded GLSL shaders, no external shader files:

```cpp
namespace buddd::engine {

struct PbrMaterialData {
    math::Vec4 base_color_factor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic_factor{1.0f};
    float roughness_factor{1.0f};
    math::Vec3 emissive_factor{0.0f, 0.0f, 0.0f};
    bool double_sided{false};

    // Texture references (may be null)
    std::shared_ptr<Texture> base_color_texture;
    std::shared_ptr<Texture> metallic_roughness_texture;
    std::shared_ptr<Texture> normal_texture;
    std::shared_ptr<Texture> occlusion_texture;
    std::shared_ptr<Texture> emissive_texture;
};

class PbrMaterial final : public Material {
public:
    explicit PbrMaterial(RenderDevice& device);

    auto set_data(const PbrMaterialData& data) -> void;
    auto data() const noexcept -> const PbrMaterialData&;

    // -- Material interface --
    auto set_uniform(std::string_view name, float value) -> Result<void> override;
    auto set_uniform(std::string_view name, int32_t value) -> Result<void> override;
    auto set_uniform(std::string_view name, bool value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> override;
    auto has_uniform(std::string_view name) const -> bool override;
    auto set_texture(std::string_view name, std::shared_ptr<Texture> texture) -> Result<void> override;
    auto has_texture(std::string_view name) const -> bool override;
    auto bind() const -> void override;

    static auto known_uniform_names() -> const std::vector<std::string>&;

    PbrMaterial(const PbrMaterial&) = delete;
    auto operator=(const PbrMaterial&) -> PbrMaterial& = delete;
    PbrMaterial(PbrMaterial&&) = delete;
    auto operator=(PbrMaterial&&) -> PbrMaterial& = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace buddd::engine
```

- `PbrMaterial` wraps an inner `Material` created via `RenderDevice`, exactly like `PhongMaterial`.
- The embedded shaders implement the glTF 2.0 metallic-roughness PBR model (see Appendix).
- `PbrMaterialData` holds the material parameters from the glTF metallic-roughness base material.
- Textures are `std::shared_ptr<Texture>` — they can be shared across multiple materials.
- If a texture is null (missing/unloadable), the shader falls back to the corresponding factor.

## User-visible behavior

### 1. ModelAsset creation flow

```
User code:
    assets().create<ModelAsset>("models/box/Box")
                              │
                              ▼
        ┌───────────────────────────────────────────┐
        │  AssetManager::create<ModelAsset>(id)     │
        │                                           │
        │  1. Lookup in cache_                      │
        │     ├── found? → return cached            │
        │     └── not found? → continue             │
        │                                           │
        │  2. Compute YAML path:                    │
        │     base_path_ + "/" + id + ".yaml"       │
        │                                           │
        │  3. Parse YAML (yaml-cpp)                 │
        │     ├── fail? → error(IoFailed)           │
        │     └── success → validate type == Model  │
        │                                           │
        │  4. Read source path from YAML             │
        │                                           │
        │  5. Load .gltf/.glb via tinygltf:         │
        │     a. Parse glTF structure                │
        │     b. Validate minimum requirements       │
        │     c. Build vertex/index buffers          │
        │     d. Convert materials → PbrMaterial     │
        │     e. Load textures → shared_ptr<Texture> │
        │     f. Build ModelNode hierarchy           │
        │     g. Create Model objects per mesh node  │
        │                                           │
        │  6. Create ModelAsset wrapping root node   │
        │                                           │
        │  7. Record dependencies:                   │
        │     YAML + .gltf/.glb + all texture images │
        │                                           │
        │  8. Store in cache_, return                │
        └───────────────────────────────────────────┘
                              │
                              ▼
                    shared_ptr<ModelAsset>
```

### 2. ModelAsset API details

#### AssetManager template extension

The existing `static_assert` in `asset_manager.tpp` is extended to include `ModelAsset`:

```cpp
static_assert(std::is_same_v<T, TextureAsset> ||
              std::is_same_v<T, MaterialAsset> ||
              std::is_same_v<T, ModelAsset>,
    "AssetManager::create<T>() is only supported for TextureAsset, MaterialAsset, and ModelAsset");
```

#### New convenience method

```cpp
[[nodiscard]] auto create_model(std::string_view id) -> Result<std::shared_ptr<ModelAsset>>;
```

Equivalent to `create<ModelAsset>(id)`.

### 3. ModelNode traversal

The application developer traverses the `ModelNode` tree to create renderable entities:

```cpp
auto model_asset = engine.assets().create<ModelAsset>("models/box/Box");
if (!model_asset) { /* handle error */ }

auto& root = (*model_asset)->root_node();
traverse_node(root, math::Mat4{1.0f});

void traverse_node(const ModelNode& node, const math::Mat4& parent_transform) {
    auto local = math::Mat4::translate(node.translation)
               * math::Mat4::rotate(node.rotation)
               * math::Mat4::scale(node.scale);
    auto world = parent_transform * local;

    if (node.model.has_value()) {
        // Create entity with MeshRenderer + LocalTransform using world transform
        // and node.model (which contains Models with PbrMaterials)
    }

    for (const auto& child : node.children) {
        traverse_node(child, world);
    }
}
```

### 3a. Utility function: `add_model_to_world`

To avoid duplicating the traversal boilerplate in every application, a free function is provided in `src/engine/render/model_utils.h`:

```cpp
namespace buddd::engine {

/// Traverses a ModelNode tree depth-first and creates ECS entities for each
/// mesh node. Each entity gets a Transform (set from the node's TRS) and a
/// MeshRenderer (holding the node's Model via shared_ptr). Parent-child
/// relationships in the entity hierarchy mirror the ModelNode tree.
///
/// @param world   The World (ECS container) to create entities in.
/// @param node    The root ModelNode to traverse (non-const: Model is move-only).
/// @param parent  Optional parent Entity for hierarchy (Entity::none() = no parent).
/// @return The Entity created for `node`, or Entity::none() if the node has no mesh.
auto add_model_to_world(
    World& world,
    ModelNode& node,
    Entity parent = Entity::none()
) -> Entity;

} // namespace buddd::engine
```

The function:
1. Traverses the `ModelNode` tree depth-first.
2. Computes the local transform matrix from the node's TRS (translation × rotation × scale).
3. For each node with a mesh (`node.model.has_value()`), creates an entity with:
   - `TransformComponent` holding the local transform.
   - `MeshRenderer` holding `shared_ptr<Model>` from `node.model`.
4. Sets parent-child relationships in the ECS (if the engine's transform system supports hierarchy).
5. Returns the entity created for the traversed node, or `entt::null` if the node has no mesh.

Applications can use this for the common case, or fall back to manual traversal when they need custom behaviour (e.g., per-node material overrides, filtering).

```cpp
// Typical usage in an app:
auto model_asset = engine.assets().create<ModelAsset>("models/helmet/DamagedHelmet");
if (model_asset) {
    add_model_to_world(engine.world(), (*model_asset)->root_node());
}
```

### 4. PbrMaterial creation during glTF loading

For each glTF material, the AssetManager:

1. Reads `pbrMetallicRoughness` from the glTF material.
2. Extracts base colour factor, metallic factor, roughness factor.
3. Loads referenced textures:
    - `baseColorTexture` → direct `Image::load` + `RenderDevice::create_texture`.
   - `metallicRoughnessTexture` → same.
   - `normalTexture` → same.
   - `occlusionTexture` → same.
   - `emissiveTexture` → same (if emissive factor is non-zero or emissive texture is present).
4. Creates a `PbrMaterial` via `RenderDevice`.
5. Applies all factors and textures via `PbrMaterial::set_data()`.
6. Stores the `shared_ptr<PbrMaterial>` for use by the `Model` objects.

Each unique glTF material produces exactly one `PbrMaterial` instance, shared across all mesh primitives that reference it.

### 5. Vertex conversion

glTF vertex attributes are mapped to the engine's `Vertex` struct (72 bytes):

| Vertex field | glTF semantic | Required? |
|---|---|---|
| `position` | `POSITION` | **Yes** — error if missing |
| `color` | `COLOR_0` | No — defaults to white (1,1,1,1) |
| `normal` | `NORMAL` | No — defaults to (0,0,1) if missing |
| `texcoord` | `TEXCOORD_0` | No — defaults to (0,0) |
| `tangent` | `TANGENT` | No — not used in V1 PBR; defaults to (0,0,0,0) |
| `texcoord2` | `TEXCOORD_1` | No — defaults to (0,0) |

**Missing POSITION attribute**: returns `InvalidArgument` error. glTF without positions is not a valid mesh.

**Coordinate system**: glTF uses a right-handed Y-up coordinate system. The engine uses a right-handed Y-up coordinate system (consistent with GLM defaults). No coordinate conversion is needed.

**Vertex colour handling**: glTF `COLOR_0` is a `VEC4` by default but may be `VEC3`. If `VEC3`, the alpha channel is set to 1.0.

### 6. glTF material → PbrMaterial mapping

| glTF metallic-roughness field | PbrMaterial field |
|---|---|
| `pbrMetallicRoughness.baseColorFactor` | `PbrMaterialData::base_color_factor` |
| `pbrMetallicRoughness.baseColorTexture` | `PbrMaterialData::base_color_texture` |
| `pbrMetallicRoughness.metallicFactor` | `PbrMaterialData::metallic_factor` |
| `pbrMetallicRoughness.roughnessFactor` | `PbrMaterialData::roughness_factor` |
| `pbrMetallicRoughness.metallicRoughnessTexture` | `PbrMaterialData::metallic_roughness_texture` |
| `normalTexture` | `PbrMaterialData::normal_texture` |
| `occlusionTexture` | `PbrMaterialData::occlusion_texture` |
| `emissiveFactor` | `PbrMaterialData::emissive_factor` |
| `emissiveTexture` | `PbrMaterialData::emissive_texture` |
| `doubleSided` | `PbrMaterialData::double_sided` |

- The metallic-roughness texture `G` channel = roughness, `B` channel = metallic (glTF convention).
- Alpha channel of base colour texture is ignored in V1 (no alpha blending/masking — deferred).
- If a texture cannot be loaded (file not found, unsupported format), the factor is used as fallback. A warning is logged.

### 7. Texture loading strategy

Textures referenced by glTF materials are loaded directly from image data, not as `TextureAsset` instances. The loading path is:

1. For each texture in the glTF, the AssetManager checks if the image data is embedded (glTF buffer view) or external (file path via `uri`).
2. **Embedded (buffer view)**: The raw pixel data is extracted from the glTF buffer, wrapped in a GLM-like image struct (or compatible with `RenderDevice::create_texture()`), and a GPU texture is created directly.
3. **External (uri)**: The URI is resolved relative to the glTF file's directory. `Image::load()` loads the image, then `RenderDevice::create_texture()` creates the GPU texture.
4. If loading fails (embedded or external), a 1×1 magenta fallback texture is used instead. Failure is logged via `std::cerr`.
5. The resulting `shared_ptr<Texture>` is stored in the `PbrMaterialData`.

Textures are **not** cached as `TextureAsset` in the AssetManager cache — they are created direct from image data and owned by the `PbrMaterial`. The glTF YAML file records the glTF binary/source file as a dependency (not individual texture files), so any texture change requires modifying the glTF source (which triggers a full model reload).

### 8. Hierarchy building algorithm

```cpp
// Pseudo-code for building ModelNode tree from tinygltf::Model
auto build_node_tree(const tinygltf::Model& model) -> Result<ModelNode> {
    // glTF has a default scene (scene 0). If no scene exists, all root-level nodes are used.
    auto& scene = model.scenes[model.default_scene > -1 ? model.default_scene : 0];

    // Create root node (identity transform, no mesh)
    ModelNode root;
    root.name = model.label;  // or empty

    for (int node_idx : scene.nodes) {
        auto child = build_node(model, node_idx);
        root.children.push_back(std::move(child));
    }

    return root;
}

auto build_node(const tinygltf::Model& model, int node_idx) -> Result<ModelNode> {
    auto& gltf_node = model.nodes[node_idx];

    ModelNode node;
    node.name = gltf_node.name;

    // Extract TRS
    if (gltf_node.translation.size() == 3)
        node.translation = {gltf_node.translation[0], gltf_node.translation[1], gltf_node.translation[2]};
    if (gltf_node.rotation.size() == 4)
        node.rotation = {gltf_node.rotation[3], gltf_node.rotation[0], gltf_node.rotation[1], gltf_node.rotation[2]};  // w, x, y, z
    if (gltf_node.scale.size() == 3)
        node.scale = {gltf_node.scale[0], gltf_node.scale[1], gltf_node.scale[2]};

    // Build mesh if node has a mesh
    if (gltf_node.mesh >= 0) {
        auto model_result = build_model(model, model.meshes[gltf_node.mesh], node_idx);
        if (model_result) {
            node.model = std::move(*model_result);
        }
        // If build_model fails (e.g., missing position), propagate error
    }

    // Build children
    for (int child_idx : gltf_node.children) {
        auto child = build_node(model, child_idx);
        node.children.push_back(std::move(child));
    }

    return node;
}
```

### 9. Model creation from glTF mesh

For each glTF mesh, all its primitives are collected into a single `Model`:

1. Collect all unique materials referenced by the mesh's primitives.
2. Build a combined vertex buffer and index buffer from all primitives.
3. Create one `SubMesh` per primitive, with the correct material index.
4. Call `Model::create_indexed()` with the combined buffers, submeshes, and materials.

**Index type**: glTF indices are typically `Uint16` or `Uint32`. The engine's `IndexType` enum already supports both. The type is selected based on `accessor.componentType` (`5123` = Uint16, `5125` = Uint32).

**Primitive mode**: glTF mode `4` (TRIANGLES) is the only supported mode in V1. Modes `0` (POINTS), `1` (LINES), `5` (TRIANGLE_FAN), `6` (TRIANGLE_STRIP) are not supported — if encountered, the primitive is skipped with a warning.

## YAML schema

File: `assets/models/<name>.yaml`

```yaml
type: Model
version: 1
source: models/box/Box.gltf       # relative to base_path, or absolute
settings:
  scale: 1.0                      # uniform scale applied to all vertices
```

Fields:
- `type` (string, required): must be `"Model"`.
- `version` (integer, optional, default 1): format version.
- `source` (string, required): path to the `.gltf` or `.glb` file. If relative, resolved against the working directory (project root). If absolute, used as-is.
- `settings` (map, optional):
  - `scale` (float, optional, default 1.0): uniform scale factor applied to all vertex positions during loading. Does not affect normals (they are renormalised after scaling).

## User stories

### Story 1 — Load a box model (Priority: P1)

As an application developer, I want to load a simple glTF model (Box) via the AssetManager and receive a `ModelAsset` with one submesh and one material.

**Given** a valid YAML file at `assets/models/box/Box.yaml` with `type: Model` pointing to `models/box/Box.gltf` (Khronos Box), and the Box.gltf file exists in the repo

**When** I call `engine.assets().create<ModelAsset>("models/box/Box")`

**Then** a `Result<std::shared_ptr<ModelAsset>>` is returned, the root `ModelNode` contains one child node with a `Model` containing 1 `SubMesh`, the material is a `PbrMaterial`, and the vertex buffer has 24 vertices (12 triangles × 3 vertices, each face separate).

**Given** the same call is made a second time

**When** I call `create<ModelAsset>("models/box/Box")` again

**Then** the cached instance is returned (same pointer address as the first call).

### Story 2 — Load a PBR model with textures (Priority: P1)

As an application developer, I want to load DamagedHelmet (a PBR model with textures) and see a fully textured helmet.

**Given** a valid YAML file at `assets/models/damaged-helmet/DamagedHelmet.yaml` with `type: Model` pointing to `assets/models/damaged-helmet/DamagedHelmet.gltf`, and all referenced texture images exist

**When** I call `engine.assets().create<ModelAsset>("models/damaged-helmet/DamagedHelmet")`

**Then** the result is successful, the root `ModelNode` has children (the helmet hierarchy), each mesh node has a `Model`, materials are `PbrMaterial` with base colour, metallic-roughness, normal, and occlusion textures set.

### Story 3 — Traverse ModelNode hierarchy (Priority: P1)

As an application developer, I want to traverse the ModelNode tree to create renderable entities with correct local transforms.

**Given** a loaded `ModelAsset` for DamagedHelmet

**When** I traverse `root_node().children` recursively, collecting each `ModelNode` with a non-null `model` and computing its world transform from local TRS

**Then** the traversal visits all mesh nodes, each has a valid `Model` with submeshes and PbrMaterials, and the world transforms match the glTF node hierarchy.

### Story 4 — Error on invalid glTF file (Priority: P1)

As an application developer, I want a clear error if the glTF file is corrupt or missing.

**Given** a YAML file pointing to a nonexistent or corrupt `.gltf` file

**When** I call `create<ModelAsset>(id)`

**Then** an `IoFailed` error is returned with a descriptive message, and the asset is not cached.

### Story 5 — Hot-reload of glTF source file (Priority: P2)

As an asset content creator, I want changes to a `.gltf` or `.glb` file to be reflected without restarting the engine.

**Given** a running engine with a loaded model asset `"models/box/Box"`

**When** the `.gltf` file is modified on disk (e.g., vertex positions changed), and `poll_file_events()` is called on the main thread

**Then** the model is reloaded in-place: GPU buffers and materials inside the existing `Model` objects are replaced. The `ModelNode` hierarchy is updated. Rendering on the next frame uses the new geometry.

### Story 6 — Error on missing position attribute (Priority: P1)

As an engine developer, I want the model loader to fail with a clear error if a glTF mesh primitive has no POSITION attribute.

**Given** a glTF file where a mesh primitive lacks `POSITION` accessor

**When** I call `create<ModelAsset>(id)`

**Then** an `InvalidArgument` error is returned indicating the missing position attribute.

### Story 7 — glTF material without textures (Priority: P2)

As an engine developer, I want a glTF model with a material that has only factors (no textures) to load successfully with a `PbrMaterial` using factor values only.

**Given** a glTF file where a material has `pbrMetallicRoughness` with `baseColorFactor: [0.5, 0.5, 0.5, 1.0]` and no textures

**When** I call `create<ModelAsset>(id)`

**Then** the model loads successfully, the `PbrMaterial` has `base_color_factor = (0.5, 0.5, 0.5, 1.0)`, and all texture pointers in the `PbrMaterialData` are null.

### Story 8 — Missing texture fallback (Priority: P2)

As an engine developer, I want a glTF model with a broken texture reference to still load, using a 1×1 magenta fallback texture.

**Given** a glTF file where a texture image file is missing on disk

**When** I call `create<ModelAsset>(id)`

**Then** the model loads successfully, a warning is logged about the missing texture, and the corresponding `PbrMaterialData` texture slot contains the magenta fallback texture.

### Story 9 — Asset type mismatch (Priority: P1)

**Given** a YAML file with `type: Texture` (not `Model`)

**When** I call `create<ModelAsset>("models/box/Box")`

**Then** an `InvalidArgument` error is returned indicating the type mismatch.

### Story 10 — Apply uniform scale from YAML (Priority: P2)

**Given** a YAML file with `settings.scale: 2.0`

**When** I call `create<ModelAsset>(id)`

**Then** all vertex positions in the resulting `Model` objects are scaled by 2.0.

### Story 11 — Empty node with no mesh (Priority: P3)

**Given** a glTF file containing a node with no mesh reference (transformation-only node)

**When** I call `create<ModelAsset>(id)`

**Then** the corresponding `ModelNode` has `model == std::nullopt`, but its transform and children are preserved.

### Story 12 — Node with unsupported primitive mode (Priority: P3)

**Given** a glTF file where a mesh primitive uses `mode: 0` (POINTS)

**When** I call `create<ModelAsset>(id)`

**Then** the primitive is skipped with a warning. Other primitives in the same mesh are loaded normally.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `ModelAsset` class exists in `src/engine/asset/model_asset.h` wrapping a `ModelNode`. Provides `root_node() -> const ModelNode&` accessor and `replace_root()` for hot-reload. | File compiles; `ModelAsset` is a concrete `Asset` subclass. |
| AC-002 | `ModelNode` struct exists with fields `name`, `translation`, `rotation`, `scale`, `std::optional<Model> model`, `std::vector<ModelNode> children`. | File compiles; struct is publicly constructible and movable. |
| AC-003 | `PbrMaterial` class exists in `src/engine/render/pbr/pbr_material.h` following the `PhongMaterial` pattern with embedded GLSL shaders. | File compiles; class extends `Material`; embedded shader sources exist. |
| AC-004 | `PbrMaterialData` struct exists with fields for base colour factor, metallic/roughness factors, emissive factor, double-sided flag, and five texture slots. | File compiles; struct is publicly accessible. |
| AC-005 | `AssetManager::create<ModelAsset>(id)` with valid YAML (`type: Model`, valid `.gltf` source) returns `Result<shared_ptr<ModelAsset>>`. | Headless test: load Khronos Box model, verify result is success. |
| AC-006 | `ModelAsset` cache works: calling `create<ModelAsset>(id)` twice returns the same `shared_ptr` address. | Headless test: two calls return same `get()` address. |
| AC-007 | Khronos Box model loads with 1 `ModelNode` child, 1 `Model`, 1 `SubMesh`, 24 vertices, 36 indices. | Headless test: verify vertex count and submesh count. |
| AC-008 | DamagedHelmet model loads successfully with PBR materials and textures. | Headless test: call `create<ModelAsset>`, verify no error, verify PbrMaterial has textures set. |
| AC-009 | `ModelNode` tree reflects glTF node hierarchy: root node has children corresponding to glTF scene root nodes. | Headless test: load DamagedHelmet, verify `root_node().children` size matches glTF scene root count. |
| AC-010 | Each `ModelNode` with a mesh has a valid `Model` with submeshes and materials. | Headless test: traverse tree, for each node with `model.has_value()`, verify submeshes and materials are non-empty. |
| AC-011 | Missing `POSITION` attribute returns `InvalidArgument` error. | Headless test: inject/modify glTF data to remove POSITION, call `create<ModelAsset>`, verify error category `InvalidArgument`. |
| AC-012 | Corrupt/invalid glTF file returns `InvalidFormat` error. Missing/unreadable file returns `IoFailed`. | Headless test: load nonexistent file → `IoFailed`. Load corrupt binary data → `InvalidFormat`. |
| AC-013 | Type mismatch: YAML with `type: Texture` requested as `ModelAsset` returns `InvalidArgument`. | Headless test: create YAML with `type: Texture`, request `ModelAsset` → `InvalidArgument`. |
| AC-014 | YAML `version: 2` returns `Unsupported` error. | Headless test: create YAML with `version: 2`, request `ModelAsset` → `Unsupported`. |
| AC-015 | PbrMaterial with embedded shaders compiles and renders. Shader known uniforms include PBR parameter names. | Headless test: create `PbrMaterial`, verify known uniforms include standard PBR uniforms. |
| AC-016 | glTF material without textures creates a `PbrMaterial` with null texture slots and correct factor values. | Headless test: load a model with factor-only materials, verify `PbrMaterialData` factor values and null textures. |
| AC-017 | Missing texture reference falls back to magenta 1×1 texture. | Headless test: inject broken texture URI in glTF, load model, verify texture is not null and has 1×1 magenta content. |
| AC-018 | `settings.scale: 2.0` from YAML scales all vertex positions by 2.0. | Headless test: load model with scale 2.0, verify vertex positions are doubled vs scale 1.0. |
| AC-019 | glTF node with no mesh produces `ModelNode` with `model == std::nullopt` and preserved children. | Headless test: load model with transform-only node, verify `model` is nullopt, children are intact. |
| AC-020 | Primitive with unsupported mode (POINTS, LINES, TRIANGLE_FAN, TRIANGLE_STRIP) is skipped with a warning. | Headless test: inject unsupported mode, load model, verify no crash and warning logged. |
| AC-021 | Hot-reload of `.gltf` source file triggers model rebuild. `replace_root()` is called and hierarchy updates. | Headless test (via `#ifdef BUDDD_TESTING`): inject synthetic `FileEvent` for `.gltf`, call `poll_file_events()`, verify `ModelNode` tree has been updated. |
| AC-022 | `AssetManager` template `create<T>()` compiles for `ModelAsset` and is rejected for unsupported types. | Static assertion: `create<int>(...)` fails at compile time. |
| AC-023 | `create_model(id)` convenience method exists and returns `Result<shared_ptr<ModelAsset>>`. | Header inspection: `create_model(id)` is declared. Unit test: call it and verify same result as `create<ModelAsset>`. |
| AC-024 | glTF index type detection: Uint16 and Uint32 indices are both supported. | Headless test: load a model with Uint32 indices, verify index buffer type matches. |
| AC-025 | `ModelAsset::replace_root()` is private and is not publicly accessible (tested via friend access). | Compile test: calling `replace_root()` from outside `AssetManager` fails. |
| AC-026 | The `PbrMaterialData` `double_sided` flag is correctly read from glTF `doubleSided` and stored. | Headless test: load a model with `doubleSided: true` material, verify flag. |
| AC-027 | Vertex colour: glTF `COLOR_0` VEC3 is expanded to VEC4 with alpha=1.0. VEC4 is passed through. | Headless test: load model with vertex colours, verify `Vertex::color` values. |
| AC-028 | Normal default: glTF mesh without NORMAL attribute produces vertices with normal (0,0,1). | Headless test: load model without normals, verify normal field. |

## E2E Verification

- **`gltf_demo_app`**: loads Box or DamagedHelmet from YAML, renders it with orbit camera, verifies correct rendering output (visual inspection or screenshot capture).
- **`hot_reload_gltf_app`**: loads a model, modifies the source `.gltf` file programmatically, triggers `poll_file_events()`, verifies the rendered model updates without restart.
- **Headless test suite**: all AC-xxx criteria are verified in headless mode via Catch2 tests.

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An application developer can load and render a glTF model from YAML in under 20 lines of C++ (excluding boilerplate). | Count lines in `gltf_demo_app`. |
| SC-002 | All model loading logic is testable in headless mode without a GPU or display server. | `ctest --preset debug` on a headless CI runner passes all model loading tests. |
| SC-003 | Box model (24 vertices) loads and creates GPU resources in under 10ms. | Headless test timing (micro-benchmark). |
| SC-004 | Hot-reload of a glTF model is reflected within 2 frames of `poll_file_events()`. | Integration test: load, modify source, poll, verify ModelNode tree changed. |
| SC-005 | Missing texture fallback (magenta) is visible immediately and does not crash. | Headless test: load model with missing texture, verify magenta fallback is used. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| `create<ModelAsset>(id)` with ID containing path traversal sequences (`../`) | The path is resolved relative to `base_path_`. Path traversal is allowed at the filesystem level but discouraged. No sanitisation for V1. |
| glTF file with no default scene | Use the first scene in the `scenes` array. If no scenes exist, treat all root-level nodes as children of the root. |
| glTF file with no meshes | Returns a `ModelAsset` with a root `ModelNode` with no children having `model`. No error — valid glTF files can have cameras/lights only. |
| glTF file with multiple scenes | Only the default scene (or first) is loaded. Other scenes are ignored. |
| glTF mesh with zero primitives | The node has `model == std::nullopt` (no Model created). |
| glTF mesh with multiple materials across primitives | Each primitive maps to a `SubMesh` with a unique material index. Shared materials are deduplicated in the materials vector. |
| glTF with external binary data (`.bin` file) | The `.bin` file path from `glTF.buffers[0].uri` is resolved relative to the glTF file. If the `.bin` file is missing, loading fails with `IoFailed`. |
| glTF with embedded binary data (data URI) | The base64-encoded data is decoded by tinygltf. No additional handling needed. |
| glTF texture with unsupported format (JPEG, etc.) | stb_image handles PNG, JPEG, BMP, GIF, etc. Unsupported formats trigger the magenta fallback. |
| glTF with KHR_materials_pbrSpecularGlossiness (extension) | Not supported in V1. The material is loaded with default PBR factors (white base colour, default metallic/roughness). A warning is logged. |
| glTF with `alphaMode: MASK` or `alphaMode: BLEND` | Not supported in V1. The material is loaded as opaque (alpha channel of base colour is ignored). A warning is logged. |
| glTF animation data present | Ignored. The model loads without animation. |
| glTF camera or light nodes | Ignored. No camera/light objects are created. |
| glTF with Draco or meshopt compression | tinygltf will fail to decompress. The error propagates as `InvalidArgument` or `Unsupported` with a descriptive message. |
| Same glTF file referenced by multiple YAML files | Each YAML file produces a separate cached `ModelAsset` entry (cache key is the ID, not the source path). Both caches are independent. |
| Hot-reload of a glTF file that fails to parse after a valid load | The old model is retained. Warning logged. No crash. |
| `poll_file_events()` during model loading | `poll_file_events()` is not re-entrant. If called while `create<ModelAsset>` is executing, behaviour is undefined (same restriction as other asset types). |
| glTF with extremely large vertex count (>1M vertices) | Model loading is synchronous. Large models may cause a visible frame hitch. No async loading in V1. |

## Error cases

| Case | Expected behaviour |
|---|---|
| YAML file not found | `create<ModelAsset>(id)` returns `make_error(Error::Category::IoFailed, "YAML file not found: <path>")` |
| YAML syntax error | Exception caught, returns `IoFailed` with parse error message. |
| YAML `type` field missing or not "Model" | `make_error(Error::Category::InvalidArgument, "Expected type 'Model', got '<type>'")` |
| YAML `version` > 1 | `make_error(Error::Category::Unsupported, "Unsupported Model version: N")` |
| YAML `source` field missing or empty | `make_error(Error::Category::InvalidArgument, "Model 'source' field is required")` |
| glTF/glb source file not found | `make_error(Error::Category::IoFailed, "glTF source not found: <path>")` |
| glTF file is corrupt/invalid | tinygltf parse failure → `make_error(Error::Category::InvalidFormat, "glTF parse error: <message from tinygltf>")` |
| Mesh primitive missing `POSITION` attribute | `make_error(Error::Category::InvalidArgument, "Mesh <name> primitive has no POSITION attribute")` |
| glTF references an accessor, buffer view, or buffer with out-of-bounds indices | tinygltf returns error, propagated as `InvalidArgument`. |
| Mesh primitive has unsupported topology mode | Primitive skipped with warning. Other primitives load normally. |
| Material extension not supported (specular-glossiness) | Warning logged. Material loaded with default PBR values. |
| Embedded texture image data fails to decode (invalid format, corrupt) | Magenta fallback used. Warning logged. |
| External texture URI is missing or unreadable | Magenta fallback used. Warning logged. |
| Scale field in YAML is negative | Negative scale is applied to positions. Normals are renormalised. This is valid (mirroring). |
| YAML `settings.scale` is zero | Zero scale collapses all vertices to origin. This is valid but unusual — no error. |
| Hot-reload: glTF file fails to parse after source change | Old model retained. Warning logged. Rendering continues. |
| Hot-reload: YAML source path changed | The model is reloaded from the new source path. If the new source fails, the old model is retained. |

## Permissions and security

- No elevated privileges required to read glTF, glb, or image files.
- Asset files are read from the local filesystem only. No network access.
- File paths from YAML are resolved against the working directory. Path traversal is possible but is an already-existing risk.
- tinygltf is a header-only parser that reads from paths or callbacks. It does not execute arbitrary code.
- The magenta fallback texture prevents rendering with uninitialised texture state.
- All GPU resources are managed through the existing `RenderDevice` abstraction, following CONST-001.
- Headless mode requires no GPU or display access, maintaining CI safety.

## Observability

All observability uses `std::cerr` consistent with the project pattern.

| Signal | Source |
|---|---|
| `[Asset] Model loaded: <id> (<vertex_count> verts, <node_count> nodes)` | After successful model creation |
| `[Asset] Cache hit: <id>` | On cache hit (debug builds only) |
| `[Asset] Model load failed: <id> - <message>` | On load failure |
| `[Asset] glTF mesh <name>: <primitive_count> primitives, <vertex_count> vertices` | Per mesh (debug builds only) |
| `[Asset] glTF material <name>: PBR created, <texture_count> textures` | Per material (debug builds only) |
| `[Asset] Hot-reload: <id> (glTF source changed)` | On glTF source change detected |
| `[Asset] Hot-reload: model reloaded: <id>` | After successful hot-reload |
| `[Asset] Hot-reload: model reload failed: <id> - retaining old model` | On hot-reload failure |
| `[Asset] Warn: unsupported primitive mode <N> in mesh <name> - skipping` | Unsupported topology mode |
| `[Asset] Warn: material ext not supported: <name>` | KHR_materials_pbrSpecularGlossiness etc. |
| `[Asset] Warn: texture load failed for <uri> - using magenta fallback` | Texture load failure |
| `[Asset] PbrMaterial created: <name>` | PbrMaterial instantiation (debug builds only) |

## Out of scope

- Animations (skeletal, vertex, morph target).
- Cameras, lights from glTF.
- Draco/meshopt compression.
- KHR_materials extensions beyond core metallic-roughness.
- Non-triangle primitive modes (points, lines, triangle fans, triangle strips).
- Scene selection (only default/first scene loaded).
- Model editing or in-memory material reassignment.
- Async model streaming.
- Additional 3D formats (OBJ, FBX, etc.).
- Model LOD.
- Automatic tangent computation.
- Alpha blending or alpha mask (alpha channel is ignored in V1).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | tinygltf version 2.9.7 (or latest stable) is available via GitHub and compatible with C++26 and the project's CMake setup. |
| A-02 | The `assets/models/` directory exists and contains the Box and DamagedHelmet model files. |
| A-03 | glTF files are well-formed according to the glTF 2.0 specification. | 
| A-04 | The engine's coordinate system is right-handed Y-up, matching glTF's default. No coordinate conversion is needed. |
| A-05 | The `math::Quat` type exists and supports multiplication with `math::Vec3` for rotation application. |
| A-06 | The magenta fallback texture is a 1×1 RGBA8 texture with values (1, 0, 1, 1), created once and shared across all `PbrMaterial` instances that need it. |
| A-07 | The `RenderDevice::create_texture()` method accepts raw pixel data (not just `Image` objects). Or `Image` can be constructed from raw pixel data. |
| A-08 | glTF textures with embedded buffer view data can be passed to `RenderDevice::create_texture()` via an `Image` wrapper or direct pixel data API. |
| A-09 | The `ModelNode` tree uses `std::optional<Model>` to indicate nodes without mesh data. This is not a performance concern for V1 (models have limited node counts). |
| A-10 | Hot-reload of a glTF model replaces the entire `ModelNode` tree via `replace_root()`. All external `shared_ptr` references to old `Model` objects or `PbrMaterial` objects may be invalidated. Callers must re-traverse the tree after hot-reload. |
| A-11 | The `.gltf` file and its associated `.bin` file are treated as a single dependency. The YAML file records the `.gltf`/`.glb` path as the dependency — the `.bin` file is tracked implicitly (if the `.bin` changes, the `.gltf` must also change to reflect a different buffer view). |
| A-12 | New files to be created: |
| | - `src/engine/asset/model_asset.h` — `ModelAsset` class |
| | - `src/engine/asset/model_asset.cpp` — implementation |
| | - `src/engine/render/pbr/pbr_material.h` — `PbrMaterial` class |
| | - `src/engine/render/pbr/pbr_material.cpp` — implementation |
| | - `src/engine/render/pbr/pbr_shaders.h` — embedded GLSL PBR shaders |
| | - `src/engine/render/model_node.h` — `ModelNode` struct |
| | - `src/engine/render/model_node.cpp` — (if needed) |
| | - `src/engine/asset/model_loader.h` — internal glTF→engine conversion |
| | - `src/engine/asset/model_loader.cpp` — tinygltf integration |
| | - `src/cmd/apps/gltf_demo_app.h` — demo app |
| | - `src/cmd/apps/gltf_demo_app.cpp` — demo app implementation |
| | - `src/cmd/apps/hot_reload_gltf_app.h` — hot-reload test app |
| | - `src/cmd/apps/hot_reload_gltf_app.cpp` — hot-reload test app implementation |
| | - `tests/model_asset_tests.cpp` — model asset tests |
| | - `assets/models/box/Box.gltf` + `.bin` — Khronos Box |
| | - `assets/models/box/Box.yaml` |
| | - `assets/models/damaged-helmet/` — DamagedHelmet glTF + textures + YAML |
| | Modified files: |
| | - `src/engine/asset/asset_manager.tpp` — add `ModelAsset` to `static_assert` |
| | - `src/engine/asset/asset_manager.h` — add `create_model()` and `load_model()` |
| | - `src/engine/asset/asset_manager.cpp` — add load_model implementation |
| | - `src/engine/CMakeLists.txt` — add tinygltf FetchContent |
| A-13 | The `ModelNode` struct and `ModelAsset` do not need to be in the render layer (no GL types). They live in `src/engine/render/` (for `ModelNode`, since it contains `std::optional<Model>`) and `src/engine/asset/` (for `ModelAsset`, as it extends `Asset`). |
| A-14 | `ModelNode` is defined in `src/engine/render/model_node.h` because it holds a `std::optional<Model>` (`Model` is in the render layer). `ModelAsset` is in `src/engine/asset/` and includes `model_node.h`. |
| A-15 | glTF material texture coordinates use `TEXCOORD_0` by default. `TEXCOORD_1` is not supported in V1 — textures with `texCoord > 0` are skipped with a warning. |

## Documents requiring updates

The following existing documents require updates as a result of this feature:

- `docs/specs/asset-manager/spec.md` — Update the non-goals section (currently states "No glTF/glb model loading (models remain programmatic for V1)") to reflect that glTF loading is now supported.
- `docs/wiki/architecture/module-map.md` — Add entries for new modules: `ModelAsset` (asset/), `ModelNode` (render/), `PbrMaterial` (render/pbr/), `ModelLoader` (asset/).
- `docs/wiki/domain/glossary.md` — Add terms: `ModelAsset`, `ModelNode`, `PbrMaterial`, `PbrMaterialData`.
- `docs/adr/` — Create a new ADR for the tinygltf dependency decision (FetchContent integration, rationale).

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | Should `ModelNode` be in `src/engine/render/` (because it contains `std::optional<Model>`) or `src/engine/asset/` (because it's an asset concept)? | **Assumption A-14**: `ModelNode` is in `src/engine/render/model_node.h` because it contains `Model`. `ModelAsset` in `src/engine/asset/` includes it. |
| Q-02 | Should embedded textures be cached in the AssetManager's texture cache or owned directly by the `PbrMaterial`? | **Assumption**: Not cached as `TextureAsset`. Created direct from image data, owned by `PbrMaterial`. The glTF source file is the single dependency. |
| Q-03 | How should glTF `alphaMode` (OPAQUE, MASK, BLEND) be handled in V1? | **Resolved**: All materials are treated as opaque. Alpha mode values other than OPAQUE produce a warning. Alpha channel of base colour is ignored. |
| Q-04 | Should tinygltf be fetched via FetchContent or included as a vendored header? | **Resolved**: FetchContent, following the existing pattern for yaml-cpp, stb, etc. |
| Q-05 | How should the `.bin` file dependency be tracked? | **Assumption A-11**: Only the `.gltf`/`.glb` path is tracked as a dependency. If the `.bin` changes independently, the `.gltf` must be re-saved (which updates its timestamp) to trigger hot-reload. |

## Appendix A: PBR Shader Model

The embedded PBR shaders implement the glTF 2.0 metallic-roughness shading model:

### Vertex shader

Standard vertex transformation with normal and tangent output:

```glsl
#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec2 a_texcoord;
layout(location = 4) in vec4 a_tangent;   // Reserved, not used in V1
layout(location = 5) in vec2 a_texcoord2;  // Reserved, not used in V1

uniform mat4 u_mvp;
uniform mat4 u_model;
uniform mat4 u_normal_mat;

out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_texcoord;

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_pos = world_pos.xyz;
    v_normal = normalize(mat3(u_normal_mat) * a_normal);
    v_texcoord = a_texcoord;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
```

### Fragment shader

Implements the Cook-Torrance BRDF with Lambertian diffuse:

```glsl
#version 450 core

in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_texcoord;

out vec4 frag_color;

// Material parameters
uniform vec4  u_base_color_factor;
uniform float u_metallic_factor;
uniform float u_roughness_factor;
uniform vec3  u_emissive_factor;

// Textures
uniform sampler2D u_base_color_texture;
uniform sampler2D u_metallic_roughness_texture;
uniform sampler2D u_normal_texture;
uniform sampler2D u_occlusion_texture;
uniform sampler2D u_emissive_texture;

// Has-texture flags (1.0 = texture present, 0.0 = use factor)
uniform float u_has_base_color_texture;
uniform float u_has_metallic_roughness_texture;
// ... etc for each texture

// Lighting (same as Phong for V1 compatibility)
uniform vec3 u_camera_pos;
uniform int  u_light_count;
uniform vec4 u_light_positions_or_dir[8];
uniform vec4 u_light_colours[8];
uniform float u_light_ranges[8];
uniform vec4 u_light_spot_directions[8];
uniform float u_light_inner_cones[8];
uniform float u_light_outer_cones[8];

const float PI = 3.14159265359;
const float k_exposure = 3.0;

float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometry_schlick_ggx(NdotV, roughness)
         * geometry_schlick_ggx(NdotL, roughness);
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_camera_pos - v_world_pos);

    // Sample base colour
    vec4 base_color = u_base_color_factor;
    if (u_has_base_color_texture > 0.5) {
        base_color *= texture(u_base_color_texture, v_texcoord);
    }

    // Sample metallic-roughness
    float metallic = u_metallic_factor;
    float roughness = u_roughness_factor;
    if (u_has_metallic_roughness_texture > 0.5) {
        vec4 mr = texture(u_metallic_roughness_texture, v_texcoord);
        roughness *= mr.g;
        metallic *= mr.b;
    }

    // Sample occlusion
    float occlusion = 1.0;
    if (u_has_occlusion_texture > 0.5) {
        occlusion = texture(u_occlusion_texture, v_texcoord).r;
    }

    // Sample emissive
    vec3 emissive = u_emissive_factor;
    if (u_has_emissive_texture > 0.5) {
        emissive *= texture(u_emissive_texture, v_texcoord).rgb;
    }

    // F0 for dielectrics (0.04) or metals (base_color.rgb)
    vec3 F0 = mix(vec3(0.04), base_color.rgb, metallic);

    // Loop over lights
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_light_count; ++i) {
        vec3 L;
        float attenuation = 1.0;
        // ... (same light calculation as Phong fragment shader)

        vec3 H = normalize(L + V);
        float NdotL = max(dot(N, L), 0.0);

        // Cook-Torrance BRDF
        float D = distribution_ggx(N, H, roughness);
        float G = geometry_smith(N, V, L, roughness);
        vec3  F = fresnel_schlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);

        vec3 specular = D * G * F / (4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001);
        vec3 diffuse = kD * base_color.rgb / PI;

        Lo += (diffuse + specular) * u_light_colours[i].rgb * NdotL * attenuation;
    }

    vec3 ambient = base_color.rgb * occlusion * 0.03;
    vec3 color = ambient + Lo + emissive;

    // Tone mapping (Reinhard)
    vec3 exposed = color * k_exposure;
    vec3 tonemapped = exposed / (exposed + vec3(1.0));
    if (isnan(tonemapped.x) || isnan(tonemapped.y) || isnan(tonemapped.z)) {
        tonemapped = vec3(0.0);
    }

    frag_color = vec4(tonemapped, 1.0);
}
```

**Uniform names** (known uniform list):
```
u_mvp, u_model, u_normal_mat
u_camera_pos
u_light_count, u_light_positions_or_dir, u_light_colours, u_light_ranges
u_light_spot_directions, u_light_inner_cones, u_light_outer_cones
u_base_color_factor, u_metallic_factor, u_roughness_factor, u_emissive_factor
u_has_base_color_texture, u_has_metallic_roughness_texture
u_has_normal_texture, u_has_occlusion_texture, u_has_emissive_texture
```

## Appendix B: glTF → Engine data flow

```
.gltf/.glb file
      │
      ▼
tinygltf::LoadImageFromFile / tinygltf::LoadFromFile
      │
      ▼
tinygltf::Model
      │
      ▼
ModelLoader::load_model(tinygltf::Model, settings)
      │
      ├─── Parse each mesh:
      │       For each primitive:
      │         - Read POSITION, NORMAL, TEXCOORD_0, COLOR_0 accessors
      │         - Convert to engine Vertex (72-byte struct)
      │         - Build index buffer (Uint16 or Uint32)
      │         - Record material index
      │       Build Model::create_indexed() with combined buffers
      │
      ├─── Parse each material:
      │       - Extract pbrMetallicRoughness factors
      │       - Load referenced textures (embedded or external)
      │       - Create PbrMaterial via RenderDevice
      │       - Apply factors and textures
      │       - Return shared_ptr<PbrMaterial>
      │
      ├─── Build ModelNode tree:
      │       - Recursively process glTF node hierarchy
      │       - Attach Model to nodes with meshes
      │       - Preserve TRS transforms
      │
      └─── Return ModelNode root
              │
              ▼
        ModelAsset(root_node)
              │
              ▼
        Cached in AssetManager::cache_
```

## Appendix C: Build system changes

Add tinygltf via FetchContent in `src/engine/CMakeLists.txt`:

```cmake
# ----- tinygltf (glTF 2.0 model loading, header-only) -----
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.9.7
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
              -DTINYGLTF_BUILD_LOADER_EXAMPLE=OFF
              -DTINYGLTF_BUILD_LOADER_TEST=OFF
)
FetchContent_MakeAvailable(tinygltf)
```

Add include directory:

```cmake
target_include_directories(buddd_engine PRIVATE
    ${stb_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
    ${tinygltf_SOURCE_DIR}          # header-only: just the include path
)
```

No link-time changes needed — tinygltf is header-only.

An ADR should be created to document the tinygltf dependency decision.
