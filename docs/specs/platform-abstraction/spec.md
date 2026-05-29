# SPEC-002 — Platform Abstraction Layer

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|
| Approved by | |
| Date | |
| Time | |

## Problem

The Buddd Engine has no abstraction over platform-level dependencies (windowing, graphics context creation). Without this layer:

- Engine code outside `src/engine/` would need to `#include <SDL3/SDL.h>` or `#include <GL/glcorearb.h>` directly, coupling every consumer to specific library versions and platform APIs.
- Unit tests cannot run on machines without a display server (CI, headless workstations, WSL without X11) because SDL3 requires a video subsystem.
- Swapping the windowing toolkit (e.g., SDL3 → GLFW) or graphics API (e.g., OpenGL → Vulkan) would require rewriting every file that touches those headers.

## Goals

- Define three abstract interface classes (`Platform`, `Window`, `RenderDevice`) with pure virtual methods and no external library types exposed in their public headers.
- Provide a concrete SDL3 backend implementation for `Platform` and `Window`.
- Provide a concrete OpenGL 4.5 Core backend implementation for `RenderDevice`.
- Provide a headless (no SDL3, no OpenGL) backend for all three interfaces, selectable at runtime via a `Backend` enum parameter to `Platform::create()`, so unit tests can run without a display.
- Enforce a hard architecture boundary: outside `src/engine/`, no code may `#include` SDL3 or OpenGL headers — all access goes through the abstractions.
- Define a project-wide `Result<T>` template alias (`template<typename T> using Result = std::expected<T, Error>`) where `Error` is a struct containing an `enum class Category` (with values like `InitFailed`, `WindowCreationFailed`, `RenderDeviceCreationFailed`, `Unsupported`, `Unknown`), an `int code = 0` carrying a backend-specific numeric error code (e.g., a GLenum for OpenGL, or 0 when none applies), and a `std::string message` for human-readable context. Provide a `make_error(Category, message, code = 0)` helper function returning `std::unexpected<Error>` for concise error construction. `Result<T>` becomes the standard error-return pattern for all engine APIs going forward.
- Integrate the new files into the existing `buddd_engine` static library target and CMake build system.

## Non-goals

- No input event handling (keyboard, mouse, gamepad) — that is a separate future feature.
- No audio, haptic, or sensor subsystem.
- No Vulkan, DirectX, Metal, or WebGPU backend.
- No multiple-window support (single window only at this stage).
- No fullscreen or window mode switching beyond what is set at creation time.
- No render pipeline, draw call abstraction, shader compilation, or GPU resource management.
- No ECS, physics, asset loading, or any game runtime system.
- No dynamic backend switching after `Platform::create()` — the backend is fixed for the lifetime of the `Platform` instance.

## Conventions

### File and directory naming

| Convention | Rule | Examples |
|---|---|---|
| Source file names | `snake_case` (lowercase ASCII letters, digits, underscores) | `platform.h`, `platform_sdl3.cpp`, `render_device_opengl.h` |
| Directory names | `snake_case` (lowercase ASCII letters, digits, underscores) | `src/engine/platform/`, `src/engine/window/`, `src/engine/render/` |

### Code style

Follow the existing project conventions from SPEC-001 (PascalCase for classes, `snake_case` for files and functions, `UPPER_SNAKE_CASE` for CMake variables). Abstract interface classes are named with a leading `I` convention is **not** used — they are plain abstract classes (e.g., `Platform`, `Window`, `RenderDevice`). Concrete implementations append the backend name (e.g., `PlatformSDL3`, `WindowSDL3`, `RenderDeviceOpenGL`).

### Namespace

All types live under `buddd::engine`. Concrete backends may use a nested namespace (e.g., `buddd::engine::sdl3`, `buddd::engine::opengl`) for internal symbols, but public interface headers expose only `buddd::engine`.

### Header inclusion rule

Outside `src/engine/`, any `#include` of `<SDL3/`, `<GL/`, `<glad/`, or any graphics-library header is a violation of the architecture boundary. Violations must be caught by code review and, if feasible, a CMake or compiler-based guard.

## Actors

| Actor | Description |
|---|---|
| Engine developer | A developer adding features to the Buddd Engine who needs to create windows or interact with the rendering surface. Depends on the abstractions, never on SDL3/OpenGL directly. |
| Application developer | A developer building a game or tool on top of the engine. Uses the engine API without knowing which windowing or graphics backend is active. |
| Build system | CMake + Ninja that provides SDL3 and OpenGL dependencies (always available when the SDL3 backend is compiled; headless backend has no extra dependencies). |
| Test suite | Catch2 v3 tests that run in headless mode — no display, no GPU required. |

