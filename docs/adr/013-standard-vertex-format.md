# ADR-013: Standard Vertex Format — Single 72-Byte Struct for All Meshes

## Status

`Accepted`

## Context

Before the Phong lighting system (SPEC-018), the Buddd Engine had no standard vertex format. Each mesh and demo defined its own ad-hoc vertex structure:

- `CubeVertex` in `cube_demo.cpp` — position (Float3) + colour (Float3), 24 bytes
- `LitCubeVertex` (proposed but never implemented) — position + colour + normal
- `setup_triangle()` in `demo_helpers.cpp` — anonymous struct with `float x, y, z, r, g, b`

This ad-hoc approach had several problems:

1. **Incompatible vertex buffers between demos**: A triangle vertex buffer could not be passed to a cube shader, and vice versa, because the vertex layouts differed. There was no single vertex format that all shaders could agree on.

2. **No standard attribute locations**: Each shader declared attributes at arbitrary locations. There was no convention for which attribute location corresponded to position, colour, normal, or texture coordinates.

3. **No extensibility for future attributes**: Adding a new attribute (e.g., tangent for normal mapping, second UV set for lightmaps) would require changing every existing vertex struct and every existing shader.

4. **Fragmented shader interface design**: Shader authors could not rely on a standard vertex layout — they had to match whatever ad-hoc struct the application code happened to define.

5. **Two vertex format descriptors per mesh**: The vertex data was described once by its C++ struct layout and again by a `VertexFormat` descriptor for the GPU. These could easily drift out of sync.

The SPEC-018 design review (2026-05-31) produced a clear requirement: a single standard `Vertex` struct used by ALL meshes, with six pre-assigned attribute locations covering current and near-future GPU inputs.

## Decision

We adopt a single standard `Vertex` struct (72 bytes, 6 attributes) defined in `src/engine/render/vertex.h`, used by ALL meshes in the engine:

```cpp
struct Vertex {
    math::Vec3 position;     // offset 0   (12B)  location 0
    math::Vec4 color;        // offset 12  (16B)  location 1  (1,1,1,1 = white default)
    math::Vec3 normal;       // offset 28  (12B)  location 2
    math::Vec2 texcoord;     // offset 40  (8B)   location 3
    math::Vec4 tangent;      // offset 48  (16B)  location 4  (reserved for normal mapping)
    math::Vec2 texcoord2;    // offset 64  (8B)   location 5  (reserved for lightmaps)
};
static_assert(sizeof(Vertex) == 72, "Vertex must be 72 bytes");
```

A companion `k_standard_vertex_format` constant describes the full 6-attribute layout for vertex buffer creation.

### Fixed attribute locations

| Location | Attribute | Type | Use |
|----------|-----------|------|-----|
| 0 | `position` | `Vec3` | Vertex position in local space (all shaders) |
| 1 | `color` | `Vec4` | Per-vertex colour (unlit shaders); white default |
| 2 | `normal` | `Vec3` | Surface normal (lit shaders) |
| 3 | `texcoord` | `Vec2` | Texture coordinates (textured shaders) |
| 4 | `tangent` | `Vec4` | Reserved for future normal mapping |
| 5 | `texcoord2` | `Vec2` | Reserved for future lightmaps |

### How unused fields are handled

Fields not used by a particular shader are left zero-initialised. The GPU safely ignores unbound attribute locations — if a shader only binds locations 0 and 1 (unlit case), locations 2–5 are never read, so their zero values have no effect.

### How existing demos were updated

- During the Phong lighting implementation (SPEC-018), `setup_triangle()` and `setup_cube()` in `demo_helpers.cpp` were updated to fill `Vertex` structs with position and colour, leaving normal/texcoord/tangent/texcoord2 as zero. The vertex format descriptor used only locations 0 and 1.
- The Phong demo fills position + normal + texcoord, leaves colour as white default.
- During the multi-material Model redesign (SPEC-020 / ADR-017), `setup_triangle()` and `setup_cube()` were removed and replaced by engine-level primitive helpers (`engine::create_cube()`, `engine::create_triangle()`, `engine::create_quad()` in `src/engine/render/primitives.h/.cpp`). These helpers use a custom 24-byte vertex format (Float3 position + Float3 colour) for simplicity — see the Compliance exception below.
- All unlit demos (triangle, cube, cube-scene, textured-cube, free-camera) continue to work identically because the GPU ignores unbound attribute locations.

