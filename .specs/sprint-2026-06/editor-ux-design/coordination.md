# Workflow Coordination: editor-ux-design

## Orchestrator

**Feature**: `editor-ux-design`
**Status**: completed
**Current step**: completed
**Initial instructions**: Draft the full editor UX design as a north-star document. The editor follows an Unreal-like model: one scene tab always on the left, additional tabs for other assets (Prefabs for MVP1), and a Game tab for play mode. Each tab type has a fixed layout (user-resizable, remembered). Tabs can be detached as new OS windows. Play mode clones the editor World; Scene tab shows runtime world read-only during play. On Stop, editor World is restored. Deliverable is a comprehensive design document that covers all panels, interactions, workflows, and tab types — forming the reference for individual feature specs later.

**Notes**:
- Human prefers Unreal-like scene management (one scene at a time, prefab editing in separate tabs)
- MVP1 tab types: Scene, Prefab, Game
- Layout per tab type: fixed structure, resizable panels, sizes remembered
- Detached tabs = new OS window with own GL context / ImGui dockspace
- Play mode: clone World, Scene tab shows runtime read-only (Option B), Stop restores editor World
- File > Open replaces current scene, prompts save if dirty
- Project panel persistent for browsing, double-click to open
- Play controls in menu/toolbar area when Game tab exists

**Clarifications confirmed**:
- Q-01: New entity → child of selected entity (not root)
- Q-02: No entity name uniqueness constraint
- Q-03: Focus camera → instant snap (no animation)
- Q-04: Game tab closes entirely on Stop
- Q-05: No isolated Play for Prefabs (MVP1)
- Q-06: Detach tab → right-click context menu on tab header
- Q-07: Console persists messages across mode transitions (never clears)

## spec-author

