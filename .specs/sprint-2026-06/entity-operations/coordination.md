# Workflow Coordination: entity-operations

## Orchestrator

**Feature**: `entity-operations`
**Status**: in-progress
**Current step**: governance-review
**Initial instructions**: Implement F-04 (Scene Panel — Entity Operations: Create, Delete, Rename) from the editor feature breakdown. Prerequisites (F-00, F-03) should be completed. Follow Command pattern for undo/redo. Create unit tests for all operations.
**Notes**: All design decisions recorded in Decision Log below.

## Decision Log

### Clarification / Grill-me (2026-06-12)

| # | Question | Decision | Rationale |
|---|---|---|---|
| 1 | Create entity UX | Right-click context menu only (no toolbar button). "Create Empty" option. | Human chose context menu only. |
| 2 | Delete entity UX | Right-click + Delete key + confirmation dialog when entity has children. Multi-select delete with single confirmation. No confirmation for leaf entities. | Human chose recommended option. |
| 3 | Rename entity UX | F2 key triggers inline rename (no double-click). Also available via right-click context menu "Rename" option. Empty name rejected (reverts to previous). | Human chose F2 only, then added right-click context menu rename. |
| 4 | Undo/redo scope | All three operations (Create, Delete, Rename) use Command pattern and support undo/redo. | Human chose all operations. |
| 5 | Selection after delete | Cleared after deletion. On undo, selection is restored via snapshot. | Human chose clear selection. |
| 6 | flush_destroyed() placement | Called once per frame in Editor::update() after command processing, before panel rendering. | Human chose Editor::update() each frame. |
| 7 | Command access to Editor/World | Commands receive `EditorContext const&` in `execute()` and `undo()`. Command::execute()/undo() signatures changed to accept EditorContext. CommandStack passes it through. | Human chose EditorContext pattern. |
| 8 | Create entity position | New entity becomes last child of the selection anchor (`EditorSelection::anchor()`). Root if no anchor. | Human chose anchor concept. |
| 9 | Command files structure | One file per command in `src/editor/commands/`: `create_entity_command.h`, `delete_entity_command.h`, `rename_entity_command.h`. | Human chose commands/ subdirectory. |
| 10 | Context menu implementation | ImGui context menu popup (`ImGui::BeginPopupContextItem`) built directly in ScenePanel's draw_ui(). | Human chose in-ScenePanel context menu. |
| 11 | Delete/F2 key handling | Handled in ScenePanel's draw_ui(), gated by `ImGui::IsWindowFocused()`. | Human chose ScenePanel focus-gated. |

---

## spec-author

**Status**: completed
**Summary**:
Fixed 2 blocking issues from spec-critic: (1) Qualified SC-003 to note component state preservation is deferred (entity identity/name/hierarchy only) — resolves contradiction with A-09; updated A-09 language to clarify v1 scope, added note in Delete behavior and Error cases. (2) Replaced infeasible delete-while-rename edge case (InputText consumes Delete key) with correct behavior — user must confirm/cancel rename first. Also fixed: AC-04 now explicitly mentions QuitCommand; added MenuBar undo/redo call-site to Key entities. Fixed all 4 warnings.
**Artifacts**:
- `.specs/sprint-2026-06/entity-operations/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review: all 2 blocking issues and 4 warnings resolved. SC-003 now qualified (entity identity/name/hierarchy only, component state deferred). Delete-while-rename now correctly states InputText consumes Delete key — user must confirm/cancel rename first. MenuBar documented in Key entities. AC-04 explicitly mentions QuitCommand. Delete undo data loss noted in user-visible behavior and error cases. No new issues. Spec accepted.
**Artifacts**:
- `.specs/sprint-2026-06/entity-operations/spec-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Fixed B-06: After `DeleteEntityCommand::undo()` recreates entities, `entity_ids_` now gets updated with the new EntityIds from the old→new mapping (the `recreated` vector). This ensures redo's `execute()` can find the recreated entities by their current IDs instead of searching for stale IDs and silently no-oping. The fix adds a loop at the end of `undo()` that iterates `entity_ids_` and replaces each old ID with its corresponding new ID from the `recreated` mapping. Updated the notes and DC-06 to reflect this.
**Artifacts**:
- `.specs/sprint-2026-06/entity-operations/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- The entity tree traversal for parent/entity lookup (used in CreateEntity and DeleteEntity commands) is O(n) per operation; a future World::get_entity(EntityId) API would be an optimization. Acceptable for v1.
- The ScenePanel context menu and keyboard handling integration is complex; testing data flow between the panel, commands, and EditorContext requires careful implementation of the helper methods.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Final re-review: B-06 fix verified correct. DeleteEntityCommand::undo() now updates entity_ids_ with new EntityIds from the old→new mapping after recreating entities. All 6 blocking issues (B-01 through B-06) resolved. Contract accepted.
**Artifacts**:
- `.specs/sprint-2026-06/entity-operations/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- W-01: `Editor::selection()` missing const overload (non-blocking, documented in wiki but not required for F-04).
- W-02: Test infrastructure is heavyweight (CmdTestHelper creates full EngineService; use minimal stub if possible).
- W-03: Confirmation dialog control flow is fragile (deleted bool + early return pattern).
- W-04: Documentation impact not tracked in Done Criteria (wiki updates listed but not in DC checklist).
- W-06: Inline rename TreeNodeEx interaction with ImGui tree state is fragile (NoTreePushOnOpen may cause state corruption).
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-12 14:13
**Warnings**:
none
**Notes**: Approved to proceed with implementation.

