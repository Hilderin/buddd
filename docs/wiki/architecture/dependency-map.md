# Dependency Map

## Target dependencies

```
buddd ──PRIVATE──► buddd_engine ──PUBLIC──► SDL3::SDL3
                       │                    ├──► OpenGL::GL
                       │                    ├──► glm::glm
                       │                    └──► stb (PRIVATE, FetchContent)
                       │
buddd_tests ──PRIVATE─┤
               ──PRIVATE──► Catch2::Catch2WithMain (external)
                           │
buddd_editor ──PUBLIC──► buddd_engine
```

| Source target | Dependency | Link type | Notes |
|---|---|---|---|
| `buddd` | `buddd_engine` | PRIVATE | CLI needs the engine's version and platform APIs |
| `buddd_editor` | `buddd_engine` | PUBLIC | Editor library: Editor class, EditorApp, and editor infrastructure |
| `buddd_tests` | `buddd_engine` | PRIVATE | Tests exercise engine library code |
| `buddd_tests` | `Catch2::Catch2WithMain` | PRIVATE | Catch2 provides `main()` and test runner |
| `buddd_engine` | `SDL3::SDL3` | PUBLIC | SDL3 library for windowing, input, and GL context management |
| `buddd_engine` | `OpenGL::GL` | PUBLIC | OpenGL rendering (4.5 Core profile) |
| `buddd_engine` | `glm::glm` | PUBLIC | GLM header-only math library — provides underlying implementation for Vec2, Vec3, Vec4, Mat4, Quat wrappers |
| `buddd_engine` | `stb` | PRIVATE | Single-header public-domain library for PNG I/O (stb_image + stb_image_write). Fetched via FetchContent. |
| `buddd_engine` | `imgui` | PRIVATE | Dear ImGui (docking branch `v1.91.8-docking`) — immediate-mode GUI library. Fetched via FetchContent. Compiled as part of `buddd_engine` via the glob in `src/engine/imgui/CMakeLists.txt`. |

## External dependencies

| Dependency | Version | Source | Fetch method |
|---|---|---|---|
| **Catch2** | v3.7.0 | `https://github.com/catchorg/Catch2.git` | CMake `FetchContent` (automatic download at configure time) |
| **SDL3** | release-3.2.30 | `https://github.com/libsdl-org/SDL.git` | CMake `FetchContent` (automatic download at configure time) |
| **GLM** | 1.0.1 | `https://github.com/g-truc/glm.git` | CMake `FetchContent` (automatic download at configure time) — header-only, no compiled library |
| **OpenGL** | 4.5 Core | System-provided (`libgl-dev` or equivalent) | `find_package(OpenGL REQUIRED)` |
| **stb** | `31c1ad37456438565541f9958214b6e762fb4` | `https://github.com/nothings/stb.git` | CMake `FetchContent` (automatic download at configure time) — header-only, only `#include` path needed. |
| **Dear ImGui** | `v1.91.8-docking` (docking branch) | `https://github.com/ocornut/imgui.git` | CMake `FetchContent` (automatic download at configure time) — compiled library sources embedded in `src/engine/imgui/` alongside wrapper module.

- Catch2, SDL3, and GLM are fetched once and cached in the build directory; subsequent configures use the cached copy.
- Compiled dependencies that are large or slow to debug (notably SDL3) are built with `-DCMAKE_BUILD_TYPE=Release` via `CMAKE_ARGS` in their `FetchContent_Declare` to avoid debugger startup slowness from their debug symbols. Header-only dependencies (GLM) are unaffected. See `src/engine/CMakeLists.txt` for the SDL3 declaration, [ADR-007](/docs/adr/ADR-007-release-dependency-build.md) for the full rationale, and the [setup guide](/docs/wiki/engineering/setup.md) for details.
- No network access is required after the initial fetch of each dependency.
- SDL3 is linked to `buddd_engine` as PUBLIC so that all consumers of the engine have access to SDL3 headers (only within `src/engine/` — external consumers must not include them directly).
- The headless backend of the platform layer has **zero** external dependencies — it uses only the C++ standard library and is always compiled.

## Build toolchain dependencies

