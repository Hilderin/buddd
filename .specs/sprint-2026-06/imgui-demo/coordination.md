# Workflow Coordination: imgui-demo

## Orchestrator

**Feature**: `imgui-demo`
**Status**: completed
**Current step**: completed
**Initial instructions**: Implement imgui in the engine so it can be used in apps and eventually the editor. Use a hybrid approach: embed official ImGui SDL3+OpenGL3 backends inside `src/engine/imgui/` as an engine-internal module, with the frame lifecycle (new_frame/render) and event routing handled automatically by the engine. Apps use ImGui::Begin()/End() directly.

**Notes**:

### Spec-critic resolutions (human-approved)

- **Init/shutdown location**: Init called inside `RenderDevice::create()` (sdl_window + gl_context available locally). Shutdown called in `RenderDeviceOpenGL::~RenderDeviceOpenGL()` or `~EngineService()` before device destruction. No GL context accessor exposed publicly.
- **Window height**: 720px (matching user-visible behavior section).
- **Standalone binary**: Removed. Demo app is a scene (`buddd run imgui-demo`), NOT a separate binary. No duplicate main() issue.
- **AC-027 (GL state leakage)**: Removed entirely. ImGui's documented state save/restore is not the engine's responsibility to test.

### Revised design — deeper embedding (human-directed)

After the spec-critic loop, the human directed a deeper integration:
- **`engine_imgui::new_frame()`** is called from inside `RenderDeviceOpenGL::begin_frame()` — not from `run_app()`.
- **`engine_imgui::render()`** is called from inside a new `RenderDevice::render_ui()` virtual method (default no-op). `run_app()` calls `device.render_ui()` as a generic step — not ImGui-specific.
- **Capture timing**: `device.render_ui()` is called BEFORE `read_pixels()`/capture, so ImGui overlay appears in screenshots.
- **`run_app()`** has zero ImGui includes and zero ImGui-specific function calls.
- **`RenderDevice`** gains one new pure virtual: `auto render_ui() -> void`.
- Event routing in `PlatformSDL3::poll_events()` is unchanged from the original plan.

### Decisions from grill-me (Definition of Ready walkthrough)

1. **Approach**: Hybrid — official ImGui SDL3/OpenGL3 backends embedded in `src/engine/imgui/`. Engine handles boilerplate (init, frame lifecycle, event routing, shutdown). Apps use ImGui directly.
2. **Frame lifecycle**: `engine_imgui::new_frame()` called before `app.on_frame_begin()`, `engine_imgui::render()` called after `app.on_render()` — both inside `run_app()`. Apps never call these.
3. **Event routing**: Engine platform feeds SDL events to `engine_imgui::on_sdl_event()` internally. Apps never touch this.
4. **ImGui configuration (fonts, ini, style)**: Owned by the app in its `setup()`. Engine only initializes backends.
5. **Headless mode**: Engine skips ImGui init when no display. `new_frame()`/`render()` become no-ops. Apps simply don't call ImGui functions in headless mode.
6. **ImGui version**: Docking branch (latest docking release).
7. **Verification**: Demo app (`src/cmd/apps/imgui_demo/`) with screenshot capture. Shows `ImGui::ShowDemoWindow()` + a custom panel. Runnable with `--capture` flag for CI.
8. **Demo app scope**: Part of this delivery — `buddd_imgui_demo` binary.
9. **ADR**: Yes, create an ADR documenting the architecture decision.
10. **Feature name**: `imgui-demo`

## spec-author

