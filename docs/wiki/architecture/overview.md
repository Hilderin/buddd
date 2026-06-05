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
│   │   ├── math/            # Math foundations (Vec2, Vec3, Vec4, Mat4, Quat, Camera)
│   │   ├── scene/           # Scene graph (World, Entity, Transform, Component, light components)
│   │   ├── platform/        # Platform abstraction (Platform, Backend)
│   │   ├── window/          # Window abstraction (Window, WindowConfig)
│   │   ├── image/           # Image I/O (ImageBuffer, Image, stb_image)
│   │   ├── input/           # Input system (KeyCode, MouseButton, InputSystem)
│   │   ├── asset/           # Asset manager (Asset, TextureAsset, MaterialAsset, FileWatcher, DependencyMap)
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
- **External dependencies**: SDL3 (fetched via `FetchContent`), GLM (fetched via `FetchContent`), OpenGL (system `find_package`), stb (fetched via `FetchContent`)

## CMake targets

| Target | Type | Directory | Description |
|---|---|---|---|
| `buddd_engine` | Static library | `src/engine/` | Core engine; exposes version API, platform abstraction layer, math foundations module, scene graph module, render pipeline module, and image I/O module. Links SDL3, OpenGL, GLM, and stb. |
| `buddd` | Executable | `src/cmd/` | CLI binary; links `buddd_engine` |
| `buddd_editor` | INTERFACE library | `src/editor/` | Placeholder — no compiled sources |
| `buddd_tests` | Executable | `tests/` | Catch2 test binary; links `buddd_engine` |

## Engine library (`buddd_engine`) internal structure

