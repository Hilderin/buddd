# IMPL-005 — Render Pipeline (Shader, Material, VertexBuffer, IndexBuffer)

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

## Source spec

`docs/specs/render-pipeline/spec.md` (SPEC-005), accepted.

All blocking issues from the spec-critic review (`docs/specs/render-pipeline/spec-critic.md`) have been resolved:

| Issue | Resolution |
|---|---|
| B-01 (open questions Q-01, Q-02, Q-03) | **Q-01**: No uniform type checking — caller responsibility. **Q-02**: Headless `Material` accepts optional `HeadlessMaterialConfig` with `std::vector<std::string>` of known uniform names. **Q-03**: Fine-grained `Error::Category` values added. |
| B-02 (missing `set_uniform(bool)`) | `auto set_uniform(std::string_view name, bool value) -> Result<void>` overload added. |
| B-03 (headless linking error simulation) | Headless backend simulates compilation errors via `#error` marker in source; simulates linking errors when vertex output variable names have no overlap with fragment input variable names. |
| B-04 (draw methods return void vs ADR-001) | `draw()` and `draw_indexed()` return `void` — precondition violations are undefined behaviour. This is a deliberate exception to ADR-001, consistent with real-time graphics API conventions. |

## Goal

Implement the render pipeline abstractions for the Buddd Engine:

1. **`ShaderType` enum** (`Vertex`, `Fragment`) and **abstract `Shader` class** — representing a single compiled shader stage (vertex or fragment), created from GLSL source strings.
2. **Abstract `Material` class** — representing a linked shader program combining one vertex shader and one fragment shader. Manages uniforms via `set_uniform` overloads (`float`, `int32_t`, `bool`, `math::Vec3`, `math::Vec4`, `math::Mat4`) and `has_uniform()` query.
3. **`VertexFormat`, `VertexAttribute`, `VertexAttributeType`** — value types describing the layout of vertex data.
4. **`PrimitiveTopology` enum** (`Triangles`, `TriangleStrip`, `Lines`, `LineStrip`, `Points`) — controls draw mode.
5. **`IndexType` enum** (`Uint16`, `Uint32`) — controls index buffer element size.
6. **Abstract `VertexBuffer` class** — GPU buffer for vertex data, created from raw byte span with a fixed `VertexFormat`.
7. **Abstract `IndexBuffer` class** — GPU buffer for index data, created from raw byte span with a fixed `IndexType`.
8. **OpenGL 4.5 Core backends** — `ShaderOpenGL`, `MaterialOpenGL`, `VertexBufferOpenGL`, `IndexBufferOpenGL` using DSA APIs (`glCreateShader`, `glCreateProgram`, `glCreateVertexArrays`, `glCreateBuffers`, `glNamedBufferStorage`, etc.).
9. **Headless backends** — `ShaderHeadless`, `MaterialHeadless`, `VertexBufferHeadless`, `IndexBufferHeadless` storing data in memory, no GPU involvement, with simulated compilation/linking error detection.
10. **`RenderDevice` factory methods** — `create_shader`, `create_material`, `create_vertex_buffer`, `create_index_buffer`, plus drawing methods `draw`, `draw_indexed`.
11. **New `Error::Category` values** — `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`.

## Non-goals

- No texture loading, creation, binding, or sampling.
- No lighting, shading models, or PBR.
- No model loading or mesh import (`.obj`, `.gltf`, etc.).
- No scene graph, transform hierarchy, or entity system.
- No compute shaders, geometry shaders, or tessellation shaders.
- No persistent mapping or streaming of vertex/index buffer data.
- No secondary command buffers, multi-draw indirect, or instanced rendering.
- No uniform buffer objects (UBO), shader storage buffer objects (SSBO), or push constants.
- No separate `Texture` abstraction.
- No framebuffer objects (FBO), render targets, or render-to-texture.
- No dynamic backend switching after resource creation.
- No serialisation or file format for shaders/materials/buffers.
- No debug markers or GPU profiling annotations.
- No caching or deduplication of compiled shaders or materials.
- No multi-window rendering.
- No changes to build system (new `.h`/`.cpp` files are picked up automatically by existing `file(GLOB_RECURSE)`).
- No test file creation (tests are specified for the test-author only).
- No modification of files outside `src/engine/`.

## Relevant constitution rules

- **CONST-001-architecture-boundaries.md**: Enforces the architecture boundary — no code outside `src/engine/` may include graphics library headers. All new abstract headers (`shader.h`, `material.h`, `vertex_buffer.h`, `index_buffer.h`, `vertex_format.h`, `primitive_topology.h`) must not expose backend types.
- **CONST-002-testing-policy.md**: Requires unit tests for all testable code. This contract specifies required tests (see Required tests section).

## Relevant ADRs

- **ADR-001** (`docs/adr/001-result-error-pattern.md`): Establishes `Result<T>` / `Error` as the project-wide error handling pattern. This contract extends `Error::Category` with new values and uses `Result<T>` for all resource creation and `set_uniform`. **Exception**: `draw()` / `draw_indexed()` return `void` (see Draw methods rationale below).
- **ADR-002** (`docs/adr/002-glm-wrapper-math.md`): Establishes the GLM wrapper pattern for math types (`math::Vec3`, `math::Vec4`, `math::Mat4`). These types are used for uniform values in `set_uniform` overloads.

### Draw methods rationale (exception to ADR-001)

`draw()` and `draw_indexed()` return `void` rather than `Result<void>` because:
- Draw calls are on a performance-sensitive hot path where per-frame error checking is impractical.
- Precondition violations (invalid topology, out-of-bounds vertex access, unlinked material, draw outside `begin_frame()`/`end_frame()`) are **undefined behaviour** — the caller must ensure correct state before drawing.
- This is a deliberate exception to ADR-001's `Result<T>` convention, consistent with real-time graphics API conventions (OpenGL, Vulkan, DirectX all use immediate draw commands with precondition-based contracts).

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/error.h` | Current `Error::Category` enum — must be extended with new values. |
| `src/engine/render/render_device.h` | Current abstract `RenderDevice` — must gain factory and draw methods. |
| `src/engine/render/render_device_opengl.h` | Current concrete OpenGL `RenderDevice` — must gain new method overrides. |
| `src/engine/render/render_device_opengl.cpp` | Current OpenGL implementation — pattern for new method stubs. |
| `src/engine/render/render_device_headless.h` | Current concrete Headless `RenderDevice` — must gain new method overrides. |
| `src/engine/render/render_device_headless.cpp` | Current Headless implementation — pattern for new method stubs. |
| `src/engine/math/vec3.h` | `math::Vec3` type — used in `set_uniform(const math::Vec3&)`. |
| `src/engine/math/vec4.h` | `math::Vec4` type — used in `set_uniform(const math::Vec4&)`. |
| `src/engine/math/mat4.h` | `math::Mat4` type — used in `set_uniform(const math::Mat4&)`. |
| `src/engine/CMakeLists.txt` | Current CMake file — no changes needed (new files picked up by `GLOB_RECURSE`). |
| `docs/specs/platform-abstraction/implementation-contract.md` | Style reference (IMPL-002) for contract format and level of detail. |

## Files allowed to change

### New files to create (22 files)

All paths are relative to the repository root.

1. `src/engine/render/primitive_topology.h`
2. `src/engine/render/vertex_format.h`
3. `src/engine/render/shader.h`
4. `src/engine/render/material.h`
5. `src/engine/render/vertex_buffer.h`
6. `src/engine/render/index_buffer.h`
7. `src/engine/render/shader_opengl.h`
8. `src/engine/render/shader_opengl.cpp`
9. `src/engine/render/material_opengl.h`
10. `src/engine/render/material_opengl.cpp`
11. `src/engine/render/vertex_buffer_opengl.h`
12. `src/engine/render/vertex_buffer_opengl.cpp`
13. `src/engine/render/index_buffer_opengl.h`
14. `src/engine/render/index_buffer_opengl.cpp`
15. `src/engine/render/shader_headless.h`
16. `src/engine/render/shader_headless.cpp`
17. `src/engine/render/material_headless.h`
18. `src/engine/render/material_headless.cpp`
19. `src/engine/render/vertex_buffer_headless.h`
20. `src/engine/render/vertex_buffer_headless.cpp`
21. `src/engine/render/index_buffer_headless.h`
22. `src/engine/render/index_buffer_headless.cpp`

### Files to modify (10 files)

23. `src/engine/error.h` — Add new `Error::Category` values and update `to_string()`.
24. `src/engine/render/render_device.h` — Add pure virtual factory methods and draw methods. Add forward declarations for new abstract types.
25. `src/engine/render/render_device_opengl.h` — Add new method overrides.
26. `src/engine/render/render_device_opengl.cpp` — Implement new factory and draw methods.
27. `src/engine/render/render_device_headless.h` — Add new method overrides.
28. `src/engine/render/render_device_headless.cpp` — Implement new factory and draw methods.
29. `src/engine/platform/platform.h` — Add `poll_events() -> bool` pure virtual method.
30. `src/engine/platform/platform_sdl3.cpp` — Implement `poll_events()` using SDL_PollEvent.
31. `src/engine/platform/platform_headless.cpp` — Implement `poll_events()` returning true.
32. `src/cmd/main.cpp` — Use `platform->poll_events()` instead of SDL3 includes.

Total: 22 new files + 10 modified files = 32 files changed. (Note: the earlier "Files to modify (5 files)" heading was updated to 10 as the feature scope expanded.)

23. `src/engine/error.h` — Add new `Error::Category` values and update `to_string()`.
24. `src/engine/render/render_device.h` — Add pure virtual factory methods and draw methods. Add forward declarations for new abstract types.
25. `src/engine/render/render_device_opengl.h` — Add new method overrides.
26. `src/engine/render/render_device_opengl.cpp` — Implement new factory and draw methods.
27. `src/engine/render/render_device_headless.h` — Add new method overrides.
28. `src/engine/render/render_device_headless.cpp` — Implement new factory and draw methods.
29. `src/cmd/main.cpp` — Add `--test` mode that creates a window, renders a triangle for 120 frames, and exits.

Total: 22 new files + 7 modified files = 29 files changed.

## Files forbidden to change

- Any file outside `src/engine/` or `src/cmd/`.
- `src/engine/version.h`
- `src/engine/version.cpp`
- Root `CMakeLists.txt`
- `CMakePresets.json`
- `src/editor/` (any file)
- `tests/` (any file — test files are for the test-author)
- `.clang-format`
- `.vscode/` (any file)
- `docs/` (any file not listed in "Files allowed to change" — includes `docs/adr/`, `docs/constitution/`, `docs/wiki/`, etc.)
- `AGENTS.md`
- `opencode.json`
- `SpecKit.md`

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Namespace | `buddd::engine` for all public types. Concrete backend classes use same namespace or anonymous namespace as needed. |
| File naming | `snake_case` (lowercase ASCII letters, digits, underscores). |
| Class naming | PascalCase (e.g., `ShaderOpenGL`, `MaterialHeadless`). |
| Enum naming | PascalCase for enum class names and values (e.g., `ShaderType::Vertex`). |
| Header guards | `#pragma once` (no `#ifndef` guards). |
| Function style | Trailing return type syntax (`auto foo() -> int`). |
| Non-copyable, non-movable | All abstract classes (`Shader`, `Material`, `VertexBuffer`, `IndexBuffer`) and all concrete backend classes must have both copy and move constructors/assignment operators `= delete`. |
| Formatting | `.clang-format` at repository root enforces LLVM style, 4-space indent, 100 column limit. |
| Includes | Standard library includes use `<>`; project includes use `""` relative to `src/engine/`. |
| Error handling | All fallible factory methods return `Result<T>`. Use `make_error()` to return errors. |
| Observability | Use `std::cerr` for lifecycle events, matching SPEC-002 and SPEC-005 conventions. |
| OpenGL includes in backend files | Use `<GL/gl.h>` for OpenGL types. Provided by `find_package(OpenGL REQUIRED)`. |
| No backend types in abstract headers | Abstract headers (`shader.h`, `material.h`, `vertex_buffer.h`, `index_buffer.h`, `vertex_format.h`, `primitive_topology.h`) must not include `<GL/`, `<SDL3/`, or any backend-specific header. |

