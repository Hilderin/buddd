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

## Phong Lighting Rules (SPEC-018)

### Standard Vertex

- **Single vertex format**: The `Vertex` struct in `src/engine/render/vertex.h` (72B stride, 6 attributes) is used by ALL meshes. Unlit demos fill `position`+`color` (other fields zero); the Phong material uses `position`+`normal`+`texcoord`.
- **Future-proofing**: `tangent` (loc 4) is reserved for normal mapping; `texcoord2` (loc 5) is reserved for lightmaps.

### Light components

- **Three distinct types**: `DirectionalLightComponent` (infinite, direction from entity rotation), `PointLightComponent` (omni-directional, position from entity translation, range-limited), `SpotLightComponent` (conical, direction from rotation, inner/outer cone angles).
- **Light lifecycle**: Light components do NOT register with `World` — they are collected fresh each frame via `World::each<T>()` during `RenderSystem::render()`. Entity destruction is reflected on the next render call.
- **Colour * intensity pre-multiplied**: The GPU uniform `u_light_colours[i]` stores `(colour.r * intensity, colour.g * intensity, colour.b * intensity, 1.0)`. Zero intensity → black contribution.

### Light count limit

- **Maximum 8 lights total** across all three types combined (`k_max_lights = 8`). Lights beyond the limit are silently ignored. Debug builds log a warning. Collection order: directional → point → spot.

### PhongMaterial

- **Self-contained**: `PhongMaterial` creates its own vertex+fragment shaders from embedded GLSL strings — no external shader creation required.
- **`has_uniform("u_model")` sentinel**: This check in `RenderSystem::render()` distinguishes lit from unlit materials. If true, all 17 lighting uniforms are set. If false, only `u_mvp` is set (backward compatible).
- **Normal matrix**: `u_normal_mat = world_mat.inverse().transpose()` — computed CPU-side per entity. The shader extracts the upper-left 3×3 via `mat3(u_normal_mat)`.
- **Material defaults**: If the user does not override them, defaults are: `u_material_ambient = Vec3(0.1)`, `u_material_specular = Vec3(1.0)`, `u_material_shininess = 32.0`, `u_material_diffuse_tint = Vec4(1.0)`.

### Shader rules

- **Diffuse colour source**: Diffuse colour comes from texture sampling (`u_diffuse_texture`), NOT per-vertex colour. Per-vertex colour (`a_color`, loc 1) is declared for vertex format compatibility but unused in the Phong shader.
- **Ambient term**: `u_material_ambient * diffuse_colour` — applied once outside the light loop, so ambient is present even with zero lights.
- **Attenuation**: Squared distance-normalized falloff for point/spot lights: `1 - (clamp(dist/range, 0, 1))²`.
- **Spot cone falloff**: Smooth transition between inner and outer cone via `spot_cone_attenuation()`: `clamp((cos_angle - cos_outer) / (cos_inner - cos_outer), 0.0, 1.0)`.
- **Specular**: Blinn-Phong model with half-vector `H = normalize(L + V)`.

### Light type encoding in GPU

Light type is encoded in `position_or_dir.w`:
- `0.0` = directional (`position_or_dir.xyz` = direction)
- `1.0` = point (`position_or_dir.xyz` = position)
- `2.0` = spot (`position_or_dir.xyz` = position, `spot_direction.xyz` = normalized direction)

### Light component accessor pattern

All three light components follow the `CameraComponent` accessor pattern:
- Mutable + const overloads using trailing return types.
- Properties are publicly mutable via non-const accessor references.
- `on_attach()` is explicitly overridden as a no-op `{}`.
- Destructor is NOT declared (compiler-generated default is sufficient).

### glsl_util rules

- `extract_uniform_names()` parses GLSL source for `uniform` declarations, handling: `uniform type name;`, `uniform type name[N];`, `uniform type name = default;`, `uniform type name[N] = default;`, and `layout(...) uniform type name;`.
- `normalize_uniform_name()` strips `[N]` array subscript suffixes (e.g., `"u_light_colours[3]"` → `"u_light_colours"`).
- Both functions are shared by `RenderDeviceOpenGL` and `RenderDeviceHeadless` — replacing previously duplicated code.

### MaterialHeadless diagnostic accessors

`MaterialHeadless` adds four diagnostic getters for headless test verification:
- `get_uniform_vec3(name) -> optional<Vec3>`
- `get_uniform_vec4(name) -> optional<Vec4>`
- `get_uniform_float(name) -> optional<float>`
- `get_uniform_int(name) -> optional<int32_t>`

All methods (set_uniform, has_uniform, getters) use `normalize_uniform_name()` to resolve bracket-syntax array uniforms.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — User-visible behavior, User stories 1-3, Conventions
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 5, 7 (version and CLI behavior)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — User stories, Acceptance criteria, Error cases, Assumptions
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — Required implementation behavior, Edge cases
- Spec: [SPEC-013](/docs/specs/input-system/spec.md) — Input System (KeyCode, InputSystem, SDL3/Headless backends, Platform integration, frame-based state model)
- Spec: [SPEC-018](/docs/specs/lighting/spec.md) — Phong Lighting System (Standard Vertex, Light Components, Phong module, RenderSystem extension, Phong demo)
- Implementation contract: [IMPL-018-002](/docs/specs/lighting/implementation-contract.md) — Phong Lighting System implementation contract