```
src/engine/
├── CMakeLists.txt           # GLOB_RECURSE collects all .h/.cpp (includes GLM FetchContent)
├── engine_service.h/.cpp   # EngineService — owns Platform→Window→RenderDevice chain
├── version.h / version.cpp  # Version API
├── error.h                  # Error struct, Result<T>, make_error, to_string
├── math/                    # Math foundations
│   ├── math.h               # Convenience header — includes all math types, utilities
│   ├── vec2.h               # Vec2 — 2D vector wrapper around glm::vec2
│   ├── vec3.h               # Vec3 — 3D vector wrapper around glm::vec3
│   ├── vec4.h               # Vec4 — 4D vector wrapper around glm::vec4
│   ├── mat4.h               # Mat4 — 4×4 column-major matrix wrapper around glm::mat4
│   ├── quat.h               # Quat — quaternion wrapper around glm::quat
│   ├── camera.h             # Camera — perspective camera (declarations)
│   └── camera.cpp           # Camera — method implementations
├── scene/                    # Scene graph (World, Entity, Transform, Component, CameraComponent)
│   ├── entity_id.h           # EntityId — 8-byte handle (index + generation)
│   ├── transform.h           # Transform — position/rotation/scale with matrix computation
│   ├── component.h           # Component — polymorphic base class with entity awareness (world_, entity_id_, entity(), on_attach())
│   ├── camera_component.h    # CameraComponent — ECS component wrapping math::Camera, auto-registers with World
│   ├── camera_component.cpp  # CameraComponent lifecycle (register/unregister via on_attach/destructor)
│   ├── entity.h              # Entity — 16-byte lightweight handle
│   ├── entity.cpp            # Entity method implementations (delegates to World)
│   ├── world.h               # World — top-level container, entity lifecycle, deferred destruction
│   ├── world.cpp             # World implementation (internal EntityNode storage)
│   ├── directional_light_component.h/.cpp  # DirectionalLightComponent — infinite parallel light from entity rotation
│   ├── point_light_component.h/.cpp        # PointLightComponent — omni-directional light with position and range
│   └── spot_light_component.h/.cpp         # SpotLightComponent — conical light with position, direction, and cone angles
├── platform/
│   ├── platform.h           # Abstract Platform class, Backend enum
│   ├── platform.cpp         # Platform::create() factory
│   ├── platform_sdl3.h/cpp  # SDL3 backend (PlatformSDL3)
│   └── platform_headless.h/cpp # Headless backend (PlatformHeadless)
├── window/
│   ├── window.h             # Abstract Window class, WindowConfig struct
│   ├── window_sdl3.h/cpp    # SDL3 backend (WindowSDL3)
│   └── window_headless.h/cpp # Headless backend (WindowHeadless)
├── image/
│   ├── image_buffer.h       # ImageBuffer — raw GPU readback data (width, height, channels, bytes)
│   ├── image.h              # Image — create from buffer, load/save PNG
│   └── image.cpp            # Image implementation (stb_image, stb_image_write, row-flipping)
├── input/                    # Input system (Keyboard, Mouse)
│   ├── key_code.h           # Public header: KeyCode enum (uint8_t, values matching SDL_Scancode)
│   ├── input_system.h       # Public header: InputSystem abstract class, MouseButton enum
│   ├── input_system.cpp     # Factory: InputSystem::create(Backend)
│   ├── input_system_sdl3.h/cpp  # SDL3 backend (InputSystemSDL3)
│   └── input_system_headless.h/cpp # Headless backend (InputSystemHeadless)
├── asset/                    # Asset manager system
│   ├── asset.h              # Abstract Asset base class
│   ├── asset_id.h           # Asset ID utilities
│   ├── asset_manager.h      # AssetManager — core class: cache, create<T>(), poll_file_events()
│   ├── asset_manager.tpp    # create<T>() template implementation
│   ├── asset_manager.cpp    # Non-template implementation (load_texture, load_material, hot-reload)
│   ├── texture_asset.h/.cpp # TextureAsset — wraps shared_ptr<Texture>
│   ├── material_asset.h/.cpp # MaterialAsset — wraps shared_ptr<Material>
│   ├── dependency_map.h/.cpp # DependencyMap — bidirectional asset↔source tracking
│   ├── file_watcher.h       # FileWatcher abstract base + NullFileWatcher + FileEvent
│   ├── file_watcher.cpp     # FileWatcher::create() factory
│   ├── file_watcher_inotify.h/.cpp # InotifyFileWatcher (Linux inotify, thread + queue)
└── render/
    ├── primitive_topology.h        # PrimitiveTopology enum
    ├── vertex_format.h             # VertexAttributeType, VertexAttribute, VertexFormat
    ├── vertex.h                    # Standard Vertex struct (72B, 6 attributes: position loc0, color loc1, normal loc2, texcoord loc3, tangent loc4, texcoord2 loc5) + k_standard_vertex_format
    ├── glsl_util.h/.cpp            # Shared GLSL utility: extract_uniform_names(), normalize_uniform_name() — used by both backends
    ├── light_data.h                # LightData struct + k_max_lights (8) — internal detail header for GPU uniform passing
    ├── shader.h                    # Abstract Shader class, ShaderType enum
    ├── material.h                  # Abstract Material class (6 set_uniform overloads, set_texture/has_texture, bind for deferred state application)
    ├── texture.h                   # Abstract Texture class (width, height, channels)
    ├── vertex_buffer.h             # Abstract VertexBuffer class
    ├── index_buffer.h              # IndexType enum, abstract IndexBuffer class
    ├── render_device.h             # Abstract RenderDevice class (factories + draw + create_texture)
    ├── render_device.cpp           # RenderDevice::create() factory
    ├── render_device_opengl.h/cpp  # OpenGL 4.5 backend (+ factory/draw overrides + create_texture via DSA; uses glsl_util for uniform discovery)
    ├── render_device_headless.h/cpp# Headless backend (+ factory/draw overrides + in-memory create_texture; uses glsl_util for uniform discovery)
    ├── shader_opengl.h/cpp         # OpenGL shader backend (ShaderOpenGL)
    ├── shader_headless.h/cpp       # Headless shader backend (ShaderHeadless)
    ├── shader_program.h            # Abstract ShaderProgram base class (uint32_t handle, CONST-001)
    ├── shader_program.cpp          # ShaderProgram vtable + default implementations
    ├── shader_program_opengl.h/cpp # ShaderProgramOpenGL — GLuint wrapper, glLinkProgram
    ├── shader_program_headless.h/cpp # ShaderProgramHeadless — generation counter, simulated linking
    ├── material_opengl.h/cpp       # OpenGL material backend (MaterialOpenGL) — deferred uniform caching + texture unit management in bind()
    ├── material_headless.h/cpp     # Headless material backend (MaterialHeadless) — now with Vec3/Vec4/float/int diagnostic getters + array subscript normalization
    ├── texture_opengl.h/cpp        # OpenGL texture backend (TextureOpenGL) — DSA-based GPU upload via glCreateTextures/glTextureStorage2D/glTextureSubImage2D
    ├── texture_headless.h/cpp      # Headless texture backend (TextureHeadless) — in-memory pixel storage
    ├── vertex_buffer_opengl.h/cpp  # OpenGL vertex buffer backend (VertexBufferOpenGL)
    ├── vertex_buffer_headless.h/cpp# Headless vertex buffer backend (VertexBufferHeadless)
    ├── index_buffer_opengl.h/cpp   # OpenGL index buffer backend (IndexBufferOpenGL)
    ├── index_buffer_headless.h/cpp # Headless index buffer backend (IndexBufferHeadless)
    ├── model.h                     # Model — concrete utility class bundling VertexBuffer + optional IndexBuffer + shared_ptr<Material>
    ├── model.cpp                   # Model implementation (factory methods, draw dispatch)
    ├── mesh_renderer.h             # MeshRenderer — ECS component (inherits Component) holding shared_ptr<Model>
    ├── mesh_renderer.cpp           # MeshRenderer implementation
    ├── render_system.h             # RenderSystem — bridges RenderDevice + World, orchestrates frame rendering
    ├── render_system.cpp           # RenderSystem implementation (begin/end_frame, camera lookup, each<MeshRenderer> iteration, MVP + lighting uniform setting, light collection)
    └── phong/                      # Phong lighting module
        ├── phong_shaders.h         # Embedded GLSL 450 core vertex + fragment shader strings (constexpr std::string_view)
        ├── phong_material.h        # PhongMaterial — Material subclass with embedded shaders, convenience setters (set_camera_position, set_lights, set_transforms)
        └── phong_material.cpp      # PhongMaterial implementation (PIMPL, delegates to inner Material)
```