## User-visible behavior

- A `Backend` enum class is defined with values `SDL3` and `Headless`, allowing runtime selection of the backend.
- All polymorphic factories return `Result<std::unique_ptr<T>>` where `T` is an abstract class (`Platform`, `Window`, `RenderDevice`). The `unique_ptr` provides natural polymorphic ownership.
- The engine provides a `Platform::create(Backend)` factory that takes a backend parameter. On success, the platform is initialized (`SDL_Init(SDL_INIT_VIDEO)` called internally for the SDL3 backend).
- The platform provides `create_window(const WindowConfig&)` returning `Result<std::unique_ptr<Window>>`. On success, a native window is created.
- A `Window` exposes width/height getters and (for internal use) a native handle accessor.
- The engine provides `RenderDevice::create(Window&)` returning `Result<std::unique_ptr<RenderDevice>>`. On success, a graphics context bound to the window is ready.
- `RenderDevice::size()` returns the framebuffer dimensions as `std::pair<int,int>` (width, height).
- `RenderDevice::begin_frame()` starts a new frame and clears the surface with a default clear color (black). `RenderDevice::end_frame()` presents the frame (swaps buffers).
- All concrete backend files live inside `src/engine/` and are invisible to external consumers.
- Both backends (SDL3 and Headless) are always compiled into `buddd_engine`. SDL3 is fetched via `FetchContent` and always available. The headless backend has zero external dependencies.
- A `Result<T>` template alias and `Error` struct in `src/engine/error.h` provide the standard error-handling pattern for all engine APIs.

## User stories

### Story 1 — Create a window and render device via abstractions (Priority: P1)

As an engine developer, I want to create a platform, open a window, and obtain a render device using only abstract interface types, so that my code does not depend on any specific windowing or graphics library.

**Given** a running engine with `Backend::SDL3`
**When** I invoke:
```
auto platform = Platform::create(Backend::SDL3);
auto window = platform->create_window({.title = "Test", .width = 800, .height = 600});
auto device = RenderDevice::create(*window);
device->begin_frame();
// ... render commands ...
device->end_frame();
```
**Then** the platform initializes, a native window appears (or is created off-screen), a graphics context is created and made current, and the frame cycle completes without error.

**Given** a running engine with `Backend::Headless`
**When** I invoke the same sequence of abstract calls
**Then** the headless backend completes each step successfully without a display, without any SDL3 or OpenGL dependency.

### Story 2 — Error propagation on initialization failure (Priority: P1)

As an engine developer, I want failed initialization (missing display, unavailable graphics driver) to produce a clear `Error` value (with `Category`, `code`, and `message`) rather than a crash or undefined behavior.

**Given** `Backend::SDL3` on a system with no display server (e.g., a headless CI runner)
**When** I call `Platform::create(Backend::SDL3)`
**Then** the return value is `make_error(Error::Category::InitFailed, "SDL_Init failed: No available video device")`.

**Given** a GPU that does not support OpenGL 4.5 Core profile
**When** I call `RenderDevice::create(window)` after successfully creating a window
**Then** the return value is `make_error(Error::Category::RenderDeviceCreationFailed, "SDL_GL_CreateContext failed")`.

### Story 3 — Full lifecycle (create, use, destroy) (Priority: P1)

As an engine developer, I want to create and destroy platform, window, and render device instances cleanly, so that resources (SDL3 subsystem, native window handle, OpenGL context) are released on destruction.

**Given** a scope containing a `Platform` instance with an active window and render device
**When** the scope exits and all three objects are destroyed
**Then** no SDL3 or OpenGL resources remain allocated — `SDL_Quit()` has been called (SDL3 backend), the native window is destroyed, and the GL context is freed. A subsequent `Platform::create()` in the same process succeeds.

### Story 4 — Backend selection at runtime (Priority: P2)

As an engine developer, I want to choose between SDL3 and headless backends at runtime via a simple enum parameter, so that the same compiled engine binary can be used for both normal execution and headless testing.

**Given** a built engine library with both backends compiled in
**When** I call `Platform::create(Backend::Headless)` in a unit test
**Then** the headless backend is used and no display or GPU is required.

**Given** the same library
**When** I call `Platform::create(Backend::SDL3)` in the production executable
**Then** the SDL3 backend is used and a native window can be created.

### Story 5 — Architecture boundary enforcement (Priority: P2)

As a project maintainer, I want to be confident that no code outside `src/engine/` bypasses the abstraction layer, so that the platform independence guarantee is preserved.

