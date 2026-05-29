# IMPL-002 — Platform Abstraction Layer

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume (user) |
| Date | 2026-05-29 |
| Time | ~16:30 UTC |

## Source spec

`docs/specs/platform-abstraction/spec.md` (SPEC-002), accepted (`docs/specs/platform-abstraction/spec-critic.md` verdict: `Accepted with warnings`, all blocking issues resolved).

Two non-blocking warnings remain:
- **W-01** — Headless observability logs are implementation-defined. This contract prescribes specific outputs (see Required implementation behavior).
- **W-02** — `WindowConfig` has no default values. This contract does **not** add defaults, staying aligned with the spec. Implementation may add defaults as an extension.

## Goal

Implement the Platform Abstraction Layer for the Buddd Engine, providing:

1. Three abstract interface classes (`Platform`, `Window`, `RenderDevice`) with pure virtual methods and no external library types in their public headers.
2. A `Backend` enum (`Backend::SDL3`, `Backend::Headless`) for runtime backend selection.
3. Concrete SDL3 backend: `PlatformSDL3`, `WindowSDL3`, `RenderDeviceOpenGL` (with OpenGL 4.5 Core profile).
4. Concrete headless backend: `PlatformHeadless`, `WindowHeadless`, `RenderDeviceHeadless` (no SDL3, no OpenGL).
5. A project-wide `Error` struct with `Category` enum, `int code`, `std::string message`, and `Result<T>` alias (`std::expected<T, Error>`).
6. CMake integration: SDL3 fetched via `FetchContent`, OpenGL via `find_package`, all 16+ new source files added to the `buddd_engine` target.

## Non-goals

- No input event handling (keyboard, mouse, gamepad).
- No audio, haptic, or sensor subsystem.
- No Vulkan, DirectX, Metal, or WebGPU backend.
- No multiple-window support.
- No fullscreen or window-mode switching post-creation.
- No render pipeline, draw call abstraction, shader compilation, or GPU resource management.
- No ECS, physics, asset loading, or game runtime.
- No dynamic backend switching after `Platform::create()`.
- No move semantics for `Platform`, `Window`, or `RenderDevice` (they are non-copyable AND non-movable).
- No test file creation (tests are specified for the test-author only).
- No modification of files outside `src/engine/`.
- No modification of `src/engine/version.h` or `src/engine/version.cpp`.

## Relevant constitution rules

- **CONST-001-architecture-boundaries.md**: Enforces the architecture boundary: no code outside `src/engine/` may include SDL3, OpenGL, or graphics library headers. This feature implements and satisfies that rule.
- **CONST-002-testing-policy.md**: Requires unit tests for all testable code. This contract specifies required tests (see Required tests section).
- **CONST-003-documentation-policy.md** and **CONST-004-security-policy.md**: Contain placeholder "TODO" text. No active rule applies.

## Relevant ADRs

- **ADR-001**: `docs/adr/001-result-error-pattern.md` — Establishes `Result<T>` / `Error` as the project-wide error handling pattern. This contract implements that pattern via `error.h`.

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/CMakeLists.txt` | Current CMake target list (only `version.h`, `version.cpp`). Must be updated with all new sources and SDL3/OpenGL dependencies. |
| `src/engine/version.h` | Existing header in `buddd::engine` namespace — serves as a style reference for headers. |
| `docs/specs/platform-abstraction/spec.md` | Authoritative spec for behavior. |
| `docs/specs/project-setup/implementation-contract.md` | Style reference (IMPL-001) for contract format and level of detail. |

## Files allowed to change

### New files to create (18 files)

All paths are relative to the repository root.

1. `src/engine/error.h`
2. `src/engine/platform/platform.h`
3. `src/engine/platform/platform.cpp`
4. `src/engine/platform/platform_sdl3.h`
5. `src/engine/platform/platform_sdl3.cpp`
6. `src/engine/platform/platform_headless.h`
7. `src/engine/platform/platform_headless.cpp`
8. `src/engine/window/window.h`
9. `src/engine/window/window_sdl3.h`
10. `src/engine/window/window_sdl3.cpp`
11. `src/engine/window/window_headless.h`
12. `src/engine/window/window_headless.cpp`
13. `src/engine/render/render_device.h`
14. `src/engine/render/render_device.cpp`
15. `src/engine/render/render_device_opengl.h`
16. `src/engine/render/render_device_opengl.cpp`
17. `src/engine/render/render_device_headless.h`
18. `src/engine/render/render_device_headless.cpp`

### Files to modify (1 file)

19. `src/engine/CMakeLists.txt` — Add all new source files, FetchContent SDL3, find_package OpenGL, link dependencies.

## Files forbidden to change

- Any file outside `src/engine/`.
- `src/engine/version.h`
- `src/engine/version.cpp`
- Root `CMakeLists.txt`
- `CMakePresets.json`
- `src/cmd/` (any file)
- `src/editor/` (any file)
- `tests/` (any file — test files will be created by the test-author)
- `.clang-format`
- `.vscode/` (any file)
- `docs/` (any file not listed in "Files allowed to change")
- `AGENTS.md`
- `opencode.json`
- `SpecKit.md`

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Namespace | `buddd::engine` for all public types. Concrete backend classes may use nested namespaces (`buddd::engine::detail`, anonymous namespace, etc.) for internal symbols. |
| File naming | `snake_case` (lowercase ASCII letters, digits, underscores). |
| Directory naming | `snake_case`. Directories to create: `src/engine/platform/`, `src/engine/window/`, `src/engine/render/`. |
| Class naming | PascalCase (e.g., `PlatformSDL3`, `RenderDeviceOpenGL`). |
| Header guards | `#pragma once` (no `#ifndef` guards). |
| Function style | Trailing return type syntax (`auto foo() -> int`). |
| Non-copyable, non-movable | All abstract classes (`Platform`, `Window`, `RenderDevice`) must have both copy and move constructors/assignment operators `= delete`. |
| Formatting | `.clang-format` at repository root enforces LLVM style, 4-space indent, 100 column limit. |
| CMake variables | `UPPER_SNAKE_CASE`. |
| SDL3 includes | Use `#include <SDL3/SDL.h>` — this single header provides all SDL3 API needed. |
| OpenGL includes | Use `#include <GL/gl.h>` for `glClear`. Provided by `find_package(OpenGL REQUIRED)`. |

