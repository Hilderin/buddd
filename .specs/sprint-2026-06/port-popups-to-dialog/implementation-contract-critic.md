# Implementation Contract Review — Port Remaining Popups to Dialog Abstraction

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] ~~Missing `on_close` callback on save-prompt CustomDialog — Escape does not clear `pending_op_` (violates AC-011).~~  
  **RESOLVED in Loop 1 fix.** The contract now includes the `on_close` callback (lines 397–404) that clears `pending_op_ = PendingOp::None` and logs the cancellation on Escape/click-outside dismissal. The callback is documented with an inline comment explaining why it is needed.

## Warnings

Non-blocking concerns for awareness:

- The one-frame-delay pattern for delete confirmation is correctly justified. The `EditorContext` is a stack-local in `draw_ui()` containing an `EngineContext const&` that is only valid per-frame. The button callback fires on an unpredictable later frame, so capturing `EditorContext` by value (which copies the `EngineContext const&`) would produce a dangling reference. The one-frame-delay (storing IDs in `pending_confirm_delete_`, executing on the next frame's `ScenePanel::draw_ui(ctx)` with a fresh `ctx`) is the correct approach given the constraint of not modifying `editor_dialog.h` or `CommandStack`.

- The save-prompt button callbacks correctly capture `this` (Editor*) and `pending_op_` by value. No `EngineContext` reference is captured — all engine access is via `engine_->platform()`, which is safe since `engine_` is set during `setup()` and never changes. This matches existing patterns.

- The delete confirmation Escape/cancel path is safe without `on_close`: if the user presses Escape, the dialog closes, `pending_confirm_delete_` is never set, and no deletion occurs. The dialog does not reopen because `draw_pending_op_modal()` does not manage delete confirmation.

## Required changes

Concrete, actionable changes requested:

~~1. **Save-prompt: add `on_close` callback** — Pass a 5th argument to the `CustomDialog` constructor in `draw_pending_op_modal()` that clears `pending_op_` on Escape/click-outside.~~ **RESOLVED.** Contract now includes the `on_close` callback.

~~2. **Test: add a test case for save-prompt Escape → Cancel** — T9 ("Save-prompt Cancel clears `pending_op_`") tests the Cancel button but not Escape. Add a test or document that the `on_close` callback is fired on Escape and clears `pending_op_`.~~ **RESOLVED.** T9 now explicitly tests both Cancel button (via button callback) and Escape (via `on_close` callback / `handle_escape()`), verifying `pending_op_` side-effects for both paths.

## Suggested improvements

Optional ideas (not required):

~~- Consider adding an `on_close` comment in the save-prompt code block to document why it's needed for Escape handling, making the intent explicit for future readers.~~ **RESOLVED.** A multi-line comment (lines 397–399) now explains that `on_close` fires on Escape/click-outside (not on button clicks) and is needed to prevent dialog re-open.

- The delete confirmation `message` string-building uses a recursive lambda `find_name` to locate the first entity's name. This could be simplified by pre-collecting entity names during the `with_children` loop above, but the current approach is functionally correct and matches the existing pre-port code pattern, so this is entirely optional.