**Given** a source file outside `src/engine/` (e.g., `src/cmd/main.cpp` or `tests/*.cpp`)
**When** it contains an `#include <SDL3/SDL.h>` or `#include <GL/glcorearb.h>`
**Then** the violation must be caught during code review. (Automated enforcement via a compile-time header guard or CI linting is a future goal, not required for this spec.)

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|---|
| AC-001 | An abstract `Platform` class exists in `src/engine/platform/platform.h` in namespace `buddd::engine` with a static `create(Backend)` factory returning `Result<std::unique_ptr<Platform>>` and a virtual destructor. | File compiles; `Platform::create(Backend)` signature matches; destructor is `virtual`. |
| AC-002 | A `WindowConfig` struct exists (in `src/engine/window/window.h`) with fields `title` (string type), `width` (int), `height` (int). | Compilation succeeds; struct has the three fields with the described types. |
| AC-003 | An abstract `Window` class exists in `src/engine/window/window.h` with width/height getters returning `int`, a virtual destructor, and an internal native-handle accessor. | File compiles; width/height getters are `auto width() const -> int` (or equivalent); destructor is `virtual`. |
| AC-004 | An abstract `RenderDevice` class exists in `src/engine/render/render_device.h` with a static `create(Window&)` factory returning `Result<std::unique_ptr<RenderDevice>>`, virtual destructor, `begin_frame()` → `void`, `end_frame()` → `void`, and `size()` → `std::pair<int,int>`. | File compiles; signatures match specification. |
| AC-005 | An `Error` struct exists in `src/engine/error.h` with an `enum class Category` (`InitFailed`, `WindowCreationFailed`, `RenderDeviceCreationFailed`, `Unsupported`, `Unknown`), an `int code = 0` field for backend-specific error codes, and a `std::string message` field. A standalone `auto to_string(const Error&) -> std::string` function is provided. A `make_error(Category, message, code = 0)` helper returns `std::unexpected<Error>`. A `template<typename T> using Result = std::expected<T, Error>` alias is also defined. | File compiles; `Result<int>` is usable in code; `to_string(error)` produces a non-empty string; `make_error(Category::InitFailed, "test")` compiles and works. |
| AC-006 | A `Backend` enum class exists (in `src/engine/platform/platform.h` or a dedicated header) with values `SDL3` and `Headless`. | Code compiles; `Backend::SDL3` and `Backend::Headless` are valid identifiers. |
| AC-007 | A concrete `PlatformSDL3` class (descendant of `Platform`) exists; `Platform::create(Backend::SDL3)` instantiates it; construction calls `SDL_Init(SDL_INIT_VIDEO)`; destruction calls `SDL_Quit()`. | Unit test verifies `Platform::create(Backend::SDL3)` succeeds; SDL3 is initialized. |
| AC-008 | A concrete `WindowSDL3` class (descendant of `Window`) exists; `Platform::create_window()` instantiates it via `SDL_CreateWindow()` with `SDL_WINDOW_OPENGL` flag. | Unit test verifies `create_window()` returns a valid `Window`; the native handle is non-null. |
| AC-009 | A concrete `RenderDeviceOpenGL` class (descendant of `RenderDevice`) exists; `RenderDevice::create(Window&)` instantiates it; it creates an OpenGL 4.5 Core profile context via `SDL_GL_CreateContext()`. | Unit test verifies `RenderDevice::create()` succeeds; `size()` matches the window dimensions. |
| AC-010 | `RenderDeviceOpenGL` creates an OpenGL 4.5 Core profile context with debug context in debug builds. | After `RenderDevice::create()` succeeds, `glGetString(GL_VERSION)` reports a version >= 4.5; `glGetString(GL_SHADING_LANGUAGE_VERSION)` returns a corresponding version; context profile is Core (verified via implementation or code review). |
| AC-011 | `begin_frame()` clears the render surface with `glClear(GL_COLOR_BUFFER_BIT)`; `end_frame()` calls `SDL_GL_SwapWindow()`. | Unit test (SDL3 backend) runs a frame cycle without crash; visual verification is manual. |
| AC-012 | Headless backend classes (`PlatformHeadless`, `WindowHeadless`, `RenderDeviceHeadless`) exist and implement all pure virtual methods without calling SDL3 or OpenGL. | `Platform::create(Backend::Headless)` returns a valid `Platform`; `create_window()` returns a valid `Window`; `RenderDevice::create()` returns a valid `RenderDevice`; none link SDL3 or OpenGL symbols. |
| AC-013 | Both SDL3 and headless backends compile without warnings (with `-Wall -Wextra` or equivalent enabled) on the reference compiler. | Build with `cmake --build` produces zero warnings related to the backend source files. |
| AC-014 | The `Platform`, `Window`, and `RenderDevice` abstract classes are non-copyable and non-movable. | Compilation of `static_assert(!std::is_copy_constructible_v<Platform>)` passes; move operations are deleted. |
| AC-015 | An architecture boundary is enforced: no code outside `src/engine/` includes SDL3 or OpenGL headers. | Code review catches violations; documented as a manual-review rule. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A developer can build and run all unit tests in headless mode on a machine without a display (e.g., CI runner) in minimal time. | Run `cmake --build --preset debug && ctest --preset debug` — headless tests pass without display. |
| SC-002 | Switching between backends requires changing only one function argument (`Backend::SDL3` ↔ `Backend::Headless`); no rebuild needed unless SDL3 dependency changes. | A single source file changes `Platform::create(Backend::SDL3)` to `Platform::create(Backend::Headless)` and vice versa; both compile and run. |
| SC-003 | All three abstract interfaces expose exactly zero external library types in their public headers (no `SDL_*`, `GL*`, or other platform types). | `grep -E '(SDL_|gl[A-Z]|GL_|GLAD)' src/engine/platform/platform.h src/engine/window/window.h src/engine/render/render_device.h src/engine/error.h` returns no matches. |

