# Module Map

## Overview

The project is composed of four CMake targets organized into four source directories. Each target has a specific role within the architecture.

## `buddd_engine` — Static library (`src/engine/`)

The engine library is the core of the project. It provides a version API, a math foundations module, and a platform abstraction layer. All source files under `src/engine/` are collected automatically via `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` in `CMakeLists.txt`.

### EngineService (`src/engine/`)

| File | Role |
|---|---|---|
| `platform.h` | Public header: `Backend` enum (`SDL3`, `Headless`), abstract `Platform` class with `create(Backend)` static factory, `virtual auto input_system() -> InputSystem& = 0`, and `virtual auto delta_time() const noexcept -> float = 0`. **F-01**: Added `auto set_on_close_request(std::function<bool()>) -> void` concrete method and `std::function<bool()> close_request_callback_` protected member. Allows Editor to intercept OS close requests (X button / Alt+F4) and show save-prompt before exiting. **F-01 (SDL3 native dialogs)**: Added `#include <optional>`, `FileDialogCallback` type alias (`std::function<void(std::optional<std::string>)>`), and two pure virtual methods: `show_open_file_dialog(FileDialogCallback, const char* filter_name, const char* filter_pattern)` and `show_save_file_dialog(FileDialogCallback, const char* filter_name, const char* filter_pattern, const char* default_name)`. **SPEC-037 (window geometry)**: Added `DisplayBounds` struct (`int x, y, width, height`) and two pure virtual methods: `display_count() -> int` and `display_bounds(int index) -> DisplayBounds`. `PlatformSDL3` implements via `SDL_GetNumVideoDisplays()` / `SDL_GetDisplayBounds()`. `PlatformHeadless` returns 0 / `{0,0,0,0}`. |
| `platform.cpp` | Factory implementation: dispatches to SDL3 or Headless backend based on `Backend` enum |
| `platform_sdl3.h` | Private header: `PlatformSDL3` concrete class (final) with embedded `InputSystemSDL3` member, `delta_time_` member, `last_frame_ticks_` for frame timing, `register_window()`/`unregister_window()` public methods, and `std::unordered_map<SDL_WindowID, Window*> window_map_` private member for routing SDL events to the correct `Window` instance. **F-01 (SDL3 native dialogs)**: Added `#include <SDL3/SDL_dialog.h>`, dialog method declarations (`show_open_file_dialog`, `show_save_file_dialog`), and private `get_sdl_window()` helper that returns the first `SDL_Window*` via `win->native_handle()` for use as dialog parent. |
| `platform_sdl3.cpp` | SDL3 backend: `SDL_Init`/`SDL_Quit` lifecycle, `SDL_CreateWindow` delegation with `SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE` flag, `SDL_SetWindowMinimumSize(320, 240)` for minimum size enforcement, window registration via `SDL_GetWindowID()` + `register_window()` in `create_window()`. **SPEC-037 (window geometry)**: After `SDL_CreateWindow`, applies saved window position (`SDL_SetWindowPosition`) and state (`SDL_MaximizeWindow`/`SDL_RestoreWindow`/`SDL_MinimizeWindow`) from `WindowConfig::x`/`y`/`state`. `poll_events()` computes delta from `SDL_GetTicks`, calls `begin_frame()`, handles `SDL_EVENT_WINDOW_RESIZED`/`MAXIMIZED`/`RESTORED` events via windowID map lookup before routing to `InputSystemSDL3::on_sdl_event()` then `engine_imgui::on_sdl_event()` for ImGui. **F-01**: `SDL_EVENT_QUIT` handler now checks `close_request_callback_` before returning `false`. If a callback is registered and returns `false`, the quit event is swallowed (`continue` — editor stays open). If no callback or returns `true`, `poll_events()` returns `false` (normal exit). **F-01 (SDL3 native dialogs)**: Implements `show_open_file_dialog()` via `SDL_ShowOpenFileDialog()` and `show_save_file_dialog()` via `SDL_ShowSaveFileDialog()` using heap-allocated callback pattern (no mutex/queue — SDL3 fires on main thread during `SDL_PollEvent`). `get_sdl_window()` iterates `window_map_` to obtain the `SDL_Window*` parent. |
| `platform_headless.h` | Private header: `PlatformHeadless` concrete class (final) with embedded `InputSystemHeadless` member and `delta_time()` override. **F-01 (SDL3 native dialogs)**: Added `show_open_file_dialog()` and `show_save_file_dialog()` declarations. |
| `platform_headless.cpp` | Headless implementation: no SDL3/OpenGL dependency, validates dimensions; `poll_events()` calls `begin_frame()`; `delta_time()` returns fixed 1/60f. No `SDL_EVENT_QUIT` handling needed — no window to close. **F-01 (SDL3 native dialogs)**: Both dialog methods are no-ops — immediately invoke callback with `std::nullopt`. |

### Window submodule (`window/`)

The `Window` class now stores a non-owning `Platform&` reference, creating a navigable back-link from `Window` to its creating `Platform`. It also exposes mouse capture API. (`window/` now forward-declares `Platform` from `platform/` — see ADR-012.)

| File | Role |
|---|---|---|
| `window.h` | Public header: `WindowConfig` struct (`title`, `width`, `height`, `x`, `y`, `state`), `WindowState` enum (`Normal`, `Maximized`, `Minimized`), `WindowPosition` struct (`int x, y`), abstract `Window` class. Stores `Platform& platform_` (protected member, set via `Window(Platform&)` protected constructor). Provides `platform() -> Platform&`, width/height getters, `native_handle()`, pure virtual `on_resize(int w, int h)` (for updating cached dimensions on window resize), and pure virtual `set_mouse_capture(bool)` / `is_mouse_captured() -> bool`. **F-01**: Added `virtual auto set_title(std::string title) -> void = 0;` for OS window title updates. **SPEC-037 (window geometry)**: Added five pure virtual methods: `position() -> WindowPosition`, `set_position(WindowPosition)`, `state() -> WindowState`, `set_state(WindowState)`, `resize(int w, int h)`. |
| `window_utils.h` | Public header (new in SPEC-037): Declares `window_state_to_string(WindowState) -> std::string` and `parse_window_state(const std::string&) -> WindowState` free functions for state↔string conversion. |
| `window_utils.cpp` | Implementation: `Normal` ↔ `"normal"`, `Maximized` ↔ `"maximized"`, `Minimized` ↔ `"minimized"`. Unknown strings parse to `Normal`. |
| `window_sdl3.h` | Private header: `WindowSDL3` concrete class wrapping `SDL_Window*`. Declares `on_resize()` override to update cached `width_`/`height_`. Implements `set_mouse_capture(bool)` via `SDL_SetWindowRelativeMouseMode` and caches state in `bool captured_`. **F-01**: Added `auto set_title(std::string title) -> void override;`. **SPEC-037**: Added `position()`, `set_position()`, `state()`, `set_state()`, `resize()` overrides. |
| `window_sdl3.cpp` | SDL3 implementation: `on_resize()` updates `width_` and `height_`. Destructor un-registers from `PlatformSDL3`'s windowID map via `unregister_window()` before `SDL_DestroyWindow`. `native_handle()` casts to `void*`. **F-01**: `set_title()` via `SDL_SetWindowTitle()`. **SPEC-037**: `position()` via `SDL_GetWindowPosition()`, `set_position()` via `SDL_SetWindowPosition()`, `state()` via `SDL_GetWindowFlags()`, `set_state()` via `SDL_RestoreWindow`/`SDL_MaximizeWindow`/`SDL_MinimizeWindow`, `resize()` via `SDL_SetWindowSize()` with immediate `width_/height_` update. |
| `window_headless.h` | Private header: `WindowHeadless` concrete class. Declares `on_resize()` override to update cached `width_`/`height_`. Mouse capture is no-op; `is_mouse_captured()` returns `false`. **F-01**: Added `auto set_title(std::string title) -> void override;`. **SPEC-037**: Added `position()`, `set_position()`, `state()`, `set_state()`, `resize()` overrides. |
| `window_headless.cpp` | Headless implementation: stores width/height, `on_resize()` updates cached dimensions (no clamping by design — accepts any values for test flexibility), `native_handle()` returns `nullptr`. **F-01**: `set_title()` is a no-op. **SPEC-037**: `position()` returns `{0,0}`, `state()` returns `WindowState::Normal`, `set_position()`/`set_state()` are no-ops, `resize()` updates `width_/height_` only. |

