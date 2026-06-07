# Data Flow

## CLI data flow

At the bootstrap stage, the CLI binary uses a simple dispatch to three commands (`run`, `version`, `help`). The `run` command creates an `App` subclass and delegates to `run_app()`:

```
User invocation
      │
      ▼
main(int argc, char* argv[])
      │
      ├── argc < 2 or argv[1] == nullptr ?
      │       └── YES ──► RunApp → run_app()  ← default (empty window)
      │
      ├── argv[1] == "run"     ──► parse <scene>, create App subclass → run_app()
      ├── argv[1] == "version" ──► VersionCommand.run(argc, argv)
      ├── argv[1] == "help"    ──► HelpCommand.run(argc, argv)
      │
      └── Unknown command ──► BUDDD_LOG_ERROR("Unknown command: '{}'", argv[1])
                              fwrite(k_usage_text, stderr)  // usage text remains as fprintf(stderr)
                              return EXIT_FAILURE
```

For `run`, the scene dispatch is:

```
├── argv[2] == nullptr or starts with '-'  → RunApp (empty window)
├── argv[2] == "triangle"                  → TriangleApp
├── argv[2] == "cube"                      → CubeApp
├── argv[2] == "cube-scene"                → CubeSceneApp
├── argv[2] == "textured-cube"             → TexturedCubeApp
├── argv[2] == "free-camera"               → FreeCameraApp
├── argv[2] == "phong"                     → PhongApp
├── argv[2] == "asset-demo"               → AssetDemoApp
├── argv[2] == "hot-reload"               → HotReloadApp
├── argv[2] == "multi-material"           → MultiMaterialApp
├── argv[2] == "gltf-demo"               → GltfDemoApp
├── argv[2] == "gltf-helmet"             → GltfHelmetApp
├── argv[2] == "hot-reload-gltf"         → HotReloadGltfApp
└── Unknown scene                          → BUDDD_LOG_ERROR("Unknown scene: '{}'", scene_name)
                                             fwrite(scene_usage, stderr)  // usage text remains as fprintf(stderr)
                                             exit 1
```

Output:

| Invocation | stdout | stderr |
|---|---|---|
| `buddd` / `buddd run` | — | `"Window opened: 1024x768"`, then `"Window closed, shutting down."` (via `BUDDD_LOG_INFO`) |
| `buddd run <scene>` | — | Scene-specific messages: frame-limited scenes print `"Scene started: <name> (N frames)"` + `"Scene complete: <name> (N frames rendered)"`. Interactive scenes print `"Scene started: <name> (interactive)"` and `"Scene complete: <name> (interactive)"` on Escape. On window close: `"Scene aborted by user (frame N)"`. If unknown scene: `"Unknown scene: '<name>'"` + usage. |
| `buddd run asset-demo` | — | `"Scene started: asset-demo (120 frames)"`, `"Scene complete: asset-demo (120 frames rendered)"` (via `BUDDD_LOG_INFO`) |
| `buddd run <scene> --capture N:path` | — | `"Captured: <path>"` (via `BUDDD_LOG_INFO` on stderr, no longer on stdout). Capture messages merged into scene output |
| `buddd version` | `"buddd 0.1.0"` | — |
| `buddd help` | Usage text (3 commands: `run`, `version`, `help`) | — |
| Unknown (including `demo`, `capture`, `test`) | — | `"Unknown command: '<cmd>'"` + usage text |

The old `--test` and `--version` flags are removed — they are caught by the unknown-command handler.

## Test data flow

```
Catch2 test runner
      │
      ▼
TEST_CASE("engine version is non-empty", "[sanity]")
      │
      └── REQUIRE_FALSE(buddd::engine::version().empty())
              │
              └──► Calls version() → returns "0.1.0" → .empty() is false → test passes
```

## Version string source

The version string `"0.1.0"` is defined in a single location:

```
src/engine/version.cpp  ──►  return "0.1.0";
```

It is consumed by:
- `src/cmd/commands/version_command.cpp` (via `buddd::engine::version()`)
- `tests/version_test.cpp` (via `buddd::engine::version()`)