## Edge cases

| Case | Expected behavior |
|---|---|
| Platform created but no window created before destruction | `SDL_Quit()` (or equivalent backend shutdown) still releases all SDL3 resources; no leak. |
| Window created but no render device before destruction | Window is destroyed cleanly; no OpenGL context is leaked (none was created). |
| Render device created and window destroyed before render device | This violates the intended lifecycle (Window should outlive RenderDevice). Behavior is undefined at the abstract level; the concrete SDL3 implementation may crash or produce a use-after-free. The spec documents this as a developer responsibility (see Assumptions). |
| `Platform::create()` called twice without destroying the first instance | The second call returns a new independent `Platform` instance. Each instance manages its own lifecycle. For the SDL3 backend, `SDL_Init` is ref-counted internally so nested calls are safe. |
| Headless window — calling the native handle accessor | Returns a null handle or a sentinel value (e.g., `nullptr`). The spec explicitly allows this. |
| Multiple windows from a single platform | Out of scope; `create_window` may be called only once before destruction. Behavior for multiple calls is undefined. |
| Window size set to zero or negative values in `WindowConfig` | `WindowCreationFailed` error is returned; no window is created. |
| Window size larger than the desktop resolution | The window is created at the requested size (clipped by the window manager as usual). This is a platform-dependent behavior; no specific error is reported. |

## Error cases

| Case | Expected behavior |
|---|---|
| SDL3 `SDL_Init` fails (e.g., no video driver available) | `Platform::create()` returns `make_error(Error::Category::InitFailed, "SDL_Init failed: <details>")`. |
| SDL3 `SDL_CreateWindow` fails | `Platform::create_window()` returns `make_error(Error::Category::WindowCreationFailed, "SDL_CreateWindow failed: <details>")`. |
| `SDL_GL_SetAttribute` or `SDL_GL_CreateContext` fails | `RenderDevice::create()` returns `make_error(Error::Category::RenderDeviceCreationFailed, "SDL_GL context creation failed: <details>")`. |
| `WindowConfig` title is empty string | Window is created with an empty title (transmitted as-is to SDL3). This is an allowed case — no error. |
| `WindowConfig` width or height is ≤ 0 | `create_window()` returns `make_error(Error::Category::WindowCreationFailed, "Invalid window dimensions")`. |
| SDL3 not found at build time via `FetchContent` | CMake configure fails with a clear FetchContent error message indicating SDL3 was not found. |
| OpenGL 4.5 Core profile not available on the system | `RenderDevice::create()` returns `make_error(Error::Category::RenderDeviceCreationFailed, "OpenGL 4.5 Core not available")` with diagnostic logging. |
| GPU does not support OpenGL 4.5 (e.g., software renderer) | Same as above — `RenderDeviceCreationFailed` is returned. |

## Permissions and security

- No elevated privileges are required to create windows or render devices.
- SDL3 and OpenGL run in the user's session context; no additional authentication or authorization is needed.
- No secrets, credentials, or environment variables are consumed by the abstraction layer.
- The headless backend requires no display server access, making it safe for CI environments without GPU or X11/Wayland.

