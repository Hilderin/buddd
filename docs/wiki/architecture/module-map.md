# Module Map

## Overview

The project is composed of four CMake targets organized into four source directories. Each target has a specific role within the architecture.

## `buddd_engine` — Static library (`src/engine/`)

The engine library is the core of the project. It provides a version API, a math foundations module, and a platform abstraction layer. All source files under `src/engine/` are collected automatically via `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` in `CMakeLists.txt`.

### EngineService (`src/engine/`)

| File | Role |
|---|---|
| `engine_service.h` | Public header: `EngineService` class — owns the `Platform` → `Window` → `RenderDevice` → `AssetManager` chain via `unique_ptr`. Factory: `EngineService::create(Backend, WindowConfig) -> Result<unique_ptr<EngineService>>`. Accessors: `platform() -> Platform&`, `window() -> Window&`, `device() -> RenderDevice&`, `assets() -> AssetManager&`. Member declaration order (`platform_`, `window_`, `device_`, `asset_manager_`) guarantees correct destruction ordering. |
| `engine_service.cpp` | Factory implementation: creates `Platform`, then `Window` (via `platform->create_window()`), then `RenderDevice` (via `RenderDevice::create()`), wrapping them in an `EngineService`. Then creates `AssetManager` via `AssetManager::create(engine_service.device(), "assets")` and stores it in `asset_manager_`. |

See [ADR-012](/docs/adr/012-navigable-object-graph-engine-service.md) for the architectural rationale.

### Version module

| File | Role |
|---|---|
| `version.h` | Public header: declares `buddd::engine::version() -> std::string_view` |
| `version.cpp` | Implementation: returns `"0.1.0"` |

### Error handling module

| File | Role |
|---|---|
| `error.h` | Public header: defines `Error` struct (with `Category` enum: `InitFailed`, `WindowCreationFailed`, `RenderDeviceCreationFailed`, `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`, `ReadbackFailed`, `TextureCreationFailed`, `IoFailed`, `InputInitFailed`, `Unsupported`, `Unknown`), `int code`, `std::string message`, `to_string()`, `make_error()`, and `Result<T>` alias (`std::expected<T, Error>`) |

### Math submodule (`math/`)

All types in namespace `buddd::engine::math`. The math module wraps GLM (`glm`) with zero-overhead C++ wrapper types — header-only (except `Camera`). GLM headers are included only inside `src/engine/math/`.

| File | Role |
|---|---|
| `math.h` | Convenience header: includes all math types, provides `radians()`, `degrees()`, math constants (`pi`, `half_pi`, `two_pi`, `epsilon`), and common math functions (`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sqrt`) |
| `vec2.h` | `Vec2` struct — 2D vector (x, y), wrapper around `glm::vec2`. Header-only. |
| `vec3.h` | `Vec3` struct — 3D vector (x, y, z), wrapper around `glm::vec3`. Header-only. |
| `vec4.h` | `Vec4` struct — 4D vector (x, y, z, w), wrapper around `glm::vec4`. Header-only. |
| `mat4.h` | `Mat4` struct — 4×4 column-major matrix, wrapper around `glm::mat4`. Header-only. |
| `quat.h` | `Quat` struct — quaternion (w, x, y, z), wrapper around `glm::quat`. Header-only. |
| `camera.h` | `Camera` class — perspective camera with position, orientation, and perspective parameters. Computes view, projection, and view-projection matrices. |
| `camera.cpp` | Camera method implementations (only type with a `.cpp` file). Contains GLM includes for implementation only. |

Each wrapper type provides a `.glm()` accessor for zero-overhead GLM interop, guarded by `static_assert(std::is_standard_layout_v<T>)`, `static_assert(sizeof(T) == sizeof(GLMType))`, and `static_assert(std::is_trivially_copyable_v<T>)`.

### Platform submodule (`platform/`)

The Platform abstraction now integrates the InputSystem: each concrete Platform backend owns an embedded InputSystem backend, and `poll_events()` calls `begin_frame()` and routes SDL events to the input system.

| File | Role |
|---|---|
| `platform.h` | Public header: `Backend` enum (`SDL3`, `Headless`), abstract `Platform` class with `create(Backend)` static factory, `virtual auto input_system() -> InputSystem& = 0`, and `virtual auto delta_time() const noexcept -> float = 0` |
| `platform.cpp` | Factory implementation: dispatches to SDL3 or Headless backend based on `Backend` enum |
| `platform_sdl3.h` | Private header: `PlatformSDL3` concrete class (final) with embedded `InputSystemSDL3` member, `delta_time_` member, and `last_frame_ticks_` for frame timing |
| `platform_sdl3.cpp` | SDL3 backend: `SDL_Init`/`SDL_Quit` lifecycle, `SDL_CreateWindow` delegation, `poll_events()` computes delta from `SDL_GetTicks`, calls `begin_frame()`, routes events to `InputSystemSDL3::on_sdl_event()` |
| `platform_headless.h` | Private header: `PlatformHeadless` concrete class (final) with embedded `InputSystemHeadless` member and `delta_time()` override |
| `platform_headless.cpp` | Headless implementation: no SDL3/OpenGL dependency, validates dimensions; `poll_events()` calls `begin_frame()`; `delta_time()` returns fixed 1/60f |

### Window submodule (`window/`)

The `Window` class now stores a non-owning `Platform&` reference, creating a navigable back-link from `Window` to its creating `Platform`. It also exposes mouse capture API. (`window/` now forward-declares `Platform` from `platform/` — see ADR-012.)

| File | Role |
|---|---|
| `window.h` | Public header: `WindowConfig` struct (`title`, `width`, `height`), abstract `Window` class. Stores `Platform& platform_` (protected member, set via new `Window(Platform&)` protected constructor). Provides `platform() -> Platform&`, width/height getters, `native_handle()`, and pure virtual `set_mouse_capture(bool)` / `is_mouse_captured() -> bool`. |
| `window_sdl3.h` | Private header: `WindowSDL3` concrete class wrapping `SDL_Window*`. Implements `set_mouse_capture(bool)` via `SDL_SetWindowRelativeMouseMode` and caches state in `bool captured_`. |
| `window_sdl3.cpp` | SDL3 implementation: `SDL_DestroyWindow` on destruction, `native_handle()` casts to `void*`. |
| `window_headless.h` | Private header: `WindowHeadless` concrete class. Mouse capture is no-op; `is_mouse_captured()` returns `false`. |
| `window_headless.cpp` | Headless implementation: stores width/height, `native_handle()` returns `nullptr`. |

### Scene submodule (`scene/`)

All types in namespace `buddd::engine`. The scene graph module provides a lightweight entity system with hierarchy, transforms, polymorphic component dispatch, and ECS components (`CameraComponent`). It depends on math wrapper types (`Vec3`, `Quat`, `Mat4`, `Camera`) from `src/engine/math/` and standard C++ headers only — no GLM, SDL3, or OpenGL dependencies.

