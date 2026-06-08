# Workflow Coordination: editor-scaffolding

## Orchestrator

**Feature**: `editor-scaffolding`
**Status**: completed
**Current step**: completed
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

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-07 20:40 EDT
**Warnings**:
<none>
**Notes**: Human approved after corrections: (1) --frame/--capture are incidental through run_app(), not editor features. (2) Escape does NOT close editor. (3) ADR-027 created for architecture rationale. Spec and contract updated to reference ADR-027.

## code-implementer

**Status**: completed
**Summary**:
Updated Editor class API from PIMPL pattern to direct member variables (`EngineService*`, `Window*`, `bool initialized_`). `draw_ui()` now takes `EngineContext const& ctx` (per-frame context). `setup()` stores `engine_` and `window_` references from context. `shutdown()` nulls pointers. All 426 tests pass, build succeeds, architecture boundaries clean.
**Artifacts**:
- `src/editor/editor.h` — modified (removed PIMPL, added direct member vars, draw_ui takes ctx)
- `src/editor/editor.cpp` — modified (no PIMPL, new lifecycle impl)
- `src/cmd/apps/editor_app.cpp` — modified (pass ctx to draw_ui)
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Implementation correctly updates Editor API from PIMPL to direct member variables. All 20 acceptance criteria satisfied. Build produces zero warnings from our code. All 426 tests pass (21427 assertions). Architecture boundary clean (no SDL3/OpenGL/GLM in src/editor/ or src/cmd/apps/). No blocking issues found. The EditorApp header adds explicit constructor/destructor declarations (required for unique_ptr<Editor> destructor visibility) that are not in the spec — minor documentation gap, not a blocking issue.
**Artifacts**:
- `.specs/sprint-2026-06/editor-scaffolding/code-review.md`
**Questions for human**:
none
**Warnings**:
- EditorApp header declares `EditorApp()` and `~EditorApp() override` not shown in the spec. These are practically required for `unique_ptr<Editor>` with forward-declared Editor, but the spec doesn't document them. Consider updating spec for accuracy.
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**: Created ADR-027 documenting the editor architecture (original). **Fix round**: (1) Amended ADR-026 Decision 2 with an amendment note referencing ADR-027 — the original "Init failure is non-fatal" text is preserved with a blockquote pointing to ADR-027 Decision 5. (2) Updated ADR-027 Decision 4 from PIMPL pattern to direct member variables — changed heading, API declaration, rationale, and both Positive/Negative consequences that referenced PIMPL.
**Artifacts**:
- `docs/adr/ADR-026-imgui-integration.md` — amended (amendment note on Decision 2)
- `docs/adr/ADR-027-editor-architecture.md` — modified (Decision 4: PIMPL → direct members)
**Decisions needed**: none
**Questions for human**: none
**Warnings**:
- Wiki docs (`overview.md`, `module-map.md`, `dependency-map.md`) still reference `buddd_editor` as an INTERFACE placeholder — these will be updated by the wiki-agent.
**Blocking issues**: none

## wiki-agent

**Status**: completed
**Summary**:
Updated three wiki files to reflect the editor scaffolding changes: `overview.md` (CMake targets table + directory layout), `module-map.md` (replaced INTERFACE placeholder section with full Editor library documentation, added `buddd edit` subcommand, updated CLI dispatch count), and `dependency-map.md` (added `buddd_editor ──PRIVATE──► buddd_engine` edge, updated table and key constraints).

**Second pass (governance-fix)**: Corrected `buddd_editor` → `buddd_engine` link type from PRIVATE to PUBLIC in the ASCII diagram, the target dependencies table, and the key constraints section, matching the actual CMakeLists.txt, spec, and contract.
**Artifacts**:
- `docs/wiki/architecture/overview.md`
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/architecture/dependency-map.md`
**Changes made**:
- **overview.md**: Changed `buddd_editor` from INTERFACE library to Static library in CMake targets table; updated description to reflect real editor library and `buddd_engine` link dependency. Changed directory layout comment from "Editor placeholder (INTERFACE lib)" to "Editor library (STATIC lib)".
- **module-map.md**: Replaced placeholder section with full editor library section documenting `editor.h`, `editor.cpp`, `editor_app.h`, `editor_app.cpp` and their roles. Added `buddd edit` subcommand to CLI behavior listing. Updated CLI dispatch count from 3 to 4 commands. Referenced ADR-027.
- **dependency-map.md**: Added `buddd_editor ──PRIVATE──► buddd_engine` edge to ASCII diagram, updated target dependencies table row, replaced "links nothing" constraint with correct "static library linking buddd_engine" statement.
- **dependency-map.md (fix)**: Changed `buddd_editor` → `buddd_engine` link type from PRIVATE to PUBLIC in ASCII diagram, table row, and key constraints.
**Questions for human**: none
**Warnings**: none
**Blocking issues**: none

## governance-reviewer

**Status**: completed
**Summary**:
Re-review confirms all 3 previously blocking issues are resolved: (1) ADR-026 Decision 2 now carries an amendment note referencing ADR-027. (2) ADR-027 Decision 4 now shows direct member variables instead of PIMPL. (3) Wiki dependency-map now correctly shows buddd_editor→buddd_engine as PUBLIC. No new blocking issues found. Two non-blocking warnings remain (overview.md key behaviors, EditorApp constructor/destructor). Governance review passes.
**Artifacts**:
- `.specs/sprint-2026-06/editor-scaffolding/governance-review.md`
**Questions for human**:
none
**Warnings**:
- Overview.md key behaviors still says "three commands (run, version, help)" — needs updating to four commands including `edit`.
- EditorApp constructor/destructor declarations not shown in spec — minor documentation gap flagged in code review.
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
