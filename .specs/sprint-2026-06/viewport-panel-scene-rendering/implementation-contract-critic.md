# Implementation Contract Review — Viewport Panel — Scene Rendering

## Summary

The contract is thorough, precise, and well-aligned with the accepted spec, existing code conventions, and ADRs. Every file change is accounted for, the architecture boundary is respected, edge cases are documented, and the implementation behaviour is unambiguous. No blocking issues found. Three warnings and three informational notes are raised for awareness.

## Strengths

- **Complete coverage**: All 24 acceptance criteria from the spec are traced to concrete implementation steps or tests.
- **Precise file boundaries**: Every file allowed/forbidden to change is listed. No ambiguity about what the Code Agent may touch.
- **Architecture boundary enforcement**: Explicit hard rule against SDL3/GL/GLM headers in editor code, with a verification step (`grep` command provided).
- **World replacement handled**: The `new_scene()`/`open_scene()` pointer-comparison approach in `draw_ui()` correctly detects World replacement and recreates the `RenderSystem`. The spec's edge case (EC-new_scene) is addressed.
- **Error handling**: FBO creation/resize failures, null FBO, zero-dimension panel, and headless mode are all explicitly handled with graceful degradation (error text overlay, skip render, no crash).
- **Refactoring discipline**: `render_impl()` extraction keeps the existing `render_scene()` API unchanged while enabling the new `render_scene_with_camera()` without duplication.
- **Test traceability**: 12 tests listed, each traced to specific ACs.

## Issues

### Blocking issues

- [ ] *(none — this contract is acceptable as-is)*

### Warnings (non-blocking)

- **W-01: Editor destructor ordering creates dangling `World*` in `RenderSystem`**  
  `editor.h` declares `panels_` (line 152) before `world_` (line 166). During `~Editor()`, members are destroyed in reverse declaration order, so `world_` (the underlying `World` object) is deleted **before** `panels_` (and thus before `ViewportPanel` and its `RenderSystem`). The `RenderSystem` holds a raw `World* world_` that becomes dangling during `ViewportPanel` destruction. This is currently safe because `~RenderSystem()` is compiler-generated and does not dereference the pointer. However, it is fragile — any future change that gives `RenderSystem` a non-trivial destructor that accesses `world_` will cause a use-after-free. Mitigation options: (a) reorder members in `editor.h` so `world_` is declared before `panels_` (but `editor.h` is in the forbidden-to-change list — this would require an exception), or (b) document this coupling explicitly in the code (`editor.h` comment or `Editor` destructor comment). The contract should at minimum warn the Code Agent to not add a non-trivial destructor to `RenderSystem` that touches `world_`.

- **W-02: `RenderSystem` move-assignment relies on implicit generation**  
  The contract's World-replacement approach uses `render_system_ = buddd::engine::RenderSystem(*device_, *editor_world_);`, which relies on compiler-generated move assignment. `RenderSystem` has user-declared constructor but no user-declared destructor, copy/move operations — the implicit move assignment is well-defined and copies the raw pointers. This is safe today but brittle: if someone later adds a `std::unique_ptr` member, deletes copy operations, or adds a destructor, the implicit move assignment is either deleted or behaves differently. Recommend either: (a) explicitly `= default` the move assignment operator in `render_system.h`, or (b) use a `std::unique_ptr<RenderSystem>` member in `ViewportPanel` instead, avoiding move-assignment entirely (and making the World replacement `reset()` + `make_unique()`).

- **W-03: Test #12 ("World pointer change") is underspecified**  
  The test description "World pointer change in `draw_ui()` triggers RenderSystem recreation" traces only to "Edge case: scene switch" with no setup details. In headless mode, testing this requires either: (a) constructing a mock `Editor` with a replaceable `World&`, or (b) using a test helper that allows injecting a new World and observing the `RenderSystem`'s bound world. The contract should specify how to verify that the `RenderSystem` was recreated (e.g., check that `render_scene_with_camera()` is called on the new instance, or expose a test-only accessor). Without a clear testing strategy, this edge case may go untested.

- **W-04: Missing `#include <imgui.h>` in `viewport_panel.cpp` is not explicitly listed**  
  The contract specifies `viewport_panel.h` includes in full detail but does not list the includes needed for `viewport_panel.cpp` (notably `<imgui.h>` for `ImGui::GetContentRegionAvail`, `ImGui::Image`, `ImGui::Text`). While obvious to any implementer, listing them would make the contract fully self-contained and avoid compile errors on first write.

### Info (observations, not requiring action)

- **I-01: `render_scene(FrameBuffer&)` inconsistency**  
  The existing `render_scene(FrameBuffer& target)` binds the FBO and delegates to `render_scene()` (which does **not** clear). The new `render_scene_with_camera()` adds a `device_->clear()` call. This inconsistency means that callers using the old `render_scene(FrameBuffer&)` get stale FBO content on the first frame (if no active camera, the FBO is bound but never cleared or drawn to). This is a pre-existing issue, not introduced by this contract, but it may confuse developers. Consider filing a follow-up to add clearing to `render_scene(FrameBuffer&)` as well.

- **I-02: `Editor::~Editor()` log message is misleading**  
  The destructor body logs `"Editor: destroyed World"` before member destruction — at that point, the `World` has not yet been destroyed (it is destroyed during member destruction, after `settings_manager_` and before `panels_`). This is pre-existing and unrelated to the contract, but worth fixing separately.

- **I-03: ImGui include pattern in headers**  
  The existing `properties_panel.h` includes `<imgui.h>` directly in its header (which propagates it to all editors that include it). The contract's `viewport_panel.h` correctly omits `<imgui.h>` (including it only in `.cpp`), following a cleaner pattern. Good.

## Required changes

None. The contract is acceptable without mandatory changes.

## Suggested improvements

1. **Document the destructor ordering dependency** — Add a comment in `render_system.h` (or the contract) that the destructor of `RenderSystem` must not access `world_`, because `Editor` destroys `world_` before panels.
2. **Specify test #12 more concretely** — Add setup details: "Create ViewportPanel, capture initial RenderSystem identity, call `new_scene()` equivalent on the Editor, trigger `draw_ui()`, verify `RenderSystem` was replaced (e.g., check internal pointer differs or call count changed)."
3. **Add `<imgui.h>` to `.cpp` include list** — In the contract's file specification for `viewport_panel.cpp`, list `#include <imgui.h>`.
4. **Consider using `unique_ptr<RenderSystem>` instead of move-assign** — Avoids reliance on implicit move assignment and makes the World-replacement code cleaner (`render_system_.reset()`, `render_system_ = std::make_unique<RenderSystem>(...)`).