## Key behaviors

- `./build/debug/src/cmd/buddd` (or `buddd run`) — opens a window (1024×768), clears the framebuffer each frame (no draw calls), runs until the user closes the window
- `./build/debug/src/cmd/buddd run triangle` — opens a window (1024×768, title "Buddd Engine"), renders a coloured triangle for exactly 120 frames at ~60 FPS, then exits automatically.
- `./build/debug/src/cmd/buddd run cube` — opens a window (1024×768), renders a rotating per-face-coloured cube (24 vertices, 36 indices) for 120 frames at ~60 FPS with MVP computed from a Camera at (3,2,3), then exits.
- `./build/debug/src/cmd/buddd run textured-cube` — opens a window (1024×768), renders a rotating UV-mapped cube with a brick texture loaded from `assets/brick.png` for 120 frames at ~60 FPS using the scene graph (World + Entity + MeshRenderer + RenderSystem), then exits.
- `./build/debug/src/cmd/buddd run free-camera` — opens a window (1024×768), renders a cube from a controllable camera (WASD for forward/back/strafe, mouse for look, Space/Control for up/down). Right-click to capture mouse (relative mode, cursor hidden); camera movement/rotation only while captured (Godot editor pattern). Runs interactively until Escape is pressed. Uses `device.window().platform().delta_time()` for frame-rate-independent movement.
- `./build/debug/src/cmd/buddd run phong` — opens a window (1024×768), renders a textured cube with Phong lighting from an orbiting PointLightComponent and a static DirectionalLightComponent fill. Interactive free-camera (WASD + mouse). The cube uses PhongMaterial with embedded GLSL shaders, a diffuse texture, and material properties (ambient, specular, shininess). Runs interactively until Escape is pressed.
- `./build/debug/src/cmd/buddd run asset-demo` — opens a window (1024×768), renders a textured cube loaded via the Asset Manager from YAML metadata (`assets/materials/demo_cube.yaml` references `assets/textures/demo_brick.yaml` and `assets/shaders/demo.vert`/`demo.frag`). Runs for 120 frames at ~60 FPS, then exits. Demonstrates the full asset pipeline: YAML parsing, shader program deduplication, texture loading, and material creation via the `AssetManager` API.
- `./build/debug/src/cmd/buddd run cube --capture 120:/tmp/out.png` — renders 120 frames of the cube and captures frame 120 to `/tmp/out.png`. The `--capture` flag can be repeated for multiple captures.
- `./build/debug/src/cmd/buddd run cube --frame 60` — runs exactly 60 frames of the cube, then exits automatically.
- `./build/debug/src/cmd/buddd version` — prints `buddd 0.1.0`
- `./build/debug/src/cmd/buddd help` — prints usage information listing three commands (`run`, `version`, `help`)
- `./build/debug/src/cmd/buddd <unknown>` — prints error to stderr and exits with code 1
- `buddd demo`, `buddd capture`, `buddd test` are **removed** — they produce an unknown command error
- Old `--test` and `--version` flags are removed (produce an unknown command error)
- `ctest --preset debug` — runs tests, all pass
- `cmake --build --preset debug --target format` — formats all C++ sources
- `Platform::create(Backend::Headless)` — creates a headless platform (no display needed, used for testing)
- `Platform::create(Backend::SDL3)` — creates an SDL3-based platform (uses offscreen video driver in tests; requires a display in production)
- Factory methods return `Result<T>` (`std::expected<T, Error>`) for error propagation
- **Exception to ADR-001**: `RenderDevice::draw()` and `draw_indexed()` return `void` rather than `Result<void>`, because draw calls are on a performance-sensitive hot path where per-frame error checking is impractical. Precondition violations are undefined behaviour.
- `RenderDeviceOpenGL::begin_frame()` sets clear colour to `(0.02, 0.02, 0.05)` (dark blue) via `glClearColor` before `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`, rather than the default black. A 24-bit depth buffer is allocated via `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` before context creation, and `GL_DEPTH_TEST` (with `GL_LESS` comparison) is enabled in the constructor, providing correct 3D occlusion. (See SPEC-012.)
- `RenderDeviceOpenGL::read_pixels()` calls `glReadBuffer(GL_BACK)` before `glReadPixels` to ensure the freshly rendered back buffer is read — this must be called before `end_frame()` (before the buffer swap).

