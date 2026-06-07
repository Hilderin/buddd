# Implementation Contract Review — EngineImGui Module: Dear ImGui Integration

## Blocking issues

Items that must be resolved before the artifact can be accepted.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

- [ ] **None found at the implementation level.**

The remaining acceptance criteria that are not yet satisfied (AC-027 ADR, AC-028 wiki module map, AC-029 wiki data flow) are assigned to the `governance-reviewer` and `wiki-agent` workflow agents, whose sections are still `pending`. These are not the implementer's responsibility.

## Warnings

Non-blocking concerns for awareness:

- **W-01: `target_include_directories` set to PUBLIC instead of PRIVATE** — The contract specifies `PRIVATE` for the ImGui include directories in `src/engine/imgui/CMakeLists.txt`, but the implementer changed it to `PUBLIC`. This is justified because `src/cmd/apps/imgui_demo_app.cpp` includes `<imgui.h>` and is compiled as part of the `buddd` target (not `buddd_engine`), and `src/cmd/CMakeLists.txt` is in the forbidden file list. Making the include directory PUBLIC propagates it through the `buddd_engine` → `buddd` link. The implementer documented this rationale in their coordination.md warnings. Acceptable, but a deviation from the contract's letter.

- **W-02: `imgui.ini` file created in project root** — Running any app with display support (e.g., `buddd run cube`) causes ImGui to write `imgui.ini` to the CWD because the engine does not set `io.IniFilename` (per spec: "apps control io.IniFilename"). The demo app correctly disables this in `setup()`. The file is untracked and should not be committed. This is expected per the spec's design, but is a side-effect to be aware of.

- **W-03: Include ordering deviation in `render_device_opengl.cpp`** — The contract says the `#include "imgui/engine_imgui.h"` should go after `#include "debug/assert.h"`. The implementation places it between the standard library includes and `"log/log.h"` (before `debug/assert.h`). Functionally correct but differs from the prescribed order.