## Alternatives considered

### Keep ad-hoc vertex structs (status quo ante)

- **Pros**: No change to existing code. Each mesh can optimise its vertex size (e.g., 24 bytes for position+colour instead of 72 bytes).
- **Cons**: No standardisation. Each new shader must be carefully paired with its vertex struct. Adding a future attribute (normal, tangent, texcoord) requires changing every existing struct and shader. The attribute location assignment is arbitrary and inconsistent.
- **Verdict**: Rejected. The lack of standardisation creates growing friction with each new feature.

### Multiple standard vertex formats (e.g., `VertexP`, `VertexPC`, `VertexPCN`)

- **Pros**: Memory-efficient — each mesh uses the smallest possible vertex size. Common patterns are named and documented.
- **Cons**: Multiplies the number of vertex format descriptors. Shaders must be written to match specific formats, reducing composability. Adding a new format requires updating all tooling. The engine would need runtime format detection or explicit format declarations per mesh.
- **Verdict**: Rejected. Over-engineering for the current scale. The 72B struct is only ~48B larger than the minimal position+colour format — a memory cost of ~576 bytes for a 12-vertex triangle or ~1.7 KB for a 36-indexed cube, which is negligible on modern GPUs.

### Single struct but with `Vec3` colour (24-byte colour field)

- **Pros**: Saves 4 bytes per vertex. Matches the old ad-hoc struct layout.
- **Cons**: Incompatible with `Vec4`-based shader interfaces. Requires padding or explicit `vec4` conversion in shaders. Aligned Vec4 at offset 12 is more GPU-friendly (avoids misaligned reads on some hardware).
- **Verdict**: Rejected. The Vec4 colour with `.a` unused is simpler for shader compatibility (directly maps to `vec4` in GLSL) and provides hardware-friendly alignment.

### Dynamic vertex format with run-time reflection

- **Pros**: Maximum flexibility — shaders declare their inputs, and the engine matches them automatically.
- **Cons**: Requires a full vertex format reflection system (VK_EXT_vertex_attribute_divisor-like or ASTC-like). Adds significant complexity for marginal benefit at the current scale.
- **Verdict**: Rejected. Way beyond the project's scope. The 6-attribute fixed format covers all current and foreseeable inputs.

## Consequences

### Positive

- **Single source of truth**: One `Vertex` struct, one `k_standard_vertex_format` constant. No more ad-hoc structs or format descriptors to maintain across files.
- **Shader portability**: Any shader that uses standard attribute locations can render any mesh. A mesh created for the Phong demo can be rendered with an unlit shader (ignoring normals/texcoords). A mesh created with only position+colour can be rendered with a lit shader (normals will be zero-vectors, producing flat ambient-only rendering — a valid degraded state).
- **Future-proof attribute reservations**: Location 4 (tangent) and location 5 (texcoord2) are reserved for upcoming features (normal mapping, lightmaps). Adding these features requires no vertex format changes — just new shader code.
- **Simplified code review**: A new mesh or shader no longer requires checking whether vertex formats match. The standard format is documented, understood, and enforced by convention.
- **No runtime cost for unused attributes**: The GPU does not read attribute data from unbound locations, so the extra bytes for `normal`, `texcoord`, `tangent`, and `texcoord2` cost only memory bandwidth for vertex buffer upload, not per-frame rendering bandwidth.

### Negative

