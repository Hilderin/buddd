# IMPL-009 — Model Utility & 3D Cube Demo

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume |
| Date | 2026-05-30 |
| Time | ~22:00 UTC |

## Source spec

`.specs/sprint-2026-05/3d-cube-demo/spec.md` (SPEC-009), accepted. All open questions (Q-01 through Q-05) are marked `[RESOLVED]`. All blocking issues (B-01 through B-05) from the spec-critic review have been resolved in the accepted spec.

## Goal

Implement the `Model` utility class (namespace `buddd::engine`) that encapsulates a `VertexBuffer`, optional `IndexBuffer`, and shared `Material` (`std::shared_ptr<Material>`) with static factory methods returning `Result<Model>` that take a pre-created `std::shared_ptr<Material>`. Add `CubeResources` struct and `setup_cube()` to `demo_helpers.h/cpp` for reusable cube setup (24 vertices with per-vertex position+colour, 36 indices, single `u_mvp` uniform). Implement the cube demo (`run_cube_demo` in new `cube_demo.h/cpp`) that renders a rotating per-face-coloured unit cube for 120 frames at ~60 FPS via a single `draw_indexed` call per frame. Register the cube demo in the existing demo dispatch chain.

## Non-goals

- No shader compilation or material linking inside `Model` factories — `Model` takes a pre-created `std::shared_ptr<Material>`.
- No `known_uniforms` parameter — the caller creates the material with the desired uniforms before passing it to `Model`.
- No per-face uniform changes (`u_color`) — face colours are encoded per vertex (Float3 color attribute).
- No `Model::create_cube()` static method — cube vertex/index data is defined in demo helpers.
- No textured cube — solid-colour per-face only.
- No lighting, shading, or normal vectors.
- No model file loading (.obj, .gltf).
- No animation or transform hierarchy beyond a single Y-axis rotation.
- No user interaction — camera fixed, rotation hardcoded.
- No multi-material support — `Model` has a single shared `Material`.
- No dynamic buffer updates after creation.
- No instanced rendering.
- No wireframe or debug rendering modes.
- No `Scene` or `Entity` integration — the cube stands alone.
- No changes to `src/engine/CMakeLists.txt` or `src/cmd/CMakeLists.txt` (GLOB_RECURSE handles new files).
- No modifications to `src/engine/math/`, `src/engine/render/` existing headers, or other existing engine core types beyond adding `model.h`/`model.cpp`.
- No new dependencies beyond what the project already uses.

## Relevant constitution rules

- **CONST-001-architecture-boundaries.md**: `Model` lives in `src/engine/render/` (inside engine boundary). `cube_demo.h` must expose no backend types (no OpenGL, SDL3, or GLM in the public header). Only `cube_demo.cpp` may include backend headers.
- **CONST-002-testing-policy.md**: All testable code must have corresponding unit tests. The `tests/model_tests.cpp` file must cover all acceptance criteria with Catch2 v3, running on the headless backend.

## Relevant ADRs

- **ADR-001** (`docs/adr/001-result-error-pattern.md`): Factory methods (`create`, `create_indexed`) return `Result<Model>`. `draw()` returns `void` as a documented exception consistent with ADR-003.
- **ADR-003** (`docs/adr/003-render-pipeline-architecture.md`): Draw methods return `void` (not `Result<void>`) because they are on a performance-sensitive hot path; precondition violations are undefined behaviour. `Model::draw()` follows the same convention since it delegates to `RenderDevice::draw()` / `RenderDevice::draw_indexed()`.
- **ADR-004** (`docs/adr/004-demo-system-architecture.md`): Demos are registered in `demo_command.cpp` with usage text and if/else dispatch.

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/render/vertex_buffer.h` | Understand `VertexBuffer` abstract interface. |
| `src/engine/render/index_buffer.h` | Understand `IndexBuffer` abstract interface and `IndexType` enum. |
| `src/engine/render/material.h` | Understand `Material` abstract interface (`set_uniform` overloads, `has_uniform`). |
| `src/engine/render/render_device.h` | Understand factory methods (`create_shader`, `create_material`, `create_vertex_buffer`, `create_index_buffer`) and draw methods (`draw`, `draw_indexed`) signatures. |
| `src/engine/render/vertex_format.h` | Understand `VertexFormat` and `VertexAttribute` struct layout, `VertexAttributeType` enum. |
| `src/engine/render/primitive_topology.h` | Understand `PrimitiveTopology` enum values. |
| `src/engine/error.h` | Understand `Result<T>`, `Error`, `Error::Category`, `make_error()`, `to_string()`. |
| `src/engine/render/shader.h` | Understand `ShaderType` enum (`Vertex`, `Fragment`). |
| `src/engine/render/render_device_headless.h` | Understand headless backend for test fixture. |
| `src/engine/math/camera.h` | Understand `Camera` API (`look_at`, `set_perspective`, `view_matrix`, `projection_matrix`). |
| `src/engine/math/mat4.h` | Understand `Mat4` static factories (`identity`, `rotate`). |
| `src/engine/math/vec3.h` | Understand `Vec3::unit_y()` for camera up vector and rotation axis. |
| `src/engine/math/math.h` | Convenience include for `math::radians()`. |
| `src/cmd/demo/demo_helpers.h` | Reference for `setup_triangle` pattern (error handling, include style). Must be read to add `CubeResources` and `setup_cube`. |
| `src/cmd/demo/demo_helpers.cpp` | Reference for shader source string convention, vertex buffer creation, and fatal error handling. Must be read to add `setup_cube` implementation. |
| `src/cmd/demo/triangle_demo.h` | Reference for demo header pattern (forward declarations, `run_triangle_demo` signature). |
| `src/cmd/demo/triangle_demo.cpp` | Reference for render loop pattern (frame-limited loop, `poll_events`, `begin_frame`/`end_frame`). |
| `src/cmd/commands/demo_command.cpp` | The file to modify for demo registration. Read existing `k_demo_usage` and dispatch chain. |
| `tests/CMakeLists.txt` | Must be read to add `model_tests.cpp` source file in both branches. |
| `tests/math_test.cpp` | Style reference for Catch2 v3 test format. |

## Files allowed to change

### New files to create (5 files)

All paths are relative to the repository root.

| # | File | Purpose |
|---|---|---|
| 1 | `src/engine/render/model.h` | `Model` class header — public API, private data members (`std::shared_ptr<Material>`), factory methods, draw method, accessors. |
| 2 | `src/engine/render/model.cpp` | `Model` factory method implementations and draw dispatch. |
| 3 | `src/cmd/demo/cube_demo.h` | `run_cube_demo` declaration only in `buddd::cmd::demo`. |
| 4 | `src/cmd/demo/cube_demo.cpp` | `run_cube_demo` implementation — camera, render loop, single indexed draw call. |
| 5 | `tests/model_tests.cpp` | Catch2 v3 tests covering `Model` API and cube data verification. |

### Files to modify (4 files)

| # | File | Change |
|---|---|---|
| 1 | `src/cmd/demo/demo_helpers.h` | Add `#include "render/model.h"`, declare `CubeResources` struct and `setup_cube` function. |
| 2 | `src/cmd/demo/demo_helpers.cpp` | Add `#include "render/model.h"`, implement `setup_cube` with shader sources, vertex/index data, material creation, and `Model::create_indexed`. |
| 3 | `src/cmd/commands/demo_command.cpp` | Add `#include "demo/cube_demo.h"`, add `cube` to `k_demo_usage`, add `"cube"` to validation condition, add dispatch branch. |
| 4 | `tests/CMakeLists.txt` | Add `model_tests.cpp` to the source file list in **both** the `if(BUDDD_HAS_DISPLAY)` and `else()` branches, after `scene_graph_tests.cpp`. |

