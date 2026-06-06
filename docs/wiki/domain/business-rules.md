# Business Rules

## CLI output behavior

### Commands

| Invocation | App | Behavior |
|---|---|---|
| `buddd` | `run` (no scene) | Empty window, interactive |
| `buddd run` | `RunApp` | Empty window, interactive |
| `buddd run triangle` | `TriangleApp` | Runs until window close |
| `buddd run cube` | `CubeApp` | Runs until window close |
| `buddd run cube-scene` | `CubeSceneApp` | Runs until window close |
| `buddd run textured-cube` | `TexturedCubeApp` | Runs until window close |
| `buddd run free-camera` | `FreeCameraApp` | Interactive, ESC to exit |
| `buddd run phong` | `PhongApp` | Interactive, ESC to exit |
| `buddd run asset-demo` | `AssetDemoApp` | Runs until window close (120 frames) |
| `buddd run hot-reload` | `HotReloadApp` | Runs until window close (60 frames), swaps texture at frame 30 |
| `buddd run <scene> --frame N` | (same App) | Limit to N frames |
| `buddd run <scene> --capture N:path` | (same App) | Capture frame N to path |
| `buddd version` | — | Prints version to stdout |
| `buddd help` | — | Prints usage text to stdout |
| `buddd <unknown>` | — | Error + usage to stderr, exit 1 |
| `buddd run <unknown>` | — | Error + scene usage to stderr, exit 1 |

### Available scenes

| Name | Description | Default behavior |
|---|---|---|
| (empty) | Interactive empty window (no scene) | Runs until window close |
| triangle | Coloured triangle | Runs until window close |
| cube | Rotating cube | Runs until window close |
| cube-scene | Cube via scene graph (World + RenderSystem) | Runs until window close |
| textured-cube | Textured cube with UV-mapped brick texture | Runs until window close |
| free-camera | Interactive free camera (WASD + mouse look, ESC to exit) | Interactive, ESC to exit |
| phong | Phong lighting demo (5 cubes + 5 lights) | Interactive, ESC to exit |
| asset-demo | Asset pipeline demo: textured cube loaded via YAML metadata | Runs until window close (120 frames) |
| hot-reload | Hot-reload test: swaps texture source at frame 30 to trigger `poll_file_events()` reload | Runs until window close (60 frames) |

### Flags for `buddd run`

- `--frame N`: Render exactly N frames, then exit. N >= 0. Default: 0 (interactive, no limit).
- `--capture N:path`: Capture frame N (1-based) to path. Repeatable.
- Unknown flags → silently ignored.
- Extra positional arguments after scene → warning printed to stderr, run proceeds.

### Observability messages

| Signal | Stream | Format |
|---|---|---|
| Window opened | stdout | `"Window opened: WxH\n"` |
| Scene started (limited) | stderr | `"Scene started: <title> (N frames)\n"` |
| Scene started (interactive) | stderr | `"Scene started: <title> (interactive)\n"` |
| Scene aborted (ESC) | stderr | `"Scene aborted by user (frame N)\n"` (N 1-based) |
| Scene aborted (window close) | stderr | `"Scene aborted by user\n"` |
| Scene completed | stderr | `"Scene complete: <title> (N frames rendered)\n"` |
| Capture saved | stdout | `"Captured: <path>\n"` |
| Window shutdown | stdout | `"Window closed, shutting down.\n"` |
| Unknown scene | stderr | `"Unknown scene: '<name>'\n\n"` + scene usage |
| Unknown command | stderr | `"Unknown command: '<cmd>'\n\n"` + usage |
| Error (parse, setup, etc.) | stderr | `"Error: <description>\n"` |

### Structured logging (new system as of SPEC-021)

Engine internal logging now uses the structured logger (`src/engine/log/`) instead of raw `std::cerr`/`printf`. The logger is a C++26 lightweight framework with five levels, hierarchical source tags, mutex-based thread safety, and multiple sinks.

**Console format** (stderr): `[LEVEL] [Tag] message\n`
**File format** (with `--log-file`): `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n`

**New CLI flags** (available on all `buddd run <scene>` invocations):

| Flag | Description |
|------|-------------|
| `--log-level=<level>` | Set global minimum log level (`trace`, `debug`, `info`, `warn`, `error`). Overrides build-type default. |
| `--log-file=<path>` | Enable file sink; output written in append mode with ISO 8601 timestamps. |
| `--log-filter=<pattern>=<level>` | Override level for tags matching a prefix (repeatable). E.g., `--log-filter=Asset:ModelLoader=trace`. |

