# Architecture Overview

## Project

**Buddd Engine** is a C++26 game engine. The project provides a reproducible CMake + Ninja build system, a directory structure that separates concerns across four CMake targets, and a platform abstraction layer that decouples engine code from windowing and graphics libraries.

## Directory layout

```
buddd2/
├── CMakeLists.txt          # Root CMake configuration
├── CMakePresets.json        # Build presets (debug, release)
├── .clang-format            # Code formatting rules (LLVM-based)
├── .vscode/                 # VS Code workspace configuration
│   ├── settings.json
│   ├── tasks.json
│   └── launch.json
├── src/
│   ├── engine/              # Engine library (static lib)
│   │   ├── error.h          # Project-wide Error/Result types
│   │   ├── platform/        # Platform abstraction (Platform, Backend)
│   │   ├── window/          # Window abstraction (Window, WindowConfig)
│   │   └── render/          # Render device abstraction (RenderDevice)
│   ├── cmd/                 # CLI binary (links engine)
│   └── editor/              # Editor placeholder (INTERFACE lib)
├── tests/                   # Unit tests (Catch2 v3)
└── docs/
    ├── constitution/        # Mandatory project rules
    ├── specs/               # Product specs and implementation contracts
    ├── adr/                 # Architecture decision records
    └── wiki/                # Operational documentation (this wiki)
```

## Build system

- **Generator**: Ninja
- **Presets**: `debug` (Debug build) and `release` (Release build)
- **Standard**: C++26 (`CMAKE_CXX_STANDARD 26`, `REQUIRED ON`, `EXTENSIONS OFF`)
- **Formatting**: `clang-format` via custom `format` CMake target
- **External dependencies**: SDL3 (fetched via `FetchContent`), OpenGL (system `find_package`)

## CMake targets

| Target | Type | Directory | Description |
|---|---|---|---|
| `buddd_engine` | Static library | `src/engine/` | Core engine; exposes version API and platform abstraction layer. Links SDL3 and OpenGL. |
| `buddd` | Executable | `src/cmd/` | CLI binary; links `buddd_engine` |
| `buddd_editor` | INTERFACE library | `src/editor/` | Placeholder — no compiled sources |
| `buddd_tests` | Executable | `tests/` | Catch2 test binary; links `buddd_engine` |

## Engine library (`buddd_engine`) internal structure

```
src/engine/
├── CMakeLists.txt           # GLOB_RECURSE collects all .h/.cpp
├── version.h / version.cpp  # Version API
├── error.h                  # Error struct, Result<T>, make_error, to_string
├── platform/
│   ├── platform.h           # Abstract Platform class, Backend enum
│   ├── platform.cpp         # Platform::create() factory
│   ├── platform_sdl3.h/cpp  # SDL3 backend (PlatformSDL3)
│   └── platform_headless.h/cpp # Headless backend (PlatformHeadless)
├── window/
│   ├── window.h             # Abstract Window class, WindowConfig struct
│   ├── window_sdl3.h/cpp    # SDL3 backend (WindowSDL3)
│   └── window_headless.h/cpp # Headless backend (WindowHeadless)
└── render/
    ├── render_device.h      # Abstract RenderDevice class
    ├── render_device.cpp    # RenderDevice::create() factory
    ├── render_device_opengl.h/cpp  # OpenGL 4.5 backend (RenderDeviceOpenGL)
    └── render_device_headless.h/cpp # Headless backend (RenderDeviceHeadless)
```

## Key behaviors

- `./build/debug/src/cmd/buddd` — prints `Buddd Engine v0.1.0`
- `./build/debug/src/cmd/buddd --version` — prints `buddd 0.1.0`
- `ctest --preset debug` — runs tests, all pass
- `cmake --build --preset debug --target format` — formats all C++ sources
- `Platform::create(Backend::Headless)` — creates a headless platform (no display needed, used for testing)
- `Platform::create(Backend::SDL3)` — creates an SDL3-based platform (uses offscreen video driver in tests; requires a display in production)
- Factory methods return `Result<T>` (`std::expected<T, Error>`) for error propagation

## Architecture boundary

A hard architecture boundary is enforced: **no code outside `src/engine/`** may `#include` SDL3 or OpenGL headers. All access to windowing and graphics functionality goes through the abstract `Platform`, `Window`, and `RenderDevice` interfaces. Concrete backend implementations live entirely within `src/engine/`.

**Narrow exception (AMEND-2026-001):** SDL3 test files (`tests/*_sdl3*.cpp` or similar) that are conditionally compiled with `BUDDD_HAS_DISPLAY=ON` may include `<SDL3/SDL.h>` **only** for setting video driver hints (e.g., `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`) before calling `Platform::create()`. This exception applies only to test files that test the SDL3 backend, only to `<SDL3/SDL.h>`, and only for setting video driver hints. All other platform, graphics, or windowing library headers remain prohibited outside `src/engine/`.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md)
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md)
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md)
- Spec: [SPEC-003](/docs/specs/sdl3-backend-tests/spec.md)
- Implementation contract: [IMPL-003](/docs/specs/sdl3-backend-tests/implementation-contract.md)