## Files forbidden to change

- Any file outside the explicitly listed "Files allowed to change" set.
- `src/engine/CMakeLists.txt` — GLOB_RECURSE will auto-discover new engine files.
- `src/cmd/CMakeLists.txt` — GLOB_RECURSE will auto-discover new demo files.
- `src/engine/math/` (any file) — math types are stable.
- `src/engine/render/` (any existing file) — existing render abstractions are stable. Only new `model.h`/`model.cpp` are created.
- `src/engine/error.h`, `src/engine/version.h`, `src/engine/version.cpp`.
- `src/cmd/demo/triangle_demo.h`, `src/cmd/demo/triangle_demo.cpp`.
- `docs/adr/` (any file).
- `docs/constitution/` (any file).
- Root `CMakeLists.txt`, `CMakePresets.json`, `.clang-format`, `.vscode/`, `AGENTS.md`, `opencode.json`.

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Namespace | `buddd::engine` for `Model`. `buddd::cmd::demo` for demo functions. |
| File naming | `snake_case` — `model.h`, `cube_demo.h`. |
| Class naming | PascalCase — `Model`. |
| Struct naming | PascalCase — `CubeResources`. |
| Header guards | `#pragma once` only (no `#ifndef` guards). |
| Function style | Trailing return type syntax (`auto foo() -> int`). |
| Formatting | `.clang-format` at repo root: LLVM style, 4-space indent, 100 column limit. |
| Local includes in engine | Use `"render/model.h"` — quoted paths relative to `src/engine/`, resolved via PUBLIC include directory. |
| Local includes in cmd | Use `"demo/cube_demo.h"` — quoted paths relative to `src/cmd/`, resolved via PRIVATE include directory. `demo_helpers.cpp` uses `"demo_helpers.h"` (same-directory) per existing convention. |
| Include order | 1. Standard library headers (`<angle brackets>`), 2. Engine/demo headers (`"quotes"`). Empty line between groups. |
| `noexcept` | Non-allocating accessors and `draw()` are `noexcept`. Factory methods (`create`, `create_indexed`) are NOT `noexcept` (they allocate/fail). |
| Move semantics | `Model` is movable but not copyable. Moved-from model is null (draw is no-op). |
| RAII for resource cleanup | `Model` stores GPU resources via `std::unique_ptr<VertexBuffer>`, `std::unique_ptr<IndexBuffer>` (optional), `std::shared_ptr<Material>`. Default destructor cleans up in reverse declaration order. |
| Demo error handling | `setup_cube` on failure: `std::fprintf(stderr, "FATAL: ...")` + `std::exit(EXIT_FAILURE)` — identical pattern to `setup_triangle`. |
| Demo start/end messages | Printed to `std::cerr` — `"Demo started: cube (120 frames)\n"` and `"Demo complete: cube (120 frames rendered)\n"`. Abort message prints frame count. |
| Test style | Catch2 v3, `TEST_CASE` with `[tag]` annotations, `REQUIRE`/`CHECK` macros. |
| `[[nodiscard]]` | `run_cube_demo` is marked `[[nodiscard]]` (like `run_triangle_demo`). |
| `std::shared_ptr<Material>` | Convert from `std::unique_ptr<Material>` via `std::shared_ptr<Material>(std::move(unique_ptr))`. |

## Required implementation behavior

### 0. `tests/CMakeLists.txt` — Add Model test file

Insert `model_tests.cpp` after `scene_graph_tests.cpp` in **both** the `if(BUDDD_HAS_DISPLAY)` and `else()` branches:

```cmake
add_executable(buddd_tests
    version_test.cpp
    platform_abstraction_test.cpp
    sdl3_backend_test.cpp       # only in display branch
    math_test.cpp
    scene_graph_tests.cpp
    model_tests.cpp             # <-- new
)
```

No changes to `target_link_libraries` or `catch_discover_tests`. The model test needs only `buddd_engine` and `Catch2::Catch2WithMain` (already linked). Tests do NOT link against demo code; cube data tests recreate the cube setup inline.

**IMPORTANT:** When editing, ensure the closing `)` of `add_executable(...)` is preserved. The `model_tests.cpp` line is inserted *before* the closing parenthesis, not replacing it.

---

### 1. `src/engine/render/model.h` — Model class header

