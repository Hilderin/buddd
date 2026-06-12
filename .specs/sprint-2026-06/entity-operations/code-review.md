# Code Review — F-04 Scene Panel — Entity Operations

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01: 13 `[[nodiscard]]` warnings in `tests/editor/entity_operations_tests.cpp` (AC-34 violation)**

  The test file calls `ctx.command_stack().undo(ctx.editor_ctx)` and `.redo(...)` without capturing or suppressing the `[[nodiscard]]` return value at lines 125, 144, 149, 167, 238, 273, 292, 296, 335, 349, 352, 368, and 410. Each triggers:

  ```
  warning: ignoring return value of 'bool buddd::editor::CommandStack::undo(...)',
  declared with attribute 'nodiscard' [-Wunused-result]
  ```

  The existing convention in `editor_tests.cpp` uses `[[maybe_unused]] auto _ = ...` to suppress this. The new test file must follow the same pattern.

  **Fix**: Wrap each offending call with `[[maybe_unused]] auto _ = command_stack().undo(...)` or `static_cast<void>(command_stack().undo(...))`.

  This directly violates AC-34: "Zero new warnings from `src/editor/` and `tests/`."

## Warnings

Non-blocking concerns for awareness:

- **W-01: DeleteEntityCommand destroy strategy differs from contract text**

  The contract specified destroying entities in reverse order (children before parents). The implementation destroys only top-level entities (the `entity_ids_` list), relying on `Entity::destroy()` → `World::destroy_entity()` → `World::mark_for_destroy()` which recursively marks all descendants. This is verified correct against engine code (`world.cpp:281-297`). The implementation is actually more efficient and avoids double-marking. However, this silent deviation from the contract should be documented.

- **W-02: Confirmation dialog control flow is fragile** (re-iterated from contract-critic W-03)

  The `draw_delete_confirmation_modal()` uses a `deleted` bool + `||` operator combined with `CloseCurrentPopup()` and an early `return`. Modal state management would be simpler and less error-prone if decision (button press) and execution (command dispatch) were separated.

- **W-03: Inline rename `TreeNodeEx` interaction is fragile** (re-iterated from contract-critic W-06)

  The rename rendering uses `ImGuiTreeNodeFlags_NoTreePushOnOpen` on a placeholder `TreeNodeEx`, manually managing `TreePop()`/child rendering. This differs from the normal tree rendering path and may cause ImGui state corruption if expand/collapse state changes during rename.

- **W-04: Context menu appears during inline rename**

  The context menu (`BeginPopupContextItem`) is rendered outside the `is_renaming` if/else block, so right-clicking during inline rename still shows the entity context menu. While the spec doesn't explicitly prohibit this, it may be surprising — the rename `InputText` is focused and right-clicking could cause focus loss (triggering `confirm_rename` via `IsItemDeactivatedAfterEdit`), then showing the context menu on the placeholder node.

- **W-05: Documentation impact not tracked as deliverables** (re-iterated from contract-critic W-04)

  The spec lists `docs/wiki/editor/editor-panels.md` and `docs/wiki/editor/cross-panel-communication.md` as requiring updates. These are not yet done and are deferred to the wiki-agent.

## Required changes

1. Fix all 13 `[[nodiscard]]` warnings in `tests/editor/entity_operations_tests.cpp` by wrapping `undo()` and `redo()` calls with `[[maybe_unused]]` suppression.

## Suggested improvements

1. The `DeleteEntityCommand` could be slightly simplified: the `saved_entities_` collection loop iterates all descendants, but only top-level entities are destroyed. The collection could stop at top-level entities since `destroy()` is recursive. However, the current approach is correct for undo (saving full subtree state) — keep as-is.

2. Add a `find_entity(World&, EntityId)` helper to eliminate repeated tree traversal code across all three commands and the ScenePanel helpers.

## Review summary