## Required implementation behavior

### 1. `src/engine/error.h`

Must contain:

```cpp
#pragma once

#include <expected>
#include <string>
#include <utility>

namespace buddd::engine {

struct Error {
    enum class Category {
        InitFailed,
        WindowCreationFailed,
        RenderDeviceCreationFailed,
        Unsupported,
        Unknown
    };

    Category category{Category::Unknown};
    int code{0};
    std::string message;

    Error() = default;
    Error(Category cat, int c, std::string msg)
        : category{cat}, code{c}, message{std::move(msg)} {}
};

auto to_string(const Error& error) -> std::string;

/// Creates a `std::unexpected<Error>` for use as a return value from Result<T> functions.
/// @param category The error category.
/// @param message  Human-readable error description.
/// @param code     Backend-specific numeric error code (default 0).
inline auto make_error(Error::Category category, std::string message, int code = 0) -> std::unexpected<Error> {
    return std::unexpected<Error>(Error{category, code, std::move(message)});
}

template<typename T>
using Result = std::expected<T, Error>;

} // namespace buddd::engine
```

`to_string()` implementation (must be in `error.h` as `inline` or in its own `.cpp` — the contract requires it inline in the header for simplicity since it is a small utility):

```cpp
inline auto to_string(const Error& error) -> std::string {
    std::string category_str;
    switch (error.category) {
        case Error::Category::InitFailed:                category_str = "InitFailed"; break;
        case Error::Category::WindowCreationFailed:      category_str = "WindowCreationFailed"; break;
        case Error::Category::RenderDeviceCreationFailed: category_str = "RenderDeviceCreationFailed"; break;
        case Error::Category::Unsupported:               category_str = "Unsupported"; break;
        case Error::Category::Unknown:                   category_str = "Unknown"; break;
    }
    return category_str + ": " + error.message + " (code " + std::to_string(error.code) + ")";
}
```

**Requirements:**
- Must be in namespace `buddd::engine`.
- `Error::Category` must have exactly the five values listed.
- `int code{0}` must have a default member initializer of `0`.
- `make_error()` is a free function that returns `std::unexpected<Error>`, enabling concise error returns like `return make_error(Error::Category::InitFailed, "SDL_Init failed: " + std::string(SDL_GetError()));`.
- `Result<T>` must be `std::expected<T, Error>`.
- `to_string()` must produce the format `"<Category>: <message> (code <code>)"`.
- `to_string()` must be `inline` if defined in the header.

### 2. `src/engine/platform/platform.h`

```cpp
#pragma once

#include "error.h"

#include <memory>

namespace buddd::engine {

enum class Backend {
    SDL3,
    Headless
};

class Window;
struct WindowConfig;

class Platform {
public:
    static auto create(Backend backend) -> Result<std::unique_ptr<Platform>>;

    virtual ~Platform() = default;

    virtual auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> = 0;

    Platform(const Platform&) = delete;
    auto operator=(const Platform&) -> Platform& = delete;
    Platform(Platform&&) = delete;
    auto operator=(Platform&&) -> Platform& = delete;

protected:
    Platform() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- `Backend` enum class with exactly two values: `SDL3` and `Headless`.
- `Platform::create(Backend)` must be a static method.
- Forward-declare `Window` and `WindowConfig` — do NOT include `window/window.h`.
- Both copy and move operations must be `= delete`.
- Constructor must be `protected`.

### 3. `src/engine/platform/platform.cpp`

Implements `Platform::create()`. This is the factory that selects and initializes the backend.

```cpp
#include "platform.h"
#include "platform_sdl3.h"
#include "platform_headless.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace buddd::engine {

