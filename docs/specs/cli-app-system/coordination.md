# Workflow Coordination: cli-app-system

## Orchestrator

**Feature**: `cli-app-system`
**Status**: completed
**Current step**: completed
**Initial instructions**: Centraliser la boucle de rendu CLI avec un pattern `App` (setup/render/shutdown), ajouter `--capture [frame:]path` multi-frame, supprimer les commandes `demo`/`capture` au profit de `run <scene>`, supprimer le dossier `capture/`, refactorer `RenderSystem` avec `render_scene()` sans begin/end.
**Notes**: Design validé avec l'humain :
- `run` = commande unique de rendu (`buddd run [<scene>] [--frame N] [--capture [N:]path]...`)
- `demo` et `capture` supprimées
- Pattern `App` avec `setup() -> Result<void>`, `render(Device&, int) -> void`, `shutdown() -> void`
- `RenderSystem::render_scene()` ajoutée (sans begin/end)
- `run_app()` appelée à 1 seul endroit dans main
- Pas de `sleep_for` dans la boucle (compte sur VSync)
- `--capture` peut être répété : `--capture 50:/tmp/a.png --capture 120:/tmp/b.png`

## spec-author

**Status**: completed
**Summary**:
Wrote `docs/specs/cli-app-system/spec.md` (SPEC-008). The spec defines: (1) a single `App` base class with setup/render/shutdown lifecycle and `AppConfig`/`CaptureSpec` structs, (2) a `run_app()` function owning the central render loop with `--frame` and `--capture [N:]path` global flag parsing, (3) seven App subclasses absorbing all demo and capture scene logic, (4) `RenderSystem::render_scene()` extraction without begin/end framing, (5) unified CLI: `buddd run [<scene>] [--frame N] [--capture [N:]path]...`, (6) removal of demo/capture commands and duplicate render loops, (7) full file change list (create/modify/delete), (8) 34 acceptance criteria, 5 success criteria, comprehensive edge/error cases.
**Artifacts**:
- `docs/specs/cli-app-system/spec.md`
**Questions for human**:
- [RESOLVED] Q-01: Syntaxe `--capture N:path`, toujours avec N explicite. Pas de capture par défaut. Résolution : `--capture` sans `N:` est une erreur.
- [RESOLVED] Q-02: Liste des scènes codée en dur (hardcoded string). Résolution : inchangé vs k_usage_text.
- [RESOLVED] Q-03: `run_app()` est une fonction libre. Résolution : déclarée dans app.h, définie dans app.cpp.
**Warnings**:
- The `phong_capture.cpp` file contains ~200 lines of manual RenderSystem reimplementation (light collection, MeshRenderer iteration). This spec removes it entirely — PhongApp uses `RenderSystem::render_scene()` and `run_app()` handles `read_pixels()`. This is a significant reduction in code but must be verified visually to ensure identical rendering.
- The driver quirk workaround (skip frame 1, capture frame 2) from the existing capture code is preserved in `run_app()`. This means `--capture 1:path` silently captures frame 2 instead. This matches existing behavior but is potentially surprising.
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
The spec is well-structured, thorough, and demonstrates deep understanding of the existing codebase. 5 blocking issues were identified and all have been resolved: (1) SC-004 removed (window dimension change makes pixel comparison infeasible), (2) consistent error message throughout, (3) render_scene() clarified as member function, (4) driver quirk remains silent per human decision, (5) extra-argument warning format added. Spec accepted for implementation contract.
**Artifacts**:
- `docs/specs/cli-app-system/spec-critic.md`
**Questions for human**:
<none>
**Warnings**:
- 5 ACs rely on manual visual inspection (AC-013, AC-021, AC-022, AC-023) — not automatable.
- Frame numbering dualism (0-based in render(), 1-based in --capture) is a persistent off-by-one risk.
- CONST-002 testing coverage gap: 7 new App subclasses have no isolated unit tests.
- No headless-only AC for `--frame` limited exit.
**Blocking issues**:
- [x] SC-004 removed (window 800×600→1024×768 rend la comparaison pixel impossible). Résolution humaine : supprimer.
- [x] Inconsistent `--capture` error message — corrigé.
- [x] `RenderSystem::render_scene()` signature clarifiée.
- [x] Driver quirk frame 1→2 : silencieux (résolution humaine).
- [x] Warning format pour extra arguments ajouté.

