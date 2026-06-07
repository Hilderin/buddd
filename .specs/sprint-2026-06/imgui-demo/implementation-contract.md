# IMPL-026 — EngineImGui Module: Dear ImGui Integration

## Source spec

`SPEC-026` at `.specs/sprint-2026-06/imgui-demo/spec.md`

## Goal

Embed Dear ImGui (docking branch) as an engine-internal module at `src/engine/imgui/` that provides a minimal public API (`init`, `shutdown`, `new_frame`, `render`, `on_sdl_event`, `is_initialized`) and integrates ImGui's frame lifecycle and event routing automatically into the engine's existing `RenderDevice` / `Platform` / `run_app()` architecture — with zero ImGui-specific code in `run_app()` or any `App` base class. Deliver a demo app (`ImguiDemoApp`) registered as the `"imgui-demo"` scene to verify the integration end-to-end.

## Non-goals

- No ImGui API wrapper or abstraction layer — apps include `<imgui.h>` and use Dear ImGui's API directly.
- No multi-viewport support.
- No ImGui input capture API or event filtering — InputSystem continues to see all events.
- No custom ImGui backends — embed unmodified official `imgui_impl_sdl3` and `imgui_impl_opengl3` backends.
- No headless ImGui integration — init is skipped when no display; lifecycle methods are no-ops.
- No changes to the `App` base class virtual interface.
- No changes to existing tests.
- No font/INI management — apps control `io.IniFilename` and fonts in their `setup()`.
- No runtime toggling of ImGui visibility.
- No standalone binary for the demo — scene-only via `buddd run imgui-demo`.
- No changes to `src/cmd/CMakeLists.txt` — the existing `file(GLOB_RECURSE ...)` picks up new `.cpp` files under `src/cmd/apps/` automatically.
- No `src/engine/imgui/` directory pre-populated with ImGui source files — ImGui sources are fetched via `FetchContent` from GitHub and referenced from the build tree.

## Relevant ADRs

- **ADR-012** (Navigable Object Graph / EngineService) — Shutdown of `engine_imgui` must happen before the GL context is destroyed. `~RenderDeviceOpenGL()` is the integration point.
- **ADR-019** (Architecture Boundaries / CONST-001) — No SDL3, OpenGL, or ImGui headers may be included from `src/cmd/`. `engine_imgui.h` forward-declares `SDL_Window` and `SDL_Event` without including SDL3 headers. The demo app must not include any SDL3, OpenGL, or ImGui backend headers — only `<imgui.h>` for the public API.
- **ADR-014** (CLI App System) — Demo is a scene in `main.cpp` dispatch, not a standalone binary.
- **ADR-001** (Result/Error Pattern) — `engine_imgui::init()` returns `Result<void>`, using the existing `Error` struct and `Error::Category` values.

## Files to inspect

These files must be read by the Code Agent before making any modifications:

| File | Why |
|---|---|
| `src/engine/render/render_device.h` | Abstract base class — add `virtual auto render_ui() -> void {}` (non-pure, default no-op). |
| `src/engine/render/render_device_opengl.h` | Concrete OpenGL backend — declare `render_ui()` override. |
| `src/engine/render/render_device_opengl.cpp` | Add `engine_imgui::new_frame()` to `begin_frame()`, add `engine_imgui::render()` to `render_ui()`, add `engine_imgui::shutdown()` to destructor. |
| `src/engine/render/render_device.cpp` | Add `engine_imgui::init()` call inside `RenderDevice::create()` after successful `RenderDeviceOpenGL` construction. |
| `src/engine/render/render_device_headless.h` | No changes needed — inherits the no-op `render_ui()` default. |
| `src/engine/platform/platform_sdl3.cpp` | Add `engine_imgui::on_sdl_event()` in the `SDL_PollEvent` loop. |
| `src/engine/engine_service.h` | No changes — just inspect for shutdown order understanding. |
| `src/engine/engine_service.cpp` | No changes — `EngineService::~EngineService()` stays `= default`; shutdown is in `~RenderDeviceOpenGL()`. |
| `src/cmd/app.cpp` | Add `device.render_ui()` call before capture/`read_pixels`. |
| `src/cmd/main.cpp` | Add `"imgui-demo"` scene dispatch. |
| `src/engine/CMakeLists.txt` | Add `FetchContent` for ImGui (docking branch, `v1.91.8-docking`), `add_subdirectory(imgui)` after `add_library(buddd_engine ...)`. |
| `src/cmd/apps/phong_app.h` / `.cpp` | Reference for App subclass pattern — `ImguiDemoApp` follows the same structure. |
| `src/cmd/app.h` | App base class — no changes, just inspect to confirm no ImGui additions needed. |

