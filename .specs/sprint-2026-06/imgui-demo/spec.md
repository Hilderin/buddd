# SPEC-026 — EngineImGui Module: Dear ImGui Integration

## Problem

The engine currently has no immediate mode GUI capability. Applications and the future editor have no way to display debug overlays, property panels, or interactive UI elements. Every GUI concept must be implemented from scratch using OpenGL primitives, or the developer must write boilerplate to integrate a third-party library.

Dear ImGui is the de-facto standard for debug/editor UI in C++ graphics applications, but embedding it requires:

1. Adding ImGui as a dependency (FetchContent or git submodule).
2. Implementing platform backends (SDL3 event routing, clipboard, cursor, mouse/keyboard input).
3. Implementing render backends (OpenGL 3.x/4.x shader setup, VAO/VBO management, texture upload for font atlas).
4. Integrating the frame lifecycle (NewFrame before app logic, Render after app rendering).
5. Managing init/shutdown of both context and backends.
6. Building a verification app to confirm everything works.

Without a module, every app that wants ImGui must repeat this boilerplate, leading to fragmentation and maintenance burden.

## Goals

- **G-01**: Introduce `engine_imgui` — an engine-internal module (`src/engine/imgui/`) that embeds Dear ImGui's official SDL3 and OpenGL3 backends and provides a minimal public API for lifecycle and event routing.
- **G-02**: Automatically integrate ImGui into the frame loop: `engine_imgui::new_frame()` is called from within `RenderDevice::begin_frame()`, and `engine_imgui::render()` is called from within a new `RenderDevice::render_ui()` method. `run_app()` calls `device.render_ui()` as a generic UI overlay step (no ImGui-specific code in `run_app()`).
- **G-03**: Automatically route SDL events to ImGui from `PlatformSDL3::poll_events()`, with zero app involvement.
- **G-04**: Apps use Dear ImGui's C++ API directly (`ImGui::Begin()`/`End()`/`Text()`/`Button()`, etc.) without any wrapper or indirection.
- **G-05**: Provide a demo app (scene `imgui-demo`) that verifies the integration end-to-end: builds, runs via `buddd run imgui-demo`, shows `ImGui::ShowDemoWindow()` plus a custom panel, and supports screenshot capture via `--capture` for CI verification.
- **G-06**: Create an ADR documenting the architecture decision.
- **G-07**: Update wiki documentation (module map, data flow) to reflect the new module.

## Non-goals

- No editor application — this feature delivers the engine module and a demo app only. The editor is a future work item.
- No ImGui API wrapper or abstraction layer — apps include `<imgui.h>` and use Dear ImGui's API directly.
- No multi-viewport support — only a single ImGui context rendering into the main window. Multi-viewport (`ImGuiConfigFlags_ViewportsEnable`) is out of scope for this spec.
- No ImGui input capture API — apps do not get to intercept or modify ImGui's event consumption. ImGui processes events internally.
- No custom ImGui backends — we embed the official unmodified `imgui_impl_sdl3` and `imgui_impl_opengl3` backends.
- No headless ImGui integration — ImGui init is skipped when there is no display. Apps simply do not call ImGui functions in headless mode.
- No changes to the `App` base class virtual interface (no new overrides for ImGui setup).
- No changes to existing tests — ImGui has no test-specific integration.
- No font management beyond what the app explicitly configures — fonts, ini file path, style, and IO flags are the app's responsibility in its `setup()`.
- No ImGui integration with the engine's input system (KeyCode, InputSystemSDL3) — ImGui reads SDL events directly from its own SDL3 backend, bypassing the engine's input abstraction.
- No runtime toggling of ImGui visibility — if the engine is initialized with ImGui support, it renders every frame. Apps that don't want ImGui simply don't include headers or call ImGui functions.

## Actors

| Actor | Description |
|---|---|
| App developer | Writes an `App` subclass that uses Dear ImGui. Includes `<imgui.h>`, calls `ImGui::Begin()`/`End()` in `on_render()`, configures fonts/INI in `setup()`. |
| Engine consumer | A developer building on top of `buddd_engine`. Gets ImGui integration "for free" if using `run_app()`. |
| Demo verifier | A developer or CI system that runs `buddd run imgui-demo --capture N:path` to verify the integration works. |

## Key entities

### `engine_imgui` namespace (`src/engine/imgui/`)

The module exposes a minimal API in the `buddd::engine::engine_imgui` namespace:

