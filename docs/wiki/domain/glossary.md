# Domain Glossary

| Term | Definition |
|---|---|
| **Buddd Engine** | The C++26 game engine project. Consists of a static library with version API and platform abstraction layer, a CLI binary, an editor placeholder, and a test suite. |
| **buddd** | The CLI binary produced from `src/cmd/`. It links the engine library and prints version information. |
| **buddd_engine** | The static library target produced from `src/engine/`. The core engine library — exposes version API and platform abstraction layer. |
| **buddd_editor** | The INTERFACE library target produced from `src/editor/`. A structural placeholder for the future editor; no code is compiled. |
| **buddd_tests** | The test executable produced from `tests/`. Links `buddd_engine` and Catch2. |
| **Platform** | Abstract interface class (`buddd::engine::Platform`) that represents the platform/windowing subsystem. Created via `Platform::create(Backend)`. Manages lifecycle of the windowing backend. |
| **Window** | Abstract interface class (`buddd::engine::Window`) that represents a native window. Created via `Platform::create_window(WindowConfig)`. Exposes width/height getters and an opaque `native_handle()`. |
| **RenderDevice** | Abstract interface class (`buddd::engine::RenderDevice`) that represents a graphics rendering device. Created via `RenderDevice::create(Window&)`. Manages frame lifecycle (`begin_frame`/`end_frame`) and exposes framebuffer `size()`. |
| **Backend** | Enum class (`buddd::engine::Backend`) with values `SDL3` and `Headless` for runtime selection of the platform/windowing backend. |
| **WindowConfig** | Struct (`buddd::engine::WindowConfig`) with fields `title` (`std::string`), `width` (`int`), `height` (`int`). Passed to `Platform::create_window()`. |
| **Error** | Struct (`buddd::engine::Error`) with `Category` enum, `int code` (backend-specific), and `std::string message`. Returned via `Result<T>` on failure. |
| **Result\<T\>** | Template alias (`buddd::engine::Result<T> = std::expected<T, Error>`) used as the standard error-return pattern for all engine APIs. |
| **make_error** | Helper function returning `std::unexpected<Error>` for concise error construction in `Result<T>`-returning functions. |
| **SDL3 backend** | Concrete implementation of `Platform`, `Window`, and `RenderDevice` using SDL3 for windowing and OpenGL 4.5 Core for rendering. |
| **Headless backend** | Concrete implementation of `Platform`, `Window`, and `RenderDevice` with no external dependencies. All operations are in-memory no-ops. Used for unit testing without a display. |
| **Architecture boundary** | The rule that no code outside `src/engine/` may `#include` SDL3 or OpenGL headers directly — all access goes through the abstract interfaces. |
| **version API** | The function `buddd::engine::version() -> std::string_view` that returns the current engine version string `"0.1.0"`. |
| **CMake preset** | A named build configuration defined in `CMakePresets.json`. The project has `debug` and `release` presets. |
| **FetchContent** | CMake module used to automatically download Catch2 v3.7.0 and SDL3 (release-3.2.30) at configure time. No manual installation required. |
| **Catch2 v3** | The C++ unit testing framework used by the project. Fetched via `FetchContent` at version v3.7.0. |

## Version scheme

The project uses [Semantic Versioning](https://semver.org/) (major.minor.patch). The initial version is `0.1.0`.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Assumptions A-05 through A-09
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — API compatibility impact section
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — Conventions, Actors, Assumptions
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — API compatibility impact
