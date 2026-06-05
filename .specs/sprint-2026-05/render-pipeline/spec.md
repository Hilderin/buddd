# SPEC-005 — Render Pipeline (Shader, Material, VertexBuffer, IndexBuffer)

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume (user) |
| Date | 2026-05-29 |
| Time | ~23:30 UTC |

## Problem

The Buddd Engine's graphics layer currently supports only screen clearing and buffer swapping via `RenderDevice::begin_frame()` / `end_frame()`. No GPU programmability exists:

- **No shader compilation**: The engine cannot load or compile GLSL shaders at runtime, so any visual output must be hardcoded into the backend.
- **No material system**: Vertex and fragment shaders cannot be paired into a linked program; uniforms cannot be set from engine code.
- **No vertex or index buffers**: There is no abstraction for uploading vertex data (positions, colors, normals, UVs) to the GPU or for issuing draw calls.
- **No "first triangle"**: The minimal rendering milestone — a coloured triangle on screen — is unreachable.

Without these primitives, the engine cannot produce any meaningful graphical output, hindering the development of every downstream render feature (textures, lighting, post-processing, debug drawing).

## Goals

- **Shader abstraction**: Define an abstract `Shader` class representing a single compiled shader stage (vertex or fragment). Shaders are created from GLSL source strings (`const char*` or `std::string`) at runtime. A `ShaderType` enum distinguishes vertex vs. fragment stages.
- **Material abstraction**: Define an abstract `Material` class representing a linked shader program that combines one vertex shader and one fragment shader. Materials manage uniforms — providing a `set_uniform(name, value)` API for supported types (`float`, `int`, `math::Vec3`, `math::Vec4`, `math::Mat4`). A `has_uniform(name)` query tells callers whether a uniform exists in the linked program.
- **VertexBuffer abstraction**: Define an abstract `VertexBuffer` class representing a GPU buffer for vertex data. Vertex data is provided as a raw byte span at creation time. The vertex format (layout/stride/attributes) is configurable via a `VertexFormat` description struct.
- **IndexBuffer abstraction**: Define an abstract `IndexBuffer` class representing an optional GPU buffer for index data (16-bit or 32-bit indices). Drawing may be non-indexed or indexed.
- **Backend implementations**: Provide OpenGL 4.5 Core and Headless backends for all four abstractions, following the established pattern from `RenderDeviceOpenGL` / `RenderDeviceHeadless`. The `RenderDevice` abstract class gains factory methods (`create_shader`, `create_material`, `create_vertex_buffer`, `create_index_buffer`) and drawing methods (`draw`, `draw_indexed`).
- **Primitive topology**: Define a `PrimitiveTopology` enum (`Triangles`, `TriangleStrip`, `Lines`, `LineStrip`, `Points`) to control the draw mode.
- **First triangle**: After implementation, an application developer can render a coloured triangle on screen using engine abstractions — no hardcoded backend code required.

## Non-goals

- No texture loading, creation, binding, or sampling.
- No lighting, shading models, or PBR.
- No model loading or mesh import (`.obj`, `.gltf`, etc.).
- No scene graph, transform hierarchy, or entity system.
- No compute shaders, geometry shaders, or tessellation shaders.
- No persistent mapping or streaming of vertex/index buffer data.
- No secondary command buffers, multi-draw indirect, or instanced rendering.
- No uniform buffer objects (UBO), shader storage buffer objects (SSBO), or push constants.
- No separate `Texture` abstraction (that is a future spec).
- No framebuffer objects (FBO), render targets, or render-to-texture.
- No dynamic backend switching after resource creation.
- No serialisation or file format for shaders/materials/buffers (everything is created at runtime from source/data in memory).
- No debug markers or GPU profiling annotations.
- No caching or deduplication of compiled shaders or materials.
- No multi-window rendering.

## Actors

| Actor | Description |
|---|---|
| Engine developer | A developer adding features to the Buddd Engine who needs to create shaders, materials, and vertex buffers; depends on the abstractions, never on OpenGL/GLSL directly outside `src/engine/`. |
| Application developer | A developer building a game or tool on top of the engine. Creates shader source, configures vertex layouts, and issues draw calls using only abstract interfaces. |
| Build system | CMake + Ninja — new `.h` and `.cpp` files in `src/engine/render/` are picked up automatically by the existing `file(GLOB_RECURSE)`. |
| Test suite | Catch2 v3 tests that exercise all abstractions in headless mode (no GPU required) and conditionally in OpenGL mode with the offscreen driver. |

## User-visible behavior

### Shader creation

- A `ShaderType` enum exists with values `Vertex` and `Fragment`.
- The abstract `Shader` class is created via `RenderDevice::create_shader(ShaderType type, std::string_view source)`.
- The factory returns `Result<std::unique_ptr<Shader>>`. On success, the shader is compiled and ready to be attached to a `Material`.
- Shader source is GLSL (`#version 450 core` assumed). The caller provides the full source string.