| Symbol | Role |
|---|---|
| `auto init(SDL_Window*, SDL_GLContext) -> Result<void>` | Creates ImGui context, initialises ImGui_ImplSDL3_InitForOpenGL() and ImGui_ImplOpenGL3_Init("#version 410 core"). Called once from within `RenderDevice::create()` when a display is available, using local `sdl_window` and `gl_context` variables. |
| `auto shutdown() -> void` | Shuts down backends (ImGui_ImplOpenGL3_Shutdown, ImGui_ImplSDL3_Shutdown) and destroys ImGui context (ImGui::DestroyContext). Called once during engine teardown — in `RenderDeviceOpenGL::~RenderDeviceOpenGL()` or `~EngineService()` before device cleanup. |
| `auto new_frame() -> void` | Calls ImGui_ImplOpenGL3_NewFrame(), ImGui_ImplSDL3_NewFrame(), ImGui::NewFrame(). Called internally by `RenderDeviceOpenGL::begin_frame()` after the buffer clear. No-op if not initialised. |
| `auto render() -> void` | Calls ImGui::Render() and ImGui_ImplOpenGL3_RenderDrawData(). Called internally by `RenderDevice::render_ui()`. No-op if not initialised. |
| `auto on_sdl_event(SDL_Event const&) -> bool` | Forwards an SDL event to ImGui_ImplSDL3_ProcessEvent(). Returns true if the event was consumed by ImGui. |
| `auto is_initialized() -> bool` | Returns whether ImGui context was successfully created. Used by run_app() and PlatformSDL3 to decide whether to call ImGui lifecycle functions. |

### `RenderDevice` — new virtual method

A new virtual method with a default no-op body is added to the `RenderDevice` abstract interface:

| Method | Role |
|---|---|
| `auto render_ui() -> void` | Renders any active UI overlay (ImGui). Called by `run_app()` after `app.on_render()` and before `read_pixels()`/`capture`. Declared as a non-pure virtual with a default no-op body in the `RenderDevice` base class. `RenderDeviceOpenGL` overrides to call `engine_imgui::render()`. `RenderDeviceHeadless` inherits the no-op default. |

This method is **not** ImGui-specific — it is a generic "render the UI layer" hook that happens to dispatch to ImGui in the OpenGL backend.

### ImGui source layout

Dear ImGui sources (from the docking branch) live in `src/engine/imgui/` alongside the module wrapper:

```
src/engine/imgui/
├── CMakeLists.txt          # Build rules for the imgui module sources
├── engine_imgui.h          # Public API header (namespace buddd::engine::engine_imgui)
├── engine_imgui.cpp        # Implementation of init/shutdown/new_frame/render/on_sdl_event
├── imgui/                  # Dear ImGui library sources (docking branch)
│   ├── imgui.cpp
│   ├── imgui.h
│   ├── imgui_demo.cpp
│   ├── imgui_draw.cpp
│   ├── imgui_internal.h
│   ├── imgui_widgets.cpp
│   ├── imgui_tables.cpp
│   ├── imstb_rectpack.h
│   ├── imstb_textedit.h
│   ├── imstb_truetype.h
│   └── ... (other .h/.cpp as needed)
├── backends/               # Official ImGui backends (embedded, unmodified)
│   ├── imgui_impl_sdl3.h
│   ├── imgui_impl_sdl3.cpp
│   ├── imgui_impl_opengl3.h
│   ├── imgui_impl_opengl3.cpp
│   └── imgui_impl_opengl3_loader.h
└── (other supporting files)
```

## User-visible behavior

### EngineImGui module — public API (`engine_imgui.h`)

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
/// app.on_frame_begin(). Calls ImGui_ImplOpenGL3_NewFrame(),
/// ImGui_ImplSDL3_NewFrame(), ImGui::NewFrame().
/// No-op if not initialised.
auto new_frame() -> void;

/// Render ImGui draw data. Must be called once per frame after
/// app.on_render() and before end_frame(). Calls ImGui::Render()
/// and ImGui_ImplOpenGL3_RenderDrawData().
/// No-op if not initialised.
auto render() -> void;

/// Forward an SDL event to ImGui_ImplSDL3_ProcessEvent().
/// Returns true if ImGui consumed the event (should not be processed
/// further by the app). No-op if not initialised — returns false.
[[nodiscard]] auto on_sdl_event(const SDL_Event& event) -> bool;