| Tool | Minimum version | Required for |
|---|---|---|
| CMake | 3.28 | Build configuration, presets, FetchContent |
| Ninja | 1.11 | Build execution |
| C++ compiler (GCC 14+ / Clang 19+) | C++26 support | Compilation |
| clang-format | 18 (optional) | `cmake --build ... --target format` |

## System dependencies for OpenGL

The project requires OpenGL 4.5 Core profile headers at build time. On Linux these are typically provided by `libgl-dev` or `mesa-common-dev` packages. The build will fail at configure time if OpenGL is not found.

## Navigable object graph

As of SPEC-016 / ADR-012, the engine establishes a navigable object graph through non-owning back-references:

```
RenderDevice  ──>  Window  ──>  Platform  ──>  InputSystem
```

- `Window` stores a non-owning `Platform&` reference (forward declaration of `Platform` in `window.h`). **New module dependency**: `window/` → `platform/`.
- `RenderDevice` declares a pure virtual `window() -> Window&` accessor. **New module dependency**: `render/` → `window/`.
- From any `RenderDevice&`, the entire upstream graph is reachable: `device.window().platform().input_system()`.
- All back-references are non-owning (`T&`), compliant with ADR-010.
- Lifecycle invariants: `Platform` outlives `Window` outlives `RenderDevice`.

## EngineService

`EngineService` (in `src/engine/engine_service.h/.cpp`) is the lifecycle owner of the component chain. Created via the factory method `EngineService::create(Backend, WindowConfig)`, it owns `Platform` → `Window` → `RenderDevice` via `std::unique_ptr`. Used by both tests and production code (`run_app()`). Member declaration order (`platform_`, `window_`, `device_`) guarantees correct destruction ordering (RenderDevice first, then Window, then Platform).

## Texture data dependency

`RenderDevice::create_texture(const Image&)` introduces a dependency from `render/` → `image/`:

- `RenderDeviceOpenGL::create_texture()` calls `glCreateTextures`/`glTextureStorage2D`/`glTextureSubImage2D` using image width, height, channels, and RGBA pixel data from the `Image` object.
- `RenderDeviceHeadless::create_texture()` copies pixel data into an in-memory `TextureHeadless`.
- `Texture` is backend-agnostic: `TextureOpenGL` and `TextureHeadless` are private headers inside `src/engine/render/`, maintaining the architecture boundary.

## Key constraints

- The engine is a **static library** (`STATIC`), not header-only. This may change in the future.
- The editor target is a **static library** that links `buddd_engine` (PUBLIC) — it is no longer a placeholder.
- Catch2 is **not** a dependency of the engine or the CLI — only of the test binary.
- The headless backend has **zero** external dependencies and is always compiled alongside the SDL3+OpenGL backend.
- `buddd_engine` links `SDL3::SDL3`, `OpenGL::GL`, and `glm::glm` as **PUBLIC** so that consumers inheriting the include paths can use SDL3, OpenGL, and GLM types **inside** `src/engine/` only.
- GLM is header-only — no compiled library, no system dependency. GLM types are never to be included directly outside `src/engine/math/` (enforced by code review).
- stb is a **PRIVATE** dependency of `buddd_engine`, included only in `src/engine/image/`. It is not exposed outside the engine and is not accessible to consumers of the engine library.

## Architecture boundary

A hard architecture boundary is enforced by convention: **no code outside `src/engine/`** may `#include <SDL3/`, `<GL/`, `<glad/`, or any graphics-library header. Similarly, **no code outside `src/engine/math/`** may include any `glm/` header directly — all math access goes through the wrapper types (`Vec2`, `Vec3`, `Vec4`, `Mat4`, `Quat`). Violations are caught by code review (automated enforcement is a future goal).

The GLM boundary specifically:
- GLM headers may be included inside `src/engine/math/` (the wrapper headers and `math.h`).
- Outside `src/engine/math/`, all math operations go through the wrapper types — the `.glm()` accessor is the sole interop path.
- Test files comparing against GLM reference output include GLM headers directly; this is acknowledged as a design tension but accepted at this stage (no automated guard).

## SceneLoader dependencies

The `SceneLoader` class (in `src/engine/scene/scene_loader.h/.cpp`) introduces the following internal dependencies:

- `SceneLoader` stores a `World&` reference — it depends on `scene/` (World, Entity, Transform, Component).
- `SceneLoader` stores a `ComponentRegistry&` reference — it depends on `component_registry/` (for `registry_.describe()`, `registry_.create()`, and `deserialize_component()`).
- `SceneLoader` stores an `AssetManager&` reference — it depends on `asset/` (for `assets_.base_path()` and constructing `SerializationContext`).
- `SceneLoader` uses yaml-cpp directly (`YAML::LoadFile()`, `YAML::Node`) — inherits the existing `buddd_engine` dependency on yaml-cpp (already fetched and available via component_registry).

```
scene_loader ──► scene/ (World, Entity, Transform, Component)
scene_loader ──► component_registry/ (ComponentRegistry, deserialize_component, SerializationContext)
scene_loader ──► asset/ (AssetManager::base_path)
```

The `SceneLoader` is auto-globbed by the existing `buddd_engine` CMakeLists.txt — no new CMake target or dependency is needed.

## Component Registry dependencies

The `component_registry/` submodule (in `src/engine/scene/component_registry/`) introduces the following internal dependencies:

- `TypeRegistry` is a static class — no instance dependency. It stores a `std::unordered_map<std::type_index, TypeEntry>` mapping C++ types to their five callbacks. It depends on `yaml-cpp` (for `YAML::Node` in encode/decode) and `error.h`/`Result<T>` (for decode/string/validate return types).
- `ComponentRegistry` owns a `std::unordered_map<std::string, std::unique_ptr<ComponentInfoBase>>`. It depends on `ComponentInfoBase`/`ComponentInfo<T>` (which depend on `Property`, `SerializationContext`, `TypeRegistry`, and `Component`).
- `Property` (internal) stores type-erased getter/setter callbacks and delegates serialization/validation to `TypeRegistry`. It forward-declares `YAML::Node` — no yaml-cpp include in the header (only in `.cpp` and `component_info.h`).
- `SerializationContext` is a lightweight struct holding an `AssetManager&` reference, creating a dependency from `component_registry/` → `asset/`.
- `serialize_component()` / `deserialize_component()` free functions produce/consume `YAML::Node` and depend on `yaml-cpp` and `ComponentInfoBase`.
- `register_all_components()` depends on all five engine component headers (`camera_component.h`, `point_light_component.h`, `directional_light_component.h`, `spot_light_component.h`, `mesh_renderer.h`).

```
component_registry/
├── type_registry ──► yaml-cpp, error.h
├── property ──► TypeRegistry (static), error.h
├── component_info ──► Property, SerializationContext, TypeRegistry, Component, yaml-cpp
├── component_registry ──► ComponentInfoBase, error.h
├── serialization_context ──► AssetManager (forward decl, reference)
├── serialization ──► ComponentInfoBase, yaml-cpp (forward decl)
└── register_all_components ──► ComponentRegistry, all component headers
```

Note: `component_info.h` includes `<yaml-cpp/yaml.h>` directly because its template function bodies require the complete `YAML::Node` type. This is a necessary deviation from the general yaml-cpp convention (headers only in `.cpp` files) for template inline definitions.

## Reference

- Spec: [SPEC-001](/.specs/sprint-2026-05/project-setup/spec.md) — Assumptions A-05 through A-10
- Implementation contract: [IMPL-001](/.specs/sprint-2026-05/project-setup/implementation-contract.md) — sections 3-10 (target definitions)
- Spec: [SPEC-002](/.specs/sprint-2026-05/platform-abstraction/spec.md) — Architecture boundary, Goals, Assumptions
- Implementation contract: [IMPL-002](/.specs/sprint-2026-05/platform-abstraction/implementation-contract.md) — CMakeLists.txt requirements, Done criteria
- Spec: [SPEC-004](/.specs/sprint-2026-05/math-foundations/spec.md) — Architecture boundary (no GLM outside `src/engine/math/`), GLM integration
- Implementation contract: [IMPL-004](/.specs/sprint-2026-05/math-foundations/implementation-contract.md) — Files allowed to change, Architecture boundary enforcement
- Spec: [SPEC-010](/.specs/sprint-2026-05/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario)
- Implementation contract: [IMPL-010](/.specs/sprint-2026-05/capture/implementation-contract.md)