auto Platform::create(Backend backend) -> Result<std::unique_ptr<Platform>> {
    switch (backend) {
        case Backend::SDL3: {
            if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                std::cerr << "Platform init failed: SDL_Init failed: "
                          << SDL_GetError() << "\n";
                return make_error(Error::Category::InitFailed,
                    "SDL_Init failed: " + std::string(SDL_GetError()));
            }
            std::cerr << "Platform backend: SDL3\n";
            std::cerr << "Platform initialized\n";
            return std::unique_ptr<Platform>(new PlatformSDL3());
        }
        case Backend::Headless: {
            std::cerr << "Platform backend: Headless\n";
            std::cerr << "Platform initialized\n";
            return std::unique_ptr<Platform>(new PlatformHeadless());
        }
    }
    return make_error(Error::Category::Unsupported, "Unknown backend");
}

} // namespace buddd::engine
```

**Requirements:**
- Must `#include <SDL3/SDL.h>` for SDL3 API calls. This include is ONLY in `platform.cpp`, NOT in any public header.
- Must `#include <iostream>` for `std::cerr` observability output.
- `SDL_Init(SDL_INIT_VIDEO)` is called directly in `Platform::create()` for the SDL3 case — NOT in the `PlatformSDL3` constructor.
- If `SDL_Init` fails, return `make_error(Error::Category::InitFailed, ...)`.
- If neither case matches, return `make_error(Error::Category::Unsupported, ...)`.
- Observability: print `"Platform backend: SDL3\n"` or `"Platform backend: Headless\n"` and `"Platform initialized\n"` on success; print failure message on error.
- Use `new PlatformSDL3()` directly (not `std::make_unique`) because the constructor is private.

### 4. `src/engine/platform/platform_sdl3.h`

```cpp
#pragma once

#include "platform.h"

namespace buddd::engine {

class PlatformSDL3 final : public Platform {
public:
    ~PlatformSDL3() override;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;

    PlatformSDL3(const PlatformSDL3&) = delete;
    auto operator=(const PlatformSDL3&) -> PlatformSDL3& = delete;
    PlatformSDL3(PlatformSDL3&&) = delete;
    auto operator=(PlatformSDL3&&) -> PlatformSDL3& = delete;

private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformSDL3() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- Private default constructor to force creation only through `Platform::create()`.
- `friend` declaration for `Platform::create(Backend)`.
- `final` class.
- `override` on all virtual methods.
- Non-copyable, non-movable.

### 5. `src/engine/platform/platform_sdl3.cpp`

```cpp
#include "platform_sdl3.h"
#include "window/window_sdl3.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace buddd::engine {

PlatformSDL3::~PlatformSDL3() {
    SDL_Quit();
    std::cerr << "Platform shutdown (SDL3)\n";
}

auto PlatformSDL3::create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> {
    if (config.width <= 0 || config.height <= 0) {
        return make_error(Error::Category::WindowCreationFailed, "Invalid window dimensions");
    }

    auto* sdl_window = SDL_CreateWindow(
        config.title.c_str(),
        config.width,
        config.height,
        SDL_WINDOW_OPENGL
    );

    if (sdl_window == nullptr) {
        return make_error(Error::Category::WindowCreationFailed,
            "SDL_CreateWindow failed: " + std::string(SDL_GetError()));
    }

    std::cerr << "Window created: " << config.width << "x" << config.height << "\n";
    return std::unique_ptr<Window>(new WindowSDL3(sdl_window, config.width, config.height));
}

} // namespace buddd::engine
```

**Requirements:**
- Validate `config.width > 0 && config.height > 0` — return `WindowCreationFailed` error if not.
- Call `SDL_CreateWindow` with the `SDL_WINDOW_OPENGL` flag.
- On failure, return `make_error(Error::Category::WindowCreationFailed, ...)` with the SDL error message.
- Observability: print `"Window created: <width>x<height>\n"` on success.
- Pass width and height to `WindowSDL3` constructor (not querying from SDL post-creation).

### 6. `src/engine/platform/platform_headless.h`

```cpp
#pragma once

#include "platform.h"

namespace buddd::engine {

class PlatformHeadless final : public Platform {
public:
    ~PlatformHeadless() override = default;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;

    PlatformHeadless(const PlatformHeadless&) = delete;
    auto operator=(const PlatformHeadless&) -> PlatformHeadless& = delete;
    PlatformHeadless(PlatformHeadless&&) = delete;
    auto operator=(PlatformHeadless&&) -> PlatformHeadless& = delete;

private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformHeadless() = default;
};

} // namespace buddd::engine
```

### 7. `src/engine/platform/platform_headless.cpp`

```cpp
#include "platform_headless.h"
#include "window/window_headless.h"

#include <iostream>