```cpp
#pragma once

#include "error.h"
#include "render/index_buffer.h"
#include "render/material.h"
#include "render/primitive_topology.h"
#include "render/vertex_buffer.h"
#include "render/vertex_format.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace buddd::engine {

// Forward declaration — RenderDevice& appears in factory method parameters.
class RenderDevice;

class Model {
public:
    // -- Null model (draw is no-op) --
    Model() noexcept = default;

    // -- Factory methods --
    /// Creates a non-indexed Model.
    /// On failure, returns an Error describing the failure.
    /// The material is shared (not owned exclusively) via shared_ptr.
    static auto create(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::shared_ptr<Material> material,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;

    /// Creates an indexed Model.
    static auto create_indexed(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::span<const std::byte> index_data,
        IndexType index_type,
        std::shared_ptr<Material> material,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;

    // -- Drawing --
    /// Issues one draw call (indexed or non-indexed depending on
    /// whether an index buffer was provided at creation).
    /// No-op on a moved-from (null) model.
    /// Behaviour is undefined if called outside begin_frame()/end_frame().
    auto draw(RenderDevice& device) const -> void;

    // -- Accessors --
    auto material() noexcept -> Material&;
    auto material() const noexcept -> const Material&;
    auto vertices() const noexcept -> const VertexBuffer&;
    auto indices() const noexcept -> const IndexBuffer&;
    auto has_indices() const noexcept -> bool;
    auto vertex_count() const noexcept -> uint32_t;
    auto index_count() const noexcept -> uint32_t;

    // -- Lifecycle --
    Model(const Model&) = delete;
    auto operator=(const Model&) -> Model& = delete;
    Model(Model&&) noexcept = default;
    auto operator=(Model&&) noexcept -> Model& = default;
    ~Model() = default;

private:
    Model(
        std::unique_ptr<VertexBuffer> vb,
        std::unique_ptr<IndexBuffer> ib,
        std::shared_ptr<Material> material,
        PrimitiveTopology topology,
        uint32_t vertex_count,
        uint32_t index_count
    ) noexcept;

    std::unique_ptr<VertexBuffer> vb_;
    std::unique_ptr<IndexBuffer> ib_;
    std::shared_ptr<Material> material_;
    PrimitiveTopology topology_{PrimitiveTopology::Triangles};
    uint32_t vertex_count_{0};
    uint32_t index_count_{0};
};

static_assert(!std::is_copy_constructible_v<Model>,
    "Model must be non-copyable");
static_assert(std::is_move_constructible_v<Model>,
    "Model must be movable");

} // namespace buddd::engine
```

**Requirements:**
- `Model` stores `std::shared_ptr<Material> material_` (shared ownership), NOT `std::unique_ptr<Material>`.
- Factory methods take `std::shared_ptr<Material>` — no shader source parameters, no `known_uniforms`.
- Default constructor creates a null model: all pointers are `nullptr`, `vertex_count_ = 0`, `index_count_ = 0`.
- The private constructor takes ownership by moving arguments.
- `Model(const Model&) = delete` and `operator=(const Model&) = delete` — non-copyable.
- `Model(Model&&) noexcept = default` and `operator=(Model&&) noexcept = default` — movable. After move, the source has all pointers set to `nullptr`.
- `~Model() = default` — the default destructor destroys each pointer in reverse declaration order: `material_` first, then `ib_`, then `vb_`.
- Both `static_assert` checks at file scope verify non-copyable and movable.
- Forward-declare `RenderDevice` inside `namespace buddd::engine` (not `#include "render/render_device.h"`).

---

### 2. `src/engine/render/model.cpp` — Model implementation

**Includes:**
```cpp
#include "render/model.h"
#include "render/render_device.h"
```

**Private constructor:**
```cpp
Model::Model(
    std::unique_ptr<VertexBuffer> vb,
    std::unique_ptr<IndexBuffer> ib,
    std::shared_ptr<Material> material,
    PrimitiveTopology topology,
    uint32_t vertex_count,
    uint32_t index_count
) noexcept
    : vb_(std::move(vb))
    , ib_(std::move(ib))
    , material_(std::move(material))
    , topology_(topology)
    , vertex_count_(vertex_count)
    , index_count_(index_count)
{}
```

**`Model::create()` factory:**

Step-by-step:
1. **Validate arguments:**
   - If `vertex_data.size() == 0`: return `make_error(Error::Category::InvalidArgument, "Vertex data is empty")`.
   - If `vertex_format.stride == 0`: return `make_error(Error::Category::InvalidArgument, "Vertex format stride must be positive")`.
   - If `vertex_format.attributes.empty()`: return `make_error(Error::Category::InvalidArgument, "Vertex format must have at least one attribute")`.

2. **Create vertex buffer:**
   - `auto vb = device.create_vertex_buffer(vertex_format, vertex_data);`
   - If `!vb`: return `std::unexpected(vb.error())` — the material `shared_ptr` is released (no leak) when it goes out of scope.

3. **Compute vertex count:**
   - `auto vcount = static_cast<uint32_t>(vertex_data.size() / vertex_format.stride);`

 4. **Construct and return Model:**
    - `return Model(std::move(*vb), nullptr, std::move(material), topology, vcount, 0);`

 5. **Observability (optional):** Log creation success to `std::cerr`:
    ```cpp
    std::cerr << "Model created (" << vcount << " vertices)\n";
    ```
    This is an observability signal from the accepted spec. It is not a testable acceptance criterion — implementers may omit or include it.

**`Model::create_indexed()` factory:**

Step-by-step:
1. **Validate arguments** (same as `create`, plus):
   - If `index_data.size() == 0`: return `make_error(Error::Category::InvalidArgument, "Index data is empty")`.

2. **Create vertex buffer** (same as step 2 in `create`).

3. **Create index buffer:**
   - `auto ib = device.create_index_buffer(index_data, index_type);`
   - If `!ib`: return `std::unexpected(ib.error())` — the vertex buffer unique_ptr and material shared_ptr are cleaned up on scope exit.

4. **Compute counts:**
   - `auto vcount = static_cast<uint32_t>(vertex_data.size() / vertex_format.stride);`
   - `auto icount = static_cast<uint32_t>(index_data.size() / (index_type == IndexType::Uint16 ? sizeof(uint16_t) : sizeof(uint32_t)));`

 5. **Construct and return Model:**
    - `return Model(std::move(*vb), std::move(*ib), std::move(material), topology, vcount, icount);`

 6. **Observability (optional):** Log creation success to `std::cerr`:
    ```cpp
    std::cerr << "Model created (" << vcount << " vertices, " << icount << " indices)\n";
    ```

**`Model::draw()`:**
```cpp
auto Model::draw(RenderDevice& device) const -> void {
    if (!vb_ || !material_) {
        return;  // null/moved-from model: no-op
    }

    if (ib_) {
        device.draw_indexed(topology_, *vb_, *ib_, *material_, index_count_, 0);
    } else {
        device.draw(topology_, *vb_, *material_, vertex_count_, 0);
    }
}
```