Component dispatch uses `dynamic_cast<T*>()` (RTTI-based) for type-safe retrieval, with zero boilerplate in component types. See ADR-006 for the decision rationale.

| File | Role |
|---|---|
| `entity_id.h` | `EntityId` struct — 8-byte handle (index + generation) for safe entity references. Header-only. |
| `transform.h` | `Transform` struct — position (`Vec3`), rotation (`Quat`), scale (`Vec3`) value type with `local_matrix()` and `world_matrix()`. Header-only. |
| `component.h` | `Component` polymorphic base class — non-copyable, non-movable. Provides entity-awareness via protected `world_`/`entity_id_` members and `entity()` accessor. Virtual `on_attach()` lifecycle hook called by `World::add_component<T>()` after attachment. Header-only. |
| `entity.h` | `Entity` class — 16-byte lightweight handle (`World*` + `EntityId`). Inline template methods for component operations. |
| `entity.cpp` | Entity non-inline method implementations — all delegate to `World`. |
| `world.h` | `World` class — top-level container managing entity lifecycle, tree hierarchy, deferred destruction. Template methods for component dispatch (`add_component`, `get_component`, `remove_component`) and type-based iteration (`each<T>()`) defined inline. Camera registration API (`register_camera`, `unregister_camera`, `active_camera`) stores a `CameraComponent&` reference in an `std::optional<CameraComponent&>` member. |
| `world.cpp` | World implementation including internal `EntityNode` type, slot-based storage, `flush_destroyed()` logic, and `mark_for_destroy()` iterative traversal. |
| `camera_component.h` | `CameraComponent` ECS component class — wraps `math::Camera`, inherits `Component`. Auto-registers with `World` via `on_attach()` and unregisters on destruction (address-based comparison). |
| `camera_component.cpp` | CameraComponent implementation: `on_attach()` calls `world().register_camera(*this)`, destructor calls `world_->unregister_camera(*this)` (with null guard). |
| `directional_light_component.h` | `DirectionalLightComponent` — infinite parallel light. Direction from entity rotation (-Z forward). Properties: `colour` (Vec3), `intensity` (float). `on_attach()` no-op. |
| `directional_light_component.cpp` | Constructor and accessor implementations. |
| `point_light_component.h` | `PointLightComponent` — omni-directional light with position (from entity translation) and range. Properties: `colour`, `intensity`, `range` (float, default 10.0). `on_attach()` no-op. |
| `point_light_component.cpp` | Constructor and accessor implementations. |
| `spot_light_component.h` | `SpotLightComponent` — conical light with position, direction, and cone angles. Properties: `colour`, `intensity`, `range`, `inner_angle` (default 0.785 rad ≈ 45°), `outer_angle` (default 1.047 rad ≈ 60°). `on_attach()` no-op. |
| `spot_light_component.cpp` | Constructor and accessor implementations. |

### Image submodule (`image/`)

All types in namespace `buddd::engine`. Provides pixel buffer representation and PNG I/O via stb_image/stb_image_write. Depends on `error.h` for `Result<T>` types.

| File | Role |
|---|---|
| `image_buffer.h` | `ImageBuffer` aggregate struct — `int width`, `int height`, `int channels`, `std::vector<std::byte> data`. Pure aggregate, no methods. |
| `image.h` | `Image` class — static `create(const ImageBuffer&) -> Result<Image>` (validates, flips rows), static `load(std::string_view) -> Result<Image>` (PNG via stb_image), `save(std::string_view) const -> Result<void>` (PNG via stb_image_write), and accessors. Non-copyable, movable. |
| `image.cpp` | Image implementation: row-flipping logic (bottom-left → top-left), stb_image/stb_image_write implementation via `#define STB_IMAGE_IMPLEMENTATION` / `STB_IMAGE_WRITE_IMPLEMENTATION`. |

### Input submodule (`input/`)

All types in namespace `buddd::engine`. Provides a frame-based input abstraction with double-buffered state for keyboard and mouse. Follows the established pattern: abstract interface (`InputSystem`) + concrete SDL3/Headless backends + static factory. The `KeyCode` enum values match `SDL_Scancode` values — conversion is `static_cast` with a bounds check, no mapping table needed. See SPEC-013 for full specification.

| File | Role |
|---|---|
| `key_code.h` | Public header: `KeyCode` enum (`uint8_t`, values matching SDL_Scancode: A–Z at 4–29, Digit1–Digit0 at 30–39, common keys at 40–57, F1–F12 at 58–69, navigation at 76–93, modifiers at 224–231). Includes `Unknown = 0` and `_Count` sentinel. |
| `input_system.h` | Public header: `MouseButton` enum (`Left`, `Right`, `Middle`, `X1`, `X2`), abstract `InputSystem` class with `create(Backend)` static factory, frame lifecycle (`begin_frame()`), keyboard queries (`is_down`, `is_pressed`, `is_released`), mouse state queries (`mouse_position`, `mouse_delta`, `mouse_wheel`, `is_mouse_down`, `is_mouse_pressed`, `is_mouse_released`). Non-copyable, non-movable. |
| `input_system.cpp` | Factory implementation: `Backend::SDL3` → `InputSystemSDL3`, `Backend::Headless` → `InputSystemHeadless` |
| `input_system_sdl3.h` | Private header: `InputSystemSDL3` concrete class (final) with private `on_sdl_event(const SDL_Event&)` — processes `SDL_EVENT_KEY_DOWN/UP`, `SDL_EVENT_MOUSE_MOTION`, `SDL_EVENT_MOUSE_BUTTON_DOWN/UP`, `SDL_EVENT_MOUSE_WHEEL` |
| `input_system_sdl3.cpp` | SDL3 backend: double-buffered state arrays, `static_cast` scancode conversion with bounds check, mouse delta/wheel accumulation |
| `input_system_headless.h` | Private header: `InputSystemHeadless` concrete class (final). All queries return false/zero defaults. |
| `input_system_headless.cpp` | Headless backend: `begin_frame()` is a no-op, all query methods return zero/false |

### Asset submodule (`asset/`)

All types in namespace `buddd::engine`. Provides a centralised ID-based asset loading, caching, and hot-reload system. Assets are defined by YAML metadata files in the `assets/` directory tree, loaded lazily on first access. Depends on `render/` for GPU resource creation, `image/` for texture loading, and yaml-cpp (PRIVATE dependency) for YAML parsing. See SPEC-019 and ADR-016.

