# Workflow Coordination: entity-selection

## Orchestrator

**Feature**: entity-selection
**Status**: completed
**Current step**: completed
**Initial instructions**: Implement F-03 (Entity Selection) from the editor feature breakdown. Click an entity in ScenePanel → it becomes selected. Inspector clears when nothing selected. Create a `Selection` value class and an `EditorSelection` manager for multi-select support. Selection stability across undo/redo is handled by Commands (via Selection snapshots). The human wants multi-select from the start.

**Notes**:
- Design decisions (12-Jun-2026, clarification):
  - `Selection` is a pure value object (cloneable, comparable, savable in Commands). Has `contains()`, `size()`, `add()`, `remove()`, `clear()`, `first()`, iteration.
  - `EditorSelection` owns the *active* `Selection`, fires change callbacks. Provides `select(id, modifier)`, `clear()`, `set_selection(ids)`, `snapshot()`/`restore(Selection)`, `anchor()`/`set_anchor()` for Shift+click, `on_change()`.
  - `SelectionModifier` enum: `Replace` (plain click: clear+select), `Toggle` (Ctrl+click: add/remove)
  - Undo/redo stability: Commands call `editor.selection().snapshot()` at construction, `editor.selection().restore(saved)` on undo.
  - Multi-select: Ctrl+click toggle, Shift+click range (linearized tree order), Ctrl+A select all. ScenePanel handles tree traversal for range/all.
  - F-03 scope: full multi-select from the start. PropertiesPanel stays as empty placeholder.
  - Callbacks deferred or minimal in F-03 (no consumers yet — Inspector/Viewport come in F-05/F-07).
  - Prerequisites: F-00 (Editor World), F-01 (Scene Load/Save), F-02 (Scene Panel Entity Tree) — all completed
