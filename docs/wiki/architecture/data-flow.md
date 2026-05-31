# Data Flow

## CLI data flow

At the bootstrap stage, the CLI binary uses a Command pattern dispatch:

```
User invocation
      │
      ▼
main(int argc, char* argv[])
      │
      ├── argc < 2 or argv[1] == nullptr ?
      │       └── YES ──► RunCommand.run(argc, argv) ← default
      │
      ├── argv[1] == "run"     ──► RunCommand.run(argc, argv)
      ├── argv[1] == "demo"    ──► DemoCommand.run(argc, argv)
      ├── argv[1] == "capture" ──► CaptureCommand.run(argc, argv)
      ├── argv[1] == "version" ──► VersionCommand.run(argc, argv)
      ├── argv[1] == "help"    ──► HelpCommand.run(argc, argv)
      │
      └── Unknown command ──► fprintf(stderr, "Unknown command: '%s'\n", argv[1])
                              fwrite(k_usage_text, stderr)
                              return EXIT_FAILURE
```

Each command produces its own output:

| Command | stdout | stderr |
|---|---|---|
| `run` / (default) | `"Window opened: 1024x768"` then `"Window closed, shutting down."` | — |
| `demo <name>` | — | `"Demo started: <name> (N frames)"` then `"Demo complete: <name> (N frames rendered)"` (or abort: `"Demo aborted by user (frame N)"`). Interactive demos (`free-camera`) print `"Demo started: free-camera (interactive)"`. On Escape, they exit with `"Demo complete: free-camera (interactive)"` via `std::cerr`. On window close, they exit with `"Demo aborted by user"` via `std::cerr`. If no name: demo usage text. If unknown name: `"Unknown demo: '<name>'"` + usage. |
| `version` | `"buddd 0.1.0"` | — |
| `help` | Usage text (5 commands: `run`, `demo`, `capture`, `version`, `help`) | — |
| `capture <scenario> [--frame N] [path]` | `"Captured: <path>"` | `"Capturing: <scenario> (N frame(s))"` then error or success. If no scenario: `"Usage: buddd capture <scenario>"` + scenario list. If unknown scenario: `"Unknown capture scenario: '<name>'"` + usage. If extra args: `"Warning: unexpected arguments..."`. |
| Unknown (including `test`) | — | `"Unknown command: '<cmd>'"` + usage text |

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

Before EngineService was introduced, the chain was constructed manually in `demo_command.cpp` and `CaptureCommand`. EngineService now formalises this pattern.

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

```
    [Frame loop: poll_events() orchestrates input and rendering]
    - Each poll_events() call:
        1. Computes delta_time from SDL_GetTicks () — time since the previous
           poll_events() call in seconds. First call after construction returns 1/60.
        2. Calls InputSystem::begin_frame() — copies current→previous state, resets
           accumulated mouse delta/wheel to zero.
        3. Processes SDL events — routes non-quit events to InputSystemSDL3::on_sdl_event()
           (keyboard, mouse-motion, mouse-button, mouse-wheel).
    - Application queries input state via device.window().platform().input_system()
      and delta time via device.window().platform().delta_time() between
      poll_events() and render/update logic.
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

### Error propagation

All factory methods (`Platform::create`, `create_window`, `RenderDevice::create`, `create_texture`) return `Result<T>` (`std::expected<T, Error>`). On failure they return `std::unexpected<Error>` constructed via `make_error()`. The `Error` struct carries:
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

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — User-visible behavior, User stories 1-3
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — section 7 (`main.cpp` behavior)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — User stories 1-5, Edge cases, Error cases
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — Required implementation behavior
- Spec: [SPEC-006](/docs/specs/cli-command-system/spec.md) — CLI Command System: dispatch rules, command behaviors, output contracts
- Implementation contract: [IMPL-006](/docs/specs/cli-command-system/implementation-contract.md) — Dispatch logic, output format correctness, edge cases
- Spec: [SPEC-007](/docs/specs/cli-command-evolution/spec.md) — CLI Command Evolution: Demo System & Empty Run
- Implementation contract: [IMPL-007](/docs/specs/cli-command-evolution/implementation-contract.md) — Demo dispatch, RunCommand simplification, output text changes
- Spec: [SPEC-009](/docs/specs/3d-cube-demo/spec.md) — Model Utility & 3D Cube Demo
- Implementation contract: [IMPL-009](/docs/specs/3d-cube-demo/implementation-contract.md) — Cube demo dispatch integration, output messages
- Spec: [SPEC-010](/docs/specs/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario)
- Implementation contract: [IMPL-010](/docs/specs/capture/implementation-contract.md)
- Spec: [SPEC-012](/docs/specs/depth-handling/spec.md) — Depth Buffer Support (24-bit depth allocation, GL_DEPTH_TEST, per-frame depth clear)
- Implementation contract: [IMPL-012](/docs/specs/depth-handling/implementation-contract.md)
- Spec: [SPEC-013](/docs/specs/input-system/spec.md) — Input System (KeyCode, InputSystem, frame-based state model, Platform integration)
- Spec: [SPEC-016](/docs/specs/architecture-refactor-device-window-platform/spec.md) — Architecture Refactor: Navigable Object Graph, EngineService
- ADR: [ADR-012](/docs/adr/012-navigable-object-graph-engine-service.md) — Navigable Object Graph, EngineService, and Abstract Interface Extensions
