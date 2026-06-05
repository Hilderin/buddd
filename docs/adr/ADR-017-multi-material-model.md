# ADR-017: Multi-Material Model Architecture — SubMesh Indexed Design, Primitive Helpers, and API Cleanup

## Status

`Accepted`

## Context

The `Model` class was originally designed for single-material geometry (SPEC-009 / ADR-013). It stored one `shared_ptr<Material>` and supported both indexed and non-indexed geometry via two factory methods (`create()` and `create_indexed()`). This design had several growing problems:

### 1. Single material only
`Model` stored exactly one `shared_ptr<Material>`. Multi-material models (common in glTF, game assets, CAD) required splitting geometry into multiple `Model` instances — losing the natural grouping of submeshes that share the same vertex buffer but use different materials.

### 2. Dual-path geometry (indexed vs non-indexed)
Two factory methods (`create()` for non-indexed, `create_indexed()` for indexed) forced every internal method (`draw()`, `material()`) to branch on whether indices exist. The `has_indices()` accessor existed purely to let callers check which path applied. This doubled the testing surface and made the API harder to reason about.

### 3. Ambiguous `material()` accessor
`Model::material()` returned `Result<Material&>` — a convenience for "the first submesh's material". This encouraged callers to assume there was always a single material, making multi-material adoption harder. It also returned a raw reference (`Material&`), which ADR-010 had already flagged as problematic for ownership semantics.

### 4. Two-phase construction (proposed but not yet implemented)
The original SPEC-020 draft proposed an additive `add_submesh()` / `set_material()` API. This would have introduced mutation after creation, making the Model stateful, complicating thread safety, and preventing const-correct draw paths.

### 5. No reusable primitive helpers
Every app that needed a cube or triangle created geometry data inline. `demo::setup_cube()` existed but lived in `src/cmd/demo/` — inaccessible from engine code or tests, and it also created materials internally (embedding shader source), coupling geometry to shading.

### 6. Demo helpers created materials internally
`demo::setup_cube()` and `demo::setup_triangle()` created materials with embedded shader source, giving apps no control over shaders, uniforms, or material type. This made it impossible to use a custom shader with a primitive.

### 7. No defensive fallback for missing materials
When a material pointer was null or out of bounds, there was no defined behaviour — the system would likely crash or render garbage. No mechanism existed for graceful degradation.

## Decision

### Decision 1: SubMesh with material_index (not material pointer)

`SubMesh` is a plain aggregate with an integer material index:

```cpp
struct SubMesh {
    uint32_t index_start;
    uint32_t index_count;
    uint32_t material_index;  // index into Model's materials() vector
};
```

Materials are stored as a flat `vector<shared_ptr<Material>>` on `Model`. Multiple submeshes can share the same `material_index`. This design:

- Enables material sharing across submeshes (index-based, not pointer-based).
- Makes serialization straightforward (material indices are stable integers, not pointers).
- Decouples submesh definitions from material ownership — the `Model` owns the material list, submeshes just reference it by index.

### Decision 2: Unified single factory, immutable after creation

`Model::create_indexed()` takes everything upfront:

```cpp
static auto create_indexed(
    RenderDevice& device,
    const VertexFormat& vertex_format,
    std::span<const std::byte> vertex_data,
    std::span<const std::byte> index_data,
    IndexType index_type,
    std::vector<SubMesh> submeshes,
    std::vector<std::shared_ptr<Material>> materials,
    PrimitiveTopology topology = PrimitiveTopology::Triangles
) -> Result<Model>;
```

No `add_submesh()`, `set_material()`, or any other mutation after creation. The Model is fully specified at construction time. This gives:

- **Const-correctness**: `draw() const` is truly const — the Model does not mutate during rendering.
- **Thread safety**: Immutable after construction; safe to read from multiple threads.
- **Simpler correctness**: No invariants to maintain across additive operations (e.g., "must set material before adding submesh").

### Decision 3: No non-indexed support

`Model::create()` (non-indexed) is removed. `has_indices()` is removed. **All Models have index buffers.**