### Scene submodule (`scene/`)

All types in namespace `buddd::engine`. The scene graph module provides a lightweight entity system with hierarchy, transforms, polymorphic component dispatch, and ECS components (`CameraComponent`). It depends on math wrapper types (`Vec3`, `Quat`, `Mat4`) from `src/engine/math/` and standard C++ headers only — no GLM, SDL3, or OpenGL dependencies.

Component dispatch uses `dynamic_cast<T*>()` (RTTI-based) for type-safe retrieval, with zero boilerplate in component types. See ADR-006 for the decision rationale.

| File | Role |
|---|---|
| `entity_id.h` | `EntityId` struct — 8-byte handle (index + generation) for safe entity references. Header-only. |
| `transform.h` | `Transform` struct — position (`Vec3`), rotation (`Quat`), scale (`Vec3`) value type with `local_matrix()` and `world_matrix()`. Header-only. |
| `component.h` | `Component` polymorphic base class — non-copyable, non-movable. Provides entity-awareness via protected `world_`/`entity_id_` members and `entity()` accessor. Virtual `on_attach()` lifecycle hook called by `World::add_component<T>()` after attachment. Header-only. |
| `entity.h` | `Entity` class — 16-byte lightweight handle (`World*` + `EntityId`). Inline template methods for component operations. **New**: `name()` / `set_name()` accessors for entity naming (default empty string). **New**: `source()` / `set_source()` accessors for entity source tracking (prefab/model origin). **New**: `component_count()` and `component_at()` for component iteration. |
| `entity.cpp` | Entity non-inline method implementations — all delegate to `World`. `Entity::name()` delegates to `World::get_name()`, `Entity::set_name()` delegates to `World::set_name()`. `Entity::source()` delegates to `World::get_source()`, `Entity::set_source()` delegates to `World::set_source()`. `Entity::component_count()` and `Entity::component_at()` delegate to `World`. |
| `world.h` | `World` class — top-level container managing entity lifecycle, tree hierarchy, deferred destruction. Template methods for component dispatch (`add_component`, `get_component`, `remove_component`) and type-based iteration (`each<T>()`) defined inline. Camera registration API (`register_camera`, `unregister_camera`, `active_camera`) stores a `CameraComponent&` reference in an `std::optional<CameraComponent&>` member. **New**: `add_component_raw(EntityId, unique_ptr<Component>)` for runtime-type component injection (used by SceneLoader). **New**: `entity_count() -> size_t` introspection method. **New**: `get_source()`/`set_source()` for entity source tracking. **New**: `root_entity_count()`/`get_root_entity()` for root entity iteration. **New**: `component_count()`/`get_component_at()` for component iteration on entities. EntityNode gains `std::string name_` and `EntitySource source_` fields. |
| `world.cpp` | World implementation including internal `EntityNode` type, slot-based storage, `flush_destroyed()` logic, and `mark_for_destroy()` iterative traversal. Implements `get_name()`, `set_name()`, `add_component_raw()`, `entity_count()`, `get_source()`, `set_source()`, `root_entity_count()`, `get_root_entity()`, `component_count()`, and `get_component_at()`. |
| `camera_component.h` | `CameraComponent` ECS component class — projection-only, inherits `Component`. Camera position/orientation from entity Transform. Auto-registers with `World` via `on_attach()` and unregisters on destruction (address-based comparison). |
| `updatable.h` | `Updatable` pure abstract interface — orthogonal to `Component`. Provides `virtual update(const EngineContext& ctx) -> void`. Components can inherit from both via multiple inheritance. Auto-registered via `World::add_component<T>()` when `std::is_base_of_v<Updatable, T>`. Auto-dispatched in `run_app()` before `render_system.render_scene()`. |
| `free_camera_movement.h` | `FreeCameraMovement` component class — inherits both `Component` and `Updatable`. Encapsulates free-camera controls: WASD movement, mouse look, right-click capture toggle, ESC to exit. Configurable `move_speed`, `mouse_sensitivity`, `pitch_clamp_degrees`, `invert_yaw`, `invert_pitch`. |
| `free_camera_movement.cpp` | FreeCameraMovement implementation — input handling, camera state (yaw/pitch), component lifecycle. |
| `camera_component.cpp` | CameraComponent implementation: projection/view matrix computation, `look_at()` convenience methods, `on_attach()` calls `world().register_camera(*this)`, destructor calls `world_->unregister_camera(*this)` (with null guard). |
| `directional_light_component.h` | `DirectionalLightComponent` — infinite parallel light. Direction from entity rotation (-Z forward). Properties: `color` (Vec3), `intensity` (float). `on_attach()` no-op. |
| `directional_light_component.cpp` | Constructor and accessor implementations. |
| `point_light_component.h` | `PointLightComponent` — omni-directional light with position (from entity translation) and range. Properties: `color`, `intensity`, `range` (float, default 10.0). `on_attach()` no-op. |
| `point_light_component.cpp` | Constructor and accessor implementations. |
| `spot_light_component.h` | `SpotLightComponent` — conical light with position, direction, and cone angles. Properties: `color`, `intensity`, `range`, `inner_angle` (default 0.785 rad ≈ 45°), `outer_angle` (default 1.047 rad ≈ 60°). `on_attach()` no-op. |
| `spot_light_component.cpp` | Constructor and accessor implementations. |
| `scene_loader.h` | `SceneLoader` class — parses YAML scene/prefab files (`type: Scene`/`type: Prefab`, `version: 1`) and populates a `World` with hierarchical entities, transforms, and deserialized components. Stores `World&`, `ComponentRegistry&`, `AssetManager&`. Public API: `load_from_file(path) -> Result<void>`, `load_from_yaml(node) -> Result<void>`, `compose_transform(prefab, instance) -> Transform` (static, public for testing). |
| `scene_loader.cpp` | SceneLoader implementation. Parses YAML via yaml-cpp, validates type/version, iterates entities with `load_entity()`, loads prefabs via `load_prefab()` with cycle detection (`loading_prefabs_` visited-set), parses transforms with `parse_transform()`, composes transforms with `compose_transform()` (position additive, scale multiplicative, rotation quaternion-multiplied). Unknown component types are skipped with a warning (forward-compatible). Unknown YAML keys produce warnings. Sets `EntitySource` on entities created from `prefab:` (`Prefab`) and `model:` (`Model`) directives. |
| `entity_source.h` | `EntitySourceType` enum (`None`, `Prefab`, `Model`) and `EntitySource` struct (`type` + `path`) — tracks whether an entity was created directly, from a prefab, or from a model directive. Header-only. |
| `scene_saver.h` | `SceneSaver` class — symmetric to `SceneLoader`. Public API: `save_to_file(path) -> Result<void>`, `save_to_yaml() -> YAML::Node`. Private helpers: `save_entity()`, `save_transform()`, `build_type_to_info_map()`. |
| `scene_saver.cpp` | SceneSaver implementation. Serializes World back to YAML respecting entity source types: prefab entities emit `prefab:` + name + transform, model entities emit `model:` + name + transform, direct entities emit full expansion with components and children. Transform fields at defaults are omitted. Uses a reverse-lookup map (`std::type_index → ComponentInfoBase*`) built at construction to find component info for serialization. |

### Component Registry submodule (`component_registry/`)

All types in namespace `buddd::engine`. Provides a runtime type registry, component metadata system, property descriptors, and generic YAML serialization for components. See [SPEC component-registry](/.specs/sprint-2026-06/component-registry/spec.md) for the full specification.