## Files allowed to change

| File | Change purpose |
|---|---|
| `src/engine/render/render_device.h` | Add `virtual auto render_ui() -> void {}` (non-pure, default no-op body). |
| `src/engine/render/render_device_opengl.h` | Declare `auto render_ui() -> void override`. |
| `src/engine/render/render_device_opengl.cpp` | (1) Add `#include "imgui/engine_imgui.h"`. (2) Call `engine_imgui::new_frame()` in `begin_frame()` after glClear. (3) Implement `render_ui()` calling `engine_imgui::render()`. (4) Call `engine_imgui::shutdown()` in destructor BEFORE `SDL_GL_DestroyContext`. |
| `src/engine/render/render_device.cpp` | (1) Add `#include "imgui/engine_imgui.h"`. (2) After successful `new RenderDeviceOpenGL(...)`, before `return`, call `engine_imgui::init(sdl_window, gl_context)` — but only when a display is available (the OpenGL branch, not headless). On failure, log warning and continue. |
| `src/engine/platform/platform_sdl3.cpp` | (1) Add `#include "imgui/engine_imgui.h"`. (2) Inside the `while (SDL_PollEvent(&event))` loop, after `input_system_.on_sdl_event(event)`, add `engine_imgui::on_sdl_event(event);`. |
| `src/engine/CMakeLists.txt` | (1) Add `FetchContent_Declare` and `FetchContent_MakeAvailable` for ImGui (docking branch, tag `v1.91.8-docking`). (2) After `add_library(buddd_engine ...)`, add `add_subdirectory(imgui)`. |
| `src/cmd/app.cpp` | After `app.on_render(ctx)` and before the capture/`read_pixels` block, add `eng.device().render_ui();`. No new includes needed. |
| `src/cmd/main.cpp` | (1) Add `#include "apps/imgui_demo_app.h"`. (2) Add an else-if in the scene dispatch: `else if (scene == "imgui-demo") app = std::make_unique<bc::app::ImguiDemoApp>();`. (3) Add `"imgui-demo"` to the usage help text in the Unknown scene error block. |

## Files forbidden to change

| File | Reason |
|---|---|
| `src/engine/engine_service.h` | No new methods needed. |
| `src/engine/engine_service.cpp` | Shutdown is in `~RenderDeviceOpenGL()`, not here. No changes. |
| `src/engine/render/render_device_headless.h` | Inherits the no-op default `render_ui()`. No override needed. |
| `src/engine/render/render_device_headless.cpp` | No changes. |
| `src/cmd/app.h` | App base class must not gain ImGui-specific methods. |
| `src/cmd/CMakeLists.txt` | Existing glob picks up new `.cpp` files automatically. No changes needed. |
| `src/engine/log/log.h` | No logging API changes. |
| `src/engine/error.h` | No new error categories needed — use existing `InitFailed` and `InvalidArgument`. |
| `src/engine/render/render_system.cpp` | No changes — ImGui rendering is separate from scene rendering. |

## Existing conventions to follow