namespace buddd::engine {

auto PlatformHeadless::create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> {
    if (config.width <= 0 || config.height <= 0) {
        return make_error(Error::Category::WindowCreationFailed, "Invalid window dimensions");
    }

    std::cerr << "Window created (Headless): " << config.width << "x" << config.height << "\n";
    return std::unique_ptr<Window>(new WindowHeadless(config.width, config.height));
}

} // namespace buddd::engine
```

**Requirements:**
- NO `#include <SDL3/SDL.h>` and NO `#include` of any OpenGL header in either `platform_headless.h` or `platform_headless.cpp`.
- Same dimension validation as the SDL3 variant.
- Observability: print `"Window created (Headless): <width>x<height>\n"` on success (consistent with SDL3 backend).

### 8. `src/engine/window/window.h`

```cpp
#pragma once

#include <memory>
#include <string>

namespace buddd::engine {

struct WindowConfig {
    std::string title;
    int width;
    int height;
};

class Window {
public:
    virtual ~Window() = default;

    virtual auto width() const noexcept -> int = 0;
    virtual auto height() const noexcept -> int = 0;
    virtual auto native_handle() const noexcept -> void* = 0;

    Window(const Window&) = delete;
    auto operator=(const Window&) -> Window& = delete;
    Window(Window&&) = delete;
    auto operator=(Window&&) -> Window& = delete;

protected:
    Window() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- `WindowConfig` struct with three fields: `std::string title`, `int width`, `int height` — NO default member initializers.
- `Window` is abstract with three pure virtual methods.
- `native_handle()` returns `void*` — this is the opaque handle for backend use.
- Non-copyable, non-movable.
- `protected` default constructor.

### 9. `src/engine/window/window_sdl3.h`

```cpp
#pragma once

#include "window.h"

#include <SDL3/SDL.h>

namespace buddd::engine {

class WindowSDL3 final : public Window {
public:
    WindowSDL3(SDL_Window* window, int width, int height);
    ~WindowSDL3() override;

    auto width() const noexcept -> int override;
    auto height() const noexcept -> int override;
    auto native_handle() const noexcept -> void* override;

    WindowSDL3(const WindowSDL3&) = delete;
    auto operator=(const WindowSDL3&) -> WindowSDL3& = delete;
    WindowSDL3(WindowSDL3&&) = delete;
    auto operator=(WindowSDL3&&) -> WindowSDL3& = delete;

private:
    SDL_Window* window_;
    int width_;
    int height_;
};

} // namespace buddd::engine
```

**Requirements:**
- `WindowSDL3` includes `<SDL3/SDL.h>` (this is inside `src/engine/`, an allowed boundary).
- Stores `SDL_Window*` as `window_`, plus `int width_` and `int height_` as cached values.

### 10. `src/engine/window/window_sdl3.cpp`

```cpp
#include "window_sdl3.h"

namespace buddd::engine {

WindowSDL3::WindowSDL3(SDL_Window* window, int width, int height)
    : window_(window), width_(width), height_(height) {}

WindowSDL3::~WindowSDL3() {
    SDL_DestroyWindow(window_);
}

auto WindowSDL3::width() const noexcept -> int {
    return width_;
}

auto WindowSDL3::height() const noexcept -> int {
    return height_;
}

auto WindowSDL3::native_handle() const noexcept -> void* {
    return static_cast<void*>(window_);
}

} // namespace buddd::engine
```

**Requirements:**
- Destructor calls `SDL_DestroyWindow(window_)`.
- `native_handle()` casts `SDL_Window*` to `void*`.

### 11. `src/engine/window/window_headless.h`

```cpp
#pragma once

#include "window.h"

namespace buddd::engine {

class WindowHeadless final : public Window {
public:
    WindowHeadless(int width, int height);
    ~WindowHeadless() override = default;

    auto width() const noexcept -> int override;
    auto height() const noexcept -> int override;
    auto native_handle() const noexcept -> void* override;

    WindowHeadless(const WindowHeadless&) = delete;
    auto operator=(const WindowHeadless&) -> WindowHeadless& = delete;
    WindowHeadless(WindowHeadless&&) = delete;
    auto operator=(WindowHeadless&&) -> WindowHeadless& = delete;

private:
    int width_;
    int height_;
};

} // namespace buddd::engine
```

**Requirements:**
- NO `#include <SDL3/SDL.h>` — not in header, not in implementation.
- `native_handle()` returns `nullptr`.

### 12. `src/engine/window/window_headless.cpp`

```cpp
#include "window_headless.h"

namespace buddd::engine {

WindowHeadless::WindowHeadless(int width, int height)
    : width_(width), height_(height) {}

auto WindowHeadless::width() const noexcept -> int {
    return width_;
}

auto WindowHeadless::height() const noexcept -> int {
    return height_;
}

auto WindowHeadless::native_handle() const noexcept -> void* {
    return nullptr;
}

} // namespace buddd::engine
```

### 13. `src/engine/render/render_device.h`