- Grill-me decisions (12-Jun-2026):
  - **Re-click behavior**: No-op — clicking already-selected entity stays selected
  - **Selection on destroy**: Calling code clears explicitly (not auto-clear)
  - **Selection API**: `Editor::selection()` accessor returns `EditorSelection&`
  - **Selection highlighting**: `ImGuiTreeNodeFlags_Selected` on ScenePanel tree nodes
  - **PropertiesPanel in F-03**: Keep empty placeholder (no "No entity selected" yet — that's F-05)
  - **Multi-select scope**: Ctrl+click toggle, Shift+click range (linearized tree order), Ctrl+A select all
  - **Shift+click range semantics**: Linearized tree order (depth-first)
  - **Multi-select Inspector behavior**: Not touched in F-03
- Loop-back #1 (12-Jun-2026): implementation-contract-critic rejected with two blocking issues (B-01: std::minmax dangling reference in collect_range, B-02: missing modifier-key guard in empty-area click). Looping to implementation-contract-author.

## spec-author

**Status**: completed
**Summary**:
Created SPEC-F-03 (Entity Selection with Multi-Select) spec at `.specs/sprint-2026-06/entity-selection/spec.md`. Covers `Selection` value class, `EditorSelection` manager with `SelectionModifier` enum, multi-select interactions (plain click, Ctrl+click toggle, Shift+click range, Ctrl+A select all, empty-area clear), `ImGuiTreeNodeFlags_Selected` highlighting, Undo/redo snapshot/restore future-proofing, and selection cleared on new_scene/open_scene. Includes 32 acceptance criteria (AC-01 through AC-32), 9 user stories with GWT format, edge cases, error cases, and documentation impact.
**Artifacts**:
- `.specs/sprint-2026-06/entity-selection/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Spec F-03 (Entity Selection with Multi-Select) is thorough, internally consistent, and aligned with ADR-027, ADR-029, F-02, and the existing wiki. All Definition of Ready criteria are satisfied. 32 acceptance criteria, 15 edge cases, 8 error cases, and full API documentation. No blocking issues found. The spec is ready for implementation contract authoring.
**Artifacts**:
- `.specs/sprint-2026-06/entity-selection/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- Callback firing on no-op mutations is unspecified (harmless in F-03, clarify in implementation contract)
- Anchor asymmetry: `clear()` clears anchor but `set_selection({})` does not (documented but subtle)
- Double-click edge: NG-09 doesn't clarify that first click of a double-click still selects (standard ImGui behavior)
- Future Command integration path: `Command::execute()` takes `EngineContext const&`, not `EditorContext const&` — how Commands access `Editor::selection()` needs future resolution
**Blocking issues**:
- [x] None — all Definition of Ready criteria are satisfied

## implementation-contract-author

**Status**: completed
**Summary**:
Fixed two blocking issues from contract-critic: (B-01) replaced dangling-reference `std::minmax` in `collect_range` with manual `(std::min)`/`(std::max)` computation; (B-02) added `!KeyCtrl && !KeyShift` guard to empty-area click. Also added `EntityId::none()` guard to inline pseudocode for `select()` and `set_selection()` per critic's recommendation. Contract is now ready for implementation.
**Artifacts**:
- `.specs/sprint-2026-06/entity-selection/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
- [x] B-01: `std::minmax` dangling reference UB in `collect_range` — fixed, replaced with manual min/max
- [x] B-02: Empty-area click missing modifier-key guard — fixed, added `!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift` guard

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review (loop-back #1): Both blocking issues resolved. B-01: `std::minmax` dangling reference replaced with safe manual min/max. B-02: Empty-area click now guarded with `!KeyCtrl && !KeyShift` per AC-27. `EntityId::none()` guard now explicit in all inline pseudocode. No new issues found. Contract is ready for implementation.
**Artifacts**:
- `.specs/sprint-2026-06/entity-selection/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- Anchor asymmetry documented but subtle (`clear()` clears anchor, `set_selection({})` does not).
- No `const` overload for `Editor::selection()` (deferred, acceptable for F-03).
- Callbacks fire on every mutation regardless of net state change (acceptably simple for F-03).
**Blocking issues**:
- [x] B-01: `std::minmax` dangling reference UB in `collect_range` — RESOLVED in loop-back #1, replaced with manual min/max.
- [x] B-02: Empty-area click missing modifier-key guard — RESOLVED in loop-back #1, `!KeyCtrl && !KeyShift` guard added.

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-12 12:42:38 EDT
**Warnings**:
none
**Notes**: Approved via question tool. Full spec, contract, and critic reviews read and accepted. Multi-select with Ctrl+click toggle, Shift+click range, Ctrl+A select all.

## code-implementer

**Status**: completed
**Summary**:
Implemented F-03 (Entity Selection with Multi-Select): Created `editor_selection.h` with `Selection` value class, `EditorSelection` manager, and `SelectionModifier` enum. Wired selection into `Editor` (member + accessor, cleared on new/open scene, Ctrl+A shortcut). Updated `ScenePanel` with entity click handling (plain/Ctrl/Shift), selection highlighting via `ImGuiTreeNodeFlags_Selected`, empty-area click clearing, and tree traversal helpers for range selection. Added 18 unit tests covering all selection operations, copy semantics, callbacks, anchor management, snapshot/restore, and Editor integration.
**Artifacts**:
- `src/editor/editor_selection.h` (created)
- `src/editor/editor.h` (modified)
- `src/editor/editor.cpp` (modified)
- `src/editor/panels/scene_panel.h` (modified)
- `tests/editor/f03_entity_selection_tests.cpp` (created)
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
All 32 acceptance criteria and 8 done criteria are satisfied. The implementation creates the `Selection` value class, `EditorSelection` manager, and `SelectionModifier` enum, wires multi-select into `Editor` and `ScenePanel` (plain/Ctrl/Shift click, Ctrl+A, empty-area clear), and provides 21 passing unit tests. Build produces zero warnings. Both blocking issues from the contract-critic (std::minmax UB, empty-area modifier guard) are correctly resolved. No changes to forbidden files. One minor non-blocking warning: a temporary file leak in the open_scene() test.
**Artifacts**:
- `.specs/sprint-2026-06/entity-selection/code-review.md`
**Questions for human**:
none
**Warnings**:
- Test temp file leak: `open_scene()` test creates a file via `mkstemp` that is never cleaned up (only the `.yaml` variant is removed).
- Spec doc inaccuracy: `snapshot()` description says "including anchor" but `Selection` is a pure set-of-ids value object (anchor is stored separately).
**Blocking issues**:
- [ ] None found

## wiki-agent

**Status**: completed
**Summary**:
Updated `docs/wiki/editor/editor-panels.md` with F-03 banner, Scene Panel selection behavior (click/Ctrl+click/Shift+click/Ctrl+A), selection API documentation, and v1 foundation entries. Updated `docs/wiki/editor/cross-panel-communication.md` with F-03 banner, new EditorSelection path in the MVP1 selection table, and v1 foundation additions. Removed "Multi-select" from deferred list.
**Artifacts**:
- `docs/wiki/editor/editor-panels.md` (modified)
- `docs/wiki/editor/cross-panel-communication.md` (modified)
**Changes made**:
- `editor-panels.md`: Updated current-status banner to include F-03; added F-03 bullet to implemented features list; updated Scene Panel section to describe click-to-select, multi-select, highlighting, and `ImGuiTreeNodeFlags_Selected`; removed "Multi-select" from deferred list; added F-03 entries to v1 foundation section documenting `Selection`, `EditorSelection`, `SelectionModifier`, and `ctx.editor.selection()` access pattern.
- `cross-panel-communication.md`: Updated current-status banner to reflect that entity selection is partially implemented (hierarchy click → EditorSelection state); added "Hierarchy click → EditorSelection" row to MVP1 Selection Paths table (Implemented F-03); marked "Hierarchy click → Inspector + Viewport" as deferred (F-05/F-07); added F-03 v1 foundation entries.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Governance review completed for F-03. All cross-document consistency checks pass: spec aligns with ADRs (ADR-027, ADR-029, ADR-019), implementation matches spec and contract (all 32 ACs, 8 DCs satisfied), wiki accurately reflects current state (editor-panels.md, cross-panel-communication.md updated). Architecture boundary fully respected (no SDL3/OpenGL/GLM headers in src/editor/). No blocking issues. Three non-blocking warnings noted (test temp file leak, minor spec doc inaccuracy on snapshot() anchoring, documented anchor asymmetry).
**Artifacts**:
- `.specs/sprint-2026-06/entity-selection/governance-review.md`
**Questions for human**:
none
**Warnings**:
- Test temp file leak: `open_scene()` test creates a mkstemp file that is never cleaned up (only the `.yaml` variant is removed).
- Spec doc inaccuracy: `snapshot()` description says "including anchor" but anchor is stored separately in `EditorSelection`, not inside `Selection`.
- Anchor asymmetry documented but subtle: `clear()` clears anchor, `set_selection({})` does not.
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