1. **Namespace**: Engine code in `buddd::engine`, CLI apps in `buddd::cmd::app`. The `engine_imgui` namespace is `buddd::engine::engine_imgui`.
2. **Logging**: Use `BUDDD_LOG_TAG("ImGui")` in `engine_imgui.cpp`. Use `BUDDD_LOG_INFO`, `BUDDD_LOG_ERROR`, `BUDDD_LOG_TRACE` macros.
3. **Result type**: `Result<T>` = `std::expected<T, Error>` from `error.h`. Use `make_error(Error::Category::..., "...")` for errors.
4. **Header guards**: `#pragma once` (not `#ifndef`).
5. **Forward declarations**: Use forward declarations instead of includes in public headers where possible. `engine_imgui.h` forward-declares `SDL_Window` and `SDL_Event`.
6. **App lifecycle pattern**: `config()` returns `AppConfig`, `setup()` returns `Result<void>`, `on_frame_begin()` and `on_render()` take `EngineContext const&`, `shutdown()` is void.
7. **Naming**: `snake_case` for functions and variables, `PascalCase` for classes/structs.
8. **App file placement**: Demo app files go in `src/cmd/apps/` with `_app` suffix (e.g., `imgui_demo_app.h`, `imgui_demo_app.cpp`).
9. **Destruction order in RenderDeviceOpenGL**: `~RenderDeviceOpenGL()` calls `engine_imgui::shutdown()` before `SDL_GL_DestroyContext(context_)` — ImGui needs the GL context to still be current when cleaning up its OpenGL resources.

## Required implementation behavior

### 1. `engine_imgui.h` — Public API header

**File**: `src/engine/imgui/engine_imgui.h` (new, create directory `src/engine/imgui/`)

```cpp
#pragma once

#include "error.h"

#include <cstdint>

// Forward declarations only — do NOT include SDL3 headers in public API
struct SDL_Window;
union SDL_Event;

namespace buddd::engine::engine_imgui {

/// Initialise ImGui context and both backends (SDL3 + OpenGL3).
/// Must be called after SDL_GL_MakeCurrent and before any ImGui calls.
/// Must be called only when a display is available.
/// Returns error if init fails.
[[nodiscard]] auto init(SDL_Window* window, void* gl_context) -> Result<void>;

/// Shut down ImGui backends and destroy context.
/// Safe to call even if init() was not called (no-op).
auto shutdown() -> void;

/// Begin a new ImGui frame. Must be called once per frame before
/// app.on_frame_begin(). No-op if not initialised.
auto new_frame() -> void;

/// Render ImGui draw data. Must be called once per frame after
/// app.on_render() and before end_frame(). No-op if not initialised.
auto render() -> void;

/// Forward an SDL event to ImGui_ImplSDL3_ProcessEvent().
/// Returns true if ImGui consumed the event.
/// No-op if not initialised — returns false.
[[nodiscard]] auto on_sdl_event(const SDL_Event& event) -> bool;

/// Returns true if ImGui was successfully initialised.
[[nodiscard]] auto is_initialized() -> bool;

} // namespace buddd::engine::engine_imgui
```

### 2. `engine_imgui.cpp` — Implementation

**File**: `src/engine/imgui/engine_imgui.cpp` (new)

**Internal state** (file-scope `static` variables):

```cpp
static ImGuiContext* s_context = nullptr;
static bool s_initialized = false;
```

**`init(SDL_Window* window, void* gl_context) -> Result<void>`**:

1. If `s_initialized`, return `make_error(Error::Category::InitFailed, "ImGui already initialised")`.
2. If `window == nullptr`, return `make_error(Error::Category::InvalidArgument, "SDL_Window cannot be null")`.
3. Call `IMGUI_CHECKVERSION()` macro.
4. `s_context = ImGui::CreateContext()`; if null, return error.
5. `ImGui::SetCurrentContext(s_context)`.
6. `ImGui_ImplSDL3_InitForOpenGL(window, gl_context)` — if returns false, call `ImGui::DestroyContext(s_context)`, set `s_context = nullptr`, return `make_error(Error::Category::InitFailed, "ImGui_ImplSDL3_InitForOpenGL failed")`.
7. `ImGui_ImplOpenGL3_Init("#version 410 core")` — if returns false, call `ImGui_ImplSDL3_Shutdown()`, `ImGui::DestroyContext(s_context)`, set `s_context = nullptr`, return `make_error(Error::Category::InitFailed, "ImGui_ImplOpenGL3_Init failed")`.
8. `s_initialized = true`.
9. `BUDDD_LOG_INFO("ImGui: initialised (backend SDL3+OpenGL3)")`.
10. Return `Result<void>{}` (success).

**`shutdown() -> void`**:

1. If not `s_initialized`, return (no-op).
2. `ImGui_ImplOpenGL3_Shutdown()`.
3. `ImGui_ImplSDL3_Shutdown()`.
4. `ImGui::DestroyContext(s_context)`.
5. `s_context = nullptr`.
6. `s_initialized = false`.
7. `BUDDD_LOG_INFO("ImGui: shutdown")`.

**`new_frame() -> void`**:

1. If not `s_initialized`, `BUDDD_LOG_TRACE("ImGui: skipped (not initialised)")`; return.
2. `ImGui_ImplOpenGL3_NewFrame()`.
3. `ImGui_ImplSDL3_NewFrame()`.
4. `ImGui::NewFrame()`.

**`render() -> void`**:

1. If not `s_initialized`, `BUDDD_LOG_TRACE("ImGui: skipped (not initialised)")`; return.
2. `ImGui::Render()`.
3. If `ImGui::GetDrawData() != nullptr` → `ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())`.

**`on_sdl_event(const SDL_Event& event) -> bool`**:

1. If not `s_initialized`, return `false`.
2. Return `ImGui_ImplSDL3_ProcessEvent(&event)` (the backend function returns `bool`).

**`is_initialized() -> bool`**:

1. Return `s_initialized`.

### 3. `RenderDevice::render_ui()` — base class default

**In `src/engine/render/render_device.h`**, add after the `read_pixels()` declaration (line 96):

```cpp
/// Render any active UI overlay (ImGui).
/// Default no-op — overridden by OpenGL backend to call engine_imgui::render().
virtual auto render_ui() -> void {}
```

### 4. `RenderDeviceOpenGL` changes

**In `src/engine/render/render_device_opengl.h`**, add after `read_pixels()` declaration (line 69):

```cpp
auto render_ui() -> void override;
```

**In `src/engine/render/render_device_opengl.cpp`**:

- Add `#include "imgui/engine_imgui.h"` near the top (after `#include "debug/assert.h"`, before `BUDDD_LOG_TAG`).

- **`begin_frame()`**: After the `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)` call on line 121, add:
  ```cpp
  engine_imgui::new_frame();
  ```

- **New `render_ui()` method** (add after `begin_frame()` or before `end_frame()`):
  ```cpp
  auto RenderDeviceOpenGL::render_ui() -> void {
      engine_imgui::render();
  }
  ```

- **`~RenderDeviceOpenGL()`**: Before `SDL_GL_DestroyContext(context_);` on line 113, add:
  ```cpp
  engine_imgui::shutdown();
  ```

### 5. `RenderDevice::create()` — init integration

**In `src/engine/render/render_device.cpp`**:

- Add `#include "imgui/engine_imgui.h"` (after `#include "window/window.h"`).

- In the OpenGL backend branch (after `SDL_GL_MakeCurrent(sdl_window, gl_context)` and before the `return` on line 46), add:
  ```cpp
  // Initialise ImGui after the GL context is current
  auto imgui_result = engine_imgui::init(sdl_window, gl_context);
  if (!imgui_result) {
      BUDDD_LOG_WARN("ImGui init failed (non-fatal): {}", to_string(imgui_result.error()));
  }
  ```