**Accessors:**
```cpp
auto Model::material() noexcept -> Material& { return *material_; }
auto Model::material() const noexcept -> const Material& { return *material_; }
auto Model::vertices() const noexcept -> const VertexBuffer& { return *vb_; }
auto Model::indices() const noexcept -> const IndexBuffer& { return *ib_; }
auto Model::has_indices() const noexcept -> bool { return ib_ != nullptr; }
auto Model::vertex_count() const noexcept -> uint32_t { return vertex_count_; }
auto Model::index_count() const noexcept -> uint32_t { return index_count_; }
```

**Requirements:**
- All accessors are `noexcept`.
- `material()`, `vertices()` dereference their respective pointers. Calling these on a null/moved-from model is undefined behaviour.
- `indices()` dereferences `ib_`. Calling `indices()` on a non-indexed model (`ib_ == nullptr`) is undefined behaviour.
- `vertex_count()` and `index_count()` return the stored member values (0 for null/moved-from models, 0 for `index_count()` on non-indexed models). These are safe to call on any model state — they return the stored member without dereferencing any pointer.

---

### 3. `src/cmd/demo/demo_helpers.h` — Add CubeResources + setup_cube

**New includes** (add after the existing `#include "render/vertex_buffer.h"`):
```cpp
#include "render/model.h"
```

**New struct and function declaration** (add inside `namespace buddd::cmd::demo`, after the `setup_triangle` declaration):
```cpp
struct CubeResources {
    std::shared_ptr<buddd::engine::Material> material;
    buddd::engine::Model model;
};

/// Creates a CubeResources for a unit cube (2×2×2, centred at origin):
/// - 24 vertices (Float3 position + Float3 color per vertex, stride 24)
/// - 36 indices (Uint16, 6 per face, CCW winding)
/// - Material with a_position (loc 0), a_color (loc 1), u_mvp (Mat4) uniform
///
/// Face colors are encoded in vertex data:
///   +X (right):  Red    (1,0,0)  -> vertices  0-3
///   -X (left):   Green  (0,1,0)  -> vertices  4-7
///   +Y (top):    Blue   (0,0,1)  -> vertices  8-11
///   -Y (bottom): Yellow (1,1,0)  -> vertices 12-15
///   +Z (front):  Cyan   (0,1,1)  -> vertices 16-19
///   -Z (back):   Magenta(1,0,1)  -> vertices 20-23
///
/// On failure, prints a FATAL error to stderr and calls std::exit(EXIT_FAILURE),
/// consistent with setup_triangle().
auto setup_cube(buddd::engine::RenderDevice& device) -> CubeResources;
```

**Requirements:**
- `CubeResources` is a simple aggregate struct — no custom constructors, no special members.
- `#include "render/model.h"` is required because `CubeResources` contains `buddd::engine::Model` by value.
- The `Material` type is already available via the existing `#include "render/material.h"`.

---

### 4. `src/cmd/demo/demo_helpers.cpp` — Add setup_cube implementation

**New includes** (add at the top, after existing includes):
```cpp
#include "render/model.h"
```

**Implementation** (add at the end of the file, after `setup_triangle`):

```cpp
auto bcd::setup_cube(
    be::RenderDevice& device
) -> CubeResources
{
    // --- Vertex shader ---
    constexpr std::string_view k_cube_vs = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_color;
        out vec3 v_color;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";

    // --- Fragment shader ---
    constexpr std::string_view k_cube_fs = R"(
        #version 450 core
        in vec3 v_color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(v_color, 1.0);
        }
    )";

    // --- Create shaders ---
    auto vs = device.create_shader(be::ShaderType::Vertex, k_cube_vs);
    if (!vs) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(vs.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    auto fs = device.create_shader(be::ShaderType::Fragment, k_cube_fs);
    if (!fs) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(fs.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    // --- Create material ---
    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    if (!mat) {
        std::fprintf(stderr, "FATAL: %s\n", be::to_string(mat.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    // Convert unique_ptr<Material> to shared_ptr<Material>
    std::shared_ptr<be::Material> shared_mat(std::move(*mat));

    // --- Vertex data: 24 vertices, stride 24 (Float3 position + Float3 color) ---
    struct CubeVertex { float px, py, pz, cr, cg, cb; };
    const CubeVertex vertices[] = {
        // +X face (right) - Red
        { 1.f, -1.f, -1.f,  1.f, 0.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 0.f, 0.f },
        { 1.f,  1.f,  1.f,  1.f, 0.f, 0.f },
        { 1.f,  1.f, -1.f,  1.f, 0.f, 0.f },
        // -X face (left) - Green
        {-1.f, -1.f, -1.f,  0.f, 1.f, 0.f },
        {-1.f, -1.f,  1.f,  0.f, 1.f, 0.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f, 0.f },
        {-1.f,  1.f, -1.f,  0.f, 1.f, 0.f },
        // +Y face (top) - Blue
        {-1.f,  1.f,  1.f,  0.f, 0.f, 1.f },
        { 1.f,  1.f,  1.f,  0.f, 0.f, 1.f },
        { 1.f,  1.f, -1.f,  0.f, 0.f, 1.f },
        {-1.f,  1.f, -1.f,  0.f, 0.f, 1.f },
        // -Y face (bottom) - Yellow
        {-1.f, -1.f, -1.f,  1.f, 1.f, 0.f },
        { 1.f, -1.f, -1.f,  1.f, 1.f, 0.f },
        { 1.f, -1.f,  1.f,  1.f, 1.f, 0.f },
        {-1.f, -1.f,  1.f,  1.f, 1.f, 0.f },
        // +Z face (front) - Cyan
        {-1.f, -1.f,  1.f,  0.f, 1.f, 1.f },
        { 1.f, -1.f,  1.f,  0.f, 1.f, 1.f },
        { 1.f,  1.f,  1.f,  0.f, 1.f, 1.f },
        {-1.f,  1.f,  1.f,  0.f, 1.f, 1.f },
        // -Z face (back) - Magenta
        { 1.f, -1.f, -1.f,  1.f, 0.f, 1.f },
        {-1.f, -1.f, -1.f,  1.f, 0.f, 1.f },
        {-1.f,  1.f, -1.f,  1.f, 0.f, 1.f },
        { 1.f,  1.f, -1.f,  1.f, 0.f, 1.f },
    };

    // --- Index data: 36 indices, Uint16, CCW winding ---
    const uint16_t indices[] = {
        // +X face
         0,  1,  2,   0,  2,  3,
        // -X face
         4,  5,  6,   4,  6,  7,
        // +Y face
         8,  9, 10,   8, 10, 11,
        // -Y face
        12, 13, 14,  12, 14, 15,
        // +Z face
        16, 17, 18,  16, 18, 19,
        // -Z face
        20, 21, 22,  20, 22, 23,
    };

    // --- Vertex format: stride=24, position at loc 0, color at loc 1 ---
    be::VertexFormat format;
    format.stride = sizeof(CubeVertex);
    format.attributes = {
        {0, be::VertexAttributeType::Float3, 0, false},
        {1, be::VertexAttributeType::Float3,
            static_cast<uint32_t>(offsetof(CubeVertex, cr)), false},
    };

    auto vertex_data = std::as_bytes(std::span(vertices));
    auto index_data = std::as_bytes(std::span(indices));

    // --- Create indexed model ---
    auto model = be::Model::create_indexed(
        device, format, vertex_data, index_data,
        be::IndexType::Uint16, shared_mat
    );
    if (!model) {
        std::fprintf(stderr, "FATAL: Failed to create cube model: %s\n",
                     be::to_string(model.error()).c_str());
        std::exit(EXIT_FAILURE);
    }

    return CubeResources{std::move(shared_mat), std::move(*model)};
}
```