### Material creation

- The abstract `Material` class is created via `RenderDevice::create_material(std::unique_ptr<Shader> vertex_shader, std::unique_ptr<Shader> fragment_shader)`.
- Ownership of the `Shader` objects is transferred to the `Material`. The material links the two shaders into a single program.
- The factory returns `Result<std::unique_ptr<Material>>`. On success, the material is linked and ready for use.
- Materials provide a `set_uniform(name, value)` API for setting uniform values:
  - `auto set_uniform(std::string_view name, float value) -> Result<void>`
  - `auto set_uniform(std::string_view name, int32_t value) -> Result<void>`
  - `auto set_uniform(std::string_view name, bool value) -> Result<void>`
  - `auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void>`
  - `auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void>`
  - `auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void>`
- Setting a uniform that does not exist in the linked program returns an error (not a crash).
- Materials provide a `has_uniform(std::string_view name) -> bool` query to check uniform existence.

### VertexFormat

- A `VertexFormat` struct defines the layout of a single vertex:
  - `uint32_t stride` — byte size of one vertex.
  - A list of `VertexAttribute` entries, each describing one attribute:
    - `uint32_t location` — attribute location index (maps to `layout(location = N)` in GLSL).
    - `VertexAttributeType type` — the data type (`Float`, `Float2`, `Float3`, `Float4`, `Int`, `Int2`, `Int3`, `Int4`, `UByte`, `UByte4`, `UByte4Norm`).
    - `uint32_t offset` — byte offset of this attribute within the vertex.
    - `bool normalized` — whether integer types are normalised (default `false`).
- `VertexFormat` is a plain data type (no virtual methods, no backend-specific behaviour).

### VertexBuffer creation

- The abstract `VertexBuffer` class is created via `RenderDevice::create_vertex_buffer(const VertexFormat& format, std::span<const std::byte> data)`.
- The factory returns `Result<std::unique_ptr<VertexBuffer>>`. The vertex format is fixed at creation time.
- The caller provides raw byte data; the implementation uploads it to the GPU (or stores it in memory for the headless backend).

### IndexBuffer creation

- The abstract `IndexBuffer` class is created via `RenderDevice::create_index_buffer(std::span<const std::byte> data, IndexType type)`.
- `IndexType` is an enum with values `Uint16` and `Uint32`.
- The factory returns `Result<std::unique_ptr<IndexBuffer>>`.

### Drawing

- `RenderDevice` gains:
  - `auto draw(PrimitiveTopology topology, const VertexBuffer& vertices, const Material& material, uint32_t vertex_count, uint32_t start_vertex = 0) -> void`
  - `auto draw_indexed(PrimitiveTopology topology, const VertexBuffer& vertices, const IndexBuffer& indices, const Material& material, uint32_t index_count, uint32_t start_index = 0) -> void`
- `begin_frame()` must be called before any draw call, and `end_frame()` after all draw calls for the frame.
- The draw call binds the material (uses the linked shader program), sets the vertex buffer (binds the VAO/VBO and configures vertex attributes), and issues the draw command.
- `draw()` and `draw_indexed()` return `void` (not `Result<void>`) because draw calls are on a performance-sensitive hot path where per-frame error checking is impractical. Precondition violations (invalid topology, out-of-bounds vertex access, unlinked material) are **undefined behaviour** — the caller must ensure correct state before drawing. This is a deliberate exception to ADR-001's `Result<T>` convention, consistent with real-time graphics API conventions.
- `PrimitiveTopology` is an enum with values: `Triangles`, `TriangleStrip`, `Lines`, `LineStrip`, `Points`.

### Event polling

To support interactive rendering without exposing SDL3 outside `src/engine/`, the abstract `Platform` class gains:
- `auto poll_events() -> bool` — polls the platform event queue. Returns `false` if the user requested to quit (e.g., window close button), `true` otherwise. In headless mode, always returns `true`. This is intentionally minimal — it only communicates a quit signal. Full event data (keyboard, mouse, etc.) is a future feature.

### Backend behaviour

- **OpenGL 4.5 Core backend**: Compiles GLSL shaders, links programs via `glCreateProgram` / `glAttachShader` / `glLinkProgram`, manages uniforms via `glGetUniformLocation` / `glUniform*`, creates and manages VAOs, VBOs, and IBOs via `glCreateVertexArrays`, `glNamedBufferStorage`, `glVertexArrayAttribFormat`, etc.
- **Headless backend**: Stores shader source, uniform state, and vertex data in memory. All operations succeed silently. The draw calls are no-ops (no GPU interaction). Uniform state is tracked so that `has_uniform` and `set_uniform` work correctly against stored uniform locations (or a simulated location cache).
- **Headless linking error simulation**: The headless backend simulates compilation errors when shader source contains the marker string `#error` anywhere in the source. It simulates linking errors when the vertex shader output variable names (declared after `out ` or `layout(...) out ` up to the first semicolon or comma) and fragment shader input variable names (declared after `in ` or `layout(...) in ` up to the first semicolon or comma) have no overlap (i.e., no output from the vertex stage feeds any input of the fragment stage). This enables AC-022 to be tested in headless mode without a GPU.
- For the OpenGL backend, the rendering context (OpenGL 4.5 Core) is already created by `RenderDeviceOpenGL` during initialisation — the new types reuse that context.

