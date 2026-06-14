# Workflow Coordination: scene-panel-right-click

## Orchestrator

**Feature**: `scene-panel-right-click`
**Status**: completed
**Current step**: completed
**Initial instructions**: Right-click in the Scene Panel should select the entity under the cursor before opening the context menu, so that "Delete" (and future actions) operate on the right-clicked entity. If the entity is already selected, do nothing. Ctrl/Shift modifiers behave like left-click but never deselect. If Ctrl+right-click or Shift+right-click on a non-selected entity, apply the modifier behavior (Toggle-add for Ctrl, Range for Shift). If the entity is already in the selection, do nothing.
**Notes**:
- Plain right-click on non-selected entity → Replace (clear + select this entity)
- Ctrl+right-click on non-selected entity → Toggle (add to selection)
- Shift+right-click on non-selected entity → Range select
- Right-click on already-selected entity → No-op (never deselect on right-click)
- Specs F-03 (Entity Selection) NG-10 and F-04 (Entity Operations) AC-32 must be updated to reflect new behavior

## spec-author

**Status**: completed
**Summary**:
Created the functional spec for right-click selection behavior in the Scene Panel. The spec defines how right-click + modifier combinations (plain, Ctrl, Shift) change the selection before opening the context menu. Documents the "never deselect on right-click" invariant, the impact on existing specs F-03 (remove NG-10) and F-04 (update AC-32), and provides full user stories, acceptance criteria, edge cases, and the specific code change in scene_panel.cpp.
**Artifacts**:
- `.specs/sprint-2026-06/scene-panel-right-click/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**: The spec is accepted with no blocking issues. It clearly defines right-click selection behavior (plain, Ctrl, Shift) with a "never deselect on right-click" invariant, well-documented edge cases, and accurate impact on existing specs F-03 and F-04. All Definition of Ready criteria are satisfied. Minor warnings about implicit modifier fallthrough and AC-12/AC-13 dependency on F-04 context menu are noted but non-blocking.
**Artifacts**:
- `.specs/sprint-2026-06/scene-panel-right-click/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- Unspecified modifier combinations (e.g., Alt+right-click) fall through to Replace behavior in the pseudocode but are not explicitly documented. Consider adding a brief note about other modifier behavior.
- AC-12 and AC-13 rely on the context menu implementation from F-04; this dependency could be more explicitly stated.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Created the implementation contract for Scene Panel right-click selection. The contract specifies replacing lines 99-106 of `scene_panel.cpp` with selection-aware right-click logic that applies modifier behavior (Replace for plain, Toggle for Ctrl, Range for Shift) before opening the context menu, with an explicit "never deselect" invariant. Also specifies updating the F-03 spec (remove NG-10) and F-04 spec (update AC-32). Manual test checklist mapped to all 16 acceptance criteria is included.
**Artifacts**:
- `.specs/sprint-2026-06/scene-panel-right-click/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- The spec-critic noted that Alt+right-click and other unhandled modifiers fall through to Replace in the pseudocode. The contract explicitly documents this fallthrough in the "Required implementation behavior" table. No action needed unless the human disagrees with this default.
- The spec-critic noted that AC-12/AC-13 depend on F-04 context menu implementation. The contract preserves this dependency (it is correct by design).
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Accepted the implementation contract for Scene Panel right-click selection. The contract is precise, accurate, and well-aligned with the spec. It provides exact code replacement blocks matching the actual source, handles all 14 edge cases, respects the "never deselect on right-click" invariant, and correctly uses existing EditorSelection APIs. No blocking issues found; three minor warnings noted (debug log detail, F-04 dependency, optional testing).
**Artifacts**:
- `.specs/sprint-2026-06/scene-panel-right-click/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- Debug log is simpler than the spec's recommendation (no modifier/entity ID in the message). Consider enriching to match the spec's Observability section for better traceability.
- AC-12/AC-13 depend on F-04's context menu implementation reading `selection().empty()`. The implementer should verify F-04's context menu contract before testing these ACs.
- No automated tests are required per the spec, but adding a test for right-click selection logic would improve regression protection. The contract already allows it.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: guillaume
**Date**: 2026-06-14 12:00:00
**Warnings**: none
**Notes**: Approved to proceed with implementation of Scene Panel right-click selection behavior.

## code-implementer

**Status**: completed
**Summary**:
Modified the right-click detection block in `ScenePanel::draw_ui()` to apply selection changes before opening the context menu. Right-click on a non-selected entity now selects it (Replace for plain, Toggle for Ctrl, Range for Shift). Right-click on an already-selected entity is a no-op. Also updated F-03 entity-selection spec (removed NG-10) and F-04 entity-operations spec (updated AC-32).
**Artifacts**:
- `src/editor/panels/scene_panel.cpp` (modified)
- `.specs/sprint-2026-06/entity-selection/spec.md` (modified)
- `.specs/sprint-2026-06/entity-operations/spec.md` (modified)
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
The implementation is approved. The right-click detection block in `ScenePanel::draw_ui()` correctly applies selection changes (Replace, Toggle-add, Range, or No-op) before opening the context menu, enforcing the "never deselect on right-click" invariant. Only the 3 allowed files were modified. Build produces zero new warnings, all 697 existing tests pass (22658 assertions), and the editor renders correctly with a demo scene. The F-03 and F-04 spec updates match requirements exactly.
**Artifacts**:
- `.specs/sprint-2026-06/scene-panel-right-click/code-review.md`
**Questions for human**:
- AC-12 and AC-13 depend on F-04's context menu reading selection state correctly. Manual testing should confirm the context menu respects the new selection state.
**Warnings**:
- Debug log on right-click selection uses a simplified message ("set"/"unchanged") rather than the richer format suggested in the spec's Observability section (modifier + entity ID). This is non-blocking.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated three wiki files to document the new right-click selection behavior in the Scene Panel. In editor-panels.md, updated the status line, Scene Panel selection bullet, and Entity Operations table. In entity-selection.md, added a "Right-click Selection" subsection with the modifier rules table and "never deselect on right-click" invariant, and updated the Selection Lifecycle table. Verified cross-panel-communication.md needs no changes (right-click uses the same EditorSelection API).
**Artifacts**:
- `docs/wiki/editor/editor-panels.md` (modified)
- `docs/wiki/editor/entity-selection.md` (modified)
**Changes made**:
- Added "right-click selection" to the current status line in both files
- Updated Scene Panel selection bullet to document right-click behavior (plain → Replace, Ctrl → Toggle-add, Shift → Range, never deselect)
- Updated Entity Operations "Select entity" row to note right-click also selects
- Added Right-click Selection subsection with full rules table and the "never deselect on right-click" invariant
- Added 8 new rows to the Selection Lifecycle table for right-click events
- Updated Last reviewed dates
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Governance review approved. All cross-document checks pass: spec-to-contract coverage is complete, contract-to-code implementation is exact, code builds with zero warnings and all tests pass, F-03 and F-04 spec updates are accurate, wiki updates reflect the new behavior, all relevant ADRs (ADR-026, ADR-027, ADR-029) are respected, coordination.md is complete with no skipped gates. No blocking issues found.
**Artifacts**:
- `.specs/sprint-2026-06/scene-panel-right-click/governance-review.md`
**Questions for human**:
- AC-12 and AC-13 depend on F-04's context menu reading selection state correctly. Manual testing should confirm the context menu respects the new selection state.
**Warnings**:
- Debug log on right-click selection is simpler than the spec's Observability recommendation (no modifier/entity ID). This is non-blocking.
- AC-12/AC-13 dependency on F-04 context menu implementation is documented. Manual verification recommended.
**Blocking issues**:
none