| File | Role |
|---|---|
| `asset.h` | Abstract `Asset` base class — virtual destructor, non-copyable, non-movable, protected default constructor. |
| `asset_id.h` | (Optional) Asset ID utilities and path resolution helpers. |
| `asset_manager.h` | `AssetManager` class — core asset system. Factory `create(RenderDevice&, string_view) -> Result<unique_ptr>`. Template `create<T>(id) -> Result<shared_ptr<T>>` for loading assets. Convenience `create_texture(id)` and `create_material(id)`. `clear()`, `base_path()`, `poll_file_events()`, `set_file_watcher_enabled(bool)`. Owns cache, shader deduplication map, dependency map, and file watcher. |
| `asset_manager.tpp` | Template implementation of `create<T>()` — included at bottom of `asset_manager.h`. |
| `asset_manager.cpp` | Non-template implementation: `load_texture()`, `load_material()`, `poll_file_events()`, hot-reload handlers (`handle_yaml_change`, `handle_source_change` — fully implemented, reload assets in-place with GPU handle swap), explicit template instantiations. |
| `texture_asset.h` | `TextureAsset` final class — wraps `std::shared_ptr<Texture>`. Accessor: `texture() -> const shared_ptr<Texture>&`. |
| `texture_asset.cpp` | TextureAsset implementation. |
| `material_asset.h` | `MaterialAsset` final class — wraps `std::shared_ptr<Material>`. Accessor: `material() -> const shared_ptr<Material>&`. |
| `material_asset.cpp` | MaterialAsset implementation. |
| `model_asset.h` | `ModelAsset` final class — wraps a `ModelNode` hierarchy tree. Loaded via `AssetManager::create<ModelAsset>(id)`. Provides `root_node()` const/mutable accessors. Private `replace_root()` for hot-reload, accessible only to `AssetManager` (friend). See SPEC-NNNN (glTF model loading). |
| `model_asset.cpp` | ModelAsset implementation. |
| `dependency_map.h` | `DependencyMap` class — bidirectional asset↔source file tracking. `add_dependency()`, `get_dependencies()`, `get_dependents()`, `remove_asset()`, `clear()`. |
| `dependency_map.cpp` | DependencyMap implementation (internal `unordered_map<string, vector<string>>` for forward and reverse directions). |
| `file_watcher.h` | Abstract `FileWatcher` base class. `FileEventType` enum (`Created`, `Modified`, `Deleted`). `FileEvent` struct (`path`, `type`). `NullFileWatcher` no-op final class. Factory `create(string_view) -> Result<unique_ptr>`. |
| `file_watcher.cpp` | `FileWatcher::create()` factory — on Linux attempts `InotifyFileWatcher`, on other platforms returns `Unsupported` (caller falls back to `NullFileWatcher`). `~FileWatcher()` destructor (vtable emission). |
| `file_watcher_inotify.h` | `InotifyFileWatcher` concrete class (Linux only, `#ifdef __linux__`). Monitors a directory tree via inotify, runs a dedicated thread, pushes events to a thread-safe queue. |
| `file_watcher_inotify.cpp` | Inotify implementation: `inotify_init1`, `add_watch_recursive()` (walks directory tree recursively adding inotify watches for all subdirectories), blocking `poll()`-based read loop, mutex-protected queue, self-pipe for wake-on-shutdown. |
| `model_loader.h` | Internal detail header: `ModelLoadResult` struct and `load_gltf_model()` free function in `buddd::engine::detail` namespace. Converts tinygltf data to engine `ModelNode` trees. Returns `Result<ModelLoadResult>`. |
| `model_loader.cpp` | ModelLoader implementation: tinygltf integration, vertex conversion (glTF Vertex → engine Vertex with POSITION/COLOR_0/NORMAL/TEXCOORD_0/TANGENT/TEXCOORD_1 mapping), index type detection (Uint16/Uint32), material conversion to `PbrMaterial`, texture loading (embedded via buffer view or external via `Image::load`), hierarchy building, magenta fallback for missing textures. |

### Render submodule (`render/`)

The render submodule now provides a full pipeline abstraction: shader compilation, material linking, vertex/index buffer management, draw calls, and an ECS integration layer bridging the scene graph to the GPU. All abstract types are backend-agnostic; concrete implementations exist for OpenGL 4.5 Core and Headless.

The render submodule depends on the `scene/` submodule: `MeshRenderer` inherits `Component` (from scene/), and `RenderSystem` takes a `World&` (from scene/) to iterate entities each frame.