/// Returns true if ImGui was successfully initialised.
[[nodiscard]] auto is_initialized() -> bool;

} // namespace buddd::engine::engine_imgui
```

### Frame lifecycle integration

ImGui lifecycle calls are fully encapsulated within `RenderDevice` and `Platform`. `run_app()` has no ImGui-specific code — it only calls the generic `device.render_ui()` method.

**`RenderDeviceOpenGL::begin_frame()`** internally becomes:
1. Clear buffers (existing)
2. If `engine_imgui::is_initialized()`: call `engine_imgui::new_frame()` (new)

**`RenderDeviceOpenGL::render_ui()`** implements the new virtual:
1. If `engine_imgui::is_initialized()`: call `engine_imgui::render()` (which calls `ImGui::Render()` + `ImGui_ImplOpenGL3_RenderDrawData()`)

**`RenderDeviceOpenGL::end_frame()`** is unchanged (SDL_GL_SwapWindow only).

In `run_app()` (`src/cmd/app.cpp`), the render loop gains one generic line:

```
[Frame loop]
1. poll_events()                   ← event routing includes engine_imgui::on_sdl_event()
2. device.begin_frame()            ← internally calls engine_imgui::new_frame() after clear
3. app.on_frame_begin(ctx)
4. world->update_updatables(ctx)
5. render_system->render_scene()
6. app.on_render(ctx)              ← app calls ImGui::Begin()/End() here
7. device.render_ui()              ← NEW: renders any UI overlay (internally calls engine_imgui::render())
8. capture (read_pixels)           ← now includes ImGui overlay
9. device.end_frame()              ← swap only (unchanged)
```

Key points:
- `new_frame()` runs inside `begin_frame()`, AFTER the GL context is current and AFTER the buffer clear, BEFORE `app.on_frame_begin()`.
- `render()` runs inside `render_ui()`, AFTER `app.on_render()` (so app ImGui calls are recorded in the draw list) and BEFORE `read_pixels` (so ImGui rendering appears in captures).
- Both are conditional on `engine_imgui::is_initialized()`.
- `run_app()` has zero ImGui `#include` directives and zero ImGui-specific function calls.

### Event routing integration

In `PlatformSDL3::poll_events()` (`src/engine/platform/platform_sdl3.cpp`), after routing each SDL event to `input_system_.on_sdl_event(event)`, the event is also routed to `engine_imgui::on_sdl_event(event)`. The ImGui event handler may report that it consumed the event (e.g., for text input) — but at this architectural level, the engine does not filter events based on ImGui's consumption. InputSystem continues to see all events for its own state tracking.

```
while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) { return false; }
    input_system_.on_sdl_event(event);       // existing
    engine_imgui::on_sdl_event(event);        // NEW — always called regardless of consumption
}
```

Note: ImGui's `io.WantCaptureMouse`, `io.WantCaptureKeyboard`, and `io.WantTextInput` flags are available to the app via `ImGui::GetIO()`. The engine does NOT use these flags to suppress InputSystem state — apps that use both ImGui and engine input must check these flags themselves if they need to avoid double-processing.

### Demo app (`src/cmd/apps/imgui_demo_app.h/.cpp`)

A new `App` subclass named `ImguiDemoApp` in namespace `buddd::cmd::app`:

- **Window**: 1280×720, title `"Buddd Engine — ImGui Demo"`.
- **Frame limit**: 300 frames (or `--frame N` override), plus interactive mode via `--capture`.
- **Scene**: `"imgui-demo"`.
- **Rendering**: No 3D scene — just a cleared framebuffer with the ImGui overlay. This keeps the demo focused on verifying ImGui integration.
- **ImGui content**:
  - `ImGui::ShowDemoWindow(nullptr)` — the full Dear ImGui demo window.
  - A custom panel titled "ImGui Demo" with:
    - A checkbox "Show Demo Window" (default true) that toggles `ImGui::ShowDemoWindow()`.
    - FPS counter: `ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate)`.
  - Uses `ImGui::Begin()`/`End()` in `on_render()`.
- **Capture**: Supports `--capture N:path`. Captured frames should show the ImGui overlay on a solid clear-color background.
- **Config**: Sets `io.IniFilename` to `nullptr` (no ini file persistence); no custom font loading.

### End-to-end verification

The demo app is the primary verification method:
1. Build succeeds: `cmake --build --preset debug` compiles the `imgui-demo` scene.
2. Running `buddd run imgui-demo --frame 10` opens a window and renders 10 frames with ImGui overlay.
3. Running `buddd run imgui-demo --frame 120 --capture 120:/tmp/imgui_demo.png` captures frame 120 (showing cube + ImGui overlay) to a valid PNG.
4. The PNG file is checked by CI to be a valid PNG (header bytes `89 50 4E 47`).