```cpp
#pragma once

#include "error.h"

#include <memory>
#include <utility>

namespace buddd::engine {

class Window;

class RenderDevice {
public:
    static auto create(Window& window) -> Result<std::unique_ptr<RenderDevice>>;

    virtual ~RenderDevice() = default;

    virtual auto begin_frame() -> void = 0;
    virtual auto end_frame() -> void = 0;
    virtual auto size() const noexcept -> std::pair<int, int> = 0;

    RenderDevice(const RenderDevice&) = delete;
    auto operator=(const RenderDevice&) -> RenderDevice& = delete;
    RenderDevice(RenderDevice&&) = delete;
    auto operator=(RenderDevice&&) -> RenderDevice& = delete;

protected:
    RenderDevice() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- Forward-declare `Window` — do NOT include `window/window.h`.
- `RenderDevice::create(Window&)` is a static method returning `Result<std::unique_ptr<RenderDevice>>`.
- Non-copyable, non-movable.

### 14. `src/engine/render/render_device.cpp`

```cpp
#include "render_device.h"
#include "render_device_opengl.h"
#include "render_device_headless.h"

#include "window/window.h"

#include <SDL3/SDL.h>
#include <iostream>

namespace buddd::engine {

auto RenderDevice::create(Window& window) -> Result<std::unique_ptr<RenderDevice>> {
    auto* native = window.native_handle();

    if (native == nullptr) {
        std::cerr << "Render device created (Headless)\n";
        return std::unique_ptr<RenderDevice>(
            new RenderDeviceHeadless(window.width(), window.height()));
    }

    // SDL3/OpenGL backend
    auto* sdl_window = static_cast<SDL_Window*>(native);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    auto* gl_context = SDL_GL_CreateContext(sdl_window);
    if (gl_context == nullptr) {
        return make_error(Error::Category::RenderDeviceCreationFailed,
            "SDL_GL_CreateContext failed: " + std::string(SDL_GetError()));
    }

    SDL_GL_MakeCurrent(sdl_window, gl_context);

    std::cerr << "Render device created (OpenGL 4.5 Core)\n";
    return std::unique_ptr<RenderDevice>(
        new RenderDeviceOpenGL(sdl_window, gl_context));
}

} // namespace buddd::engine
```

**Requirements:**
- Must `#include <SDL3/SDL.h>` for all SDL_GL_* functions.
- Dispatch logic: if `window.native_handle()` is `nullptr`, create `RenderDeviceHeadless`; otherwise, cast to `SDL_Window*` and create `RenderDeviceOpenGL`.
- `SDL_GL_SetAttribute` for Core profile 4.5.
- `#ifndef NDEBUG` guard for debug context flag.
- `SDL_GL_CreateContext` and `SDL_GL_MakeCurrent` both called in the factory.
- On failure, return `RenderDeviceCreationFailed`.
- Observability: print `"Render device created (Headless)\n"` or `"Render device created (OpenGL 4.5 Core)\n"`.

### 15. `src/engine/render/render_device_opengl.h`

```cpp
#pragma once

#include "render_device.h"

#include <SDL3/SDL.h>

namespace buddd::engine {

class RenderDeviceOpenGL final : public RenderDevice {
public:
    RenderDeviceOpenGL(SDL_Window* window, SDL_GLContext context);
    ~RenderDeviceOpenGL() override;

    auto begin_frame() -> void override;
    auto end_frame() -> void override;
    auto size() const noexcept -> std::pair<int, int> override;

    RenderDeviceOpenGL(const RenderDeviceOpenGL&) = delete;
    auto operator=(const RenderDeviceOpenGL&) -> RenderDeviceOpenGL& = delete;
    RenderDeviceOpenGL(RenderDeviceOpenGL&&) = delete;
    auto operator=(RenderDeviceOpenGL&&) -> RenderDeviceOpenGL& = delete;

private:
    SDL_Window* window_;
    SDL_GLContext context_;
};

} // namespace buddd::engine
```

**Requirements:**
- Stores `SDL_Window*` and `SDL_GLContext`.
- Non-copyable, non-movable.

### 16. `src/engine/render/render_device_opengl.cpp`

```cpp
#include "render_device_opengl.h"

#include <GL/gl.h>

namespace buddd::engine {

RenderDeviceOpenGL::RenderDeviceOpenGL(SDL_Window* window, SDL_GLContext context)
    : window_(window), context_(context) {}

RenderDeviceOpenGL::~RenderDeviceOpenGL() {
    SDL_GL_DestroyContext(context_);
}

auto RenderDeviceOpenGL::begin_frame() -> void {
    glClear(GL_COLOR_BUFFER_BIT);
}

auto RenderDeviceOpenGL::end_frame() -> void {
    SDL_GL_SwapWindow(window_);
}

auto RenderDeviceOpenGL::size() const noexcept -> std::pair<int, int> {
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);
    return {w, h};
}

} // namespace buddd::engine
```

