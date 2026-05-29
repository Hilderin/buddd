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

| File | Role |
|---|---|
| `render_device.h` | Public header: abstract `RenderDevice` class with `create(Window&)` static factory, `begin_frame()`, `end_frame()`, `size()` |
| `render_device.cpp` | Factory implementation: dispatches to OpenGL or Headless backend based on `native_handle()` value |
| `render_device_opengl.h` | Private header: `RenderDeviceOpenGL` concrete class wrapping `SDL_Window*` and `SDL_GLContext` |
| `render_device_opengl.cpp` | OpenGL 4.5 Core implementation: `glClear` on begin, `SDL_GL_SwapWindow` on end, `SDL_GL_DestroyContext` on destruction |
| `render_device_headless.h` | Private header: `RenderDeviceHeadless` concrete class |
| `render_device_headless.cpp` | Headless implementation: all methods no-op except `size()` |

The library exposes a PUBLIC include directory of `${CMAKE_CURRENT_SOURCE_DIR}` (i.e., `src/engine/`), allowing consumers to `#include "error.h"`, `#include "platform/platform.h"`, etc.

## `buddd` — CLI executable (`src/cmd/`)

The command-line binary. Links `buddd_engine` as PRIVATE.

| File | Role |
|---|---|
| `main.cpp` | Entry point: parses `argc`/`argv`, calls `buddd::engine::version()`, prints output |

Behavior:
- No arguments → prints `Buddd Engine v0.1.0`
- `--version` as sole argument → prints `buddd 0.1.0`
- Any other argument combination → falls through to the greeting branch

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
