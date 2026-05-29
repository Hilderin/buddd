# Domain Glossary

| Term | Definition |
|---|---|
| **Buddd Engine** | The C++26 game engine project. Consists of a static library with version API and platform abstraction layer, a CLI binary, an editor placeholder, and a test suite. |
| **buddd** | The CLI binary produced from `src/cmd/`. It links the engine library and prints version information. |
| **buddd_engine** | The static library target produced from `src/engine/`. The core engine library — exposes version API, platform abstraction layer, and math foundations module (Vec2, Vec3, Vec4, Mat4, Quat, Camera). |
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
| **Architecture boundary** | The rule that no code outside `src/engine/` may `#include` SDL3, OpenGL, or GLM headers directly — all platform/graphics access goes through the abstract interfaces, and all math access goes through the wrapper types (Vec2, Vec3, Vec4, Mat4, Quat). |
| **version API** | The function `buddd::engine::version() -> std::string_view` that returns the current engine version string `"0.1.0"`. |
| **CMake preset** | A named build configuration defined in `CMakePresets.json`. The project has `debug` and `release` presets. |
| **FetchContent** | CMake module used to automatically download Catch2 v3.7.0, SDL3 (release-3.2.30), and GLM (1.0.1) at configure time. No manual installation required. |
| **Catch2 v3** | The C++ unit testing framework used by the project. Fetched via `FetchContent` at version v3.7.0. |
| **BUDDD_HAS_DISPLAY** | CMake option (default `ON`) that controls whether SDL3 backend tests are compiled. Set to `OFF` (e.g., `cmake -DBUDDD_HAS_DISPLAY=OFF`) to exclude SDL3 backend tests in headless environments like CI. |
| **Offscreen video driver** | An SDL3 video driver (set via `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`) that renders to an offscreen framebuffer instead of a physical display. Used by SDL3 backend tests so they run in any environment without requiring a display server. |

### Math module terms

| Term | Definition |
|---|---|
| **Math module** | The `buddd::engine::math` subsystem under `src/engine/math/` providing vector, matrix, quaternion, and camera types for the engine. |
| **GLM** | OpenGL Mathematics (`glm`) — a header-only C++ math library used as the zero-overhead implementation backend for all math wrapper types. Fetched via `FetchContent` at tag `1.0.1`. |
| **Vec2** | A 2D vector struct (`buddd::engine::math::Vec2`) with public `x`, `y` members. Wraps `glm::vec2`. Provides arithmetic, `length()`, `normalize()`, `normalized()`, `dot()`, and constants (`zero()`, `one()`, `unit_x()`, `unit_y()`). Header-only. |
| **Vec3** | A 3D vector struct (`buddd::engine::math::Vec3`) with public `x`, `y`, `z` members. Wraps `glm::vec3`. Same operations as Vec2 plus `cross()`, `lerp()`, and `unit_z()`. Header-only. |
| **Vec4** | A 4D vector struct (`buddd::engine::math::Vec4`) with public `x`, `y`, `z`, `w` members. Wraps `glm::vec4`. Supports homogeneous coordinates. Header-only. |
| **Mat4** | A 4×4 column-major matrix struct (`buddd::engine::math::Mat4`) wrapping `glm::mat4`. Supports matrix arithmetic, `transpose()`, `inverse()`, `determinant()`, and static factories (`identity()`, `perspective()`, `ortho()`, `look_at()`, `translate()`, `rotate()`, `scale()`). Header-only. Memory layout is directly compatible with `glUniformMatrix4fv` with `GL_FALSE` for the transpose parameter. |
| **Quat** | A quaternion struct (`buddd::engine::math::Quat`) with public `w`, `x`, `y`, `z` members. Wraps `glm::quat`. Supports composition (`*`), vector rotation, `conjugate()`, `inverse()`, `to_mat4()`, `slerp()`, `angle_axis()`, `from_euler()`. Header-only. |
| **Camera** | A perspective camera class (`buddd::engine::math::Camera`) with position (`Vec3`), orientation (`Quat`), and perspective parameters (FOV, aspect, near, far). Computes `view_matrix()` (via `Mat4::look_at` with forward/up derived from orientation), `projection_matrix()` (via `Mat4::perspective`), and `view_projection_matrix()`. Right-handed, Y-up, OpenGL convention (looks down -Z). Only math type with a `.cpp` file. |
| **Wrapper type** | One of Vec2, Vec3, Vec4, Mat4, Quat — a C++ struct that wraps a corresponding GLM type with identical memory layout. Each wrapper provides a `.glm()` accessor returning a reference to the underlying GLM type via `reinterpret_cast` (zero-overhead, ABI-guaranteed by `static_assert`). |
| **Architecture boundary (math)** | The rule that no GLM headers may be `#include`d outside `src/engine/math/`. All math operations outside that directory must use the wrapper types. The `.glm()` accessor is the sole interop path. |
| **`.glm()` accessor** | A method on each wrapper type (Vec2, Vec3, Vec4, Mat4, Quat) that returns a `T&` / `const T&` reference to the underlying GLM type via `reinterpret_cast`. Safe because `static_assert` guarantees identical layout, size, and standard-layout conformance. |

## Version scheme

The project uses [Semantic Versioning](https://semver.org/) (major.minor.patch). The initial version is `0.1.0`.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Assumptions A-05 through A-09
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — API compatibility impact section
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — Conventions, Actors, Assumptions
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — API compatibility impact
- Spec: [SPEC-004](/docs/specs/math-foundations/spec.md) — Type specifications, memory layout, operations, GLM integration
- Implementation contract: [IMPL-004](/docs/specs/math-foundations/implementation-contract.md) — File definitions, delegation patterns
