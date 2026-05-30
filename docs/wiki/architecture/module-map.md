# Module Map

## Overview

The project is composed of four CMake targets organized into four source directories. Each target has a specific role within the architecture.

## `buddd_engine` — Static library (`src/engine/`)

The engine library is the core of the project. It provides a version API, a math foundations module, and a platform abstraction layer. All source files under `src/engine/` are collected automatically via `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` in `CMakeLists.txt`.

### Version module

| File | Role |
|---|---|
| `version.h` | Public header: declares `buddd::engine::version() -> std::string_view` |
| `version.cpp` | Implementation: returns `"0.1.0"` |

### Error handling module

| File | Role |
|---|---|
| `error.h` | Public header: defines `Error` struct (with `Category` enum, `int code`, `std::string message`), `to_string()`, `make_error()`, and `Result<T>` alias (`std::expected<T, Error>`) |

### Math submodule (`math/`)

All types in namespace `buddd::engine::math`. The math module wraps GLM (`glm`) with zero-overhead C++ wrapper types — header-only (except `Camera`). GLM headers are included only inside `src/engine/math/`.

| File | Role |
|---|---|
| `math.h` | Convenience header: includes all math types, provides `radians()`, `degrees()`, math constants (`pi`, `half_pi`, `two_pi`, `epsilon`), and common math functions (`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sqrt`) |
| `vec2.h` | `Vec2` struct — 2D vector (x, y), wrapper around `glm::vec2`. Header-only. |
| `vec3.h` | `Vec3` struct — 3D vector (x, y, z), wrapper around `glm::vec3`. Header-only. |
| `vec4.h` | `Vec4` struct — 4D vector (x, y, z, w), wrapper around `glm::vec4`. Header-only. |
| `mat4.h` | `Mat4` struct — 4×4 column-major matrix, wrapper around `glm::mat4`. Header-only. |
| `quat.h` | `Quat` struct — quaternion (w, x, y, z), wrapper around `glm::quat`. Header-only. |
| `camera.h` | `Camera` class — perspective camera with position, orientation, and perspective parameters. Computes view, projection, and view-projection matrices. |
| `camera.cpp` | Camera method implementations (only type with a `.cpp` file). Contains GLM includes for implementation only. |

Each wrapper type provides a `.glm()` accessor for zero-overhead GLM interop, guarded by `static_assert(std::is_standard_layout_v<T>)`, `static_assert(sizeof(T) == sizeof(GLMType))`, and `static_assert(std::is_trivially_copyable_v<T>)`.

### Platform submodule (`platform/`)

| File | Role |
|---|---|
| `platform.h` | Public header: `Backend` enum (`SDL3`, `Headless`), abstract `Platform` class with `create(Backend)` static factory |
| `platform.cpp` | Factory implementation: dispatches to SDL3 or Headless backend based on `Backend` enum |
| `platform_sdl3.h` | Private header: `PlatformSDL3` concrete class (final) |
| `platform_sdl3.cpp` | SDL3 backend: `SDL_Init`/`SDL_Quit` lifecycle, `SDL_CreateWindow` delegation |
| `platform_headless.h` | Private header: `PlatformHeadless` concrete class (final) |
| `platform_headless.cpp` | Headless implementation: no SDL3/OpenGL dependency, validates dimensions |

### Window submodule (`window/`)

| File | Role |
|---|---|
| `window.h` | Public header: `WindowConfig` struct (`title`, `width`, `height`), abstract `Window` class with width/height getters and `native_handle()` |
| `window_sdl3.h` | Private header: `WindowSDL3` concrete class wrapping `SDL_Window*` |
| `window_sdl3.cpp` | SDL3 implementation: `SDL_DestroyWindow` on destruction, `native_handle()` casts to `void*` |
| `window_headless.h` | Private header: `WindowHeadless` concrete class |
| `window_headless.cpp` | Headless implementation: stores width/height, `native_handle()` returns `nullptr` |

### Render submodule (`render/`)