Note: CI cannot render OpenGL, so the demo app is built and verified to compile, but capture verification is manual or run on a display-enabled runner. A future CI improvement may add headless build verification.

### ImGui source inclusion in build

The engine's top-level `CMakeLists.txt` declares ImGui via `FetchContent`, downloading the docking branch. The `src/engine/imgui/CMakeLists.txt` defines a static OBJECT library or adds sources to `buddd_engine` via the glob.

The `src/engine/CMakeLists.txt` is modified to add the `imgui/` subdirectory to the glob, or an explicit source list for `src/engine/imgui/` is added.

### GLSL version compatibility

The engine creates an OpenGL 4.5 Core context (GLSL 450 core). ImGui's `ImGui_ImplOpenGL3_Init()` is called with `"#version 410 core"` as the GLSL version string. The ImGui backend uses `#version 410 core` shaders for GL contexts >= 4.1, which is fully compatible with GL 4.5 Core Profile. The engine's own shaders use `#version 450 core` — both coexist in the same GL context without conflict because they are separate shader programs.

ImGui's `ImGui_ImplOpenGL3_RenderDrawData()` saves, modifies, and restores OpenGL state (program, VAO, VBO, blend, scissor, depth test, etc.), ensuring no state leaks between the engine's rendering and ImGui's rendering.

### Build dependencies

| Dependency | How it's added | Status |
|---|---|---|
| ImGui (docking branch) | `FetchContent` in `src/engine/CMakeLists.txt` | New |
| SDL3 | Already present via `FetchContent` | Existing |
| OpenGL 4.5 Core | Already via `find_package(OpenGL REQUIRED)` | Existing |

`FetchContent_Declare` for ImGui:
```cmake
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8-docking
)
FetchContent_MakeAvailable(imgui)
```

## User stories

### Story 1 — App uses ImGui directly (Priority: P1)

As an app developer, I want to call `ImGui::Begin()`/`End()` inside my App's `on_render()` and see the UI rendered over my scene, so that I can build debug overlays and editor tools.

**Given** the engine is compiled with display support and `engine_imgui` is initialised
**When** my `App::on_render()` calls `ImGui::Begin("My Window")` ... `ImGui::End()`
**Then** the ImGui window is rendered over the 3D scene in every frame.

### Story 2 — Demo app builds and runs (Priority: P1)

As a developer verifying the integration, I want to run the demo and see Dear ImGui's demo window plus a custom panel.

**Given** the project builds with display support
**When** I run `buddd run imgui-demo --frame 60`
**Then** a 1280×720 window opens, shows the ImGui Demo Window and an "ImGui Demo" panel on a solid-color background, and exits after 60 frames with code 0.

### Story 3 — Capture with ImGui overlay (Priority: P1)

As a developer, I want to capture a frame that includes ImGui rendering, so that I can verify the integration in CI (manual run).

**Given** the project builds with display support
**When** I run `buddd run imgui-demo --frame 120 --capture 120:/tmp/imgui_capture.png`
**Then** frame 120 is saved to `/tmp/imgui_capture.png` as a valid PNG showing both the cube and the ImGui overlay.

### Story 4 — Headless no-op (Priority: P2)

As a developer, I want to run a headless build without ImGui crashing, so that CI without a display still passes.

**Given** the project is compiled with `BUDDD_HAS_DISPLAY=OFF`
**When** `run_app()` runs (any App subclass)
**Then** `engine_imgui::init()` is never called, `new_frame()`/`render()`/`on_sdl_event()` are no-ops, and the app runs without crashing even if it never calls ImGui.

### Story 5 — Event routing (Priority: P2)

As a developer, I want mouse and keyboard input to reach ImGui's interaction system (buttons, sliders, text input), so that the UI is interactive.

**Given** the engine is running with ImGui initialised and ImGui Demo Window is visible
**When** I click a button in the ImGui Demo Window with the mouse
**Then** the button responds visually (hover highlight, click animation) and the associated action triggers.

### Story 6 — ImGui state restore after init failure (Priority: P2)

As an engine maintainer, I want ImGui init failure (e.g., failed shader compilation) to not crash the engine.

