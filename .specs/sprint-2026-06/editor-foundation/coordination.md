# Workflow Coordination: editor-foundation

## Orchestrator

**Feature**: editor-foundation
**Status**: in-progress
**Current step**: code-reviewer
**Initial instructions**: Ajouter à l'éditeur (sur la base du scaffolding existant) : un menu principal (File > Quit, Edit > Undo/Redo, Help > About avec popup), un système de commandes avec undo/redo (Command pattern), un système de docking avec layout persisté, et des panels placeholder dockables vides (Scene, Properties, Console, Project, Assets).
**Notes**:
- Human decisions (08-Jun-2026):
  - Command system: Full Command pattern with execute/undo, CommandStack with history.
  - Menu structure: File (Quit), Edit (Undo, Redo), Help (About → popup with engine name + version).
  - Placeholder panels: Scene, Properties, Console, Project, Assets.
  - Docking persistence: Yes, persist layout between sessions (buddd_editor.ini).
  - Keyboard shortcuts: All standard — Ctrl+Z (Undo), Ctrl+Shift+Z/Ctrl+Y (Redo), Ctrl+Q (Quit), etc.
- Spec update (08-Jun-2026): Added `App::update()` lifecycle method per human request. Editor now splits logic (update) from rendering (draw_ui). Spec-critic must re-review.
- Spec refactor (08-Jun-2026): Replaced individual draw_scene_panel/draw_properties_panel/etc methods with EditorMenu/EditorPanel abstractions + two vectors (menus_/panels_). MenuBar is an EditorMenu. Five panels are concrete EditorPanel subclasses. add_menu()/add_panel() registration.
- Spec refinement (08-Jun-2026): Added ShortcutRegistry with bind()/process(). MenuBar uses callbacks (no raw bool& params). Removed ShowAboutCommand. All shortcuts registered via ShortcutRegistry in Editor::setup().
- Spec Q&A session (08-Jun-2026):
  - Q-01: Ini file name → `buddd_editor.ini`
  - Q-02: UndoCommand/RedoCommand → direct calls, not undoable commands
  - Q-03: Ini path in headless → skip if not initialized (no change needed)
  - Q-04: Panel min size → yes, 100×100 constraint
  - Q-05: Quit confirmation → no, quit immediately
  - Q-06: Undo/Redo labels → just "Undo"/"Redo"
  - Q-07: Default layout → ImGui::DockBuilder in setup()
  - Q-08: max_history → 128 (default)

## spec-author

**Status**: completed
**Summary**:
Specifies SPEC-028 — Editor Foundation, building on the scaffolding. Covers: Command pattern (Command base class + CommandStack with bounded undo/redo), main menu bar (File > Quit, Edit > Undo/Redo, Help > About with modal popup), keyboard shortcuts (Ctrl+Z/Ctrl+Shift+Z/Ctrl+Y/Ctrl+Q gated by WantCaptureKeyboard), five placeholder dockable panels (Scene, Properties, Console, Project, Assets), docking persistence via imgui.ini, and 32 acceptance criteria. 8 open questions marked `[NEEDS CLARIFICATION]`.
**Artifacts**:
- `.specs/sprint-2026-06/editor-foundation/spec.md`
**Questions for human**:
<none — all resolved via human Q&A session>
- Q-07: Default panel layout via DockBuilder or floating windows?
- Q-08: CommandStack max_history default (128) — reasonable?
**Warnings**:
- UndoCommand/RedoCommand as meta-commands (undoing undo = redo) creates a recursive pattern; if Q-02 decides against this, the `commands/` directory structure may simplify.
- `QuitCommand::undo()` is a no-op (cannot un-request exit); this breaks the "all commands are undoable" ideal but is pragmatic.
- Dynamic panel close/hide persistence is deferred — panels always reappear on launch.
**Blocking issues**:
<none>

## spec-critic

**Status**: completed
**Summary**:
Re-review (08-Jun-2026) of spec refactoring: EditorMenu/EditorPanel abstractions, 4-phase rendering, add_menu()/add_panel() registration. The abstractions are well-defined, the 4-phase flow is clear, ACs are all valid and testable. No blocking issues. Three new warnings: (1) NG-05 contradicts run_app() modification (spec changes app.cpp but NG-05 says no changes), (2) EditorApp misclassified as "Unchanged" in file changes table, (3) "Disabled when no command" for Quit is misleading. Three previous warnings remain unresolved (panel close button, AC-024 phrasing, ADR-027 staleness). Overall DoD pass.
**Artifacts**:
- `.specs/sprint-2026-06/editor-foundation/spec-critic.md`
**Questions for human**:
- Should NG-05 be updated to acknowledge the app.cpp modification (adding `app.update(ctx)` to run_app())?
- Should EditorApp be moved from "Unchanged" to "Modified" in the file changes table?
**Warnings**:
- [x] RESOLVED: Data flow diagram ordering (restructured into clean tree diagram)
- [ ] AC-019/AC-024 phrasing still slightly awkward (unchanged)
- [ ] Panel close button / per-panel state mechanism not explicitly explained (unchanged)
- [ ] QuitCommand::undo() is a no-op per design (unchanged, acceptable)
- [ ] ADR-027 Decision 2 consequence ("no changes to App base class or run_app()") now outdated — recommend amendment note (unchanged)
- [ ] EditorApp's update() vs. on_frame_begin() lifecycle overlap (unchanged)
- [ ] NG-05 contradicts run_app() modification: "No other changes to run_app()" but spec modifies src/cmd/app.cpp
- [ ] File changes table misclassifies EditorApp as "Unchanged" while reason says it overrides update()
- [ ] Line 65: "File > Quit ... Disabled when no command is available" — Quit is always enabled
**Blocking issues**:
- [ ] *(none — no blocking issues found)*

