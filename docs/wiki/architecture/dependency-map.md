# Dependency Map

## Target dependencies

```
buddd ──PRIVATE──► buddd_engine ──PUBLIC──► SDL3::SDL3
                      │                    └──► OpenGL::GL
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

## External dependencies

| Dependency | Version | Source | Fetch method |
|---|---|---|---|
| **Catch2** | v3.7.0 | `https://github.com/catchorg/Catch2.git` | CMake `FetchContent` (automatic download at configure time) |
| **SDL3** | release-3.2.30 | `https://github.com/libsdl-org/SDL.git` | CMake `FetchContent` (automatic download at configure time) |
| **OpenGL** | 4.5 Core | System-provided (`libgl-dev` or equivalent) | `find_package(OpenGL REQUIRED)` |

- Catch2 and SDL3 are fetched once and cached in the build directory; subsequent configures use the cached copy.
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
- `buddd_engine` links `SDL3::SDL3` and `OpenGL::GL` as **PUBLIC** so that consumers inheriting the include paths can use SDL3 and OpenGL types **inside** `src/engine/` only.

## Architecture boundary

A hard architecture boundary is enforced by convention: **no code outside `src/engine/`** may `#include <SDL3/`, `<GL/`, `<glad/`, or any graphics-library header. All platform/graphics access goes through the abstract `Platform`, `Window`, and `RenderDevice` interfaces. Violations are caught by code review (automated enforcement is a future goal).

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Assumptions A-05 through A-10
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 3-10 (target definitions)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — Architecture boundary, Goals, Assumptions
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — CMakeLists.txt requirements, Done criteria
