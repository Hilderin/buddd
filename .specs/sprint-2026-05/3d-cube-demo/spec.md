# SPEC-009 — Model Utility & 3D Cube Demo

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

## Problem

The Buddd Engine can render a coloured triangle (SPEC-005) but has no higher-level primitive to represent a renderable object. Every demo or test that wants to draw geometry must manually create shaders, a material, a vertex buffer, and optionally an index buffer — then coordinate their lifetimes and draw calls. This is repetitive, error-prone, and obscures the intent of the code.

At the same time, the engine has no interactive 3D demo. The only existing demo (`triangle`) is 2D and static (no camera, no transform, no animation). A spinning cube with per-face colours is a canonical next milestone: it exercises:

- Indexed drawing (vertex reuse via index buffer)
- The `Camera` math type (view / projection matrices)
- Per-frame transform updates (rotation matrix, MVP computation)
- Per-vertex colour attributes (face colours encoded in vertex data)
- A real-time render loop with varying state

Without this spec, there is no reusable `Model` abstraction and no 3D demo to validate the render pipeline end-to-end.

## Goals

- **Model utility class**: Provide a reusable `Model` class in `src/engine/render/model.h` (namespace `buddd::engine`) that encapsulates:
  - A `VertexBuffer` (always present)
  - An optional `IndexBuffer`
  - A shared `Material` reference (`std::shared_ptr<Material>`)
  - A `draw(RenderDevice&)` convenience that issues the appropriate draw call with the stored topology, buffers, and material.
  - Factory methods that take an already-created `Material` by const reference and accept vertex/index data, returning `Result<Model>`. No shader compilation or material linking occurs inside Model.
- **Cube demo**: Provide `buddd demo cube` that renders a coloured rotating cube for 120 frames at ~60 FPS.
- **Per-face colours via vertex data**: Six distinct colours, one per face, encoded as a per-vertex `Float3 color` attribute alongside `Float3 position`. No per-face uniform changes needed.
- **Reusable cube setup**: Provide `setup_cube()` in `demo_helpers.h/cpp` that creates a `CubeResources` struct containing a `shared_ptr<Material>` and a `Model`, to be shared across demos and tests.
- **Camera integration**: Use the existing `Camera` type (`buddd::engine::math::Camera`) with a fixed eye position looking at the origin. Compute MVP per frame and pass via `u_mvp` uniform.
- **Demo registration**: Register the cube demo in `demo_command.cpp` so it appears in usage text and dispatch.
- **Testability**: The Model class must work with both OpenGL and Headless backends. The headless backend should be able to simulate the cube demo (no crash, uniform tracking works).

## Non-goals

- No per-face uniform changes — face colours are encoded in vertex data (not via `u_color`).
- No static `Model::create_cube()` method — cube vertex/index data is defined in the demo code.
- No textured cube — this is solid-colour per face only.
- No lighting, shading, or normal vectors.
- No model file loading (.obj, .gltf, etc.).
- No animation or transform hierarchy beyond the single Y-axis rotation.
- No user interaction (mouse, keyboard) — the camera is fixed and the rotation is hardcoded.
- No multi-material support in Model v1 — Model has a single Material.
- No dynamic buffer updates after creation — data is uploaded once at construction.
- No instanced rendering.
- No wireframe or debug rendering modes.
- No `Scene` or `Entity` integration — the cube stands alone.
- No frame-rate independence beyond the fixed ~60 FPS sleep-based approach (identical to the triangle demo).
- No shader compilation or material linking inside Model — Model takes a pre-created Material.
- No `Material::clone()` or polymorphic copying requirement — the `const Material&` to `shared_ptr<Material>` conversion mechanism is a spec open question (see Q-01).

## Actors

| Actor | Description |
|---|---|
| Engine developer | A developer adding features who needs to create renderable geometry quickly. Uses `Model::create()` or `Model::create_indexed()` to bundle geometry with an existing Material. |
| Application developer | A developer building on top of the engine who wants to draw a 3D object. Creates a `Model`, sets uniforms on its material per frame, and calls `draw()`. |
| End user | Runs `buddd demo cube` and sees a spinning multicoloured cube in a window for ~2 seconds. |
| Test suite | Catch2 v3 tests that create a `Model` in headless mode, exercise its API, and verify uniform state without a GPU. |

## User-visible behavior

### Model class

`Model` lives in `src/engine/render/model.h` + `src/engine/render/model.cpp`, namespace `buddd::engine`. It is a concrete class (not abstract) that owns GPU resources through the abstract `VertexBuffer`, `IndexBuffer` (optional), and a `shared_ptr<Material>`.

Model does **not** own the Material in an exclusive sense — it shares ownership via `std::shared_ptr<Material>`. The Material is created externally (e.g., via `RenderDevice::create_material`) and passed into Model factories as a `std::shared_ptr<Material>`. The caller typically converts their `std::unique_ptr<Material>` (returned by `create_material`) to a shared_ptr via `std::shared_ptr<Material>(std::move(unique_material))`.