This is inside the `if (native != nullptr)` branch — headless path never reaches this code.

### 6. PlatformSDL3 — event routing

**In `src/engine/platform/platform_sdl3.cpp`**:

- Add `#include "imgui/engine_imgui.h"`.

- In the `while (SDL_PollEvent(&event))` loop (around lines 32-38), after `input_system_.on_sdl_event(event);`, add:
  ```cpp
  engine_imgui::on_sdl_event(event);
  ```
  This line is UNCONDITIONAL — it runs for every non-quit event regardless of whether ImGui consumed it. The return value is intentionally ignored; InputSystem always sees all events.

### 7. `run_app()` — render_ui() call

**In `src/cmd/app.cpp`**:

- After `app.on_render(ctx);` (line 130) and before the capture block (line 132), add:
  ```cpp
  // Render any active UI overlay (ImGui)
  eng.device().render_ui();
  ```

No new includes needed — `render_device.h` is already included, and `render_ui()` is on `RenderDevice`.

### 8. Demo app — `ImguiDemoApp`

**File**: `src/cmd/apps/imgui_demo_app.h` (new)

```cpp
#pragma once

#include "app.h"

#include "scene/entity.h"

#include <chrono>

namespace buddd::cmd::app {

class ImguiDemoApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine \u2014 ImGui Demo", 1280, 720};
    }

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_frame_begin(buddd::engine::EngineContext const& ctx) -> void override;

    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

private:
    bool show_demo_window_ = true;
};

} // namespace buddd::cmd::app
```

**File**: `src/cmd/apps/imgui_demo_app.cpp` (new)

```cpp
#include "apps/imgui_demo_app.h"

#include <imgui.h>

BUDDD_LOG_TAG("ImGuiDemo");

namespace be = buddd::engine;

auto buddd::cmd::app::ImguiDemoApp::setup(be::EngineContext const& /*ctx*/)
    -> be::Result<void>
{
    // Disable ImGui ini file persistence
    ImGui::GetIO().IniFilename = nullptr;
    return {};
}

auto buddd::cmd::app::ImguiDemoApp::on_frame_begin(be::EngineContext const& /*ctx*/) -> void {
    // No 3D scene to animate
}

auto buddd::cmd::app::ImguiDemoApp::on_render(be::EngineContext const& /*ctx*/) -> void {
    // Show the Dear ImGui Demo window
    if (show_demo_window_) {
        ImGui::ShowDemoWindow();
    }

    // Custom info panel
    ImGui::Begin("ImGui Demo");
    ImGui::Checkbox("Show Demo Window", &show_demo_window_);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}
```

### 9. `main.cpp` — scene dispatch

**In `src/cmd/main.cpp`**:

- Add `#include "apps/imgui_demo_app.h"` in the include block (line 4-17 area, alphabetically after `hot_reload_gltf_app.h`).
- Add an else-if in the scene dispatch chain (after the `hot-reload-gltf` branch around line 107):
  ```cpp
  else if (scene == "imgui-demo")
      app = std::make_unique<bc::app::ImguiDemoApp>();
  ```
- In the usage help text (around lines 113-126), add a line for `imgui-demo`:
  ```cpp
  "  imgui-demo   ImGui integration demo: ShowDemoWindow + custom panel (120 frames)\n"
  ```

### 10. Build system — `src/engine/CMakeLists.txt`

Add the following BEFORE the existing `find_package(OpenGL REQUIRED)` line:

```cmake
# ----- Dear ImGui (immediate-mode GUI library) -----
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8-docking
)
FetchContent_MakeAvailable(imgui)
```

After `add_library(buddd_engine STATIC ${ENGINE_SOURCES})` and before `target_compile_definitions(...)`, add:

```cmake
add_subdirectory(imgui)
```

### 11. Build system — `src/engine/imgui/CMakeLists.txt`

**File**: `src/engine/imgui/CMakeLists.txt` (new)