The render submodule now provides a full pipeline abstraction: shader compilation, material linking, vertex/index buffer management, and draw calls. All abstract types are backend-agnostic; concrete implementations exist for OpenGL 4.5 Core and Headless.

| File | Role |
|---|---|
| `primitive_topology.h` | Public header: `PrimitiveTopology` enum (`Triangles`, `TriangleStrip`, `Lines`, `LineStrip`, `Points`). Header-only value type. |
| `vertex_format.h` | Public header: `VertexAttributeType` enum (11 types), `VertexAttribute` struct, `VertexFormat` struct. Header-only value types. |
| `shader.h` | Public header: `ShaderType` enum (`Vertex`, `Fragment`), abstract `Shader` class with `type()` pure virtual. Non-copyable, non-movable. |
| `material.h` | Public header: abstract `Material` class with 6 `set_uniform` overloads (`float`, `int32_t`, `bool`, `math::Vec3`, `math::Vec4`, `math::Mat4`) and `has_uniform()`. Non-copyable, non-movable. |
| `vertex_buffer.h` | Public header: abstract `VertexBuffer` class with `format()` pure virtual. Non-copyable, non-movable. |
| `index_buffer.h` | Public header: `IndexType` enum (`Uint16`, `Uint32`), abstract `IndexBuffer` class with `type()` pure virtual. Non-copyable, non-movable. |
| `render_device.h` | Public header: abstract `RenderDevice` class with `create(Window&)` static factory, `begin_frame()`, `end_frame()`, `size()`, resource factory methods (`create_shader`, `create_material`, `create_vertex_buffer`, `create_index_buffer`), and draw methods (`draw`, `draw_indexed`). Draw methods return `void` — deliberate exception to ADR-001. |
| `render_device.cpp` | Factory implementation: dispatches to OpenGL or Headless backend based on `native_handle()` value |
| `render_device_opengl.h` | Private header: `RenderDeviceOpenGL` concrete class wrapping `SDL_Window*` and `SDL_GLContext` |
| `render_device_opengl.cpp` | OpenGL 4.5 Core implementation: GLSL compilation via `glCreateShader`/`glCompileShader`, program linking via `glCreateProgram`/`glLinkProgram`, VAO/VBO/IBO management via DSA APIs (`glCreateVertexArrays`, `glNamedBufferStorage`, etc.), and draw dispatch via `glDrawArrays`/`glDrawElements` |
| `render_device_headless.h` | Private header: `RenderDeviceHeadless` concrete class with diagnostic counters |
| `render_device_headless.cpp` | Headless implementation: stores shader source and vertex data in memory; simulates compilation errors via `#error` marker and linking errors via vertex/fragment I/O mismatch detection; draw calls are no-ops |
| `shader_opengl.h` | Private header: `ShaderOpenGL` concrete class wrapping a `GLuint` shader handle |
| `shader_opengl.cpp` | OpenGL shader backend: resource lifetime managed via `glCreateShader`/`glDeleteShader` |
| `shader_headless.h` | Private header: `ShaderHeadless` concrete class storing type and GLSL source string |
| `shader_headless.cpp` | Headless shader backend: stores source for linking-error simulation and uniform discovery |
| `material_opengl.h` | Private header: `MaterialOpenGL` concrete class with `glGetUniformLocation`-based uniform management and location caching |
| `material_opengl.cpp` | OpenGL material backend: uniform dispatch via `glUniform1f`/`glUniform1i`/`glUniform3fv`/`glUniform4fv`/`glUniformMatrix4fv`; program destruction via `glDeleteProgram` |
| `material_headless.h` | Private header: `MaterialHeadless` concrete class with `std::unordered_set` of known uniform names and `std::variant`-based uniform value storage |
| `material_headless.cpp` | Headless material backend: in-memory uniform state tracking; `has_uniform` checks known names + previously-set names; `set_uniform` returns `UniformNotFound` for unknown names |
| `vertex_buffer_opengl.h` | Private header: `VertexBufferOpenGL` wrapping VAO and VBO handles |
| `vertex_buffer_opengl.cpp` | OpenGL vertex buffer backend: VAO/VBO creation via `glCreateVertexArrays`/`glCreateBuffers`, attribute configuration via `glVertexArrayAttribFormat`, `glVertexArrayVertexBuffer`, `glVertexArrayAttribBinding` |
| `vertex_buffer_headless.h` | Private header: `VertexBufferHeadless` storing format and vertex data in memory |
| `vertex_buffer_headless.cpp` | Headless vertex buffer backend: data stored in `std::vector<std::byte>` |
| `index_buffer_opengl.h` | Private header: `IndexBufferOpenGL` wrapping IBO handle |
| `index_buffer_opengl.cpp` | OpenGL index buffer backend: buffer creation via `glCreateBuffers`/`glNamedBufferStorage` |
| `index_buffer_headless.h` | Private header: `IndexBufferHeadless` storing type and index data in memory |
| `index_buffer_headless.cpp` | Headless index buffer backend: data stored in `std::vector<std::byte>` |