## Architecture boundary

A hard architecture boundary is enforced: **no code outside `src/engine/`** may `#include` SDL3, OpenGL, or GLM headers. All access to windowing and graphics functionality goes through the abstract `Platform`, `Window`, and `RenderDevice` interfaces. All access to linear algebra types goes through the math wrapper types (`Vec2`, `Vec3`, `Vec4`, `Mat4`, `Quat`). Concrete backend implementations and math wrappers live entirely within `src/engine/`.

### Navigable object graph

As of SPEC-016 / ADR-012, the engine establishes a navigable object graph:

```
RenderDevice  ──>  Window  ──>  Platform  ──>  InputSystem
```

- `Window` stores a non-owning `Platform&` reference (forward declaration in `window.h`).
- `RenderDevice` exposes a pure virtual `window() -> Window&` accessor, implemented by each backend.
- From any `RenderDevice&`, the entire upstream graph is reachable: `device.window().platform().input_system()`.
- Demo functions no longer receive a separate `Platform&` parameter — they access platform services via the navigable graph.
- `EngineService` owns the entire chain via `std::unique_ptr` with correct destruction ordering. See [ADR-012](/docs/adr/012-navigable-object-graph-engine-service.md).

### GLM boundary

GLM headers are included **only inside `src/engine/math/`** (the wrapper headers) and in the implementation files under `src/engine/` that include those wrappers. Outside `src/engine/math/`, no code may `#include` any GLM header directly. All math operations go through the wrapper types. The `.glm()` accessor on each wrapper type provides the official zero-overhead interop path via `reinterpret_cast` (guaranteed safe by `static_assert` checks for identical layout, size, and standard-layout conformance).

**Narrow exception (AMEND-2026-001):** Test files (`tests/*.cpp`) that are conditionally compiled with `BUDDD_HAS_DISPLAY=ON` may include `<SDL3/SDL.h>` for testing SDL3-dependent engine functionality. This permits SDL3 API calls needed to set up test environments (e.g., `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`), inject synthetic events (e.g., `SDL_PushEvent()`), and exercise SDL3 backends. The exception remains narrow: it applies only to test files that test the SDL3 backend, only to `<SDL3/SDL.h>`, and only under `#ifdef BUDDD_HAS_DISPLAY`. All other platform, graphics, windowing, or math library headers remain prohibited outside `src/engine/`.

## Key behaviors (scene graph)