```cmake
# Add ImGui library sources and backend sources to the engine target.
# These reside in the FetchContent download directory (${imgui_SOURCE_DIR}),
# not in the source tree.
target_sources(buddd_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/engine_imgui.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(buddd_engine PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
```

## Required tests

### Unit tests

No new unit tests are required for this contract. The existing test infrastructure does not exercise display-dependent code paths, and ImGui requires a GL context. The integration is verified through acceptance criteria (see below).

### E2E / Integration verification

These command-based verifications trace to spec acceptance criteria. Run each after the implementation is complete:

| Command | ACs verified |
|---|---|
| `buddd run imgui-demo --frame 10` | AC-016, AC-017, AC-020, AC-022 — runs 10 frames, shows ImGui windows and custom panel, exits with code 0. |
| `buddd run imgui-demo --frame 120 --capture 120:/tmp/imgui_capture.png` | AC-018, AC-021 — PNG file created with valid header, showing ImGui overlay on solid-color background. |
| `buddd run cube --frame 10 --capture 10:/tmp/cube_before.png` then `buddd run cube --frame 10 --capture 10:/tmp/cube_after.png` | SC-003 — pixel-identical output (pre-ImGui regression check). |
| `cmake -B build/headless -DBUDDD_HAS_DISPLAY=OFF && cmake --build build/headless` | AC-023 — headless build succeeds. |

## Edge cases

| Case | Expected behavior | Where enforced |
|---|---|---|
| Headless mode (no display) | `engine_imgui::init()` is never called (RenderDevice::create() returns Headless before reaching the init call). `is_initialized()` is false. All lifecycle functions are no-ops. | `render_device.cpp` — init only in OpenGL branch |
| Double init | Second call returns `Error::Category::InitFailed`. State unchanged. | `engine_imgui::init()` — early check |
| Double shutdown | Second call is no-op. | `engine_imgui::shutdown()` — early check |
| ImGui backend init failure | `init()` returns error, logs warning, engine continues without ImGui. `is_initialized()` is false. Backend shutdown handled before returning error. | `engine_imgui::init()` steps 6-7 |
| ImGui functions called before init | Undefined by ImGui — but `new_frame()`/`render()` check `s_initialized` and are no-ops before init. | `engine_imgui::new_frame()`/`render()` |
| SDL_Window* null | `init()` returns `Error::Category::InvalidArgument`. | `engine_imgui::init()` step 2 |
| ImGui draw data null during render | `render()` checks `GetDrawData()` before calling `RenderDrawData`. | `engine_imgui::render()` step 3 |
| Event routing for unrecognized SDL events | `ImGui_ImplSDL3_ProcessEvent()` returns false. `on_sdl_event()` returns false. | ImGui backend behavior |
| SDL_EVENT_QUIT during event loop | Quit event is handled by the existing `return false` path in `poll_events()` — ImGui never sees it because the event is returned before reaching the ImGui routing line. | `platform_sdl3.cpp` event loop |
| Window is minimised / zero-size | ImGui handles internally (fb_width/fb_height > 0 check in render). | ImGui backend |
| ImGui ini file persistence | Demo app sets `io.IniFilename = nullptr` in `setup()`. Engine does not manage ini files. | `imgui_demo_app.cpp` setup |
| CONST-001 violation (SDL3/OpenGL headers in src/cmd/) | `engine_imgui.h` uses forward declarations only. Demo app includes only `<imgui.h>`. Neither includes SDL3 or OpenGL headers. | Verify with grep |
| `RenderDeviceHeadless` does not implement `render_ui()` | Inherits the no-op default from `RenderDevice` base class. Correct. | `render_device.h` default body `{}` |
| Shutdown while GL context is still current | `~RenderDeviceOpenGL()` calls `engine_imgui::shutdown()` before `SDL_GL_DestroyContext()`, ensuring GL context is valid during ImGui backend cleanup. | `render_device_opengl.cpp` destructor |