The library exposes a PUBLIC include directory of `${CMAKE_CURRENT_SOURCE_DIR}` (i.e., `src/engine/`), allowing consumers to `#include "error.h"`, `#include "platform/platform.h"`, etc.

## `buddd` — CLI executable (`src/cmd/`)

The command-line binary. Links `buddd_engine` as PRIVATE.

Uses a Command pattern: each subcommand is extracted into its own `.h`/`.cpp` pair under `src/cmd/commands/`, and `main.cpp` is a thin dispatcher (single if/else-if chain). Per-demo files live under `src/cmd/demos/` with shared helper code for demos.

### Build system

`src/cmd/CMakeLists.txt` uses `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)` covering `src/cmd/*.cpp` (for `main.cpp`), `src/cmd/commands/*.cpp` (for command files), and `src/cmd/demos/*.cpp` (for per-demo files). New commands can be added by creating files in `src/cmd/commands/` and wiring them into the dispatch — no CMakeLists.txt change needed. New demos can be added by creating files in `src/cmd/demos/` and adding a dispatch branch in `DemoCommand`.

### File structure

| File | Role |
|---|---|
| `main.cpp` | Thin dispatcher: parse first positional argument, dispatch to the matching command via if/else-if chain, return its exit code. No static helper functions, no engine header includes. |

### Command files (`src/cmd/commands/`)

| File | Role |
|---|---|
| `version_command.h` / `version_command.cpp` | `buddd::cmd::VersionCommand` — prints `buddd <version>` from `be::version()` to stdout and exits 0. Extra args silently ignored. |
| `demo_command.h` / `demo_command.cpp` | `buddd::cmd::DemoCommand` — parses a demo name from `argv[2]`, opens 800×600 window titled "Buddd Engine — Demo: \<name\>", dispatches to the matching per-demo function. Prints usage if no name is given or if the demo name is unknown. Warns on stderr if extra args follow the demo name. |
| `run_command.h` / `run_command.cpp` | `buddd::cmd::RunCommand` — opens 1024×768 window titled "Buddd Engine", clears the framebuffer each frame (no draw calls), interactive until user closes the window. Extra args silently ignored. |
| `help_command.h` / `help_command.cpp` | `buddd::cmd::HelpCommand` — prints usage text to stdout and exits 0. Also defines `k_usage_text` constant used by the unknown-command handler in `main.cpp`. Extra args silently ignored. |

### Demo files (`src/cmd/demos/`)

Each demo is a `.h`/`.cpp` pair exposing a single free function in the `buddd::cmd::demo` namespace. The per-demo function receives a `Platform&`, `RenderDevice&`, and `argc`/`argv` (where `argv[0]` is the demo name).

