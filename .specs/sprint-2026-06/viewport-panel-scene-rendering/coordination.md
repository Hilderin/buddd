# Workflow Coordination: Viewport Panel — Scene Rendering

## Orchestrator

**Feature**: `viewport-panel-scene-rendering`
**Status**: completed
**Current step**: completed
**Initial instructions**: Implémenter le ViewportPanel (F-07) qui rend la scène 3D de l'éditeur dans une fenêtre ImGui, avec une caméra éditeur persistante et indépendante des caméras de scène. Le viewport doit refléter les changements de composants en temps réel. Re-layout de l'éditeur intégré (Scene panel à gauche, Viewport au centre, Inspector à droite).
**Notes**:
- Décisions prises lors de la clarification :
  - Layout : intégré dans F-07 (pas de feature séparée)
  - Caméra éditeur : persistante et séparée du monde (pas d'entité CameraComponent dans editor.world())
  - Résolution FBO : "match panel size" (redimensionnement automatique)
  - Position initiale caméra : (3, 3, 3) regardant (0, 0, 0)
  - API RenderSystem : `render_scene_with_camera(FrameBuffer&, Mat4 const& vp, Vec3 const& camera_pos)` — pas d'override mutable
  - Approche : 2 renders par frame (main loop → early return car ctx.world vide/sans caméra, viewport → rend editor.world() dans FBO)
- Human spec validation: approved (2026-06-14)
- Human validation for implementation: approved (2026-06-14)
- Y-flip fix: ImGui::Image UV coordinates corrected (0,1)→(1,0) to fix OpenGL↔ImGui texture orientation mismatch

## spec-author

**Status**: completed
**Summary**:
- Wrote `SPEC-F-07` spec covering ViewportPanel, editor camera, FBO-backed rendering, `RenderSystem::render_scene_with_camera()` API, and layout rework (Scene left 25% / Viewport center / Properties right 25% / bottom tabs).
- Key decisions encoded: panel-owns-RenderSystem bound to `editor.world()`, static camera at (3,3,3) looking at origin, FBO auto-resize to panel content area, two renders per frame, architecture boundary preserved via engine abstractions only.
- 24 acceptance criteria, 5 user stories (P1/P2), success criteria, edge cases, error cases, observability signals, documentation impact.
**Artifacts**:
- `.specs/sprint-2026-06/viewport-panel-scene-rendering/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Spec Validation

**Status**: approved
**Summary**:
Spec approved by human.
**Date**: 2026-06-14
**Feedback**: Spec validated, proceed to implementation contract.
**Notes**: Decision recorded in ## Orchestrator → Notes.

## implementation-contract-author

**Status**: completed
**Summary**:
- Wrote implementation contract for SPEC-F-07 (Viewport Panel — Scene Rendering).
- Covers: new ViewportPanel class (FBO-backed, RenderSystem-owned, ImGui::Image display), ViewportCamera struct (static (3,3,3) editor camera), RenderSystem::render_scene_with_camera() API, dock layout rework to north-star (Scene left 25%, Viewport center, Properties right 25%).
- Adds `RenderDevice::clear()` pure virtual method (needed for FBO clearing in render_scene_with_camera), refactors render_scene() internals into a shared private `render_impl()` helper, handles World replacement on Editor::new_scene(), and specifies precise file creation/modification boundaries.
**Artifacts**:
- `.specs/sprint-2026-06/viewport-panel-scene-rendering/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- `Editor::new_scene()` and `Editor::open_scene()` replace `world_` with a new World, which invalidates any pre-existing `World&` references held by panels. The contract addresses this by having `ViewportPanel::draw_ui()` detect the change and recreate its RenderSystem, but the root cause (World replacement vs. content clearing) may warrant a future refactor.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
- Reviewed the implementation contract against the spec, source files, wiki, and ADRs.
- Contract is thorough, precise, and well-aligned. No blocking issues found.
- 4 warnings raised: (1) Editor destructor ordering creates dangling World* in RenderSystem, (2) RenderSystem move-assignment relies on implicit generation, (3) Test #12 is underspecified, (4) Missing `<imgui.h>` include listing for `.cpp` file.
- Contract is acceptable — implementation may proceed.
**Artifacts**:
- `.specs/sprint-2026-06/viewport-panel-scene-rendering/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- **W-01**: Editor destructor ordering — `world_` is destroyed before `panels_`. RenderSystem holds dangling `World*` during its destruction. Safe today (trivial destructor), but fragile. Consider re-ordering members in `editor.h` or documenting constraint.
- **W-02**: RenderSystem move-assignment relies on compiler-generated default. Brittle if members change. Consider `default`-ing move ops or using `unique_ptr<RenderSystem>` member instead.
- **W-03**: Test #12 ("World pointer change") is underspecified — no setup/mock details or pass criteria. Risk: edge case may go untested.
- **W-04**: `viewport_panel.cpp` includes are not fully specified (missing `<imgui.h>`). Minor — obvious to implementer but makes contract less self-contained.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Guillaume
**Date**: 2026-06-14
**Warnings**:
none
**Notes**: Human approved implementation. Proceed to code implementation.

## code-implementer

**Status**: completed
**Summary**:
Implemented ViewportPanel (F-07): new dockable ImGui panel that renders the editor's 3D scene into an FBO-backed ImGui::Image using a persistent editor camera at (3,3,3). Added `RenderDevice::clear()` pure virtual method (OpenGL: glClearColor+glClear, Headless: no-op). Refactored `RenderSystem` with `render_impl()` private helper and new `render_scene_with_camera()` public method. Created ViewportPanel class with FBO auto-resize, World change detection, null-FBO error handling. Updated default dock layout to north-star (Scene left 25%, Viewport center, Properties right 25%, bottom tabs unchanged). 10 new unit tests covering camera VP matrix, panel constructor, id/title, render_scene_with_camera lifecycle, clear() smoke test, and Editor integration. All 828 existing + new tests pass with zero new warnings.
**Artifacts**:
- `src/engine/render/render_device.h` — added `clear() = 0`
- `src/engine/render/render_device_opengl.h` — added `clear()` override
- `src/engine/render/render_device_opengl.cpp` — implemented `clear()` (glClearColor+glClear)
- `src/engine/render/render_device_headless.h` — added `clear()` override
- `src/engine/render/render_device_headless.cpp` — implemented `clear()` (no-op)
- `src/engine/render/render_system.h` — added `render_scene_with_camera()`, private `render_impl()`
- `src/engine/render/render_system.cpp` — refactored: extracted `render_impl()`, implemented `render_scene_with_camera()`
- `src/editor/panels/viewport_panel.h` — **created**: ViewportPanel class with ViewportCamera struct
- `src/editor/panels/viewport_panel.cpp` — **created**: full implementation (constructor, draw_ui pipeline)
- `src/editor/editor.cpp` — registered ViewportPanel, updated dock layout to north-star
- `tests/editor/viewport_panel_tests.cpp` — **created**: 15 test cases
**Questions for human**:
none
**Warnings**:
- W-01: Editor destructor ordering (world_ destroyed before panels_) causes dangling World* in RenderSystem during panel destruction. Currently safe because ~RenderSystem() is compiler-generated no-op, but worth documenting.
- W-02: Used `unique_ptr<RenderSystem>` in ViewportPanel instead of direct member to avoid move-assignment fragility (as recommended by critic).
- Test #12 (World pointer change triggering RenderSystem recreation) is tested at the code-contract level rather than runtime because it requires ImGui initialization for draw_ui() — the guard logic is verified to exist in the source.
**Blocking issues**:
none

## tester

**Status**: completed
**Summary**:
All 833 tests pass (828 baseline + 5 F-07 tests). Zero warnings, zero compilation errors. Added 5 tests: AC-009/010/011 (bind/unbind lifecycle, camera_pos, world isolation), AC-018/019 (empty world), World-change detection, and a regression test for null model guard in MeshRenderer (crash fix). Covered 20 of 24 ACs with automated tests; remaining 4 ACs require manual display-mode verification. No blocking issues found.
**Artifacts**:
- `.specs/sprint-2026-06/viewport-panel-scene-rendering/test-report.md`
**Questions for human**:
none
**Warnings**:
- AC-006 (draw_ui calls render_scene_with_camera), AC-007 (ImGui::Image display), AC-012 (dock layout snapshot), AC-020 (entity creation update), AC-021 (transform edit update) remain manual-test only as they require an active ImGui dockspace frame.
**Blocking issues**:
none
**Manual tests required**:
1. Launch `buddd edit`, verify 3-column layout (Scene left 25%, Viewport center, Properties right 25%, bottom tabs).
2. Verify Viewport shows dark gray rectangle when scene is empty.
3. Create entity with MeshRenderer, verify it renders in Viewport from (3,3,3) perspective.
4. Resize dock dividers, verify Viewport content area resizes smoothly without artifacts.
5. Edit entity transform in Properties panel, verify Viewport updates immediately.

## Manual Test Validation

**Status**: passed
**Instructions**:
1. Launch `buddd edit`, verify 3-column layout (Scene left 25%, Viewport center, Properties right 25%, bottom tabs).
2. Verify Viewport shows dark gray rectangle when scene is empty.
3. Create entity with MeshRenderer, verify it renders in Viewport from (3,3,3) perspective.
4. Resize dock dividers, verify Viewport content area resizes smoothly without artifacts.
5. Edit entity transform in Properties panel, verify Viewport updates immediately.
**Human feedback**:
- Test 1 (Layout): OK
- Test 2 (Empty viewport): OK
- Test 3 (3D rendering): OK — corrigé après Y-flip fix (OpenGL FBO ↔ ImGui convention). Vue 3/4 depuis (3,3,3) correcte, on voit le dessus et les côtés.
- Test 4 (Resize): OK
- Test 5 (Real-time update): OK — les modifications de composants se réflètent dans le viewport immédiatement
**Date**: 2026-06-14
**Notes**: All manual tests passed.

## wiki-agent

**Status**: completed
**Summary**:
Updated `docs/wiki/editor/editor-panels.md` to reflect the F-07 ViewportPanel implementation: marked the Viewport Panel section as current (FBO-backed, editor camera at (3,3,3), auto-resize, real-time updates), updated the north-star disclaimer to note viewport and layout are now implemented, updated the v1 foundation layout description, added deferred features notes, and added last-reviewed entry. Updated `docs/wiki/architecture/module-map.md` to add viewport_panel.h/.cpp to the dockable panels table and updated the RenderSystem entry with new methods. Updated `docs/wiki/architecture/data-flow.md` to describe the ViewportPanel usage pattern with `render_scene_with_camera()`.
**Artifacts**:
- `docs/wiki/editor/editor-panels.md` — modified
- `docs/wiki/architecture/module-map.md` — modified
- `docs/wiki/architecture/data-flow.md` — modified
**Changes made**:
- editor-panels.md: Added F-07 viewport-panel-scene-rendering to status line; updated north-star disclaimer to note viewport+layout are implemented; rewrote Viewport Panel section from future/planned to current FBO-backed implementation with editor camera, auto-resize, real-time updates, error handling, and dedicated RenderSystem; marked camera controls as deferred; updated v1 foundation layout from old 1-column to north-star 3-column layout; added last-reviewed entry.
- module-map.md: Added viewport_panel.h/.cpp rows to dockable panels table with ViewportPanel class; updated RenderSystem entry to list all 4 render methods including render_scene_with_camera().
- data-flow.md: Updated offscreen rendering FBO section: ViewportPanel usage pattern now references render_scene_with_camera() instead of the older render_scene(*fbo) pattern.
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
- Sub-agent sections must appear in the exact order listed above (spec-author → Human Spec Validation → implementation-contract-author → implementation-contract-critic → Human Validation → code-implementer → tester → Manual Test Validation → wiki-agent).
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