### 30. `src/engine/platform/platform.h` — Add `poll_events()`

Add to the `Platform` class (after `create_window()`):

```cpp
    /// Polls the platform event queue.
    /// Returns false if the user requested to quit (e.g., window close button),
    /// true otherwise. In headless mode, always returns true.
    virtual auto poll_events() -> bool = 0;
```

**Requirements:**
- Pure virtual method.
- Returns `bool` (not `Result<bool>`) — event polling is not a fallible operation; the boolean signals quit vs continue.
- Add no new includes — `bool` is a built-in type.

### 31. `src/engine/platform/platform_sdl3.cpp` — Implement `poll_events()`

Add to the `PlatformSDL3` class (in both header and implementation):

In `platform_sdl3.h`:
```cpp
auto poll_events() -> bool override;
```

In `platform_sdl3.cpp`:
```cpp
auto PlatformSDL3::poll_events() -> bool {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
        // Other events are discarded — input handling is future work.
    }
    return true;
}
```

**Requirements:**
- Uses `SDL_PollEvent` in a loop to drain the event queue.
- Returns `false` on `SDL_EVENT_QUIT`, `true` otherwise.
- All other event types are discarded (keyboard, mouse, etc. are out of scope).
- The existing `#include <SDL3/SDL.h>` in `platform_sdl3.cpp` already provides the SDL types.

### 32. `src/engine/platform/platform_headless.cpp` — Implement `poll_events()`

Add to the `PlatformHeadless` class (in both header and implementation):

In `platform_headless.h`:
```cpp
auto poll_events() -> bool override;
```

In `platform_headless.cpp`:
```cpp
auto PlatformHeadless::poll_events() -> bool {
    return true;  // Headless: never quits.
}
```

**Requirements:**
- Always returns `true` — no events to process in headless mode.
- No SDL3 or OpenGL includes needed.

### 33. `src/cmd/main.cpp` — Interactive and `--test` modes

Modify the CLI entry point to support an interactive default mode and a `--test` mode:

```cpp
#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>

// ... existing includes and code ...

auto main(int argc, char* argv[]) -> int {
    // --version mode (preserved from existing behavior)
    if (argc == 2 && std::string_view{argv[1]} == "--version") {
        std::printf("buddd %s\n", buddd::engine::version().data());
        return 0;
    }

    // --test mode: automated 120-frame render then exit
    if (argc == 2 && std::string_view{argv[1]} == "--test") {
        return run_test_mode();
    }

    // Default mode: render until window is closed by the user
    return run_interactive();
}

static auto run_test_mode() -> int {
    // Create SDL3 platform (requires display)
    auto platform = buddd::engine::Platform::create(buddd::engine::Backend::SDL3);
    if (!platform) {
        std::cerr << "Failed to create platform: "
                  << buddd::engine::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto window = platform->create_window({
        .title = "Buddd Engine — Render Test",
        .width = 800,
        .height = 600
    });
    if (!window) {
        std::cerr << "Failed to create window: "
                  << buddd::engine::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto device = buddd::engine::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "Failed to create render device: "
                  << buddd::engine::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Create shaders
    constexpr std::string_view vertex_source = R"(
        #version 450 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_color;
        out vec3 v_color;
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";

    constexpr std::string_view fragment_source = R"(
        #version 450 core
        in vec3 v_color;
        out vec4 frag_color;
        void main() {
            frag_color = vec4(v_color, 1.0);
        }
    )";

    auto vs = (*device)->create_shader(
        buddd::engine::ShaderType::Vertex, vertex_source);
    if (!vs) {
        std::cerr << "Vertex shader creation failed: "
                  << buddd::engine::to_string(vs.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto fs = (*device)->create_shader(
        buddd::engine::ShaderType::Fragment, fragment_source);
    if (!fs) {
        std::cerr << "Fragment shader creation failed: "
                  << buddd::engine::to_string(fs.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Create material
    auto material = (*device)->create_material(
        std::move(*vs), std::move(*fs));
    if (!material) {
        std::cerr << "Material creation failed: "
                  << buddd::engine::to_string(material.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Create vertex buffer: triangle with position (Float3) and color (Float3)
    struct Vertex { float x, y, z, r, g, b; };
    const Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f },  // top, red
        {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f },  // bottom-left, green
        { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f },  // bottom-right, blue
    };

    buddd::engine::VertexFormat format;
    format.stride = sizeof(Vertex);
    format.attributes = {
        {0, buddd::engine::VertexAttributeType::Float3, offsetof(Vertex, x), false},
        {1, buddd::engine::VertexAttributeType::Float3, offsetof(Vertex, r), false},
    };

    auto vertex_data = std::as_bytes(std::span(vertices));
    auto vb = (*device)->create_vertex_buffer(format, vertex_data);
    if (!vb) {
        std::cerr << "Vertex buffer creation failed: "
                  << buddd::engine::to_string(vb.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Render loop: ~120 frames at 60 FPS (~2 seconds)
    constexpr int target_frames = 120;
    constexpr auto frame_duration = std::chrono::milliseconds(16); // ~60 FPS

    std::cerr << "Render test started: " << target_frames << " frames\n";

    for (int frame = 0; frame < target_frames; ++frame) {
        auto frame_start = std::chrono::steady_clock::now();

        (*device)->begin_frame();
        (*device)->draw(
            buddd::engine::PrimitiveTopology::Triangles,
            **vb, **material, 3);
        (*device)->end_frame();

        // Process platform events (allow window close to abort early)
        if (!(*platform)->poll_events()) {
            std::cerr << "Render test aborted by user (frame " << frame << ")\n";
            return EXIT_SUCCESS;
        }

        // Frame rate limiting
        auto elapsed = std::chrono::steady_clock::now() - frame_start;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }

    std::cerr << "Render test complete: " << target_frames << " frames rendered\n";
    return EXIT_SUCCESS;
}

static auto run_interactive() -> int {
    // Create SDL3 platform (requires display)
    auto platform = buddd::engine::Platform::create(buddd::engine::Backend::SDL3);
    if (!platform) {
        std::cerr << "FATAL: " << buddd::engine::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto window = (*platform)->create_window({
        .title = "Buddd Engine",
        .width = 1024,
        .height = 768
    });
    if (!window) {
        std::cerr << "FATAL: " << buddd::engine::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    std::printf("Window opened: %dx%d\n", (*window)->width(), (*window)->height());

    auto device = buddd::engine::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "FATAL: " << buddd::engine::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Create shaders, material, and vertex buffer (same as run_test_mode)
    // ... (shared setup code — implementation may factor out shared setup) ...

    // Render loop: runs until the window is closed by the user
    while ((*platform)->poll_events()) {

        (*device)->begin_frame();
        (*device)->draw(
            buddd::engine::PrimitiveTopology::Triangles,
            **vb, **material, 3);
        (*device)->end_frame();
    }

    std::printf("Window closed, shutting down.\n");
    return EXIT_SUCCESS;
}
```

**Requirements:**
- The existing `main()` function is replaced. It handles three cases:
  1. `--version`: Print version and exit (unchanged from existing behavior).
  2. `--test`: Call `run_test_mode()` — automated 120-frame render then exit.
  3. No arguments (default): Call `run_interactive()` — render until window closed.