#### Factory methods

```cpp
namespace buddd::engine {

class Model {
public:
    /// Creates a non-indexed Model.
    /// On failure, returns an Error describing the failure (buffer creation
    /// or invalid arguments only — no shader compilation occurs here).
    /// The material is shared (not owned exclusively) via shared_ptr.
    static auto create(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::shared_ptr<Material> material,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;

    /// Creates an indexed Model.
    /// On failure, returns an Error describing the failure.
    static auto create_indexed(
        RenderDevice& device,
        const VertexFormat& vertex_format,
        std::span<const std::byte> vertex_data,
        std::span<const std::byte> index_data,
        IndexType index_type,
        std::shared_ptr<Material> material,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    ) -> Result<Model>;
```

- `topology` sets the primitive topology for all subsequent draw calls. It defaults to `PrimitiveTopology::Triangles`. The topology is fixed at model creation time and cannot be changed.
- `material` is a `std::shared_ptr<Material>` that shares ownership of the Material with the Model. The caller typically converts a `std::unique_ptr<Material>` (returned by `RenderDevice::create_material()`) to a `shared_ptr` via the `std::shared_ptr<Material>(std::move(unique_material))` constructor.
- Both factory methods create a vertex buffer (and optionally an index buffer), then bundle them with the material into a `Model` value.
- No shader compilation or material linking happens inside these factories — those are the caller's responsibility.
- If any step fails, the returned `Error` contains the appropriate category (e.g., `InvalidArgument` for precondition violations, or `ResourceCreationFailed` for buffer creation failures from the RenderDevice). Resources from prior successful steps are properly cleaned up via RAII.

#### Drawing

```cpp
    /// Draws the entire model using the current material state.
    ///
    /// If the model has an index buffer, issues one indexed draw call
    /// covering all indices (index_count vertices starting at index 0).
    ///
    /// If the model has no index buffer, issues one non-indexed draw call
    /// covering all vertices (vertex_count vertices starting at vertex 0).
    ///
    /// Uses the primitive topology that was specified at model creation time.
    ///
    /// The caller is responsible for setting any desired uniforms on
    /// material() before calling draw().
    ///
    /// Behaviour is undefined if called outside a begin_frame()/end_frame() pair.
    ///
    /// Returns void (not Result<void>) by consistency with RenderDevice::draw()
    /// and RenderDevice::draw_indexed(). Drawing is a hot-path operation where
    /// precondition violations (invalid state, out-of-bounds access) are undefined
    /// behaviour — the caller must ensure correct state before drawing. This is the
    /// same exception established in ADR-003 for RenderDevice draw methods.
    auto draw(RenderDevice& device) const -> void;
```

#### Accessors

```cpp
    /// Returns a reference to the material. The caller can set uniforms
    /// on the material before calling draw().
    auto material() noexcept -> Material&;
    auto material() const noexcept -> const Material&;

    /// Returns a reference to the vertex buffer.
    auto vertices() const noexcept -> const VertexBuffer&;

    /// Returns a reference to the index buffer.
    /// Behaviour is undefined if the model has no index buffer.
    auto indices() const noexcept -> const IndexBuffer&;

    /// Returns true if the model was created with an index buffer
    /// (i.e., via create_indexed).
    auto has_indices() const noexcept -> bool;

    /// Returns the number of vertices in the vertex buffer.
    auto vertex_count() const noexcept -> uint32_t;

    /// Returns the number of indices in the index buffer.
    /// Returns 0 if the model has no index buffer.
    auto index_count() const noexcept -> uint32_t;
```

#### Lifecycle

```cpp
    // Non-copyable (owns unique GPU resources; shared_ptr<Material> is movable).
    Model(const Model&) = delete;
    auto operator=(const Model&) -> Model& = delete;

    // Movable — transfers ownership of GPU resources and shared material ref.
    Model(Model&&) noexcept = default;
    auto operator=(Model&&) noexcept -> Model& = default;

    ~Model() = default;

private:
    std::unique_ptr<VertexBuffer> vb_;
    std::unique_ptr<IndexBuffer> ib_;
    std::shared_ptr<Material> material_;
    PrimitiveTopology topology_{Triangles};
    uint32_t vertex_count_{0};
    uint32_t index_count_{0};

    Model(
        std::unique_ptr<VertexBuffer> vb,
        std::unique_ptr<IndexBuffer> ib,
        std::shared_ptr<Material> material,
        PrimitiveTopology topology,
        uint32_t vertex_count,
        uint32_t index_count
    ) noexcept;
};
```

- `Model` is movable but not copyable.
- The destructor destroys the owned `VertexBuffer` and `IndexBuffer` resources via their virtual destructors. The `shared_ptr<Material>` decrements its refcount but does not necessarily destroy the Material.
- A default-constructed or moved-from `Model` is in a "null" state where `draw()` is a no-op (does not crash) and accessors return null references (undefined behaviour).

