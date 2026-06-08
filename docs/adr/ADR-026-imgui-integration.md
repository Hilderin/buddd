# ADR-026: Dear ImGui Integration as an Engine-Internal Module

## Status

`Accepted`

## Context

SPEC-026 introduced the need for immediate-mode GUI capability in the Buddd Engine. Dear ImGui is the de-facto standard for debug/editor UI in C++ graphics applications, but embedding it requires platform backend integration (SDL3 event routing, clipboard, cursor, mouse/keyboard input), render backend integration (OpenGL 3.x/4.x setup, VAO/VBO management, font atlas texture upload), and frame lifecycle management (NewFrame before app logic, Render after app rendering).

Without a dedicated engine module, every app that wants ImGui must repeat this boilerplate, leading to fragmentation and maintenance burden.

The following constraints shaped the decision:

- **CONST-001** (documented in ADR-019): No SDL3, OpenGL, or GLM headers outside `src/engine/`. SDL3 and OpenGL types must not leak into `src/cmd/` or any other non-engine code.
- **ADR-012**: Shutdown of the engine component chain in `EngineService` / `~RenderDeviceOpenGL()` must happen before the GL context is destroyed — the ImGui shutdown integration point must respect this ordering.
- **ADR-003**: Draw methods are `void` (not `Result<void>`) and `Platform::poll_events()` exists on the platform abstraction. ImGui rendering follows the same `void` convention.
- **ADR-019**: Architecture boundaries are enforced by code review; any exception requires explicit ADR-level justification.
- **App base class must not change**: The `App` virtual interface (`config()`, `setup()`, `on_frame_begin()`, `on_render()`, `shutdown()`) was established in ADR-014 and is used by all demo apps. Adding ImGui-specific virtual methods to `App` would couple every app subclass to ImGui.
- **`run_app()` must stay generic**: The frame loop in `src/cmd/app.cpp` is the single entry point for all apps. It must not acquire ImGui-specific includes or function calls.

### Alternatives considered

| Alternative | Verdict |
|---|---|
| **A: Use ImGui backends directly in app code** — Each app includes `imgui_impl_sdl3.h` and `imgui_impl_opengl3.h` directly, calls `Init`, `NewFrame`, `Render`, `Shutdown` in its own frame loop. | **Rejected.** Violates CONST-001 (SDL3 and OpenGL backend headers would appear in `src/cmd/`). Duplicates boilerplate across every app. No central lifecycle management. App base class would need to change to provide hooks. |
| **B: Write custom ImGui backends on top of engine abstractions** — Implement `ImGui_ImplBuddd_Init`, `ImGui_ImplBuddd_NewFrame`, `ImGui_ImplBuddd_RenderDrawData` using only `Platform`, `Window`, `RenderDevice` abstract interfaces. | **Rejected.** ImGui's backend interface is deeply tied to raw SDL event types (`SDL_Event`, `SDL_Keycode`, `SDL_Scancode`) and raw OpenGL state manipulation (`glUseProgram`, `glBindVAO`, `glBindBuffer`, shader compilation). Abstracting these would require either (a) exposing SDL event internals through the engine abstraction (CONST-001 violation), or (b) writing a full SDL3 event parser + OpenGL state machine in the engine layer (disproportionate effort, ~2× the size of the official backends). Additionally, `RenderDevice` does not expose low-level GL operations (raw program/buffer binding), so a custom backend would either need a new `RenderDevice` API or bypass it entirely. |
| **C: Hybrid — embed official backends in engine, expose clean API** (chosen) | **Accepted.** Embed the unmodified `imgui_impl_sdl3` and `imgui_impl_opengl3` backends inside `src/engine/imgui/`. Wrap them in a minimal public API (`engine_imgui::init`, `shutdown`, `new_frame`, `render`, `on_sdl_event`, `is_initialized`). ImGui sources are fetched via `FetchContent`. Backend headers stay inside `src/engine/`. The public API forward-declares SDL types without including SDL3 headers. Frame lifecycle is automated inside `RenderDevice::begin_frame()` and `RenderDevice::render_ui()`. Event routing is automated inside `PlatformSDL3::poll_events()`. |

## Decision

### Decision 1: Embed official ImGui SDL3 + OpenGL3 backends as an engine-internal module

ImGui's unmodified `imgui_impl_sdl3` and `imgui_impl_opengl3` backends are placed inside `src/engine/imgui/backends/` (sourced from the `FetchContent` download directory). The wrapper module `engine_imgui` (`src/engine/imgui/engine_imgui.h/.cpp`) provides a minimal public API in namespace `buddd::engine::engine_imgui`:

```
auto init(SDL_Window*, void* gl_context) -> Result<void>;
auto shutdown() -> void;
auto new_frame() -> void;
auto render() -> void;
auto on_sdl_event(SDL_Event const&) -> bool;
auto is_initialized() -> bool;
```

- `engine_imgui.h` forward-declares `SDL_Window` and `SDL_Event` — it does NOT include any SDL3 or OpenGL headers, preserving CONST-001.
- `engine_imgui.cpp` includes the SDL3 and OpenGL backend headers (inside `src/engine/`, so CONST-001 compliant).
- File-scope `static` state (`s_context`, `s_initialized`) keeps the module self-contained with no global state pollution.
- Apps include only `<imgui.h>` (from the ImGui library) — they use `ImGui::Begin()`/`End()` directly, with no wrapper.

### Decision 2: Init inside `RenderDevice::create()`, shutdown in `~RenderDeviceOpenGL()`

**Init**: Called inside `RenderDevice::create()` (in `render_device.cpp`) in the display-available branch, after `SDL_GL_MakeCurrent` succeeds and after `new RenderDeviceOpenGL(...)` completes. Both `sdl_window` and `gl_context` are available as local variables — no public GL context accessor is exposed.

**Shutdown**: Called in `RenderDeviceOpenGL::~RenderDeviceOpenGL()` (in `render_device_opengl.cpp`) **before** `SDL_GL_DestroyContext(context_)`, ensuring the GL context is still current during ImGui backend cleanup. This respects the destruction ordering established by ADR-012 (GL context outlives ImGui resources).

**Init failure is non-fatal**: If `engine_imgui::init()` returns an error, a warning is logged and the engine continues without ImGui. `is_initialized()` returns `false`, and all lifecycle methods become no-ops.

> **AMENDED by ADR-027 (Editor Architecture)**: ImGui init failure in the SDL3/display path is now **fatal** — `RenderDevice::create()` propagates the error instead of logging a warning. Headless mode is unaffected (ImGui init is not called). See ADR-027 Decision 5 for details.

### Decision 3: Frame lifecycle automated via `RenderDevice` hooks

ImGui's frame lifecycle is integrated into the engine's existing `RenderDevice` and `run_app()` architecture with **zero ImGui-specific code in `run_app()`**:

1. **`engine_imgui::new_frame()`** is called from within `RenderDeviceOpenGL::begin_frame()` — after `glClear()` and before returning. This means the new ImGui frame starts automatically whenever `begin_frame()` is called.

2. **A new virtual method `RenderDevice::render_ui() -> void {}`** is added to the `RenderDevice` base class with a default no-op body. `RenderDeviceOpenGL` overrides it to call `engine_imgui::render()`. `RenderDeviceHeadless` inherits the no-op default — no changes needed.

3. **`run_app()`** calls `device.render_ui()` as a single generic line after `app.on_render(ctx)` and before the capture/`read_pixels` block. This is not ImGui-specific — it is a generic "render any UI overlay" hook.

4. **Because `render_ui()` is called before `read_pixels()`**, the ImGui overlay appears in captured screenshots.

### Decision 4: Event routing automated inside `PlatformSDL3::poll_events()`

`engine_imgui::on_sdl_event(event)` is called unconditionally (regardless of return value) inside the `SDL_PollEvent` loop in `PlatformSDL3::poll_events()`, after `input_system_.on_sdl_event(event)`. The return value is intentionally ignored — `InputSystem` continues to see all events for its own state tracking. Apps that wish to check `ImGui::GetIO().WantCaptureMouse` / `WantCaptureKeyboard` may do so themselves.

### Decision 5: ImGui fetched via `FetchContent` from the docking branch

ImGui is declared via `FetchContent` in `src/engine/CMakeLists.txt`:
```cmake
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8-docking
)
FetchContent_MakeAvailable(imgui)
```

ImGui library sources (`imgui.cpp`, `imgui_demo.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`) and backend sources (`imgui_impl_sdl3.cpp`, `imgui_impl_opengl3.cpp`) are added to the `buddd_engine` target via `target_sources()` in `src/engine/imgui/CMakeLists.txt`, referencing the `FetchContent` download directory.

## Consequences

### Positive