| File | Role |
|---|---|
| `type_registry.h` / `type_registry.cpp` | `TypeRegistry` — static class mapping each C++ type to YAML encode/decode, string conversion, and validation callbacks. Eight built-in types pre-registered at startup (`float`, `int32_t`, `bool`, `std::string`, `Vec3`, `Vec4`, `Quat`, `std::shared_ptr<Model>`). External code registers custom types via `TypeRegistry::register_type<T>(TypeInfo<T>)`. |
| `property.h` / `property.cpp` | `Property` — internal type-erased descriptor wrapping a component field's name, getter/setter lambdas, serialization callbacks (delegating to TypeRegistry), and optional `PropertyFlags` constraints. `PropertyFlags` struct provides numeric min/max/step and enum choices. **New**: `DefaultChecker` type and constructor parameter — a callable that compares a property's current value against the component's registered default. `serialize()` returns a null node when the value matches the default, enabling default-valued properties to be omitted from saved YAML. Not user-facing. |
| `component_info.h` | `ComponentInfoBase` (type-erased base) and `ComponentInfo<T>` (typed template). Each component type has one info object holding its canonical string name, factory function, and property descriptors. `ComponentInfo<T>` provides three `add_property<PropType>()` overloads: (A) convention-based (deferred for v1), (B) simple lambdas, (C) context-aware lambdas. **New**: `add_property()` now computes a default checker lambda and passes it to the `Property` constructor. `serialize()` skips null-node properties (properties whose value matches the registered default), producing compact YAML output. |
| `component_registry.h` / `component_registry.cpp` | `ComponentRegistry` — maps string type names to `ComponentInfoBase*`. Key API: `register_component<T>(name)`, `create(name)`, `describe(name)`, `all_types()`. Populated once at startup via `register_all_components()`. |
| `serialization_context.h` | `SerializationContext` — struct holding an `AssetManager&` reference, passed to all TypeRegistry callbacks and serialize/deserialize functions for context-dependent operations (e.g., asset ID resolution). |
| `serialization.h` / `serialization.cpp` | Free functions `serialize_component()` and `deserialize_component()` — iterate a component's properties and delegate to TypeRegistry for YAML I/O. Produce YAML mapping nodes with one key per property. Unknown keys produce a warning (skipped). |
| `register_all_components.h` / `register_all_components.cpp` | Registration entry point: `register_builtin_types()` pre-registers the eight built-in TypeRegistry types; `register_all_components(ComponentRegistry&)` registers all six engine components (`CameraComponent`, `PointLightComponent`, `DirectionalLightComponent`, `SpotLightComponent`, `MeshRenderer`, `FreeCameraMovement`) with their properties. `FreeCameraMovement` is registered as `"free_camera_movement"` with 5 properties (`move_speed`, `mouse_sensitivity`, `pitch_clamp_degrees`, `invert_yaw`, `invert_pitch`). Called from `EngineService` initialization where `ComponentRegistry` is now stored persistently as a member. |

### Image submodule (`image/`)

All types in namespace `buddd::engine`. Provides pixel buffer representation and PNG I/O via stb_image/stb_image_write. Depends on `error.h` for `Result<T>` types.

| File | Role |
|---|---|
| `image_buffer.h` | `ImageBuffer` aggregate struct — `int width`, `int height`, `int channels`, `std::vector<std::byte> data`. Pure aggregate, no methods. |
| `image.h` | `Image` class — static `create(const ImageBuffer&) -> Result<Image>` (validates, flips rows), static `load(std::string_view) -> Result<Image>` (PNG via stb_image), `save(std::string_view) const -> Result<void>` (PNG via stb_image_write), and accessors. Non-copyable, movable. |
| `image.cpp` | Image implementation: row-flipping logic (bottom-left → top-left), stb_image/stb_image_write implementation via `#define STB_IMAGE_IMPLEMENTATION` / `STB_IMAGE_WRITE_IMPLEMENTATION`. |

### ImGui submodule (`imgui/`)

All types in namespace `buddd::engine::engine_imgui`. Provides an immediate-mode GUI integration embedding Dear ImGui's official SDL3 and OpenGL3 backends. Dear ImGui (docking branch, tag `v1.91.8-docking`) is fetched via `FetchContent` in `src/engine/CMakeLists.txt` and compiled as part of `buddd_engine`. The module exposes a minimal public API for lifecycle and event routing; apps use Dear ImGui's C++ API directly (`ImGui::Begin()`/`End()`/`Text()`/`Button()` etc.) without any wrapper or indirection. See SPEC-026 and ADR-026.

The frame lifecycle is fully automated: `engine_imgui::new_frame()` is called from within `RenderDeviceOpenGL::begin_frame()` after the buffer clear, and `engine_imgui::render()` is called from within the new `RenderDevice::render_ui()` virtual method (default no-op). `run_app()` calls `device.render_ui()` as a generic UI step — no ImGui-specific code in `run_app()`. SDL events are routed to ImGui in `PlatformSDL3::poll_events()` after the engine's input system.

Headless: All ImGui functions are no-ops when not initialised (`is_initialized()` returns false).

| File | Role |
|---|---|
| `imgui/engine_imgui.h` | **Public header.** Declares `init(SDL_Window*, void*) -> Result<void>`, `shutdown()`, `new_frame()`, `render()`, `on_sdl_event(const SDL_Event&) -> bool`, `is_initialized() -> bool` in namespace `buddd::engine::engine_imgui`. Forward-declares `SDL_Window` and `SDL_Event` — no SDL3 headers in public API. |
| `imgui/engine_imgui.cpp` | Implementation: wraps ImGui context creation/destruction, `ImGui_ImplSDL3_InitForOpenGL`/`ImGui_ImplOpenGL3_Init`, NewFrame/render chain, SDL event forwarding. No-op guards via `is_initialized()`. |
| `imgui/CMakeLists.txt` | Build rules: adds `engine_imgui.cpp` to `buddd_engine` sources, sets include directories for ImGui (`imgui/` subdirectory) and backend headers (`backends/`). |