The version in `CMakeLists.txt` (`project(buddd VERSION 0.1.0 ...)`) must be kept in sync with `version.cpp` manually — no automation is introduced at bootstrap.

## EngineService lifecycle

As of SPEC-016 / ADR-012, the `EngineService` class owns the entire Platform → Window → RenderDevice chain. It is the single entry point for engine lifecycle in both tests and production:

```
EngineService::create(Backend, WindowConfig)
        │
        ├── 1. Platform::create(backend)
        │         │
        │         └── [Platform initialized]
        │             - SDL3 backend: SDL_Init(SDL_INIT_VIDEO) called
        │             - Headless backend: no-op initialization
        │
        ├── 2. platform->create_window(WindowConfig)
        │         │
        │         ├── Valid config (width>0, height>0)
        │         │       │
        │         │       └── [Window created with Platform& back-link]
        │         │           - SDL3 backend: SDL_CreateWindow with SDL_WINDOW_OPENGL flag
        │         │             WindowSDL3(sdl_window, w, h, *this)
        │         │           - Headless backend: in-memory width/height storage
        │         │             WindowHeadless(w, h, *this)
        │         │
        │         └── Invalid config (width≤0 or height≤0)
        │                 │
        │                 └── Error{WindowCreationFailed, "Invalid window dimensions"}
        │
        ├── 3. RenderDevice::create(window)
        │         │
        │         ├── native_handle() != nullptr (SDL3 backend)
        │         │       │
        │         │       └── [OpenGL 4.5 Core context created with 24-bit depth buffer]
        │         │           - RenderDeviceOpenGL(window, sdl_window, gl_context)
        │         │           - SDL_GL_SetAttribute for Core profile 4.5
        │         │           - SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)
        │         │           - SDL_GL_CreateContext, SDL_GL_MakeCurrent
        │         │           - GL_DEPTH_TEST (GL_LESS) enabled
        │         │
        │         └── native_handle() == nullptr (Headless backend)
        │                 │
        │                 └── [Headless render device]
        │                     - RenderDeviceHeadless(window)
        │                     - size() delegates to window_.width() / height()
        │                     - begin_frame() and end_frame() are no-ops
        │
        └── EngineService{platform, window, device}
            - Accessors: .platform(), .window(), .device()
            - Navigable graph: device().window().platform().input_system()
            - Member order guarantees destruction: ~device → ~window → ~platform
```

### Legacy manual lifecycle (pre-SPEC-016, pre-EngineService)

Before EngineService was introduced, the chain was constructed manually in `DemoCommand` and `CaptureCommand` (both now removed). EngineService now formalises this pattern, and `run_app()` uses it exclusively.

### Navigable object graph access

From any `RenderDevice&`, the full upstream graph is reachable without additional parameters:

```
RenderDevice& device
    │
    ├── device.window()                       → Window&
    │       │
    │       ├── .platform()                   → Platform&
    │       ├── .set_mouse_capture(bool)      → void
    │       └── .is_mouse_captured() → bool
    │
    └── device.window().platform()
            │
            ├── .input_system()               → InputSystem&
            ├── .delta_time()                 → float
            └── .poll_events()                → void
```

### Frame loop (after EngineService setup)

The frame loop lives in `run_app()` in `src/cmd/app.cpp`. `run_app()` creates a `World` and `RenderSystem` unconditionally for every app (empty World is ~1KB, `render_scene()` on empty World is a no-op). These are owned by `run_app()` and passed to the app via `EngineContext`.