**Requirements:**
- All 24 vertices are exact as listed above — each vertex is 6 floats (position + colour), stride = 24 bytes.
- Face colours match the spec exactly: +X=Red, -X=Green, +Y=Blue, -Y=Yellow, +Z=Cyan, -Z=Magenta.
- All 36 index values are in range [0, 23] — `IndexType::Uint16` is safe.
- Index winding is CCW when viewed from outside the cube (2 triangles per face, 6 indices per face).
- The vertex shader has `layout(location = 0) in vec3 a_position` and `layout(location = 1) in vec3 a_color`, output `v_color`.
- The fragment shader has `in vec3 v_color` and passes through as `frag_color = vec4(v_color, 1.0)`.
- There is NO `u_color` uniform — the only uniform is `u_mvp` (mat4).
- The material is created WITHOUT `known_uniforms` — no uniforms are pre-declared. The headless backend tracks uniforms dynamically.
- Error handling on each step: print `"FATAL: ..."` to stderr and `std::exit(EXIT_FAILURE)`.

---

### 5. `src/cmd/demo/cube_demo.h` — Cube demo header

```cpp
#pragma once

namespace buddd::engine {
class Platform;
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

/// Runs the cube demo: 120-frame render loop.
/// Each frame: compute rotation angle, build model matrix (rotate Y),
/// compute MVP, set u_mvp on material, then issue a single draw_indexed call.
///
/// @param platform  The engine platform (for event polling).
/// @param device    The render device (for rendering).
/// @param argc      Argument count (argv[0] is the demo name).
/// @param argv      Argument vector (argv[0] is the demo name).
/// @return 0 on success, non-zero on error.
[[nodiscard]] auto run_cube_demo(buddd::engine::Platform& platform,
                                 buddd::engine::RenderDevice& device,
                                 int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo
```

**Requirements:**
- Only forward-declare `Platform` and `RenderDevice`. No `#include` of any engine headers or backend-specific types.
- Namespace: `buddd::cmd::demo`.
- `run_cube_demo` is `[[nodiscard]]` (consistent with `run_triangle_demo`).
- No `setup_cube` declaration here — it is in `demo_helpers.h`.

---

### 6. `src/cmd/demo/cube_demo.cpp` — Cube demo implementation

```cpp
#include "demo/cube_demo.h"
#include "demo/demo_helpers.h"

#include "platform/platform.h"
#include "render/render_device.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/mat4.h"
#include "math/vec3.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace be = buddd::engine;

auto buddd::cmd::demo::run_cube_demo(
    be::Platform& platform, be::RenderDevice& device,
    [[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) -> int
{
    auto cube = setup_cube(device);

    // Camera setup
    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{3.0f, 2.0f, 3.0f},     // eye
        be::math::Vec3{0.0f, 0.0f, 0.0f},     // centre
        be::math::Vec3::unit_y()               // up
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );

    // Render loop: ~120 frames at 60 FPS (~2 seconds)
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16);
    auto demo_start = std::chrono::steady_clock::now();

    std::cerr << "Demo started: cube (" << target_frames << " frames)\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        if (!platform.poll_events()) {
            std::cerr << "Demo aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        // Compute elapsed time and rotation
        auto elapsed = std::chrono::steady_clock::now() - demo_start;
        float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
        float angle = elapsed_seconds * 0.5f;  // 0.5 rad/s around Y

        be::math::Mat4 model_matrix =
            be::math::Mat4::rotate(angle, be::math::Vec3::unit_y());
        be::math::Mat4 mvp =
            camera.projection_matrix() * camera.view_matrix() * model_matrix;

        device.begin_frame();

        // Set MVP uniform (face colours are in vertex attributes)
        cube.material->set_uniform("u_mvp", mvp);

        // Single indexed draw call covering all 36 indices
        cube.model.draw(device);

        device.end_frame();

        // Frame rate limiting
        auto frame_elapsed = std::chrono::steady_clock::now() - frame_start;
        if (frame_elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - frame_elapsed);
        }
    }

    std::cerr << "Demo complete: cube (" << target_frames << " frames rendered)\n";
    return EXIT_SUCCESS;
}
```

**Requirements:**
- Calls `setup_cube(device)` once at the start.
- Camera eye position: `(3.0f, 2.0f, 3.0f)`, looking at origin `(0.0f, 0.0f, 0.0f)`, up `Vec3::unit_y()`.
- Perspective: 60° FOV (via `math::radians(60.0f)`), aspect = 800/600, near 0.1, far 100.0.
- Elapsed time measured from `demo_start` (before loop) using `std::chrono::steady_clock`.
- Rotation: `angle = elapsed_seconds * 0.5f`, axis = `Vec3::unit_y()`.
- MVP = `projection * view * model` with `model = Mat4::rotate(angle, unit_y)`.
- Exactly **ONE** `draw_indexed` call per frame (via `cube.model.draw(device)`), which covers all 36 indices.
- `u_mvp` is set once per frame before drawing. There is NO `u_color` uniform.
- Frame-limited to ~60 FPS with sleep (identical to triangle demo).
- Early exit on window close: return `EXIT_SUCCESS` with abort message.
- Start message: `"Demo started: cube (120 frames)\n"` to `std::cerr`.
- End message: `"Demo complete: cube (120 frames rendered)\n"` to `std::cerr`.
- Abort message: `"Demo aborted by user (frame N)\n"` to `std::cerr`.

