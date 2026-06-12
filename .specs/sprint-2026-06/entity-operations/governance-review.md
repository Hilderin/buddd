# Governance Review — F-04 Scene Panel — Entity Operations

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] All 34 acceptance criteria (AC-01 through AC-34) are addressed in the implementation contract and verified by the code review. No gaps.
- [x] All 16 done criteria (DC-01 through DC-16) are implemented, verified by the code review, and confirmed working (0 warnings, 574 tests pass).
- [x] The DeleteEntityCommand destroy strategy differs from the contract text (contract specified reverse-order child-before-parent; implementation uses recursive `Entity::destroy()` on top-level entities only). The implementation is correct, more efficient, and avoids a dangling pointer issue in `World::flush_destroyed()`. Code comments document this. No functional gap.
- [x] The `stored_parent_id_` dead-code warning (code-review W-06) was based on a pre-fix version; the current code reads it at lines 36–38 of `create_entity_command.h`. No longer an issue.
- [x] Spec-critic blocking issues (SC-003/A-09 contradiction, Delete-while-rename infeasibility) resolved in spec v2 and correctly reflected in contract and code.
- [x] Contract-critic blocking issues (B-01 through B-06) resolved in contract v2 and correctly implemented.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-027 (Editor Architecture)**: Implementation follows the established architecture — `Editor` uses direct member variables (`command_stack_`), Commands receive context via `EditorContext`, no SDL3/OpenGL/GLM in editor code. The `App::update()` extension (SPEC-028 amendment) is used for `flush_destroyed()` lifecycle. Fully aligned.
- [x] **ADR-029 (Editor UX Decisions)**: Decision 7 (entity creation as child of selected) is implemented via `EditorSelection::anchor()` — a compatible refinement approved in the grill-me process. Decision 10 (duplicate names allowed) respected. No prefab operations (per Decision 8). Fully aligned.
- [x] **ADR-011 (Ownership/Nullability/NoDiscard)**: `Editor::command_stack()` accessor has `[[nodiscard]]`. No raw pointers in new public API. Fully aligned.
- [x] **ADR-026 (Dear ImGui Integration)**: Standard ImGui APIs (TreeNodeEx, InputText, BeginPopupContextItem, BeginPopupModal, IsWindowFocused, IsKeyPressed, SetKeyboardFocusHere) used correctly. Fully aligned.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/editor/editor-panels.md`**: Updated with F-04 entity operations (Create Empty, Delete, Rename), context menu behavior, keyboard shortcuts (Delete, F2), Command signature change, `flush_destroyed()` lifecycle, and A-09 component-state-deferred assumption. Status banner includes F-04. Last reviewed updated. Consistent with implementation.
- [x] **`docs/wiki/editor/cross-panel-communication.md`**: Updated with F-04 entity operation path in MVP1 Selection Paths table, Command use of `EditorContext`, selection snapshot/restore pattern, and `flush_destroyed()` lifecycle. Consistent with implementation.
- [x] **`docs/wiki/editor/entity-selection.md`**: Updated with F-04 snapshot/restore pattern used by all three Commands, Selection Lifecycle entry for entity-destroyed selection. Consistent with implementation.

## Warnings

Non-blocking concerns for awareness:

- **Done Criteria not checked in contract**: DC-01 through DC-16 in `implementation-contract.md` still show `[ ]` (unchecked). While the code-review verified all are implemented, the contract was never updated to mark them `[x]`. Cosmetic — no functional impact.
- **DeleteEntityCommand destroy strategy vs contract**: The contract specified reverse-order destruction (children before parents), but the implementation destroys top-level entities only and relies on recursive `Entity::destroy()`. This is correct and more efficient, but the contract text was not updated to match.
- **Confirmation dialog control flow**: Uses a `deleted` bool + early return + `CloseCurrentPopup()` pattern — functional but fragile, as noted in contract-critic W-03 and code-review W-02.
- **Context menu during inline rename**: Right-clicking during inline rename shows the context menu (may trigger focus loss → rename confirmation). Not prohibited by spec but potentially surprising. Noted in code-review W-04.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None required. All three wiki files (`editor-panels.md`, `cross-panel-communication.md`, `entity-selection.md`) have been updated by the wiki-agent and are consistent with the implementation. No ADR amendments are needed.