## User stories

### Story 1 — Compile and combine shaders into a material (Priority: P1)

As an engine developer, I want to create vertex and fragment shaders from GLSL source strings and combine them into a linked material, so that I can use custom shader programs for rendering.

**Given** a valid OpenGL 4.5 Core backend (or headless backend) with an initialised `RenderDevice`

**When** I call:
```
auto vs = device->create_shader(ShaderType::Vertex, vertex_source);
auto fs = device->create_shader(ShaderType::Fragment, fragment_source);
auto mat = device->create_material(std::move(*vs), std::move(*fs));
```

**Then** both shaders compile successfully, the material links without error, and the material is ready for drawing.

**Given** the headless backend

**When** I call the same sequence

**Then** all steps succeed with no OpenGL or SDL3 involvement.

### Story 2 — Set uniforms on a material (Priority: P1)

As an engine developer, I want to set uniform values of various types on a material, so that I can pass data (transforms, colours, time, boolean flags) to shaders.

**Given** a valid `Material` instance with known uniforms (e.g., `"u_color"`, `"u_model"`, `"u_time"`, `"u_flag"`)

**When** I call:
```
mat->set_uniform("u_color", math::Vec4{1.0f, 0.0f, 0.0f, 1.0f});
mat->set_uniform("u_time", 1.5f);
mat->set_uniform("u_model", math::Mat4::identity());
mat->set_uniform("u_flag", true);
```

**Then** each call succeeds without error.

**Given** a material that does **not** have a uniform named `"u_nonexistent"`

**When** I call `mat->set_uniform("u_nonexistent", 0.0f)`

**Then** the call returns an error (e.g., `Error::Category::Unsupported` or a new category like `UniformNotFound` with a message indicating the uniform name).

### Story 3 — Upload vertex data and draw a triangle (Priority: P1)

As an application developer, I want to create a vertex buffer with position and colour data, create a matching material, and issue a draw call to render a triangle on screen.

**Given** a running engine with an OpenGL backend, an active window, and a render device

**When** I define a vertex format with position (Float3) and colour (Float3) attributes, upload three vertices, create a material with a minimal passthrough vertex shader and a flat-colour fragment shader, then call `device->draw(PrimitiveTopology::Triangles, *vb, *mat, 3)` between `begin_frame()` and `end_frame()`

**Then** a coloured triangle is rendered on screen (visually verifiable).

**Given** the headless backend

**When** I issue the same draw call sequence

**Then** no crash occurs, no GPU interaction happens, and the draw call completes silently.

### Story 4 — Indexed drawing (Priority: P2)

As an application developer, I want to draw geometry using an index buffer, so that I can reuse vertices across triangles (e.g., for a quad or a cube).

**Given** a vertex buffer with 4 vertices (a quad) and an index buffer with 6 indices (two triangles)

**When** I call `device->draw_indexed(PrimitiveTopology::Triangles, *vb, *ib, *mat, 6)`

**Then** the indexed draw call succeeds and renders the expected geometry (OpenGL backend: visual; headless: no-op).

### Story 5 — Query uniform existence (Priority: P2)

As an engine developer, I want to check whether a uniform exists in a material before attempting to set it, so that I can handle optional uniforms gracefully (e.g., when composing effects).

**Given** a material with known uniforms

**When** I call `mat->has_uniform("u_color")` and `mat->has_uniform("u_nonexistent")`

**Then** `has_uniform("u_color")` returns `true` and `has_uniform("u_nonexistent")` returns `false`.

### Story 6 — Error propagation on shader compilation failure (Priority: P1)

As an engine developer, I want to receive a meaningful error (with shader compilation log) when shader source is invalid, so that I can diagnose and fix GLSL errors quickly.

**Given** a `RenderDevice` instance

**When** I call `device->create_shader(ShaderType::Vertex, "invalid glsl source")`

**Then** the factory returns an error result with `Error::Category::Unsupported` (or a new category such as `ShaderCompilationFailed`) and a human-readable message containing the GLSL compiler error log.

### Story 7 — Default mode: interactive window (Priority: P1)

As an end user, I want to run `buddd` without arguments and see a window with a coloured triangle that stays open until I close it, so that I can visually inspect the render output.

