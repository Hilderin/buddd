# Implementation Contract Review — F-04 Scene Panel — Entity Operations

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01: `DeleteEntityCommand::undo()` clears `entity_ids_`, breaking redo (AC-29, Story 7)**  
  **FIXED** — `entity_ids_.clear()` and `saved_entities_.clear()` removed from `undo()`. Comments on lines 586–588 document the permanent-ID approach. Verified at lines 525–590.

- [x] **B-02: `DeleteEntityCommand::collect_and_save` uses `entity.parent().has_value()` — does not compile**  
  **FIXED** — Line 465 now reads `state.parent_old_id = parent_entity.id();`. The `Entity::parent()` value type returns `EntityId::none()` for root entities naturally.

- [x] **B-03: Single-entity delete confirmation dialog fails to show entity name (spec Story 3, AC-12)**  
  **FIXED** — `pending_deletion_first_name_` declared at line 737, populated via tree traversal at lines 932–952, used in dialog text at lines 858–861.

- [x] **B-04: `CreateEntityCommand` missing `#include <string>`**  
  **FIXED** — `<string>` added at line 272.

- [x] **B-05: `CreateEntityCommand::execute()` lacks fallback when parent entity not found on redo**  
  **FIXED** — Root-create fallback added at lines 340–346 with `BUDDD_LOG_DEBUG` warning.

- [x] **B-06: `DeleteEntityCommand::execute()` on redo cannot find recreated entities — stale EntityIds (AC-29)**  
  After `undo()` recreates destroyed entities via `world.add_entity()` / `create_child()`, the recreated entities receive **new** `EntityId` values (EntityId uses generational indices — `index` + `generation` in `entity_id.h`). However, `entity_ids_` still contains the **original old IDs**. On `redo()` → `execute()`:
  1. `saved_entities_.clear()` (line 458) discards the undo state.
  2. The traversal loop (lines 474–495) searches for entities matching old IDs in `entity_ids_` — no matches found because entities now have new IDs.
  3. `saved_entities_` remains empty; the destroy loop (lines 498–517) does nothing.
  4. Selection is cleared. The command silently becomes a no-op.
  
  The local `recreated` vector in `undo()` (lines 533–537) correctly tracks the old→new ID mapping, but it is **discarded** when `undo()` returns.  
  **Fix**: At the end of `DeleteEntityCommand::undo()`, update `entity_ids_` with the new IDs by iterating `recreated`. For each ID in `entity_ids_`, if it appears as an `old_id` in `recreated`, replace it with the corresponding `new_id`. This ensures `execute()` on redo finds the current entities and re-deletes them.

## Warnings

Non-blocking concerns for awareness:

- **W-01: `Editor::selection()` missing const overload** — The entity-selection wiki documents `auto selection() const -> EditorSelection const&;` but `editor.h` only has the non-const version. The contract does not add it. Non-blocking for F-04 (commands need mutable access), but an inconsistency with documented API.

- **W-02: Test infrastructure is heavyweight** — The `CmdTestHelper` creates a full `EngineService`+`EngineContext` for every command test, following the `HeadlessTestContext` pattern. This is slow and may trigger display-dependent code paths. Consider whether commands can be tested with a simpler `EditorContext` constructed from a minimal `EngineContext` stub.

- **W-03: Confirmation dialog control flow is fragile** — The `draw_delete_confirmation_modal()` uses a `deleted` bool flag combined with `ImGui::CloseCurrentPopup()` and an early `return` inside an `if (ImGui::Button("Cancel") || deleted)` block. Modal state management would be simpler if decision (button press) and execution (command dispatch) were separated.

- **W-04: Documentation impact not tracked in Done Criteria** — The spec lists `docs/wiki/editor/editor-panels.md` and `docs/wiki/editor/cross-panel-communication.md` as requiring updates. The contract mentions these in a "Documentation impact" section but does NOT include them in the Done Criteria checklist (DC-01 through DC-16). Wiki updates are spec requirements that should be tracked as explicit deliverables.

- ~~**W-05: `DeleteEntityCommand::undo()` calls `entity_ids_.clear()` and `saved_entities_.clear()` after restore**~~ **RESOLVED** — These calls were removed from `undo()` per B-01 fix. The broader redo state management concern is now covered by B-06 (stale EntityIds).

- **W-06: Inline rename `TreeNodeEx` interaction with ImGui tree state is fragile** — The rename rendering (lines 801–841) creates a `TreeNodeEx` with `ImGuiTreeNodeFlags_NoTreePushOnOpen` and manually manages `TreePop()`/child rendering. This differs from the normal tree rendering path and may cause ImGui state corruption if the entity expand/collapse state changes during rename. A safer approach: render the `InputText` overlay on top of the existing `TreeNodeEx` label without replacing the tree node structure.

## Required changes

### Previously resolved (all 6 fixes confirmed in this re-review)

1. ~~**Fix `DeleteEntityCommand::undo()`**: Remove `entity_ids_.clear();` and `saved_entities_.clear();` so redo works (B-01).~~ ✅ Done.
2. ~~**Fix `parent_entity.has_value()` calls in DeleteEntityCommand**: Replace with `parent_entity.id()` check (B-02).~~ ✅ Done.
3. ~~**Add `pending_deletion_first_name_` member and populate it**: Add `std::string` member, fetch entity name in `execute_delete_entity()` (B-03).~~ ✅ Done.
4. ~~**Add `#include <string>` to `create_entity_command.h`** (B-04).~~ ✅ Done.
5. ~~**Add root-create fallback to `CreateEntityCommand::execute()`** after traversal loop (B-05).~~ ✅ Done.
6. ~~**Fix `DeleteEntityCommand::undo()` stale EntityIds**: After recreating entities in `undo()`, update `entity_ids_` with the new IDs from the `recreated` mapping. This ensures `execute()` on redo finds the current entities (B-06).~~ ✅ Done.

## Suggested improvements

Optional ideas (not required):

1. **Add `selection() const` overload to `Editor`** for const-correct consumers — already documented in wiki.
2. **Extract `find_entity(World&, EntityId)` helper** to eliminate repeated tree traversal code across all three commands.
3. **Add a unit test for the confirmation dialog state machine** — verify `show_delete_confirmation_` is set when deleting parent entity, cleared on cancel/confirm.
4. **Add DC-17 and DC-18** for the two wiki documentation updates listed in the Documentation impact section.

## Review summary

### Final re-review (2026-06-12) — B-06 fix verified

**All 6 blocking issues (B-01 through B-06) are confirmed resolved.**

The B-06 fix is correct: `DeleteEntityCommand::undo()` now updates `entity_ids_` with new EntityIds from the `recreated` mapping after recreating entities (lines 586–593). This ensures that `redo()` → `execute()` can find the recreated entities by their current IDs.

The fix correctly:
- Iterates each ID in `entity_ids_` and replaces stale old IDs with the corresponding `r.new_id` from the `recreated` vector.
- Preserves the generational EntityId semantics — recreated entities get new IDs, and `entity_ids_` is updated accordingly.
- Allows redo's `execute()` to find the recreated entities and re-destroy them, satisfying AC-29.

No new blocking issues are introduced by this fix.

**Verdict: Accepted** — All blocking issues resolved. Contract is ready for implementation.