### Cube demo — demo_helpers additions

The cube resource creation lives in `demo_helpers.h` / `demo_helpers.cpp` for reusability alongside `setup_triangle`.

#### CubeResources struct

```cpp
struct CubeResources {
    std::shared_ptr<buddd::engine::Material> material;
    buddd::engine::Model model;
};
```

#### setup_cube function

```cpp
/// Creates a CubeResources for a unit cube (2×2×2, centred at origin):
/// - 24 vertices (Float3 position + Float3 color per vertex, stride 24)
/// - 36 indices (Uint16, 6 per face, CCW winding)
/// - Material with a_position (loc 0), a_color (loc 1), u_mvp (Mat4) uniform
///
/// Face colors are encoded in vertex data:
///   +X (right):  Red    (1,0,0)  → vertices  0-3
///   -X (left):   Green  (0,1,0)  → vertices  4-7
///   +Y (top):    Blue   (0,0,1)  → vertices  8-11
///   -Y (bottom): Yellow (1,1,0)  → vertices 12-15
///   +Z (front):  Cyan   (0,1,1)  → vertices 16-19
///   -Z (back):   Magenta(1,0,1)  → vertices 20-23
///
/// On failure, prints a FATAL error to stderr and calls std::exit(EXIT_FAILURE),
/// consistent with setup_triangle().
auto setup_cube(buddd::engine::RenderDevice& device) -> CubeResources;
```

#### setup_cube behaviour

`setup_cube(device)` performs the following:

1. Defines GLSL vertex shader source with:
   - `layout(location = 0) in vec3 a_position;`
   - `layout(location = 1) in vec3 a_color;`
   - `out vec3 v_color;`
   - `uniform mat4 u_mvp;`
   - Body: `gl_Position = u_mvp * vec4(a_position, 1.0); v_color = a_color;`

2. Defines GLSL fragment shader source with:
   - `in vec3 v_color;`
   - `out vec4 frag_color;`
   - Body: `frag_color = vec4(v_color, 1.0);`

 3. Creates vertex and fragment shaders via `device.create_shader`, then creates a `Material` via `device.create_material` (returning `std::unique_ptr<Material>`).

 4. Converts the `std::unique_ptr<Material>` to `std::shared_ptr<Material>`: `std::shared_ptr<Material>(std::move(*unique_mat))`. This shared_ptr is stored in `CubeResources::material` and passed to `Model::create_indexed()` to be shared with the Model.

5. Defines 24 vertices for a unit cube (axis-aligned, centred at origin, size 2×2×2):
   - Each vertex is position (Float3) + colour (Float3), stride = 24 bytes.
   - 4 vertices per face, 6 faces total.
   - Six distinct colours as specified above.

6. Defines 36 indices (6 per face, 2 triangles per face):
   - Index type: `Uint16` (values fit in 16 bits: 0–23).
   - Each face's 4 vertices are indexed as two triangles with counter-clockwise (CCW) winding when viewed from outside the cube. This matches the OpenGL default front-face convention (`GL_CCW`), ensuring faces are visible under default back-face culling.

7. Creates an **indexed** `Model` via `Model::create_indexed(...)` with:
   - Vertex format: `{stride=24, attributes=[{location=0, type=Float3, offset=0}, {location=1, type=Float3, offset=12}]}`
   - Topology: `PrimitiveTopology::Triangles` (default, can be omitted)

8. On failure at any step, prints to stderr and calls `std::exit(EXIT_FAILURE)` — matching the `setup_triangle` error pattern.

### Cube demo — cube_demo files

The cube demo entry point lives in new files:
- `src/cmd/demo/cube_demo.h`
- `src/cmd/demo/cube_demo.cpp`

Namespace: `buddd::cmd::demo`.

#### Entry points