**Given** a built `buddd` executable on a system with a display

**When** I run `./buddd`

**Then** a window opens, a coloured triangle is rendered every frame, and the window stays open (rendering continuously) until the user closes it (via the window's close button or Alt+F4), at which point the process exits with code 0.

### Story 8 — Test mode: automated 120-frame run (Priority: P1)

As an engine developer, I want to run `./buddd --test` to render a coloured triangle for exactly 120 frames and exit automatically, so that I can quickly verify the render pipeline works end-to-end without manual interaction.

**Given** a built `buddd` executable on a system with a display

**When** I run `./buddd --test`

**Then** a window opens, a coloured triangle is rendered for exactly 120 frames (approx. 2 seconds at 60 FPS), the window closes, and the process exits with code 0.

**Given** a built `buddd` executable on a headless system (no display)

**When** I run `./buddd --test`

**Then** the process prints an appropriate error message and exits with a non-zero exit code (the SDL3 backend cannot initialise without a display).

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | A `ShaderType` enum exists in namespace `buddd::engine` with values `Vertex` and `Fragment`. | File compiles; `ShaderType::Vertex` and `ShaderType::Fragment` are valid identifiers. |
| AC-002 | An abstract `Shader` class exists in `src/engine/render/shader.h` in namespace `buddd::engine` with a virtual destructor, non-copyable, non-movable. `ShaderType` is accessible from the header. | File compiles; class is abstract with at least one pure virtual method; destructor is `virtual`; copy/move operations are deleted. |
| AC-003 | An abstract `Material` class exists in `src/engine/render/material.h` in namespace `buddd::engine` with a virtual destructor, non-copyable, non-movable, and `set_uniform` overloads for `float`, `int32_t`, `bool`, `math::Vec3`, `math::Vec4`, `math::Mat4`, each returning `Result<void>`. A `has_uniform(std::string_view)` method returns `bool`. | File compiles; all five `set_uniform` overloads exist with correct signatures; `has_uniform` exists. |
| AC-004 | A `VertexAttributeType` enum exists with values `Float`, `Float2`, `Float3`, `Float4`, `Int`, `Int2`, `Int3`, `Int4`, `UByte`, `UByte4`, `UByte4Norm`. | File compiles; all values are valid identifiers. |
| AC-005 | A `VertexAttribute` struct exists with `uint32_t location`, `VertexAttributeType type`, `uint32_t offset`, and `bool normalized` (default `false`). | File compiles; struct has the described fields with correct types and default. |
| AC-006 | A `VertexFormat` struct exists with `uint32_t stride` and a container of `VertexAttribute` entries. | File compiles; `stride` is `uint32_t`; attributes are accessible. |
| AC-007 | An abstract `VertexBuffer` class exists in `src/engine/render/vertex_buffer.h` in namespace `buddd::engine` with a virtual destructor, non-copyable, non-movable. | File compiles; class is abstract; destructor is `virtual`; copy/move operations are deleted. |
| AC-008 | An `IndexType` enum exists with values `Uint16` and `Uint32`. | File compiles; `IndexType::Uint16` and `IndexType::Uint32` are valid identifiers. |
| AC-009 | An abstract `IndexBuffer` class exists in `src/engine/render/index_buffer.h` in namespace `buddd::engine` with a virtual destructor, non-copyable, non-movable. | File compiles; class is abstract; destructor is `virtual`; copy/move operations are deleted. |
| AC-010 | A `PrimitiveTopology` enum exists with values `Triangles`, `TriangleStrip`, `Lines`, `LineStrip`, `Points`. | File compiles; all values are valid identifiers. |
| AC-011 | `RenderDevice` gains pure virtual factory methods: `create_shader(ShaderType, std::string_view) -> Result<std::unique_ptr<Shader>>`, `create_material(std::unique_ptr<Shader>, std::unique_ptr<Shader>) -> Result<std::unique_ptr<Material>>`, `create_vertex_buffer(const VertexFormat&, std::span<const std::byte>) -> Result<std::unique_ptr<VertexBuffer>>`, `create_index_buffer(std::span<const std::byte>, IndexType) -> Result<std::unique_ptr<IndexBuffer>>`. | `render_device.h` compiles with the new methods; all four signatures match the spec. |
| AC-012 | `RenderDevice` gains pure virtual draw methods: `draw(PrimitiveTopology, const VertexBuffer&, const Material&, uint32_t, uint32_t) -> void` and `draw_indexed(PrimitiveTopology, const VertexBuffer&, const IndexBuffer&, const Material&, uint32_t, uint32_t) -> void`. Default arguments: `start_vertex = 0`, `start_index = 0`. | `render_device.h` compiles with the two draw methods. |
| AC-013 | Concrete `ShaderOpenGL` and `ShaderHeadless` classes exist, implementing `Shader`. | Compilation succeeds; `RenderDeviceOpenGL::create_shader()` returns a `ShaderOpenGL` instance; `RenderDeviceHeadless::create_shader()` returns a `ShaderHeadless` instance. |
| AC-014 | Concrete `MaterialOpenGL` and `MaterialHeadless` classes exist, implementing `Material`. | Compilation succeeds; `RenderDeviceOpenGL::create_material()` returns a `MaterialOpenGL` instance; `RenderDeviceHeadless::create_material()` returns a `MaterialHeadless` instance. |
| AC-015 | Concrete `VertexBufferOpenGL` and `VertexBufferHeadless` classes exist, implementing `VertexBuffer`. | Compilation succeeds; `RenderDeviceOpenGL::create_vertex_buffer()` returns a `VertexBufferOpenGL` instance; `RenderDeviceHeadless::create_vertex_buffer()` returns a `VertexBufferHeadless` instance. |
| AC-016 | Concrete `IndexBufferOpenGL` and `IndexBufferHeadless` classes exist, implementing `IndexBuffer`. | Compilation succeeds; `RenderDeviceOpenGL::create_index_buffer()` returns a `IndexBufferOpenGL` instance; `RenderDeviceHeadless::create_index_buffer()` returns a `IndexBufferHeadless` instance. |
| AC-017 | OpenGL backends use OpenGL 4.5 Core profile (DSA, `glCreateShader`, `glCreateProgram`, `glCreateVertexArrays`, `glNamedBufferStorage` or equivalent). | Code review confirms no deprecated OpenGL functions are used. |
| AC-018 | Headless backends do not include `<SDL3/` or `<GL/` headers, and do not call any SDL3 or OpenGL functions. | Grep of headless source files returns no SDL3 or GL includes or function calls. |
| AC-019 | `Material::set_uniform` with an unknown uniform name returns an error result (not a crash). | Unit test (headless) verifies that `set_uniform("nonexistent", 0.0f)` returns `Error` with appropriate category. |
| AC-020 | `Material::has_uniform` returns `false` for unknown uniform names and `true` for known ones (headless backend: simulated based on the set of names provided at material creation or discovered during linking). | Unit test (headless) verifies both cases. |
| AC-021 | Invalid shader source returns an error from `create_shader` with a message containing the compilation error (OpenGL backend) or a simulated error (headless backend). | Unit test verifies error is returned; message is non-empty. |
| AC-022 | Shader linking failure (e.g., mismatched stage inputs/outputs) returns an error from `create_material` with a message containing the link error. | Unit test (headless or OpenGL with offscreen) verifies error is returned for incompatible shaders. Headless backend simulates linking failure using the source-marker convention (see User-visible behavior > Backend behaviour). |
| AC-023 | `create_vertex_buffer` with empty data (zero-length span) returns an error. | Unit test verifies error result for empty vertex data. |
| AC-024 | `create_index_buffer` with empty data returns an error. | Unit test verifies error result for empty index data. |
| AC-025 | All four abstract classes (`Shader`, `Material`, `VertexBuffer`, `IndexBuffer`) are non-copyable and non-movable. | `static_assert(!std::is_copy_constructible_v<Shader>)` passes; move operations are deleted. |
| AC-026 | A complete "first triangle" demo compiles and runs under the OpenGL backend — renderer produces visible output. | Manual visual verification: a coloured triangle appears in the window. Also verifiable via automated test using the offscreen driver (pixel readback is out of scope; this criterion is visual/manual). |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An engine or application developer can create a material from two GLSL source strings and draw a triangle using fewer than 50 lines of C++ (excluding shader source strings and boilerplate). | Count lines in a minimal demo program that creates `RenderDevice`, creates shaders and material, uploads vertex data, and draws. |
| SC-002 | All new abstractions are testable in headless mode without a GPU or display server, achieving the same testability bar as `RenderDevice`. | `ctest --preset debug` on a headless CI runner passes all render-pipeline tests (headless backend). |
| SC-003 | The four abstract interface classes expose exactly zero external library types in their public headers (no `GL*`, `SDL_*`, or other backend types). | `grep -E '(SDL_|gl[A-Z]|GL_|GLAD)' src/engine/render/shader.h src/engine/render/material.h src/engine/render/vertex_buffer.h src/engine/render/index_buffer.h src/engine/render/vertex_format.h` returns no matches. |
| SC-004 | Headless backends add no SDL3 or OpenGL link-time dependency to `buddd_engine`. | Build succeeds with `BUDDD_HAS_DISPLAY=OFF`; headless-only tests pass. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| Shader compiled from empty source string | `create_shader` returns an error (e.g., `Error::Category::Unsupported` or `ShaderCompilationFailed`) with a descriptive message. |
| Material created with mismatched vertex/fragment shaders (e.g., output variable missing from fragment input) | `create_material` returns an error with the linker error log. |
| Material created with a null or moved-from shader (after ownership transfer) | The factory takes `unique_ptr` by value; a null `unique_ptr` results in an error (e.g., `Error::Category::InvalidArgument` or `Unsupported`). This is a precondition: the caller should not pass a null pointer. |
| `set_uniform` called with a name that is an active uniform but with the wrong type (e.g., setting a `float` on a `sampler2D` uniform) | Implementation-defined at the backend level. The OpenGL backend may silently succeed (OpenGL allows this) or produce unexpected rendering. The spec does not mandate type checking at this stage. |
| `set_uniform` called on a material that was not linked successfully | Precondition violation (material creation already failed). Calling methods on a material that was never successfully created is undefined behaviour. |
| `draw` or `draw_indexed` called outside a `begin_frame()`/`end_frame()` pair | Undefined behaviour at the abstract level. The OpenGL backend may produce GL errors or crash. The headless backend silently succeeds. |
| `draw` called with `vertex_count = 0` | No-op (nothing is drawn). No error is returned. |
| `draw` called with `start_vertex` beyond the vertex buffer's vertex count | Undefined behaviour. The OpenGL backend will read out-of-bounds GPU memory (may produce garbage or crash depending on GL implementation). The headless backend silently succeeds. |
| `draw_indexed` with index values that exceed the vertex count | Undefined behaviour (same as above — out-of-bounds vertex read). |
| `IndexType::Uint16` but index data size is not a multiple of 2 | The `create_index_buffer` factory may either return an error or round up; the spec does not mandate validation. Undefined if data is misaligned. |
| `IndexType::Uint32` but index data size is not a multiple of 4 | Same as above. |
| Multiple draw calls with the same vertex buffer but different materials | Each draw call binds the material's shader program before issuing the draw command. Legal and expected usage. |
| `VertexFormat` with zero attributes | `create_vertex_buffer` returns an error (a vertex buffer with no attributes is not useful). |
| `VertexFormat` with a stride of zero | `create_vertex_buffer` returns an error (stride must be positive). |
| Multiple vertex buffers created from the same device | Each buffer is independent; they can be used in different draw calls. Legal and expected usage. |
| Material used across multiple frames (uniforms set once, draw every frame) | Legal and expected usage. Uniform values persist across frames until changed. |
| Headless backend: querying `has_uniform` after material creation | Returns `true` only for names that were either discovered during the simulated "linking" or explicitly registered. The headless backend must accept a uniform name list at construction (e.g., via a stub parameter) or always return `false` for all names except those that have been successfully `set_uniform`-ed. The chosen behaviour is documented in assumptions. |
| Headless backend: `draw`/`draw_indexed` with `PrimitiveTopology` values | No-op; no validation of the topology value is required. |

## Error cases

| Case | Expected behaviour |
|---|---|
| Shader compilation fails (invalid GLSL) | `create_shader` returns `make_error(Error::Category::Unsupported, glsl_compiler_log)` (or equivalent). |
| Shader linking fails (incompatible stages) | `create_material` returns `make_error(Error::Category::Unsupported, linker_error_log)`. |
| Null/empty shader source | `create_shader` returns `make_error(Error::Category::InvalidArgument, ...)` or equivalent. |
| Null/moved-from shader passed to `create_material` | `create_material` returns `make_error(Error::Category::InvalidArgument, ...)`. |
| Empty vertex data (zero span) | `create_vertex_buffer` returns `make_error(Error::Category::InvalidArgument, "Vertex data is empty")`. |
| Empty index data (zero span) | `create_index_buffer` returns `make_error(Error::Category::InvalidArgument, "Index data is empty")`. |
| VertexFormat stride is zero | `create_vertex_buffer` returns `make_error(Error::Category::InvalidArgument, "Vertex format stride must be positive")`. |
| VertexFormat has zero attributes | `create_vertex_buffer` returns `make_error(Error::Category::InvalidArgument, "Vertex format must have at least one attribute")`. |
| Setting a non-existent uniform | `set_uniform` returns `make_error(Error::Category::Unsupported, "Uniform 'name' not found")`. |
| Setting a uniform with a value type that does not match the GLSL type | **Caller responsibility** — no type checking is performed by the engine. The OpenGL backend may silently succeed (OpenGL allows this) or produce unexpected rendering. The spec explicitly does not mandate type checking (see Q-01). |
| OpenGL context lost during resource creation | `create_shader` / `create_material` / `create_vertex_buffer` / `create_index_buffer` returns `RenderDeviceCreationFailed` or a new category such as `ResourceCreationFailed`. The implementation should detect GL errors and propagate them. |
| `Error::Category` extensions | New categories may be added as needed (e.g., `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`). The `Error::Category` enum is extensible. |

## Permissions and security

- No elevated privileges are required to create shaders, materials, or buffers.
- Shader source is provided at runtime by the application — the engine does not load shader files from disk.
- No network access, secrets, or credentials are involved in the render pipeline.
- The headless backend requires no GPU or display access, maintaining CI safety.
- The architecture boundary (CONST-001) remains in full effect: no code outside `src/engine/` may include OpenGL, SDL3, or GLM headers. All new abstract headers (`shader.h`, `material.h`, `vertex_buffer.h`, `index_buffer.h`, `vertex_format.h`) expose no backend types.
- OpenGL backend implementations live inside `src/engine/render/` and are invisible to external consumers.
- GLSL shader compilation is inherently unsafe at the driver level (malicious shaders could crash the GPU driver or, in extreme cases, cause system instability). The engine does not validate shader source beyond what the GLSL compiler provides. Application developers are responsible for the shader source they provide.

## Observability

All observability uses `std::cerr` directly, consistent with SPEC-002.

| Signal | Source |
|---|---|
| Shader creation success/failure | `std::cerr << "Shader created (type=" << type << ")\n"` or `std::cerr << "Shader compilation failed: " << log << "\n"` |
| Material creation success/failure | `std::cerr << "Material created (" << vs_name << " + " << fs_name << ")\n"` or similar on `create_material()` |
| Vertex/index buffer creation | `std::cerr << "Vertex buffer created (" << vertex_count << " vertices)\n"` or similar |
| Draw call count per frame (debug builds only) | `std::cerr << "Draw: " << topology << " " << vertex_count << " vertices\n"` after each draw call in debug builds |
| Uniform set operations (debug builds only) | `std::cerr << "Uniform set: " << name << " (type=" << type << ")\n"` in debug builds |
| Backend-specific OpenGL errors | `glGetError` check after GL operations in debug builds; output via `std::cerr` |
| Unsupported uniform set (not found) | `std::cerr << "Uniform not found: " << name << "\n"` on error return from `set_uniform` |

## Out of scope

- Textures, samplers, and image uniforms.
- Lighting, shading models, PBR, or any fixed-function emulation.
- Model or mesh file loading (`.obj`, `.gltf`, `.glb`).
- Scene graph, transform hierarchy, entity-component-system integration.
- Compute shaders, geometry shaders, tessellation shaders, or any non-vertex/fragment stages.
- Shader reflection — uniform discovery is manual (the developer knows their uniform names).
- Persistent mapping, streaming, or dynamic updates of vertex/index buffer data after creation (the initial data upload is the only supported path).
- Instanced rendering or multi-draw indirect.
- Uniform buffer objects (UBO), shader storage buffer objects (SSBO), or push constants.
- Framebuffer objects, render targets, or render-to-texture.
- Debug markers, GPU queries, or timestamp queries.
- Shader/mesh/material caching or asset management.
- Multiple windows or swap chains.
- Pixel readback (glReadPixels) or GPU-to-CPU data transfer.
- Synchronisation primitives (fences, barriers).
- Thread-safe resource creation or concurrent rendering.
- Build-system changes beyond adding new `.h` / `.cpp` files (the existing `file(GLOB_RECURSE)` in `src/engine/CMakeLists.txt` automatically picks them up).

Note: The `Error::Category` enum (`src/engine/error.h`) may need new values for render-pipeline errors (e.g., `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`). Adding categories is explicitly allowed per ADR-001 ("The `Error::Category` enum is extensible — new categories are added as new subsystems are introduced.").

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The project uses OpenGL 4.5 Core profile exclusively for the GL backend. All new OpenGL code uses DSA (Direct State Access) APIs such as `glCreateShader`, `glCreateProgram`, `glCreateVertexArrays`, `glVertexArrayAttribFormat`, `glVertexArrayVertexBuffer`, `glNamedBufferStorage`, and `glNamedBufferSubData`. No deprecated APIs (`glBegin`/`glEnd`, `glVertexAttribPointer` with bound VAO, etc.) are used. |
| A-02 | GLSL version is `#version 450 core`. The engine does not inject or modify the version string. |
| A-03 | All shader source strings are null-terminated (`const char*` or `std::string_view` data is null-terminated). The implementation passes the source directly to `glShaderSource`. |
| A-04 | The `Result<void>` pattern is used for `set_uniform`. This requires either `std::expected<void, Error>` (valid in C++23/C++26) or a wrapper type. The project's C++26 toolchain supports this. |
| A-05 | The `Error::Category` enum is extended with new values as needed (e.g., `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`). These are additive and do not break existing code. |
| A-06 | The headless backend for `Material` does not perform actual GLSL linking. It stores the uniform names from discovery (either via a simulated uniform table or by accepting a list of known uniform names at construction). `has_uniform` returns `true` for names in this table, and `set_uniform` stores the value in a map (so the uniform state is tracked without a GPU program). |
| A-07 | The headless backend for `VertexBuffer` and `IndexBuffer` stores the data in a `std::vector<std::byte>` in memory. No GPU upload occurs. The `VertexFormat` is stored for inspection. |
| A-08 | A new `Error::Category` value `InvalidArgument` is added for precondition violations (empty data, zero stride, etc.), consistent with other engine APIs. |
| A-09 | `PrimitiveTopology`, `VertexAttributeType`, `VertexAttribute`, `VertexFormat`, `ShaderType`, and `IndexType` are value types (structs/enums) with no virtual methods. They live in their own header(s) or are defined inline in the relevant abstract class headers. |
| A-10 | The `start_vertex` and `start_index` default arguments are `0`. For the OpenGL backend, `start_vertex` maps to the `first` parameter of `glDrawArrays`/`glDrawElements`, and `start_index` maps to the `indices` pointer offset (computed as `start_index * index_byte_size`). |
| A-11 | `end_frame()` calls `SDL_GL_SwapWindow()` (OpenGL backend) or is a no-op (headless backend). No implicit draw flush or finish is added beyond what `SwapWindow` provides. The draw commands themselves are executed when issued (immediate mode, no deferred command buffer). |
| A-12 | The engine uses a single VAO for each `VertexBuffer` in the OpenGL backend. The VAO is created at buffer creation time and configured with the vertex format's attributes via `glVertexArrayAttribFormat`, `glVertexArrayAttribBinding`, and `glEnableVertexArrayAttrib`. When a draw call uses a `VertexBuffer`, the corresponding VAO is bound. |
| A-13 | The `RenderDevice` is the sole factory for all render resources — it knows which backend is active and instantiates the correct concrete type. No separate factory class or builder is needed. |
| A-14 | The `Material` takes ownership of the `Shader` objects via `unique_ptr`. If material creation fails (e.g., linking error), the shader objects are still owned by the material parameter and are destroyed when the `unique_ptr` goes out of scope at the call site (the caller retains no ownership). This means material creation failure also discards the shader objects. If shader reuse across materials is needed later, the API can be extended (e.g., shared_ptr or explicit shader cloning). |
| A-15 | The `RenderDeviceOpenGL::create_material` implementation calls `glAttachShader` and `glLinkProgram`. After linking, the shader objects are marked for deletion with `glDeleteShader` (the program will detach them when it is deleted). This is the standard OpenGL pattern: the program holds a reference to the compiled shader code even after `glDeleteShader`. |
| A-16 | The `RenderDeviceOpenGL::create_vertex_buffer` implementation creates a VAO (via `glCreateVertexArrays`), a VBO (via `glCreateBuffers`), uploads data (via `glNamedBufferStorage` with `GL_DYNAMIC_DRAW` or `GL_STATIC_DRAW`), and configures vertex attributes (via `glVertexArrayAttribFormat`, `glVertexArrayVertexBuffer`, `glVertexArrayAttribBinding`, `glEnableVertexArrayAttrib`). The VAO is stored in the `VertexBufferOpenGL` instance. |
| A-17 | The `RenderDeviceOpenGL::create_index_buffer` implementation creates an IBO (via `glCreateBuffers`) and uploads index data (via `glNamedBufferStorage`). |
| A-18 | For the OpenGL backend, `draw()` binds the material's program and the vertex buffer's VAO, then calls `glDrawArrays`. `draw_indexed()` additionally binds the index buffer and calls `glDrawElements`. |
| A-19 | The `draw` methods on `RenderDevice` are immediate — they execute draw commands synchronously on the calling thread. No command queue or deferred rendering is introduced. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | [RESOLVED] Uniform type checking — the engine does NOT attempt type checking. Setting a uniform with the wrong GLSL type (e.g., calling `set_uniform(name, float)` on a `vec4` uniform) is documented as "caller responsibility" / undefined behaviour. This is consistent with the non-goal of no shader reflection and keeps the API simple. | **Resolution**: No type checking. Added to edge cases. |
| Q-02 | [RESOLVED] The headless `Material` backend accepts an optional `HeadlessMaterialConfig` with a list of known uniform names (`std::vector<std::string>`) at construction. `has_uniform` returns `true` for names in this list OR names that have been previously set via `set_uniform`. This gives callers full control over test scenarios. The factory signature becomes `create_material(unique_ptr<Shader>, unique_ptr<Shader>, config = {})` where config is an implementation detail of the headless backend (the OpenGL backend ignores it). | **Resolution**: `HeadlessMaterialConfig` with uniform name list. Documented in assumptions. |
| Q-03 | [RESOLVED] Fine-grained `Error::Category` values are added: `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`. This is consistent with the existing pattern (each failure mode has its own category), simplifies error matching at call sites, and is explicitly allowed by ADR-001. | **Resolution**: Fine-grained categories. See A-05. |