**Requirements:**
- `begin_frame()` calls `glClear(GL_COLOR_BUFFER_BIT)` — no other OpenGL state changes.
- `end_frame()` calls `SDL_GL_SwapWindow(window_)`.
- `size()` queries SDL for current window size via `SDL_GetWindowSize` (not from cached values).
- Destructor calls `SDL_GL_DestroyContext(context_)`.
- Include `<GL/gl.h>` for `glClear`. (`find_package(OpenGL REQUIRED)` provides this header.)

### 17. `src/engine/render/render_device_headless.h`

```cpp
#pragma once

#include "render_device.h"

namespace buddd::engine {

class RenderDeviceHeadless final : public RenderDevice {
public:
    RenderDeviceHeadless(int width, int height);
    ~RenderDeviceHeadless() override = default;

    auto begin_frame() -> void override;
    auto end_frame() -> void override;
    auto size() const noexcept -> std::pair<int, int> override;

    RenderDeviceHeadless(const RenderDeviceHeadless&) = delete;
    auto operator=(const RenderDeviceHeadless&) -> RenderDeviceHeadless& = delete;
    RenderDeviceHeadless(RenderDeviceHeadless&&) = delete;
    auto operator=(RenderDeviceHeadless&&) -> RenderDeviceHeadless& = delete;

private:
    int width_;
    int height_;
};

} // namespace buddd::engine
```

### 18. `src/engine/render/render_device_headless.cpp`

```cpp
#include "render_device_headless.h"

namespace buddd::engine {

RenderDeviceHeadless::RenderDeviceHeadless(int width, int height)
    : width_(width), height_(height) {}

auto RenderDeviceHeadless::begin_frame() -> void {
    // no-op
}

auto RenderDeviceHeadless::end_frame() -> void {
    // no-op
}

auto RenderDeviceHeadless::size() const noexcept -> std::pair<int, int> {
    return {width_, height_};
}

} // namespace buddd::engine
```

**Requirements:**
- NO `#include <SDL3/SDL.h>` or `<GL/gl.h>` in ANY headless file.
- All methods are no-ops except `size()` which returns stored dimensions.

### 19. `src/engine/CMakeLists.txt` (modified)

The new `CMakeLists.txt` must be:

```cmake
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.30
)
FetchContent_MakeAvailable(SDL3)

find_package(OpenGL REQUIRED)

# Collect all engine source files automatically using GLOB.
# New files added to src/engine/ subdirectories are picked up on re-configure.
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(buddd_engine STATIC ${ENGINE_SOURCES})

target_include_directories(buddd_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(buddd_engine PUBLIC
    SDL3::SDL3
    OpenGL::GL
)
```

**Requirements:**
- `FetchContent` for SDL3 must be declared with **exactly** `GIT_TAG release-3.2.30` (this is a verified existing tag).
- `FetchContent_MakeAvailable(SDL3)` makes SDL3 targets available.
- `find_package(OpenGL REQUIRED)` must be called.
- `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` collects all `.h` and `.cpp` files under `src/engine/`. The `CONFIGURE_DEPENDS` flag ensures CMake re-globs when a file is added or removed.
- `target_include_directories` with `${CMAKE_CURRENT_SOURCE_DIR}` must stay (for `#include "error.h"`, `#include "platform/platform.h"`, etc. to work).
- `target_link_libraries` must link `SDL3::SDL3` and `OpenGL::GL` as **PUBLIC**.
- The `FetchContent` block must appear BEFORE `add_library` (so SDL3 targets are available when linking).
- The `find_package(OpenGL REQUIRED)` must also appear before `add_library`.

## Required tests

The following tests MUST be present in the `buddd_tests` binary. They are specified here for the test-author who will create the test files. The implementation-author does NOT create test files.

### Headless backend tests (always runnable, no display required)

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-01 | `"Platform::create(Headless) succeeds"` | `[headless]` `[platform]` | `Platform::create(Backend::Headless)` returns a valid `std::unique_ptr<Platform>`. |
| T-02 | `"Headless Platform creates Window with valid config"` | `[headless]` `[window]` | After `Platform::create(Headless)`, calling `create_window({.title="Test", .width=800, .height=600})` returns a valid `std::unique_ptr<Window>`. |
| T-03 | `"Headless Window creates RenderDevice"` | `[headless]` `[render]` | After creating a headless window, `RenderDevice::create(window)` returns a valid `std::unique_ptr<RenderDevice>`. |
| T-04 | `"Headless frame cycle completes"` | `[headless]` `[render]` | `begin_frame()` and `end_frame()` can be called sequentially without error. |
| T-05 | `"Headless RenderDevice::size() returns correct dimensions"` | `[headless]` `[render]` | `RenderDevice::size()` returns `{800, 600}` matching the window config. |
| T-06 | `"Headless Window::native_handle() returns nullptr"` | `[headless]` `[window]` | `window.native_handle()` is `nullptr`. |
| T-07 | `"WindowConfig negative dimensions return error"` | `[headless]` `[window]` | Creating a window with `{.title="Bad", .width=-1, .height=100}` returns an error with category `WindowCreationFailed`. |
| T-08 | `"Error struct construction and to_string"` | `[headless]` `[error]` | Creating `Error{Error::Category::InitFailed, 42, "test"}` and calling `to_string()` returns `"InitFailed: test (code 42)"`. |
| T-09 | `"make_error helper compiles and returns correct category"` | `[headless]` `[error]` | Calling `make_error(Error::Category::WindowCreationFailed, "test")` returns a `std::unexpected<Error>` whose `error().category` is `WindowCreationFailed` and `error().message` is `"test"`. |
| T-10 | `"make_error with explicit code"` | `[headless]` `[error]` | Calling `make_error(Error::Category::InitFailed, "msg", 42)` returns `std::unexpected<Error>` with `error().code == 42`. |
| T-11 | `"Result<T> compiles with unique_ptr"` | `[headless]` `[error]` | A function returning `Result<std::unique_ptr<int>>` compiles and works correctly. |

