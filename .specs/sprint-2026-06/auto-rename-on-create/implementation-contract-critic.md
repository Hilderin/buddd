# Implementation Contract Review — Auto-Rename on Create

## Blocking issues

No blocking issues found. The contract is precise, covers all spec requirements, and is compatible with existing code.

## Warnings

- **Escape handler code has a minor bug, but the note clarifies intent**: The provided Escape handler code (section 3d) calls `cancel_rename()` before logging, then attempts to log `renaming_entity_.has_value()` which has just been reset. The contract's own note immediately below the code block acknowledges this and clarifies the fix (capture the entity ID before `cancel_rename()`). This is not a blocking issue since the specification is clear, but the implementer must **not** copy-paste the code block blindly — they must apply the fix described in the note.

- **Pointer stability for `pending_create_command_` relies on an unstated guarantee**: The contract stores a raw pointer to a `CreateEntityCommand` after `execute()` moves it into `CommandStack`'s internal vector. This is **safe** because the command is heap-allocated and owned by a `std::unique_ptr` (vector reallocation moves `unique_ptr`s but not the pointed-to objects). However, the contract does not explicitly address this correctness argument. The implementer should be aware of this assumption.

- **Wiki line number inaccuracy**: The documentation impact section states the wiki table is "at line 344", but in the current `docs/wiki/editor/editor-panels.md` this content is at line 335. The Implementation Contract Author will correct this when applying wiki updates; the implementer should verify the correct location.

- **No unit test for the Escape → discard flow**: The deferred undo mechanism (Escape sets `pending_undo_creation_`, end of `draw_ui()` calls `CommandStack::undo()`) is only covered by manual E2E tests. The 8 unit tests cover the `CreateEntityCommand` changes well, but the integration of ScenePanel's Escape handling with undo is not unit-tested. Not blocking — manual coverage is acceptable per the spec.

- **Empty-area click handler has subtle sequencing dependencies**: The empty-area left-click handler (section 3h) relies on `confirm_rename()` having already been called by `IsItemDeactivatedAfterEdit()` before the empty-area handler runs. The contract acknowledges this subtlty, but the logic is fragile and the implementer must be careful to avoid double-confirmation.

## Required changes

None.

## Suggested improvements

- Consider adding a brief note in the contract about why `pending_create_command_` is safe: the raw pointer remains valid because `CommandStack` owns the command via `std::unique_ptr` in `std::vector`, and moving `unique_ptr`s does not change the command object's address, even on vector reallocation.

- Consider adding a unit test for the deferred undo path (Escape → `pending_undo_creation_` → `CommandStack::undo()`) to verify the stack state after discard.