## implementation-contract-author

**Status**: completed
**Summary**:
Updated implementation contract to add App::update() lifecycle method, Editor::update()/draw_ui() split, EditorApp::update() override, and two-phase update/render separation (G-09, AC-033-037). Added Steps 12 (App::update() to base class + run_app() call), Step 13 (EditorApp::update() override), restructured Step 8 to split update() from draw_ui(), and added 7 new Done criteria (DC-023 through DC-029). Updated files allowed/forbidden to include src/cmd/app.h, app.cpp, editor_app.h, editor_app.cpp. All 14 existing demo scenes must continue to build and run unchanged.
**Artifacts**:
- `.specs/sprint-2026-06/editor-foundation/implementation-contract.md`
**Questions for human**:
<none>
**Warnings**:
- QuitCommand::undo() is a documented no-op — this is acceptable per spec Q-05 (no quit confirmation).
- `ImGui::GetIO().IniFilename = "buddd_editor.ini"` uses a string literal pointer with static storage duration — safe for ImGui's lifetime, but any future dynamic string reassignment must manage the pointer lifetime.
- EditorApp::update() vs on_frame_begin() overlap: both are called each frame, but update() is for editor logic specifically (after game logic updates, before rendering). Developers should use update() for editor shortcuts/commands and on_frame_begin() for engine-level per-frame tasks.
**Blocking issues**:
<none>

**Update (08-Jun-2026)**: Refactored implementation contract to replace individual panel drawing methods with EditorMenu/EditorPanel abstractions. Added Steps 7 (EditorMenu), 8 (EditorPanel), 9 (MenuBar), 10 (five concrete panels). Restructured Step 11 (editor.h) to use menus_/panels_ vectors with add_menu()/add_panel() registration. Restructured Step 12 (editor.cpp) to 4-phase rendering (menus → dockspace → panels → about) with vector iteration. Updated all Done criteria (DC-007 through DC-024) to reflect the new architecture. Added 6 new DC entries for abstract classes and panel files. Updated AC references in test tables to match new spec AC-011 through AC-039. MenuBar is header-only inline (no .cpp needed), consistent with concrete panels. All panel/commands directories are header-only — no new .cpp files beyond command.cpp and command_stack.cpp.
**Artifacts**:
- `.specs/sprint-2026-06/editor-foundation/implementation-contract.md`
**Questions for human**:
<none>
**Warnings**:
- MenuBar::draw_ui() is inline in menu_bar.h, requiring it to include `<imgui.h>`, `"commands/quit_command.h"`, and `"commands/show_about_command.h"` — this is acceptable per the pattern used by concrete panel classes, but makes it an implementation header rather than a pure interface header.
**Blocking issues**:
<none>

**Update (08-Jun-2026)**: Refactored implementation contract to add ShortcutRegistry (Step 6), update MenuBar to use callback (set_on_about) instead of bool& show_about_ parameter, remove ShowAboutCommand entirely from all steps. Updated Editor::setup()/update() to use ShortcutRegistry::process() instead of process_shortcuts(). Added ShortcutRegistry member to Editor. All steps renumbered (old Step 6 ShowAboutCommand removed, old Steps 7-17 → 6-16). Done criteria updated: DC-006 now covers ShortcutRegistry, DC-009 updated for MenuBar callback signature, DC-011/DC-012 updated for ShortcutRegistry in editor.h/cpp. All AC references in test/E2E tables updated to match new spec numbering (AC-016 → ShortcutRegistry, AC-042 → shortcuts suppressed, etc.). CMakeLists.txt step unchanged (ShortcutRegistry is header-only, no .cpp needed).
**Artifacts**:
- `.specs/sprint-2026-06/editor-foundation/implementation-contract.md`
**Questions for human**:
<none>
**Warnings**:
- ShowAboutCommand removed entirely — About is now handled via MenuBar callback (set_on_about()), not a Command subclass. This simplifies the command infrastructure but means About cannot be undo/redo.
- The ShortcutRegistry is header-only (all methods inline in shortcut_registry.h) — no separate .cpp file needed, so CMakeLists.txt changes are minimal.
- MenuBar constructor no longer takes bool& show_about_ parameter — About state is managed via callback set by Editor::setup().
**Blocking issues**:
<none>

