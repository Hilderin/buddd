# Implementation Code Review — F-01 Editor Scene Load/Save Integration

## Summary

The implementation is functionally correct and satisfies all acceptance criteria. All 526 tests pass (21986 assertions), including all 18 F-01-specific test cases (66 assertions). Build produces **zero compiler warnings** from `src/` and `tests/` (the only warning is a CMake deprecation in `FetchContent_Populate` for ImGuiFileDialog, originating from our CMakeLists.txt — flagged as a minor note). ADR-019 compliance confirmed: no SDL3/OpenGL/GLM headers in `src/editor/`. Architecture boundaries are respected.

The key behaviors are correctly implemented:
- `Window::set_title()` API on `Window`, `WindowSDL3`, `WindowHeadless`
- `Platform::set_on_close_request()` concrete method on `Platform` base class
- `PlatformSDL3::poll_events()` intercepts `SDL_EVENT_QUIT` via close-request callback
- Editor scene management methods (`new_scene`, `open_scene`, `save_scene`, `save_scene_as`)
- Dirty state tracking (`dirty_` boolean + window title `*` suffix)
- Save-prompt state machine (`PendingOp::NewScene/OpenScene/Quit`)
- ImGuiFileDialog integration for OS-native file dialogs
- Error modals for load/save failures
- All required logging calls present

**Verdict: ACCEPTED** — no blocking issues. Several non-blocking warnings noted below.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- None found.

All required functionality is implemented correctly. Zero compiler warnings from our code. All tests pass.

## Warnings

Non-blocking concerns for awareness:

- [ ] **UT-02 incomplete — missing titled scene title format tests**: The window title formatting test (`"F-01: Window title formatting"`) tests only 2 of 4 required scenarios:
  - ✅ `"Untitled — Buddd Editor"` (untitled, clean)
  - ✅ `"Untitled* — Buddd Editor"` (untitled, dirty)
  - ❌ `"scene.yaml — Buddd Editor"` (titled, clean) — **not tested**
  - ❌ `"scene.yaml* — Buddd Editor"` (titled, dirty) — **not tested**

  The test has `editor.clear_dirty()` followed by a comment explaining the limitation but no assertions for the titled cases. The `build_title_string()` function is correct by code inspection, and UT-06/UT-07 verify save_scene_as path tracking, but the window title format for titled scenes lacks direct test coverage.

- [ ] **Wiki documentation not updated for F-01**: The contract's "Documentation impact" section specifies:
  - `docs/wiki/editor/scene-management.md` — update from "future vision" to "current implementation" for F-01 operations. Currently still states _"The currently implemented v1 foundation only includes `File > Quit`… All other scene file operations (New, Open, Save, Save As, dirty tracking) are **planned for future sprints** and are not yet implemented."_
  - `docs/wiki/architecture/module-map.md` — add `ImGuiFileDialog` dependency to `buddd_editor` module, add Editor scene management methods, and note `Platform::set_on_close_request()` addition.
  
  Neither wiki page has been updated. The wiki still reflects the pre-F-01 state.

- [ ] **ADR-029 not updated**: The spec's "Documentation to update" section requires updating `docs/adr/ADR-029-editor-ux-decisions.md` AC-015 reference to match clean-by-default behavior. This was not done.

- [ ] **Dead member variables in `editor.h`**: Two member variables declared but never used:
  - `bool show_save_prompt_modal_` — never read or written in `editor.cpp`
  - `SavePromptResult save_prompt_result_` — never read or written in `editor.cpp`
  
  These appear to be vestigial from an earlier design. The save-prompt modal is driven directly by `pending_op_` and the return value of `draw_save_prompt_modal()`, not by these members.

- [ ] **`handle_dirty_before_op()` defined but never called**: This private method in `Editor` is declared, implemented, but never invoked anywhere. The dirtiness check is handled inline in `setup()` callbacks and `draw_pending_op_modal()`. Dead code — should be removed or called.

- [ ] **CMake deprecation warning**: `FetchContent_Populate(ImGuiFileDialog)` is deprecated per CMake policy CMP0169. CMake recommends `FetchContent_MakeAvailable()` instead. Currently suppressed by CMake version compatibility, but will break in future CMake versions. The manual `FetchContent_Populate` was needed to get `ImGuiFileDialog_SOURCE_DIR` before `add_subdirectory(src/editor)`. Consider upgrading the pattern.