- `run_test_mode()`:
  - Creates a SDL3 platform, window (800×600), and OpenGL render device.
  - Defines a minimal passthrough vertex shader and per-vertex colour fragment shader (GLSL 450 core).
  - Creates a vertex buffer with a triangle (3 vertices: position Float3 + colour Float3).
  - Runs a loop for exactly 120 frames at ~60 FPS, calling `begin_frame()` → `draw()` → `end_frame()` each frame.
  - Processes SDL events to allow early exit if the window is closed (aborted by user).
  - Prints status messages to `std::cerr`.
  - Returns `EXIT_SUCCESS` on completion, `EXIT_FAILURE` on any error.
- `run_interactive()`:
  - Creates a SDL3 platform, window (1024×768), and OpenGL render device.
  - Creates the same shaders, material, and vertex buffer as `run_test_mode()`.
  - Runs an infinite render loop (with event processing) until `SDL_EVENT_QUIT` is received.
  - Exits with `EXIT_SUCCESS` when the window is closed.
- If `--test` or default mode is used on a system without a display, `Platform::create(SDL3)` fails and a descriptive error is printed (handled by existing error propagation).
- Shared setup code (shader/material/vertex buffer creation) between the two modes should be factored into a helper function or duplicated — the contract does not mandate either approach, but duplication is acceptable for simplicity.
- Add `#include` directives for all new types used:
  - `"render/shader.h"`, `"render/material.h"`, `"render/vertex_buffer.h"`, `"render/vertex_format.h"`, `"render/primitive_topology.h"`
- Add `#include <thread>` and `#include <chrono>` for frame timing.
- Do NOT include `<SDL3/SDL.h>` — event polling is done through the abstract `Platform::poll_events()` method.
- Do NOT include `<SDL3/SDL.h>` or any other graphics library header — this would violate CONST-001.

## Required implementation behavior

### 1. `src/engine/error.h` — Add new `Error::Category` values

Add the following values to the `Error::Category` enum (before `Unknown`):

```cpp
enum class Category {
    InitFailed,
    WindowCreationFailed,
    RenderDeviceCreationFailed,
    ShaderCompilationFailed,    // new
    LinkingFailed,              // new
    ResourceCreationFailed,     // new
    InvalidArgument,            // new
    UniformNotFound,            // new
    Unsupported,
    Unknown
};
```

Update the `to_string()` switch to include the new categories:

```cpp
case Error::Category::ShaderCompilationFailed:  category_str = "ShaderCompilationFailed"; break;
case Error::Category::LinkingFailed:            category_str = "LinkingFailed"; break;
case Error::Category::ResourceCreationFailed:   category_str = "ResourceCreationFailed"; break;
case Error::Category::InvalidArgument:          category_str = "InvalidArgument"; break;
case Error::Category::UniformNotFound:          category_str = "UniformNotFound"; break;
```

**Requirements:**
- New values are added before `Unknown` (existing values must not be reordered).
- All existing values remain unchanged.
- `to_string()` handles all new values.

### 2. `src/engine/render/primitive_topology.h`

```cpp
#pragma once

namespace buddd::engine {

enum class PrimitiveTopology {
    Triangles,
    TriangleStrip,
    Lines,
    LineStrip,
    Points
};

} // namespace buddd::engine
```

**Requirements:**
- Header-only, no `.cpp` file.
- Pure value type — no methods, no constructors, no virtual anything.
- No includes (not even `error.h`).
- **Integer backing values** — not specified (implementation-defined). The OpenGL backend must use a switch to map to `GL_TRIANGLES`, `GL_TRIANGLE_STRIP`, `GL_LINES`, `GL_LINE_STRIP`, `GL_POINTS`.

### 3. `src/engine/render/vertex_format.h`

```cpp
#pragma once

#include <cstdint>
#include <vector>

namespace buddd::engine {

enum class VertexAttributeType {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UByte,
    UByte4,
    UByte4Norm
};

struct VertexAttribute {
    uint32_t location;
    VertexAttributeType type;
    uint32_t offset;
    bool normalized{false};
};

struct VertexFormat {
    uint32_t stride;
    std::vector<VertexAttribute> attributes;
};

} // namespace buddd::engine
```

**Requirements:**
- Header-only, no `.cpp` file.
- `VertexFormat::attributes` is `std::vector<VertexAttribute>` (as suggested by spec-critic W-01).
- `VertexAttribute::normalized` defaults to `false` (as required by spec).
- No virtual methods, no backend-specific behaviour.

### 4. `src/engine/render/shader.h`

```cpp
#pragma once

#include "error.h"

#include <memory>
#include <string_view>

namespace buddd::engine {

enum class ShaderType {
    Vertex,
    Fragment
};

class Shader {
public:
    virtual ~Shader() = default;

    virtual auto type() const noexcept -> ShaderType = 0;

    Shader(const Shader&) = delete;
    auto operator=(const Shader&) -> Shader& = delete;
    Shader(Shader&&) = delete;
    auto operator=(Shader&&) -> Shader& = delete;

protected:
    Shader() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- `ShaderType` enum with exactly two values.
- `Shader` is abstract with `type()` as the sole pure virtual method.
- Virtual destructor (not pure virtual — pure virtual destructor would require a body).
- Non-copyable, non-movable.
- Protected default constructor.

### 5. `src/engine/render/material.h`

```cpp
#pragma once

#include "error.h"
#include "shader.h"

#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"

#include <memory>
#include <string_view>

namespace buddd::engine {

class Material {
public:
    virtual ~Material() = default;