Dependencies: `buddd_engine` (via FetchContent: ImGui docking branch), SDL3 (existing), OpenGL 4.5 Core (existing). The ImGui library sources (`imgui/` subdirectory with `imgui.cpp`, `imgui_demo.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, etc.) and backends (`backends/` with `imgui_impl_sdl3.h/.cpp`, `imgui_impl_opengl3.h/.cpp`) are embedded in `src/engine/imgui/` alongside the module wrapper.

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
| `asset_manager.cpp` | Non-template implementation: `load_texture()`, `load_material()`, `poll_file_events()`, hot-reload handlers (`handle_yaml_change`, `handle_source_change` — fully implemented, reload assets in-place with GPU handle swap), explicit template instantiations. Diagnostic output (cache events, loading status, texture creation) uses `BUDDD_LOG_*` macros via tag `Asset`. |
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
| `file_watcher_inotify.cpp` | Inotify implementation: `inotify_init1`, `add_watch_recursive()` (walks directory tree recursively adding inotify watches for all subdirectories), blocking `poll()`-based read loop, mutex-protected queue, self-pipe for wake-on-shutdown. Diagnostic output (file events, watcher lifecycle) uses `BUDDD_LOG_DEBUG`/`BUDDD_LOG_INFO` via tag `Asset:FileWatcher`. |
| `model_loader.h` | Internal detail header: `ModelLoadResult` struct and `load_gltf_model()` free function in `buddd::engine::detail` namespace. Converts tinygltf data to engine `ModelNode` trees. Returns `Result<ModelLoadResult>`. |
| `model_loader.cpp` | ModelLoader implementation: tinygltf integration, vertex conversion (glTF Vertex → engine Vertex with POSITION/COLOR_0/NORMAL/TEXCOORD_0/TANGENT/TEXCOORD_1 mapping), index type detection (Uint16/Uint32), material conversion to `PbrMaterial`, texture loading (embedded via buffer view or external via `Image::load`), hierarchy building, magenta fallback for missing textures. Diagnostic output (glTF errors, warnings) uses `BUDDD_LOG_ERROR`/`BUDDD_LOG_WARN` via tag `Asset:ModelLoader`. |

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
| `light_data.h` | Detail header: `LightData` struct with fields `position_or_dir` (Vec4, .w=type), `color` (Vec4, .rgb = color * intensity pre-multiplied), `range` (float), `spot_direction` (Vec4), `inner_cone_cos` (float), `outer_cone_cos` (float). `k_max_lights = 8`. |
| `shader.h` | Public header: `ShaderType` enum (`Vertex`, `Fragment`), abstract `Shader` class with `type()` pure virtual. Non-copyable, non-movable. |
| `material.h` | Public header: abstract `Material` class with 6 `set_uniform` overloads (`float`, `int32_t`, `bool`, `math::Vec3`, `math::Vec4`, `math::Mat4`), `has_uniform()`, `set_texture(name, shared_ptr<Texture>)`, `has_texture(name)`, and `bind()` (deferred state application: program activation + uniforms + textures). Non-copyable, non-movable. |
| `vertex_buffer.h` | Public header: abstract `VertexBuffer` class with `format()` pure virtual. Non-copyable, non-movable. |
| `index_buffer.h` | Public header: `IndexType` enum (`Uint16`, `Uint32`), abstract `IndexBuffer` class with `type()` pure virtual. Non-copyable, non-movable. |
| `render_device.h` | Public header: abstract `RenderDevice` class with `create(Window&)` static factory, `begin_frame()`, `end_frame()`, `size()`, resource factory methods (`create_shader`, `create_material`, `create_material(shared_ptr<ShaderProgram>)`, `create_vertex_buffer`, `create_index_buffer`, `create_texture(const Image&)`, `read_pixels()`), and draw methods (`draw`, `draw_indexed`). Draw methods return `void` — deliberate exception to ADR-001. `create_texture` returns `Result<std::unique_ptr<Texture>>`. **New**: pure virtual `window() -> Window&` enables navigation to `Window` and via it to `Platform`/`InputSystem`. **New**: virtual diagnostic accessors `frame_begin_count()`, `frame_end_count()`, `draw_call_count()` with default `0` implementations (overridden by headless backend). **New**: `fallback_material()` returns a shared magenta `Material&` used by `Model::draw()` for null/out-of-bounds material references. **New**: virtual `render_ui() -> void` (default no-op body) renders any active UI overlay — `RenderDeviceOpenGL` overrides to call `engine_imgui::render()`. See ADR-012 and ADR-017. The `create_material(shared_ptr<ShaderProgram>)` overload enables shader program deduplication — materials share the compiled GL program but retain independent uniform/texture state. |
| `render_device.cpp` | Factory implementation: dispatches to OpenGL or Headless backend based on `native_handle()` value. Passes `Window&` to both backend constructors. Calls `engine_imgui::init(sdl_window, gl_context)` after `SDL_GL_MakeCurrent` when a display is available. Diagnostic output (backend selection errors, ImGui init) uses `BUDDD_LOG_*` via tag `Render`. |
| `render_device_opengl.h` | Private header: `RenderDeviceOpenGL` concrete class. Stores `Window& window_` (for the `window()` accessor) and `SDL_Window* sdl_window_` for internal SDL calls, plus `SDL_GLContext`. Declares `render_ui()` override to call `engine_imgui::render()`. |
| `render_device_opengl.cpp` | OpenGL 4.5 Core implementation: GLSL compilation via `glCreateShader`/`glCompileShader`, program linking via `glCreateProgram`/`glLinkProgram`, VAO/VBO/IBO management via DSA APIs (`glCreateVertexArrays`, `glNamedBufferStorage`, etc.), texture creation via DSA (`glCreateTextures`/`glTextureStorage2D`/`glTextureSubImage2D`), and draw dispatch via `glDrawArrays`/`glDrawElements`. `draw()`/`draw_indexed()` call `material.bind()` before draw to apply deferred state. `begin_frame()` calls `engine_imgui::new_frame()` after the buffer clear. `render_ui()` calls `engine_imgui::render()`. Destructor calls `engine_imgui::shutdown()` before `SDL_GL_DestroyContext`. Diagnostic output (compilation, linking, fallback material creation) uses `BUDDD_LOG_*` macros via tag `Render:OpenGL`. |
| `render_device_headless.h` | Private header: `RenderDeviceHeadless` concrete class. Stores `Window& window_` (replaces `int width_, height_`). `size()` delegates to `window_.width()`/`window_.height()`. Overrides diagnostic counters (`frame_begin_count()`, `frame_end_count()`, `draw_call_count()`). Unconditional `read_pixels()` error. |
| `render_device_headless.cpp` | Headless implementation: stores shader source and vertex data in memory; simulates compilation errors via `#error` marker and linking errors via vertex/fragment I/O mismatch detection; draw calls are no-ops; increments `frame_begin_count_`/`frame_end_count_` in `begin_frame()`/`end_frame_`(). Diagnostic output (shader compilation, linking errors) uses `BUDDD_LOG_*` macros via tag `Render:Headless`. |
| `shader_opengl.h` | Private header: `ShaderOpenGL` concrete class wrapping a `GLuint` shader handle |
| `shader_opengl.cpp` | OpenGL shader backend: resource lifetime managed via `glCreateShader`/`glDeleteShader` |
| `shader_headless.h` | Private header: `ShaderHeadless` concrete class storing type and GLSL source string |
| `shader_headless.cpp` | Headless shader backend: stores source for linking-error simulation and uniform discovery |
| `shader_program.h` | Abstract `ShaderProgram` base class — wraps a compiled shader program handle (`uint32_t`). Pure virtual `handle()`, `is_valid()`, `replace_handle()`, `release_handle()`. Non-copyable, non-movable. ADR-019 compliant (uses `uint32_t` not `GLuint` in the base). |
| `shader_program.cpp` | Vtable emission + default virtual implementations (`testing_handle()`, `vs_source()`, `fs_source()`). |
| `shader_program_opengl.h` | `ShaderProgramOpenGL` concrete class — wraps a `GLuint` program handle. `replace_handle` calls `glDeleteProgram` before assigning. `release_handle()` extracts the handle to prevent double-deletion. |
| `shader_program_opengl.cpp` | OpenGL implementation: `glCreateProgram`, `glAttachShader`, `glLinkProgram`, `glDeleteProgram`. |
| `shader_program_headless.h` | `ShaderProgramHeadless` concrete class — stores generation counter and source strings. `handle()` always returns 0. `replace_handle` is no-op. |
| `shader_program_headless.cpp` | Headless implementation: simulated linking via vertex/fragment I/O matching, generation counter for `testing_handle()`. |
| `material_opengl.h` | Private header: `MaterialOpenGL` concrete class with deferred uniform caching (`std::unordered_map<std::string, std::variant<...>> uniform_cache_`), texture map (`std::unordered_map<std::string, std::shared_ptr<Texture>> texture_map_`), `glGetUniformLocation`-based location caching, and a mutable texture unit counter (`mutable int next_unit_{0}`) for automatic unit assignment during `bind()`. |
| `material_opengl.cpp` | OpenGL material backend: `set_uniform` caches values in `uniform_cache_` (no immediate GL calls). `set_texture` stores the shared texture pointer by name. `bind()` applies deferred state: calls `glUseProgram`, then flushes all cached uniforms via `glUniform1f`/`glUniform1i`/`glUniform3fv`/`glUniform4fv`/`glUniformMatrix4fv`, then binds textures to sequential units via `glActiveTexture`/`glBindTexture` and sets `glUniform1i` for each sampler uniform. Program destruction via `glDeleteProgram`. Diagnostic output (uniform failures) uses `BUDDD_LOG_WARN` via tag `Render:OpenGL`. |
| `material_headless.h` | Private header: `MaterialHeadless` concrete class with `std::unordered_set` of known uniform names, `std::variant`-based uniform value storage, and diagnostic accessors: `get_uniform_mat4(name)`, `get_uniform_vec3(name)`, `get_uniform_vec4(name)`, `get_uniform_float(name)`, `get_uniform_int(name)`. Stores texture map (`std::unordered_map<std::string, std::shared_ptr<Texture>>`) for `set_texture`/`has_texture`. |
| `material_headless.cpp` | Headless material backend: in-memory uniform state tracking; `has_uniform` checks known names + previously-set names; `set_uniform` returns `UniformNotFound` for unknown names; diagnostic getters return stored value or `std::nullopt`; all methods use `normalize_uniform_name()` to resolve array subscript brackets; `set_texture` and `has_texture` operate on in-memory texture map; `bind()` is a no-op. Diagnostic output (uniform errors) uses `BUDDD_LOG_WARN` via tag `Render:Headless`. |
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
| `model_node.h` | Public header: `ModelNode` struct — a node in the glTF model hierarchy. Fields: `name`, `translation` (Vec3), `rotation` (Quat), `scale` (Vec3), `std::shared_ptr<Model> model` (nullptr if node has no mesh), `std::vector<ModelNode> children`. Move-only, publicly constructible. Multiple entities can reference the same model — GPU buffers are shared via `shared_ptr`. See glTF model loading spec. |
| `model_utils.h` | Public inline header: `add_model_to_world()` free function traverses a `ModelNode` tree depth-first and creates ECS entities with `Transform` + `MeshRenderer` for each mesh node. Uses `World`/`Entity` (engine's custom ECS). All implementation is inline. See glTF model loading spec. |
| `primitives.h` | Public header: Geometry-only factory functions in `buddd::engine` namespace — `create_cube(device, material)`, `create_triangle(device, material)`, `create_quad(device, material)`. Each returns a `Result<Model>` with one SubMesh covering all indices and the caller-provided material. 24-byte stride (Float3 position + Float3 color). See [ADR-017](/docs/adr/ADR-017-multi-material-model.md). |
| `primitives.cpp` | Implementation of primitive helpers: inline vertex/index data definitions, delegates to `Model::create_indexed()` for buffer creation. |
| `mesh_renderer.h` | Public header: `MeshRenderer` ECS component — inherits `Component`, holds a `std::shared_ptr<Model>`. Provides `model()` accessor. Used by `RenderSystem` to discover drawable entities via `World::each<MeshRenderer>()`. |
| `mesh_renderer.cpp` | MeshRenderer implementation: constructor stores the shared Model pointer. |
| `render_system.h` | Public header: `RenderSystem` engine-level class — bridges `RenderDevice` and `World`. Constructor takes `RenderDevice&` and `World&`. Single `render()` method: calls `begin_frame()`/`end_frame()`, queries `active_camera()` for view-projection, iterates `World::each<MeshRenderer>()` to issue draw calls. |
| `render_system.cpp` | RenderSystem implementation: `render()` orchestrates one frame — begin/end frame lifecycle, camera lookup, MVP computation per MeshRenderer entity, uniform setting, and draw dispatch. **Extended for Phong lighting**: before MeshRenderer iteration, collects all `DirectionalLightComponent`, `PointLightComponent`, and `SpotLightComponent` entities into a `LightData` array (max 8). For each MeshRenderer, checks `has_uniform("u_model")` sentinel: if true, sets all lighting uniforms (u_model, u_normal_mat, u_camera_pos, flat array light uniforms, material defaults). If false (unlit material), only sets u_mvp (backward compat). Logs warnings via `BUDDD_LOG_WARN` for missing camera or uniform failures (per-entity skip). |

### Phong submodule (`render/phong/`)

| File | Role |
|---|---|
| `phong_shaders.h` | Detail header: `constexpr std::string_view` constants containing embedded GLSL 450 core vertex and fragment shader source for the Phong reflection model. Vertex shader: inputs `a_position` (loc 0), `a_color` (loc 1, unused), `a_normal` (loc 2), `a_texcoord` (loc 3); uniforms `u_mvp`, `u_model`, `u_normal_mat`. Fragment shader: `#define MAX_LIGHTS 8`, flat array uniforms for light data (`u_light_positions_or_dir`, `u_light_colors`, `u_light_ranges`, `u_light_spot_directions`, `u_light_inner_cones`, `u_light_outer_cones`), material uniforms (`u_material_ambient`, `u_material_specular`, `u_material_shininess`, `u_material_diffuse_tint`), sampler `u_diffuse_texture.` Implements Blinn-Phong specular, Lambertian diffuse, squared-distance attenuation, and spotlight cone falloff via `spot_cone_attenuation()`. |
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

Uses an `App` lifecycle pattern: a virtual `App` base class (`src/cmd/app.h`) defines `config()` / `setup()` / `render()` / `shutdown()`, and a centralised `run_app()` free function owns the render loop. Scene implementations are `App` subclasses in `src/cmd/apps/`. The CLI dispatches four commands (`run`, `edit`, `version`, `help`) — see [ADR-014](/docs/adr/ADR-014-cli-app-system.md) and [ADR-027](/docs/adr/ADR-027-editor-architecture.md) for the architectural rationale.

### Build system

`src/cmd/CMakeLists.txt` uses `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)` covering `src/cmd/*.cpp` (for `main.cpp` and app files), `src/cmd/commands/*.cpp` (for command files), `src/cmd/demo/*.cpp` (for demo helpers), and `src/cmd/apps/*.cpp` (for App subclasses). New scenes can be added by creating files in `src/cmd/apps/` and adding a dispatch branch in `main.cpp` — no CMakeLists.txt change needed.

### File structure

| File | Role |
|---|---|
| `main.cpp` | Dispatcher: parse first positional argument, dispatch to matching handler. If no arg or arg is `run`: parse `<scene>`, create the appropriate `App` subclass, call `run_app()`. Also handles `version` and `help` commands. No engine header includes beyond those needed for forward declarations. Error/parse diagnostics use `BUDDD_LOG_ERROR`/`BUDDD_LOG_WARN` via tag `App`; pre-init bootstrap error and usage/help text blocks remain as `fprintf(stderr)` (exempted per SPEC-022). |
| `app.h` | Declares `AppConfig` struct (title, width, height, window_x, window_y, window_state), `App` base class with virtual lifecycle (`config()`, `setup(EngineContext const&)`, `on_frame_begin(EngineContext const&)` (default no-op), `on_render(EngineContext const&)` (default no-op, replaces `render()`), `shutdown()`), and `run_app()` free function. Removed: `render(RenderDevice&, int)`, `world()`, `is_running()`/`set_running()`/`running_`. |
| `app.cpp` | Implementation of `run_app()`: creates `EngineService` via `EngineService::create()`, then creates `World`+`RenderSystem` unconditionally (owned by `run_app()`), calls `app.setup(ctx)` with full `EngineContext`, runs the central render loop (`poll_events()` → `begin_frame()` → `on_frame_begin(ctx)` → `World::update_updatables(ctx)` → `render_system.render_scene()` → `app.on_render(ctx)` → capture injection → `end_frame()` with frame limiting via `--frame` and capture injection via `--capture`), then calls `app.shutdown()`. Creates `EngineContext` per frame with all 7 fields (`services`, `window`, `device`, `world`, `render_system`, `delta_time`, `frame`). Exit is signalled via `ctx.is_exit_requested()` only. Lifecycle messages use `BUDDD_LOG_INFO` via tag `App`. |
| `app_config.h` | Declares `CaptureSpec` struct (frame number + path) and `RunningArgs` struct (frame limit + capture specs), plus `parse_running_args()` to parse `--frame N` and `--capture N:path` from argv. |
| `app_config.cpp` | Implementation of `parse_running_args()`. |

### Command files (`src/cmd/commands/`)

| File | Role |
|---|---|
| `version_command.h` / `version_command.cpp` | `buddd::cmd::VersionCommand` — prints `buddd <version>` from `be::version()` to stdout and exits 0. Extra args silently ignored. |
| `help_command.h` / `help_command.cpp` | `buddd::cmd::HelpCommand` — prints usage text to stdout and exits 0. Also defines `k_usage_text` constant used by the unknown-command handler in `main.cpp`. Extra args silently ignored. |

### App subclasses (`src/cmd/apps/`)

Each scene is an `App` subclass. The `config()` method returns the window configuration; `setup(EngineContext const&)` performs one-time initialisation via the full context (device, services, etc.); `shutdown()` handles cleanup. Per-frame rendering is handled by `run_app()` automatically calling `render_system.render_scene()` before `app.on_render(ctx)`. Apps that need per-frame state updates (transforms, animations, hot-reload polling) override `on_frame_begin(ctx)`. Scene state lives as member variables — Entity handles are stored by value (16-byte handle, no `unique_ptr<Entity>`).

| File | Role |
|---|---|
| `run_app.h` / `run_app.cpp` | `RunApp` — empty window, clears framebuffer each frame (no draw calls), runs interactively until window close. |
| `triangle_app.h` / `triangle_app.cpp` | `TriangleApp` — 120-frame coloured triangle demo using `engine::create_triangle()`. |
| `cube_app.h` / `cube_app.cpp` | `CubeApp` — 120-frame rotating per-face-coloured cube (camera entity + MVP). Uses `engine::create_cube()`. |
| `cube_scene_app.h` / `cube_scene_app.cpp` | `CubeSceneApp` — 120-frame rotating cube using `World` + `RenderSystem` (ECS approach). |
| `textured_cube_app.h` / `textured_cube_app.cpp` | `TexturedCubeApp` — 120-frame rotating UV-mapped cube with brick texture using scene graph. |
| `free_camera_app.h` / `free_camera_app.cpp` | `FreeCameraApp` — interactive fly-through camera (WASD + mouse look + Space/Control). Uses `Platform::delta_time()` for frame-rate-independent movement. Exit via Escape key. Uses `PhongMaterial` with orbiting point light + directional fill. |
| `phong_app.h` / `phong_app.cpp` | `PhongApp` — interactive Phong lighting demo. Textured cubes with `PhongMaterial`, orbiting `PointLightComponent`, static `DirectionalLightComponent` fill. Interactive free-camera (WASD + mouse, right-click to capture). Runs until Escape. Uses ECS: World + RenderSystem + light components + MeshRenderer + PhongMaterial. |
| `hot_reload_app.h` / `hot_reload_app.cpp` | `HotReloadApp` — hot-reload verification test. Loads a material from YAML, swaps texture at frame 30 via `poll_file_events()`. Use with `--capture 30:before.png --capture 60:after.png` to verify before/after. Overrides `on_frame_begin(ctx)` to call `ctx.services.assets().poll_file_events()` and update entity rotation. |
| `multi_material_app.h` / `multi_material_app.cpp` | `MultiMaterialApp` — 120-frame multi-material demo. Creates a cube with 3 submeshes (red/green/blue face pairs) using `Model::create_indexed()` directly. Each submesh references a different material index. Demonstrates multi-material draw call batching. |
| `gltf_demo_app.h` / `gltf_demo_app.cpp` | `GltfDemoApp` — loads a glTF model (Box or DamagedHelmet) from YAML via `ctx.services.assets().create<ModelAsset>()`, traverses the `ModelNode` tree using `add_model_to_world()`, and renders with orbit camera and PBR materials. Continuous Y rotation in `on_frame_begin(ctx)` using `ctx.frame`. See glTF model loading spec. |
| `gltf_helmet_app.h` / `gltf_helmet_app.cpp` | `GltfHelmetApp` — loads DamagedHelmet model with free-camera controls. Uses `ctx.services.assets()` for model loading. Camera auto-updated via `FreeCameraMovement` (Updatable system). No `on_render()` override needed (render_scene is automatic). 1280×720 window, directional light (white, 1.5, -45° pitch, 45° yaw). |
| `hot_reload_gltf_app.h` / `hot_reload_gltf_app.cpp` | `HotReloadGltfApp` — hot-reload verification for glTF models. Loads a model via `ctx.services.assets()`, stores `AssetManager*` (non-owning) from `setup()`. Swaps source file at frame N, triggers `poll_file_events()`, validates the model updates in-place. Camera animation in `on_frame_begin(ctx)` using `ctx.frame`. Extends the hot-reload pattern from `HotReloadApp`. |
| `imgui_demo_app.h` / `imgui_demo_app.cpp` | `ImguiDemoApp` — 300-frame ImGui verification demo. No 3D scene — just a cleared framebuffer with ImGui overlay. Shows `ImGui::ShowDemoWindow()` and a custom "ImGui Demo" panel with FPS counter and show-demo toggle checkbox. 1280×720 window, title `"Buddd Engine — ImGui Demo"`. Supports `--capture` for CI verification. |
| `scene_app.h` / `scene_app.cpp` | `SceneApp` — `App` subclass that loads a scene from a CLI-provided YAML file path. `config()` returns `AppConfig` with title derived from the file stem (e.g., `"demo"`), 1024×768. `setup()` creates a `SceneLoader` and calls `load_from_file(scene_path_)`. Dispatched from `main.cpp` when `buddd run <path.yaml>` is used (YAML extension auto-detection takes priority before named-scene dispatch). |

### Demo helpers (`src/cmd/demo/`)

The `demo_helpers.*` files are now **empty placeholders**. All helper functions (`setup_cube()`, `setup_triangle()`) and types (`CubeResources`) have been removed. Use `engine::create_cube()`, `engine::create_triangle()`, and `engine::create_quad()` from `src/engine/render/primitives.h` instead. See [ADR-017](/docs/adr/ADR-017-multi-material-model.md).

| File | Role |
|---|---|
| `demo_helpers.h` / `demo_helpers.cpp` | Intentionally empty. All code migrated to engine primitives. |

### Subcommand behavior

- `buddd` (no arguments) or `buddd run` → opens 1024×768 window, empties framebuffer each frame (no draw calls), runs until user closes window
- `buddd run <path.yaml>` — YAML scene file auto-detection (takes priority before named-scene dispatch). If the argument ends with `.yaml`/`.yml` (case-insensitive) and the file exists, a `SceneApp` is created that loads the YAML file and populates the World. `--frame N` and `--capture N:path` work via `run_app()` infrastructure. If the file does not exist, an error is printed and the app exits with code 1.
- `buddd run <scene> [--frame N] [--capture N:path]...` → runs the named scene. Available scenes: `triangle` (120 frames, coloured triangle), `cube` (120 frames, rotating coloured cube), `cube-scene` (120 frames, ECS-based cube), `textured-cube` (120 frames, UV-mapped cube with brick texture), `free-camera` (interactive, WASD + mouse look + Space/Control), `phong` (interactive, Phong lighting with orbiting point light + directional fill), `hot-reload` (60 frames, hot-reload verification, swaps texture at frame 30), `multi-material` (120 frames, cube with red/green/blue submeshes), `gltf-demo` (interactive, loads Box/DamagedHelmet glTF model with PBR materials and orbit camera), `hot-reload-gltf` (hot-reload verification for glTF models, swaps model file at frame N), **`imgui-demo`** (300 frames, ImGui overlay with demo window and custom panel on cleared background, 1280×720). `--frame N` limits rendering to N frames. `--capture N:path` captures frame N to a PNG file (repeatable for multiple captures). If no scene is given, defaults to `RunApp` (empty window). If scene is unknown, prints error to stderr and exits 1. Extra unexpected positional arguments print a warning on stderr.
- `buddd version` → prints `buddd 0.1.0` to stdout
- `buddd edit` → opens the editor (1280×800 ImGui-docked window, title "Buddd Editor"). Requires display; errors out if `BUDDD_HAS_DISPLAY=OFF`. Editor does not exit on Escape — only window close exits.
- `buddd edit <path.yaml>` — opens the editor with the given scene file pre-loaded. The argument is detected as a YAML path (`.yaml`/`.yml`, case-insensitive). If the path is a regular file, the editor opens with that scene loaded during `setup()`. If the path does not exist or is not a regular file, an error is printed to stderr and the process exits with code 1 (no editor window opened). `--frame N` and `--capture N:path` flags continue to work after the scene path.
- `buddd edit <arg>` where `<arg>` is not a YAML path and does not start with `-` → unknown argument error to stderr, exit code 1.
- `buddd help` → prints usage information listing four commands (`run`, `edit`, `version`, `help`)
- Unknown command → prints `"Unknown command: '<cmd>'"` followed by usage to stderr, exits with code 1
- `buddd test` is **removed** — produces an unknown command error
- Old `--test` and `--version` flags are **dropped** — produce an unknown command error

**Note**: Old `demo` and `capture` subcommands are permanently removed per [ADR-014](/docs/adr/ADR-014-cli-app-system.md). Use `buddd run <scene>` with `--capture` instead.

## `buddd_editor` — Static library (`src/editor/`)

The editor library provides the interactive editor for `buddd edit`. It links `buddd_engine` (PUBLIC) and uses namespace `buddd::editor`. See [ADR-027](/docs/adr/ADR-027-editor-architecture.md) for the architectural rationale. No SDL3, OpenGL, or GLM headers are included from editor code (per ADR-019).

**F-01 (SDL3 native dialogs)**: OS file dialogs use SDL3 native APIs (`SDL_ShowOpenFileDialog` / `SDL_ShowSaveFileDialog`), abstracted through the `Platform` interface. The Platform defines a `FileDialogCallback` type alias so editor code never includes SDL3 headers (ADR-019). `ImGuiFileDialog` was removed from the build system (no longer fetched via `FetchContent` or compiled as part of `buddd_editor`). The simplified callback design uses heap-allocated `std::function` (deleted by the SDL C-lambda after invocation) — no mutex, no intermediate result queue. See [SPEC-F-01](/.specs/sprint-2026-06/editor-scene-load-save/spec.md) and [IMPL-F-01](/.specs/sprint-2026-06/editor-scene-load-save/implementation-contract.md).

### Command system (`src/editor/`)

| File | Role |
|---|---|
| `command.h` | `Command` abstract base class — pure virtual `execute()`, `undo()`, `name()` methods. All editor actions are modelled as Command subclasses. |
| `command.cpp` | Empty file for build consistency (all methods are pure virtual or defaulted). |
| `command_stack.h` | `CommandStack` class — bounded undo/redo stack (default 128, min 1). Methods: `execute(unique_ptr<Command>)`, `undo() -> bool`, `redo() -> bool`, `can_undo()`, `can_redo()`, `undo_name()`, `redo_name()`, `clear()`. |
| `command_stack.cpp` | Implementation: `execute()` calls `command->execute()`, pushes to undo stack, clears redo stack, enforces max_history bound. `undo()` pops from undo, calls `command->undo()`, pushes to redo. `redo()` pops from redo, calls `command->execute()`, pushes to undo. |

### Concrete commands (`src/editor/commands/`)

| File | Class | Role |
|---|---|---|
| `quit_command.h` | `QuitCommand` | Header-only. `execute()` calls `ctx.request_exit()`. `undo()` is a no-op (cannot un-request exit). `name()` returns `"Quit"`. |

### Shortcut registry (`src/editor/`)

| File | Role |
|---|---|
| `shortcut_registry.h` | `ShortcutRegistry` — header-only. Maps keyboard shortcuts (`KeyCode` + `Modifiers` struct) to `std::function<void()>` callbacks. `bind()` registers a binding. `process(InputSystem const&, bool want_capture)` iterates all bindings each frame. Action key checked via `is_pressed()` (edge-triggered, fires once per press). Modifiers checked via `is_down()` (both left/right variants). Gated by `WantCaptureKeyboard`. |

### Editor UI abstractions (`src/editor/`)

| File | Class | Role |
|---|---|---|
| `editor_menu.h` | `EditorMenu` | Abstract base class for overlay elements drawn before the ImGui dockspace. Pure virtual `id()`, optional `update(ctx)` and `draw_ui(ctx)` with default no-op. |
| `editor_panel.h` | `EditorPanel` | Abstract base class for dockable editor panels. Pure virtual `id()` and `title()`, optional `update(ctx)` and `draw_ui(ctx)` with default no-op. |

### Menu bar (`src/editor/panels/`)

| File | Class | Role |
|---|---|---|
| `menu_bar.h` | `MenuBar` | Header-only concrete `EditorMenu`. `id()` returns `"menu_bar"`. `draw_ui()` renders main menu bar via `ImGui::BeginMainMenuBar()`: **File** > New Scene (Ctrl+N), Open Scene (Ctrl+O), separator, Save Scene (Ctrl+S), Save Scene As (Ctrl+Shift+S), separator, Quit (Ctrl+Q), **Edit** > Undo (Ctrl+Z, disabled when undo stack empty) / Redo (Ctrl+Shift+Z/Ctrl+Y, disabled when redo stack empty), **Help** > About (opens modal popup via `set_on_about()` callback). **F-01**: Added callbacks `set_on_new_scene()`, `set_on_open_scene()`, `set_on_save_scene()`, `set_on_save_scene_as()`, `set_on_quit()`. The old direct `ctx.request_exit()` call for Quit is replaced by the `on_quit_` callback which checks dirty state first. Takes `CommandStack&`. |

### Concrete dockable panels (`src/editor/panels/`)

Five header-only `EditorPanel` subclasses, each with 100×100 minimum size constraint via `ImGui::SetNextWindowSizeConstraints()`. No functional content — placeholders for future features.

| File | Class | Id | Title |
|---|---|---|---|
| `scene_panel.h` | `ScenePanel` | `"scene"` | `"Scene"` |
| `properties_panel.h` | `PropertiesPanel` | `"properties"` | `"Properties"` |
| `console_panel.h` | `ConsolePanel` | `"console"` | `"Console"` |
| `project_panel.h` | `ProjectPanel` | `"project"` | `"Project"` |
| `assets_panel.h` | `AssetsPanel` | `"assets"` | `"Assets"` |

### Editor class (`src/editor/`)

| File | Role |
|---|---|
| `editor.h` | `Editor` class — central orchestrator. Lifecycle: `Editor()` → `setup(ctx)` → `update(ctx) x N` → `draw_ui(ctx) x N` → `shutdown()`. Public methods: `setup()` (registers menus/panels/shortcuts, sets `IniFilename`), `update()` (processes shortcuts via `ShortcutRegistry`, delegates to `menu->update()`/`panel->update()`), `add_menu()`/`add_panel()` (registration), `draw_ui()` (now 7-phase rendering), `shutdown()`, `world()` (returns `World&`). **F-01 additions**: Public scene management methods: `mark_dirty()`, `clear_dirty()`, `is_dirty()`, `current_file_path()`, `new_scene()`, `open_scene(path)`, `save_scene()`, `save_scene_as(path)`. Private state: `bool dirty_`, `std::optional<std::string> current_file_path_`, `PendingOp pending_op_`, `std::optional<std::string> pending_file_path_`, `std::string error_modal_title_`, `std::string error_modal_message_`, `bool show_error_modal_`, `bool request_exit_next_frame_`. Private methods: `update_window_title()`, `build_title_string()`, `draw_save_prompt_modal()`, `show_error_modal()`, `draw_error_modals()`, `draw_pending_op_modal()`, `execute_pending_op()`. **F-01 (SDL3 native dialogs)**: Removed `show_file_dialog_`, `file_dialog_action_`, `draw_file_dialog()`, `show_save_prompt_modal_`, `save_prompt_result_`, `handle_dirty_before_op()`. Added `request_exit_next_frame_` flag for Quit→Save As callback. No `#include <ImGuiFileDialog.h>` — dialog operations go through `Platform::show_open_file_dialog()` / `show_save_file_dialog()`. Enums: `SavePromptResult` (`Save`, `Discard`, `Cancel`), `PendingOp` (`None`, `NewScene`, `OpenScene`, `Quit`). |
| `editor.cpp` | Implementation. `setup()`: initialises window geometry cache from current window state (`cached_w_/cached_h_/cached_x_/cached_y_` — window position/size/state were already applied by `EditorApp` before this point), then creates `MenuBar` with all 6 callbacks, registers 5 panels, binds shortcuts, sets `ImGui::GetIO().IniFilename`, calls `update_window_title()`, and registers OS close-request handler. No `Window::resize()`, `Window::set_position()`, or `Window::set_state()` calls — geometry is applied earlier by the two-tier App flow. `update()`: processes shortcuts, **tracks last-known Normal geometry** (caches position/size when window state is Normal), delegates to menus/panels. `draw_ui()`: 7-phase rendering. `shutdown()`: saves cached window geometry (last-known Normal position/size, not current maximized/minimized values) to `user_project_settings`, then calls `save_all()`. |

### EditorApp (`src/cmd/apps/`)

| File | Role |
|---|---|
| `editor_app.h` | `EditorApp` — `App` subclass. Stores `unique_ptr<Editor>`. Constructor takes `std::optional<std::string> scene_path` (default `std::nullopt`). Declares `config()` (reads saved window geometry from YAML), `setup()` (re-validates position via Platform API), `update()`, `on_render()`, `shutdown()` overrides. |
| `editor_app.cpp` | Implementation: stores `scene_path_` from constructor. **`config()`**: reads saved `editor.window.*` values directly from `settings.yaml`, validates size (minimum 400×300) and state (minimized→normal), returns `AppConfig` with saved geometry (or defaults). **`setup()`**: re-validates window position against live `Platform::display_count()`/`display_bounds()` — if the saved position is off-screen, the window keeps its default centred position. Then creates `Editor` and calls `editor_->setup(ctx)`. If `scene_path_` has a value, loads the scene. `update()` calls `editor_->update(ctx)`. `on_render()` calls `editor_->draw_ui(ctx)`. `shutdown()` calls `editor_->shutdown()`. |

### App lifecycle extension (`src/cmd/`)

| File | Change |
|---|---|
| `app.h` | `App` base class gains `virtual auto update(EngineContext const& ctx) -> void {}` (default no-op). Lifecycle comment updated: `on_frame_begin() x N -> update() x N -> on_render() x N`. |
| `app.cpp` | `run_app()` render loop: `app.update(ctx)` called after `World::update_updatables(ctx)` and before `render_system->render_scene()`. |

### CLI integration

- `buddd edit [<scene>]` → if a YAML scene path is provided, creates `EditorApp{scene_path}` which loads the scene during `setup()`. Without a path, creates `EditorApp{std::nullopt}` (empty untitled editor — existing behaviour). Requires display; errors out in headless mode.
- The editor does NOT exit on Escape — only window close exits.
- Editor has a two-phase lifecycle: `update()` (logic: shortcuts, state) runs before `render_scene()`, `draw_ui()` (UI rendering) runs in `on_render()` after `render_scene()`.
- Docking layout persisted via `buddd_editor.ini` in the current working directory.

## `buddd_tests` — Test executable (`tests/`)

The unit test binary. Links `buddd_engine` (PRIVATE) and `Catch2::Catch2WithMain` (PRIVATE). Catch2 provides its own `main()` entry point.

| File | Role |
| |---|---|---|
| `version_tests.cpp` | Single Catch2 test: `"engine version is non-empty"` tagged `[sanity]` |
| `cmd_tests.cpp` | CLI command integration tests (tagged `[cli]`): argument parsing, error handling, default command, capture CLI tests — uses shared helpers from `test_helpers.h` |
| `demo_tests.cpp` | **No `[cli][demo]` subprocess tests** — removed as part of SPEC-016. File retained for future non-subprocess demo tests. Demo correctness verified via compilation and EngineService creation tests. |
| `platform_abstraction_tests.cpp` | Headless platform tests (T-01 through T-12), always compiled |
| `sdl3_backend_tests.cpp` | SDL3 backend tests (conditionally compiled with `BUDDD_HAS_DISPLAY=ON`) |
| `math_tests.cpp` | Math foundations tests (T-01 through T-71): Vec2, Vec3, Vec4, Mat4, Quat, view_matrix/look_at_rotation utilities, interop, and edge cases |
| `scene_graph_tests.cpp` | Scene graph tests (T-01 through T-49): EntityId, Transform, Component, Entity, World, hierarchy, deferred destruction, pending-destroy contract, and edge cases — all headless, compiled in both BUDDD_HAS_DISPLAY branches |
| `model_tests.cpp` | Model and cube tests (24 test cases: T-01 through T-24): Model factory methods, accessors, draw dispatch, move semantics, null model safety, cube data verification, shared material ownership, and demo loop simulation — all headless, compiled in both BUDDD_HAS_DISPLAY branches. Uses `EngineService::create()` instead of direct `RenderDeviceHeadless` construction. |
| `model_asset_tests.cpp` | Model asset tests (21+ test cases covering AC-005 through AC-028): ModelAsset loading via AssetManager, ModelNode hierarchy verification, PbrMaterial creation with textures, error cases (missing POSITION, corrupt glTF, missing file, type mismatch, unsupported version), vertex scale, missing texture fallback, Uint32 indices, node without mesh, unsupported primitive mode, COL0R_0 VEC3 expansion, normal default, hot-reload simulation, `create_model()` convenience, `replace_root()` privacy. All headless. |
| `image_tests.cpp` | Image unit tests (tagged `[image]`): ImageBuffer aggregate, Image::create validation, row-flipping, save/load round-trip, load error cases, copy/move semantics, accessors, save error cases. All headless (CPU-only). |
| `input_tests.cpp` | Input system tests: 9 headless tests (factory, headless defaults, double-buffered state model, KeyCode round-trip, edge cases) + 8 SDL3 tests (event processing integration, keyboard, mouse, wheel, accumulation, frame reset) — SDL3 tests conditional on `BUDDD_HAS_DISPLAY`. |
| `render_device_tests.cpp` | Render device tests: uses `EngineService::create()` instead of constructing `RenderDeviceHeadless` directly. Tests headless read_pixels error, navigable graph access (device.window().platform()), and diagnostic counters. |
| `scene_rendering_tests.cpp` | Scene rendering tests (AC-001 through AC-030): Component entity awareness, World::each<T>() iteration, camera registration lifecycle, CameraComponent auto-register/unregister, RenderSystem begin/end_frame, draw call counting, MVP computation, no-camera warning, uniform failure skip, cube-scene demo integration — all headless, compiled in both BUDDD_HAS_DISPLAY branches. Uses `EngineService::create()` for headless engine setup. |
| `texture_tests.cpp` | Texture unit tests (13 headless cases): Texture factory via `create_texture`, `set_texture`/`has_texture` on material, `bind()` deferred state application, headless texture data access, edge cases (null texture, zero dimensions, unknown texture name, empty data, unique_ptr→shared_ptr conversion), material with multiple textures. One OpenGL-only test (T-12) for `create_texture` — conditional on `BUDDD_HAS_DISPLAY`. |
| `lighting_tests.cpp` | Phong lighting tests (32 test cases, tagged `[lighting]`): Vertex struct layout, all three light component construction/accessors/on_attach, PhongMaterial creation and known uniforms, `glsl_util` extract/normalize, LightData struct, RenderSystem light collection (directional/point/spot), 8-light cap, color*intensity premultiply, normal matrix, backward compat, camera pos, material defaults, component destruction, zero-lights ambient, MaterialHeadless array subscript normalization and diagnostic accessors, spot cone uniforms, layout qualifier parsing — all headless. |
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
- Implementation contract: [IMPL-006](/.specs/sprint-2026-05/cli-command-system/implementation-contract.md) — File list, dispatch logic, CMake glob, ADR-019 compliance
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
- Spec: [SPEC-023](/.specs/sprint-2026-06/developer-assertions/spec.md) — Developer Assertions (five macros, Fatal level, debug break, NDEBUG-only build detection)
- ADR: [ADR-018](/docs/adr/ADR-018-tinygltf-dependency.md) — tinygltf dependency for glTF 2.0 model loading
- ADR: [ADR-012](/docs/adr/ADR-012-navigable-object-graph-engine-service.md) — Navigable Object Graph, EngineService, and Abstract Interface Extensions
- ADR: [ADR-014](/docs/adr/ADR-014-cli-app-system.md) — CLI App System: centralised render loop with App lifecycle, unified `run` command (partially supersedes ADR-004)
- ADR: [ADR-021](/docs/adr/ADR-021-developer-assertions.md) — Developer Assertions (Fatal level, five macros, debug break, fixed Assert tag)
- ADR: [ADR-023](/docs/adr/ADR-023-updatable-components.md) — Updatable Components & EngineContext (Updatable interface, EngineContext struct, World auto-registration, App::setup(EngineService&), run_app auto-dispatch)
- Spec: [SPEC-036](/.specs/sprint-2026-06/settings-system/spec.md) — Settings System (three tiers, TypeRegistry integration, YAML storage, observer pattern)
