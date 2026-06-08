# Workflow Coordination: editor-scaffolding

## Orchestrator

**Feature**: `editor-scaffolding`
**Status**: in-progress
**Current step**: human-validation
**Initial instructions**: Create the editor scaffolding: a reusable `buddd_editor` library with an `Editor` class skeleton, an `EditorApp` that extends the `App` lifecycle, and a `buddd edit` CLI command. The editor should open an empty ImGui-docked window. The editor library lives in `src/editor/`, uses namespace `buddd::editor`, and links `buddd_engine`. This is scaffolding only — no editor panels or features beyond the ImGui dockspace.
**Notes**: Grill-me (Definition of Ready walkthrough) complete. All criteria satisfied. Decisions recorded in ## Decision Log below. **Update 2026-06-07**: Human clarified that ImGui init failure in the SDL3/display path should be a fatal error in the engine (`RenderDevice::create()` returns error instead of non-fatal warning). This changes ADR-026 behavior. Will require ADR amendment or new ADR. **Correction 2026-06-07**: Human corrected two points during validation: (1) `--frame`/`--capture` work incidentally through `run_app()` — NG-05 updated from "no flags" to "not editor features". (2) Escape should NOT close the editor — spec updated to remove Escape behavior. This is an editor, not a demo scene.

## Decision Log

| ID | Question | Decision |
|---|---|---|
| D-01 | Editor architecture | Editor is a library (`buddd_editor` static lib) in `src/editor/`, used by CLI command `buddd edit`. |
| D-02 | Editor lifecycle | Reuse existing App lifecycle: `EditorApp` extends `App`, uses `run_app()`. |
| D-03 | Namespace | `buddd::editor` |
| D-04 | Editor class API | `Editor()` constructor → `setup(EngineContext&)` → `draw_ui()` → `shutdown()`. Called from `EditorApp::setup()` / `on_render()` / `shutdown()`. |
| D-05 | ImGui hook | `draw_ui()` called from `EditorApp::on_render()` (before `render_ui()` flushes). |
| D-06 | Scope boundary | **IN**: library scaffolding, Editor class skeleton, EditorApp, CLI `edit` command, ImGui dockspace window (1280x800, title "Buddd Editor"). `--frame`/`--capture` work incidentally via `run_app()` but are not editor features. **OUT**: any editor panels, scene viewport, project management, gizmos/selection/undo. |
| D-07 | Edge: headless | `buddd edit` requires display. If `BUDDD_HAS_DISPLAY=OFF`, error + exit. |
| D-08 | Edge: ImGui fail (SUPERSEDED by D-12) | ~~Error + exit for `buddd edit` (engine treats as non-fatal, but EditorApp checks and exits).~~ Replaced by D-12: engine-level fatal in display mode. |
| D-09 | Escape behavior | Editor does NOT exit on Escape. Only window close exits. No Escape handler — this is an editor, not a demo scene. |
| D-10 | Verification | Manual (visual) + headless unit test (construct Editor, setup/shutdown, verify no crash). Test links `buddd_editor`. |
| D-11 | Docs to update | Wiki: overview.md (CMake targets table + directory), module-map.md (new section), dependency-map.md (new edge). Defer README and data-flow. |
| D-12 | ImGui init failure (SDL3 path) | Make fatal in engine. `RenderDevice::create()` returns error if `engine_imgui::init()` fails in the display path. Non-fatal warning is replaced with error propagation. Headless path unaffected. Requires amending ADR-026. |

## spec-author

**Status**: completed
**Summary**: Fixed const mismatch (EngineContext& → const&), added tests/CMakeLists.txt to modified files table
**Artifacts**:
- `.specs/sprint-2026-06/editor-scaffolding/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review confirms all three blocking issues are resolved: (1) `Editor::setup()` now uses `EngineContext const&`, (2) D-08 is marked SUPERSEDED by D-12, (3) `tests/CMakeLists.txt` added to modified files table. No new issues found. All Definition of Ready criteria satisfied. Spec is accepted and ready for implementation.
**Artifacts**:
- `.specs/sprint-2026-06/editor-scaffolding/spec-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Created `implementation-contract.md` covering all 10 implementation steps: engine change (ImGui init fatal), editor library (STATIC CMake, Editor class with PIMPL), EditorApp subclass, CLI edit dispatch, CMake link updates, headless unit test, and ADR amendment notice. Defined precise API contracts, file-by-file changes, build system edits, test plan, edge cases, and 24+ verifiable Done criteria. Enforces ADR-019 architecture boundary (no SDL3/OpenGL/GLM outside src/engine/).
**Artifacts**:
- `.specs/sprint-2026-06/editor-scaffolding/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Review completed. No blocking issues found. Contract is thorough, complete, and correctly implements the spec. Three non-blocking warnings noted: (1) test code calls draw_ui() after failed setup contradicting spec's explicit instruction — could cause UB in display mode, (2) subdirectory order misstated in contract (actual order differs from claimed), (3) [NEEDS CLARIFICATION] marker in test template. Contract is accepted and ready for implementation.
**Artifacts**:
- `.specs/sprint-2026-06/editor-scaffolding/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- Test code template calls draw_ui() after potentially failed setup(), contradicting spec's explicit "draw_ui() is not called in this test" — could cause UB with uninitialized ImGui.
- Contract states root CMakeLists.txt subdirectory order is engine→editor→cmd, but actual order is engine→cmd→editor (non-impactful but factually inaccurate).
- Test code template contains a [NEEDS CLARIFICATION] marker in a comment — should be removed for a finalized contract.
**Blocking issues**:
none

## Human Validation

**Status**: pending
**Approver**: <git user name>
**Date**: <date and time>
**Warnings**:
<none>
**Notes**: <any human feedback or conditions>

## code-implementer

**Status**: pending
**Summary**:
<none>
**Artifacts**:
<none>
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## code-reviewer

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- `.specs/sprint-2026-06/editor-scaffolding/code-review.md`
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**: Created ADR-027 documenting the editor architecture: editor as a separate static library (`buddd_editor`) in `src/editor/` with PIMPL pattern, reuse of existing App lifecycle via `EditorApp`, namespace `buddd::editor`, `buddd edit` CLI command, ImGui init failure changed to fatal in display mode (amends ADR-026), architecture boundary enforcement (no SDL3/OpenGL/GLM in src/editor/), and `--frame`/`--capture` as incidental features.
**Artifacts**:
- `docs/adr/ADR-027-editor-architecture.md`
**Decisions needed**: None — all decisions are documented in ADR-027.
**Questions for human**: none
**Warnings**:
- ADR-027 amends ADR-026 Decision 2 (init failure non-fatal → fatal in display mode). ADR-026 should be updated to reflect this amendment.
- Wiki docs (`overview.md`, `module-map.md`, `dependency-map.md`) still reference `buddd_editor` as an INTERFACE placeholder — these will be updated by the wiki-agent.
**Blocking issues**: none

## wiki-agent

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- <list of wiki files created or modified, or "none">
**Changes made**:
<none>
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## governance-reviewer

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- `.specs/sprint-2026-06/editor-scaffolding/governance-review.md`
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
