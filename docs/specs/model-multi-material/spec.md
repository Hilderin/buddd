# SPEC-010 — Multi-Material Model, Primitive Helpers & API Cleanup

## Status

`Draft`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | |
| Date | |

## Problem

The current `Model` class (SPEC-009) has several design issues:

1. **Single material only**: `Model` stores one `shared_ptr<Material>`, making multi-material models (glTF, etc.) impossible without splitting into multiple `Model` instances.
2. **Non-indexed support**: `Model` supports both indexed and non-indexed geometry via two factory methods (`create` and `create_indexed`). This dual path complicates every internal method (`draw()`, `material()`, etc.) and adds branching.
3. **Two-phase creation**: The proposed `add_submesh()` / `set_material()` additive API adds mutation where immutability is simpler and safer.
4. **SubMesh.material is a shared_ptr**: Direct material pointer in SubMesh prevents multiple submeshes from cleanly sharing the same material by index, and makes it impossible to have a material list separate from submesh definitions.
5. **No primitive helpers**: Every app that needs a cube or triangle creates geometry data inline. `demo::setup_cube()` exists but lives in `cmd/` and can't be used by engine code or tests.
6. **material() accessor is ambiguous**: Returning `Result<Material&>` for the "first submesh's material" is a convenience that encourages bad patterns (assuming there's a single material).

The engine needs a clean, unified Model that supports multi-material natively, provides reusable primitive helpers, and removes legacy dual-path complexity.

## Goals

- **New SubMesh struct**: `{uint32_t index_start, uint32_t index_count, uint32_t material_index}` — materials are stored in a flat list on the Model, referenced by index.
- **Unified single factory**: `Model::create_indexed()` takes `std::vector<SubMesh>` + `std::vector<std::shared_ptr<Material>>` upfront. No mutation after creation.
- **No non-indexed support**: All Models have an index buffer. `Model::create()` (non-indexed) is removed.
- **No `material()` accessor**: Removed. Use `model.materials()[i]` to access materials by index.
- **No `add_submesh()` / `set_material()` / two-phase**: Everything is specified at creation.
- **Primitive helpers**: `engine::create_cube()`, `engine::create_triangle()`, `engine::create_quad()` in `src/engine/render/primitives.h/.cpp`.
- **Fallback material**: `RenderDevice::fallback_material()` for defensive drawing (null/out-of-bounds materials).
- **Migration**: All existing apps migrated to new API.
- **Verification**: Multi-material demo (`buddd run multi-material`) with 3 submeshes (red/green/blue).

## Non-goals

- No model loading from files (glTF, OBJ, FBX) — deferred.
- No submesh removal or reordering after creation.
- No material deduplication or sharing optimization (beyond index-based sharing).
- No instanced rendering or multi-draw indirect.
- No LOD (Level-of-Detail) per submesh.
- No skeletal animation or skinning.
- No submesh transform offsets (all submeshes share the same local-space geometry).
- No shader compilation or material linking inside Model factories.
- No `MeshRenderer` changes — it already calls `model.draw(device)`.
- No changes to `error.h` — existing error categories suffice.
- No changes to `render_device.h` beyond adding `fallback_material()`.

## Actors

| Actor | Description |
|---|---|
| Engine developer | Creates Models with multiple materials via the unified factory. Uses primitive helpers for quick geometry. |
| Application developer | Works with glTF assets. Iterates `model.materials()` to set uniforms per material. Calls `model.draw()`. |
| Demo app maintainer | Updates existing apps to use primitive helpers and new Model API. |
| Test suite | Headless tests verify multi-material draw call counts, primitive geometry, and fallback behavior. |

## SubMesh struct

```cpp
namespace buddd::engine {

struct SubMesh {
    uint32_t index_start;       ///< First index in the Model's index buffer.
    uint32_t index_count;       ///< Number of indices in this submesh.
    uint32_t material_index;    ///< Index into the Model's materials() vector.
};

} // namespace buddd::engine
```

- `SubMesh` is a plain aggregate — compiler-generated defaults.
- `material_index` must be `< model.materials().size()`. Out-of-bounds access uses the fallback material at draw time.
- Multiple submeshes can share the same `material_index`.

## Model API

```cpp
class Model {
public:
    // -- Factory (the only one) --
    /// Creates an indexed Model with submeshes and materials.
    /// All geometry and material data is specified upfront.
    /// Returns InvalidArgument on empty vertex data, empty index data,
    /// or invalid vertex format.
    [[nodiscard]] static auto create_indexed(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::span<const std::byte> index_data,
        IndexType index_type,
        std::vector<SubMesh> submeshes,
        std::vector<std::shared_ptr<Material>> materials,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;

    // -- Drawing --
    /// Issues one draw_indexed call per SubMesh.
    /// Each call binds materials_[submesh.material_index] (or fallback if null/out-of-bounds).
    /// No-op if submeshes vector is empty.
    auto draw(RenderDevice& device) const -> void;

    // -- Accessors --
    auto submeshes() const noexcept -> const std::vector<SubMesh>&;
    auto materials() const noexcept -> const std::vector<std::shared_ptr<Material>>&;
    auto vertices() const noexcept -> const VertexBuffer&;
    auto indices() const noexcept -> const IndexBuffer&;
    auto vertex_count() const noexcept -> uint32_t;
    auto index_count() const noexcept -> uint32_t;

    // -- Lifecycle --
    Model(const Model&) = delete;
    auto operator=(const Model&) -> Model& = delete;
    Model(Model&& other) noexcept;
    auto operator=(Model&& other) noexcept -> Model&;
    ~Model() = default;

private:
    Model(
        std::unique_ptr<VertexBuffer> vb,
        std::unique_ptr<IndexBuffer> ib,
        std::vector<SubMesh> submeshes,
        std::vector<std::shared_ptr<Material>> materials,
        PrimitiveTopology topology,
        uint32_t vertex_count,
        uint32_t index_count
    ) noexcept;

    std::unique_ptr<VertexBuffer> vb_;
    std::unique_ptr<IndexBuffer> ib_;
    std::vector<SubMesh> submeshes_;
    std::vector<std::shared_ptr<Material>> materials_;
    PrimitiveTopology topology_;
    uint32_t vertex_count_;
    uint32_t index_count_;
};
```

### Factory behavior

- Creates VertexBuffer from vertex_data.
- Creates IndexBuffer from index_data.
- Stores submeshes and materials as members.
- Validates: non-empty vertex_data, non-empty index_data, valid vertex format.
- Does NOT validate index ranges or material_index bounds (caller responsibility).
- On error: returns appropriate InvalidArgument or ResourceCreationFailed error. Partial resources cleaned up via RAII.

### draw() behavior

```
for each submesh in submeshes_:
    material = materials_[submesh.material_index]  if valid and non-null
            ? *materials_[submesh.material_index]
            : device.fallback_material()
    device.draw_indexed(topology_, *vb_, *ib_, material,
                        submesh.index_count, submesh.index_start)
```

- If `submeshes_` is empty → no-op (no crash).
- If `materials_` is empty → all submeshes use fallback → draws with magenta.
- Null material at valid index → uses fallback for that submesh.
- Out-of-bounds `material_index` → uses fallback for that submesh.
- Moved-from (null) Model → no-op.

### material() and has_indices() removed

- `material()` is removed. Callers must use `materials()[i]` or iterate `materials()`.
- `has_indices()` is removed. All Models have indices.
- `Model::create()` (non-indexed) is removed. All creation goes through `create_indexed()`.

## Primitive helpers

New file: `src/engine/render/primitives.h` and `src/engine/render/primitives.cpp`.

These are **geometry factories** — they create vertex/index data for common primitives and wrap them in a Model with the caller-provided material.

```cpp
namespace buddd::engine {

/// Creates a unit cube (2×2×2, centred at origin).
/// 24 vertices (Float3 position + Float3 colour), 36 Uint16 indices, 1 submesh.
/// The provided material is stored at materials()[0].
[[nodiscard]] auto create_cube(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

/// Creates a coloured right triangle.
/// 3 vertices (Float3 position + Float3 colour), 3 Uint16 indices, 1 submesh.
[[nodiscard]] auto create_triangle(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

/// Creates a unit quad (2 triangles) in the XY plane.
/// 4 vertices (Float3 position + Float3 colour), 6 Uint16 indices, 1 submesh.
[[nodiscard]] auto create_quad(
    RenderDevice& device,
    std::shared_ptr<Material> material
) -> Result<Model>;

} // namespace buddd::engine
```

Each helper:
- Defines inline vertex/index data (Float3 position + Float3 colour, stride 24).
- Calls `Model::create_indexed()` with one SubMesh covering all indices and the provided material.
- Returns the Model on success, or propagates errors from buffer creation.

The material is **mandatory** — primitives cannot render without a material/shaders. Callers create their own material (with desired shaders and uniforms) and pass it in.

Migration note: existing `demo::setup_cube()` and `demo::setup_triangle()` created materials internally with embedded shaders. After migration, each app creates its own material first, then passes it to the primitive helper. This gives apps full control over shaders, uniforms, and material type.

Migration example (cube_app):
```cpp
// Before:
auto cube = demo::setup_cube(device);
cube.material->set_uniform("u_mvp", mvp);
cube.model.draw(device);

// After:
auto vs = device.create_shader(Vertex, vertex_src);
auto fs = device.create_shader(Fragment, fragment_src);
auto mat = std::shared_ptr<Material>(std::move(*device.create_material(*vs, *fs, {"u_mvp"})));
auto model = engine::create_cube(device, mat);
model.materials()[0]->set_uniform("u_mvp", mvp);
model.draw(device);
```

## RenderDevice addition

```cpp
class RenderDevice {
public:
    // ...
    /// Returns a reference to a shared fallback material that renders
    /// solid magenta (RGB 1,0,1). Created once, lives as long as the RenderDevice.
    virtual auto fallback_material() noexcept -> Material& = 0;
    // ...
};
```

## Migration: existing apps and demos

All existing `Model::create()` calls (non-indexed) become `Model::create_indexed()` with the appropriate index data.

| File | Current code | New code |
|---|---|---|
| `demo_helpers.cpp` — `setup_cube()` | `Model::create_indexed(...)` with single material | **Removed**. Use `create_cube()` from engine. |
| `demo_helpers.cpp` — `setup_triangle()` | `Model::create()` + VertexBuffer, no indices | **Removed**. Use `create_triangle()` from engine. |
| `demo_helpers.h` | `CubeResources`, `setup_cube()`, `setup_triangle()` | **Removed**. |
| `cube_app.cpp` | Uses `demo::setup_cube()`, `cube_->material->set_uniform()` | Uses `engine::create_cube()`, `model.materials()[0]->set_uniform()` |
| `triangle_app.cpp` | Uses `demo::setup_triangle()` | Uses `engine::create_triangle()` |
| `phong_app.cpp` | Manual cube creation, `model.material()` | Manual cube via primitives or inline, `model.materials()[i]` for uniforms |
| `cube_scene_app.cpp` | Uses `demo::setup_cube()` | Uses `engine::create_cube()` |
| `textured_cube_app.cpp` | Manual cube with texture | Uses `engine::create_cube()` or manual, `model.materials()[0]` for uniforms |
| `free_camera_app.cpp` | Uses `demo::setup_cube()` | Uses `engine::create_cube()` |
| `asset_demo_app.cpp` | Manual model creation | Uses `engine::create_cube()` or manual |
| `hot_reload_app.cpp` | Manual model creation | Uses `engine::create_cube()` or manual |
| `run_app.cpp` | Creates non-indexed triangle | Uses `engine::create_triangle()` |
| `render_system.cpp` | Uses `mr.model().material()` | Uses `mr.model().materials()[0]` |
| `tests/model_tests.cpp` | 14+ uses of `Model::create()`, 7+ of `has_indices()`, 6+ of `model.material()`, multiple uses of old `create_indexed()` (single material) | **Rewrite**: each `Model::create()` → `create_indexed()`, `model.material()` → `model.materials()[0]`, `has_indices()` calls removed, old `create_indexed()` calls updated to SubMesh+vector signature |
| `tests/lighting_tests.cpp` | Lines 86, 600, 851 use `Model::create()` (non-indexed) | Each `Model::create()` → `create_indexed()` with full index buffer |
| `tests/scene_rendering_tests.cpp` | Lines 303, 389, 449, 596, 599 use `Model::create()` (non-indexed) | Each `Model::create()` → `create_indexed()` with full index buffer |

## Multi-material demo app

New files: `src/cmd/apps/multi_material_app.h` and `src/cmd/apps/multi_material_app.cpp`.

Pattern: `App` subclass (like `CubeApp`).

### Behavior

1. Creates cube geometry (24 vertices, 36 Uint16 indices — same data as `create_cube`).
2. Creates three materials via `device.create_material()`:
   - Material A: solid red (fragment outputs `vec4(1,0,0,1)`)
   - Material B: solid green (fragment outputs `vec4(0,1,0,1)`)
   - Material C: solid blue (fragment outputs `vec4(0,0,1,1)`)
3. Creates Model:
   ```cpp
   auto model = Model::create_indexed(device, format, vertices, indices, Uint16,
       {
           SubMesh{0, 12, 0},   // first two faces → red
           SubMesh{12, 12, 1},  // next two faces → green
           SubMesh{24, 12, 2},  // last two faces → blue
       },
       {red_mat, green_mat, blue_mat}
   );
   ```
4. 120-frame render loop: compute MVP, set `u_mvp` on each material, `model.draw(device)`.

### Registration

In `main.cpp`:
- Add `#include "apps/multi_material_app.h"`
- Add `else if (scene == "multi-material") app = std::make_unique<bc::app::MultiMaterialApp>();`
- Add usage text line

## User stories

### Story 1 — Create multi-material Model (Priority: P1)

As an engine developer, I want to create a Model with multiple submeshes referencing different materials by index.

```cpp
auto model = Model::create_indexed(device, format, vertices, indices, Uint16,
    { SubMesh{0, 36, mat_red_idx}, SubMesh{36, 24, mat_blue_idx} },
    { red_material, blue_material }
);
// model.submeshes().size() == 2
// model.submeshes()[0].material_index == 0  → materials()[0] == red_material
// model.materials().size() == 2
```

### Story 2 — Use primitive helper (Priority: P1)

As an app developer, I want to create a cube with a single call.

```cpp
auto mat = std::make_shared<...>(device.create_material(...));
auto cube = engine::create_cube(device, mat);
// cube.submeshes().size() == 1
// cube.materials().size() == 1
// cube.materials()[0] == mat
```

### Story 3 — Draw iterates submeshes (Priority: P1)

As an engine developer, I want `Model::draw()` to issue one draw call per submesh with the correct material.

Given a Model with 3 submeshes and 3 materials, calling `model.draw(device)` issues 3 `draw_indexed()` calls, each with the correct index range and corresponding material.

### Story 4 — Fallback material for null/out-of-bounds (Priority: P1)

Given a Model where `materials_[1]` is null, the draw call for submeshes with `material_index == 1` uses `device.fallback_material()` (magenta).

### Story 5 — Run multi-material demo (Priority: P1)

As a user, `buddd run multi-material` shows a cube with red, green, and blue face pairs for ~2 seconds.

### Story 6 — Run existing demos unchanged (Priority: P1)

`buddd run triangle`, `buddd run cube`, and all other existing demos work after migration. Visual output is identical.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `SubMesh` struct exists with fields `uint32_t index_start`, `uint32_t index_count`, `uint32_t material_index`. | File compiles. |
| AC-002 | `Model::create_indexed()` accepts `vector<SubMesh>` and `vector<shared_ptr<Material>>`. Creates buffers and stores submeshes/materials. | Headless test: create model, verify submeshes and materials match input. |
| AC-003 | `Model::create_indexed()` returns `InvalidArgument` for empty vertex data. | Headless test. |
| AC-004 | `Model::create_indexed()` returns `InvalidArgument` for empty index data. | Headless test. |
| AC-005 | `Model::draw()` issues N draw calls for N submeshes. | Headless test: create model with 3 submeshes, draw, verify draw_call_count increased by 3. |
| AC-006 | `Model::draw()` binds the correct material for each submesh via material_index. | Headless test: verify materials used match expected via material tracking. |
| AC-007 | `Model::draw()` uses fallback material when `materials_[material_index]` is null. | Headless test: create model with null material at index 0, draw, verify fallback was used. |
| AC-008 | `Model::draw()` uses fallback material when `material_index >= materials().size()`. | Headless test: material_index = 5 with only 2 materials, draw uses fallback. |
| AC-009 | `Model::draw()` is a no-op on empty submeshes. | Headless test: create model with empty submeshes, draw, draw_call_count unchanged. |
| AC-010 | `Model::draw()` is a no-op on moved-from (null) Model. | Headless test. |
| AC-011 | `engine::create_cube()` returns a Model with 1 submesh, 1 material, 36 indices, 24 vertices. | Headless test. |
| AC-012 | `engine::create_triangle()` returns a Model with 1 submesh, 1 material, 3 indices, 3 vertices. | Headless test. |
| AC-013 | `engine::create_quad()` returns a Model with 1 submesh, 1 material, 6 indices, 4 vertices. | Headless test. |
| AC-014 | `RenderDevice::fallback_material()` returns a valid `Material&` that renders magenta. | Headless test: call fallback_material(), verify it's a valid material. |
| AC-015 | `demo::setup_cube()` and `demo::setup_triangle()` are removed. | File inspection: `demo_helpers.h` no longer declares them. |
| AC-016 | `CubeResources` struct is removed. | File inspection: not found in codebase. |
| AC-017 | `Model::material()` does not exist. | Code does not compile if referenced. All callers migrated. |
| AC-018 | `Model::create()` (non-indexed) does not exist. | Code does not compile if referenced. All callers migrated. |
| AC-019 | `Model::has_indices()` does not exist. | Code does not compile if referenced. |
| AC-020 | `buddd run triangle` works identically to before migration. | Run the demo, verify visual output and no crash. |
| AC-021 | `buddd run cube` works identically to before migration. | Run the demo, verify visual output and no crash. |
| AC-022 | `buddd run multi-material` runs without crash, shows 3 coloured face pairs. | Run the demo, verify completion. |
| AC-023 | `Model` is non-copyable and movable. Moving preserves submeshes/materials. | `static_assert` and headless test. |
| AC-024 | `Model::submeshes()` returns const ref. `Model::materials()` returns const ref. | Headless test: const-correct access compiles. |

## E2E Verification

- Run all existing demos (`buddd run triangle`, `buddd run cube`, etc.) — verify they work identically.
- Run `buddd run multi-material` — visually verify 3 coloured face pairs.
- Headless test suite covers all acceptance criteria.

## Edge cases

| Case | Expected behavior |
|---|---|
| Empty submeshes vector | `draw()` is no-op |
| Empty materials vector | All submeshes use fallback (magenta) |
| Null material at valid index | That submesh uses fallback |
| material_index out of bounds | That submesh uses fallback |
| Multiple submeshes sharing same material_index | Single material used for multiple draw calls |
| Moved-from Model | draw() is no-op |
| Index buffer creation fails | Factory returns ResourceCreationFailed error |
| Vertex buffer creation fails | Factory returns ResourceCreationFailed error, prior resources cleaned up |
| `create_cube()` called twice | Returns two independent Models with identical geometry |
| `draw()` called outside begin_frame/end_frame | Undefined behavior (same as before) |
| `submesh.index_start + submesh.index_count` exceeds index buffer length | Undefined behavior (caller error, not validated at creation) |

## Documents requiring updates

| Document | Reason |
|---|---|
| `docs/specs/3d-cube-demo/spec.md` (SPEC-009) | Model API completely redesigned. SPEC-009's Model design is superseded. |
| `docs/wiki/architecture/module-map.md` | Model public API and new primitives module need documenting. |
| `docs/wiki/domain/glossary.md` | Remove CubeResources, setup_cube. Add SubMesh, create_cube, etc. |
| `docs/adr/ADR-010-no-raw-pointers.md` | `Result<Material&>` no longer used (material() removed), so ADR-010 impact reduced. |
| `src/cmd/apps/*.cpp` (all apps) | Migration to new Model API. |
| `tests/model_tests.cpp` | Rewrite for SubMesh-based API: `Model::create()` → `create_indexed()`, `material()` → `materials()[0]`, remove `has_indices()` calls, update old `create_indexed()` callers. |
| `tests/lighting_tests.cpp` | Migrate `Model::create()` (non-indexed) calls to `create_indexed()`. |
| `tests/scene_rendering_tests.cpp` | Migrate `Model::create()` (non-indexed) calls to `create_indexed()`. |

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | All geometry uses the standard Vertex format (Float3 position + Float3 color, stride 24 for primitives; 72B full Vertex for advanced). |
| A-02 | Primitive helpers use 24-byte stride (position + color only) for simplicity, matching SPEC-009's cube. |
| A-03 | `Model::create_indexed()` does not validate submesh index ranges or material_index bounds at creation. Invalid values produce undefined behavior at draw time (or use fallback). |
| A-04 | The fallback material is created once and lives for the lifetime of RenderDevice. |
| A-05 | All existing apps are migrated in the same implementation pass. No deprecated API stubs are kept. |
| A-06 | CMakeLists.txt in `src/cmd/` uses `GLOB_RECURSE` — new `.cpp` files in `apps/` are picked up automatically. |

## Out of scope

(Listed in the Non-goals section above, plus:)
- Primitive helpers for sphere, cylinder, torus, etc.
- Model loading from files (glTF, OBJ, FBX).
- Submesh-based culling or visibility.
- LOD, skeletal animation.
- Material deduplication optimization.
- `MeshRenderer` changes.
- Changes to `error.h` or `RenderDevice::draw()` signature.
- Performance optimization (e.g., sorting submeshes by material).
