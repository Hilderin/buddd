# Governance Review — Scene Panel Right-Click Selection

## Verdict

**APPROVED** — All cross-document governance checks pass. No blocking issues found.

## Cross-document coherence

Contradictions or gaps between spec, contract, code, tests, wiki, and external specs:

- [x] **Spec-to-contract**: All 8 goals (G-01 through G-08) and 16 acceptance criteria (AC-01 through AC-16) from the spec are covered in the implementation contract. The right-click selection rules table is faithfully reproduced. ✅
- [x] **Contract-to-code**: The implementation contract's AFTER block (lines 94–133) exactly matches the actual code at `src/editor/panels/scene_panel.cpp` lines 99–135. The selection mutation happens before `context_menu_entity_` and `open_context_menu` are set. ✅
- [x] **Code-to-tests**: Code review confirms builds succeed with zero new warnings, all 697 existing tests pass (22658 assertions). ✅
- [x] **F-03 spec update**: `.specs/sprint-2026-06/entity-selection/spec.md` — NG-10 (right-click no-op) has been removed from Non-goals. The former NG-11 ("No changes to World or Entity classes") is correctly renumbered to NG-10. The "Right-click selection behaviour" bullet is removed from Out of scope. ✅
- [x] **F-04 spec update**: `.specs/sprint-2026-06/entity-operations/spec.md` — AC-32 has been updated from "Context menu does not change selection — right-click alone does not select." to "Right-click on a non-selected entity selects it (Replace) before the context menu opens. Right-click on an already-selected entity does not change the selection." ✅
- [x] **Cross-spec consistency**: The right-click selection feature's spec references F-03 and F-04 correctly. The F-03 and F-04 specs are now internally consistent with the new right-click behavior. No remaining contradictions. ✅
- [x] **coordination.md consistency**: All previous gate statuses are consistent (no skipped gates). The workflow progressed in order: orchestrator → spec-author → spec-critic → implementation-contract-author → implementation-contract-critic → Human Validation → code-implementer → code-reviewer → wiki-agent → governance-reviewer. All prior sections are marked `completed` or `approved`. ✅

## ADR alignment

Required ADRs exist and are not violated:

- [x] **ADR-026 (ImGui Integration)**: Right-click detection uses `ImGui::IsItemHovered` / `ImGui::IsMouseReleased` / `ImGui::GetIO()` per existing conventions. No changes to the ImGui integration layer. No SDL3/OpenGL headers introduced in editor code. ✅
- [x] **ADR-027 (Editor Architecture)**: Only `src/editor/panels/scene_panel.cpp` was modified within the `buddd_editor` static library. No architecture boundaries crossed. No new files, no public API changes. ✅
- [x] **ADR-029 (Editor UX Decisions)**: Entity selection flow is extended with right-click behavior while preserving the existing left-click multi-select UX. The "never deselect on right-click" invariant is consistent with the UX design direction that right-click selection must be intuitive and safe. No ADR amendments needed. ✅
- [x] **No new ADR required**: The feature does not introduce any novel architecture decisions, API changes, or technology choices that warrant a new ADR. Selection is already documented as non-undoable (ADR-029), and right-click selection follows the same rules as left-click with the additional "never deselect" invariant. ✅

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **docs/wiki/editor/editor-panels.md**: Updated status line mentions "right-click selection". Scene Panel selection bullet documents right-click behavior (plain → Replace, Ctrl → Toggle-add, Shift → Range, never deselect). Entity Operations table notes "right-click on non-selected entity also selects it before context menu". Documentation is accurate and reflects current implementation state. ✅
- [x] **docs/wiki/editor/entity-selection.md**: Updated status line mentions "right-click selection". Right-click Selection subsection added with full rules table and "never deselect on right-click" invariant. Selection Lifecycle table includes 8 new rows for right-click events. Last reviewed date updated to 2026-06-14. Documentation is accurate and reflects current implementation state. ✅
- [x] **Wiki does not become law**: The wiki is documented as operational understanding, not authoritative specification. The spec at `.specs/sprint-2026-06/scene-panel-right-click/spec.md` remains the authoritative source. Wiki updates were made by the wiki-agent as a post-implementation step, consistent with the governance model. ✅

## Blocking issues

None.

## Warnings

Non-blocking concerns for awareness:

- Debug log on right-click selection uses a simplified message (`"set"` / `"unchanged"`) rather than the richer format suggested in the spec's Observability section (which recommended including modifier and entity ID). This was noted by spec-critic, contract-critic, and code-reviewer as a non-blocking concern. The existing debug logging is adequate for operational purposes.
- AC-12 (Context menu "Delete" enabled after right-click) and AC-13 (Context menu "Rename" enabled after right-click) depend on F-04's context menu implementation reading `ctx.editor.selection().empty()` correctly. The code change is correct, but manual testing should confirm the context menu respects the new selection state. This dependency is properly documented and is not a blocking concern.

## Required governance updates

None — all governance documents (ADRs, wiki, specs) have been updated or verified as consistent. No new ADRs, spec amendments, or wiki corrections are required.

## Summary

The Scene Panel right-click selection feature passes all governance checks with no blocking issues. All seven cross-document validation criteria are satisfied: spec-to-contract coverage is complete, contract-to-code implementation is exact, code builds and tests pass, F-03 and F-04 spec updates are accurate and consistent, wiki updates reflect the new behavior correctly, all relevant ADRs are respected (ADR-026, ADR-027, ADR-029), coordination.md is complete with no skipped gates, and all previous gate statuses are consistent.