Rationale:
- Indexed geometry is the universal standard in 3D rendering — glTF, OBJ, FBX, and all GPU APIs (OpenGL, Vulkan, DirectX) expect indexed draw calls.
- The non-indexed path duplicated every internal method (`draw()` had two control-flow branches) and doubled the test matrix.
- The memory cost of a minimal index buffer (e.g., 3 `uint16_t` indices for a triangle = 6 bytes) is negligible.
- Removing the dual path simplifies the entire class: no branches in `draw()`, no `has_indices()` accessor, no factory overloads.

### Decision 4: No `material()` accessor

`Model::material()` is removed entirely. Callers use `model.materials()[i]` to access materials by index.

This:
- Eliminates the ambiguity of "which material" — there is no single material, only a list.
- Removes the `Result<Material&>` return type (a pattern that ADR-010 discouraged).
- Forces callers to be explicit about which material index they want.
- Works uniformly for both single-material and multi-material models.

### Decision 5: Primitive helpers are geometry-only factories

`engine::create_cube()`, `engine::create_triangle()`, `engine::create_quad()` are free functions in `src/engine/render/primitives.h/.cpp`. They:

- **Create geometry only** — vertex and index data with standard format (Float3 position + Float3 colour, 24-byte stride).
- **Do NOT create materials** — the caller provides a `shared_ptr<Material>` as a parameter.
- Wrap the geometry in a `Model` with one SubMesh covering all indices and the caller's material.
- Are usable from engine code, tests, and apps alike (unlike `demo::setup_cube()` which lived in `src/cmd/`).

Design rationale: Separation of concerns — geometry generation (vertex positions, index lists) is independent of shading (shaders, uniforms, material type). The caller chooses the material, giving full control over rendering appearance.

### Decision 6: Fallback material on RenderDevice

`RenderDevice::fallback_material()` returns a `Material&` that renders solid magenta (RGB 1,0,1). It is:

- Lazily created on first call (cached for device lifetime).
- Used automatically by `Model::draw()` when a submesh's `material_index` is out of bounds or the material pointer is null.
- A defensive mechanism — ensures visible output even with broken material references, rather than crashing or producing undefined behaviour.

### Decision 7: Breaking change — full migration in one pass

All existing apps (11 apps) and all test files were migrated to the new API in a single implementation pass. No backward-compatible stubs or deprecation periods were kept. Rationale:

- The engine has no external consumers — all code is in-repo.
- Keeping legacy stubs would double the maintenance surface and never force migration.
- A clean break produces simpler code and lets developers focus on the new patterns immediately.

The old `demo::setup_cube()` and `demo::setup_triangle()` helpers (which lived in `src/cmd/demo/` and created materials internally) were removed entirely. Apps now create their own materials and pass them to the engine-level primitive helpers.

## Alternatives considered

### SubMesh with shared_ptr<Material> instead of material_index

| Alternative | Verdict |
|---|---|
| `SubMesh` stores `shared_ptr<Material>` directly | **Rejected.** Prevents material sharing by index (two submeshes wanting the same material must compare pointers, not indices). Makes serialization harder (need to handle pointer identity). Couples submesh definition to material ownership. The index-based approach is simpler, more flexible, and more aligned with glTF's mesh primitive model. |

### Two-phase construction (add_submesh / set_material)

| Alternative | Verdict |
|---|---|
| Keep `add_submesh()` and `set_material()` from original draft | **Rejected.** Mutation after creation prevents const-correct draw paths, complicates thread safety, and introduces ordering invariants ("must set material before drawing"). The single-factory approach is simpler and safer. |

### Keep non-indexed support

| Alternative | Verdict |
|---|---|
| Keep `Model::create()` (non-indexed) and `has_indices()` | **Rejected.** The dual path complicates every internal method. Indexed geometry is the universal standard — the 6-byte index buffer for a triangle is a trivial cost. The simplification of removing the dual path outweighs the minimal memory cost. |

### Keep material() returning Result<Material&>

| Alternative | Verdict |
|---|---|
| Keep `material()` as a convenience for the first material | **Rejected.** Ambiguous for multi-material models. Returns a raw reference (ADR-010 violation). Encourages bad patterns (assuming single material). The explicit `materials()[i]` is clearer and works for all cases. |

### Primitive helpers create materials internally