- Entity creation, hierarchy, and component management are available after including `<scene/entity.h>`.
- Components use `dynamic_cast<T*>()` for type-safe dispatch (requires RTTI, enabled by default).
- `get_component<T>()` returns `std::optional<T&>` (C++26) — a type-safe optional reference.
- `World::each<T>()` iterates all entities having a component of type `T`, passing `(Entity, T&)` to a callback. The callback may return `false` to early-exit the iteration.
- Every `Component` is entity-aware: `World::add_component<T>()` sets `world_` and `entity_id_` on the component, then calls the virtual `on_attach()` hook. Derived components may override `on_attach()` for setup logic (e.g., `CameraComponent` auto-registers with the World).
- Camera registration: `World::register_camera(CameraComponent&)` stores a reference as the active camera (last-registered-wins). `World::unregister_camera(const CameraComponent&)` clears by address comparison. `World::active_camera()` returns `std::optional<CameraComponent&>`. `CameraComponent` auto-registers on attachment and unregisters on destruction.
- Light components (`DirectionalLightComponent`, `PointLightComponent`, `SpotLightComponent`) follow a simpler pattern: they inherit `Component`, override `on_attach()` as a no-op, and do NOT register/unregister with `World`. Their lifecycle is purely managed by the entity. `RenderSystem` collects them each frame via `World::each<T>()`.
- Entity destruction is deferred: `entity.destroy()` marks for destruction; `world.flush_destroyed()` reclaims resources in reverse depth order.
- The scene graph uses per-entity `std::vector<std::unique_ptr<Component>>` storage (no ECS flat arrays in v1). The storage strategy is hidden behind the `World` implementation for forward compatibility.
- Scene graph types are pure memory management and spatial computation — no display, GPU, or external dependencies beyond math wrappers.

## Reference

- Spec: [SPEC-001](/.specs/sprint-2026-05/project-setup/spec.md)
- Implementation contract: [IMPL-001](/.specs/sprint-2026-05/project-setup/implementation-contract.md)
- Spec: [SPEC-002](/.specs/sprint-2026-05/platform-abstraction/spec.md)
- Implementation contract: [IMPL-002](/.specs/sprint-2026-05/platform-abstraction/implementation-contract.md)
- Spec: [SPEC-003](/.specs/sprint-2026-05/sdl3-backend-tests/spec.md)
- Implementation contract: [IMPL-003](/.specs/sprint-2026-05/sdl3-backend-tests/implementation-contract.md)
- Spec: [SPEC-004](/.specs/sprint-2026-05/math-foundations/spec.md) — Math Foundations (Vec2, Vec3, Vec4, Mat4, Quat, Camera)
- Implementation contract: [IMPL-004](/.specs/sprint-2026-05/math-foundations/implementation-contract.md)
- Spec: [SPEC-005](/.specs/sprint-2026-05/render-pipeline/spec.md) — Render Pipeline (Shader, Material, VertexBuffer, IndexBuffer, PrimitiveTopology, CLI modes)
- Implementation contract: [IMPL-005](/.specs/sprint-2026-05/render-pipeline/implementation-contract.md)
- Spec: [SPEC-006](/.specs/sprint-2026-05/cli-command-system/spec.md) — CLI Command System
- Implementation contract: [IMPL-006](/.specs/sprint-2026-05/cli-command-system/implementation-contract.md)
- Spec: [SPEC-007](/.specs/sprint-2026-05/cli-command-evolution/spec.md) — CLI Command Evolution: Demo System & Empty Run
- Implementation contract: [IMPL-007](/.specs/sprint-2026-05/cli-command-evolution/implementation-contract.md)
- Spec: [SPEC-008](/.specs/sprint-2026-05/scene-graph/spec.md) — Scene Graph (World, Entity, Transform, Components, Hierarchy)
- Implementation contract: [IMPL-008](/.specs/sprint-2026-05/scene-graph/implementation-contract.md)
- Spec: [SPEC-009](/.specs/sprint-2026-05/3d-cube-demo/spec.md) — Model Utility & 3D Cube Demo (Model class, CubeResources, cube demo)
- Implementation contract: [IMPL-009](/.specs/sprint-2026-05/3d-cube-demo/implementation-contract.md)
- Spec: [SPEC-010](/.specs/sprint-2026-05/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario)
- Implementation contract: [IMPL-010](/.specs/sprint-2026-05/capture/implementation-contract.md)
- Spec: [SPEC-011](/.specs/sprint-2026-05/scene-rendering/spec.md) — Scene Rendering (Component entity awareness, World::each, CameraComponent, MeshRenderer, RenderSystem, cube-scene demo)
- Implementation contract: [IMPL-011](/.specs/sprint-2026-05/scene-rendering/implementation-contract.md)
- Spec: [SPEC-013](/.specs/sprint-2026-05/input-system/spec.md) — Input System (KeyCode, InputSystem, SDL3/Headless backends, Platform integration)