| File | Role |
|---|---|
| `primitive_topology.h` | Public header: `PrimitiveTopology` enum (`Triangles`, `TriangleStrip`, `Lines`, `LineStrip`, `Points`). Header-only value type. |
| `vertex_format.h` | Public header: `VertexAttributeType` enum (11 types), `VertexAttribute` struct, `VertexFormat` struct. Header-only value types. |
| `vertex.h` | Public header: Standard `Vertex` struct (72B stride, 6 attributes: position loc0, color loc1, normal loc2, texcoord loc3, tangent loc4, texcoord2 loc5) and `k_standard_vertex_format` constant. Used by ALL meshes — unlit demos fill position+color (other fields zero), Phong material uses position+normal+texcoord. |
| `glsl_util.h` | Detail header: declares `extract_uniform_names()` (parses GLSL source for uniform declarations, handling `uniform type name;` / `name[N]` / `= default;` / `layout(...)`) and `normalize_uniform_name()` (strips `[N]` array subscript suffix). Shared by both `RenderDeviceOpenGL` and `RenderDeviceHeadless`. |
| `glsl_util.cpp` | Implementations of both functions. Replaces previously duplicated local `extract_uniform_names()` in `render_device_headless.cpp`. |
| `light_data.h` | Detail header: `LightData` struct with fields `position_or_dir` (Vec4, .w=type), `colour` (Vec4), `range` (float), `spot_direction` (Vec4), `inner_cone_cos` (float), `outer_cone_cos` (float). `k_max_lights = 8`. |
| `shader.h` | Public header: `ShaderType` enum (`Vertex`, `Fragment`), abstract `Shader` class with `type()` pure virtual. Non-copyable, non-movable. |
| `material.h` | Public header: abstract `Material` class with 6 `set_uniform` overloads (`float`, `int32_t`, `bool`, `math::Vec3`, `math::Vec4`, `math::Mat4`), `has_uniform()`, `set_texture(name, shared_ptr<Texture>)`, `has_texture(name)`, and `bind()` (deferred state application: program activation + uniforms + textures). Non-copyable, non-movable. |
| `vertex_buffer.h` | Public header: abstract `VertexBuffer` class with `format()` pure virtual. Non-copyable, non-movable. |
| `index_buffer.h` | Public header: `IndexType` enum (`Uint16`, `Uint32`), abstract `IndexBuffer` class with `type()` pure virtual. Non-copyable, non-movable. |
| `render_device.h` | Public header: abstract `RenderDevice` class with `create(Window&)` static factory, `begin_frame()`, `end_frame()`, `size()`, resource factory methods (`create_shader`, `create_material`, `create_material(shared_ptr<ShaderProgram>)`, `create_vertex_buffer`, `create_index_buffer`, `create_texture(const Image&)`, `read_pixels()`), and draw methods (`draw`, `draw_indexed`). Draw methods return `void` — deliberate exception to ADR-001. `create_texture` returns `Result<std::unique_ptr<Texture>>`. **New**: pure virtual `window() -> Window&` enables navigation to `Window` and via it to `Platform`/`InputSystem`. **New**: virtual diagnostic accessors `frame_begin_count()`, `frame_end_count()`, `draw_call_count()` with default `0` implementations (overridden by headless backend). **New**: `fallback_material()` returns a shared magenta `Material&` used by `Model::draw()` for null/out-of-bounds material references. See ADR-012 and ADR-017. The `create_material(shared_ptr<ShaderProgram>)` overload enables shader program deduplication — materials share the compiled GL program but retain independent uniform/texture state. |
| `render_device.cpp` | Factory implementation: dispatches to OpenGL or Headless backend based on `native_handle()` value. Passes `Window&` to both backend constructors. |
| `render_device_opengl.h` | Private header: `RenderDeviceOpenGL` concrete class. Stores `Window& window_` (for the `window()` accessor) and `SDL_Window* sdl_window_` for internal SDL calls, plus `SDL_GLContext`. |
| `render_device_opengl.cpp` | OpenGL 4.5 Core implementation: GLSL compilation via `glCreateShader`/`glCompileShader`, program linking via `glCreateProgram`/`glLinkProgram`, VAO/VBO/IBO management via DSA APIs (`glCreateVertexArrays`, `glNamedBufferStorage`, etc.), texture creation via DSA (`glCreateTextures`/`glTextureStorage2D`/`glTextureSubImage2D`), and draw dispatch via `glDrawArrays`/`glDrawElements`. `draw()`/`draw_indexed()` call `material.bind()` before draw to apply deferred state. |
| `render_device_headless.h` | Private header: `RenderDeviceHeadless` concrete class. Stores `Window& window_` (replaces `int width_, height_`). `size()` delegates to `window_.width()`/`window_.height()`. Overrides diagnostic counters (`frame_begin_count()`, `frame_end_count()`, `draw_call_count()`). Unconditional `read_pixels()` error. |
| `render_device_headless.cpp` | Headless implementation: stores shader source and vertex data in memory; simulates compilation errors via `#error` marker and linking errors via vertex/fragment I/O mismatch detection; draw calls are no-ops; increments `frame_begin_count_`/`frame_end_count_` in `begin_frame()`/`end_frame_`() |
| `shader_opengl.h` | Private header: `ShaderOpenGL` concrete class wrapping a `GLuint` shader handle |
| `shader_opengl.cpp` | OpenGL shader backend: resource lifetime managed via `glCreateShader`/`glDeleteShader` |
| `shader_headless.h` | Private header: `ShaderHeadless` concrete class storing type and GLSL source string |
| `shader_headless.cpp` | Headless shader backend: stores source for linking-error simulation and uniform discovery |
| `shader_program.h` | Abstract `ShaderProgram` base class — wraps a compiled shader program handle (`uint32_t`). Pure virtual `handle()`, `is_valid()`, `replace_handle()`, `release_handle()`. Non-copyable, non-movable. CONST-001 compliant (uses `uint32_t` not `GLuint` in the base). |
| `shader_program.cpp` | Vtable emission + default virtual implementations (`testing_handle()`, `vs_source()`, `fs_source()`). |
| `shader_program_opengl.h` | `ShaderProgramOpenGL` concrete class — wraps a `GLuint` program handle. `replace_handle` calls `glDeleteProgram` before assigning. `release_handle()` extracts the handle to prevent double-deletion. |
| `shader_program_opengl.cpp` | OpenGL implementation: `glCreateProgram`, `glAttachShader`, `glLinkProgram`, `glDeleteProgram`. |
| `shader_program_headless.h` | `ShaderProgramHeadless` concrete class — stores generation counter and source strings. `handle()` always returns 0. `replace_handle` is no-op. |
| `shader_program_headless.cpp` | Headless implementation: simulated linking via vertex/fragment I/O matching, generation counter for `testing_handle()`. |
| `material_opengl.h` | Private header: `MaterialOpenGL` concrete class with deferred uniform caching (`std::unordered_map<std::string, std::variant<...>> uniform_cache_`), texture map (`std::unordered_map<std::string, std::shared_ptr<Texture>> texture_map_`), `glGetUniformLocation`-based location caching, and a mutable texture unit counter (`mutable int next_unit_{0}`) for automatic unit assignment during `bind()`. |
| `material_opengl.cpp` | OpenGL material backend: `set_uniform` caches values in `uniform_cache_` (no immediate GL calls). `set_texture` stores the shared texture pointer by name. `bind()` applies deferred state: calls `glUseProgram`, then flushes all cached uniforms via `glUniform1f`/`glUniform1i`/`glUniform3fv`/`glUniform4fv`/`glUniformMatrix4fv`, then binds textures to sequential units via `glActiveTexture`/`glBindTexture` and sets `glUniform1i` for each sampler uniform. Program destruction via `glDeleteProgram`. |
| `material_headless.h` | Private header: `MaterialHeadless` concrete class with `std::unordered_set` of known uniform names, `std::variant`-based uniform value storage, and diagnostic accessors: `get_uniform_mat4(name)`, `get_uniform_vec3(name)`, `get_uniform_vec4(name)`, `get_uniform_float(name)`, `get_uniform_int(name)`. Stores texture map (`std::unordered_map<std::string, std::shared_ptr<Texture>>`) for `set_texture`/`has_texture`. |
| `material_headless.cpp` | Headless material backend: in-memory uniform state tracking; `has_uniform` checks known names + previously-set names; `set_uniform` returns `UniformNotFound` for unknown names; diagnostic getters return stored value or `std::nullopt`; all methods use `normalize_uniform_name()` to resolve array subscript brackets; `set_texture` and `has_texture` operate on in-memory texture map; `bind()` is a no-op. |
| `texture.h` | Public header: abstract `Texture` class with pure virtual `width()`, `height()`, `channels()`. Non-copyable, non-movable. |
| `texture_opengl.h` | Private header: `TextureOpenGL` final class wrapping a `GLuint` texture handle. Provides `handle()` accessor. |
| `texture_opengl.cpp` | OpenGL texture backend: DSA-based GPU upload via `glCreateTextures`/`glTextureStorage2D`/`glTextureSubImage2D`; sets `GL_TEXTURE_MIN_FILTER`/`GL_MAG_FILTER` to `GL_LINEAR` and `GL_TEXTURE_WRAP_S`/`GL_TEXTURE_WRAP_T` to `GL_CLAMP_TO_EDGE`. Resource lifetime via `glDeleteTextures` in destructor. Returns `TextureCreationFailed` on zero-dimension or null data. |
| `texture_headless.h` | Private header: `TextureHeadless` final class storing width/height/channels and raw pixel data (`std::vector<std::byte>`). Provides `data()` accessor for diagnostic query. |
| `texture_headless.cpp` | Headless texture backend: in-memory pixel storage; validates dimensions and data size in constructor.
| `vertex_buffer_opengl.h` | Private header: `VertexBufferOpenGL` wrapping VAO and VBO handles |
| `vertex_buffer_opengl.cpp` | OpenGL vertex buffer backend: VAO/VBO creation via `glCreateVertexArrays`/`glCreateBuffers`, attribute configuration via `glVertexArrayAttribFormat`, `glVertexArrayVertexBuffer`, `glVertexArrayAttribBinding` |
| `vertex_buffer_headless.h` | Private header: `VertexBufferHeadless` storing format and vertex data in memory |
| `vertex_buffer_headless.cpp` | Headless vertex buffer backend: data stored in `std::vector<std::byte>` |
| `index_buffer_opengl.h` | Private header: `IndexBufferOpenGL` wrapping IBO handle |
| `index_buffer_opengl.cpp` | OpenGL index buffer backend: buffer creation via `glCreateBuffers`/`glNamedBufferStorage` |
| `index_buffer_headless.h` | Private header: `IndexBufferHeadless` storing type and index data in memory |
| `index_buffer_headless.cpp` | Headless index buffer backend: data stored in `std::vector<std::byte>` |
| `model.h` | Public header: `SubMesh` struct (`{index_start, index_count, material_index}`) and `Model` concrete class — bundles `VertexBuffer` + `IndexBuffer` + `std::vector<SubMesh>` + `std::vector<std::shared_ptr<Material>>`. Single factory method: `create_indexed()` (takes `vector<SubMesh>` + `vector<shared_ptr<Material>>` upfront). `draw()` issues one `draw_indexed` call per SubMesh, binding the correct material by index (or fallback). Accessors: `submeshes()`, `materials()`, `vertices()`, `indices()`, `vertex_count()`, `index_count()`. No `material()`, `has_indices()`, or `create()` (non-indexed). Non-copyable, movable, default-constructible (null model). See [ADR-017](/docs/adr/ADR-017-multi-material-model.md). |
| `model.cpp` | Implementation of `Model::create_indexed()` factory (argument validation, buffer creation, RAII cleanup) and `Model::draw()` (iterates submeshes, binds material or fallback, issues indexed draw call per submesh). Single-path — all Models are indexed. |
| `model_node.h` | Public header: `ModelNode` struct — a node in the glTF model hierarchy. Fields: `name`, `translation` (Vec3), `rotation` (Quat), `scale` (Vec3), `std::optional<Model> model` (absent if node has no mesh), `std::vector<ModelNode> children`. Move-only, publicly constructible. Defined in `render/` because it holds `std::optional<Model>`. See glTF model loading spec. |
| `model_utils.h` | Public inline header: `add_model_to_world()` free function traverses a `ModelNode` tree depth-first and creates ECS entities with `Transform` + `MeshRenderer` for each mesh node. Uses `World`/`Entity` (engine's custom ECS). All implementation is inline. See glTF model loading spec. |
| `primitives.h` | Public header: Geometry-only factory functions in `buddd::engine` namespace — `create_cube(device, material)`, `create_triangle(device, material)`, `create_quad(device, material)`. Each returns a `Result<Model>` with one SubMesh covering all indices and the caller-provided material. 24-byte stride (Float3 position + Float3 colour). See [ADR-017](/docs/adr/ADR-017-multi-material-model.md). |
| `primitives.cpp` | Implementation of primitive helpers: inline vertex/index data definitions, delegates to `Model::create_indexed()` for buffer creation. |
| `mesh_renderer.h` | Public header: `MeshRenderer` ECS component — inherits `Component`, holds a `std::shared_ptr<Model>`. Provides `model()` accessor. Used by `RenderSystem` to discover drawable entities via `World::each<MeshRenderer>()`. |
| `mesh_renderer.cpp` | MeshRenderer implementation: constructor stores the shared Model pointer. |
| `render_system.h` | Public header: `RenderSystem` engine-level class — bridges `RenderDevice` and `World`. Constructor takes `RenderDevice&` and `World&`. Single `render()` method: calls `begin_frame()`/`end_frame()`, queries `active_camera()` for view-projection, iterates `World::each<MeshRenderer>()` to issue draw calls. |
| `render_system.cpp` | RenderSystem implementation: `render()` orchestrates one frame — begin/end frame lifecycle, camera lookup, MVP computation per MeshRenderer entity, uniform setting, and draw dispatch. **Extended for Phong lighting**: before MeshRenderer iteration, collects all `DirectionalLightComponent`, `PointLightComponent`, and `SpotLightComponent` entities into a `LightData` array (max 8). For each MeshRenderer, checks `has_uniform("u_model")` sentinel: if true, sets all lighting uniforms (u_model, u_normal_mat, u_camera_pos, flat array light uniforms, material defaults). If false (unlit material), only sets u_mvp (backward compat). Logs warnings to `std::cerr` for missing camera or uniform failures (per-entity skip). |

### Phong submodule (`render/phong/`)

| File | Role |
|---|---|
| `phong_shaders.h` | Detail header: `constexpr std::string_view` constants containing embedded GLSL 450 core vertex and fragment shader source for the Phong reflection model. Vertex shader: inputs `a_position` (loc 0), `a_color` (loc 1, unused), `a_normal` (loc 2), `a_texcoord` (loc 3); uniforms `u_mvp`, `u_model`, `u_normal_mat`. Fragment shader: `#define MAX_LIGHTS 8`, flat array uniforms for light data (`u_light_positions_or_dir`, `u_light_colours`, `u_light_ranges`, `u_light_spot_directions`, `u_light_inner_cones`, `u_light_outer_cones`), material uniforms (`u_material_ambient`, `u_material_specular`, `u_material_shininess`, `u_material_diffuse_tint`), sampler `u_diffuse_texture.` Implements Blinn-Phong specular, Lambertian diffuse, squared-distance attenuation, and spotlight cone falloff via `spot_cone_attenuation()`. |
| `phong_material.h` | `PhongMaterial` — self-contained `Material` subclass. Creates its own vertex+fragment shaders from embedded GLSL strings (no external shader creation required). Declares all standard Phong uniforms in its known_uniforms list (17 uniforms). Provides convenience setters: `set_camera_position(Vec3)`, `set_lights(const LightData*, int)`, `set_transforms(Mat4 model, Mat4 view_projection)`. Uses PIMPL (`std::unique_ptr<Impl>`). Non-copyable, non-movable. |
| `phong_material.cpp` | Implementation: constructor creates shaders from `phong_shaders.h` constants, creates inner `Material` via `device.create_material()`, delegates all `set_uniform` calls to inner material. `set_lights()` iterates up to `k_max_lights` and sets bracket-syntax array uniforms per light index. `set_transforms()` sets `u_model`, `u_mvp`, and `u_normal_mat` (= `model.inverse().transpose()`). `known_uniform_names()` returns a static vector of all 17 standard Phong uniform names. |

### PBR submodule (`render/pbr/`)

| File | Role |
|---|---|
| `pbr_shaders.h` | Detail header: `constexpr std::string_view` constants containing embedded GLSL 450 core vertex and fragment shader source for the glTF 2.0 metallic-roughness PBR model. Vertex shader: inputs `a_position` (loc 0), `a_color` (loc 1), `a_normal` (loc 2), `a_texcoord` (loc 3), `a_tangent` (loc 4), `a_texcoord2` (loc 5); uniforms `u_mvp`, `u_model`, `u_normal_mat`. Fragment shader: implements Cook-Torrance BRDF with Lambertian diffuse, uniforms `u_base_color_factor`, `u_metallic_factor`, `u_roughness_factor`, `u_emissive_factor`, 5 texture samplers, per-texture has flags, and lighting uniforms matching the Phong convention (`u_camera_pos`, `u_light_count`, flat arrays). |
| `pbr_material.h` | `PbrMaterial` — self-contained `Material` subclass following the `PhongMaterial` pattern. Creates its own vertex+fragment shaders from embedded GLSL strings. `PbrMaterialData` struct holds material factors (`base_color_factor`, `metallic_factor`, `roughness_factor`, `emissive_factor`, `double_sided`) and five texture slots (`base_color_texture`, `metallic_roughness_texture`, `normal_texture`, `occlusion_texture`, `emissive_texture`). `set_data()` applies all factors and textures in one call. Uses PIMPL (`std::unique_ptr<Impl>`). Non-copyable, non-movable. |
| `pbr_material.cpp` | Implementation: constructor creates shaders from `pbr_shaders.h` constants, creates inner `Material` via `device.create_material()`, delegates all `set_uniform`/`set_texture`/`bind` calls to inner material. `set_data()` populates all PBR uniforms and texture bindings from a single `PbrMaterialData` struct. Known uniforms include all PBR parameter names, texture has-flags, and lighting uniforms. |

The library exposes a PUBLIC include directory of `${CMAKE_CURRENT_SOURCE_DIR}` (i.e., `src/engine/`), allowing consumers to `#include "error.h"`, `#include "platform/platform.h"`, etc.

## `buddd` — CLI executable (`src/cmd/`)

The command-line binary. Links `buddd_engine` as PRIVATE.

Uses an `App` lifecycle pattern: a virtual `App` base class (`src/cmd/app.h`) defines `config()` / `setup()` / `render()` / `shutdown()`, and a centralised `run_app()` free function owns the render loop. Scene implementations are `App` subclasses in `src/cmd/apps/`. The CLI dispatches only three commands (`run`, `version`, `help`) — see [ADR-014](/docs/adr/014-cli-app-system.md) for the architectural rationale.

### Build system

`src/cmd/CMakeLists.txt` uses `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)` covering `src/cmd/*.cpp` (for `main.cpp` and app files), `src/cmd/commands/*.cpp` (for command files), `src/cmd/demo/*.cpp` (for demo helpers), and `src/cmd/apps/*.cpp` (for App subclasses). New scenes can be added by creating files in `src/cmd/apps/` and adding a dispatch branch in `main.cpp` — no CMakeLists.txt change needed.

### File structure

| File | Role |
|---|---|
| `main.cpp` | Dispatcher: parse first positional argument, dispatch to matching handler. If no arg or arg is `run`: parse `<scene>`, create the appropriate `App` subclass, call `run_app()`. Also handles `version` and `help` commands. No engine header includes beyond those needed for forward declarations. |
| `app.h` | Declares `AppConfig` struct (title, width, height), `App` base class with virtual lifecycle (`config()`, `setup()`, `on_frame_begin()` (default no-op), `render()`, `shutdown()`), and `run_app()` free function. |
| `app.cpp` | Implementation of `run_app()`: creates `Platform` / `Window` / `RenderDevice`, calls `app.setup()`, runs the central render loop (calling `begin_frame()` → `app.on_frame_begin()` → `app.render()` → capture injection → `end_frame()` with frame limiting via `--frame` and capture injection via `--capture`), then calls `app.shutdown()`. |
| `app_config.h` | Declares `CaptureSpec` struct (frame number + path) and `RunningArgs` struct (frame limit + capture specs), plus `parse_running_args()` to parse `--frame N` and `--capture N:path` from argv. |
| `app_config.cpp` | Implementation of `parse_running_args()`. |

### Command files (`src/cmd/commands/`)

| File | Role |
|---|---|
| `version_command.h` / `version_command.cpp` | `buddd::cmd::VersionCommand` — prints `buddd <version>` from `be::version()` to stdout and exits 0. Extra args silently ignored. |
| `help_command.h` / `help_command.cpp` | `buddd::cmd::HelpCommand` — prints usage text to stdout and exits 0. Also defines `k_usage_text` constant used by the unknown-command handler in `main.cpp`. Extra args silently ignored. |

### App subclasses (`src/cmd/apps/`)

Each scene is an `App` subclass whose `render()` method contains **only** the per-frame rendering logic (no `begin_frame()` / `end_frame()` — these are owned by `run_app()`). The `config()` method returns the window configuration; `setup()` performs one-time initialisation; `shutdown()` handles cleanup. Scene state (cameras, render systems, animation timers) lives as member variables.

| File | Role |
|---|---|
| `run_app.h` / `run_app.cpp` | `RunApp` — empty window, clears framebuffer each frame (no draw calls), runs interactively until window close. |
| `triangle_app.h` / `triangle_app.cpp` | `TriangleApp` — 120-frame coloured triangle demo using `engine::create_triangle()`. |
| `cube_app.h` / `cube_app.cpp` | `CubeApp` — 120-frame rotating per-face-coloured cube (Camera + MVP). Uses `engine::create_cube()`. |
| `cube_scene_app.h` / `cube_scene_app.cpp` | `CubeSceneApp` — 120-frame rotating cube using `World` + `RenderSystem` (ECS approach). |
| `textured_cube_app.h` / `textured_cube_app.cpp` | `TexturedCubeApp` — 120-frame rotating UV-mapped cube with brick texture using scene graph. |
| `free_camera_app.h` / `free_camera_app.cpp` | `FreeCameraApp` — interactive fly-through camera (WASD + mouse look + Space/Control). Uses `Platform::delta_time()` for frame-rate-independent movement. Exit via Escape key. Uses `PhongMaterial` with orbiting point light + directional fill. |
| `phong_app.h` / `phong_app.cpp` | `PhongApp` — interactive Phong lighting demo. Textured cubes with `PhongMaterial`, orbiting `PointLightComponent`, static `DirectionalLightComponent` fill. Interactive free-camera (WASD + mouse, right-click to capture). Runs until Escape. Uses ECS: World + RenderSystem + light components + MeshRenderer + PhongMaterial. |
| `hot_reload_app.h` / `hot_reload_app.cpp` | `HotReloadApp` — hot-reload verification test. Loads a material from YAML, swaps texture at frame 30 via `poll_file_events()`. Use with `--capture 30:before.png --capture 60:after.png` to verify before/after. Overrides `on_frame_begin()` to call `asset_manager_->poll_file_events()`. |
| `multi_material_app.h` / `multi_material_app.cpp` | `MultiMaterialApp` — 120-frame multi-material demo. Creates a cube with 3 submeshes (red/green/blue face pairs) using `Model::create_indexed()` directly. Each submesh references a different material index. Demonstrates multi-material draw call batching. |
| `gltf_demo_app.h` / `gltf_demo_app.cpp` | `GltfDemoApp` — loads a glTF model (Box or DamagedHelmet) from YAML via `AssetManager::create<ModelAsset>()`, traverses the `ModelNode` tree using `add_model_to_world()`, and renders with orbit camera and PBR materials. Continuous Y rotation. See glTF model loading spec. |
| `hot_reload_gltf_app.h` / `hot_reload_gltf_app.cpp` | `HotReloadGltfApp` — hot-reload verification for glTF models. Loads a model, swaps source file at frame N, triggers `poll_file_events()`, validates the model updates in-place. Uses same camera system as `gltf_demo_app`. Extends the hot-reload pattern from `HotReloadApp`. |

### Demo helpers (`src/cmd/demo/`)

The `demo_helpers.*` files are now **empty placeholders**. All helper functions (`setup_cube()`, `setup_triangle()`) and types (`CubeResources`) have been removed. Use `engine::create_cube()`, `engine::create_triangle()`, and `engine::create_quad()` from `src/engine/render/primitives.h` instead. See [ADR-017](/docs/adr/ADR-017-multi-material-model.md).

| File | Role |
|---|---|
| `demo_helpers.h` / `demo_helpers.cpp` | Intentionally empty. All code migrated to engine primitives. |

### Subcommand behavior

- `buddd` (no arguments) or `buddd run` → opens 1024×768 window, empties framebuffer each frame (no draw calls), runs until user closes window
- `buddd run <scene> [--frame N] [--capture N:path]...` → runs the named scene. Available scenes: `triangle` (120 frames, coloured triangle), `cube` (120 frames, rotating coloured cube), `cube-scene` (120 frames, ECS-based cube), `textured-cube` (120 frames, UV-mapped cube with brick texture), `free-camera` (interactive, WASD + mouse look + Space/Control), `phong` (interactive, Phong lighting with orbiting point light + directional fill), `hot-reload` (60 frames, hot-reload verification, swaps texture at frame 30), `multi-material` (120 frames, cube with red/green/blue submeshes), `gltf-demo` (interactive, loads Box/DamagedHelmet glTF model with PBR materials and orbit camera), `hot-reload-gltf` (hot-reload verification for glTF models, swaps model file at frame N). `--frame N` limits rendering to N frames. `--capture N:path` captures frame N to a PNG file (repeatable for multiple captures). If no scene is given, defaults to `RunApp` (empty window). If scene is unknown, prints error to stderr and exits 1. Extra unexpected positional arguments print a warning on stderr.
- `buddd version` → prints `buddd 0.1.0` to stdout
- `buddd help` → prints usage information listing three commands (`run`, `version`, `help`)
- Unknown command → prints `"Unknown command: '<cmd>'"` followed by usage to stderr, exits with code 1
- `buddd test` is **removed** — produces an unknown command error
- Old `--test` and `--version` flags are **dropped** — produce an unknown command error

**Note**: Old `demo` and `capture` subcommands are permanently removed per [ADR-014](/docs/adr/014-cli-app-system.md). Use `buddd run <scene>` with `--capture` instead.

## `buddd_editor` — INTERFACE library placeholder (`src/editor/`)

A placeholder for the future editor application. Currently defines an INTERFACE library target with no sources, no dependencies, and no include directories. No binary is produced.

## `buddd_tests` — Test executable (`tests/`)

The unit test binary. Links `buddd_engine` (PRIVATE) and `Catch2::Catch2WithMain` (PRIVATE). Catch2 provides its own `main()` entry point.

| File | Role |
| |---|---|---|
| `version_tests.cpp` | Single Catch2 test: `"engine version is non-empty"` tagged `[sanity]` |
| `cmd_tests.cpp` | CLI command integration tests (tagged `[cli]`): argument parsing, error handling, default command, capture CLI tests — uses shared helpers from `test_helpers.h` |
| `demo_tests.cpp` | **No `[cli][demo]` subprocess tests** — removed as part of SPEC-016. File retained for future non-subprocess demo tests. Demo correctness verified via compilation and EngineService creation tests. |
| `platform_abstraction_tests.cpp` | Headless platform tests (T-01 through T-12), always compiled |
| `sdl3_backend_tests.cpp` | SDL3 backend tests (conditionally compiled with `BUDDD_HAS_DISPLAY=ON`) |
| `math_tests.cpp` | Math foundations tests (T-01 through T-71): Vec2, Vec3, Vec4, Mat4, Quat, Camera, utilities, interop, and edge cases |
| `scene_graph_tests.cpp` | Scene graph tests (T-01 through T-49): EntityId, Transform, Component, Entity, World, hierarchy, deferred destruction, pending-destroy contract, and edge cases — all headless, compiled in both BUDDD_HAS_DISPLAY branches |
| `model_tests.cpp` | Model and cube tests (24 test cases: T-01 through T-24): Model factory methods, accessors, draw dispatch, move semantics, null model safety, cube data verification, shared material ownership, and demo loop simulation — all headless, compiled in both BUDDD_HAS_DISPLAY branches. Uses `EngineService::create()` instead of direct `RenderDeviceHeadless` construction. |
| `model_asset_tests.cpp` | Model asset tests (21+ test cases covering AC-005 through AC-028): ModelAsset loading via AssetManager, ModelNode hierarchy verification, PbrMaterial creation with textures, error cases (missing POSITION, corrupt glTF, missing file, type mismatch, unsupported version), vertex scale, missing texture fallback, Uint32 indices, node without mesh, unsupported primitive mode, COL0R_0 VEC3 expansion, normal default, hot-reload simulation, `create_model()` convenience, `replace_root()` privacy. All headless. |
| `image_tests.cpp` | Image unit tests (tagged `[image]`): ImageBuffer aggregate, Image::create validation, row-flipping, save/load round-trip, load error cases, copy/move semantics, accessors, save error cases. All headless (CPU-only). |
| `input_tests.cpp` | Input system tests: 9 headless tests (factory, headless defaults, double-buffered state model, KeyCode round-trip, edge cases) + 8 SDL3 tests (event processing integration, keyboard, mouse, wheel, accumulation, frame reset) — SDL3 tests conditional on `BUDDD_HAS_DISPLAY`. |
| `render_device_tests.cpp` | Render device tests: uses `EngineService::create()` instead of constructing `RenderDeviceHeadless` directly. Tests headless read_pixels error, navigable graph access (device.window().platform()), and diagnostic counters. |
| `scene_rendering_tests.cpp` | Scene rendering tests (AC-001 through AC-030): Component entity awareness, World::each<T>() iteration, camera registration lifecycle, CameraComponent auto-register/unregister, RenderSystem begin/end_frame, draw call counting, MVP computation, no-camera warning, uniform failure skip, cube-scene demo integration — all headless, compiled in both BUDDD_HAS_DISPLAY branches. Uses `EngineService::create()` for headless engine setup. |
| `texture_tests.cpp` | Texture unit tests (13 headless cases): Texture factory via `create_texture`, `set_texture`/`has_texture` on material, `bind()` deferred state application, headless texture data access, edge cases (null texture, zero dimensions, unknown texture name, empty data, unique_ptr→shared_ptr conversion), material with multiple textures. One OpenGL-only test (T-12) for `create_texture` — conditional on `BUDDD_HAS_DISPLAY`. |
| `lighting_tests.cpp` | Phong lighting tests (32 test cases, tagged `[lighting]`): Vertex struct layout, all three light component construction/accessors/on_attach, PhongMaterial creation and known uniforms, `glsl_util` extract/normalize, LightData struct, RenderSystem light collection (directional/point/spot), 8-light cap, colour*intensity premultiply, normal matrix, backward compat, camera pos, material defaults, component destruction, zero-lights ambient, MaterialHeadless array subscript normalization and diagnostic accessors, spot cone uniforms, layout qualifier parsing — all headless. |
| `test_helpers.h` | Shared CLI test utilities: `buddd_binary_path()`, `temp_filename()`, `run_buddd()`, `CommandResult` |

## API conventions

- **No raw pointers in public API signatures**: Parameters and return types in public headers must avoid `T*` — prefer `T&` (guaranteed non-null), `std::optional<T&>` (nullable), `std::reference_wrapper<T>` (stored reference), or `std::span<T>` (contiguous ranges). Exceptions: `const char*` for C string literal interop, legacy C interop in the platform layer, strictly private implementation details, and `void*` callback contexts at C API boundaries. See ADR-010 for full rationale and replacement mappings.

## Source naming conventions

- Source files: `snake_case` (e.g., `version.h`, `main.cpp`, `version_tests.cpp`)
- Directories: `snake_case` (e.g., `src/engine/`, `src/cmd/`, `tests/`)
- CMake target names: `snake_case` (e.g., `buddd_engine`, `buddd_tests`)
- Test case names: sentence case (e.g., `"engine version is non-empty"`)
- Test files: plural `_tests.cpp` suffix (e.g., `cmd_tests.cpp`, `math_tests.cpp`) — per ADR-009. The GLOB pattern `*_tests.cpp` in `tests/CMakeLists.txt` enforces this convention. New test files must use the `_tests.cpp` suffix or they will be silently excluded from the build.

## Reference

- Spec: [SPEC-001](/.specs/sprint-2026-05/project-setup/spec.md) — Goals, Conventions, Directory structure
- Implementation contract: [IMPL-001](/.specs/sprint-2026-05/project-setup/implementation-contract.md) — sections 3-10 (individual target specifications)
- Spec: [SPEC-002](/.specs/sprint-2026-05/platform-abstraction/spec.md) — Platform, Window, RenderDevice module definitions
- Implementation contract: [IMPL-002](/.specs/sprint-2026-05/platform-abstraction/implementation-contract.md) — File directory structure, Existing conventions to follow
- Spec: [SPEC-004](/.specs/sprint-2026-05/math-foundations/spec.md) — Math type specifications, memory layout, operations, GLM integration
- Implementation contract: [IMPL-004](/.specs/sprint-2026-05/math-foundations/implementation-contract.md) — File list, header structure, delegation pattern
- Spec: [SPEC-005](/.specs/sprint-2026-05/render-pipeline/spec.md) — Shader, Material, VertexBuffer, IndexBuffer, PrimitiveTopology, CLI modes
- Implementation contract: [IMPL-005](/.specs/sprint-2026-05/render-pipeline/implementation-contract.md) — File directory structure, open questions, draw-methods-as-void exception
- Spec: [SPEC-006](/.specs/sprint-2026-05/cli-command-system/spec.md) — CLI Command System: Command pattern, subcommand structure, file layout
- Implementation contract: [IMPL-006](/.specs/sprint-2026-05/cli-command-system/implementation-contract.md) — File list, dispatch logic, CMake glob, CONST-001 compliance
- Spec: [SPEC-007](/.specs/sprint-2026-05/cli-command-evolution/spec.md) — CLI Command Evolution: Demo System & Empty Run
- Implementation contract: [IMPL-007](/.specs/sprint-2026-05/cli-command-evolution/implementation-contract.md) — Replacement of TestCommand with DemoCommand, per-demo files, RunCommand simplification
- Spec: [SPEC-008](/.specs/sprint-2026-05/scene-graph/spec.md) — Scene Graph (World, Entity, Transform, Components, Hierarchy)
- Implementation contract: [IMPL-008](/.specs/sprint-2026-05/scene-graph/implementation-contract.md) — Files allowed to create/modify, entity node structure, template method inline conventions, noexcept specification table, test requirements (T-01 through T-49)
- Spec: [SPEC-009](/.specs/sprint-2026-05/3d-cube-demo/spec.md) — Model Utility & 3D Cube Demo (Model class, CubeResources, cube demo)
- Implementation contract: [IMPL-009](/.specs/sprint-2026-05/3d-cube-demo/implementation-contract.md) — Files allowed to create/modify, factory method signatures, test requirements (T-01 through T-24), draw-methods-as-void exception extension
- Spec: [SPEC-010](/.specs/sprint-2026-05/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario)
- Implementation contract: [IMPL-010](/.specs/sprint-2026-05/capture/implementation-contract.md)
- Spec: [SPEC-011](/.specs/sprint-2026-05/scene-rendering/spec.md) — Scene Rendering (Component entity awareness, World::each, CameraComponent, MeshRenderer, RenderSystem, cube-scene demo)
- Implementation contract: [IMPL-011](/.specs/sprint-2026-05/scene-rendering/implementation-contract.md)
- Spec: [SPEC-013](/.specs/sprint-2026-05/input-system/spec.md) — Input System (KeyCode, InputSystem, SDL3/Headless backends, Platform integration)
- Spec: [SPEC-016](/.specs/sprint-2026-05/architecture-refactor-device-window-platform/spec.md) — Architecture Refactor: Navigable Object Graph (RenderDevice → Window → Platform → InputSystem)
- Spec: [SPEC-020](/.specs/sprint-2026-06/model-multi-material/spec.md) — Multi-material Model (SubMesh, unified create_indexed factory, primitives, fallback material)
- Implementation contract: [IMPL-020](/.specs/sprint-2026-06/model-multi-material/implementation-contract.md) — Multi-Material Model, Primitive Helpers & API Cleanup
- Spec: [SPEC-NNNN](/.specs/sprint-2026-06/gltf-model-loading/spec.md) — glTF Model Loading (ModelAsset, ModelNode, PbrMaterial, PbrMaterialData, ModelLoader)
- ADR: [ADR-018](/docs/adr/018-tinygltf-dependency.md) — tinygltf dependency for glTF 2.0 model loading
- ADR: [ADR-012](/docs/adr/012-navigable-object-graph-engine-service.md) — Navigable Object Graph, EngineService, and Abstract Interface Extensions
- ADR: [ADR-014](/docs/adr/014-cli-app-system.md) — CLI App System: centralised render loop with App lifecycle, unified `run` command (partially supersedes ADR-004)