## Observability

All observability uses `std::cerr` directly. No logging framework is specified or required at this stage.

| Signal | Source |
|---|---|
| Backend selected | `std::cerr << "Platform backend: SDL3\n"` or `"Platform backend: Headless\n"` on `Platform::create()` |
| Platform initialization success/failure | `std::cerr << "Platform initialized\n"` or `std::cerr << "Platform init failed: " << to_string(error) << "\n"` |
| Window creation success/failure | `std::cerr << "Window created: " << width << "x" << height << "\n"` or similar on `create_window()` |
| Render device creation success/failure | `std::cerr << "Render device created (" << backend << ")\n"` or similar on `RenderDevice::create()` |

## Out of scope

- Input event handling, polling, or callbacks.
- Audio, haptic, or sensor subsystems.
- Multiple window support.
- Fullscreen, borderless, or window-mode transitions post-creation.
- Vulkan, DirectX, Metal, or WebGPU render backends.
- Render pipeline abstractions (shaders, buffers, textures, draw calls).
- GPU resource management, memory allocators, or command buffers.
- ECS, physics, audio, or any game runtime system.
- Backend switching after `Platform::create()` (the backend is fixed for the Platform lifetime).
- Integration with a logging framework — observability uses `std::cerr`.
- CI configuration for enforcing the architecture boundary.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | SDL3 is fetched via CMake `FetchContent` from the SDL GitHub repository. This is the only discovery mechanism at this stage. |
| A-02 | OpenGL headers are available on the build system (via `find_package(OpenGL REQUIRED)` or equivalent). The implementation uses the system's `GL/glcorearb.h` or an OpenGL loader like `glad`. |
| A-03 | The reference compiler and platform support C++26 `std::expected` (GCC 14+, Clang 19+, or equivalent). This is consistent with SPEC-001 Assumption A-04. |
| A-04 | The headless backend does not require any platform-specific headers or libraries beyond the C++ standard library. It is always compiled alongside the SDL3 backend. |
| A-05 | `Window` must outlive the `RenderDevice` that was created from it. Violating this is undefined behavior at the abstract level. |
| A-06 | `Platform` must outlive any `Window` and `RenderDevice` created from it. Violating this is undefined behavior. |
| A-07 | The native handle accessor on `Window` returns a type alias (e.g., `using native_handle = void*`) that is opaque to consumers. For the SDL3 backend, this is a `SDL_Window*` cast to `void*`. For the headless backend, this is `nullptr`. |
| A-08 | `WindowConfig` uses `std::string` for the title field and simple `int` for dimensions. No builder pattern is needed at this stage. |
| A-09 | The `Error` struct contains an `enum class Category`, an `int code = 0` for backend-specific numeric error codes (defaults to 0), and a `std::string message`. A standalone `auto to_string(const Error&) -> std::string` function is provided for logging, formatting the error as `"<category>: <message> (code <code>)"`. |
| A-10 | The SDL3 backend initializes only `SDL_INIT_VIDEO`. Other subsystems (events, joystick, audio) are not initialized and will be added by future features when needed. |
| A-11 | `end_frame()` synchronizes (swap buffers) — no explicit `glFinish()` or fence is added at this stage. The implementation may add a `glFinish()` call in debug builds for diagnostic purposes. |
| A-12 | The project uses a flat namespace `buddd::engine` for public types. Concrete backend classes may be placed in an anonymous namespace or a private detail namespace — the spec does not mandate an internal namespace structure. |
| A-13 | Move semantics for `Platform`, `Window`, and `RenderDevice` are not required at this stage. The classes are non-copyable and not movable. If move support is needed later, it can be added as a separate change. |

## Open questions

| ID | Question | Impact |
|---|---|---|---|
| Q-01 | [RESOLVED] `Platform::create()` returns `Result<std::unique_ptr<Platform>>` (pointer semantics via `Result<T>` where `T` is `std::unique_ptr<Platform>`). The `Result<T>` pattern becomes a project-wide standard using `Error` struct (with `Category` enum + `message` string). | **Scope**: API ergonomics and project-wide error convention. |
| Q-02 | [RESOLVED] SDL3 is discovered via CMake `FetchContent` from the SDL GitHub repository, consistent with how Catch2 is already handled. | **Scope**: Build system and CI setup. |
| Q-03 | [RESOLVED] `begin_frame()` performs an automatic `glClear(GL_COLOR_BUFFER_BIT)` with a default clear color (black). The caller may change the clear color via a future `set_clear_color()` method (not in this spec). | **Scope**: Render device API semantics. |