- **CONST-001 preserved**: SDL3 and OpenGL backend headers live entirely inside `src/engine/`. `engine_imgui.h` uses forward declarations only. `src/cmd/` code includes only `<imgui.h>` (the public ImGui header, which does not include SDL3 or OpenGL headers).
- **No boilerplate for apps**: Apps call `ImGui::Begin()`/`End()` in `on_render()` and get automatic frame lifecycle, event routing, init, and shutdown. No per-app backend setup needed.
- **`run_app()` stays generic**: The only addition is `device.render_ui()` — one generic line with no ImGui includes or type references.
- **`App` base class unchanged**: No new virtual methods were added to the `App` interface. Apps that never touch ImGui are unaffected.
- **Backends remain unmodified**: Official ImGui backends are embedded as-is. Updates can be pulled by changing the `FetchContent` tag. No fork maintenance.
- **Non-fatal init failure**: If ImGui init fails (e.g., shader compilation error), the engine continues without it. Lifecycle methods are guarded by `is_initialized()`.
- **Capture includes ImGui**: `render_ui()` is called before `read_pixels()`, so ImGui overlays appear in captured frames.
- **Headless safe**: `engine_imgui::init()` is never called in headless mode (the code path is inside the display-available branch). `is_initialized()` is always false. All lifecycle methods are no-ops.
- **Shutdown ordering guaranteed**: `~RenderDeviceOpenGL()` calls `engine_imgui::shutdown()` before `SDL_GL_DestroyContext()`, ensuring the GL context is current during ImGui resource cleanup (per ADR-012 destruction ordering).

### Negative

- **New dependency**: ImGui docking branch is fetched at configure time via `FetchContent`. Build time increases due to ImGui source compilation. Network access required for first build.
- **File-scope static state**: `engine_imgui.cpp` uses file-scope `static` variables (`s_context`, `s_initialized`). This is an exception to the project's preference for explicit state management, justified by the module's self-contained nature and the fact that there is exactly one ImGui context per process.
- **`RenderDevice` gains a non-rendering virtual method**: `render_ui()` is not a traditional rendering operation — it draws UI overlay data. Adding it to `RenderDevice` slightly broadens the class's responsibility. The default no-op body minimises the impact on headless and future backends.
- **Backend update coupling**: When updating the ImGui version, the backend sources must remain API-compatible with the engine's SDL3 and OpenGL usage. Major ImGui releases may require backend API changes.
- **No multi-viewport support in this scope**: Multi-viewport (`ImGuiConfigFlags_ViewportsEnable`) is explicitly not supported. Adding it in the future would require window management changes and is a separate ADR.
- **No input event filtering**: The engine does not suppress InputSystem state when ImGui captures mouse/keyboard. Apps that use both ImGui and engine input must check `ImGui::GetIO().WantCaptureMouse` / `WantCaptureKeyboard` themselves.

### Preserved invariants

- CONST-001: No SDL3/OpenGL/GLM headers outside `src/engine/`. Verified by `grep -rnE '#include.*(SDL3|SDL_opengl|GL/|glad)' src/cmd/` — zero matches.
- ADR-012: Destruction ordering preserved — `engine_imgui::shutdown()` called before `SDL_GL_DestroyContext()` in `~RenderDeviceOpenGL()`.
- ADR-003: Draw methods remain `void`. `render_ui()` returns `void`. `Platform::poll_events()` unchanged.
- ADR-014: Demo app is a scene in `main.cpp` dispatch (`buddd run imgui-demo`), not a standalone binary.
- ADR-001: `engine_imgui::init()` returns `Result<void>` using the existing `Error` struct. Other API functions return `void` or `bool`.
- App base class unchanged — no new virtual methods.
- `run_app()` unchanged apart from one generic `device.render_ui()` call.

## Related documents

- SPEC-026 (`.specs/sprint-2026-06/imgui-demo/spec.md`): Spec-level documentation of the ImGui integration requirements and acceptance criteria.
- IMPL-026 (`.specs/sprint-2026-06/imgui-demo/implementation-contract.md`): Contract-level implementation details.
- ADR-019 (`docs/adr/ADR-019-architecture-boundaries.md`): Architecture boundary (CONST-001) — preserved by forward-declaring SDL types in the public API.
- ADR-012 (`docs/adr/ADR-012-navigable-object-graph-engine-service.md`): EngineService destruction ordering — ImGui shutdown before GL context destruction.
- ADR-003 (`docs/adr/ADR-003-render-pipeline-architecture.md`): Draw method `void` convention — `render_ui()` follows the same pattern.
- ADR-014 (`docs/adr/ADR-014-cli-app-system.md`): CLI app system — demo is a scene, not a separate binary.
- ADR-001 (`docs/adr/ADR-001-result-error-pattern.md`): `Result<T>` error pattern — used by `engine_imgui::init()`.