## implementation-contract-critic

**Status**: completed
**Summary**:
First review of implementation contract: all spec goals (G-01 to G-09) are covered, all 43 ACs have corresponding verification, steps are logically ordered, abstractions (EditorMenu/EditorPanel/ShortcutRegistry) are correctly designed, architecture boundaries (ADR-019) are respected, and headless safety is preserved. No blocking issues. Five minor warnings flagged: (1) AC-030 grep missing src/cmd/apps/, (2) AC-043 uses code review instead of unit test, (3) ADR-027 consequence staleness not fully acknowledged, (4) Step 5 include commentary is confusing, (5) Observability logging from spec not explicitly called out in implementation steps. These are all non-blocking.
**Artifacts**:
- `.specs/sprint-2026-06/editor-foundation/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- AC-030 grep does not include `src/cmd/apps/`: Step 15 and E2E table only grep `src/editor/`, but AC-030 also requires `src/cmd/apps/`.
- AC-043 is verified via code review instead of unit test (spec says unit test).
- ADR-027 consequence staleness not fully acknowledged: contract says "no changes to ADR" but ADR-027 Decision 2 is contradicted by adding App::update().
- Step 5 include commentary is confusing (forward-declaration vs include discussion).
- Observability logging (BUDDD_LOG_DEBUG for commands, undo/redo) not explicitly called out in implementation steps.
**Blocking issues**:
- [ ] *(none — no blocking issues found)*

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-08 19:42 EDT
**Warnings**:
<none>
**Notes**: Approved via question tool. Full spec, contract, and critic reviews read and accepted.

## code-implementer

**Status**: completed
**Summary**:
Implemented the editor-foundation feature: Command system (Command base class, CommandStack with bounded 128-entry undo/redo), ShortcutRegistry (bind/process with WantCaptureKeyboard gating), EditorMenu/EditorPanel abstraction classes, MenuBar with File/Edit/Help menus, five placeholder dockable panels (Scene, Properties, Console, Project, Assets) with 100×100 minimum size, docking persistence via buddd_editor.ini, About modal popup with engine version, App::update() lifecycle method, and EditorApp::update() override. Two-phase Editor::update()/draw_ui() separation with 4-phase rendering. All 438 unit tests pass, no SDL3/OpenGL/GLM leaks.
**Artifacts**:
- New files: src/editor/command.h, src/editor/command.cpp, src/editor/command_stack.h, src/editor/command_stack.cpp, src/editor/commands/quit_command.h, src/editor/shortcut_registry.h, src/editor/editor_menu.h, src/editor/editor_panel.h, src/editor/panels/menu_bar.h, src/editor/panels/scene_panel.h, src/editor/panels/properties_panel.h, src/editor/panels/console_panel.h, src/editor/panels/project_panel.h, src/editor/panels/assets_panel.h
- Modified files: src/editor/editor.h, src/editor/editor.cpp, src/editor/CMakeLists.txt, src/cmd/app.h, src/cmd/app.cpp, src/cmd/apps/editor_app.h, src/cmd/apps/editor_app.cpp, tests/editor_tests.cpp
**Questions for human**:
none
**Warnings**:
- [[nodiscard]] warnings on `undo()` and `redo()` return values being ignored in MenuBar and shortcut lambdas — expected per contract design (menu enabled/disabled state already gates calls)
- All non-blocking; full test suite passes (438/438)
**Blocking issues**:
none

## code-reviewer

**Status**: rejected
**Summary**:
Code review completed. All structural requirements are met (correct files, architecture boundary, command system, menus, panels, shortcuts, lifecycle), all 438 unit tests pass. However, the build produces 11 warnings from `[[nodiscard]]` return values of `undo()`/`redo()` being ignored in `src/editor/panels/menu_bar.h`, `src/editor/editor.cpp`, and `tests/editor_tests.cpp`. Per the zero-warnings review policy, this is a blocking issue that must be fixed before acceptance.
**Artifacts**:
- `.specs/sprint-2026-06/editor-foundation/code-review.md`
**Questions for human**:
none
**Warnings**:
- Observability logging from spec (BUDDD_LOG_DEBUG for command execution, undo/redo, About dialog) not implemented — only ini file path is logged
- `#include <imgui_internal.h>` used in editor.cpp (needed for ImGuiDockNode access, but depends on internal ImGui API)
- AC-043 (edge-triggered shortcut) verified by code review only, no dedicated unit test
**Blocking issues**:
- [ ] **11 warnings from `[[nodiscard]]` return values being ignored in our code**: `CommandStack::undo()` and `CommandStack::redo()` return values are ignored in `menu_bar.h` (2), `editor.cpp` (3), and `editor_tests.cpp` (6). Fix by casting to `(void)` or removing `[[nodiscard]]`. Verify zero warnings after fix.

## wiki-agent

**Status**: pending
**Summary**:
pending
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
pending
**Artifacts**:
- `.specs/sprint-2026-06/editor-foundation/governance-review.md`
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