**Status**: completed
**Summary**:
Loop 2: Resolved all 5 blocking issues from spec-critic: (1) Project panel is now a tab in the bottom panel area alongside Console and Assets, with the Scene Tab Layout diagram and panel structure description updated accordingly; (2) AC-003 now specifies new entities are appended as the last child; (3) Added a "Documentation to update" section listing wiki pages, ADR-027, and a new editor-panels page; (4) A-03 now explicitly states World::clone() is not yet implemented and is a required prerequisite for Play mode; (5) AC-010 and Play Mode visual indicator now specify definitive details (#FF3300 3px border, [Playing] title prefix, dark-red status bar, Inspector read-only with lock icons). Also addressed warnings: Undo/Redo menu now clarifies single-level entity deletion undo; SC-003/SC-004 now note aspirational nature. Updated all affected user stories, acceptance criteria, E2E verification step, panel descriptions, and the Key entities section.

**Artifacts**:
- `.specs/sprint-2026-06/editor-ux-design/spec.md`
**Questions for human**:
none
**Warnings**:
- Detached tabs (multi-window) are prioritized as P2 but may be technically complex for MVP1 — consider deferring to post-MVP1 if implementation effort exceeds initial estimates.
- Panel size persistence is session-only (A-08); if users expect persistence across restarts, this may need to be re-prioritized.
- The Undo/Redo menu now clarifies the scope (single-level entity deletion undo) but the generic Ctrl+Z/Ctrl+Shfit+Z shortcuts may still confuse users expecting standard multi-level undo.
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
All 5 previous blocking issues are resolved in the updated spec. (1) Project panel is now a tab in the bottom panel area alongside Console and Assets, with consistent layout diagrams and descriptions. (2) AC-003 now specifies "appended as the last child." (3) A "Documentation to update" section lists four documents. (4) A-03 explicitly flags World::clone() as a not-yet-implemented prerequisite. (5) AC-010 and the Play Mode section now provide definitive visual indicator details (#FF3300 border, [Playing] prefix, dark-red status bar, Inspector lock icons). No new blocking issues found. The spec satisfies all Definition of Ready criteria.

**Artifacts**:
- `.specs/sprint-2026-06/editor-ux-design/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- Detached tabs (multi-window) are prioritized as P2 but may be technically complex for MVP1 — consider deferring to post-MVP1 if implementation effort exceeds initial estimates.
- The Undo/Redo menu shows generic "Undo"/"Redo" labels but MVP1 only supports single-level undo for entity deletions. Consider renaming or adding a tooltip.
- Performance budgets (SC-003, SC-004) remain aspirational without concrete resource constraints or defined test environment.
- AC-024 panel size persistence scope is ambiguous (shared across tabs of same type vs per-tab-instance); spec text clarifies "per tab type, session only" — sufficient for north-star.
- Story 2 uses "if one is selected" which is slightly ambiguous vs spec text's "if a single entity is selected"; AC-003 is definitive.
- Permissions statement about "file system access only within project directory" is slightly contradicted by unrestricted OS file dialogs for Open/Save.
**Blocking issues**:
none

## implementation-contract-author

**Status**: pending
**Summary**:

**Artifacts**:
- `.specs/sprint-2026-06/editor-ux-design/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: pending
**Summary**:

**Artifacts**:
- `.specs/sprint-2026-06/editor-ux-design/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Validation

**Status**: pending
**Approver**: 
**Date**: 
**Warnings**:
none
**Notes**: 

## code-implementer

**Status**: pending
**Summary**:

**Artifacts**:
- 
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: pending
**Summary**:

**Artifacts**:
- `.specs/sprint-2026-06/editor-ux-design/code-review.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
Created ADR-029 (`docs/adr/ADR-029-editor-ux-decisions.md`) capturing the 10 key architectural UX decisions from the editor-ux-design spec. Decisions cover: tab-as-editor-context model, one-scene-at-a-time, fixed layout per tab type, detached tabs as separate OS windows, Play mode with World cloning + read-only inspection, bottom panel tab bar (Project/Console/Assets), entity creation as child of selected, prefab editing in tabs, Console persistence across mode transitions, and Play mode visual indicator (#FF3300 border, [Playing] title, dark-red status bar, read-only Inspector). References ADR-027 (Editor Architecture), ADR-026 (ImGui), and ADR-019 (Architecture Boundaries).

**Artifacts**:
- `docs/adr/ADR-029-editor-ux-decisions.md`
**Decisions needed**:
none
**Questions for human**:
none
**Warnings**:
- `World::clone()` is a prerequisite for Play mode (spec A-03) — no implementation of Play mode can begin until the engine gains deep-clone capability.
- Detached tabs (multi-window) require the editor library to own SDL window creation through the engine's `Window` abstraction, preserving the ADR-019 architecture boundary.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Applied two documentation fixes identified by governance-reviewer: (1) Updated ADR-029 status from "in-progress" to "Accepted" in `docs/wiki/editor/editor-panels.md`; (2) Updated `docs/wiki/decisions/adr-index.md` to add missing entries for ADR-025 (gap — no file exists), ADR-026 (ImGui integration, Accepted), ADR-027 (Editor architecture, Accepted), ADR-028 (Component type registry, Accepted), and ADR-029 (Editor UX decisions, Accepted).

**Loop 2:** Created comprehensive wiki coverage for the full editor UX workflow. Created two new wiki pages (`scene-management.md`, `cross-panel-communication.md`). Added 7 new subsections to `editor-panels.md` (Editor Camera Controls, Inspector Property Editors, Edge Cases, Error Cases, Log Channels, Assumptions, Decisions). Updated `docs/wiki/README.md` with links to the new pages.
**Artifacts**:
- `docs/wiki/editor/editor-panels.md`
- `docs/wiki/decisions/adr-index.md`
- `docs/wiki/editor/scene-management.md` (new)
- `docs/wiki/editor/cross-panel-communication.md` (new)
**Changes made**:
- Fixed `docs/wiki/editor/editor-panels.md` line 309: changed ADR-029 status from "in-progress" to "Accepted".
- Added 5 missing ADR entries (ADR-025 through ADR-029) to `docs/wiki/decisions/adr-index.md`, reading actual ADR files to get correct titles and confirming all as Accepted (ADR-025 noted as a gap — no file found).
- Created `docs/wiki/editor/scene-management.md` with scene file operations, dirty state rules, scene replacement workflow, and untitled scene behavior.
- Created `docs/wiki/editor/cross-panel-communication.md` with entity selection flow diagram, play mode state transition diagram, full panel state per mode table, and visual play mode indicator summary.
- Added **Editor Camera Controls** subsection to Viewport Panel section in `editor-panels.md` with full camera input table.
- Added **Inspector Property Editors** subsection to Inspector Panel section with type-to-widget mapping table and Add/Remove component notes.
- Added **Edge Cases** section (EC-01 through EC-14) as reference table.
- Added **Error Cases** section (ER-01 through ER-11) as reference table.
- Added **Log Channels** section with channel listing and key log signal table.
- Added **Assumptions** section (A-01 through A-13) from spec.
- Added **Decisions** section (D-01 through D-10) from spec Q&A with rationale.
- Updated `docs/wiki/README.md` with entries for the two new wiki pages.
**Questions for human**:
none
**Warnings**:
- ADR-025 has no corresponding file in `docs/adr/` — the ADR sequence has a gap at ADR-025 that may need investigation.
- `World::clone()` is a prerequisite for Play mode (spec A-03) — no implementation of Play mode can begin until the engine gains deep-clone capability.
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Validated cross-document coherence across spec.md, ADR-029, ADR-027, and the wiki. All documents are consistent with no fundamental contradictions. ADR-029 correctly captures the 10 key UX decisions from the spec and does not contradict ADR-027 (technical architecture). The wiki page faithfully reflects the spec and ADRs. The wiki README links to the new page. Two minor documentation issues found: (1) wiki references ADR-029 as "in-progress" but it is Accepted; (2) ADR Index is stale (missing ADR-025–029). These are non-blocking documentation updates.

**Artifacts**:
- `.specs/sprint-2026-06/editor-ux-design/governance-review.md`
**Questions for human**:
none
**Warnings**:
- Wiki references ADR-029 as "in-progress" on line 309 of `docs/wiki/editor/editor-panels.md` but ADR-029 is Accepted. Needs update.
- `docs/wiki/decisions/adr-index.md` is stale — only lists ADRs up to ADR-024. ADR-025–029 are missing.
- No implementation-contract.md or code-review.md exist yet (expected — workflow is at `adr+wiki-publishing` stage).
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