**Examples:**
```bash
buddd run phong --log-level=debug --log-file=/tmp/phong.log
buddd run gltf-demo --log-level=info --log-filter=Asset:ModelLoader=trace
```

See the full API reference in [docs/wiki/domain/logging.md](/docs/wiki/domain/logging.md) or the specification in [SPEC-021](/.specs/sprint-2026-06/logging-system/spec.md).

### Exit codes

| Condition | Code |
|---|---|
| Normal completion | EXIT_SUCCESS (0) |
| Unknown command | EXIT_FAILURE (1) |
| Unknown scene | EXIT_FAILURE (1) |
| --frame parse error | EXIT_FAILURE (1) |
| --frame too small for captures | EXIT_FAILURE (1) |
| --capture parse error | EXIT_FAILURE (1) |
| Platform/Window/Device creation failure | EXIT_FAILURE (1) |
| app.setup() returns error | EXIT_FAILURE (1) |
| All captures fail | EXIT_FAILURE (1) |
| Some captures succeed, some fail | EXIT_SUCCESS (0) |
| Window closed early | EXIT_SUCCESS (0) |
| ESC pressed | EXIT_SUCCESS (0) |

### App lifecycle (from `run_app()`)

1. `app.config()` → `AppConfig`
2. `Platform::create(backend)`
3. `window = platform->create_window(AppConfig)`
4. print "Window opened: WxH"
5. `device = RenderDevice::create(window)`
6. `app.setup(device)` → if error, `shutdown()` and exit 1
7. print start message
8. Loop until frame limit or window close or ESC:
   a. `poll_events()`
   b. `device->begin_frame()`
   c. `app.on_frame_begin()` — per-frame hook (default no-op, overridden by apps like `AssetDemoApp` and `HotReloadApp` for hot-reload polling)
   d. `app.render(device, frame)`
   e. capture injection
   f. `device->end_frame()`
9. print completion/abort message
10. `app.shutdown()`
11. print "Window closed, shutting down."
12. return exit code

### Driver quirk

`--capture 1:path` captures frame 2 (not frame 1). The `CaptureSpec::effective_frame()` method returns `(frame < 2) ? 2 : frame`. This is a known off-by-one issue documented in ADR-014.

### Capture-frame auto-set and validation

When `--capture` is specified without `--frame`, the frame limit is auto-set to the maximum `effective_frame()` across all captures. When `--frame N` is explicitly set and `N < max_capture_effective_frame`, an error is printed to stderr and the program exits with code 1. These rules ensure captures always fire and never silently produce nothing. See SPEC-009 for details.

### Backend selection

Backend is compile-time via `BUDDD_HAS_DISPLAY` define. SDL3 when `ON`, Headless when `OFF`.

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

## Asset Manager Rules (SPEC-019)

### Asset ID naming convention

- **Asset ID = relative path from `assets/` without `.yaml` extension**. E.g., `"textures/brick"` maps to `assets/textures/brick.yaml`.
- Asset IDs are unique — they serve as cache keys in `AssetManager::create<T>(id)`.
- Asset IDs use forward slashes (`/`) as path separators regardless of platform.
- The `base_path` passed to `AssetManager::create()` defaults to `"assets"` (resolved relative to the working directory).

### YAML schemas

**Texture YAML** (`assets/textures/<name>.yaml`):
```yaml
type: Texture          # required, must be "Texture"
version: 1              # optional, default 1
source: path/to.png     # required, absolute or relative to CWD
settings:               # optional, parsed but NOT applied in V1
  wrap_s: repeat        # repeat / clamp / mirrored_repeat
  wrap_t: repeat
  min_filter: linear    # nearest / linear / nearest_mipmap_linear / linear_mipmap_linear
  mag_filter: linear    # nearest / linear
  generate_mipmaps: true
```

**Material YAML** (`assets/materials/<name>.yaml`):
```yaml
type: Material          # required, must be "Material"
version: 1              # optional, default 1
shaders:                # required
  vertex: path/to.vert  # required, absolute or relative to CWD
  fragment: path/to.frag # required
textures:               # optional — map sampler name → asset ID
  albedo: textures/brick
constants:              # optional — map uniform name → float value
  roughness: 0.5
```

### Loading rules