---

### 7. `src/cmd/commands/demo_command.cpp` — Demo registration

**Changes:**

1. **Add include** after the existing `#include "demo/triangle_demo.h"`:
```cpp
#include "demo/cube_demo.h"
```

2. **Update `k_demo_usage`** constant — add the cube line after the triangle line:
```cpp
inline constexpr std::string_view k_demo_usage =
    "Usage: buddd demo <demo>\n"
    "\n"
    "Available demos:\n"
    "  triangle     Run the triangle demo (120 frames)\n"
    "  cube         Run the cube demo (120 frames, rotating coloured cube)\n"
    "\n"
    "Demo names are case-sensitive.\n";
```

3. **Update validation condition** — change:
```cpp
if (demo_name != "triangle") {
```
to:
```cpp
if (demo_name != "triangle" && demo_name != "cube") {
```

4. **Add dispatch branch** — replace the direct return of `run_triangle_demo` with:
```cpp
    // Dispatch to per-demo function using if/else chain
    // Pass argc - 2, argv + 2 so the demo receives argv[0] == demo name
    if (demo_name == "triangle") {
        return buddd::cmd::demo::run_triangle_demo(**platform, **device, argc - 2, argv + 2);
    } else {
        // demo_name == "cube" (validated above)
        return buddd::cmd::demo::run_cube_demo(**platform, **device, argc - 2, argv + 2);
    }
```

**Requirements:**
- The `"cube"` usage text line must match exactly: `"  cube         Run the cube demo (120 frames, rotating coloured cube)\n"`.
- The validation condition must accept both `"triangle"` and `"cube"`.
- Both demos continue to work: `buddd demo triangle` and `buddd demo cube`.
- No changes to the existing `#include "demo/triangle_demo.h"`.

---

### 8. `tests/model_tests.cpp` — Model unit tests

The test file tests the `Model` class API and cube data verification using the **headless backend** (`RenderDeviceHeadless`). Tests do NOT call `setup_cube` (which is in demo code, not linked to tests). Instead, tests create materials and models inline.

**Header includes:**
```cpp
#include "render/model.h"
#include "render/render_device.h"
#include "render/render_device_headless.h"
#include "render/shader.h"
#include "render/material.h"
#include "render/vertex_format.h"
#include "render/primitive_topology.h"
#include "error.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
```

**Test fixture / helpers:**

```cpp
namespace be = buddd::engine;

/// Creates a headless device for testing.
auto create_headless_device() -> std::unique_ptr<be::RenderDevice> {
    return std::make_unique<be::RenderDeviceHeadless>(800, 600);
}

/// Minimal vertex shader with position + color attributes and u_mvp uniform.
constexpr std::string_view k_test_vs = R"(
    #version 450 core
    layout(location = 0) in vec3 a_position;
    layout(location = 1) in vec3 a_color;
    out vec3 v_color;
    uniform mat4 u_mvp;
    void main() {
        gl_Position = u_mvp * vec4(a_position, 1.0);
        v_color = a_color;
    }
)";

/// Minimal fragment shader that passes vertex color through.
constexpr std::string_view k_test_fs = R"(
    #version 450 core
    in vec3 v_color;
    out vec4 frag_color;
    void main() {
        frag_color = vec4(v_color, 1.0);
    }
)";

/// Creates a test material with the standard position+color shaders.
auto create_test_material(be::RenderDevice& device) -> std::shared_ptr<be::Material> {
    auto vs = device.create_shader(be::ShaderType::Vertex, k_test_vs);
    REQUIRE(vs.has_value());
    auto fs = device.create_shader(be::ShaderType::Fragment, k_test_fs);
    REQUIRE(fs.has_value());
    auto mat = device.create_material(std::move(*vs), std::move(*fs));
    REQUIRE(mat.has_value());
    return std::shared_ptr<be::Material>(std::move(*mat));
}
```

**Test data structures** (for cube data verification tests):

```cpp
// Interleaved vertex: position (Float3) + color (Float3), stride = 24
struct CubeVertex { float px, py, pz, cr, cg, cb; };
```

**Test cases** — see Required tests section below for the full table.

---

## Required tests

All tests are in `tests/model_tests.cpp`, using Catch2 v3, running on `RenderDeviceHeadless(800, 600)`. Every test creates its own device and material.

### Model factory tests

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| T-01 | `"Model default construction creates null model"` | `[model]` | Default `Model{}` — calling `draw(device)` on null model is a no-op (no crash). Verifiable by calling draw between begin_frame/end_frame and checking no exception/crash. |
| T-02 | `"Model is non-copyable and movable"` | `[model]` | `static_assert(!std::is_copy_constructible_v<Model>)`. Move construct and move assign compile and execute. |
| T-03 | `"Model::create with valid data succeeds"` | `[model]` | Create non-indexed model with 3 vertices (Float3 position only). Verify `model.has_indices() == false`, `model.vertex_count() == 3`, `model.index_count() == 0`. |
| T-04 | `"Model::create_indexed with valid data succeeds"` | `[model]` | Create indexed model with 4 vertices, 6 indices (Uint16). Verify `model.has_indices() == true`, `model.vertex_count() == 4`, `model.index_count() == 6`. |
| T-05 | `"Model::create returns InvalidArgument for empty vertex data"` | `[model]` | `Model::create(device, format, {}, material)` returns `Error::Category::InvalidArgument`. |
| T-06 | `"Model::create returns InvalidArgument for zero stride"` | `[model]` | `VertexFormat{0, ...}` returns `InvalidArgument`. |
| T-07 | `"Model::create returns InvalidArgument for zero attributes"` | `[model]` | `VertexFormat{12, {}}` returns `InvalidArgument`. |
| T-08 | `"Model::create_indexed returns InvalidArgument for empty index data"` | `[model]` | `Model::create_indexed(device, format, vertex_data, {}, Uint16, material)` returns `InvalidArgument`. |

### Model accessor tests

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| T-09 | `"Model::material returns writable reference"` | `[model]` | Create model with material. Set a uniform via `model.material().set_uniform(...)`. Verify the uniform value persists (headless backend tracks uniform state). |
| T-10 | `"Model::material const overload"` | `[model]` | Const reference to model, call `.material()` returns `const Material&`. Compiles. |
| T-11 | `"Model::vertices returns non-null reference"` | `[model]` | `&model.vertices()` is a valid non-null pointer. |
| T-12 | `"Model::indices returns reference on indexed model"` | `[model]` | On indexed model: `model.has_indices() == true`, `&model.indices()` is a valid non-null pointer. |