**Given** ImGui is compiled but `engine_imgui::init()` returns an error
**When** `run_app()` proceeds
**Then** `is_initialized()` returns false, `new_frame()`/`render()`/`on_sdl_event()` are no-ops, and the app runs normally without ImGui.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `src/engine/imgui/engine_imgui.h` exists and declares `init()`, `shutdown()`, `new_frame()`, `render()`, `on_sdl_event()`, `is_initialized()` in namespace `buddd::engine::engine_imgui`. | File exists, compiles, symbols resolve. |
| AC-002 | ImGui is fetched via `FetchContent` from the docking branch (tag `v1.91.8-docking` or later). | Inspect `src/engine/CMakeLists.txt` for `FetchContent_Declare(imgui ...)`. |
| AC-003 | `engine_imgui::init()` creates an ImGui context, calls `ImGui_ImplSDL3_InitForOpenGL()`, and `ImGui_ImplOpenGL3_Init("#version 410 core")`. | Call init, then verify `ImGui::GetCurrentContext() != nullptr` and `ImGui::GetIO().BackendPlatformName != nullptr`. |
| AC-004 | `engine_imgui::shutdown()` calls `ImGui_ImplOpenGL3_Shutdown()`, `ImGui_ImplSDL3_Shutdown()`, and `ImGui::DestroyContext()`. | Call init then shutdown; after shutdown `ImGui::GetCurrentContext()` returns nullptr. Safe to call twice (no crash). |
| AC-005 | `engine_imgui::new_frame()` calls ImGui's NewFrame chain. | After `new_frame()`, `ImGui::GetIO().DeltaTime > 0` (frame data was populated). |
| AC-006 | `engine_imgui::render()` calls `ImGui::Render()` and `ImGui_ImplOpenGL3_RenderDrawData()`. | After `render()`, `ImGui::GetDrawData()` returns non-null but current draw data is consumed. |
| AC-007 | `engine_imgui::on_sdl_event()` forwards events to `ImGui_ImplSDL3_ProcessEvent()` and returns its result. | Inject an `SDL_EVENT_MOUSEMOTION` event; verify `on_sdl_event()` returns false (ImGui does not filter basic events at this level). |
| AC-008 | `engine_imgui::is_initialized()` returns false before `init()`, true after successful `init()`, false after `shutdown()`. | Call `is_initialized()` before/after/success/failure init. |
| AC-009 | `engine_imgui::init()` returns an error if called without a current GL context, and `is_initialized()` remains false. | Call init with invalid params or without MakeCurrent; verify `Result` is error and `is_initialized()` is false. |
| AC-010 | `engine_imgui::new_frame()` is called from within `RenderDeviceOpenGL::begin_frame()` after the buffer clear. | Inspect `render_device_opengl.cpp` — `engine_imgui::new_frame()` is called inside `begin_frame()` after the clear. |
| AC-011 | `RenderDevice` declares `render_ui()` as a virtual method with default no-op body. `RenderDeviceOpenGL::render_ui()` calls `engine_imgui::render()`. `run_app()` calls `device.render_ui()` after `app.on_render()` and before `read_pixels`. | (1) Inspect `render_device.h` — `render_ui()` is declared as virtual with default no-op. (2) Inspect `render_device_opengl.cpp` — `render_ui()` calls `engine_imgui::render()`. (3) Inspect `app.cpp` — `device.render_ui()` appears between `app.on_render(ctx)` and capture/`read_pixels`. |
| AC-012 | ImGui event routing is added in `PlatformSDL3::poll_events()` after `input_system_.on_sdl_event()`. | Inspect `platform_sdl3.cpp` — `engine_imgui::on_sdl_event(event)` is called inside the SDL_PollEvent loop. |
| AC-013 | ImGui init is called inside `RenderDevice::create()` after `new RenderDeviceOpenGL(sdl_window, ...)` succeeds, where both `sdl_window` and `gl_context` are available as local variables. Shutdown is called in `RenderDeviceOpenGL::~RenderDeviceOpenGL()` (or `~EngineService()` before device cleanup). | Inspect `render_device.cpp` — `engine_imgui::init(sdl_window, gl_context)` is called inside `RenderDevice::create()` right before the return. Inspect `render_device_opengl.cpp` or `engine_service.cpp` — `engine_imgui::shutdown()` is called before the device is destroyed. |
| AC-014 | `src/cmd/apps/imgui_demo_app.h` and `imgui_demo_app.cpp` exist and define `ImguiDemoApp` (an `App` subclass). | Files exist and compile. |
| AC-015 | `ImguiDemoApp::config()` returns `AppConfig` with title `"Buddd Engine — ImGui Demo"`, width 1280, height 720. | Inspect or test programmatically. |
| AC-016 | `ImguiDemoApp::on_render()` calls `ImGui::ShowDemoWindow()` when the "Show Demo Window" checkbox is true (default). | Run `buddd run imgui-demo --frame 10`; ImGui Demo Window is visible. |
| AC-017 | `ImguiDemoApp::on_render()` draws a custom "ImGui Demo" panel with FPS counter and show-demo toggle checkbox. | Run `buddd run imgui-demo --frame 10`; the panel is visible with FPS text and checkbox. |
| AC-018 | Captured frame shows ImGui overlay on a solid clear-color background. | Run `buddd run imgui-demo --frame 120 --capture 120:/tmp/ac18.png`; the PNG shows ImGui windows on a solid-color background. |
| AC-019 | `buddd run imgui-demo --frame 0` (interactive) runs until window close. | Run interactively; verify window stays open until closed manually. |
| AC-020 | `buddd run imgui-demo --frame 10` runs exactly 10 frames and exits. | Run command; verify exit code 0 and 10 frames rendered (stderr log message). |
| AC-021 | `buddd run imgui-demo --frame 120 --capture 120:/tmp/ac21.png` captures a valid PNG. | File `/tmp/ac21.png` exists and has valid PNG header. |
| AC-022 | The demo app (`ImguiDemoApp`) is registered as a CLI scene in `main.cpp` dispatch for scene name `"imgui-demo"`. | Run `buddd run imgui-demo --frame 10` and verify it runs the ImGui demo. |
| AC-023 | Headless build (`BUDDD_HAS_DISPLAY=OFF`) compiles successfully with ImGui code present. | `cmake -B build/headless -DBUDDD_HAS_DISPLAY=OFF` and `cmake --build build/headless` succeed. |
| AC-024 | Calling `engine_imgui::shutdown()` twice does not crash (double-shutdown safety). | Call shutdown() followed by shutdown(); no crash, no undefined behaviour. |
| AC-025 | Calling `engine_imgui::new_frame()` or `render()` when `is_initialized()` is false is a no-op (no crash). | Call new_frame() and render() before init(); no crash. |
| AC-026 | `src/engine/imgui/` directory contains the embedded ImGui source files (imgui.h, imgui.cpp, etc.) or references them via the FetchContent source directory. | Directory exists with working build. |
| AC-027 | The ADR for the ImGui architecture decision is created at `docs/adr/ADR-026-imgui-integration.md`. | File exists with status "Accepted" and documents approach, rationale, key decisions. |
| AC-028 | The wiki module map is updated to document the new `src/engine/imgui/` module. | `docs/wiki/architecture/module-map.md` contains a section about the imgui module. |
| AC-029 | The wiki data flow is updated to document the new ImGui frame lifecycle and event routing. | `docs/wiki/architecture/data-flow.md` documents the ImGui hooks in the frame loop and event poll. |
| AC-030 | No SDL3 or OpenGL headers are included from any file under `src/cmd/` (CONST-001 preserved). | Run `grep -rnE '#include.*(SDL3|SDL_opengl|GL/|glad)' src/cmd/` — zero matches. |