```
    [Frame loop: run_app() orchestrates rendering each frame]
    - Before the loop:
        - Create World (std::unique_ptr)
        - Create RenderSystem(device, world) (std::unique_ptr)
        - Construct EngineContext with all 7 fields for setup()
        - app.setup(ctx) — one-time initialisation

    - Each frame:
        1. poll_events() — dispatches SDL events, computes delta_time, calls
           InputSystem::begin_frame() to advance input state. Returns false
           on window close → break.
        2. device->begin_frame() — clears buffers, starts GPU frame.
        3. Construct per-frame EngineContext with all 7 fields:
           {services, window, device, world, render_system, delta_time, frame}
        4. app.on_frame_begin(ctx) — per-frame hook (default no-op, apps override
           for tasks like hot-reload polling via ctx.services.assets().poll_file_events(),
           transform updates, camera animation). If ctx.is_exit_requested() → end_frame + break.
        5. World::update_updatables(ctx) — all registered Updatable components run.
           If ctx.is_exit_requested() → end_frame + break.
        6. render_system.render_scene() — automatic scene rendering (begin_frame/end_frame
           are owned by run_app(), render_scene only issues draw calls).
        7. app.on_render(ctx) — custom rendering overlay (default no-op, replaces old
           app.render(device, frame)). Runs AFTER render_scene().
        8. Capture injection (if --capture matches current frame).
        9. device->end_frame() — swap buffers, finalize GPU frame.
        10. ++frame

    - After the loop:
        - app.shutdown() — cleanup
        - World and RenderSystem destroyed by unique_ptr
    - Exit is signalled via EngineContext::request_exit() / is_exit_requested() only.
      The old App::is_running()/set_running()/running_ members are removed.
    - Updatable components receive the full EngineContext with all 7 fields.
```

### Texture data flow

Textures flow from loaded PNG files through the image subsystem to the render pipeline:

```
assets/brick.png
        │
        ▼
Image::load("assets/brick.png")
        │
        ├── Success ──► Image (width, height, channels=4, RGBA pixel data)
        │                    │
        │                    ▼
        │               RenderDevice::create_texture(image)
        │                    │
        │                    ├── OpenGL backend ──► TextureOpenGL (GPU texture via DSA)
        │                    │     glCreateTextures(GL_TEXTURE_2D, 1, &handle)
        │                    │     glTextureStorage2D(handle, 1, GL_RGBA8, w, h)
        │                    │     glTextureSubImage2D(handle, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data)
        │                    │     glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        │                    │     glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        │                    │     glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
        │                    │     glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
        │                    │
        │                    └── Headless backend ──► TextureHeadless (in-memory pixels)
        │
        ▼
Material::set_texture("uTexture", shared_ptr<Texture>)
        │
        ▼
Material::bind()  [called at draw time by RenderDevice::draw/draw_indexed]
        │
        ├── 1. glUseProgram(program)
        ├── 2. For each cached uniform: glUniform*(location, value)
        ├── 3. For each bound texture:
        │         glActiveTexture(GL_TEXTURE0 + unit)
        │         glBindTexture(GL_TEXTURE_2D, handle)
        │         glUniform1i(sampler_location, unit)
        │         unit++
        └── 4. (Ready for draw call)
```

The key design decisions in this flow:

- **Deferred uniform application**: `Material::set_uniform()` caches values but does NOT call `glUniform*` immediately. All GL state is applied in `Material::bind()`, which must be called after `glUseProgram()` — fixing the ordering bug where `glUniform*` calls before `glUseProgram()` were silently ignored.
- **Automatic texture unit management**: `MaterialOpenGL::bind()` assigns texture units sequentially (`GL_TEXTURE0`, `GL_TEXTURE1`, ...) per draw call, starting at 0 each time `bind()` is called.
- **`create_texture(const Image&)`**: accepts an `Image` (loaded PNG with row-flipping already applied via `Image::load` / `Image::create`). The image data is expected to be RGBA (4 channels). Returns `Result<std::unique_ptr<Texture>>`.

### Asset loading data flow (SPEC-019)

The Asset Manager provides ID-based lazy loading of assets from YAML metadata files. The flow for `AssetManager::create<T>(id)`:

```
User code:
    asset_manager.create<TextureAsset>("textures/brick")
                              │
                              ▼
    1. Compute path: base_path_ + "/" + id + ".yaml"
    2. Check cache_ — return cached if found (type-validated via dynamic_cast)
    3. Parse YAML with yaml-cpp (exceptions caught, converted to Result<T>)
    4. Validate `type` field matches T ("Texture" or "Material")
    5. Validate `version` field (must be 1)
    6. Load asset:
       ├── TextureAsset: read `source` path → Image::load() → device.create_texture()
       └── MaterialAsset:
           ├── Load vertex/fragment shader source from disk
           ├── Deduplicate ShaderProgram by (vert_path, frag_path):
           │   ├── If already compiled → reuse shared_ptr<ShaderProgram>
           │   └── Else → device.create_shader() + ShaderProgram::create() → cache
           ├── device.create_material(shared_ptr<ShaderProgram>)  ← new overload
           ├── Resolve texture refs (recursive create<TextureAsset>())
           ├── Apply constant overrides (set_uniform from YAML constants)
    7. Record dependencies in DependencyMap (YAML + all source files)
    8. Store in cache_, return shared_ptr<T>
```

