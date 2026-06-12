# Spec Review — F-04 Scene Panel — Entity Operations

## Summary

The F-04 spec is well-structured, thorough, and covers most edge cases comprehensively. On re-review, **all 2 blocking issues and 4 warnings from the previous review are resolved**. The spec is now accepted without blocking issues.

---

## Definition of Ready Checklist

### Clarity & Completeness

| Criterion | Status | Notes |
|---|---|---|
| Scope is clearly defined | ✅ | Goals, non-goals, and out-of-scope sections are thorough and unambiguous. |
| Dependencies are identified | ✅ | F-02 (entity tree, EditorContext), F-03 (selection, anchor), engine APIs, ImGui APIs are all referenced. |
| Edge cases and error conditions are described | ✅ | 22 edge cases and 10 error cases are documented. Comprehensive. |
| Expected behavior is unambiguous and testable | ❌ | **Blocking**: Delete-while-rename edge case describes behavior that is technically impossible with standard ImGui InputText focus semantics. |

### Verification

| Criterion | Status | Notes |
|---|---|---|
| E2E verification method defined | ✅ | Headless unit tests (CI), manual smoke test, clean build verification — all three methods are described. |
| Acceptance criteria are specific, measurable, and verifiable | ✅ | 34 ACs with verification methods (unit test, manual, or inspect). Mix of automated and manual coverage. |
| Success and failure states are described | ⚠️ | Success criteria exist (SC-001–004), but SC-003 contradicts A-09 (see blocking issue #1). Error cases section is thorough. |

### Documentation

| Criterion | Status | Notes |
|---|---|---|
| Interface changes are documented | ✅ | Command/CommandStack signature changes have "before/after" code blocks. New command files listed. Editor/ScenePanel changes documented. |
| Existing docs that must be updated are listed | ✅ | Documentation impact section lists 2 wiki files needing updates (+ 1 confirmed unchanged). |

### Technical

| Criterion | Status | Notes |
|---|---|---|
| Technical constraints are identified | ✅ | ImGui APIs, engine APIs, build changes (new files in `src/editor/commands/`), and Command pattern constraints are documented. |
| Risks or unknowns are surfaced | ⚠️ | A-12 (breaking change to existing Command subclasses) is noted. However, the impact on MenuBar's existing `undo()`/`redo()` calls is NOT surfaced. |
| Performance or resource implications are noted | ✅ | Child count O(1) noted for 1000+ children. flush_destroyed() lifecycle ensures clean iteration. |

---

## Blocking issues

Items that must be resolved before the artifact can proceed.

- [x] **SC-003 contradicts A-09 (data loss claim vs assumption)**: SC-003 states "All three entity operations can be undone and redone without data loss." But A-09 explicitly says DeleteEntityCommand does NOT preserve component state — entities are restored with only default Transform, losing all custom component data. This is data loss. Either SC-003 must be qualified (e.g., add "entity identity, name, and hierarchy are preserved; component state preservation is deferred"), or A-09 must be changed to include component state serialization. Current wording is a direct contradiction.
- [x] **Delete-while-rename edge case is technically infeasible as described**: The edge case on line 413 says: "If rename is active, pressing Delete first confirms rename (via focus loss), then processes deletion." However, ImGui `InputText` with `ImGuiInputTextFlags_EnterReturnsTrue` captures keyboard input when focused — pressing Delete while focused on the InputText deletes a character, it does NOT cause focus loss. The Delete key event never reaches the ScenePanel's Delete handler. This behavior is ambiguous and cannot be implemented as-written. Options to resolve: (a) document that Delete key is consumed by InputText and does not trigger deletion during rename — user must confirm/cancel rename first; (b) specify a different mechanism (e.g., ScenePanel checks for Delete before rendering the InputText if rename is active, which would break text editing). This must be clarified to be testable.

---

## Warnings

All 4 warnings from the previous review have been addressed in the updated spec:

- ~~**Breaking change impact on `MenuBar` not documented**~~ → **RESOLVED**: Added to Key entities section (line 204-205).
- ~~**ADR-029 Decision 7 vs anchor-based parenting**~~ → **RESOLVED**: This was an informational note; anchor model was already approved in grill-me. No further action needed.
- ~~**Delete undo data loss should be more prominent**~~ → **RESOLVED**: Added note in Delete behavior (line 78) and Error cases (line 445).
- ~~**`QuitCommand` breaking change**~~ → **RESOLVED**: AC-04 now explicitly mentions QuitCommand (line 360).
- ~~**`Editor::command_stack()` accessor not yet public**~~ → **RESOLVED**: Tracked in A-01. Fine for implementation phase.
- ~~**AC-04 "Existing Command subclasses"**~~ → **RESOLVED**: AC-04 now explicitly mentions QuitCommand.

---

## Required changes

Concrete, actionable changes requested:

1. **Fix SC-003 vs A-09 contradiction**: Either qualify SC-003 to note that component state preservation is deferred (not data-loss-free for delete undo), or expand A-09 to include component state serialization.
2. **Clarify Delete-while-rename interaction**: Replace the edge case description with a feasible behavior. Recommended: "If rename is active, the Delete key is consumed by the inline InputText widget (deletes a character). To delete the entity, the user must first confirm (Enter) or cancel (Escape) the rename, then press Delete. The ScenePanel's Delete key handler is only active when no inline rename is in progress."
3. **Document MenuBar undo/redo call-site change**: Add to the "Key entities" section that `MenuBar::draw_ui()` must pass `ctx` to `command_stack_.undo(ctx)` and `command_stack_.redo(ctx)`.

---

## Suggested improvements

Optional ideas (not required):

- Add a note in the user-visible Delete section about component data not being preserved on undo (linking to A-09).
- Add an acceptance criterion that verifies the ScenePanel Delete handler is disabled during inline rename.
- Consider adding a cross-reference from the context menu behavior to the selection model (F-03) for readers unfamiliar with `anchor()` semantics.
- Consider adding a note about the `QuitCommand` signature update in the "Key entities" section alongside the other command changes.

---

## Consistency Checks

### ADR-029 (Editor UX Decisions)

- **Decision 7 (Entity Creation as Child of Selected)**: The spec uses `EditorSelection::anchor()` which is more nuanced than ADR-029's "single entity selected → child." The two are mostly compatible for the common case (plain-click selection), but diverge for Toggle-only selections. The grill-me approved the anchor model (Coordination Log, Decision 8). Minor inconsistency — see Warnings above.
- **Decision 8 (Prefab Editing in Tabs)**: No impact — prefab operations are out of scope (NG-04).
- **Decision 9, 10**: Not relevant to entity operations.
- **ADR-029's duplicate names allowance**: Respected — spec does not enforce unique names. ✓
- **Overall**: Largely consistent. The anchor-based parenting is a reasonable refinement of ADR-029's simpler model.

### F-02 / F-03 Specs (Predecessors)

- **EditorContext pattern**: Uses `EditorContext const&` in Command signatures, extending the F-02 plumbing correctly. ✓
- **EditorSelection/snapshot/restore**: Uses `EditorSelection::snapshot()` and `restore()` as designed in F-03. ✓
- **anchor() semantics**: Uses `EditorSelection::anchor()` consistent with F-03's anchor model (set on Replace, unchanged on Toggle). ✓
- **flush_destroyed()**: Lifecycle in `Editor::update()` does not conflict with F-02's panel iteration. ✓
- **Consistent with both predecessor specs.** ✓

### Existing Code

- **`QuitCommand`**: Will need signature update (item tracked in A-12). Impact is manageable.
- **`MenuBar`**: `undo()`/`redo()` call sites at `menu_bar.h:84,87` must be updated to pass `EditorContext const&` — see Warnings.
- **`CommandStack`**: Current interface at `command_stack.h` has no `EditorContext` parameters. Spec changes are clearly described.
- **No other existing code is impacted** beyond the documented changes.

### Feature Breakdown Document

No feature breakdown document was found during search. The spec aligns with the coordination.md decision log and the accepted ADR-029 decisions.

---

## Review history

| Date | Verdict | Summary |
|---|---|---|
| 2026-06-12 | Rejected | Two blocking issues (SC-003/A-09 contradiction, Delete-while-rename infeasible). MenuBar impact not documented. |
| 2026-06-12 | Accepted | Re-review: all 2 blocking issues resolved (SC-003 qualified, delete-while-rename fixed to feasible behavior). All 4 warnings addressed (MenuBar documented, AC-04 mentions QuitCommand, delete undo data loss noted in user-visible behavior and error cases). No new issues introduced. |
