# Spec Review — EngineImGui Module: Dear ImGui Integration

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01: AC-015 window height contradicts user-visible behavior description**
  **RESOLVED**: AC-015 now specifies height 720, matching the user-visible behavior section. The spec is consistent.

- [x] **B-02: No public accessor for SDL_GLContext — `engine_imgui::init()` cannot be called**
  **RESOLVED**: `engine_imgui::init()` is now called inside `RenderDevice::create()` where `sdl_window` and `gl_context` are available as local variables (AC-013, line 327). No public accessor is needed.

- [x] **B-03: Init/shutdown integration point is ambiguous — AC-013 vs. practical feasibility**
  **RESOLVED**: Init is called inside `RenderDevice::create()`. Shutdown is called in `RenderDeviceOpenGL::~RenderDeviceOpenGL()` (or `~EngineService()` before device cleanup). AC-013 has been updated to match this design.

- [x] **B-04: Standalone `buddd_imgui_demo` binary will cause duplicate `main()` symbol**
  **RESOLVED**: The standalone binary was removed. The demo app is now a scene (`"imgui-demo"`) dispatched from `main.cpp`, not a separate binary. No duplicate `main()` issue.

- [x] **B-05: AC-027 verification is impractical — requires non-existent reference captures**
  **RESOLVED**: The old AC-027 (GL state leakage test) was removed. The ACs have been renumbered. The new AC-027 verifies the ADR file exists.

- [x] **B-06: `RenderDevice::render_ui()` described as both "pure virtual" AND having a "default no-op" — mutually exclusive in C++**
  **RESOLVED**: The entity table (line 71) now correctly says "non-pure virtual with a default no-op body". AC-011 (line 325) now says "virtual method with default no-op body". Both occurrences specified in the resolution are fixed.

  **Remaining minor inconsistency**: Line 67 still says "A new pure virtual method is added to the RenderDevice abstract interface:" — this contradicts the corrected table entry below it. The table entry is authoritative and correct, but the intro text should be updated to match for consistency. Suggested: change "pure virtual" to "virtual" on line 67. This is a documentation polish issue, not a C++ contradiction.

## Warnings

Non-blocking concerns for awareness:

- **W-01 (resolved)**: EngineService vs. run_app() lifecycle ownership — the human-approved resolution (init in `RenderDevice::create()`, shutdown in `~RenderDeviceOpenGL()`) resolves this. No longer a concern.

- **W-02 (resolved)**: `Result<void>` / C++23 compatibility — the project uses `CMAKE_CXX_STANDARD 26` (confirmed in `CMakeLists.txt:4`). `std::expected<void, Error>` is fully supported. No issue.

- **W-03 (carried forward): Demo app window size 1280×720 differs from default 1024×768**
  All existing demo apps use 1024×768 (the default `AppConfig`). The ImGui demo specifies 1280×720. The spec documents this explicitly (line 208), so it is clearly intentional. Minor inconsistency with other apps; documented but rationale is implicit.

- **W-04 (resolved)**: Old AC-027 (GL state leakage) was removed from the spec entirely. The concern about state leakage is delegated to ImGui's documented state save/restore guarantees. Acceptable for a third-party library integration.

- **W-05 (resolved)**: Error categories confirmed correct. No issue.

- **W-06 (new): Shutdown location has binary "or" — implementer must pick one**
  The spec (line 59, AC-013) says shutdown is called in `RenderDeviceOpenGL::~RenderDeviceOpenGL()` **or** `~EngineService()` before device cleanup. The human-approved resolution (coordination.md) permits either location, but the implementer must choose one. If shutdown is in `~RenderDeviceOpenGL()`, it runs before the GL context is destroyed (safe). If in `~EngineService()`, it runs before the device is destroyed (also safe). Recommend picking one and making it explicit to avoid ambiguity during code review.

- **W-07 (new): AC-007 tests ImGui internal backend behavior**
  AC-007 verifies that `on_sdl_event()` returns `false` for `SDL_EVENT_MOUSEMOTION` events. This tests `ImGui_ImplSDL3_ProcessEvent()` internal behavior, which may change across ImGui versions. If the ImGui backend future-updates to consume mouse motion events, this AC would break. Acceptable as a smoke test for the current integration.

## Required changes

Concrete, actionable changes requested:

1. **(NEW — B-06)** Resolve the contradiction in `RenderDevice::render_ui()` declaration. Either:
   - Change from "pure virtual" to a regular virtual method with default empty body (`virtual void render_ui() {}`), so `RenderDeviceHeadless` inherits the no-op default. **OR**
   - Keep it pure virtual (`= 0`) and require `RenderDeviceHeadless` to provide an explicit empty override.
   
   The preferred approach (matching the spec's stated intent that Headless inherits the no-op) is the first option: make it a non-pure virtual with default body `{}`. Update line 67 and the entity table accordingly.

## Suggested improvements

Optional ideas (not required):

- The spec is thorough and well-structured — 30 ACs, 18 edge cases, 7 error cases, 11 assumptions, and 6 resolved open questions show careful thinking.
- Consider adding the demo scene `"imgui-demo"` to the data-flow wiki page's scene dispatch list (currently out of date). This is tracked in AC-029 (wiki data flow update).
- B-06 is the sole remaining blocking issue. Once resolved, the spec should be ready for implementation contract authoring.