Shader program deduplication flow:

```
Two materials with same (vert, frag) paths:
    Material A                    Material B
         │                             │
         ▼                             ▼
    both look up ShaderProgramKey{vert_path, frag_path}
         │                             │
         └──────────┬──────────────────┘
                    ▼
    shader_programs_[key] → shared_ptr<ShaderProgram>
                    │
         ┌──────────┴──────────┐
         ▼                     ▼
    Material A (own          Material B (own
    uniforms/textures)       uniforms/textures)
         │                     │
         └──────────┬──────────┘
                    ▼
    Both share same GL program handle (ShaderProgram)
    Each has independent Material object
```

Hot-reload flow (via `app.on_frame_begin()` which calls `poll_file_events()`):

```
FileWatcher thread (inotify):          Main thread:
    │                                       │
    ├── detects file change                 │
    ├── pushes FileEvent                    │
    │   to thread-safe queue                │
    │                                       ├── poll_file_events() called
    │                                       │   per frame by user code
    │                                       │
    │                                       ├── drains queue
    │                                       │   ├── YAML change:
    │                                       │   │   → reload asset metadata
    │                                       │   │   → re-resolve dependencies
    │                                       │   │   → update cache
    │                                       │   ├── Image file change:
    │                                       │   │   → reload Image
    │                                       │   │   → create new GPU texture
    │                                       │   │   → swap GL handle in-place
    │                                       │   │     (replace_gl_handle)
    │                                       │   └── Shader file change:
    │                                       │       → recompile shaders
    │                                       │       → new ShaderProgram
    │                                       │       → move-assign into
    │                                       │         existing shared_ptr
    │                                       │
    │                                       └── (Materials auto-see new
    │                                            handle at bind() time)
```

The FileWatcher is Linux-only (inotify) and watches **all subdirectories recursively** — every directory under the watch path gets an inotify watch added. On non-Linux or headless mode, a `NullFileWatcher` is used — `poll_file_events()` returns immediately with no events.

Hot-reload is **fully implemented** — `handle_yaml_change()` and `handle_source_change()` in `AssetManager` perform actual in-place GPU handle swaps (texture via `replace_gl_handle()`, shaders via `replace_handle()` on the `ShaderProgram`). See the `HotReloadApp` test app for verification.

### Error propagation

All factory methods (`Platform::create`, `create_window`, `RenderDevice::create`, `create_texture`, `AssetManager::create`) return `Result<T>` (`std::expected<T, Error>`). On failure they return `std::unexpected<Error>` constructed via `make_error()`. Two overloads simplify propagation: `make_error(const Error&)` (creates `std::unexpected<Error>` from an existing error) and `make_error(const Result<T>&)` (extracts the error from a failed `Result`). Use `return make_error(vs)` instead of `return std::unexpected(vs.error())`. The `Error` struct carries:
- `Category`: `InitFailed`, `WindowCreationFailed`, `RenderDeviceCreationFailed`, `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`, `ReadbackFailed`, `TextureCreationFailed`, `IoFailed`, `Unsupported`, `InputInitFailed`, `Unknown`
- `code`: backend-specific numeric error code (defaults to 0)
- `message`: human-readable description

### Phong rendering lifecycle (SPEC-018)

The `RenderSystem::render()` method is extended with a light collection phase before MeshRenderer iteration:

```
RenderSystem::render()
    │
    ├── 1. begin_frame()
    │
    ├── 2. active_camera() → camera_pos, view_projection
    │
    ├── 3. Collect lights (before MeshRenderer iteration)
    │       │
    │       ├── DirectionalLightComponent.each(...)
    │       │   └── LightData: position_or_dir.w = 0, direction from entity rotation (-Z forward)
    │       │       colour = colour * intensity (pre-multiplied)
    │       │
    │       ├── PointLightComponent.each(...)
    │       │   └── LightData: position_or_dir.w = 1, position from entity translation
    │       │       range from component, colour pre-multiplied
    │       │
    │       └── SpotLightComponent.each(...)
    │           └── LightData: position_or_dir.w = 2, position from translation,
    │               spot_direction from rotation, inner_cone/outer_cone as cos(angle)
    │
    │   Max 8 lights total (k_max_lights). Lights beyond limit are silently ignored
    │   (debug build logs warning). Collected fresh each frame via World::each<T>().
    │
    ├── 4. each<MeshRenderer>(...)
    │       │
    │       ├── Always: set_uniform("u_mvp", view_projection * world_matrix)
    │       │   ↳ On failure: log warning, skip entity (SPEC-011 AC-024 pattern)
    │       │
    │       ├── if material.has_uniform("u_model"):  ← Phong sentinel
    │       │   │
    │       │   ├── set_uniform("u_model", world_mat)
    │       │   ├── set_uniform("u_normal_mat", world_mat.inverse().transpose())
    │       │   ├── set_uniform("u_camera_pos", camera_pos)
    │       │   ├── set_uniform("u_light_count", light_count)
    │       │   ├── For each light i ∈ [0, light_count):
    │       │   │   ├── set_uniform("u_light_positions_or_dir[i]", ld.position_or_dir)
    │       │   │   ├── set_uniform("u_light_colours[i]", ld.colour)
    │       │   │   ├── set_uniform("u_light_ranges[i]", ld.range)
    │       │   │   ├── set_uniform("u_light_spot_directions[i]", ld.spot_direction)
    │       │   │   ├── set_uniform("u_light_inner_cones[i]", ld.inner_cone_cos)
    │       │   │   └── set_uniform("u_light_outer_cones[i]", ld.outer_cone_cos)
    │       │   │
    │       │   └── Material defaults:
    │       │       ├── set_uniform("u_material_ambient", Vec3(0.1))
    │       │       ├── set_uniform("u_material_specular", Vec3(1.0))
    │       │       ├── set_uniform("u_material_shininess", 32.0f)
    │       │       └── set_uniform("u_material_diffuse_tint", Vec4(1.0))
    │       │
    │       └── else: only u_mvp set (backward compatible with unlit materials)
    │
    ├── 5. model.draw(*device_)   ← draw call for each entity
    │
    └── 6. end_frame()

The `has_uniform("u_model")` sentinel pattern:
- `PhongMaterial` declares `u_model` in its known uniforms → has_uniform returns true → lighting uniforms set.
- Unlit materials (old cube shaders) do NOT declare `u_model` → has_uniform returns false → only u_mvp set.
- This provides zero-overhead backward compatibility: unlit rendering paths are not affected.

### Normal matrix computation

For each lit entity, the normal matrix is computed CPU-side:
```
normal_mat = world_mat.inverse().transpose()
```
The shader extracts the upper-left 3×3 via `mat3(u_normal_mat)` and applies it to the vertex normal. This correctly handles non-uniform scaling by preserving orthogonality.

### PhongMaterial uniform delegation

`PhongMaterial` uses PIMPL: it owns an inner `Material` created via `device.create_material()`. All `set_uniform()`, `has_uniform()`, `set_texture()`, and `bind()` calls delegate to this inner material.

### Light type encoding

Light type is encoded in `position_or_dir.w`:
- `0.0` = directional (direction stored in .xyz)
- `1.0` = point (position stored in .xyz)
- `2.0` = spot (position in .xyz, direction in separate `spot_direction` field)

### Phong shader fragment flow

```
Fragment shader (per-pixel):
    N = normalize(v_normal)
    V = normalize(u_camera_pos - v_world_pos)
    diffuse_colour = texture(u_diffuse_texture, v_texcoord).rgb * u_material_diffuse_tint.rgb
    final_colour = u_material_ambient * diffuse_colour    ← ambient term (outside light loop)

    for each light i:
        L, attenuation = computed from light type:
            - directional: L = normalize(pos_or_dir.xyz), attenuation = 1.0
            - point: L = light_to_frag / dist, attenuation = 1 - (clamp(dist/range,0,1))²
            - spot: same as point + cone falloff via spot_cone_attenuation(cos_angle, cos_inner, cos_outer)

        diffuse = diffuse_colour * light_col * max(dot(N, L), 0.0)        ← Lambert
        specular = u_material_specular * light_col * pow(max(dot(N, H), 0.0), u_material_shininess)  ← Blinn-Phong
        final_colour += (diffuse + specular) * attenuation

    frag_color = vec4(final_colour, 1.0)