| File | Role |
|---|---|
| `demo_helpers.h` / `demo_helpers.cpp` | **Moved** from `src/cmd/`. Header declaring `buddd::cmd::demo::setup_triangle()` — shared helper for rendering a coloured triangle (used by `triangle_demo`). |
| `triangle_demo.h` / `triangle_demo.cpp` | Declares `buddd::cmd::demo::run_triangle_demo()` — 120-frame render loop with a coloured triangle (extracted from the old `test_command.cpp`). |

### Subcommand behavior

- `buddd` (no arguments) or `buddd run` → opens 1024×768 window, empties framebuffer each frame (no draw calls), runs until user closes window
- `buddd demo <name>` → opens 800×600 window titled "Buddd Engine — Demo: \<name\>", runs the named demo, then exits. Currently available: `triangle` (120 frames). If no name is given, prints usage to stderr and exits 1. If the name is unknown, prints error to stderr and exits 1.
- `buddd version` → prints `buddd 0.1.0` to stdout
- `buddd help` → prints usage information listing all four commands (`run`, `demo`, `version`, `help`)
- Unknown command → prints `"Unknown command: '<cmd>'"` followed by updated usage to stderr, exits with code 1
- `buddd test` is **removed** — it produces an unknown command error (use `buddd demo triangle` instead)
- Old `--test` and `--version` flags are **dropped** — they produce an unknown command error

## `buddd_editor` — INTERFACE library placeholder (`src/editor/`)

A placeholder for the future editor application. Currently defines an INTERFACE library target with no sources, no dependencies, and no include directories. No binary is produced.

## `buddd_tests` — Test executable (`tests/`)

The unit test binary. Links `buddd_engine` (PRIVATE) and `Catch2::Catch2WithMain` (PRIVATE). Catch2 provides its own `main()` entry point.

| File | Role |
|---|---|---|
| `version_test.cpp` | Single Catch2 test: `"engine version is non-empty"` tagged `[sanity]` |
| `platform_abstraction_test.cpp` | Headless platform tests (T-01 through T-12), always compiled |
| `sdl3_backend_test.cpp` | SDL3 backend tests (conditionally compiled with `BUDDD_HAS_DISPLAY=ON`) |
| `math_test.cpp` | Math foundations tests (T-01 through T-71): Vec2, Vec3, Vec4, Mat4, Quat, Camera, utilities, interop, and edge cases |

## Source naming conventions

- Source files: `snake_case` (e.g., `version.h`, `main.cpp`, `version_test.cpp`)
- Directories: `snake_case` (e.g., `src/engine/`, `src/cmd/`, `tests/`)
- CMake target names: `snake_case` (e.g., `buddd_engine`, `buddd_tests`)
- Test case names: sentence case (e.g., `"engine version is non-empty"`)

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Goals, Conventions, Directory structure
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 3-10 (individual target specifications)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — Platform, Window, RenderDevice module definitions
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — File directory structure, Existing conventions to follow
- Spec: [SPEC-004](/docs/specs/math-foundations/spec.md) — Math type specifications, memory layout, operations, GLM integration
- Implementation contract: [IMPL-004](/docs/specs/math-foundations/implementation-contract.md) — File list, header structure, delegation pattern
- Spec: [SPEC-005](/docs/specs/render-pipeline/spec.md) — Shader, Material, VertexBuffer, IndexBuffer, PrimitiveTopology, CLI modes
- Implementation contract: [IMPL-005](/docs/specs/render-pipeline/implementation-contract.md) — File directory structure, open questions, draw-methods-as-void exception
- Spec: [SPEC-006](/docs/specs/cli-command-system/spec.md) — CLI Command System: Command pattern, subcommand structure, file layout
- Implementation contract: [IMPL-006](/docs/specs/cli-command-system/implementation-contract.md) — File list, dispatch logic, CMake glob, CONST-001 compliance
- Spec: [SPEC-007](/docs/specs/cli-command-evolution/spec.md) — CLI Command Evolution: Demo System & Empty Run
- Implementation contract: [IMPL-007](/docs/specs/cli-command-evolution/implementation-contract.md) — Replacement of TestCommand with DemoCommand, per-demo files, RunCommand simplification