## E2E Verification

The feature is verified end-to-end through the demo app:

1. **Build verification**: `cmake --build --preset debug` compiles the `imgui-demo` scene without error.
2. **Functional verification** (manual, display required): Run `buddd run imgui-demo --frame 120 --capture 120:/tmp/verify.png`. The window displays the ImGui Demo Window and "ImGui Demo" panel on a solid-color background. The captured PNG shows the ImGui overlay.
3. **Headless build verification**: `cmake -B build/headless -DBUDDD_HAS_DISPLAY=OFF && cmake --build build/headless` succeeds with no errors.
4. **Regression verification**: Run `buddd run cube --frame 10` and compare captured output with pre-ImGui known-good capture — rendering must be identical.

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An app developer can add ImGui UI to a new App subclass in under 5 minutes — just include `<imgui.h>` and call `ImGui::Begin()`/`End()` in `on_render()`. | Create a minimal App that draws `ImGui::Text("Hello")` — no additional boilerplate needed beyond the App subclass. |
| SC-002 | `buddd run imgui-demo --frame 120 --capture 120:/tmp/capture.png` completes in under 5 seconds. | Measure wall-clock time from invocation to exit. |
| SC-003 | Existing non-ImGui apps produce identical captured output before and after the ImGui integration (no GL state leaks). | Run `buddd run cube --frame 10 --capture 10:/tmp/before.png` and `buddd run cube --frame 10 --capture 10:/tmp/after.png`; pixel-diff is zero. |
| SC-004 | ImGui does not increase the compile time of apps that don't use it by more than 10% (the ImGui library and backends are compiled as part of `buddd_engine`). | Measure `cmake --build` time with and without ImGui; ratio ≤ 1.10. |

## Edge cases