### Model draw tests

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| T-13 | `"Model::draw on non-indexed model"` | `[model]` | Create non-indexed model, call `draw` between `begin_frame()`/`end_frame()`. No crash. |
| T-14 | `"Model::draw on indexed model"` | `[model]` | Create indexed model, call `draw` between `begin_frame()`/`end_frame()`. No crash. |
| T-15 | `"Model::draw on null model is no-op"` | `[model]` | Default-constructed `Model{}.draw(device)` — no crash. |
| T-16 | `"Moved-from Model draw is no-op"` | `[model]` | Create model, move to another variable, call draw on the moved-from model — no crash. |

### Model move semantics tests

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| T-17 | `"Move constructor transfers ownership"` | `[model]` | After `Model m2(std::move(m1))`, `m2.has_indices()` equals original status, `m2.vertex_count()` is correct. `m1` is null (draw on `m1` is no-op). |
| T-18 | `"Move assignment transfers ownership"` | `[model]` | After `Model m2; m2 = std::move(m1)`, `m2` is valid, `m1` is null. Draw on `m2` works. |

### Cube data / material tests

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| T-19 | `"Model with 24 vertices and 36 indices (cube data)"` | `[cube]` | Create model with 24 cube vertices (stride 24), 36 Uint16 indices, material with u_mvp. Verify `model.vertex_count() == 24`, `model.index_count() == 36`, `model.has_indices() == true`. |
| T-20 | `"Cube material has u_mvp uniform"` | `[cube]` | Material created with `u_mvp` uniform: `model.material().has_uniform("u_mvp") == true`. |
| T-21 | `"Cube material does NOT have u_color uniform"` | `[cube]` | `model.material().has_uniform("u_color") == false` (no u_color uniform in this spec). |
| T-22 | `"Setting u_mvp on cube material succeeds"` | `[cube]` | `model.material().set_uniform("u_mvp", be::math::Mat4::identity())` returns success. |

### Demo run tests

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| T-23 | `"run_cube_demo completes without crash (headless)"` | `[cube]` | Create headless Platform + RenderDevice. Call `run_cube_demo(platform, device, 2, dummy_argv)` where `dummy_argv = {"buddd", "cube"}`. Returns `EXIT_SUCCESS`. No crash during 120-frame loop. |

### Shared ownership test

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| T-24 | `"Model shares material ownership via shared_ptr"` | `[model]` | Create `shared_ptr<Material>`, create Model with it. Verify `&model.material() == shared_ptr.get()` (same object). Material stays alive when original shared_ptr is reset if Model still holds a reference. |

**Total: 24 test cases.**

---

## Edge cases

All edge cases from the spec (see `.specs/sprint-2026-05/3d-cube-demo/spec.md#edge-cases`) must be handled by the implementation as follows:

| Edge case | Required behavior |
|---|---|
| `Model::create()` with empty vertex data (zero-length span) | Returns `Error::Category::InvalidArgument`. |
| `Model::create_indexed()` with empty vertex data | Returns `Error::Category::InvalidArgument`. |
| `Model::create_indexed()` with empty index data (zero-length span) | Returns `Error::Category::InvalidArgument`. |
| `Model::create()` with `VertexFormat` stride of 0 | Returns `Error::Category::InvalidArgument`. |
| `Model::create()` with `VertexFormat` having zero attributes | Returns `Error::Category::InvalidArgument`. |
| Vertex data size is not a multiple of stride | The factory still creates the buffer (no validation — same as SPEC-005). UB at draw time if backend requires alignment. |
| `Model::draw()` called on a moved-from (null) model | No-op — no crash, no draw call issued. |
| `Model::draw()` called outside `begin_frame()`/`end_frame()` | Undefined behaviour (same as SPEC-005 for `RenderDevice` draw methods). |
| `Model::draw()` called on a non-indexed model | Issues `device.draw(topology_, *vb_, *material_, vertex_count_, 0)`. |
| `Model::draw()` called on an indexed model | Issues `device.draw_indexed(topology_, *vb_, *ib_, *material_, index_count_, 0)`. |
| `Model::material()` called on a null/moved-from model | Undefined behaviour (dereferences null `shared_ptr`). |
| `Model::vertices()` called on a null/moved-from model | Undefined behaviour. |
| `Model::indices()` called on a non-indexed model | Undefined behaviour (caller should check `has_indices()` first). |
| Multiple `Model::draw()` calls with different uniform values between them | Each draw call reflects the current uniform state at the time of the call. Legal and expected. |
| Camera aspect ratio does not match window dimensions | The cube appears stretched. The demo matches 800×600 — this is a demo concern, not an engine concern. |
| User closes the window before 120 frames | The loop exits early, return value is `EXIT_SUCCESS`. |
| `setup_cube` called with a headless backend | Creates successfully with simulated resources. All uniforms are trackable. |
| Index buffer created with `Uint16` but index values exceed 65535 | Undefined behaviour in the OpenGL backend. For the cube demo, indices are in range 0–23, so `Uint16` is safe. |
| Two `Model` instances sharing the same `RenderDevice` | Each model owns its own buffers but can share the same `Material` via `shared_ptr`. Legal and expected. |
| `Model::draw()` called after the `RenderDevice` that created it has been destroyed | Undefined behaviour — the GPU resources (buffers, material program) are no longer valid. |
| Material is destroyed while `Model` still holds a `shared_ptr` to it | The material stays alive until the last `shared_ptr` is destroyed (safe reference counting). |
| `Model::draw()` called on a model with only `vb_` but null `material_` | No-op (null material_ guard). |

## Security impact

None. The `Model` class and cube demo operate entirely in user space with no I/O beyond window creation (already handled by `demo_command.cpp`). GLSL shader source for the cube demo is embedded in the demo code — no shader files are loaded from disk. The architecture boundary (CONST-001) is maintained: `cube_demo.h` exposes no backend types. `model.h` (in `src/engine/render/`) is part of the engine layer and may include backend headers internally in its `.cpp` file, but the public header exposes no backend types.

## Data and migration impact

None. No schema changes, data migrations, seed data, or persistent state. New files only.

## API compatibility impact