### SDL3 backend tests (conditional or deferred)

| ID | Test name | Tags | Verification |
|---|---|---|---|
| T-12 | `"Backend enum values exist"` | `[sdl3]` `[platform]` | `Backend::SDL3` and `Backend::Headless` are valid identifiers. (Can be compiled without running.) |
| T-13 | `"Platform::create(SDL3) success"` | `[sdl3]` `[platform]` | Run only on systems with a display. Returns valid Platform. (Marked with `[!mayfail]` or `[!shouldfail]` as appropriate for CI.) |

T-13 and any other SDL3-specific tests that require a display should be conditionally compiled (e.g., guarded by `#ifdef BUDDD_HAS_DISPLAY` or similar) or marked with the appropriate Catch2 failure tags so CI does not fail.

### Non-functional requirements

- All headless tests must pass in CI without a display server.
- All tests must use `REQUIRE`/`REQUIRE_FALSE` (not `CHECK`) for assertions.
- Test files go in `tests/` and are created by the test-author, not the implementation-author.

## Edge cases

| Case | Expected behavior |
|---|---|
| `Platform::create()` called twice without destroying the first instance | Each call returns an independent `Platform` instance. `SDL_Init` is ref-counted internally by SDL3 so nested calls are safe. |
| Platform created but no window created before destruction | `~PlatformSDL3()` calls `SDL_Quit()`. No window resources to leak. |
| Window created but no render device before destruction | `~WindowSDL3()` calls `SDL_DestroyWindow()`. No GL context to leak. |
| Render device created and window destroyed before render device | Undefined behavior (developer responsibility). The contract does not guard against this. |
| `WindowConfig.width` or `height` ≤ 0 | `create_window()` returns `make_error(Error::Category::WindowCreationFailed, "Invalid window dimensions")`. |
| `WindowConfig.title` empty string | Window is created with an empty title. No error. |
| Headless window native handle accessor | Returns `nullptr`. |
| SDL3 `SDL_Init` fails (no display) | `Platform::create(Backend::SDL3)` returns `make_error(Error::Category::InitFailed, "SDL_Init failed: ...")`. |
| SDL3 `SDL_CreateWindow` fails | `Platform::create_window()` returns `make_error(Error::Category::WindowCreationFailed, "SDL_CreateWindow failed: ...")`. |
| `SDL_GL_CreateContext` fails | `RenderDevice::create()` returns `make_error(Error::Category::RenderDeviceCreationFailed, "SDL_GL_CreateContext failed: ...")`. |
| `SDL_GL_SetAttribute` fails | `SDL_GL_SetAttribute` return values are not checked individually. If an attribute fails, `SDL_GL_CreateContext` will fail shortly after, producing `RenderDeviceCreationFailed` with the SDL error message. |
| OpenGL 4.5 Core profile not available | `SDL_GL_CreateContext` fails and an error is returned via `RenderDeviceCreationFailed`. |
| `Platform::create()` with invalid backend enum value | Returns `make_error(Error::Category::Unsupported, "Unknown backend")`. The switch must have no default case to trigger compiler warnings on missing enum values, but a return after the switch handles unknown values. |
| Multiple `create_window()` calls on same Platform | Undefined behavior (not supported in this spec). The implementation does not guard against this. |

## Security impact

None. No elevated privileges are required. No secrets, credentials, or environment variables are consumed. SDL3 and OpenGL run in the user's session context. The headless backend requires no display server access and is safe for CI environments without GPU or X11/Wayland.

## Data and migration impact

None. No persistent state, database, or file format is introduced. The platform abstraction layer is purely in-memory.

## API compatibility impact

The following public API surface is introduced:

```cpp
namespace buddd::engine {

// Error handling
enum class Error::Category { InitFailed, WindowCreationFailed, RenderDeviceCreationFailed, Unsupported, Unknown };
struct Error { Category category{Unknown}; int code{0}; std::string message; };
auto to_string(const Error&) -> std::string;
auto make_error(Error::Category, std::string, int code = 0) -> std::unexpected<Error>;
template<typename T> using Result = std::expected<T, Error>;

// Backend selection
enum class Backend { SDL3, Headless };

// Platform
class Platform {
    static auto create(Backend) -> Result<std::unique_ptr<Platform>>;
    virtual auto create_window(const WindowConfig&) -> Result<std::unique_ptr<Window>> = 0;
};

// Window
struct WindowConfig { std::string title; int width; int height; };
class Window {
    virtual auto width() const noexcept -> int = 0;
    virtual auto height() const noexcept -> int = 0;
    virtual auto native_handle() const noexcept -> void* = 0;
};

// Render device
class RenderDevice {
    static auto create(Window&) -> Result<std::unique_ptr<RenderDevice>>;
    virtual auto begin_frame() -> void = 0;
    virtual auto end_frame() -> void = 0;
    virtual auto size() const noexcept -> std::pair<int,int> = 0;
};

} // namespace buddd::engine
```

**Backward compatibility**: This is the first version of these APIs. All types are introduced in this contract. Once accepted, changing any of the following constitutes a breaking change:
- Namespace, class name, or enum value name.
- Function signature, return type, or parameter type.
- Adding or removing virtual methods.
- Changing `Result<T>` definition.
- Changing `Error` struct fields or `Category` enum values.

## Documentation impact

- No README, wiki page, or other documentation files are created or modified.
- The spec (`docs/specs/platform-abstraction/spec.md`) remains authoritative.
- The `SpecKit.md` and `AGENTS.md` remain untouched.
- The API surface described above is the public contract.

## ADR impact

None. No architectural decision requires an ADR — the patterns (runtime backend selection via enum, `Result<T>` error handling, `FetchContent` for SDL3, factory methods on abstract classes) are design decisions documented in the spec.

## Constitution impact

None. No constitution rules need to be added or amended.

## Done criteria

The implementation is complete when all of the following are true:

1. **Files exist**: All 18 new files listed in "Files allowed to change" exist with correct content that matches the required implementation behavior.
2. **`src/engine/CMakeLists.txt` modified**: The updated file uses `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` to collect all sources, includes FetchContent for SDL3 (`release-3.2.30`), calls `find_package(OpenGL REQUIRED)`, and links both `SDL3::SDL3` and `OpenGL::GL` as PUBLIC.
3. **Build succeeds**: `cmake --preset debug && cmake --build --preset debug` exits 0 with zero warnings related to the new backend source files on the reference compiler.
4. **Release preset works**: `cmake --preset release && cmake --build --preset release` succeeds.
5. **Architecture boundary verified**: Running `grep -E '(SDL_|gl[A-Z]|GL_|GLAD)' src/engine/platform/platform.h src/engine/window/window.h src/engine/render/render_device.h src/engine/error.h` returns zero matches. No SDL3 or OpenGL types leak into public headers.
6. **Error type compiles**: `Result<int>` is usable; `to_string(error)` produces `"<Category>: <message> (code <code>)"`.
7. **Backend enum compiles**: `Backend::SDL3` and `Backend::Headless` are valid identifiers.
8. **Platform factory works for Headless**: `Platform::create(Backend::Headless)` compiles and produces a valid object.
9. **Platform factory works for SDL3**: `Platform::create(Backend::SDL3)` compiles. (Runtime success depends on display availability.)
10. **Headless full cycle compiles**: Code using `Platform::create(Headless)` → `create_window()` → `RenderDevice::create()` → `begin_frame()` → `end_frame()` compiles.
11. **Abstract classes are non-copyable and non-movable**: A `static_assert(!std::is_copy_constructible_v<Platform>)` in a test would pass.
12. **No SDL3 or OpenGL includes in headless files**: Grep for `SDL_` or `gl` in `*_headless.*` files returns nothing.
13. **Non-modified forbidden files**: `src/engine/version.h`, `src/engine/version.cpp`, root `CMakeLists.txt`, `CMakePresets.json`, and all files in `src/cmd/`, `src/editor/`, `tests/` remain unchanged.

## Verification commands (copy-paste ready)

```bash
# Configure and build
cmake --preset debug
cmake --build --preset debug

# Verify architecture boundary (no leaks in public headers)
grep -E '(SDL_|gl[A-Z]|GL_|GLAD)' src/engine/platform/platform.h src/engine/window/window.h src/engine/render/render_device.h src/engine/error.h
# Expected: zero matches

# Verify no SDL3/OpenGL in headless files
grep -E '(SDL_|gl[A-Z]|GL_)' src/engine/platform/platform_headless.h src/engine/platform/platform_headless.cpp src/engine/window/window_headless.h src/engine/window/window_headless.cpp src/engine/render/render_device_headless.h src/engine/render/render_device_headless.cpp
# Expected: zero matches

# Verify forbidden files are unchanged
git diff --name-only
# Should NOT include: version.h, version.cpp, root CMakeLists.txt, CMakePresets.json, anything in src/cmd/, src/editor/, tests/

# Build release preset (optional, P2)
cmake --preset release
cmake --build --preset release
```