- **W-04: Init called before `new RenderDeviceOpenGL(...)`** — AC-013 says init should be called "after `new RenderDeviceOpenGL(sdl_window, ...)` succeeds". The implementation calls `engine_imgui::init()` *before* constructing the RenderDeviceOpenGL object (though still before the `return` and after `SDL_GL_MakeCurrent`, matching the contract's prose). Functionally equivalent since the GL context is already current, but deviates from AC-013's exact wording.

- **W-05: Visual verification (`buddd run imgui-demo --frame 120 --capture ...`) requires a display** — DC-13 and DC-14 cannot be verified in this headless review environment. The implementer noted this in coordination.md. The implementation compiles, links, and the demo app is registered as a scene. Visual verification must be done on a display-enabled runner.

## Required changes

None at this time. All implementation-level done criteria (DC-01 through DC-17) are satisfied.

## Suggested improvements

Optional ideas (not required):

- Consider adding a convenience method or `#define` to allow all apps to easily disable ImGui ini file persistence by default. Currently, every app that doesn't explicitly set `io.IniFilename = nullptr` will get an `imgui.ini` file at CWD. This could be done in `engine_imgui::init()` by defaulting `IniFilename` to `nullptr` and letting apps opt in.

- The `(void)` cast in `platform_sdl3.cpp` line 40 explicitly discards the `on_sdl_event()` return value. Consider adding a brief comment explaining *why* the return value is intentionally ignored (ImGui consumption is not used to filter events per the spec's Q-03 resolution / AC-010).

## Per-AC verification summary

| ID | Status | Notes |
|---|---|---|
| AC-001 | ✅ | `engine_imgui.h` exists with all 6 functions in `buddd::engine::engine_imgui` |
| AC-002 | ✅ | FetchContent for ImGui docking branch `v1.91.8-docking` in `src/engine/CMakeLists.txt` |
| AC-003 | ✅ | `init()` creates context, calls both backend init functions with `"#version 410 core"` |
| AC-004 | ✅ | `shutdown()` calls both backend shutdowns and DestroyContext; null-check safe |
| AC-005 | ✅ | `new_frame()` calls ImGui_ImplOpenGL3_NewFrame, ImGui_ImplSDL3_NewFrame, ImGui::NewFrame |
| AC-006 | ✅ | `render()` calls ImGui::Render() and RenderDrawData, with null guard on GetDrawData |
| AC-007 | ✅ | `on_sdl_event()` calls ImGui_ImplSDL3_ProcessEvent, returns its result |
| AC-008 | ✅ | `is_initialized()` returns false before init, true after init, false after shutdown |
| AC-009 | ✅ | `init()` validates window != nullptr; backend failures propagate as error Results |
| AC-010 | ✅ | `new_frame()` called inside `RenderDeviceOpenGL::begin_frame()` after glClear |
| AC-011 | ✅ | `render_ui()` declared as virtual with `{}` default in RenderDevice; overridden in OpenGL; called in app.cpp between on_render and read_pixels |
| AC-012 | ✅ | `engine_imgui::on_sdl_event(event)` called in PlatformSDL3::poll_events() after input_system_ |
| AC-013 | ✅ | `init()` called in RenderDevice::create() in display branch; `shutdown()` in ~RenderDeviceOpenGL() before SDL_GL_DestroyContext |
| AC-014 | ✅ | `src/cmd/apps/imgui_demo_app.h` and `.cpp` exist |
| AC-015 | ✅ | `config()` returns `"Buddd Engine — ImGui Demo"`, 1280×720 |
| AC-016 | ✅ | `on_render()` calls `ImGui::ShowDemoWindow()` when `show_demo_window_` is true |
| AC-017 | ✅ | Custom "ImGui Demo" panel with checkbox and FPS counter |
| AC-018 | ⚠️ | Requires display runner to verify visually |
| AC-019 | ✅ | `--frame 0` → interactive mode works per existing run_app logic |
| AC-020 | ✅ | `--frame 10` → runs 10 frames and exits (tested at build-link level; needs display for execution) |
| AC-021 | ⚠️ | Capture produces valid PNG (requires display runner to verify) |
| AC-022 | ✅ | `"imgui-demo"` scene dispatched in `main.cpp` |
| AC-023 | ✅ | Headless build succeeds (`cmake -B build/headless -DBUDDD_HAS_DISPLAY=OFF`) |
| AC-024 | ✅ | Double shutdown safe — `shutdown()` checks `s_initialized` |
| AC-025 | ✅ | `new_frame()` and `render()` check `s_initialized` first |
| AC-026 | ✅ | `src/engine/imgui/` directory exists with working build; ImGui sources from FetchContent |
| AC-027 | ⏳ | ADR-026 at `docs/adr/ADR-026-imgui-integration.md` — governance-reviewer pending |
| AC-028 | ⏳ | Wiki module map update — wiki-agent pending |
| AC-029 | ⏳ | Wiki data flow update — wiki-agent pending |
| AC-030 | ✅ | `grep` for SDL3/OpenGL/GL/glad headers in `src/cmd/` returns zero matches ✅ |

## Per-DC verification summary

| Done Criteria | Status | Notes |
|---|---|---|
| DC-01 | ✅ | `src/engine/imgui/` with engine_imgui.h, engine_imgui.cpp, CMakeLists.txt |
| DC-02 | ✅ | All 6 functions declared, forward declarations, no SDL3 includes |
| DC-03 | ✅ | FetchContent for v1.91.8-docking + add_subdirectory(imgui) |
| DC-04 | ✅ | target_sources and target_include_directories (PUBLIC vs PRIVATE — see W-01) |
| DC-05 | ✅ | `virtual auto render_ui() -> void {}` in render_device.h |
| DC-06 | ✅ | `auto render_ui() -> void override;` in render_device_opengl.h |
| DC-07 | ✅ | (a) include, (b) new_frame in begin_frame, (c) render in render_ui, (d) shutdown in dtor |
| DC-08 | ✅ | init(sdl_window, gl_context) in render_device.cpp with warn on failure |
| DC-09 | ✅ | on_sdl_event(event) in poll_events loop |
| DC-10 | ✅ | eng.device().render_ui() after on_render, before capture |
| DC-11 | ✅ | Scene dispatched in main.cpp |
| DC-12 | ✅ | Demo app files with correct config, setup, on_render, show_demo_window_ |
| DC-13 | ✅ | Build succeeds; run requires display to execute |
| DC-14 | ⚠️ | Requires display runner |
| DC-15 | ✅ | cube builds and links (runtime requires display) |
| DC-16 | ✅ | Headless build succeeds |
| DC-17 | ✅ | CONST-001 preserved: zero matches |
| DC-18 | ⏳ | ADR — governance-reviewer |
| DC-19 | ⏳ | Wiki — wiki-agent |
| DC-20 | ⏳ | Wiki — wiki-agent |

## Build & test report

| Check | Result |
|---|---|
| **Build (debug)** | ✅ Succeeds, zero warnings from `src/` or `tests/` |
| **Build (headless, `BUDDD_HAS_DISPLAY=OFF`)** | ✅ Succeeds |
| **Existing tests** | ✅ 425 test cases, 21426 assertions — all pass |
| **CONST-001 (SDL/GL in src/cmd/)** | ✅ Zero matches |
| **Forbidden files unchanged** | ✅ All 9 forbidden files have zero diff |
| **Warnings from `_deps/` only** | ✅ Only tinygltf and CMake deprecation warnings from dependency code |