## implementation-contract-author

**Status**: completed
**Summary**:
Wrote `docs/specs/cli-app-system/implementation-contract.md` (IMPL-008). The contract defines: (1) the App interface (app.h/app.cpp) with CaptureSpec, AppConfig, App base class (setup/render/shutdown), and run_app() free function, (2) 7 App subclasses in src/cmd/apps/ absorbing all demo and capture scene logic, (3) the central render loop in run_app() with --frame N and --capture N:path parsing, driver quirk workaround (frame 1→2), and capture save logic, (4) RenderSystem::render_scene() extraction, (5) new main.cpp dispatch (no demo/capture commands, run <scene> dispatch with hardcoded scene list), (6) updated CMakeLists.txt globs, (7) updated help text, (8) deletion of 22 old files, (9) all edge cases, security/data/API/compatibility impact, and Done criteria with file-level checkboxes.
**Artifacts**:
- `docs/specs/cli-app-system/implementation-contract.md`
**Questions for human**:
- [RESOLVED] AppConfig enrichi avec window_title/window_width/window_height. Chaque App définit son titre via config_ en constructeur.
- [RESOLVED] Warning extra args : imprimé dans main.cpp avant run_app().
- [RESOLVED] parse_global_flags() extrait dans src/cmd/app_config.h/.cpp (testable unitairement).
**Warnings**:
- The phong_capture.cpp manual reimplementation of RenderSystem::render() (~200 lines of light collection, MeshRenderer iteration, uniform setting) is completely removed. PhongApp uses RenderSystem::render_scene() instead. This must be visually verified to ensure identical rendering.
- The aspect ratio for all scenes changes from 800×600 (old demo/capture) to 1024×768 (new unified window size). CubeApp and CubeSceneApp camera aspects must use 1024/768.
- The driver quirk (frame 1 → 2) is silent with no warning, matching the existing capture behavior and the human design decision.
- 5 ACs rely on manual visual inspection (AC-013, AC-021, AC-022, AC-023) — not automatable.
- 22 files are deleted — ensure no external references exist before deletion.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
All 6 blocking issues have been resolved. The driver quirk logic bug was fixed by computing `effective_frame` before the match condition (`int effective_frame = (spec.frame < 2) ? 2 : spec.frame; if (effective_frame == frame + 1)`). Verified correct for `--capture 1:path` maps to frame 2, `--capture 2:path` maps to 2nd rendered frame. No new issues introduced. Contract accepted for implementation.
**Artifacts**:
- `docs/specs/cli-app-system/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- `render_scene()` line-number dependency is fragile (references exact line numbers which may shift).
- No test for `parse_global_flags()` `start` parameter.
- `App::config_` member architecture differs from spec (protected member vs private pointer) — no explicit deviation note.
- `demo_helpers` `std::exit()` bypasses `App::shutdown()` — known limitation.
- `fps_limit` parsed but unused (reserved for future use).
- 5 ACs rely on manual visual inspection.
- Frame numbering dualism (0-based render vs 1-based capture).
- No-scene extra-args handling: `buddd run --frame 60` treated as unknown scene. Pre-existing design choice.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-04 19:48 EDT
**Warnings**:
<none>
**Notes**: Approuvé. Lancement de l'implémentation.

## code-implementer

**Status**: completed
**Summary**:
Implemented the CLI App System refactoring per IMPL-008. Created the `App` base class with setup/render/shutdown lifecycle, `run_app()` central render loop, and 7 `App` subclasses (RunApp, TriangleApp, CubeApp, CubeSceneApp, TexturedCubeApp, FreeCameraApp, PhongApp) absorbing all old demo/capture logic. Added `RenderSystem::render_scene()`. Rewrote `main.cpp` dispatch — removed `demo`/`capture` commands, added `run <scene>` with `--frame N` and `--capture N:path` flags. Updated help text, CMakeLists globs. Deleted 22 old files. All 279 tests pass (100%).
**Artifacts**:
- Created: `src/cmd/app_config.h`, `src/cmd/app_config.cpp`, `src/cmd/app.h`, `src/cmd/app.cpp`, `src/cmd/apps/run_app.h/.cpp`, `src/cmd/apps/triangle_app.h/.cpp`, `src/cmd/apps/cube_app.h/.cpp`, `src/cmd/apps/cube_scene_app.h/.cpp`, `src/cmd/apps/textured_cube_app.h/.cpp`, `src/cmd/apps/free_camera_app.h/.cpp`, `src/cmd/apps/phong_app.h/.cpp`, `tests/cli_app_tests.cpp`
- Modified: `src/cmd/main.cpp`, `src/cmd/commands/help_command.h`, `src/cmd/CMakeLists.txt`, `src/engine/render/render_system.h`, `src/engine/render/render_system.cpp`, `tests/cmd_tests.cpp`
- Deleted: `src/cmd/commands/run_command.*`, `src/cmd/commands/demo_command.*`, `src/cmd/commands/capture_command.*`, `src/cmd/capture/` (entire dir), `src/cmd/demo/cube_demo.*`, `src/cmd/demo/cube_scene_demo.*`, `src/cmd/demo/free_camera_demo.*`, `src/cmd/demo/phong_demo.*`, `src/cmd/demo/textured_cube_demo.*`, `src/cmd/demo/triangle_demo.*`
**Questions for human**:
- none
**Warnings**:
- Old test assertions in `tests/cmd_tests.cpp` were updated to match new CLI (demo/capture removed). New tests in `tests/cli_app_tests.cpp` cover CLI app system scenarios.
- CONST-001 compliance verified: zero matches for SDL3/OpenGL/GLM headers in `src/cmd/`.
- PhongApp uses `RenderSystem::render_scene()` instead of the old manual ~200-line reimplementation from phong_capture.cpp — visual verification needed to ensure identical rendering.
- Camera aspect ratio for all scenes changed from 800×600 to 1024×768 (unified window size).
- Unit tests for `parse_running_args()` are tested indirectly via CLI integration tests (subprocess calls).
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
1 blocking issue found (0-based frame number in abort message) — FIXED (frame → frame+1 in app.cpp line 100). All 279 tests pass. All files confirmed. CONST-001 clean. Review complete.
**Artifacts**:
- `docs/specs/cli-app-system/code-review.md`
**Questions for human**:
<none>
**Warnings**:
- `--frame 0` handled as error (N<1) rather than interactive mode — spec has internal inconsistency on this edge case; implementation chose the stricter error-case interpretation.
- `is_running()` public accessor added to `App` class (not in spec) — necessary for free-function `run_app()` encapsulation.
- Extra-argument warning format differs slightly from spec language (accepted during spec-critic review).
- No direct unit tests for `parse_running_args()` — tested indirectly via subprocess CLI tests.
- Aspect ratio change: all scenes use 1024×768 (was 800×600 for demos) — intentional.

**Blocking issues**:
- [ ] **Scene abort message uses 0-based frame number (violates implementation contract)**: In `src/cmd/app.cpp` line 100, `frame` is 0-based but should be `frame + 1` per the implementation contract step 10 which specifies 1-based frame numbering for the abort message.

## adr-agent

**Status**: completed
**Summary**:
Assessed the CLI App System refactoring and determined an ADR is needed. The refactoring reverses core architectural decisions from ADR-004 (per-demo free functions with individual render loops → centralised `App` lifecycle + `run_app()`), changes the module structure (`src/cmd/demo/` → `src/cmd/apps/`), introduces a new `App` base class pattern with setup/render/shutdown lifecycle, and unifies the CLI under a single `run` command. Wrote ADR-014 documenting the decision, alternatives considered, consequences, and which parts of ADR-004 are superseded.
**Artifacts**:
- `docs/adr/014-cli-app-system.md`
**Decisions needed**:
none (ADR records the decision already made by the spec and implemented)
**Questions for human**:
none
**Warnings**:
- ADR-014 partially supersedes ADR-004 (per-demo render loops, `DemoCommand` dispatch, `src/cmd/demo/` directory for scene code, per-demo free functions). ADR-004 remains accepted but its affected sections are now historical.
- Frame numbering dualism (0-based in render(), 1-based in --capture) persists as noted in ADR-014 consequences.
- `demo_helpers` `std::exit()` bypasses `App::shutdown()` — pre-existing limitation documented in ADR-014.
**Blocking issues**:
none

## constitution-agent

**Status**: completed
**Summary**:
Constitution review complete. Verified all new and modified files in `src/cmd/` (app.h, app.cpp, app_config.h, app_config.cpp, apps/*.h/.cpp, main.cpp) for CONST-001 compliance — zero SDL3, OpenGL, or GLM includes found. All access to platform/graphics APIs correctly goes through the engine abstraction layer (`RenderDevice`, `Result<void>` forward declarations). No constitution changes are needed: the refactoring introduces a new `App` abstraction pattern but does not create new architecture rules, new dependency risks, or new testing/documentation/security gaps beyond those already captured.
**Artifacts**:
none
**Changes needed**:
none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated the operational wiki to reflect the CLI App System refactoring (ADR-014). The major changes: (1) rewrote the CLI section in module-map.md to describe the App lifecycle pattern instead of the old Command pattern, (2) removed references to `demo`/`capture` commands and old command classes throughout, (3) updated data-flow.md CLI dispatch diagram and output table for the unified `run` command, (4) updated overview.md key behaviors with new `buddd run <scene> [--frame N] [--capture N:path]` syntax, (5) updated testing.md CLI integration tests table to match new CLI, (6) added ADR-014 to the ADR index, (7) fixed stale `demo_command.cpp` references in glossary and dependency-map.
**Artifacts**:
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/architecture/data-flow.md`
- `docs/wiki/architecture/overview.md`
- `docs/wiki/engineering/testing.md`
- `docs/wiki/decisions/adr-index.md`
- `docs/wiki/domain/glossary.md`
- `docs/wiki/architecture/dependency-map.md`
**Changes made**:
- module-map.md: Replaced old Command pattern + per-demo file description with App lifecycle, App subclasses table, updated build system (apps/*.cpp), removed Capture files section, removed old command files, updated subcommand behavior for `run`-only dispatch.
- data-flow.md: Rewrote CLI data flow diagram (no more DemoCommand/CaptureCommand), updated output table for new CLI, added scene dispatch sub-diagram, updated legacy lifecycle note, updated Reference section.
- overview.md: Rewrote Key behaviors — all 8 examples now use `buddd run <scene>` syntax, removed old `demo`/`capture` examples, added `--frame` and `--capture` examples.
- testing.md: Updated CLI integration tests table — replaced old `demo`/`capture` tests with new `run <scene>` tests, added `--frame N` and `--capture` test cases.
- adr-index.md: Added ADR-014 entry.
- glossary.md: Changed "Used by tests and `demo_command.cpp`" → "Used by tests and `run_app()`".
- dependency-map.md: Same fix for `demo_command.cpp` → `run_app()`.
**Questions for human**:
none
**Warnings**:
- The aspect ratio change (800×600 → 1024×768 for all scenes) is documented implicitly in the updated key behaviors but may affect visual tests.
- ADR-014 partially supersedes ADR-004 (per-demo render loops, DemoCommand dispatch, scene code in src/cmd/demo/). The ADR index still lists ADR-004 as Accepted — this is correct per project convention (accepted ADRs remain accepted even when partially superseded).
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Governance review complete. All validation criteria met: (1) No constitution violations — CONST-001 clean (zero SDL3/OpenGL/GLM headers in src/cmd/), CONST-002 satisfied (tests exist and pass). (2) ADR-014 exists, is Accepted, and is fully consistent with spec/contract. (3) Cross-document coherence verified across all 9 artifacts — no contradictions. (4) All workflow artifacts exist and are complete. (5) Code-review gate passed — the sole blocking issue (0-based frame number) is FIXED (frame+1 confirmed in app.cpp). (6) Human validation recorded (approved by Hilderin 2026-06-04). No blocking issues. Governance review accepted.
**Artifacts**:
- `docs/specs/cli-app-system/governance-review.md`
**Questions for human**:
none
**Warnings**:
- `is_running()` public accessor added in implementation but not documented in spec — necessary design adaptation for free-function `run_app()`.
- 5 ACs rely on manual visual inspection (AC-013, AC-021–AC-024) — not automatable.
- Frame numbering dualism (0-based render vs 1-based capture/observability) persists as documented off-by-one risk.
- `demo_helpers` `std::exit()` bypasses `App::shutdown()` — pre-existing limitation, not introduced by this workflow.
- SPEC-008 numbering collision (shared with scene-graph spec) — pre-existing project issue.
- App subclass rendering verified only via integration tests, not isolated unit tests.
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