**Implementation quality**: The code is well-structured, follows existing conventions (`#pragma once`, namespaces, `[[nodiscard]]` on query methods), and implements the Command pattern correctly. All three commands correctly snapshot and restore selection. The `CreateEntityCommand` correctly handles anchor-based parenting with root fallback. The `DeleteEntityCommand` correctly handles parent hierarchy restoration on undo with the B-06 fix (EntityId remapping for redo). The `RenameEntityCommand` correctly validates empty/same-name input.

**Spec/Contract alignment**: All 34 acceptance criteria are addressed by the implementation. All 16 done criteria are covered by unit tests. The implementation compiles with the signature changes to `Command`/`CommandStack`/`QuitCommand`/`MenuBar`. The `flush_destroyed()` lifecycle is correctly placed in `Editor::update()`. No forbidden files were modified.

**Test coverage**: 16 test cases in `entity_operations_tests.cpp` cover all three commands, undo/redo cycles, selection snapshot/restore, and edge cases. All 574 existing tests pass (22179 assertions). The `editor_tests.cpp` `CommandStack` tests are updated for the new signature.

**Build warnings**: **Blocking** — 13 `[[nodiscard]]` warnings from `tests/editor/entity_operations_tests.cpp` violate AC-34. Zero warnings from `src/editor/`.

**Verdict**: Rejected — the `[[nodiscard]]` warnings must be fixed before acceptance.

---

# Re-review 2 — Re-verification of human-reported bug fixes

## Summary of fixes verified

1. **Create Empty via right-click now uses right-clicked entity** ✅ — `scene_panel.h:129` passes `entity.id()` to `execute_create_entity(ctx, entity.id())`. `CreateEntityCommand` constructor accepts `std::optional<EntityId> explicit_parent`. Verified.

2. **Undo→Redo of child creation** ✅ — `explicit_parent_` is a constructor parameter stored as member (line 130), so it survives undo/redo cycles. `stored_parent_id_` is also written for safety, though it is never read (see W-06).

3. **Rename in context menu** ✅ — The rename `MenuItem` at line 138 has no disabled condition and uses `entity.id()` (right-clicked entity) instead of selection.

4. **F2 crash (PopID mismatch)** ✅ — `ImGuiTreeNodeFlags_NoTreePushOnOpen` removed from the rename placeholder `TreeNodeEx` at line 81. `TreePop()` at line 86 is now properly balanced with `TreeNodeEx()`.

5. **`[[nodiscard]]` warnings** ✅ — All 13 previously reported `undo()`/`redo()` calls in `entity_operations_tests.cpp` are now wrapped with `static_cast<void>()`. No remaining warnings.

## Additional validation

- **Build**: `cmake --build --preset debug` produces **zero warnings** from `src/` or `tests/`. ✅
- **Tests**: All 574 tests pass (22179 assertions). ✅
- **Forbidden files**: No changes to `src/engine/`, `editor_context.h`, `editor_selection.h`, `CMakeLists.txt`, or other forbidden files. ✅

## New issues found

- **W-06: `stored_parent_id_` is set but never read in `CreateEntityCommand`**

  Line 38 sets `stored_parent_id_ = parent_id;` with the comment "Stored parent for redo (so redo doesn't depend on selection state)". However, `stored_parent_id_` is never read anywhere in the class. The `execute()` method uses `explicit_parent_` (which persists across undo/redo as a member) or re-derives the parent from `selection.anchor()`. This is dead code — the member and its write can be removed in a cleanup pass. No functional impact because the actual behavior is correct (explicit_parent_ persists, and undo restores the selection including anchor).

## Review summary

**All 5 human-reported bugs are fixed correctly.** Zero build warnings, all 574 tests pass. The `stored_parent_id_` dead variable is a non-blocking code quality issue (W-06). All acceptance criteria are met. AC-34 (zero warnings) is now satisfied.

**Verdict**: **Accepted** — All blocking issues resolved. The implementation is ready.