```

### LightData array uniform naming

Light uniforms use bracket-syntax array naming convention:
```cpp
material.set_uniform("u_light_colours[0]", ld.colour);
material.set_uniform("u_light_colours[1]", ld2.colour);
```

`MaterialHeadless` uses `normalize_uniform_name()` to strip the `[N]` suffix before checking against declared base names in `known_uniforms`. The OpenGL backend resolves locations via `glGetUniformLocation` at call time.

### Lifecycle rules

- `Platform` must outlive any `Window` and `RenderDevice` created from it.
- `Window` must outlive the `RenderDevice` that was created from it.
- Violating these rules is undefined behavior at the abstract level.
- The backend is fixed for the lifetime of a `Platform` instance — no runtime switching.
- `EngineService` guarantees the lifecycle invariants via member declaration order (`platform_`, `window_`, `device_`), ensuring `~RenderDevice` → `~Window` → `~Platform` on destruction.
- All back-references (`Window::platform_`, `RenderDevice::window_`) are non-owning (`T&`), compliant with ADR-010. See ADR-012 for the full rationale.

## Reference

- Spec: [SPEC-001](/.specs/sprint-2026-05/project-setup/spec.md) — User-visible behavior, User stories 1-3
- Implementation contract: [IMPL-001](/.specs/sprint-2026-05/project-setup/implementation-contract.md) — section 7 (`main.cpp` behavior)
- Spec: [SPEC-002](/.specs/sprint-2026-05/platform-abstraction/spec.md) — User stories 1-5, Edge cases, Error cases
- Implementation contract: [IMPL-002](/.specs/sprint-2026-05/platform-abstraction/implementation-contract.md) — Required implementation behavior
- Spec: [SPEC-006](/.specs/sprint-2026-05/cli-command-system/spec.md) — CLI Command System: dispatch rules, command behaviors, output contracts
- Implementation contract: [IMPL-006](/.specs/sprint-2026-05/cli-command-system/implementation-contract.md) — Dispatch logic, output format correctness, edge cases
- Spec: [SPEC-007](/.specs/sprint-2026-05/cli-command-evolution/spec.md) — CLI Command Evolution: Demo System & Empty Run (historical — superseded by SPEC-008 CLI App System)
- Spec: [SPEC-008](/.specs/sprint-2026-06/cli-app-system/spec.md) — CLI App System (centralised render loop, App lifecycle, unified `run` command, capture support)
- Implementation contract: [IMPL-008](/.specs/sprint-2026-06/cli-app-system/implementation-contract.md) — CLI App System implementation
- Spec: [SPEC-010](/.specs/sprint-2026-05/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario) (historical — capture now integrated into `run_app()`)
- Spec: [SPEC-012](/.specs/sprint-2026-05/depth-handling/spec.md) — Depth Buffer Support (24-bit depth allocation, GL_DEPTH_TEST, per-frame depth clear)
- Spec: [SPEC-013](/.specs/sprint-2026-05/input-system/spec.md) — Input System (KeyCode, InputSystem, frame-based state model, Platform integration)
- Spec: [SPEC-016](/.specs/sprint-2026-05/architecture-refactor-device-window-platform/spec.md) — Architecture Refactor: Navigable Object Graph, EngineService
- ADR: [ADR-012](/docs/adr/ADR-012-navigable-object-graph-engine-service.md) — Navigable Object Graph, EngineService, and Abstract Interface Extensions
- ADR: [ADR-014](/docs/adr/ADR-014-cli-app-system.md) — CLI App System: centralised render loop with App lifecycle, unified `run` command (partially supersedes ADR-004)