- **Vertex buffer memory waste**: Every mesh allocates 72 bytes per vertex even if it only uses 28 bytes (position+colour). For a triangle (12 vertices), this wastes ~528 bytes. For a cube (24 vertices), ~1 KB. For complex meshes (10k+ vertices), ~440 KB. This is negligible on modern GPUs with hundreds of MB to GB of VRAM.
- **Vertex upload time increase**: Larger vertex buffers take slightly longer to upload to the GPU. The impact is proportional to the vertex count and negligible for the project's current scale.
- **Unlit shader compatibility quirk**: The existing unlit shaders read `a_color` as `vec3`, but the `Vertex` struct stores `color` as `Vec4` (16 bytes). This is a type mismatch — the GPU reads 16 bytes of colour data while the shader expects 12 bytes. In practice this works because OpenGL reads contiguous vertex attributes by location, and the extra byte is simply not consumed by the `vec3` input. However, this is technically undefined behaviour in the OpenGL specification. Mitigation: the spec documents this incompatibility and all existing shaders work correctly in practice across all tested GPUs.
- **Misaligned reads on some GPU architectures**: `Vec3` fields (position at offset 0, normal at offset 28) are not 16-byte aligned. On some GPU hardware (e.g., AMD GCN/RDNA), this may cause a performance penalty. Mitigation: this is the standard layout used by the vast majority of game engines for cross-platform vertex formats, and the alignment penalty is small.
- **Breaking change for any external code using ad-hoc vertex structs**: All existing mesh creation code must be updated to use `Vertex`. The engine currently has no external consumers; all meshes are in `src/cmd/demo/` or tests, and all were updated as part of the Phong lighting implementation.

### Compliance

- All new mesh creation code SHALL use `buddd::engine::Vertex` and `k_standard_vertex_format`, except where the explicit exception below applies.
- All new shaders SHALL use the standard attribute locations documented above.
- Code review SHALL flag any new ad-hoc vertex structs or custom vertex formats, except where the explicit exception below applies.

#### Exception: Primitive helpers and low-level API usage

The standard `Vertex` format is **required** for:

- Models loaded via the AssetManager (glTF, OBJ, etc.) — these must use the standard 72-byte format for interoperability with all engine shaders.
- Meshes rendered with standard engine shaders (unlit, Phong, textured).
- Any code path that passes through the engine's high-level rendering pipeline where interoperation with multiple shaders is expected.

Simpler custom vertex formats are **permitted** for:

- Primitive helpers in `src/engine/render/primitives.h/.cpp` (`engine::create_cube()`, `engine::create_triangle()`, `engine::create_quad()`), which use a 24-byte Float3 position + Float3 colour format. The simpler format avoids padding 48 unused bytes per vertex for geometry that has no need for normals, texcoords, or tangents.
- Any code that uses the engine's low-level rendering APIs directly (i.e., bypassing the AssetManager and standard shader pipeline) and provides its own vertex format descriptor and shaders.

**Rationale**: The standard Vertex format exists to guarantee interoperability between the AssetManager, standard shaders, and engine-managed content. Primitive helpers and low-level API callers are self-contained — they create both the geometry and the shader, so they can agree on whatever vertex format they choose without creating interoperability problems. The distinction is between the **engine-managed path** (standard Vertex required) and the **custom path** (custom formats allowed).

## Related documents

- SPEC-018 (`.specs/sprint-2026-05/lighting/spec.md`): The Phong lighting specification that introduced the standard Vertex struct.
- IMPL-018-002 (`.specs/sprint-2026-05/lighting/implementation-contract.md`): Implementation contract for the standard Vertex struct (lines 138–182).
- `src/engine/render/vertex.h`: Canonical implementation of the Vertex struct and `k_standard_vertex_format`.
- `src/engine/render/vertex_format.h`: The `VertexFormat`, `VertexAttribute`, and `VertexAttributeType` definitions that `k_standard_vertex_format` uses.
- ADR-003 (`docs/adr/003-render-pipeline-architecture.md`): Render pipeline architecture — establishes the interface-backend pattern within which `Vertex` lives.
- ADR-002 (`docs/adr/002-glm-wrapper-math.md`): GLM wrapper pattern — `Vertex` uses `math::Vec2`, `math::Vec3`, `math::Vec4` wrapper types, ensuring no GLM types leak beyond `src/engine/math/`.