## code-implementer

**Status**: completed
**Summary**:
Fixed 4 bugs reported by human: (1) Create Empty via right-click now passes right-clicked entity as explicit parent to CreateEntityCommand. (2) Undo→Redo works — CreateEntityCommand stores explicit parent in constructor (persists across undo/redo). (3) Rename context menu uses right-clicked entity (not selection). (4) F2 crash fixed — removed NoTreePushOnOpen flag from rename TreeNodeEx (caused PopID mismatch). Also fixed [[nodiscard]] warnings (static_cast<void> on undo/redo calls). All 574 tests pass, zero warnings.
**Artifacts**:
- `src/editor/command.h` (modified — signature change)
- `src/editor/command_stack.h` (modified — signature change)
- `src/editor/command_stack.cpp` (modified — forward ctx, logging)
- `src/editor/commands/quit_command.h` (modified — use ctx.engine)
- `src/editor/commands/create_entity_command.h` (created)
- `src/editor/commands/delete_entity_command.h` (created)
- `src/editor/commands/rename_entity_command.h` (created)
- `src/editor/editor.h` (modified — command_stack() accessor)
- `src/editor/editor.cpp` (modified — flush_destroyed, shortcuts)
- `src/editor/panels/menu_bar.h` (modified — undo/redo pass ctx)
- `src/editor/panels/scene_panel.h` (modified — context menus, rename, delete confirmation)
- `tests/editor/editor_tests.cpp` (modified — ToggleCommand, pass EditorContext)
- `tests/editor/entity_operations_tests.cpp` (created)
**Questions for human**:
none
**Warnings**:
- The `DeleteEntityCommand` now only destroys top-level entities (not children individually) since `Entity::destroy()` recursively marks descendants via `mark_for_destroy`. This avoids a dangling pointer issue in `World::flush_destroyed()` when a parent is flushed before its child.
- Two wiki documentation updates are needed (listed in spec): `docs/wiki/editor/editor-panels.md` and `docs/wiki/editor/cross-panel-communication.md` — deferred to the wiki-agent.
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
All 5 human-reported bugs verified fixed: (1) Create Empty uses right-clicked entity via explicit parent, (2) Undo→Redo of child creation works (explicit_parent_ persists across cycles), (3) Rename context menu uses right-clicked entity (always enabled), (4) F2 crash fixed (NoTreePushOnOpen removed, TreePop balanced), (5) [[nodiscard]] warnings resolved (13 calls wrapped with static_cast<void>). Build: zero warnings from src/ or tests/. Tests: 574 pass (22179 assertions). No forbidden files modified. One new non-blocking finding: stored_parent_id_ dead variable (set, never read) — no functional impact. Verdict: Accepted.
**Artifacts**:
- `.specs/sprint-2026-06/entity-operations/code-review.md`
**Questions for human**:
none
**Warnings**:
- W-06: `stored_parent_id_` in `CreateEntityCommand` is set at line 38 but never read. Dead code — the member and its write can be removed in a cleanup pass. No functional impact.
- W-01: DeleteEntityCommand destroy strategy differs from contract text (uses recursive destroy instead of reverse-order).
- W-02: Confirmation dialog control flow is fragile (deleted bool + early return).
- W-04: Context menu appears during inline rename (right-click during rename may cause unexpected behavior).
- W-05: Documentation impact not tracked as deliverables (wiki updates deferred).
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated wiki to document F-04 entity operations (Create Empty, Delete, Rename), context menu behavior, keyboard shortcuts (Delete, F2), inline rename UX, confirmation dialog for hierarchical deletion, Command signature change (execute/undo now accept EditorContext const&), selection snapshot/restore pattern, and flush_destroyed() lifecycle.
**Artifacts**:
- `docs/wiki/editor/editor-panels.md` (modified)
- `docs/wiki/editor/cross-panel-communication.md` (modified)
- `docs/wiki/editor/entity-selection.md` (modified)
**Changes made**:
- **editor-panels.md**: Updated status banner to include F-04; added F-04 bullet to implemented features; updated Scene Panel content section (removed "deferred" language, documented CRUD operations); corrected Entity Operations table to match actual F-04 behavior (context menu only, no + button, F2 not double-click, full hierarchy undo); added F-04 additions to v1 foundation section; updated A-09 assumption; added F-04 spec to Related specs; updated Last reviewed.
- **cross-panel-communication.md**: Updated status banner for F-04; added F-04 entity operation path to MVP1 Selection Paths table; added F-04 additions to v1 foundation section; updated Last reviewed.
- **entity-selection.md**: Updated Selection Lifecycle table (F-04 now handles entity-destroyed selection); updated Snapshot/Restore section to reference all three F-04 Commands; updated Last reviewed.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Governance review completed. All checks pass: no ADR contradictions, wiki files are consistent with implementation, all 34 acceptance criteria are addressed, all 16 done criteria verified. No blocking issues found. Four minor warnings documented (cosmetic DC checkboxes, destroy strategy deviation, fragile confirmation dialog, context menu during rename).
**Artifacts**:
- `.specs/sprint-2026-06/entity-operations/governance-review.md`
**Questions for human**:
none
**Warnings**:
- Done Criteria not checked in contract (DC-01–16 still `[ ]` — cosmetic, no functional impact)
- DeleteEntityCommand destroy strategy deviates from contract text (recursive top-level-only vs reverse-order) — correct but undocumented
- Confirmation dialog control flow is fragile (deleted bool + early return pattern)
- Context menu appears during inline rename (may cause unexpected focus-loss confirmation)
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
