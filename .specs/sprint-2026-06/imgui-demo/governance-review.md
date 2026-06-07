# Governance Review — EngineImGui Module: Dear ImGui Integration

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Init location**: Spec AC-013 says init called "after new RenderDeviceOpenGL succeeds". Implementation calls init *before* device construction (but after GL context is current). Functionally equivalent and human-approved. No blocking contradiction.
- [x] **render_ui() signature**: Former pure-virtual/non-pure-virtual contradiction (B-06) resolved. All documents now consistently describe `render_ui()` as a non-pure virtual with default no-op body `{}`.
- [x] **Shutdown location**: Spec had binary "or" (RenderDeviceOpenGL dtor vs EngineService). Implementation contract pinned to `~RenderDeviceOpenGL()`. Code follows contract. All docs now consistent.
- [x] **Line 67 "pure virtual" leftover** (spec-critic B-06 remainder): Current spec line 67 correctly reads "virtual method with a default no-op body" — this was fixed. No remaining inconsistency.
- [x] **Contract `#include` paths**: All three engine-side includes use `#include "imgui/engine_imgui.h"` (was B-01, resolved).
- [x] **CONST-001 preservation**: AC-030 verified by grep — zero SDL3/OpenGL/GL headers in `src/cmd/`. Consistent across all documents.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-026** (`docs/adr/ADR-026-imgui-integration.md`): Created with status "Accepted". Documents the chosen hybrid approach (embed official backends, expose clean API), all five key decisions (init/shutdown locations, frame lifecycle, event routing, FetchContent), alternatives considered, and consequences. Consistent with spec, contract, and implementation.
- [x] **ADR-019 (CONST-001)**: Preserved — `engine_imgui.h` forward-declares SDL types, no SDL3/OpenGL/GLM headers outside `src/engine/`. Verified by AC-030 grep.
- [x] **ADR-012 (EngineService destruction ordering)**: ImGui shutdown called before `SDL_GL_DestroyContext` in `~RenderDeviceOpenGL()`, preserving the context-lifetime invariant.
- [x] **ADR-003 (void draw methods)**: `render_ui()` returns `void`, following the established draw-method convention.
- [x] **ADR-014 (CLI App System)**: Demo app registered as `"imgui-demo"` scene in `main.cpp` dispatch, not a standalone binary. No changes to `App` base class.
- [x] **ADR-001 (Result/Error pattern)**: `init()` returns `Result<void>` using existing `Error::Category` values (`InitFailed`, `InvalidArgument`). Other API functions return `void` or `bool`.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **module-map.md** (`docs/wiki/architecture/module-map.md`): Contains `## ImGui submodule` section (lines 142-156) correctly documenting namespace, API, file listing, dependencies (FetchContent docking branch v1.91.8-docking), frame lifecycle hooks, event routing, headless behavior. Platform section references `engine_imgui::on_sdl_event()`. Scene dispatch list includes `imgui-demo`. All accurate.
- [x] **data-flow.md** (`docs/wiki/architecture/data-flow.md`): EngineService lifecycle diagram shows ImGui init after `SDL_GL_MakeCurrent` (lines 126-129). Frame loop (lines 181-217) correctly shows new_frame inside begin_frame, render_ui between on_render and capture. Dedicated "ImGui integration hooks" section (lines 488-537) with ASCII diagram covering init/shutdown/poll_events/frame_loop. Scene dispatch updated (line 41). References SPEC-026 and ADR-026. All accurate.
- [x] **dependency-map.md** (`docs/wiki/architecture/dependency-map.md`): ImGui listed as PRIVATE dependency of `buddd_engine` (line 27), FetchContent entry (line 38) with correct version/tag. All accurate.
- [x] **overview.md** (`docs/wiki/architecture/overview.md`): Key behavior for `imgui-demo` scene (line 177) correctly describes 1280×720 window, ImGui overlay, `--capture` support. All accurate.
- [x] **Wiki does not become law**: All wiki content references SPEC-026 and ADR-026 as authoritative sources. No contradictory rules established in wiki alone.

## Warnings

Non-blocking concerns for awareness:

- **Init before device construction** (carried from code-review W-04): `engine_imgui::init()` called before `new RenderDeviceOpenGL(...)`, deviating from AC-013's exact wording. Functionally equivalent since GL context is already current. Human-approved in coordination.md.
- **PUBLIC vs PRIVATE include dirs** (carried from code-review W-01): `src/engine/imgui/CMakeLists.txt` uses PUBLIC instead of contract's PRIVATE. Justified by `src/cmd/CMakeLists.txt` being in the forbidden file list — the `buddd` target needs the ImGui include path to compile `imgui_demo_app.cpp`.
- **`imgui.ini` file in project root** (carried from code-review W-02): Apps that don't set `io.IniFilename` get an `imgui.ini` file at CWD. Per spec design (apps control ini). Demo app correctly disables it. Side-effect to be aware of.
- **Visual verification requires display** (carried from code-review W-05): AC-018/AC-021 require a display-enabled runner to verify ImGui overlay appearance in captured frames. Cannot be verified in headless CI.
- **Contract code sample missing `BUDDD_LOG_TAG`** (carried from contract-critic W-01): The `engine_imgui.cpp` code sample in the implementation contract doesn't show the mandatory `BUDDD_LOG_TAG("ImGui")`. Implementer correctly inferred it.
- **Demo app window size differs from other apps** (carried from spec-critic W-03): 1280×720 vs 1024×768 default. Intentional but rationale is implicit.
- **AC-007 tests ImGui internal backend behavior** (carried from spec-critic W-07): Tests that `on_sdl_event()` returns false for mouse motion events. May break if ImGui backend behavior changes in a future update.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None required. ADR-026 is created with status "Accepted". Wiki is fully updated. All governance artifacts are complete and consistent.