```cpp
namespace buddd::cmd::demo {

/// Runs the cube demo: 120-frame render loop.
/// Each frame: compute rotation angle (elapsed time * 0.5 rad/s),
/// build model matrix (rotate Y), compute MVP, set u_mvp on material,
/// then issue a single draw_indexed call.
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

#### Demo loop behaviour

`run_cube_demo(platform, device, argc, argv)`:

1. Calls `setup_cube(device)` to create the `CubeResources` (material + model).

2. Sets up a `Camera` instance:
   - Position: `(3.0f, 2.0f, 3.0f)`
   - Looking at: `(0.0f, 0.0f, 0.0f)` (via `camera.look_at({3.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, Vec3::unit_y())`)
   - Perspective parameters: `camera.set_perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f)` — matching the 800×600 window aspect ratio (~1.333).

3. Runs a render loop of `target_frames = 120` iterations, frame-limited to ~60 FPS (16 ms per frame), identical to the triangle demo loop.

   For each frame:

   a. Poll events via `platform.poll_events()`. If the user closes the window, exit early with success.

   b. Compute elapsed time since demo start (in seconds, using `std::chrono::steady_clock`).

   c. Compute rotation angle: `angle = elapsed_seconds * 0.5f` (0.5 radians per second around Y).

   d. Compute model matrix: `model = Mat4::rotate(angle, Vec3::unit_y())`.

   e. Compute MVP: `mvp = camera.projection_matrix() * camera.view_matrix() * model`.

   f. Call `device.begin_frame()`.

   g. Set `u_mvp` uniform on the material: `cube.material->set_uniform("u_mvp", mvp);`.

   h. Issue a single draw call: `cube.model.draw(device);` — internally, this issues one `draw_indexed` call with all 36 indices, rendering all six faces with their per-vertex colours.

   i. Call `device.end_frame()`.

   j. Sleep for the remainder of the 16 ms frame budget.

4. Prints a start message (`"Demo started: cube (120 frames)\n"`) and a completion message (`"Demo complete: cube (120 frames rendered)\n"`) to stderr.

The cube demo assumes back-face culling is enabled (OpenGL default) and uses counter-clockwise winding, so faces viewed from outside the cube are visible and back-faces (interior) are culled. This is the standard rendering convention; the demo does not change the default culling state.

### Demo command registration

In `demo_command.cpp`:

1. Include `"demo/cube_demo.h"`.
2. Add `"  cube         Run the cube demo (120 frames, rotating coloured cube)\n"` to the `k_demo_usage` constant.
3. Add `demo_name != "cube"` to the validation condition alongside `"triangle"`.
4. Add an `else if` branch (or extend the chain) to dispatch to `run_cube_demo` when `demo_name == "cube"`.

The registration pattern must not break the existing `triangle` demo — both demos remain available via `buddd demo triangle` and `buddd demo cube`.

## User stories

### Story 1 — Create a non-indexed Model from vertex data and a material (Priority: P1)

As an engine developer, I want to create a `Model` from vertex data and an already-created `Material`, so that I can render geometry without manually managing buffers and material lifecycles.

**Given** a valid `RenderDevice` (OpenGL or Headless) and an existing `std::shared_ptr<Material> my_material`

**When** I write:
```cpp
VertexFormat format{12, {{0, VertexAttributeType::Float3, 0}}};
float vertices[] = { /* ... */ };
auto model = Model::create(device, format, std::as_bytes(std::span(vertices)),
                           my_material);