- The `Model` class is a new public API under `buddd::engine` namespace — no backward compatibility concerns (no prior API to break).
- The factory methods accept `std::shared_ptr<Material>` — callers convert their `unique_ptr<Material>` from `create_material()` to `shared_ptr` before passing it in.
- The `PrimitiveTopology` parameter has a default value `PrimitiveTopology::Triangles`, which is forward-compatible — callers who omit it get the same behaviour.
- The demo registration extends the existing dispatch chain without breaking the existing `triangle` demo.

## Documentation impact

None. The wiki and documentation are out of scope for implementation contracts. The existing spec serves as the primary API reference.

## ADR impact

None. The implementation follows existing ADR decisions:
- ADR-001 (`Result<T>` pattern) is followed for factory methods.
- ADR-003 (`draw()` returns `void` exception) is extended to `Model::draw()` with the same rationale (hot-path dispatch, precondition violations are UB).
- ADR-004 (demo system) is followed for demo registration.
- No new ADR is required.

## Constitution impact

None. The implementation respects CONST-001 (architecture boundaries — no GLM/SDL3/OpenGL in `cube_demo.h` public header, `Model` lives in `src/engine/render/` inside engine boundary) and CONST-002 (testing policy — all testable code has corresponding tests).

## Done criteria

The implementation is complete when ALL of the following are satisfied:

### Build and compilation
- [ ] `src/engine/render/model.h` compiles without errors.
- [ ] `src/engine/render/model.cpp` compiles without errors.
- [ ] `src/cmd/demo/demo_helpers.h` compiles without errors (modified).
- [ ] `src/cmd/demo/demo_helpers.cpp` compiles without errors (modified).
- [ ] `src/cmd/demo/cube_demo.h` compiles without errors.
- [ ] `src/cmd/demo/cube_demo.cpp` compiles without errors.
- [ ] `tests/model_tests.cpp` compiles without errors.
- [ ] `src/cmd/commands/demo_command.cpp` compiles without errors (modified).
- [ ] `tests/CMakeLists.txt` is modified to include `model_tests.cpp` in both branches.
- [ ] `cmake --build --preset debug` succeeds with no warnings related to new code.
- [ ] No GLM, SDL3, or OpenGL headers are included in `src/cmd/demo/cube_demo.h` (verified by code review).
- [ ] `static_assert(!std::is_copy_constructible_v<Model>)` compiles and passes.
- [ ] `static_assert(std::is_move_constructible_v<Model>)` compiles and passes.
- [ ] `Model` stores `std::shared_ptr<Material>` (not `std::unique_ptr<Material>`). Verified by code review.

### Test results
- [ ] All test cases pass (`ctest --preset debug` or `./build/debug/tests/buddd_tests [model] [cube]`):
  - T-01 through T-18 (Model API): ALL pass
  - T-19 through T-22 (Cube data): ALL pass
  - T-23 (run_cube_demo): pass
  - T-24 (shared ownership): pass
  - Total: 24 runnable test cases

### Acceptance criteria coverage
- [ ] AC-001 (Model class exists): file `src/engine/render/model.h` exists, `buddd::engine::Model` is a valid type.
- [ ] AC-002 (Model::create signature): compiles with `std::shared_ptr<Material>` and `PrimitiveTopology` default, no shader source parameters.
- [ ] AC-003 (Model::create_indexed signature): compiles with `IndexType`, `PrimitiveTopology`, and `std::shared_ptr<Material>`.
- [ ] AC-004 (Model::draw dispatch): covered by T-13, T-14, T-15 — code review confirms `draw` dispatches correctly for indexed vs non-indexed.
- [ ] AC-005 (Model::material): covered by T-09, T-10.
- [ ] AC-006 (Model::vertices): covered by T-11.
- [ ] AC-007 (Model::indices / has_indices): covered by T-12, T-04, T-03.
- [ ] AC-008 (vertex_count / index_count): covered by T-03, T-04.
- [ ] AC-009 (non-copyable, movable): covered by T-02, T-17, T-18.
- [ ] AC-010 (empty vertex data -> InvalidArgument): covered by T-05.
- [ ] AC-011 (empty index data -> InvalidArgument): covered by T-08.
- [ ] AC-012 (create chains errors, no leak on failure): code review of factory method confirms RAII cleanup. `std::shared_ptr<Material>` releases refcount on scope exit. ASAN-clean on failure path.
- [ ] AC-013 (create_indexed creates index buffer): code review confirms `device.create_index_buffer` is called.
- [ ] AC-014 (setup_cube: 24 vertices, 36 indices, Uint16): covered by T-19. Code review confirms vertex count and index count.
- [ ] AC-015 (shader sources match spec): code review confirms shader source strings match: `a_position` (loc 0), `a_color` (loc 1), `u_mvp` (Mat4), no `u_color`.
- [ ] AC-016 (6 face colours encoded in vertex data): code review confirms vertex data has correct face colours at correct vertex positions. Covered by T-19 (vertex data structure).
- [ ] AC-017 (Camera at (3,2,3), MVP computation): code review confirms camera.look_at and MVP equation in `cube_demo.cpp`.
- [ ] AC-018 (120 frames at ~60 FPS): covered by T-23. Code review confirms loop bound = 120.
- [ ] AC-019 (cube registered in demo_command.cpp): code review confirms usage text and dispatch branch.
- [ ] AC-020 (triangle still works): code review confirms triangle dispatch branch is preserved unchanged.
- [ ] AC-021 (moved-from draw is no-op): covered by T-16.
- [ ] AC-022 (zero stride -> InvalidArgument): covered by T-06.
- [ ] AC-023 (zero attributes -> InvalidArgument): covered by T-07.
- [ ] AC-024 (has_uniform("u_mvp") true, has_uniform("u_color") false): covered by T-20, T-21.
- [ ] AC-025 (set_uniform("u_mvp") succeeds): covered by T-22.
- [ ] AC-026 (no backend headers in cube_demo.h): verified by code review — `cube_demo.h` has only forward declarations.
- [ ] AC-027 (CCW winding): code review of index data confirms CCW winding for each face when viewed from outside. The index buffer ordering matches the specification.

### Memory safety
- [ ] ASAN build (`-fsanitize=address`) shows no leaks or use-after-free in model tests.
- [ ] Failure paths (null data, zero stride, zero attributes) do not leak prior resources.

### Demo verification
- [ ] `./buddd demo` shows `cube` in the usage text.
- [ ] `./buddd demo cube` runs the cube demo (visual verification on OpenGL backend with display — the cube is visible, rotating, and all six faces are distinguishable by colour).
- [ ] `./buddd demo triangle` still runs the triangle demo (visual verification).