- Assets are loaded **lazily** — no filesystem scan at startup. The first `create<T>(id)` call parses the YAML and loads all dependencies.
- Loading the same asset ID twice returns the **cached instance** (same `shared_ptr` address). Cache persists for the lifetime of the `AssetManager`.
- `create<T>(id)` validates that the YAML `type` field matches the requested C++ type `T`. Mismatch returns `Error::Category::InvalidArgument`.
- `create<T>(id)` validates the `version` field. If present and not `1`, returns `Error::Category::Unsupported`.
- Texture `settings` fields (wrap, filter, mipmap) are **parsed and validated but NOT applied in V1**. GPU texture creation uses the current defaults (linear filtering, clamp-to-edge wrapping).

### Shader program deduplication

- Two `MaterialAsset`s with the same `(vertex_path, fragment_path)` share a single `shared_ptr<ShaderProgram>` — they share **only** the compiled GL program.
- Each `Material` retains independent uniform/texture state.
- The deduplication map lives inside `AssetManager` (`unordered_map<ShaderProgramKey, shared_ptr<ShaderProgram>>`).

### Hot-reload (V1 — fully implemented)

- The FileWatcher is Linux inotify only (`#ifdef __linux__`). On non-Linux or headless, `NullFileWatcher` is used.
- `poll_file_events()` must be called explicitly by user code — the engine does not call it automatically. Apps override `on_frame_begin()` to call `asset_manager_->poll_file_events()` once per frame.
- **Fully implemented**: `handle_yaml_change()` and `handle_source_change()` perform in-place GPU handle swaps:
  - **Texture source change**: reloads image, creates new GPU texture, extracts native GL handle via `release_gl_handle()`, injects into existing `Texture` via `replace_gl_handle()`.
  - **Shader source change**: recompiles both vertex+fragment shaders, creates new `ShaderProgram`, extracts handle via `release_handle()`, injects into existing shared `ShaderProgram` via `replace_handle()`.
  - **YAML metadata change**: re-parses YAML, updates dependency map, and (for materials) re-resolves texture bindings and constant overrides.
  - All existing `shared_ptr<Material>` / `shared_ptr<Texture>` references remain valid — handles swap transparently.
- **Recursive inotify**: `InotifyFileWatcher::add_watch_recursive()` walks the entire directory tree at startup, adding an inotify watch for every subdirectory. File events are reported with relative paths matching those stored in `DependencyMap`.
- **HotReloadApp**: Test app (`buddd run hot-reload`) loads a textured cube via YAML, swaps `hot_reload_a.png` → `hot_reload_live.png` at frame 30, and calls `poll_file_events()` to trigger reload. Use with dual `--capture` to verify before/after:
  ```
  buddd run hot-reload --frame 60 --capture 30:/tmp/before.png --capture 60:/tmp/after.png
  ```

### MaterialHeadless diagnostic accessors

`MaterialHeadless` adds four diagnostic getters for headless test verification:
- `get_uniform_vec3(name) -> optional<Vec3>`
- `get_uniform_vec4(name) -> optional<Vec4>`
- `get_uniform_float(name) -> optional<float>`
- `get_uniform_int(name) -> optional<int32_t>`

All methods (set_uniform, has_uniform, getters) use `normalize_uniform_name()` to resolve bracket-syntax array uniforms.

## Reference

- Spec: [SPEC-001](/.specs/sprint-2026-05/project-setup/spec.md) — Project conventions, Version API contract
- Implementation contract: [IMPL-001](/.specs/sprint-2026-05/project-setup/implementation-contract.md) — sections 5, 7 (version API contract only; CLI behavior superseded by SPEC-008)
- Spec: [SPEC-002](/.specs/sprint-2026-05/platform-abstraction/spec.md) — User stories, Acceptance criteria, Error cases, Assumptions
- Implementation contract: [IMPL-002](/.specs/sprint-2026-05/platform-abstraction/implementation-contract.md) — Required implementation behavior, Edge cases
- Spec: [SPEC-008](/.specs/sprint-2026-06/cli-app-system/spec.md) — CLI App System (commands, scenes, flags, observability, exit codes, app lifecycle)
- ADR: [ADR-014](/docs/adr/ADR-014-cli-app-system.md) — CLI App System lifecycle, driver quirk (--capture off-by-one)
- Spec: [SPEC-013](/.specs/sprint-2026-05/input-system/spec.md) — Input System (KeyCode, InputSystem, SDL3/Headless backends, Platform integration, frame-based state model)
- Spec: [SPEC-018](/.specs/sprint-2026-05/lighting/spec.md) — Phong Lighting System (Standard Vertex, Light Components, Phong module, RenderSystem extension, Phong demo)
- Implementation contract: [IMPL-018-002](/.specs/sprint-2026-05/lighting/implementation-contract.md) — Phong Lighting System implementation contract