```
**Then** `model` is a valid `Result<Model>` containing a ready-to-use model. `model->material()` returns a reference to the provided material. `model->vertex_count()` returns the correct vertex count. `model->has_indices()` returns `false`.

### Story 2 — Create an indexed Model (Priority: P1)

As an engine developer, I want to create a `Model` with an index buffer, so that I can reuse vertices across triangles.

**Given** a valid `RenderDevice` and an existing `Material`

**When** I call `Model::create_indexed(...)` with vertex data, index data, and the material

**Then** the model is created successfully. `model->has_indices()` returns `true`. `model->index_count()` returns the correct index count. `model->indices()` returns a reference to an `IndexBuffer`.

### Story 3 — Draw a Model with one call (Priority: P1)

As an engine developer, I want to call `model.draw(device)` between `begin_frame()` and `end_frame()` to render the model.

**Given** a valid indexed `Model` with a material whose `u_mvp` uniform is set

**When** I call `model.draw(device)` between `begin_frame()` and `end_frame()`

**Then** one indexed draw call is issued with the model's index count and topology `Triangles`. The material's current uniform values are used.

### Story 4 — Run the cube demo and see a spinning coloured cube (Priority: P1)

As an end user, I want to run `buddd demo cube` and see a window with a rotating multi-coloured cube for ~2 seconds.

**Given** a built `buddd` executable on a system with a display

**When** I run `./buddd demo cube`

**Then** a window opens with title `"Buddd Engine — Demo: cube"`, a coloured cube appears and rotates slowly around the Y axis, and after ~120 frames (~2 seconds) the window closes and the process exits with code 0. During the demo, six distinct colours (red, green, blue, yellow, cyan, magenta) are visible on the six faces.

### Story 5 — Reusable cube setup via setup_cube (Priority: P2)

As a demo developer, I want to create a cube's resources (material and model) with a single function call, so that I can reuse the cube in multiple demos or tests without duplicating vertex/index data.

**Given** a valid `RenderDevice`

**When** I call `auto cube = setup_cube(device)`

**Then** a `CubeResources` struct is returned containing a `shared_ptr<Material>` and a `Model` with 24 vertices (stride 24, position + color), 36 indices, and a material with a single `u_mvp` uniform. The six face colours (red, green, blue, yellow, cyan, magenta) are encoded in the vertex data.

### Story 6 — Model creation failure returns a descriptive error (Priority: P1)

As an engine developer, I want to receive a meaningful error when model creation fails due to invalid arguments, so that I can diagnose the problem.

**Given** zero-length vertex data

**When** I call `Model::create(device, format, {}, *material)`

**Then** the returned `Result<Model>` contains an `Error` with category `InvalidArgument` and a non-empty message.

### Story 7 — Headless backend: Model creation and draw are no-ops (Priority: P1)

As a CI/test engineer, I want `Model` to work in headless mode (no GPU, no display), so that unit tests can exercise the API without hardware dependencies.

**Given** a headless `RenderDevice` and a material created via the headless backend

**When** I create a `Model` and call `model.draw(device)`

**Then** creation succeeds, all accessors return valid values, and `draw()` completes without crashing or involving any GPU. Uniform state is tracked correctly (set and query works).

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `Model` class exists in `src/engine/render/model.h`, namespace `buddd::engine`. | File compiles; `buddd::engine::Model` is a valid type. |
| AC-002 | `Model::create(...)` is a static method returning `Result<Model>`. | Compiles with the signature: `static auto create(RenderDevice&, const VertexFormat&, std::span<const std::byte>, std::shared_ptr<Material>, PrimitiveTopology = Triangles) -> Result<Model>`. |
| AC-003 | `Model::create_indexed(...)` is a static method returning `Result<Model>` with additional `std::span<const std::byte> index_data`, `IndexType index_type`, and `PrimitiveTopology` parameters. | Compiles with the correct signature including `std::shared_ptr<Material>` and `PrimitiveTopology topology = PrimitiveTopology::Triangles`. |
| AC-004 | `Model::draw(RenderDevice&) const` exists. For a non-indexed model, issues exactly one `device.draw(topology, ...)` with `vertex_count` vertices. For an indexed model, issues exactly one `device.draw_indexed(topology, ..., index_count, 0)`. The topology used is the one specified at model creation (default `Triangles`). | Headless backend test: create both types of model, call draw, verify no crash. Code review confirms correct draw/dispatch logic using the stored topology. |
| AC-005 | `Model::material()` returns a reference to the material. `Model::material()` const overload returns a const reference. | Unit test: get reference, set a uniform via reference, verify value persists. |
| AC-006 | `Model::vertices()` returns a const reference to the vertex buffer. | Unit test: `&model.vertices()` is a non-null pointer. |
| AC-007 | `Model::indices()` returns a const reference to the index buffer. Calling `indices()` on a non-indexed model is undefined behaviour. `Model::has_indices()` returns `true` for indexed models, `false` otherwise. | Unit tests verify both paths. |
| AC-008 | `Model::vertex_count()` returns the correct vertex count (data size / stride). `Model::index_count()` returns the correct index count for indexed models, 0 for non-indexed. | Unit tests verify counts match input data sizes. |
| AC-009 | `Model` is non-copyable and movable. | `static_assert(!std::is_copy_constructible_v<Model>)` passes. Moving a model transfers ownership; the source becomes a null model (draw is no-op). |
| AC-010 | `Model::create()` with a zero-length `vertex_data` span returns `Error::Category::InvalidArgument`. | Unit test (headless) verifies error result. |
| AC-011 | `Model::create_indexed()` with zero-length `index_data` returns `Error::Category::InvalidArgument`. | Unit test (headless) verifies error result. |
| AC-012 | `Model::create()` creates a `VertexBuffer` from the provided vertex data and converts the `const Material&` to `std::shared_ptr<Material>`. On failure of any step, the error is propagated and prior resources are not leaked. | Code review confirms the factory chains error results correctly (or uses RAII cleanup). ASAN-clean on failure path. |
| AC-013 | `Model::create_indexed()` additionally creates an `IndexBuffer` from provided index data and bundles it into the model. | Code review confirms. |
| AC-014 | `setup_cube(device)` creates a `Model` with 24 vertices (stride 24 bytes: Float3 position + Float3 color), 36 indices (`Uint16`), and a material with `u_mvp` uniform. | Unit test (headless): create the model, verify `model.vertex_count() == 24`, `model.has_indices() == true`, `model.index_count() == 36`. |
| AC-015 | `setup_cube(device)` uses shader sources with `a_position` (location 0) and `a_color` (location 1) vertex attributes, and a single `u_mvp` uniform of type `mat4`. No `u_color` uniform exists. | Code review confirms shader source strings match the spec. |
| AC-016 | The six face colours (red, green, blue, yellow, cyan, magenta) are encoded in vertex data at the correct vertex positions (vertices 0–3: red, 4–7: green, 8–11: blue, 12–15: yellow, 16–19: cyan, 20–23: magenta). | Code review confirms face colours match the spec at the correct vertex positions. Headless test can additionally verify by inspecting vertex buffer data. |
| AC-017 | The cube demo uses `Camera` at position (3, 2, 3) looking at the origin, with `Vec3::unit_y()` as up vector. Perspective: 60° FOV, aspect 800/600, near 0.1, far 100.0. The MVP is computed as `projection * view * model` where `model = Mat4::rotate(angle, Vec3::unit_y())`. | Code review confirms camera setup and MVP computation. |
| AC-018 | The cube demo loop runs for exactly 120 frames at ~60 FPS (16 ms per frame, with sleep). | Unit test (headless or OpenGL): `run_cube_demo` runs without crash. Frame count is verified via instrumentation or timing (or code review of the loop bound). |
| AC-019 | `buddd demo cube` is registered in `demo_command.cpp` usage text and dispatch. | Running `./buddd demo` shows `cube` in the available demos list. `./buddd demo cube` runs the cube demo. |
| AC-020 | `buddd demo triangle` continues to work unchanged after the cube demo is added. | `./buddd demo triangle` runs the triangle demo successfully. |
| AC-021 | A moved-from `Model` is in a null state where `draw()` is a no-op (no crash). | Unit test: create model, move to another variable, call draw on source — no crash, no draw call issued (verified via headless backend draw-count instrumentation or code review). |
| AC-022 | `Model::create()` with a vertex format that has zero stride returns `Error::Category::InvalidArgument`. | Unit test (headless) verifies error result. |
| AC-023 | `Model::create()` with a vertex format that has zero attributes returns `Error::Category::InvalidArgument`. | Unit test (headless) verifies error result. |
| AC-024 | Headless backend: after `setup_cube`, `model.material().has_uniform("u_mvp")` returns `true`. There is no `u_color` uniform — `has_uniform("u_color")` returns `false`. | Unit test (headless) verifies uniform existence. |
| AC-025 | Headless backend: after `setup_cube`, setting `u_mvp` on the material succeeds (no error). Setting `u_color` is not required (the uniform does not exist). | Unit test (headless) verifies `set_uniform("u_mvp", ...)` returns success. |
| AC-026 | The cube demo does not use any GLM, SDL3, or OpenGL headers in `src/cmd/demo/cube_demo.h` (public header). Only `src/cmd/demo/cube_demo.cpp` may include engine headers. | Code review confirms the `.h` file includes no backend-specific headers. |
| AC-027 | Cube face triangles use counter-clockwise (CCW) winding when viewed from outside. The index buffer data is arranged so that front-facing triangles use CCW order. | Code review of the index buffer data verifies CCW winding order. Headless test: draw the model (no crash). At runtime with OpenGL backend, the cube faces are visible (not back-face culled). |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An engine developer can render any indexed or non-indexed geometry using fewer than 15 lines of C++ (excluding shader source, material creation, and vertex data) by using `Model::create()`, setting uniforms, and calling `draw()`. | Count lines in a minimal rendering snippet using `Model`. |
| SC-002 | `buddd demo cube` produces visibly correct output on an OpenGL 4.5 Core backend — all six faces are distinguishable by colour and the cube rotates smoothly around Y. | Manual visual inspection. |
| SC-003 | All new tests pass on headless CI (no display, no GPU). | `cmake --build --preset debug && ctest --preset debug` — all Model and cube demo tests pass. |
| SC-004 | `Model` creation and destruction does not leak memory in any code path (success or failure). | ASAN build shows no leaks in a test that creates and destroys 100 `Model` instances, including failure-case paths. |
| SC-005 | The cube demo completes in under 3 seconds real time (120 frames × ~16 ms = ~1.92 s minimum + overhead). | Timing instrumentation: the wall-clock duration of `run_cube_demo` is less than 3 seconds on a typical development machine. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| `Model::create()` with empty vertex data (zero-length span) | Returns `Error::Category::InvalidArgument`. |
| `Model::create_indexed()` with empty vertex data | Returns `Error::Category::InvalidArgument`. |
| `Model::create_indexed()` with empty index data (zero-length span) | Returns `Error::Category::InvalidArgument`. |
| `Model::create()` with `VertexFormat` stride of 0 | Returns `Error::Category::InvalidArgument`. |
| `Model::create()` with `VertexFormat` having zero attributes | Returns `Error::Category::InvalidArgument`. |
| Vertex data size is not a multiple of stride | The factory still creates the buffer (the underlying `create_vertex_buffer` does not validate divisibility per SPEC-005). Behaviour is then undefined at draw time. |
| `Model::draw()` called on a moved-from (null) model | No-op — no crash, no draw call issued. |
| `Model::draw()` called outside `begin_frame()`/`end_frame()` | Undefined behaviour (same as SPEC-005). |
| `Model::draw()` called on a non-indexed model | Issues `device.draw(topology, *vb_, *material_, vertex_count_, 0)`. |
| `Model::draw()` called on an indexed model | Issues `device.draw_indexed(topology, *vb_, *ib_, *material_, index_count_, 0)`. |
| `Model::material()` called on a moved-from model | Undefined behaviour (reference to destroyed/empty state). |
| `Model::vertices()` called on a moved-from model | Undefined behaviour. |
| `Model::indices()` called on a non-indexed model | Undefined behaviour (caller should check `has_indices()` first). |
| Multiple `Model::draw()` calls with different uniform values between them | Each draw call reflects the current uniform state at the time of the call. Legal and expected. |
| Camera aspect ratio does not match window dimensions | The cube will appear stretched or squished. The demo assumes the 800×600 aspect ratio. This is a demo concern, not an engine concern. |
| User closes the window before 120 frames | The loop exits early, return value is `EXIT_SUCCESS`. |
| `setup_cube` called with a headless backend | Creates successfully with simulated resources. All uniforms are trackable. |
| Cube vertex data with non-unit size (not 2×2×2) | The model matrix does not include scale; the cube appears at whatever size the vertex data defines. The spec defines a unit cube (size 2). |
| Index buffer created with `Uint16` but index values exceed 65535 | Undefined behaviour in the OpenGL backend. For the cube demo, indices are in range 0–23, so `Uint16` is safe. |
| Two `Model` instances sharing the same `RenderDevice` and the same `Material` | Each model owns its own buffers. Both models share the material via `shared_ptr`. Legal and expected. |
| `Model::draw()` called after the `RenderDevice` that created it has been destroyed | Undefined behaviour — the GPU resources (buffers, material program) are no longer valid. The caller must ensure the `RenderDevice` outlives all `Model` instances created from it. |
| Material passed to `Model::create()` is a moved-from or default-constructed material | Undefined behaviour — the Material reference must point to a valid, linked material. |
| Material is destroyed while `Model` still holds a `shared_ptr` to it | The material stays alive until the last `shared_ptr` is destroyed (safe reference counting). |

## Error cases

| Case | Expected behaviour |
|---|---|
| `Model::create()` — vertex buffer creation fails | Returns `make_error(Error::Category::ResourceCreationFailed, ...)`. |
| `Model::create_indexed()` — vertex buffer creation fails | Returns `make_error(Error::Category::ResourceCreationFailed, ...)`. |
| `Model::create_indexed()` — index buffer creation fails | Returns `make_error(Error::Category::ResourceCreationFailed, ...)`. Prior resources are destroyed via RAII cleanup. |
| `setup_cube` — any shader/material/buffer creation step fails | Prints a FATAL message to stderr and calls `std::exit(EXIT_FAILURE)`. Consistent with `setup_triangle`. |
| `Model::create()` — vertex data is empty | Returns `make_error(Error::Category::InvalidArgument, "Vertex data is empty")`. |
| `Model::create_indexed()` — index data is empty | Returns `make_error(Error::Category::InvalidArgument, "Index data is empty")`. |
| `Model::create()` — `VertexFormat` stride is zero | Returns `make_error(Error::Category::InvalidArgument, "Vertex format stride must be positive")`. |
| `Model::create()` — `VertexFormat` has zero attributes | Returns `make_error(Error::Category::InvalidArgument, "Vertex format must have at least one attribute")`. |

## Permissions and security

- No elevated privileges required.
- No network access, secrets, or credentials involved.
- The `Model` class and cube demo operate entirely in user space with no I/O beyond window creation (already handled by `demo_command.cpp`).
- GLSL shader source for the cube demo is embedded in the demo code — no shader files are loaded from disk.
- The architecture boundary (CONST-001) is maintained: no code outside `src/engine/` includes OpenGL, SDL3, or GLM headers. `model.h` (in `src/engine/render/`) is part of the engine layer and may include backend headers internally in its `.cpp` file, but the public header exposes no backend types.
- The demo headers (`cube_demo.h`) must not expose backend types either.

## Observability

All observability uses `std::cerr`, consistent with the project pattern.

| Signal | Source |
|---|---|
| `Model` creation success | `std::cerr << "Model created (" << vertex_count << " vertices"; if (has_indices) { std::cerr << ", " << index_count << " indices"; } std::cerr << ")\n";` |
| `Model` creation failure (any step) | Error propagated to caller via `Result<Error>`. The caller (e.g., `setup_cube`) prints the error to `std::cerr` and exits. |
| Cube demo start | `std::cerr << "Demo started: cube (120 frames)\n"` (in `run_cube_demo`). |
| Cube demo end (normal) | `std::cerr << "Demo complete: cube (120 frames rendered)\n"` (in `run_cube_demo`). |
| Cube demo aborted by user | `std::cerr << "Demo aborted by user (frame " << frame << ")\n"` (in `run_cube_demo`). |
| Uniform set operations (debug builds only) | Per SPEC-005, the backend logs uniform sets in debug builds. |

## Out of scope

- Multi-material `Model` (v1 has a single material).
- Material arrays or per-submesh materials.
- `Model::set_vertex_buffer()`, `Model::set_index_buffer()`, or any mutation of geometry after construction.
- `Model::set_material()` or material replacement.
- Dynamic buffer updates (streaming, morphing).
- Bounding volumes, frustum culling, or ray intersection for the Model.
- Model loading from files (.obj, .gltf, .glb, etc.).
- Texture coordinates, normals, or tangents on the cube (position + colour vertices only).
- Lighting, shading, or PBR on the cube.
- Animation blending or interpolation — the rotation is a simple increasing angle.
- Camera movement, orbit controls, or any user interaction.
- Multiple cubes or instanced rendering.
- Wireframe or overlay rendering.
- Integration with the scene graph (`Entity`, `World`, `Transform`) — the cube stands alone.
- Cube edges or outlines — the cube faces are flat-shaded without visible edges.
- Per-face vertex normals — the cube uses flat shading with a single colour per face (no lighting), so normals are not needed.
- MSAA, anti-aliasing, or any post-processing.
- Frame pacing or variable refresh rate — fixed ~60 FPS with sleep.
- Shader compilation or material linking inside `Model` factories — those are the caller's responsibility.
- `Material::clone()` or polymorphic copying — the mechanism for converting a `const Material&` to `shared_ptr<Material>` is deliberately left open (see Q-01).
- Per-face uniform changes (`u_color`) — face colours are vertex attributes, not uniforms.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `Error::Category` enum already has `ResourceCreationFailed`, `InvalidArgument`, and `UniformNotFound` values (added in SPEC-005). No new error categories are needed for SPEC-009. |
| A-02 | The `RenderDevice` factory methods (`create_shader`, `create_material`, `create_vertex_buffer`, `create_index_buffer`) and draw methods are fully implemented and stable from SPEC-005. |
| A-03 | `Model` stores its GPU resources via `std::unique_ptr<VertexBuffer>`, `std::unique_ptr<IndexBuffer>` (optional), and `std::shared_ptr<Material>`. This enables cheap move semantics and shared material ownership. |
| A-04 | `Model` is a concrete class (not abstract, not a template). The factory methods are static and return `Result<Model>` by value (RVO/move-eligible). |
| A-05 | The `VertexFormat` for a position+colour vertex (Float3 + Float3) is `{stride=24, attributes=[{location=0, type=Float3, offset=0}, {location=1, type=Float3, offset=12}]}`. |
| A-06 | The cube demo's vertex/index data is defined directly in `setup_cube` using stack-allocated arrays. No dynamic allocation is needed for the geometry data before uploading to the GPU. |
| A-07 | The cube demo uses `IndexType::Uint16` because all 24 vertex indices fit in 16 bits. |
| A-08 | The cube is a unit cube (2×2×2, axis-aligned, centred at origin). Vertices range from -1 to +1 on each axis. |
| A-09 | The demo uses `std::chrono::steady_clock` for elapsed time computation, consistent with the triangle demo. |
| A-10 | The rotation speed is 0.5 radians per second around the Y axis (approximately 28.6 degrees per second — a visible but gentle rotation). |
| A-11 | A moved-from `Model` has all internal pointers set to `nullptr`. Calling `draw()` on a moved-from model is safe (no-op). Accessing material/vertices/indices on a moved-from model is undefined behaviour. |
| A-12 | The `Model` destructor does not need to be explicitly defined — the default destructor destroys each member in reverse declaration order, which correctly releases GPU resources and decrements the shared_ptr refcount. |
| A-13 | The `setup_cube` function follows the same error-handling pattern as `setup_triangle`: on any failure, print to stderr and `std::exit(EXIT_FAILURE)`. This is acceptable for demo code. |
| A-14 | The cube demo's `run_cube_demo` function signature matches the pattern established by `run_triangle_demo`: `(Platform&, RenderDevice&, int, const char* const*) -> int`. |
| A-15 | The `setup_cube` function is added to `demo_helpers.h`/`demo_helpers.cpp`. The files `src/cmd/demo/cube_demo.h` and `src/cmd/demo/cube_demo.cpp` are new files in the `src/cmd/demo/` directory, which is already compiled as part of the `buddd` executable. The CMake configuration picks up new files automatically. |
| A-16 | The `demo_command.cpp` validate-and-dispatch chain uses a simple if/else pattern. The registration adds `demo_name != "cube"` to the existing check (changing it from `demo_name != "triangle"` to a combined check) or adds a separate `else if` before the "unknown demo" fallback. |
| A-17 | The `Camera` type is in namespace `buddd::engine::math` and provides `set_perspective(fov_y_radians, aspect, near, far)` and `look_at(eye, center, up)` methods, with `Vec3::unit_y()` available as the Y-up convention. |
| A-18 | `CubeResources` is a simple aggregate struct with `std::shared_ptr<buddd::engine::Material> material` and `buddd::engine::Model model`. It requires no special constructors or methods. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | [RESOLVED] The factory takes `std::shared_ptr<Material>` directly, not `const Material&`. The caller converts their `unique_ptr<Material>` to `shared_ptr<Material>` via `shared_ptr<Material>(std::move(unique_material))` before passing it to `Model::create()` or `Model::create_indexed()`. This is the simplest, safest approach — no changes to the `Material` base class are needed. | **API signature.** The factory parameter type is `std::shared_ptr<Material>`. |
| Q-02 | [RESOLVED] No `--test` flag needed. The existing demo architecture (SPEC-007) uses `buddd demo <name>` for all demos, each with its own frame-count loop. The cube demo follows the same pattern. | Confirmed. Aligns with SPEC-007 demo architecture. |

(End of file)