**Status**: completed
**Summary**:
Fixed all 5 blocking issues and incorporated human-directed deeper embedding:
B-01: Window height corrected to 720.
B-02/B-03: Init inside RenderDevice::create(), shutdown in ~RenderDeviceOpenGL.
B-04: Standalone binary removed, scene-only approach.
B-05: AC-027 removed.
Plus: new_frame inside RenderDevice::begin_frame(), render inside new
RenderDevice::render_ui() virtual method, run_app() has zero ImGui-specific code.
G-02, AC-010, AC-011, frame lifecycle section, and API table all updated.
**Artifacts**:
- `.specs/sprint-2026-06/imgui-demo/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
B-06 resolved. The `render_ui()` table entry now correctly says "non-pure virtual with a default no-op body", and AC-011 says "virtual method with default no-op body". Both C++ contradictions fixed. A minor leftover inconsistency remains on line 67 (intro still says "pure virtual") — documented as a warning. No blocking issues remain. Spec is ready for implementation contract authoring.
**Artifacts**:
- `.specs/sprint-2026-06/imgui-demo/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- **Line 67 leftover**: "A new pure virtual method is added" still contradicts the corrected table entry (says "pure virtual" instead of "virtual"). The table entry is authoritative and correct; line 67 should be updated for consistency. Not a blocking issue.
- W-03: Demo window 1280×720 differs from default 1024×768 used by all other apps (intentional but undocumented)
- W-06: Shutdown location has binary "or" (RenderDeviceOpenGL dtor vs. EngineService) — implementer must pick one
- W-07: AC-007 tests ImGui internal backend behavior (mouse events not consumed); may break on ImGui version update
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Created prescriptive implementation contract for the EngineImGui module.
Covers 11 files to modify/create with exact API signatures (engine_imgui.h),
detailed implementation notes per file (init/shutdown/new_frame/render/on_sdl_event),
build system changes (FetchContent for ImGui docking branch v1.91.8-docking),
demo app (ImguiDemoApp with Phong-lit orbiting cube + ImGui overlay),
and 20 concrete done criteria. Resolved the spec-critic W-06 shutdown ambiguity
by pinning shutdown to ~RenderDeviceOpenGL() before SDL_GL_DestroyContext.
**Artifacts**:
- `.specs/sprint-2026-06/imgui-demo/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- The ImGui docking branch tag `v1.91.8-docking` may need updating if a newer release
  is available when implemented. The contract pins this version, but the implementer
  should verify the tag exists.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
B-01 (wrong include paths) fully resolved — all three engine-side includes now use
`#include "imgui/engine_imgui.h"`. Zero remaining `#include "engine/imgui/..."` references
in the contract. Four non-blocking warnings (W-01 through W-04) remain valid but do not
block implementation. Contract is complete and implementable.
**Artifacts**:
- `.specs/sprint-2026-06/imgui-demo/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- W-01: `BUDDD_LOG_TAG("ImGui")` not shown in `engine_imgui.cpp` code sample
- W-02: Missing `#include` directives in `engine_imgui.cpp` code sample
- W-03: `engine_imgui::render()` null check deviates slightly from AC-006 wording
- W-04: CMake glob picks up `engine_imgui.cpp` automatically, making explicit `target_sources()` redundant
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-07 19:20:16 EDT
**Warnings**:
none
**Notes**: Approved with simplified demo (no 3D scene, just ImGui overlay on cleared background). Init location confirmed: inside RenderDevice::create() where sdl_window and gl_context are local variables.

## code-implementer