    virtual auto set_uniform(std::string_view name, float value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, int32_t value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, bool value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> = 0;

    virtual auto has_uniform(std::string_view name) const -> bool = 0;

    Material(const Material&) = delete;
    auto operator=(const Material&) -> Material& = delete;
    Material(Material&&) = delete;
    auto operator=(Material&&) -> Material& = delete;

protected:
    Material() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- Six `set_uniform` overloads: `float`, `int32_t`, `bool`, `const math::Vec3&`, `const math::Vec4&`, `const math::Mat4&`.
- Each returns `Result<void>`.
- `has_uniform` returns `bool` and is `const`.
- Non-copyable, non-movable.
- Includes `math/vec3.h`, `math/vec4.h`, `math/mat4.h` for the math types (these are within `src/engine/` and are part of the abstract API by design per ADR-002).
- Includes `shader.h` for `ShaderType` (though Material does not expose Shader directly, having the include allows callers to use both headers).

### 6. `src/engine/render/vertex_buffer.h`

```cpp
#pragma once

#include "error.h"
#include "vertex_format.h"

#include <memory>
#include <span>

namespace buddd::engine {

class VertexBuffer {
public:
    virtual ~VertexBuffer() = default;

    virtual auto format() const noexcept -> const VertexFormat& = 0;

    VertexBuffer(const VertexBuffer&) = delete;
    auto operator=(const VertexBuffer&) -> VertexBuffer& = delete;
    VertexBuffer(VertexBuffer&&) = delete;
    auto operator=(VertexBuffer&&) -> VertexBuffer& = delete;

protected:
    VertexBuffer() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- `format()` returns a const reference to the `VertexFormat` stored at creation time.
- Non-copyable, non-movable.

### 7. `src/engine/render/index_buffer.h`

```cpp
#pragma once

#include "error.h"

#include <memory>
#include <span>

namespace buddd::engine {

enum class IndexType {
    Uint16,
    Uint32
};

class IndexBuffer {
public:
    virtual ~IndexBuffer() = default;

    virtual auto type() const noexcept -> IndexType = 0;

    IndexBuffer(const IndexBuffer&) = delete;
    auto operator=(const IndexBuffer&) -> IndexBuffer& = delete;
    IndexBuffer(IndexBuffer&&) = delete;
    auto operator=(IndexBuffer&&) -> IndexBuffer& = delete;

protected:
    IndexBuffer() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- `IndexType` enum with exactly two values.
- `type()` returns the `IndexType` stored at creation time.
- Non-copyable, non-movable.

### 8. `src/engine/render/render_device.h` — Add new methods

Add forward declarations before the class:

```cpp
class Shader;
class Material;
class VertexBuffer;
class IndexBuffer;
enum class ShaderType;
enum class PrimitiveTopology;
enum class IndexType;
struct VertexFormat;
```

Add to the `RenderDevice` class (after `size()`):

```cpp
    // -- Resource factories --
    virtual auto create_shader(ShaderType type, std::string_view source) -> Result<std::unique_ptr<Shader>> = 0;
    virtual auto create_material(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader,
        std::span<const std::string> known_uniforms = {}
    ) -> Result<std::unique_ptr<Material>> = 0;
    virtual auto create_vertex_buffer(
        const VertexFormat& format,
        std::span<const std::byte> data
    ) -> Result<std::unique_ptr<VertexBuffer>> = 0;
    virtual auto create_index_buffer(
        std::span<const std::byte> data,
        IndexType type
    ) -> Result<std::unique_ptr<IndexBuffer>> = 0;

    // -- Drawing --
    virtual auto draw(
        PrimitiveTopology topology,
        const VertexBuffer& vertices,
        const Material& material,
        uint32_t vertex_count,
        uint32_t start_vertex = 0
    ) -> void = 0;

    virtual auto draw_indexed(
        PrimitiveTopology topology,
        const VertexBuffer& vertices,
        const IndexBuffer& indices,
        const Material& material,
        uint32_t index_count,
        uint32_t start_index = 0
    ) -> void = 0;
```

Add includes:
```cpp
#include <span>
#include <string>
#include <vector>    // for span<const string> default argument type
```

**Requirements:**
- All factory methods return `Result<std::unique_ptr<T>>`.
- All factory methods are pure virtual.
- `create_material` has an optional `known_uniforms` parameter defaulting to `{}`. The OpenGL backend ignores it; the headless backend uses it for uniform discovery.
- `draw` and `draw_indexed` return `void`.
- `start_vertex` and `start_index` default to `0`.
- The draw methods take `const VertexBuffer&` and `const Material&` by const reference (not pointer, not unique_ptr).
- The `index_count` parameter in `draw_indexed` is the number of **indices** to draw (not vertices).

### 9. `src/engine/render/render_device_opengl.h` — Add new method overrides

Add to `RenderDeviceOpenGL` class (after existing method declarations):

```cpp
    // -- Resource factories --
    auto create_shader(ShaderType type, std::string_view source) -> Result<std::unique_ptr<Shader>> override;
    auto create_material(
        std::unique_ptr<Shader> vertex_shader,
        std::unique_ptr<Shader> fragment_shader,
        std::span<const std::string> known_uniforms = {}
    ) -> Result<std::unique_ptr<Material>> override;
    auto create_vertex_buffer(
        const VertexFormat& format,
        std::span<const std::byte> data
    ) -> Result<std::unique_ptr<VertexBuffer>> override;
    auto create_index_buffer(
        std::span<const std::byte> data,
        IndexType type
    ) -> Result<std::unique_ptr<IndexBuffer>> override;

    // -- Drawing --
    auto draw(
        PrimitiveTopology topology,
        const VertexBuffer& vertices,
        const Material& material,
        uint32_t vertex_count,
        uint32_t start_vertex = 0
    ) -> void override;
    auto draw_indexed(
        PrimitiveTopology topology,
        const VertexBuffer& vertices,
        const IndexBuffer& indices,
        const Material& material,
        uint32_t index_count,
        uint32_t start_index = 0
    ) -> void override;
```

Add includes:
```cpp
#include "shader.h"
#include "material.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "primitive_topology.h"
```

### 10. `src/engine/render/render_device_opengl.cpp` — Implement new methods

**Shader creation:**

```cpp
auto RenderDeviceOpenGL::create_shader(ShaderType type, std::string_view source)
    -> Result<std::unique_ptr<Shader>>
{
    if (source.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Shader source is empty");
    }

    GLenum gl_type = (type == ShaderType::Vertex) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    GLuint shader_id = glCreateShader(gl_type);

    const GLchar* sources[] = { source.data() };
    GLint lengths[] = { static_cast<GLint>(source.size()) };
    glShaderSource(shader_id, 1, sources, lengths);
    glCompileShader(shader_id);

    GLint compile_status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);

    if (compile_status != GL_TRUE) {
        GLint log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(log_length, '\0');
        glGetShaderInfoLog(shader_id, log_length, nullptr, log.data());
        glDeleteShader(shader_id);

        std::cerr << "Shader compilation failed: " << log << "\n";
        return make_error(Error::Category::ShaderCompilationFailed, std::move(log));
    }

    std::cerr << "Shader created (type="
              << (type == ShaderType::Vertex ? "Vertex" : "Fragment")
              << ")\n";

    return std::unique_ptr<Shader>(new ShaderOpenGL(shader_id, type));
}
```

**Material creation:**

```cpp
auto RenderDeviceOpenGL::create_material(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader,
    std::span<const std::string> /*known_uniforms*/
) -> Result<std::unique_ptr<Material>>
{
    if (!vertex_shader || !fragment_shader) {
        return make_error(Error::Category::InvalidArgument,
            "Null shader passed to create_material");
    }

    auto& vs = static_cast<ShaderOpenGL&>(*vertex_shader);
    auto& fs = static_cast<ShaderOpenGL&>(*fragment_shader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs.handle());
    glAttachShader(program, fs.handle());
    glLinkProgram(program);

    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);

    if (link_status != GL_TRUE) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        std::string log(log_length, '\0');
        glGetProgramInfoLog(program, log_length, nullptr, log.data());
        glDeleteProgram(program);

        std::cerr << "Material linking failed: " << log << "\n";
        return make_error(Error::Category::LinkingFailed, std::move(log));
    }

    // Shaders are marked for deletion; they stay alive until program is deleted.
    glDeleteShader(vs.handle());
    glDeleteShader(fs.handle());

    std::cerr << "Material created\n";
    return std::unique_ptr<Material>(new MaterialOpenGL(program));
}
```

**Vertex buffer creation:**

```cpp
auto RenderDeviceOpenGL::create_vertex_buffer(
    const VertexFormat& format,
    std::span<const std::byte> data
) -> Result<std::unique_ptr<VertexBuffer>>
{
    if (data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex data is empty");
    }
    if (format.stride == 0) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex format stride must be positive");
    }
    if (format.attributes.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex format must have at least one attribute");
    }

    GLuint vao;
    glCreateVertexArrays(1, &vao);

    GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, data.size(), data.data(), GL_DYNAMIC_DRAW);

    // Configure VAO
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, static_cast<GLsizei>(format.stride));

    for (const auto& attr : format.attributes) {
        auto [gl_type, component_count] = vertex_attribute_type_to_gl(attr.type);

        glEnableVertexArrayAttrib(vao, attr.location);
        glVertexArrayAttribFormat(
            vao,
            attr.location,
            component_count,
            gl_type,
            attr.normalized ? GL_TRUE : GL_FALSE,
            static_cast<GLuint>(attr.offset)
        );
        glVertexArrayAttribBinding(vao, attr.location, 0);
    }

    uint32_t vertex_count = static_cast<uint32_t>(data.size() / format.stride);
    std::cerr << "Vertex buffer created (" << vertex_count
              << " vertices, " << format.attributes.size() << " attributes)\n";

    return std::unique_ptr<VertexBuffer>(
        new VertexBufferOpenGL(vao, vbo, format, data.size()));
}
```

**Index buffer creation:**

```cpp
auto RenderDeviceOpenGL::create_index_buffer(
    std::span<const std::byte> data,
    IndexType type
) -> Result<std::unique_ptr<IndexBuffer>>
{
    if (data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Index data is empty");
    }

    GLuint ibo;
    glCreateBuffers(1, &ibo);
    glNamedBufferStorage(ibo, data.size(), data.data(), GL_DYNAMIC_DRAW);

    std::cerr << "Index buffer created (" << data.size() << " bytes, "
              << (type == IndexType::Uint16 ? "Uint16" : "Uint32") << ")\n";

    return std::unique_ptr<IndexBuffer>(
        new IndexBufferOpenGL(ibo, type, data.size()));
}
```

**Draw methods:**

```cpp
auto RenderDeviceOpenGL::draw(
    PrimitiveTopology topology,
    const VertexBuffer& vertices,
    const Material& material,
    uint32_t vertex_count,
    uint32_t start_vertex
) -> void
{
    auto& mat = static_cast<const MaterialOpenGL&>(material);
    auto& vb = static_cast<const VertexBufferOpenGL&>(vertices);

    glUseProgram(mat.program());
    glBindVertexArray(vb.vao());
    glDrawArrays(primitive_topology_to_gl(topology), start_vertex, vertex_count);

#ifndef NDEBUG
    std::cerr << "Draw: " << primitive_topology_to_string(topology)
              << " " << vertex_count << " vertices\n";
#endif
}

auto RenderDeviceOpenGL::draw_indexed(
    PrimitiveTopology topology,
    const VertexBuffer& vertices,
    const IndexBuffer& indices,
    const Material& material,
    uint32_t index_count,
    uint32_t start_index
) -> void
{
    auto& mat = static_cast<const MaterialOpenGL&>(material);
    auto& vb = static_cast<const VertexBufferOpenGL&>(vertices);
    auto& ib = static_cast<const IndexBufferOpenGL&>(indices);

    GLenum gl_index_type = (ib.index_type() == IndexType::Uint16)
        ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

    GLsizeiptr index_byte_size = (ib.index_type() == IndexType::Uint16)
        ? sizeof(uint16_t) : sizeof(uint32_t);

    glUseProgram(mat.program());
    glBindVertexArray(vb.vao());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib.handle());
    glDrawElements(
        primitive_topology_to_gl(topology),
        index_count,
        gl_index_type,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(start_index * index_byte_size))
    );

#ifndef NDEBUG
    std::cerr << "Draw indexed: " << primitive_topology_to_string(topology)
              << " " << index_count << " indices\n";
#endif
}
```

**Requirements:**
- All factory methods must return `Result<T>` with appropriate error categories.
- `create_shader`: empty source → `InvalidArgument`. Compilation failure → `ShaderCompilationFailed` with info log.
- `create_material`: null shader → `InvalidArgument`. Linking failure → `LinkingFailed` with info log.
- `create_vertex_buffer`: empty data, zero stride, or zero attributes → `InvalidArgument`.
- `create_index_buffer`: empty data → `InvalidArgument`.
- Draw methods return `void` and use `static_cast` to downcast to concrete types (safe because the factory creates the matching types).
- All observability outputs use `std::cerr` as prescribed in the spec's Observability section.
- Debug builds print draw call info.
- Helper functions `vertex_attribute_type_to_gl`, `primitive_topology_to_gl`, `primitive_topology_to_string` are implementation details (could be free functions in the `.cpp` file, anonymous namespace, or private static methods).

**VertexAttributeType → OpenGL mapping helper:**

```
Float     → {GL_FLOAT,           1}
Float2    → {GL_FLOAT,           2}
Float3    → {GL_FLOAT,           3}
Float4    → {GL_FLOAT,           4}
Int       → {GL_INT,             1}
Int2      → {GL_INT,             2}
Int3      → {GL_INT,             3}
Int4      → {GL_INT,             4}
UByte     → {GL_UNSIGNED_BYTE,   1}
UByte4    → {GL_UNSIGNED_BYTE,   4}
UByte4Norm→ {GL_UNSIGNED_BYTE,   4}
```

**PrimitiveTopology → GL mapping:**

```
Triangles     → GL_TRIANGLES
TriangleStrip → GL_TRIANGLE_STRIP
Lines         → GL_LINES
LineStrip     → GL_LINE_STRIP
Points        → GL_POINTS
```

**PrimitiveTopology → string mapping (for debug):**

```
Triangles     → "Triangles"
TriangleStrip → "TriangleStrip"
Lines         → "Lines"
LineStrip     → "LineStrip"
Points        → "Points"
```

### 11. `src/engine/render/render_device_headless.h` — Add new method overrides

Same override declarations as `render_device_opengl.h` but without OpenGL-specific includes.

Add forward declarations for the headless concrete types.

New private members:
```cpp
    // Headless state tracking (optional — for diagnostics)
    int shader_count_{0};
    int material_count_{0};
    int vertex_buffer_count_{0};
    int index_buffer_count_{0};
    int draw_call_count_{0};
```

### 12. `src/engine/render/render_device_headless.cpp` — Implement new methods

**Shader creation:**

```cpp
auto RenderDeviceHeadless::create_shader(ShaderType type, std::string_view source)
    -> Result<std::unique_ptr<Shader>>
{
    if (source.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Shader source is empty");
    }

    // Simulate compilation error if source contains "#error"
    if (source.find("#error") != std::string_view::npos) {
        std::cerr << "Shader compilation failed (simulated)\n";
        return make_error(Error::Category::ShaderCompilationFailed,
            "Simulated compilation error: #error directive found in source");
    }

    ++shader_count_;

    std::cerr << "Shader created (Headless, type="
              << (type == ShaderType::Vertex ? "Vertex" : "Fragment")
              << ")\n";

    return std::unique_ptr<Shader>(new ShaderHeadless(type, std::string(source)));
}
```

**Material creation — including linking error simulation:**

```cpp
auto RenderDeviceHeadless::create_material(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader,
    std::span<const std::string> known_uniforms
) -> Result<std::unique_ptr<Material>>
{
    if (!vertex_shader || !fragment_shader) {
        return make_error(Error::Category::InvalidArgument,
            "Null shader passed to create_material");
    }

    auto& vs = static_cast<ShaderHeadless&>(*vertex_shader);
    auto& fs = static_cast<ShaderHeadless&>(*fragment_shader);

    // Simulate linking error: vertex output names vs fragment input names
    auto vs_outputs = extract_variable_names(vs.source(), /*is_output=*/true);
    auto fs_inputs  = extract_variable_names(fs.source(), /*is_output=*/false);

    // Check if any vertex output matches any fragment input
    bool has_matching = false;
    for (const auto& vs_out : vs_outputs) {
        for (const auto& fs_in : fs_inputs) {
            if (vs_out == fs_in) {
                has_matching = true;
                break;
            }
        }
        if (has_matching) break;
    }

    if (!fs_inputs.empty() && !has_matching) {
        std::cerr << "Material linking failed (simulated: no matching "
                     "vertex output / fragment input variables)\n";
        return make_error(Error::Category::LinkingFailed,
            "Simulated linking error: vertex shader outputs ("
            + join(vs_outputs, ", ") + ") do not match fragment shader inputs ("
            + join(fs_inputs, ", ") + ")");
    }

    ++material_count_;

    // Collect uniform names: from shader source parsing + explicit known_uniforms
    std::unordered_set<std::string> uniform_names;
    auto vs_uniforms = extract_uniform_names(vs.source());
    auto fs_uniforms = extract_uniform_names(fs.source());
    uniform_names.insert(vs_uniforms.begin(), vs_uniforms.end());
    uniform_names.insert(fs_uniforms.begin(), fs_uniforms.end());
    for (const auto& name : known_uniforms) {
        uniform_names.insert(name);
    }

    std::cerr << "Material created (Headless)\n";

    return std::unique_ptr<Material>(
        new MaterialHeadless(std::move(uniform_names)));
}
```

**Vertex buffer and index buffer creation:**

```cpp
auto RenderDeviceHeadless::create_vertex_buffer(
    const VertexFormat& format,
    std::span<const std::byte> data
) -> Result<std::unique_ptr<VertexBuffer>>
{
    if (data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex data is empty");
    }
    if (format.stride == 0) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex format stride must be positive");
    }
    if (format.attributes.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Vertex format must have at least one attribute");
    }

    ++vertex_buffer_count_;

    uint32_t vertex_count = static_cast<uint32_t>(data.size() / format.stride);
    std::cerr << "Vertex buffer created (Headless, " << vertex_count
              << " vertices)\n";

    return std::unique_ptr<VertexBuffer>(
        new VertexBufferHeadless(format, data));
}

auto RenderDeviceHeadless::create_index_buffer(
    std::span<const std::byte> data,
    IndexType type
) -> Result<std::unique_ptr<IndexBuffer>>
{
    if (data.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "Index data is empty");
    }

    ++index_buffer_count_;

    std::cerr << "Index buffer created (Headless, " << data.size()
              << " bytes)\n";

    return std::unique_ptr<IndexBuffer>(
        new IndexBufferHeadless(type, data));
}
```

**Draw methods (no-ops):**

```cpp
auto RenderDeviceHeadless::draw(
    PrimitiveTopology /*topology*/,
    const VertexBuffer& /*vertices*/,
    const Material& /*material*/,
    uint32_t /*vertex_count*/,
    uint32_t /*start_vertex*/
) -> void
{
    ++draw_call_count_;

#ifndef NDEBUG
    std::cerr << "Draw (Headless, " << /*vertex_count*/ "?"
              << " vertices)\n";
#endif
}

auto RenderDeviceHeadless::draw_indexed(
    PrimitiveTopology /*topology*/,
    const VertexBuffer& /*vertices*/,
    const IndexBuffer& /*indices*/,
    const Material& /*material*/,
    uint32_t /*index_count*/,
    uint32_t /*start_index*/
) -> void
{
    ++draw_call_count_;

#ifndef NDEBUG
    std::cerr << "Draw indexed (Headless, " << /*index_count*/ "?"
              << " indices)\n";
#endif
}
```

**Requirements:**
- NO `#include <SDL3/SDL.h>` or `<GL/gl.h>` in ANY headless file.
- All creation methods validate inputs identically to OpenGL backend (same errors for empty data, zero stride, zero attributes, null shaders).
- `#error` in source → `ShaderCompilationFailed`.
- Vertex output/fragment input variable name mismatch → `LinkingFailed`.
- `extract_variable_names()` parses GLSL source to find `out type name;` (or `layout(...) out type name;`) for vertex outputs, and `in type name;` (or `layout(...) in type name;`) for fragment inputs. Returns `std::vector<std::string>` of variable names. A simple regex/scan approach is sufficient — a full GLSL parser is not required. The implementation must handle:
  - `out vec4 v_color;`
  - `layout(location = 0) out vec4 v_color;`
  - `in vec4 v_color;`
  - `layout(location = 0) in vec4 v_color;`
  - Semicolons ending declarations.
- `extract_uniform_names()` parses GLSL source to find `uniform type name;` declarations and returns the uniform names.
- `join()` is a helper that concatenates strings with a separator.
- Draw methods increment a counter and are otherwise no-ops.

### 13. `src/engine/render/shader_opengl.h`

```cpp
#pragma once

#include "shader.h"

#include <GL/gl.h>

namespace buddd::engine {

class ShaderOpenGL final : public Shader {
public:
    ShaderOpenGL(GLuint handle, ShaderType type);
    ~ShaderOpenGL() override;

    auto type() const noexcept -> ShaderType override;

    auto handle() const noexcept -> GLuint;

    ShaderOpenGL(const ShaderOpenGL&) = delete;
    auto operator=(const ShaderOpenGL&) -> ShaderOpenGL& = delete;
    ShaderOpenGL(ShaderOpenGL&&) = delete;
    auto operator=(ShaderOpenGL&&) -> ShaderOpenGL& = delete;

private:
    GLuint handle_;
    ShaderType type_;
};

} // namespace buddd::engine
```

**Requirements:**
- Destructor calls `glDeleteShader(handle_)`.
- `handle()` returns the GL shader ID, used by `MaterialOpenGL` and `RenderDeviceOpenGL::create_material`.

### 14. `src/engine/render/shader_opengl.cpp`

```cpp
#include "shader_opengl.h"

namespace buddd::engine {

ShaderOpenGL::ShaderOpenGL(GLuint handle, ShaderType type)
    : handle_(handle), type_(type) {}

ShaderOpenGL::~ShaderOpenGL() {
    glDeleteShader(handle_);
}

auto ShaderOpenGL::type() const noexcept -> ShaderType {
    return type_;
}

auto ShaderOpenGL::handle() const noexcept -> GLuint {
    return handle_;
}

} // namespace buddd::engine
```

### 15. `src/engine/render/material_opengl.h`

```cpp
#pragma once

#include "material.h"

#include <GL/gl.h>

#include <string>
#include <unordered_map>

namespace buddd::engine {

class MaterialOpenGL final : public Material {
public:
    explicit MaterialOpenGL(GLuint program);
    ~MaterialOpenGL() override;

    auto set_uniform(std::string_view name, float value) -> Result<void> override;
    auto set_uniform(std::string_view name, int32_t value) -> Result<void> override;
    auto set_uniform(std::string_view name, bool value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> override;

    auto has_uniform(std::string_view name) const -> bool override;

    auto program() const noexcept -> GLuint;

    MaterialOpenGL(const MaterialOpenGL&) = delete;
    auto operator=(const MaterialOpenGL&) -> MaterialOpenGL& = delete;
    MaterialOpenGL(MaterialOpenGL&&) = delete;
    auto operator=(MaterialOpenGL&&) -> MaterialOpenGL& = delete;

private:
    auto get_uniform_location(std::string_view name) -> Result<GLint>;

    GLuint program_;
    mutable std::unordered_map<std::string, GLint> location_cache_;
};

} // namespace buddd::engine
```

**Requirements:**
- `program()` returns the GL program handle, used by `RenderDeviceOpenGL::draw()`.
- `get_uniform_location()` queries `glGetUniformLocation(program_, name.data())`.
- Results are cached in `location_cache_` (mutable since `has_uniform` is `const`).
- If `glGetUniformLocation` returns `-1`, return `make_error(Error::Category::UniformNotFound, ...)`.
- The destructor calls `glDeleteProgram(program_)`.
- **No uniform type checking** — the OpenGL backend passes the value to the appropriate `glUniform*` function regardless of the actual GLSL type in the shader. This is caller responsibility per Q-01.

### 16. `src/engine/render/material_opengl.cpp`

```cpp
#include "material_opengl.h"

#include <iostream>

namespace buddd::engine {

MaterialOpenGL::MaterialOpenGL(GLuint program)
    : program_(program) {}

MaterialOpenGL::~MaterialOpenGL() {
    glDeleteProgram(program_);
}

auto MaterialOpenGL::program() const noexcept -> GLuint {
    return program_;
}

auto MaterialOpenGL::get_uniform_location(std::string_view name) -> Result<GLint> {
    auto it = location_cache_.find(std::string(name));
    if (it != location_cache_.end()) {
        if (it->second == -1) {
            return make_error(Error::Category::UniformNotFound,
                "Uniform '" + std::string(name) + "' not found");
        }
        return it->second;
    }

    GLint location = glGetUniformLocation(program_, name.data());
    location_cache_[std::string(name)] = location;

    if (location == -1) {
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    return location;
}

auto MaterialOpenGL::set_uniform(std::string_view name, float value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform1f(*loc, value);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=float)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, int32_t value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform1i(*loc, value);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=int)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, bool value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform1i(*loc, value ? 1 : 0);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=bool)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform3fv(*loc, 1, &value.x);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Vec3)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    glUniform4fv(*loc, 1, &value.x);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Vec4)\n";
#endif
    return {};
}

auto MaterialOpenGL::set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> {
    auto loc = get_uniform_location(name);
    if (!loc) return std::unexpected(loc.error());
    // GLM/Mat4 is column-major; GL_FALSE means "don't transpose"
    glUniformMatrix4fv(*loc, 1, GL_FALSE, &value[0].x);
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=Mat4)\n";
#endif
    return {};
}

auto MaterialOpenGL::has_uniform(std::string_view name) const -> bool {
    // const_cast to call get_uniform_location on a const object
    // Or use a different approach: check the cache or call glGetUniformLocation directly
    GLint location = glGetUniformLocation(program_, name.data());
    return location != -1;
}

} // namespace buddd::engine
```

**Requirements:**
- `has_uniform` is const. Since `glGetUniformLocation` is thread-safe in OpenGL (read-only), calling it directly is fine.
- `set_uniform` for `Mat4` passes `&value[0].x` as the data pointer — this works because `Vec4` is standard layout and `Mat4` is an array of 4 `Vec4` columns, matching `glm::mat4` layout (column-major).
- `set_uniform` for `Vec3` passes `&value.x` as the data pointer, which `glUniform3fv` reads as 3 consecutive floats.

### 17. `src/engine/render/material_headless.h`

```cpp
#pragma once

#include "material.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace buddd::engine {

class MaterialHeadless final : public Material {
public:
    explicit MaterialHeadless(std::unordered_set<std::string> known_uniforms);
    ~MaterialHeadless() override = default;

    auto set_uniform(std::string_view name, float value) -> Result<void> override;
    auto set_uniform(std::string_view name, int32_t value) -> Result<void> override;
    auto set_uniform(std::string_view name, bool value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> override;
    auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> override;

    auto has_uniform(std::string_view name) const -> bool override;

    MaterialHeadless(const MaterialHeadless&) = delete;
    auto operator=(const MaterialHeadless&) -> MaterialHeadless& = delete;
    MaterialHeadless(MaterialHeadless&&) = delete;
    auto operator=(MaterialHeadless&&) -> MaterialHeadless& = delete;

private:
    std::unordered_set<std::string> known_uniforms_;
    std::unordered_map<std::string, std::variant<float, int32_t, bool, math::Vec3, math::Vec4, math::Mat4>> uniform_values_;
};

} // namespace buddd::engine
```

**Requirements:**
- `known_uniforms_` is populated at construction (from `extract_uniform_names()` + `known_uniforms` parameter in factory).
- `has_uniform(name)` returns `true` if `name` is in `known_uniforms_` OR if `name` has been previously set via `set_uniform`.
- `set_uniform` stores the value in `uniform_values_` for potential later inspection. Returns `UniformNotFound` error if the name is not in `known_uniforms_` and has not been previously set.
- Storage via `std::variant` enables headless state tracking without GPU involvement.

### 18. `src/engine/render/material_headless.cpp`

```cpp
#include "material_headless.h"

#include <iostream>

namespace buddd::engine {

MaterialHeadless::MaterialHeadless(std::unordered_set<std::string> known_uniforms)
    : known_uniforms_(std::move(known_uniforms)) {}

auto MaterialHeadless::has_uniform(std::string_view name) const -> bool {
    return known_uniforms_.count(std::string(name)) > 0
        || uniform_values_.count(std::string(name)) > 0;
}

auto MaterialHeadless::set_uniform_impl(std::string_view name) -> Result<void> {
    auto key = std::string(name);
    if (known_uniforms_.count(key) == 0 && uniform_values_.count(key) == 0) {
        std::cerr << "Uniform not found: " << name << "\n";
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    // Mark as known even if not previously registered
    known_uniforms_.insert(key);
    return {};
}
```

**Note on implementation:** Each `set_uniform` overload should:
1. Check if the name is known (from `known_uniforms_` or `uniform_values_`).
2. If not known, return `UniformNotFound` error.
3. If known, store the value in `uniform_values_` and return success.

The individual overloads follow this pattern:

```cpp
auto MaterialHeadless::set_uniform(std::string_view name, float value) -> Result<void> {
    auto key = std::string(name);
    if (known_uniforms_.count(key) == 0 && uniform_values_.count(key) == 0) {
        std::cerr << "Uniform not found: " << name << "\n";
        return make_error(Error::Category::UniformNotFound,
            "Uniform '" + std::string(name) + "' not found");
    }
    known_uniforms_.insert(key);
    uniform_values_[key] = value;
#ifndef NDEBUG
    std::cerr << "Uniform set: " << name << " (type=float)\n";
#endif
    return {};
}
```

(Same pattern for all 6 overloads — just the variant value type changes.)

### 19. `src/engine/render/vertex_buffer_opengl.h`

```cpp
#pragma once

#include "vertex_buffer.h"

#include <GL/gl.h>

namespace buddd::engine {

class VertexBufferOpenGL final : public VertexBuffer {
public:
    VertexBufferOpenGL(GLuint vao, GLuint vbo, VertexFormat format, size_t byte_size);
    ~VertexBufferOpenGL() override;

    auto format() const noexcept -> const VertexFormat& override;

    auto vao() const noexcept -> GLuint;
    auto vbo() const noexcept -> GLuint;

    VertexBufferOpenGL(const VertexBufferOpenGL&) = delete;
    auto operator=(const VertexBufferOpenGL&) -> VertexBufferOpenGL& = delete;
    VertexBufferOpenGL(VertexBufferOpenGL&&) = delete;
    auto operator=(VertexBufferOpenGL&&) -> VertexBufferOpenGL& = delete;

private:
    GLuint vao_;
    GLuint vbo_;
    VertexFormat format_;
    size_t byte_size_;
};

} // namespace buddd::engine
```

**Requirements:**
- Destructor calls `glDeleteBuffers(1, &vbo_)` and `glDeleteVertexArrays(1, &vao_)`.
- `vao()` and `vbo()` used by `RenderDeviceOpenGL::draw()`.

### 20. `src/engine/render/vertex_buffer_opengl.cpp`

```cpp
#include "vertex_buffer_opengl.h"

namespace buddd::engine {

VertexBufferOpenGL::VertexBufferOpenGL(GLuint vao, GLuint vbo, VertexFormat format, size_t byte_size)
    : vao_(vao), vbo_(vbo), format_(std::move(format)), byte_size_(byte_size) {}

VertexBufferOpenGL::~VertexBufferOpenGL() {
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

auto VertexBufferOpenGL::format() const noexcept -> const VertexFormat& {
    return format_;
}

auto VertexBufferOpenGL::vao() const noexcept -> GLuint {
    return vao_;
}

auto VertexBufferOpenGL::vbo() const noexcept -> GLuint {
    return vbo_;
}

} // namespace buddd::engine
```

### 21. `src/engine/render/index_buffer_opengl.h`

```cpp
#pragma once

#include "index_buffer.h"

#include <GL/gl.h>

namespace buddd::engine {

class IndexBufferOpenGL final : public IndexBuffer {
public:
    IndexBufferOpenGL(GLuint handle, IndexType type, size_t byte_size);
    ~IndexBufferOpenGL() override;

    auto type() const noexcept -> IndexType override;

    auto handle() const noexcept -> GLuint;
    auto index_type() const noexcept -> IndexType;
    auto byte_size() const noexcept -> size_t;

    IndexBufferOpenGL(const IndexBufferOpenGL&) = delete;
    auto operator=(const IndexBufferOpenGL&) -> IndexBufferOpenGL& = delete;
    IndexBufferOpenGL(IndexBufferOpenGL&&) = delete;
    auto operator=(IndexBufferOpenGL&&) -> IndexBufferOpenGL& = delete;

private:
    GLuint handle_;
    IndexType type_;
    size_t byte_size_;
};

} // namespace buddd::engine
```

**Requirements:**
- Destructor calls `glDeleteBuffers(1, &handle_)`.
- `index_type()` returns the `IndexType` (used by draw call to determine `GL_UNSIGNED_SHORT` vs `GL_UNSIGNED_INT`).

### 22. `src/engine/render/index_buffer_opengl.cpp`

Implementation follows the same pattern as `vertex_buffer_opengl.cpp` — constructor stores values, destructor calls `glDeleteBuffers`.

### 23. `src/engine/render/shader_headless.h`

```cpp
#pragma once

#include "shader.h"

#include <string>

namespace buddd::engine {

class ShaderHeadless final : public Shader {
public:
    ShaderHeadless(ShaderType type, std::string source);
    ~ShaderHeadless() override = default;

    auto type() const noexcept -> ShaderType override;

    auto source() const noexcept -> const std::string&;

    ShaderHeadless(const ShaderHeadless&) = delete;
    auto operator=(const ShaderHeadless&) -> ShaderHeadless& = delete;
    ShaderHeadless(ShaderHeadless&&) = delete;
    auto operator=(ShaderHeadless&&) -> ShaderHeadless& = delete;

private:
    ShaderType type_;
    std::string source_;
};

} // namespace buddd::engine
```

**Requirements:**
- `source()` returns the stored GLSL source string (used by `RenderDeviceHeadless::create_material` for linking error simulation and uniform name discovery).
- NO OpenGL or SDL3 includes.

### 24. `src/engine/render/vertex_buffer_headless.h`

```cpp
#pragma once

#include "vertex_buffer.h"

#include <cstddef>
#include <vector>

namespace buddd::engine {

class VertexBufferHeadless final : public VertexBuffer {
public:
    VertexBufferHeadless(VertexFormat format, std::span<const std::byte> data);
    ~VertexBufferHeadless() override = default;

    auto format() const noexcept -> const VertexFormat& override;

    VertexBufferHeadless(const VertexBufferHeadless&) = delete;
    auto operator=(const VertexBufferHeadless&) -> VertexBufferHeadless& = delete;
    VertexBufferHeadless(VertexBufferHeadless&&) = delete;
    auto operator=(VertexBufferHeadless&&) -> VertexBufferHeadless& = delete;

private:
    VertexFormat format_;
    std::vector<std::byte> data_;
};

} // namespace buddd::engine
```

**Requirements:**
- Stores vertex data in `std::vector<std::byte>` (no GPU upload).
- NO OpenGL or SDL3 includes.

### 25. `src/engine/render/index_buffer_headless.h`

```cpp
#pragma once

#include "index_buffer.h"

#include <cstddef>
#include <vector>

namespace buddd::engine {

class IndexBufferHeadless final : public IndexBuffer {
public:
    IndexBufferHeadless(IndexType type, std::span<const std::byte> data);
    ~IndexBufferHeadless() override = default;

    auto type() const noexcept -> IndexType override;

    IndexBufferHeadless(const IndexBufferHeadless&) = delete;
    auto operator=(const IndexBufferHeadless&) -> IndexBufferHeadless& = delete;
    IndexBufferHeadless(IndexBufferHeadless&&) = delete;
    auto operator=(IndexBufferHeadless&&) -> IndexBufferHeadless& = delete;

private:
    IndexType type_;
    std::vector<std::byte> data_;
};

} // namespace buddd::engine
```

**Requirements:**
- Stores index data in `std::vector<std::byte>` (no GPU upload).
- NO OpenGL or SDL3 includes.

## Required tests

The following tests MUST be present in the `buddd_tests` binary. They are specified here for the test-author who will create the test files. The implementation-author does NOT create test files.

### Headless render pipeline tests (always runnable, no display required)

| ID | Test name | Tags | Verification |
|---|---|---|---|
| RP-T-01 | `"ShaderType enum has Vertex and Fragment"` | `[headless]` `[shader]` | `ShaderType::Vertex` and `ShaderType::Fragment` are valid identifiers. |
| RP-T-02 | `"Headless create_shader succeeds with valid source"` | `[headless]` `[shader]` | `device->create_shader(ShaderType::Vertex, valid_glsl_source)` returns a valid `unique_ptr<Shader>`. |
| RP-T-03 | `"Headless create_shader fails with empty source"` | `[headless]` `[shader]` | `device->create_shader(ShaderType::Vertex, "")` returns an error with category `InvalidArgument`. |
| RP-T-04 | `"Headless create_shader fails with #error in source"` | `[headless]` `[shader]` | `device->create_shader(ShaderType::Vertex, "#error bad")` returns an error with category `ShaderCompilationFailed`. |
| RP-T-05 | `"Headless create_material succeeds with matching shaders"` | `[headless]` `[material]` | After creating valid VS and FS, `device->create_material(move(vs), move(fs))` returns a valid `unique_ptr<Material>`. |
| RP-T-06 | `"Headless create_material fails with null shaders"` | `[headless]` `[material]` | `device->create_material(nullptr, valid_fs_unique_ptr)` returns `InvalidArgument`. `device->create_material(valid_vs_unique_ptr, nullptr)` returns `InvalidArgument`. |
| RP-T-07 | `"Headless create_material fails with mismatched shader I/O"` | `[headless]` `[material]` | Create a vertex shader with `out vec4 v_color;` and a fragment shader with `in vec4 f_color;` (different names). `create_material` returns `LinkingFailed`. |
| RP-T-08 | `"Headless set_uniform succeeds for known uniform"` | `[headless]` `[material]` | After creating a material (with `known_uniforms` including `"u_test"`), `mat->set_uniform("u_test", 1.0f)` succeeds. |
| RP-T-09 | `"Headless set_uniform fails for unknown uniform"` | `[headless]` `[material]` | Calling `mat->set_uniform("nonexistent", 0.0f)` returns `UniformNotFound`. |
| RP-T-10 | `"Headless has_uniform returns true for known uniforms"` | `[headless]` `[material]` | `mat->has_uniform("u_test")` returns `true` for a uniform passed via `known_uniforms` or set via `set_uniform`. |
| RP-T-11 | `"Headless has_uniform returns false for unknown uniforms"` | `[headless]` `[material]` | `mat->has_uniform("nonexistent")` returns `false`. |
| RP-T-12 | `"Headless set_uniform all six overloads compile and return Result<void>"` | `[headless]` `[material]` | Call `set_uniform` with `float`, `int32_t`, `bool`, `math::Vec3`, `math::Vec4`, `math::Mat4` — all return `Result<void>`. |
| RP-T-13 | `"PrimitiveTopology enum values exist"` | `[headless]` `[primitive]` | All five values are valid identifiers. |
| RP-T-14 | `"VertexAttributeType enum values exist"` | `[headless]` `[vertex]` | All 11 values are valid identifiers. |
| RP-T-15 | `"VertexFormat struct compiles"` | `[headless]` `[vertex]` | `VertexFormat{32, {}}` compiles. |
| RP-T-16 | `"Headless create_vertex_buffer succeeds with valid data"` | `[headless]` `[vertex]` | `device->create_vertex_buffer(format, data_span)` with valid data returns success. |
| RP-T-17 | `"Headless create_vertex_buffer fails with empty data"` | `[headless]` `[vertex]` | `device->create_vertex_buffer(format, {})` returns `InvalidArgument`. |
| RP-T-18 | `"Headless create_vertex_buffer fails with zero stride"` | `[headless]` `[vertex]` | `device->create_vertex_buffer(VertexFormat{0, {attr}}, data)` returns `InvalidArgument`. |
| RP-T-19 | `"Headless create_vertex_buffer fails with zero attributes"` | `[headless]` `[vertex]` | `device->create_vertex_buffer(VertexFormat{12, {}}, data)` returns `InvalidArgument`. |
| RP-T-20 | `"IndexType enum values exist"` | `[headless]` `[index]` | `IndexType::Uint16` and `IndexType::Uint32` are valid identifiers. |
| RP-T-21 | `"Headless create_index_buffer succeeds with valid data"` | `[headless]` `[index]` | `device->create_index_buffer(data, IndexType::Uint16)` returns success. |
| RP-T-22 | `"Headless create_index_buffer fails with empty data"` | `[headless]` `[index]` | `device->create_index_buffer({}, IndexType::Uint16)` returns `InvalidArgument`. |
| RP-T-23 | `"Headless draw completes silently"` | `[headless]` `[draw]` | After creating VB, IB, and Material, `device->draw(Triangles, *vb, *mat, 3)` and `device->draw_indexed(Triangles, *vb, *ib, *mat, 6)` compile and do not crash. |
| RP-T-24 | `"Headless draw with vertex_count=0 is a no-op"` | `[headless]` `[draw]` | `device->draw(Triangles, *vb, *mat, 0)` does not crash. |
| RP-T-25 | `"Abstract classes are non-copyable and non-movable"` | `[headless]` `[compile]` | `static_assert(!std::is_copy_constructible_v<Shader>)` passes. Same for `Material`, `VertexBuffer`, `IndexBuffer`. |
| RP-T-26 | `"New Error::Category values compile"` | `[headless]` `[error]` | `Error::Category::ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound` are valid. |
| RP-T-27 | `"Uniform not found error message contains uniform name"` | `[headless]` `[material]` | `set_uniform("missing", 0.0f)` returns error whose `message` contains `"missing"`. |
| RP-T-28 | `"Headless set_uniform(bool) stores correct value"` | `[headless]` `[material]` | After `set_uniform("flag", true)`, `has_uniform("flag")` returns `true`. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| Shader compiled from empty source string | `create_shader` returns `InvalidArgument` error. |
| Material created with mismatched vertex/fragment shader I/O (OpenGL) | `create_material` returns `LinkingFailed` with link error log. |
| Material created with mismatched vertex/fragment shader I/O (Headless) | `create_material` returns `LinkingFailed` with simulated error message listing variable names. |
| Material created with a null or moved-from `unique_ptr<Shader>` | `create_material` returns `InvalidArgument`. |
| `set_uniform` with a name that exists but with wrong GLSL type | No type checking. OpenGL backend silently succeeds. Headless silently succeeds. Caller responsibility. |
| `set_uniform` on a material that was never successfully linked | Precondition violation — undefined behaviour (material creation already failed). |
| `draw` or `draw_indexed` outside `begin_frame()`/`end_frame()` | Undefined behaviour at the abstract level. OpenGL: may produce GL errors or crash. Headless: silently succeeds. |
| `draw` with `vertex_count = 0` | No-op — nothing is drawn. No error. |
| `draw` with `start_vertex` beyond the vertex buffer's capacity | Undefined behaviour (OpenGL reads out-of-bounds GPU memory). Headless: silently succeeds. |
| `draw_indexed` with index values exceeding vertex count | Undefined behaviour (same as above). |
| `VertexFormat` with zero attributes | `create_vertex_buffer` returns `InvalidArgument`. |
| `VertexFormat` with stride of zero | `create_vertex_buffer` returns `InvalidArgument`. |
| Multiple draw calls with same vertex buffer but different materials | Each draw call binds the material's program before issuing the command. Legal and expected. |
| Material used across multiple frames | Uniform values persist across frames until changed. Legal and expected. |
| Headless `has_uniform` after material creation | Returns `true` for names in `known_uniforms_` (from shader parsing + `known_uniforms` parameter) OR names previously set via `set_uniform`. |
| Headless `draw`/`draw_indexed` with any `PrimitiveTopology` value | No-op — no validation of topology value required. |

## Security impact

- No elevated privileges are required to create shaders, materials, or buffers.
- Shader source is provided at runtime by the application — the engine does not load shader files from disk.
- No network access, secrets, or credentials are involved in the render pipeline.
- The headless backend requires no GPU or display access, maintaining CI safety.
- GLSL shader compilation is inherently unsafe at the driver level (malicious shaders could crash the GPU driver or, in extreme cases, cause system instability). The engine does not validate shader source beyond what the GLSL compiler provides. Application developers are responsible for the shader source they provide.
- The architecture boundary (CONST-001) remains in full effect: no code outside `src/engine/` may include OpenGL, SDL3, or GLM headers. All new abstract headers (`shader.h`, `material.h`, `vertex_buffer.h`, `index_buffer.h`, `vertex_format.h`, `primitive_topology.h`) expose no backend types.

## Data and migration impact

None. No persistent state, database, or file format is introduced. The render pipeline abstractions are purely in-memory runtime constructs.

## API compatibility impact

The following public API surface is added or modified:

### Additions to `Error::Category`
```cpp
enum class Category {
    // ... existing values unchanged ...
    ShaderCompilationFailed,   // new
    LinkingFailed,             // new
    ResourceCreationFailed,    // new
    InvalidArgument,           // new
    UniformNotFound,           // new
    // ... Unsupported, Unknown unchanged ...
};
```

### New value types (no virtual methods)
```cpp
enum class ShaderType { Vertex, Fragment };
enum class PrimitiveTopology { Triangles, TriangleStrip, Lines, LineStrip, Points };
enum class VertexAttributeType { Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, UByte, UByte4, UByte4Norm };
enum class IndexType { Uint16, Uint32 };
struct VertexAttribute { uint32_t location; VertexAttributeType type; uint32_t offset; bool normalized = false; };
struct VertexFormat { uint32_t stride; std::vector<VertexAttribute> attributes; };
```

### New abstract types
```cpp
class Shader {
    virtual ~Shader() = default;
    virtual auto type() const noexcept -> ShaderType = 0;
};

class Material {
    virtual ~Material() = default;
    virtual auto set_uniform(std::string_view name, float value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, int32_t value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, bool value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Vec3& value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Vec4& value) -> Result<void> = 0;
    virtual auto set_uniform(std::string_view name, const math::Mat4& value) -> Result<void> = 0;
    virtual auto has_uniform(std::string_view name) const -> bool = 0;
};

class VertexBuffer {
    virtual ~VertexBuffer() = default;
    virtual auto format() const noexcept -> const VertexFormat& = 0;
};

class IndexBuffer {
    virtual ~IndexBuffer() = default;
    virtual auto type() const noexcept -> IndexType = 0;
};
```

### Additions to `RenderDevice`
```cpp
virtual auto create_shader(ShaderType type, std::string_view source) -> Result<std::unique_ptr<Shader>> = 0;
virtual auto create_material(
    std::unique_ptr<Shader> vertex_shader,
    std::unique_ptr<Shader> fragment_shader,
    std::span<const std::string> known_uniforms = {}
) -> Result<std::unique_ptr<Material>> = 0;
virtual auto create_vertex_buffer(const VertexFormat& format, std::span<const std::byte> data) -> Result<std::unique_ptr<VertexBuffer>> = 0;
virtual auto create_index_buffer(std::span<const std::byte> data, IndexType type) -> Result<std::unique_ptr<IndexBuffer>> = 0;
virtual auto draw(PrimitiveTopology topology, const VertexBuffer& vertices, const Material& material, uint32_t vertex_count, uint32_t start_vertex = 0) -> void = 0;
virtual auto draw_indexed(PrimitiveTopology topology, const VertexBuffer& vertices, const IndexBuffer& indices, const Material& material, uint32_t index_count, uint32_t start_index = 0) -> void = 0;
```

**Breaking changes**: Adding pure virtual methods to `RenderDevice` is a breaking change for any existing concrete subclass. The only concrete subclasses are `RenderDeviceOpenGL` and `RenderDeviceHeadless`, both of which are modified in this contract. No other code is affected.

**Backward compatibility**: All additions are additive to the `Error::Category` enum (new values appended before `Unknown`). The new types and classes are entirely new — no existing types are modified.

## Documentation impact

- No README, wiki page, or other documentation files are created or modified.
- The spec (`docs/specs/render-pipeline/spec.md`) remains authoritative.
- The `SpecKit.md` and `AGENTS.md` remain untouched.
- The API surface described above is the public contract.

## ADR impact

- **ADR-001** (`Result<T>` pattern): This contract extends `Error::Category` with new values (consistent with ADR-001's "The `Error::Category` enum is extensible" clause). The `draw()`/`draw_indexed()` methods return `void` as a deliberate exception to the general `Result<T>` pattern. No ADR amendment is required — the exception is documented in this contract and in the spec.
- **ADR-002** (GLM wrapper math): The `set_uniform` overloads use `math::Vec3`, `math::Vec4`, and `math::Mat4`, which are consistent with ADR-002.

## Constitution impact

None. No constitution rules need to be added or amended. CONST-001 (architecture boundaries) is maintained — all new abstract headers are backend-type-free. CONST-002 (testing policy) is satisfied by the required tests listed above.

## Done criteria

The implementation is complete when all of the following are true:

1. **Files exist**: All 22 new files listed in "Files allowed to change" exist with correct content.
2. **Files modified**: All 5 modified files have the correct changes described in the Required implementation behavior section.
3. **Build succeeds**: `cmake --preset debug && cmake --build --preset debug` exits 0 with zero warnings related to the new render pipeline source files.
4. **Release preset works**: `cmake --preset release && cmake --build --preset release` succeeds.
5. **Architecture boundary verified**: Running `grep -E '(GL_|gl[A-Z]|SDL_|GLAD)' src/engine/render/shader.h src/engine/render/material.h src/engine/render/vertex_buffer.h src/engine/render/index_buffer.h src/engine/render/vertex_format.h src/engine/render/primitive_topology.h` returns **zero matches**. No OpenGL or SDL3 types leak into public headers.
6. **No OpenGL/SDL3 in headless files**: Running `grep -E '(GL_|gl[A-Z]|SDL_)' src/engine/render/*_headless.*` returns **zero matches**.
7. **New Error::Category values compile**: `Error::Category::ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound` are all valid identifiers.
8. **All enums compile**: `ShaderType`, `PrimitiveTopology`, `VertexAttributeType`, `IndexType` are valid.
9. **Abstract classes are non-copyable and non-movable**: A `static_assert(!std::is_copy_constructible_v<Shader>)` in a test would pass. Same for `Material`, `VertexBuffer`, `IndexBuffer`.
10. **Headless full cycle compiles**: Code using the headless backend — `Platform::create(Headless)` → `create_window()` → `RenderDevice::create()` → `create_shader()` → `create_material()` → `create_vertex_buffer()` → `draw()` → `begin_frame()` → `end_frame()` — compiles without error.
11. **Forbidden files unchanged**: `git diff --name-only` does NOT include `src/engine/version.h`, `src/engine/version.cpp`, root `CMakeLists.txt`, `CMakePresets.json`, or anything in `src/editor/`, `tests/`.
12. **No modifications to files outside allowed list**: `git diff --name-only` lists only the files in "Files allowed to change".
13. **Default mode (interactive) works**: `./build/debug/src/cmd/buddd` runs on a system with a display, opens a window with a coloured triangle, stays open until the user closes it, and exits with code 0. (Manual visual verification.)
14. **`--test` mode works**: `./build/debug/src/cmd/buddd --test` runs on a system with a display, renders a triangle for exactly 120 frames, and exits with code 0. (Manual visual verification: a coloured triangle should appear in the window for ~2 seconds.)

## Verification commands (copy-paste ready)

```bash
# Configure and build
cmake --preset debug
cmake --build --preset debug

# Verify architecture boundary (no leaks in public headers)
grep -E '(GL_|gl[A-Z]|SDL_|GLAD)' src/engine/render/shader.h src/engine/render/material.h src/engine/render/vertex_buffer.h src/engine/render/index_buffer.h src/engine/render/vertex_format.h src/engine/render/primitive_topology.h
# Expected: zero matches

# Verify no OpenGL/SDL3 in headless files
grep -E '(GL_|gl[A-Z]|SDL_)' src/engine/render/*_headless.*
# Expected: zero matches

# Verify forbidden files are unchanged
git diff --name-only
# Should NOT include: version.h, version.cpp, root CMakeLists.txt, CMakePresets.json, anything in src/cmd/, src/editor/, tests/

# Build release preset (optional, P2)
cmake --preset release
cmake --build --preset release
```