| Alternative | Verdict |
|---|---|
| `create_cube()` creates its own default material with embedded shaders (like old `demo::setup_cube()`) | **Rejected.** Couples geometry generation to shading decisions. Apps could not control shaders or uniforms. The chosen approach — geometry-only factories with caller-provided materials — is a clean separation of concerns. |

### Fallback as optional / no fallback

| Alternative | Verdict |
|---|---|
| Crash or undefined behaviour on null/out-of-bounds material | **Rejected.** Defensive programming principle: a rendering engine should produce visible output even with broken asset references. The magenta fallback makes debugging easier (you see purple immediately) and prevents crashes. |
| Return `std::optional<Material&>` from fallback | **Rejected.** The fallback must always succeed — it is a last-resort defensive mechanism. A `Material&` is the correct contract. |

### Gradual migration with deprecated stubs

| Alternative | Verdict |
|---|---|
| Mark old API as deprecated, keep both paths for 2–3 releases | **Rejected.** No external consumers exist. Keeping dead code paths increases maintenance burden with zero benefit. A clean break is appropriate for an in-development engine with a single internal codebase. |

## Consequences

### Positive

- **Multi-material support**: Models can now represent arbitrary submesh-material mappings, enabling glTF import and complex game assets.
- **Simpler Model internals**: No dual-path branching in `draw()`, no `has_indices()` maintenance, no `material()` ambiguity.
- **Immutable-by-design**: Whole-model specification at construction time eliminates mutation-related bugs and enables const-correct rendering.
- **Separated concerns**: Primitives handle geometry; apps handle materials. No embedded shader source in geometry helpers.
- **Defensive rendering**: Fallback material ensures graceful degradation on null/out-of-bounds material references.
- **Cleaner public API**: `Model` now has exactly one factory, six accessors, and one draw method. Easy to learn and reason about.
- **Index-based material sharing**: Multiple submeshes can reference the same material by index without pointer-identity complexity.
- **Testability**: The unified factory is easier to test — one creation path, no mutation edge cases.

### Negative

- **Breaking change**: All existing Model usage had to be migrated. 11 apps, 3 test files, and `render_system.cpp` were updated. No backward compatibility period.
- **All geometry requires index buffers**: Even a simple triangle needs 3 indices (6 bytes). Minimal overhead, but it is a hard requirement — no degenerate non-indexed path exists.
- **No runtime material swapping**: Materials cannot be changed after Model creation. If dynamic material changes are needed, a new Model must be created (or the caller manages materials separately).
- **Primitive helpers use 24-byte stride only**: `create_cube()` etc. use Float3 position + Float3 colour format. Callers needing normals, texcoords, or other attributes must create geometry manually or extend the helpers.
- **Fallback material is engine-singleton**: `RenderDevice::fallback_material()` returns one shared magenta material for all fallback cases. No per-model or per-scene fallback customization.

### Compliance

- All new Model creation code SHALL use `Model::create_indexed()` with `vector<SubMesh>` and `vector<shared_ptr<Material>>`.
- `Model::material()` and `Model::has_indices()` SHALL NOT be re-added. Any PR attempting to add them SHALL be rejected.
- Non-indexed geometry SHALL NOT be supported. If non-indexed rendering is needed, the caller must generate index data.
- Primitive helpers SHALL remain geometry-only (no embedded material creation).
- The fallback material SHALL remain magenta for immediate visual debugging.

## Related documents

- SPEC-020 (`.specs/sprint-2026-06/model-multi-material/spec.md`): Full specification of the multi-material Model redesign.
- SPEC-009 (`.specs/sprint-2026-05/3d-cube-demo/spec.md`): The original Model design that this ADR supersedes/redesigns.
- ADR-013 (`docs/adr/013-standard-vertex-format.md`): Standard Vertex struct used by primitives and all meshes.
- ADR-010 (`docs/adr/010-no-raw-pointers-in-public-api.md`): Impact reduced by removal of `material()` returning `Result<Material&>`.
- ADR-003 (`docs/adr/003-render-pipeline-architecture.md`): Render pipeline architecture — `Model::draw()` uses `RenderDevice::draw_indexed()` which returns void per ADR-003 Decision 1.