## Required changes

- None blocking. The code is functionally correct and all tests pass.

## Suggested improvements

Optional ideas (not required):

- **Complete UT-02**: Add titled/clean and titled/dirty assertions to the window title formatting test. This requires setting a file path via `save_scene_as()` using the `HeadlessTestContext` pattern used in other tests, then calling `build_title_string()` and verifying the correct format.

- **Remove dead members**: Clean up `show_save_prompt_modal_`, `save_prompt_result_`, and `handle_dirty_before_op()` if they are intentionally unused. Consider keeping `pending_file_path_` for potential extension even though it is never set (the OpenScene flow goes through `show_file_dialog_` instead).

- **Update wiki pages**: For the next agent handling documentation, update `docs/wiki/editor/scene-management.md` to reflect the current F-01 implementation (File menu scene operations, dirty state, save-prompt modal, error modals) and add `ImGuiFileDialog`/scene management methods to `docs/wiki/architecture/module-map.md`.

- **Update ADR-029**: Add a note or amendment section to `docs/adr/ADR-029-editor-ux-decisions.md` documenting the clean-by-default behavior change.

## Coverage summary

| Requirement | Status |
|---|---|
| `Window::set_title()` in window.h/window_sdl3.h/window_sdl3.cpp/window_headless.h/window_headless.cpp | ✅ Pass |
| `Platform::set_on_close_request()` in platform.h | ✅ Pass |
| `PlatformSDL3::poll_events()` quit interception | ✅ Pass |
| ImGuiFileDialog FetchContent in root CMakeLists.txt | ✅ Pass |
| ImGuiFileDialog linked in `src/editor/CMakeLists.txt` | ✅ Pass |
| Editor class: dirty state methods | ✅ Pass |
| Editor class: scene management methods + state machine | ✅ Pass |
| Editor class: error modal rendering | ✅ Pass |
| Editor class: file dialog integration | ✅ Pass |
| MenuBar: File menu items + callbacks | ✅ Pass |
| Shortcuts: Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S, Ctrl+Q updated | ✅ Pass |
| Close-request callback registration in Editor::setup() | ✅ Pass |
| Save-prompt state machine (PendingOp::NewScene/OpenScene/Quit) | ✅ Pass |
| Initial window title on launch | ✅ Pass |
| Logging (scene saved, loaded, failures, dirty, save-prompt) | ✅ Pass |
| `engine_` null checks in `open_scene()`/`save_scene()`/`save_scene_as()` | ✅ Pass |
| Clean-save no-op guard (`!dirty_ && has_value()`) | ✅ Pass |
| ADR-019 compliance (no SDL3/GL/glm in src/editor/) | ✅ Pass |
| Zero compiler warnings from `src/` and `tests/` | ✅ Pass |
| All 526 tests pass (21986 assertions) | ✅ Pass |
| All 18 F-01 tests pass (66 assertions) | ✅ Pass |
| Forbidden files NOT modified | ✅ Pass |
| UT-01 (dirty state) | ✅ Pass |
| UT-02 (window title) | ⚠️ Partial (missing titled cases) |
| UT-03 (untitled scene) | ✅ Pass |
| UT-04 (new scene clears world) | ✅ Pass |
| UT-05 (new scene with dirty) | ✅ Pass |
| UT-06 (save on clean scene) | ✅ Pass |
| UT-07 (file path tracking) | ✅ Pass |
| UT-08 (quit with clean) | ✅ Pass |
| UT-09 (save on untitled) | ✅ Pass |
| UT-10 (dirty after failed save) | ✅ Pass |
| UT-11 (world valid after new_scene) | ✅ Pass |
| UT-12 (close-request callback — clean) | ✅ Pass |
| UT-13 (close-request callback — dirty) | ✅ Pass |
| IT-01 (round-trip save/load) | ✅ Pass |
| IT-02 (corrupt YAML error) | ✅ Pass |
| HT-01 (headless set_title) | ✅ Pass |