**Status**: completed
**Summary**:
Implemented the EngineImGui module embedding Dear ImGui (docking branch v1.91.8-docking) via FetchContent. Created `engine_imgui.h/.cpp` with init/shutdown/new_frame/render/on_sdl_event/is_initialized API. Integrated frame lifecycle into `RenderDeviceOpenGL::begin_frame()` (new_frame after clear) and new `RenderDevice::render_ui()` virtual (render before capture). Added event routing in `PlatformSDL3::poll_events()`. Created `ImguiDemoApp` as an `"imgui-demo"` scene with ShowDemoWindow + custom FPS panel. Modified CMakeLists.txt for ImGui dependency and `add_subdirectory(imgui)`.
**Artifacts**:
- `src/engine/imgui/engine_imgui.h` (new)
- `src/engine/imgui/engine_imgui.cpp` (new)
- `src/engine/imgui/CMakeLists.txt` (new)
- `src/engine/CMakeLists.txt` (modified)
- `src/engine/render/render_device.h` (modified)
- `src/engine/render/render_device_opengl.h` (modified)
- `src/engine/render/render_device_opengl.cpp` (modified)
- `src/engine/render/render_device.cpp` (modified)
- `src/engine/platform/platform_sdl3.cpp` (modified)
- `src/cmd/app.cpp` (modified)
- `src/cmd/main.cpp` (modified)
- `src/cmd/apps/imgui_demo_app.h` (new)
- `src/cmd/apps/imgui_demo_app.cpp` (new)
**Questions for human**:
- DC-13 (`buddd run imgui-demo --frame 10`) and DC-14 (`--capture`) require a display to execute. Need manual verification on display-enabled runner.
**Warnings**:
- Changed `target_include_directories` from `PRIVATE` to `PUBLIC` in `src/engine/imgui/CMakeLists.txt` so the `buddd` executable can find `<imgui.h>` via the `buddd_engine` static library link (cannot modify `src/cmd/CMakeLists.txt` as it's in the forbidden list).
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
All implementation-level done criteria (DC-01 through DC-17) are satisfied. Build succeeds with zero warnings from our code, all 425 existing tests pass, CONST-001 is preserved, and all 30 spec ACs are addressed (AC-027/028/029 are assigned to subsequent workflow agents). Three minor deviations from the contract (PUBLIC vs PRIVATE include dirs, include ordering, init-before-device-construction) are documented as warnings. No blocking issues found at the implementation level.
**Artifacts**:
- `.specs/sprint-2026-06/imgui-demo/code-review.md`
**Questions for human**:
- DC-14 and AC-018/AC-021 require a display runner to visually verify the ImGui overlay appears in captured frames. Manual verification needed.
**Warnings**:
- `target_include_directories` changed from PRIVATE to PUBLIC in `src/engine/imgui/CMakeLists.txt` to let the `buddd` target resolve `<imgui.h>` — justified by forbidden-file constraint on `src/cmd/CMakeLists.txt`
- `imgui.ini` file appears in project root when running apps that don't set `io.IniFilename` (per spec: apps control INI, engine does not manage it)
- Include order in `render_device_opengl.cpp` differs from the contract's prescribed position
- `engine_imgui::init()` is called before `new RenderDeviceOpenGL(...)`, deviating from AC-013's wording ("after new...succeeds") but functionally equivalent
**Blocking issues**:
- [ ] None found at the implementation level. AC-027 (ADR), AC-028 (wiki module map), AC-029 (wiki data flow) are assigned to governance-reviewer and wiki-agent respectively.

## wiki-agent

**Status**: completed
**Summary**:
Updated the operational wiki to document the new EngineImGui module per AC-028 and
AC-029. Added the `imgui/` submodule to module-map.md with dependencies, key files,
and integration points. Updated data-flow.md with the full ImGui frame lifecycle
(new_frame inside begin_frame, render_ui virtual method, render after on_render,
shutdown in destructor) and event routing (on_sdl_event in poll_events). Also
updated dependency-map.md (ImGui FetchContent entry), architecture overview
(directory listing + key behaviors + target description), and adr-index.md (ADR-026
reference).
**Artifacts**:
- `docs/wiki/architecture/module-map.md` (modified)
- `docs/wiki/architecture/data-flow.md` (modified)
- `docs/wiki/architecture/dependency-map.md` (modified)
- `docs/wiki/architecture/overview.md` (modified)
**Changes made**:
- module-map.md: Added ImGui submodule section with file listing, dependencies,
  and integration summary. Updated platform_sdl3.cpp, render_device.h/.cpp,
  render_device_opengl.h/.cpp entries. Added ImguiDemoApp to app subclasses
  and imgui-demo scene to subcommand behavior.
- data-flow.md: Added ImGui init note to EngineService lifecycle diagram.
  Updated frame loop with render_ui() step and new_frame() in begin_frame.
  Added dedicated "ImGui integration hooks" section with ascii diagrams for
  init/shutdown/poll_events/frame_loop. Added SPEC-026 and ADR-026 references.
- dependency-map.md: Added ImGui as buddd_engine dependency (PRIVATE, FetchContent
  docking branch v1.91.8-docking) and external dependency entry.
- overview.md: Added imgui/ to directory layout and engine internal structure.
  Updated `buddd_engine` target description and external dependencies list.
  Added imgui-demo to key behaviors.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance validation passed. All 30 ACs are satisfied (AC-018/AC-021
require display runner for visual verification — known limitation). ADR-026 exists
with status "Accepted" and is consistent with spec/contract/code/wiki. CONST-001
preserved (verified by grep). Wiki fully updated: module-map.md, data-flow.md,
dependency-map.md, overview.md all reflect the implementation. No blocking issues
found at the governance level. Seven non-blocking warnings carried forward.
**Artifacts**:
- `.specs/sprint-2026-06/imgui-demo/governance-review.md`
**Questions for human**:
none
**Warnings**:
- Init called before device construction (deviation from AC-013 wording, functionally equivalent, human-approved)
- PUBLIC vs PRIVATE include dirs in imgui/CMakeLists.txt (justified by forbidden-file constraint)
- `imgui.ini` file in project root when apps don't set io.IniFilename
- AC-018/AC-021 require display runner for visual verification
- Contract code sample missing BUDDD_LOG_TAG (implementer correctly inferred it)
- Demo app window size 1280×720 differs from other apps' 1024×768 (intentional, undocumented)
- AC-007 tests ImGui internal backend behavior (may break on ImGui version update)
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
Created ADR-026 documenting the architecture decision to embed Dear ImGui
(docking branch) as an engine-internal module at `src/engine/imgui/`.
Documents the chosen hybrid approach (embed official SDL3+OpenGL3 backends,
expose clean API, automate frame lifecycle/events), three alternatives
considered, and key consequences including CONST-001 compliance, frame
lifecycle hooks in RenderDevice, and FetchContent dependency management.
**Artifacts**:
- `docs/adr/ADR-026-imgui-integration.md`
**Decisions needed**:
none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