## Security impact

- ImGui is a client-side rendering library with no network requests, no file I/O by default (ini file is opt-in), and no external code execution.
- The engine does not expose ImGui's file I/O or clipboard to untrusted input.
- ImGui ini file is controlled by the app (demo app disables it).
- CONST-001 (architecture boundary) is preserved: SDL3/OpenGL headers stay inside `src/engine/`. `src/cmd/` files include only `engine_imgui.h` (forward declarations) and `<imgui.h>` (public ImGui header).

## Data and migration impact

None. No schema changes, data migrations, seed data, or data loss risks.

## API compatibility impact

- `RenderDevice` base class gains a new non-pure virtual method `render_ui()` with a default no-op body. This is backward compatible — existing subclasses (`RenderDeviceOpenGL`, `RenderDeviceHeadless`) continue to compile without changes. `RenderDeviceOpenGL` gains an explicit override.
- No existing public API signatures are modified (only added to).
- No changes to the `App` base class.

## Documentation impact

| Document | Changes |
|---|---|
| **README** (`docs/README.md` or project root) | None. |
| **Wiki: Module Map** (`docs/wiki/architecture/module-map.md`) | Add `src/engine/imgui/` section documenting `engine_imgui.h`, `engine_imgui.cpp`, the module's namespace, API functions, and its role. Refer to AC-028. |
| **Wiki: Data Flow** (`docs/wiki/architecture/data-flow.md`) | Update the frame loop section to show the ImGui hooks: `engine_imgui::new_frame()` inside `begin_frame()`, `engine_imgui::render()` inside `render_ui()`, `device.render_ui()` call in `run_app()`, and the `engine_imgui::on_sdl_event()` call in `poll_events()`. Update the scene dispatch list to include `"imgui-demo"`. Refer to AC-029. |
| **ADR** | Create `docs/adr/ADR-026-imgui-integration.md` documenting the architecture decision to embed ImGui as an engine module with automatic lifecycle integration. Refer to AC-027. |

## ADR impact

A new ADR is required (ADR-026). Document the following decisions:

1. **Approach**: Hybrid — embed official ImGui SDL3/OpenGL3 backends inside `src/engine/imgui/` as an engine-internal module. Engine handles boilerplate (init, frame lifecycle, event routing, shutdown). Apps use ImGui directly via `<imgui.h>`.
2. **Init location**: Inside `RenderDevice::create()` where `sdl_window` and `gl_context` are available as local variables. No public GL context accessor needed.
3. **Shutdown location**: In `RenderDeviceOpenGL::~RenderDeviceOpenGL()` before `SDL_GL_DestroyContext()`, ensuring GL context is current during backend cleanup.
4. **Frame lifecycle**: `new_frame()` from `RenderDeviceOpenGL::begin_frame()`, `render()` from `RenderDevice::render_ui()` virtual method, `run_app()` calls the generic `device.render_ui()`.
5. **Event routing**: Engine platform feeds SDL events to `engine_imgui::on_sdl_event()` unconditionally.
6. **ImGui configuration**: Owned by the app in its `setup()`. Engine only initialises backends.
7. **Headless mode**: Engine skips ImGui init when no display. `new_frame()`/`render()` become no-ops.
8. **ImGui version**: Docking branch (`v1.91.8-docking` via FetchContent).
9. **Verification**: Demo app (`buddd run imgui-demo`) with `--capture` support.
10. **Architecture boundary (ADR-019/CONST-001)**: Preserved — `engine_imgui.h` forward-declares SDL types, demo app does not include SDL3/OpenGL headers.

## Done criteria

The implementation is complete when all of the following are verifiable:

- [ ] **DC-01**: `src/engine/imgui/` directory exists with `engine_imgui.h`, `engine_imgui.cpp`, `CMakeLists.txt`.
- [ ] **DC-02**: `src/engine/imgui/engine_imgui.h` exists and declares all six functions (`init`, `shutdown`, `new_frame`, `render`, `on_sdl_event`, `is_initialized`) in `buddd::engine::engine_imgui` namespace, with forward declarations of `SDL_Window` and `SDL_Event` (no SDL3 includes).
- [ ] **DC-03**: `src/engine/CMakeLists.txt` contains `FetchContent_Declare` for ImGui docking branch (tag `v1.91.8-docking`) and `add_subdirectory(imgui)` after `add_library(buddd_engine ...)`.
- [ ] **DC-04**: `src/engine/imgui/CMakeLists.txt` adds ImGui sources and backends to `buddd_engine` via `target_sources()` and adds include directories for `imgui/` and `backends/`.
- [ ] **DC-05**: `render_device.h` declares `virtual auto render_ui() -> void {}` (non-pure, default no-op).
- [ ] **DC-06**: `render_device_opengl.h` declares `auto render_ui() -> void override`.
- [ ] **DC-07**: `render_device_opengl.cpp` contains three ImGui integration points:
  - (a) `#include "imgui/engine_imgui.h"`
  - (b) `engine_imgui::new_frame()` called inside `begin_frame()` after `glClear()`.
  - (c) `engine_imgui::render()` called inside `render_ui()`.
  - (d) `engine_imgui::shutdown()` called in `~RenderDeviceOpenGL()` BEFORE `SDL_GL_DestroyContext()`.
- [ ] **DC-08**: `render_device.cpp` contains `engine_imgui::init(sdl_window, gl_context)` inside the display-available branch, with warning on failure.
- [ ] **DC-09**: `platform_sdl3.cpp` contains `engine_imgui::on_sdl_event(event)` inside the `SDL_PollEvent` loop after `input_system_.on_sdl_event(event)`.
- [ ] **DC-10**: `app.cpp` contains `eng.device().render_ui()` after `app.on_render(ctx)` and before the capture block.
- [ ] **DC-11**: `main.cpp` dispatches `"imgui-demo"` scene to `ImguiDemoApp`.
- [ ] **DC-12**: `src/cmd/apps/imgui_demo_app.h` and `src/cmd/apps/imgui_demo_app.cpp` exist and define `ImguiDemoApp` with:
  - `config()` returning `AppConfig{"Buddd Engine — ImGui Demo", 1280, 720}`.
  - `setup()` disabling ImGui ini file (`io.IniFilename = nullptr`), no 3D scene.
  - `on_frame_begin()` as a no-op (no 3D scene to animate).
  - `on_render()` calling `ImGui::ShowDemoWindow()` (conditional) and the "ImGui Demo" panel with Show Demo Window checkbox and FPS text.
  - Member variable: `show_demo_window_` (bool, default true).
- [ ] **DC-13**: `buddd run imgui-demo --frame 10` exits with code 0 (no crash).
- [ ] **DC-14**: `buddd run imgui-demo --frame 120 --capture 120:/tmp/dc14.png` produces a valid PNG file with ImGui overlay on solid-color background.
- [ ] **DC-15**: `buddd run cube --frame 10` runs normally (no regression from ImGui changes).
- [ ] **DC-16**: `cmake -B build/headless -DBUDDD_HAS_DISPLAY=OFF && cmake --build build/headless` succeeds (headless build compiles with ImGui code present).
- [ ] **DC-17**: `grep -rnE '#include.*(SDL3|SDL_opengl|GL/|glad)' src/cmd/` returns zero matches (CONST-001 preserved).
- [ ] **DC-18**: ADR-026 created at `docs/adr/ADR-026-imgui-integration.md` with status "Accepted".
- [ ] **DC-19**: Wiki module map (`docs/wiki/architecture/module-map.md`) updated with `src/engine/imgui/` section.
- [ ] **DC-20**: Wiki data flow (`docs/wiki/architecture/data-flow.md`) updated with ImGui frame lifecycle hooks and scene dispatch entry for `"imgui-demo"`.
