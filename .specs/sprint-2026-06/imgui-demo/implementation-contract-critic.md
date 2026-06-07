# Implementation Contract Review — EngineImGui Module: Dear ImGui Integration

## Blocking issues

Items that must be resolved before the artifact can be accepted.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

- [x] **B-01: Wrong include paths for `engine_imgui.h` in engine source files** — *RESOLVED*

  The contract previously used `#include "engine/imgui/engine_imgui.h"` in three engine files
  (`render_device_opengl.cpp`, `render_device.cpp`, `platform_sdl3.cpp`). All three have been
  changed to `#include "imgui/engine_imgui.h"`, which correctly resolves to
  `src/engine/imgui/engine_imgui.h` via the engine's PUBLIC include directory
  (`${CMAKE_CURRENT_SOURCE_DIR}` = `src/engine/`).

  Grep confirms zero remaining `#include "engine/imgui/..."` directives in the contract.

## Warnings

Non-blocking concerns for awareness:

- [ ] **W-01: `BUDDD_LOG_TAG("ImGui")` not shown in `engine_imgui.cpp` code sample**

  The conventions section (line 83) states: "Use `BUDDD_LOG_TAG("ImGui")` in `engine_imgui.cpp`."
  However, the code sample in Section 2 does not include this line. The logging macros
  (`BUDDD_LOG_INFO`, `BUDDD_LOG_ERROR`, `BUDDD_LOG_TRACE`) used in the implementation require a
  tag declaration via `BUDDD_LOG_TAG`. If the implementer follows the code sample literally
  without reading the conventions section, the file would fail to compile.

  **Recommendation**: Add `BUDDD_LOG_TAG("ImGui");` as the first line after `#include` directives
  in the `engine_imgui.cpp` code sample.

- [ ] **W-02: Missing `#include` directives in `engine_imgui.cpp` code sample**

  The implementation code in Section 2 (lines 141–194) shows the internal state and function
  bodies but does not list any `#include` directives. The implementer must infer the needed
  headers from the function calls:
  - `"imgui/engine_imgui.h"` (self-header, for the API declarations)
  - `<imgui.h>` (for `ImGui::CreateContext`, `ImGuiIO`, etc.)
  - `"imgui_impl_sdl3.h"` (for `ImGui_ImplSDL3_InitForOpenGL`, `ImGui_ImplSDL3_ProcessEvent`, etc.)
  - `"imgui_impl_opengl3.h"` (for `ImGui_ImplOpenGL3_Init`, `ImGui_ImplOpenGL3_Shutdown`, etc.)

  While these are deducible from context, listing them explicitly would remove ambiguity.

- [ ] **W-03: `engine_imgui::render()` null check deviates slightly from AC-006 wording**

  AC-006 states: "After `render()`, `ImGui::GetDrawData()` returns non-null but current draw
  data is consumed." The contract's implementation in Section 2 step 3 checks
  `ImGui::GetDrawData() != nullptr` before calling `RenderDrawData`. This null check is defensive
  and functionally correct, but the AC wording implies the data should always be non-null after
  `ImGui::Render()`. The null check is acceptable practice, but the implementer should be aware
  that on valid paths `GetDrawData()` should always return non-null after `Render()`.

- [ ] **W-04: CMake glob picks up `engine_imgui.cpp` automatically**

  The existing `src/engine/CMakeLists.txt` uses `file(GLOB_RECURSE ...)` which collects ALL `.h`
  and `.cpp` files under `src/engine/`. This means `src/engine/imgui/engine_imgui.cpp` will be
  compiled as part of `buddd_engine` even without the `target_sources()` call in
  `src/engine/imgui/CMakeLists.txt`. The explicit `target_sources()` addition in Section 11 is
  redundant but harmless (CMake deduplicates). No action needed, but be aware the glob handles
  the engine-side file automatically.

## Required changes

None — the single blocking issue (B-01) has been fully resolved. The contract is complete and
implementable.

## Suggested improvements

Optional ideas (not required):

- Add `#include` directives to the `engine_imgui.cpp` code sample for completeness.
- Consider whether the `render()` null check for `GetDrawData()` should log a warning or trace
  message when null is unexpectedly returned, to aid debugging.
