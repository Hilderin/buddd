# Business Rules

## CLI output behavior

| Input | Output | Exit code |
|---|---|---|
| `buddd` (no arguments) | `Buddd Engine v0.1.0` | 0 |
| `buddd --version` | `buddd 0.1.0` | 0 |
| `buddd --help` | `Buddd Engine v0.1.0` | 0 |
| `buddd <any other arguments>` | `Buddd Engine v0.1.0` | 0 |

- The only recognized flag is `--version` as the sole argument.
- All other argument combinations (including `--help`, multiple args, unknown flags) fall through to the greeting.
- There is no error output (stderr is empty) for any argument combination.
- The greeting message format is `Buddd Engine v<version>` with a trailing newline.
- The version output format is `buddd <version>` with a trailing newline.

## Version API contract

```cpp
namespace buddd::engine {
    auto version() -> std::string_view;
}
```

- The function returns a `std::string_view` pointing to a compile-time constant string.
- The return value is never empty (at minimum, it contains a valid version string).
- The initial return value is `"0.1.0"`.
- Changing the namespace, function name, return type, or semantic meaning of the returned string constitutes a breaking change.
- The `version.cpp` string and the `project()` VERSION in `CMakeLists.txt` must be kept in sync manually.

## Project conventions

- Source files use `snake_case` naming.
- Directory names use `snake_case`.
- Code formatting is enforced via `.clang-format` (LLVM style with 4-space indent, 100-column limit, `c++26` standard).
- CMake targets use `snake_case` naming.
- Formatting is applied by running `cmake --build --preset debug --target format`.

## Platform abstraction layer

### Error handling contract

```cpp
namespace buddd::engine {
    enum class Error::Category { InitFailed, WindowCreationFailed, RenderDeviceCreationFailed, ShaderCompilationFailed, LinkingFailed, ResourceCreationFailed, InvalidArgument, UniformNotFound, ReadbackFailed, TextureCreationFailed, IoFailed, InputInitFailed, Unsupported, Unknown };
    struct Error {
        Category category{Category::Unknown};
        int code{0};
        std::string message;
    };
    auto to_string(const Error&) -> std::string;  // format: "<Category>: <message> (code <code>)"
    auto make_error(Error::Category, std::string, int code = 0) -> std::unexpected<Error>;
    template<typename T> using Result = std::expected<T, Error>;
}
```

- `Result<T>` is the standard error-return pattern for all engine APIs going forward.
- `make_error()` is the standard way to construct error returns in `Result<T>`-returning functions.
- `to_string()` produces the format `"<Category>: <message> (code <code>)"`.
- The `Error` struct's `code` field carries a backend-specific numeric error code (e.g., a GLenum for OpenGL, or 0 when none applies).

### Backend selection

- `Backend` enum class has exactly two values: `SDL3` and `Headless`.
- Backend is selected at `Platform::create(Backend)` and is **fixed for the lifetime** of the `Platform` instance.
- No dynamic backend switching is supported.

### Lifecycle rules

- `Platform` must outlive any `Window` and `RenderDevice` created from it.
- `Window` must outlive the `RenderDevice` that was created from it.
- Violating these rules is undefined behavior at the abstract level; the concrete SDL3/OpenGL implementation may crash or produce a use-after-free.
- Abstract classes (`Platform`, `Window`, `RenderDevice`) are **non-copyable and non-movable**.

### Factory behavior

| Factory | Input | Success | Failure |
|---|---|---|---|
| `Platform::create(Backend)` | `Backend::SDL3` | Initializes SDL video subsystem; returns `unique_ptr<PlatformSDL3>` | Returns `InitFailed` error |
| `Platform::create(Backend)` | `Backend::Headless` | Returns `unique_ptr<PlatformHeadless>` (no external deps) | Never fails |
| `Platform::create_window(WindowConfig)` | Valid config (width>0, height>0) | Creates native window or headless equivalent | Returns `WindowCreationFailed` for invalid dimensions or SDL errors |
| `RenderDevice::create(Window&)` | Non-null native handle | Creates OpenGL 4.5 Core context | Returns `RenderDeviceCreationFailed` |
| `RenderDevice::create(Window&)` | Null native handle (headless) | Returns `RenderDeviceHeadless` | Never fails |

### WindowConfig validation

- `width` and `height` must both be > 0. If either is ≤ 0, `create_window()` returns `make_error(Error::Category::WindowCreationFailed, "Invalid window dimensions")`.
- An empty `title` string is allowed — the window is created with an empty title.

### Architecture boundary

A hard architecture boundary is enforced: **no code outside `src/engine/`** may `#include <SDL3/`, `<GL/`, `<glad/`, or any graphics-library header. All platform/graphics/input access goes through the abstract `Platform`, `Window`, `RenderDevice`, and `InputSystem` interfaces. Concrete backend implementations (SDL3, OpenGL) live entirely within `src/engine/`. Violations are caught by code review.

### File and directory naming

- `snake_case` for source files and directories (e.g., `platform_sdl3.cpp`, `render/`).
- PascalCase for classes (e.g., `PlatformSDL3`, `RenderDeviceOpenGL`).
- No `I` prefix for abstract interfaces (e.g., `Platform`, not `IPlatform`).
- Concrete implementations append the backend name (e.g., `PlatformSDL3`, `WindowHeadless`).

### Namespace conventions

- All public types live under `buddd::engine`.
- Concrete backends may use nested namespaces (e.g., `buddd::engine::detail`) for internal symbols.
- Public interface headers expose only `buddd::engine`.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — User-visible behavior, User stories 1-3, Conventions
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 5, 7 (version and CLI behavior)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — User stories, Acceptance criteria, Error cases, Assumptions
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — Required implementation behavior, Edge cases
- Spec: [SPEC-013](/docs/specs/input-system/spec.md) — Input System (KeyCode, InputSystem, SDL3/Headless backends, Platform integration, frame-based state model)
