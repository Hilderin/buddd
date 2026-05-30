# Dependency Map

## Target dependencies

```
buddd ──PRIVATE──► buddd_engine ──PUBLIC──► SDL3::SDL3
                       │                    ├──► OpenGL::GL
                       │                    ├──► glm::glm
                       │                    └──► stb (PRIVATE, FetchContent)
                       │
buddd_tests ──PRIVATE─┤
               ──PRIVATE──► Catch2::Catch2WithMain (external)
                           │
buddd_editor            (standalone, no dependencies)
```

| Source target | Dependency | Link type | Notes |
|---|---|---|---|
| `buddd` | `buddd_engine` | PRIVATE | CLI needs the engine's version and platform APIs |
| `buddd_editor` | *(none)* | — | INTERFACE placeholder, no compiled code |
| `buddd_tests` | `buddd_engine` | PRIVATE | Tests exercise engine library code |
| `buddd_tests` | `Catch2::Catch2WithMain` | PRIVATE | Catch2 provides `main()` and test runner |
| `buddd_engine` | `SDL3::SDL3` | PUBLIC | SDL3 library for windowing, input, and GL context management |
| `buddd_engine` | `OpenGL::GL` | PUBLIC | OpenGL rendering (4.5 Core profile) |
| `buddd_engine` | `glm::glm` | PUBLIC | GLM header-only math library — provides underlying implementation for Vec2, Vec3, Vec4, Mat4, Quat wrappers |
| `buddd_engine` | `stb` | PRIVATE | Single-header public-domain library for PNG I/O (stb_image + stb_image_write). Fetched via FetchContent. |

## External dependencies

| Dependency | Version | Source | Fetch method |
|---|---|---|---|
| **Catch2** | v3.7.0 | `https://github.com/catchorg/Catch2.git` | CMake `FetchContent` (automatic download at configure time) |
| **SDL3** | release-3.2.30 | `https://github.com/libsdl-org/SDL.git` | CMake `FetchContent` (automatic download at configure time) |
| **GLM** | 1.0.1 | `https://github.com/g-truc/glm.git` | CMake `FetchContent` (automatic download at configure time) — header-only, no compiled library |
| **OpenGL** | 4.5 Core | System-provided (`libgl-dev` or equivalent) | `find_package(OpenGL REQUIRED)` |
| **stb** | `31c1ad37456438565541f9958214b6e762fb4` | `https://github.com/nothings/stb.git` | CMake `FetchContent` (automatic download at configure time) — header-only, only `#include` path needed. |

- Catch2, SDL3, and GLM are fetched once and cached in the build directory; subsequent configures use the cached copy.
- Compiled dependencies that are large or slow to debug (notably SDL3) are built with `-DCMAKE_BUILD_TYPE=Release` via `CMAKE_ARGS` in their `FetchContent_Declare` to avoid debugger startup slowness from their debug symbols. Header-only dependencies (GLM) are unaffected. See `src/engine/CMakeLists.txt` for the SDL3 declaration, [ADR-007](/docs/adr/007-release-dependency-build.md) for the full rationale, and the [setup guide](/docs/wiki/engineering/setup.md) for details.
- No network access is required after the initial fetch of each dependency.
- SDL3 is linked to `buddd_engine` as PUBLIC so that all consumers of the engine have access to SDL3 headers (only within `src/engine/` — external consumers must not include them directly).
- The headless backend of the platform layer has **zero** external dependencies — it uses only the C++ standard library and is always compiled.

## Build toolchain dependencies

| Tool | Minimum version | Required for |
|---|---|---|
| CMake | 3.28 | Build configuration, presets, FetchContent |
| Ninja | 1.11 | Build execution |
| C++ compiler (GCC 14+ / Clang 19+) | C++26 support | Compilation |
| clang-format | 18 (optional) | `cmake --build ... --target format` |

## System dependencies for OpenGL

The project requires OpenGL 4.5 Core profile headers at build time. On Linux these are typically provided by `libgl-dev` or `mesa-common-dev` packages. The build will fail at configure time if OpenGL is not found.

## Key constraints

- The engine is a **static library** (`STATIC`), not header-only. This may change in the future.
- The editor target produces **no binary** and links **nothing** — it is a structural placeholder.
- Catch2 is **not** a dependency of the engine or the CLI — only of the test binary.
- The headless backend has **zero** external dependencies and is always compiled alongside the SDL3+OpenGL backend.
- `buddd_engine` links `SDL3::SDL3`, `OpenGL::GL`, and `glm::glm` as **PUBLIC** so that consumers inheriting the include paths can use SDL3, OpenGL, and GLM types **inside** `src/engine/` only.
- GLM is header-only — no compiled library, no system dependency. GLM types are never to be included directly outside `src/engine/math/` (enforced by code review).
- stb is a **PRIVATE** dependency of `buddd_engine`, included only in `src/engine/image/`. It is not exposed outside the engine and is not accessible to consumers of the engine library.

## Architecture boundary

A hard architecture boundary is enforced by convention: **no code outside `src/engine/`** may `#include <SDL3/`, `<GL/`, `<glad/`, or any graphics-library header. Similarly, **no code outside `src/engine/math/`** may include any `glm/` header directly — all math access goes through the wrapper types (`Vec2`, `Vec3`, `Vec4`, `Mat4`, `Quat`, `Camera`). Violations are caught by code review (automated enforcement is a future goal).

The GLM boundary specifically:
- GLM headers may be included inside `src/engine/math/` (the wrapper headers and `camera.cpp`).
- Outside `src/engine/math/`, all math operations go through the wrapper types — the `.glm()` accessor is the sole interop path.
- Test files comparing against GLM reference output include GLM headers directly; this is acknowledged as a design tension but accepted at this stage (no automated guard).

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Assumptions A-05 through A-10
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 3-10 (target definitions)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — Architecture boundary, Goals, Assumptions
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — CMakeLists.txt requirements, Done criteria
- Spec: [SPEC-004](/docs/specs/math-foundations/spec.md) — Architecture boundary (no GLM outside `src/engine/math/`), GLM integration
- Implementation contract: [IMPL-004](/docs/specs/math-foundations/implementation-contract.md) — Files allowed to change, Architecture boundary enforcement
- Spec: [SPEC-010](/docs/specs/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario)
- Implementation contract: [IMPL-010](/docs/specs/capture/implementation-contract.md)