| Case | Expected behavior |
|---|---|
| Headless build (no display) | `engine_imgui::init()` is never called. `is_initialized()` always returns false. All lifecycle/event functions are no-ops. |
| ImGui init fails (e.g., shader compilation error) | `init()` returns error. `is_initialized()` returns false. Engine continues without ImGui. |
| App never calls any ImGui function | No crash. ImGui new_frame/render run every frame with an empty draw list. |
| App calls ImGui functions before engine_imgui::init() | Undefined behaviour (ImGui context doesn't exist). Not guarded by the engine — app developer's responsibility. |
| Double shutdown (engine_imgui::shutdown called twice) | Second call is a no-op (null context check). |
| Multiple init calls | Second call is an error (context already exists). Returns error Result. |
| Window is minimised / zero-size | ImGui handles this internally (RenderDrawData checks fb_width/fb_height > 0). |
| SDL_EVENT_QUIT during event loop | Quit event is handled by the existing `return false` path in `poll_events()`. ImGui never sees the quit event. |
| ImGui `io.WantCaptureMouse` / `io.WantCaptureKeyboard` | The engine does NOT filter events based on these flags. InputSystem continues to see all events. Apps that use both ImGui and input system must check these flags themselves. |
| ImGui ini file persistence | App controls `io.IniFilename`. Demo app sets it to `nullptr` (no persistence). The engine does not manage ini files. |
| Font atlas creation failure | ImGui handles this internally — if font atlas creation fails, no text renders but ImGui functions still work. |
| `--capture` with ImGui overlay | The capture happens after `device.render_ui()` (which calls `engine_imgui::render()`), so the captured framebuffer includes ImGui draw data. |
| Window resize during ImGui session | ImGui handles via `ImGui_ImplSDL3_NewFrame()` which queries the window size from SDL. |
| Keyboard input while ImGui text field is focused | ImGui consumes keyboard events via `io.WantCaptureKeyboard`. The engine's InputSystem still records key state, but the app should check `io.WantCaptureKeyboard` before processing game input. |
| SDL_Event type not recognized by ImGui | `ImGui_ImplSDL3_ProcessEvent()` returns false for unrecognized event types. `on_sdl_event()` returns `false`. This is the normal case for most engine-internal events. |

## Error cases

| Case | Expected behavior |
|---|---|
| `engine_imgui::init()` called without a current GL context | Returns `Error{InitFailed, "No current GL context"}`. `is_initialized()` remains false. |
| `engine_imgui::init()` called when already initialised | Returns `Error{InitFailed, "ImGui already initialised"}`. State unchanged. |
| ImGui `FetchContent` fails (no network, invalid tag) | Build fails at configure time with CMake error. Clear error message from FetchContent. |
| `ImGui_ImplOpenGL3_Init()` returns false (shader compile failure) | `init()` returns error. `is_initialized()` is false. |
| `ImGui_ImplSDL3_InitForOpenGL()` fails | `init()` returns error. `DestroyContext()` is called to clean up the created ImGui context before returning error. |
| `SDL_Window*` parameter to `init()` is null | `init()` returns `Error{InvalidArgument, "SDL_Window cannot be null"}`. |
| ImGui draw data is null during render | `render()` checks `is_initialized()` and `ImGui::GetDrawData()`; if null, returns early (no-op). |

## Permissions and security

- ImGui is a client-side rendering library. It does not make network requests, access files by default (ini file is opt-in), or execute external code.
- The engine does not expose ImGui's file I/O or clipboard access to untrusted input.
- ImGui's ini file is controlled by the app via `io.IniFilename`. The demo app disables it.
- No elevated privileges are required.
- CONST-001 is preserved: SDL3 and OpenGL headers stay inside `src/engine/`. `src/cmd/` files do not include them.
- ImGui sources include `<imgui.h>` and backend headers, which do not expose SDL3 or OpenGL types in their public API beyond opaque pointers (`SDL_Window*` is forward-declared in `engine_imgui.h`, not included).

## Observability

| Signal | Source |
|---|---|
| ImGui initialised | `BUDDD_LOG_INFO("ImGui: initialised (backend SDL3+OpenGL3)")` in `engine_imgui::init()` on success |
| ImGui init failure | `BUDDD_LOG_ERROR("ImGui: init failed: {}")` in `engine_imgui::init()` on failure |
| ImGui shutdown | `BUDDD_LOG_INFO("ImGui: shutdown")` in `engine_imgui::shutdown()` |
| ImGui render skipped (not initialised) | `BUDDD_LOG_TRACE("ImGui: skipped (not initialised)")` in `new_frame()`/`render()` when not initialised |
| Frame lifecycle messages (existing) | Unchanged from SPEC-008: scene started/complete/aborted via `BUDDD_LOG_INFO` |

## Out of scope

- Multi-viewport support (`ImGuiConfigFlags_ViewportsEnable`).
- ImGui docking/tabbed window layout persistence.
- ImGui integration with the engine's InputSystem (key code mapping, suppression of engine input when ImGui wants capture).
- Custom ImGui backends or modifications to the official backends.
- Editor application — this spec delivers the engine module and a verification demo only.
- ImGui theming/styling beyond what the app configures.
- CI display-backed automated screenshot comparison (requires GPU in CI).
- Runtime loading of ImGui as a shared library.
- ImGui integration with AssetManager / texture loading (font atlas only).
- ImGui viewport rendering into separate OS windows.
- Any ImGui API beyond direct usage (no wrapper, no abstractions, no helpers).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | ImGui docking branch tag `v1.91.8-docking` (or later) is available and API-compatible with the backends we embed. |
| A-02 | `ImGui_ImplOpenGL3_Init("#version 410 core")` is compatible with an OpenGL 4.5 Core Profile context. GLSL 410 core shaders are a subset of 450 core (verified). |
| A-03 | ImGui's embedded OpenGL loader (`imgui_impl_opengl3_loader.h`, based on gl3w) does not conflict with the engine's `GL_GLEXT_PROTOTYPES` approach, because they are compiled in separate translation units. |
| A-04 | The `ImGui_ImplOpenGL3_RenderDrawData()` function correctly saves and restores all modified OpenGL state (program, VAO, VBO, blend, scissor, depth, stencil, polygon mode, viewport, sampler). |
| A-05 | `ImGui_ImplSDL3_ProcessEvent()` correctly handles all SDL3 event types that ImGui cares about (mouse, keyboard, text input, window events). |
| A-06 | `RenderDevice` outlives all ImGui usage. `engine_imgui::shutdown()` is called in `RenderDeviceOpenGL::~RenderDeviceOpenGL()` (or `~EngineService()` before device destruction), after the render loop has exited. |
| A-07 | The `run_app()` function is the only frame loop entry point. All apps go through it, so ImGui lifecycle hooks in `run_app()` cover all app types. |
| A-08 | `SDL_Window*` is obtained from `WindowSDL3::native_handle()` (which casts the internal `SDL_Window*` to `void*`). `engine_imgui::init()` receives this as a `SDL_Window*`. |
| A-09 | The demo app's `ImguiDemoApp` class is registered in the `main.cpp` scene dispatch table and run via `buddd run imgui-demo`. No separate binary or `main()` function. |
| A-10 | ImGui's `io.WantCaptureMouse`, `io.WantCaptureKeyboard`, and `io.WantTextInput` flags are available for apps to check — but the engine does NOT use them to filter events. |
| A-11 | The `FetchContent` for ImGui's docking branch does not require any special CMake configuration beyond the basic `GIT_REPOSITORY` and `GIT_TAG`. The repository is header-only in its library files, requiring compilation of `.cpp` files. |
| A-12 | The GLSL version string `"#version 410 core"` is compatible with all target platforms (Linux, Windows, macOS) running OpenGL 4.5 Core. |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | Should ImGui be compiled as part of `buddd_engine` (via glob), or as a separate static library? | **Part of `buddd_engine`**. Compiled as part of the engine static library via the glob pattern. No separate target. |
| Q-02 | What GLSL version string should be passed to `ImGui_ImplOpenGL3_Init()`? | **`"#version 410 core"`**. Engine runs OpenGL 4.5 Core; 410 is the closest GLSL version to the engine's 450 core that ImGui has built-in shaders for. |
| Q-03 | Should ImGui event consumption (`on_sdl_event` returning true) suppress input from reaching InputSystem? | **No**. The engine does not filter events based on ImGui's consumption. Apps check `io.WantCaptureMouse/Keyboard` if needed. |
| Q-04 | Should the demo app be a standalone binary or a scene in `buddd run`? | **Scene only**. The demo is registered as a scene accessible via `buddd run imgui-demo`. No standalone binary. |
| Q-05 | Should ImGui's ini file be stored in a project-specific path or disabled by default? | **Disabled by default** (io.IniFilename = nullptr). The demo app controls its own ini config. |
| Q-06 | Should we integrate ImGui into the `App` base class via a virtual method (e.g., `on_imgui()`), or keep it entirely behind the scenes? | **Behind the scenes**. Apps call ImGui in `on_render()`. No new virtual methods. |
