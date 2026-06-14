# SPEC-2026-008 — Port Remaining Popups to Dialog Abstraction

## Problem

The Editor's reusable Dialog abstraction (`Dialog` / `CustomDialog` / `DialogButton` in `src/editor/editor_dialog.h`) was introduced in SPEC-2026-007 and the About popup was ported to it. However, three popups remain using raw ImGui popup code and ad-hoc state members scattered across the codebase:

1. **Error modals** in `Editor` — use `error_modal_title_`, `error_modal_message_`, `show_error_modal_` booleans with 15+ call sites.
2. **Delete confirmation** in `ScenePanel` — uses `show_delete_confirmation_`, `pending_deletion_ids_`, `pending_deletion_with_children_`, `pending_deletion_first_name_` with manual OpenPopup/BeginPopupModal.
3. **Save-prompt modal** in `Editor` — uses `SavePromptResult` enum, `save_prompt_requested_`, `save_prompt_seen_`, `pending_file_path_` (dead code), multi-frame state machine, `draw_save_prompt_modal()` returning an optional result that `draw_pending_op_modal()` polls.

These ad-hoc implementations violate DRY, make it harder to add new dialogs, and introduce inconsistency in modal lifecycle management. Additionally, the current dialog render loop in `Editor::draw_ui()` Phase 4 uses `dialog->title()` as the ImGui popup ID, which causes popup ID collisions when two dialogs have the same title but different IDs.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Consistent dialog rendering**: All modals (error, delete confirmation, save-prompt) use the Dialog abstraction and are rendered in Phase 4, alongside the existing About dialog. |
| G-02 | **Error modals with unique IDs**: Each error instance gets its own dialog with a random unique ID, preventing silent deduplication. Multiple errors can stack. |
| G-03 | **Delete confirmation via CustomDialog**: The "Confirm Delete" dialog in ScenePanel uses `CustomDialog` with a content function, triggered via `ctx.editor.open_dialog()`. |
| G-04 | **Callback-driven save-prompt**: The save-prompt modal uses `CustomDialog` with button callbacks that directly execute actions, eliminating the multi-frame polling state machine (`SavePromptResult`, `save_prompt_requested_`, `save_prompt_seen_`, `draw_save_prompt_modal()`). |
| G-05 | **No ImGui popup ID collisions**: The `"title###id"` pattern is applied in the dialog render loop to prevent collisions between dialogs with the same title but different IDs. |
| G-06 | **Remove dead code**: `pending_file_path_`, `SavePromptResult` enum, old state members, and unused function declarations are removed. |
| G-07 | **Test coverage**: Tests verify the ported dialogs work correctly via the Dialog abstraction. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No changes to the Dialog abstraction itself** (`editor_dialog.h`) — no new base class features, no API changes. |
| NG-02 | **No changes to engine APIs or build system** — no new CMake targets, no SDL3/OpenGL/GLM changes. |
| NG-03 | **No changes to menu/panel architecture** — dialogs remain in the existing `dialogs_` vector. |
| NG-04 | **No changes to the PendingOp enum** — `PendingOp` remains for tracking the pending scene operation across frames. The save-prompt is converted from a polled-result pattern to a callback pattern, but `pending_op_` still exists. |
| NG-05 | **No new dialog types** — only the three existing popup types are ported. No toasts, overlays, or non-blocking notifications. |
| NG-06 | **No focus-stealing / auto-focus behavior changes** — existing modal behavior preserved. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Triggers error modals (failed load/save), delete confirmations (deleting entities with children), and save-prompts (dirty scene operations). Interacts with dialog buttons and Escape to dismiss. |
| **Editor developer** | Uses `Editor::open_dialog()` to show dialogs. Creates `CustomDialog` instances for error modals, delete confirmation, and save-prompt without adding new state members. |
| **ScenePanel developer** | Triggers delete confirmation via `ctx.editor.open_dialog()` instead of managing state booleans. |

## User-visible behavior

### Visual behavior (unchanged)

All three ported popups behave identically from the user's perspective:

| Popup | Before | After |
|---|---|---|
| **Error modal** | Title + message + OK button. Single modal shown at a time (second overwrites first). | Title + message + OK button. Each error gets its own dialog. Multiple errors stack. |
| **Delete confirmation** | "Delete X and its Y children?" / "Delete Z entities?..." + Delete/Cancel buttons. | Same text and buttons. Behavior unchanged. |
| **Save-prompt** | "Save changes to [name]?" + Save/Don't Save/Cancel buttons. Same Save-as-untitled chaining. | Same text and buttons. Behavior unchanged — Save on untitled still opens Save As dialog. |

### Internal changes (visible to developers)

| Aspect | Before | After |
|---|---|---|
| **Error modals** | Fixed title used as ImGui popup ID. Single boolean toggle. | Random unique ID per instance (`time() + random()`). No dedup — every error is a separate dialog. |
| **Delete confirmation** | State members in ScenePanel (`show_delete_confirmation_`, `pending_deletion_ids_`, etc). | State captured in lambdas passed to CustomDialog. No ad-hoc state members. |
| **Save-prompt** | `SavePromptResult` enum. `draw_save_prompt_modal()` returns `std::optional<SavePromptResult>`. Polled by `draw_pending_op_modal()`. `save_prompt_requested_`/`save_prompt_seen_` flags. | CustomDialog with button callbacks. Button callbacks directly execute actions (save, then continue pending op). No result type, no polling, no request/seen flags. |
| **Dialog ID collisions** | `ImGui::OpenPopup(dialog->title().c_str())` — collisions when same title. | `ImGui::OpenPopup((dialog->title() + "###" + dialog->id()).c_str())` — titles can match, IDs must differ. |

## User stories

### Story 1 — Error modals with unique IDs (Priority: P1)

As an editor user, I want multiple error modals to appear one after another (stacked), so that I don't miss an error if a second error occurs while the first is still visible.

**Given** the editor shows an error modal with title "Load Error" and message "File not found"
**When** a second error with title "Save Error" occurs before the first is dismissed
**Then** both error modals appear stacked (second on top)
**And** dismissing the second (OK or Escape) reveals the first still visible
**And** the user can dismiss each independently

**Given** the editor encounters an error
**When** the popup is shown
**Then** it has an "OK" button that dismisses only that specific dialog
**And** pressing Escape also dismisses only that dialog (or the topmost if stacked)

### Story 2 — Delete confirmation via CustomDialog (Priority: P1)

As an editor user, I want the delete confirmation dialog to look and behave identically to before, using the standard dialog rendering path.

**Given** I right-click an entity with children and select "Delete"
**When** the confirmation dialog appears
**Then** it shows "Delete [name] and its [N] children?" (or "Delete [Z] entities?...")
**And** clicking "Delete" executes the deletion and closes the dialog
**And** clicking "Cancel" or pressing Escape dismisses the dialog without deleting

**Given** the delete confirmation dialog is open
**When** I attempt to trigger another deletion
**Then** the dedup mechanism in `open_dialog()` prevents a second dialog from opening

### Story 3 — Save-prompt via callback-driven CustomDialog (Priority: P1)

As an editor user, I want the save-prompt modal to work identically to before (Save/Don't Save/Cancel with correct continuation), using the standard dialog rendering path.

**Given** the scene is dirty and I trigger New Scene (or Open Scene, or Quit)
**When** the save-prompt appears
**Then** it shows "Save changes to [scene name]?"
**And** three buttons: Save, Don't Save, Cancel

**Given** the save-prompt is showing
**When** I click "Save" and the scene has a file path
**Then** the scene is saved
**And** the pending operation (New/Open/Quit) proceeds after the save completes

**Given** the save-prompt is showing and the scene is untitled
**When** I click "Save"
**Then** a Save As file dialog opens
**And** after the user picks a path and the save succeeds, the pending operation proceeds

**Given** the save-prompt is showing
**When** I click "Don't Save"
**Then** changes are discarded
**And** the pending operation proceeds

**Given** the save-prompt is showing
**When** I click "Cancel" or press Escape
**Then** the pending operation is aborted
**And** the current scene remains open unchanged

### Story 4 — No ImGui ID collisions (Priority: P1)

As an editor developer, I want dialogs with the same title but different IDs to render correctly, so that the `"title###id"` pattern prevents ImGui popup ID collisions.

**Given** two error dialogs with the same title "Load Error" but different IDs are open
**When** Phase 4 of `draw_ui()` runs
**Then** both dialogs use distinct ImGui popup IDs (`"Load Error###<id1>"` and `"Load Error###<id2>"`)
**And** both modals render without ImGui ID assertion failures

### Story 5 — Simplified save-prompt state machine (Priority: P2)

As an editor developer, I want the save-prompt to use the Dialog abstraction instead of a multi-frame polling state machine, so that I don't need to manage `save_prompt_requested_`, `save_prompt_seen_`, and `SavePromptResult`.

**Given** the Editor class before the port
**When** the port is complete
**Then** `SavePromptResult` enum is removed
**And** `save_prompt_requested_` and `save_prompt_seen_` fields are removed
**And** `draw_save_prompt_modal()` declaration and implementation are removed
**And** `pending_file_path_` (dead code) is removed
**And** `draw_pending_op_modal()` is simplified to use `open_dialog()` with a CustomDialog

### Story 6 — Dead state removed (Priority: P2)

As an editor developer, I want all ad-hoc popup state members removed from `Editor` and `ScenePanel`, so that the codebase is cleaner and there is one less way to create popups.

**Given** `editor.h` before cleanup
**When** the port is complete
**Then** `error_modal_title_`, `error_modal_message_`, `show_error_modal_` are removed
**And** `show_error_modal()` and `draw_error_modals()` declarations are removed
**And** `save_prompt_requested_`, `save_prompt_seen_`, `pending_file_path_` are removed
**And** `draw_save_prompt_modal()`, `SavePromptResult` are removed

**Given** `scene_panel.h` before cleanup
**When** the port is complete
**Then** `show_delete_confirmation_`, `pending_deletion_ids_`, `pending_deletion_with_children_`, `pending_deletion_first_name_` are removed
**And** `draw_delete_confirmation_modal()` declaration is removed

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | Error modals use `CustomDialog` with a randomly generated unique ID (`time() + random()`). No fixed ID. | Inspect `Editor::show_error_modal()` (or equivalent code path) — verify the CustomDialog is constructed with a unique ID e.g., via a helper function that generates a unique string. |
| AC-002 | Error modal renders with the given title in the title bar and message as text body, with a single "OK" button that dismisses it. | Unit test: open an error dialog via the error-triggering mechanism, verify the dialog appears with correct title and text. Manual: trigger a load error, verify title and message match. |
| AC-003 | Multiple error modals can stack. Dismissing one (OK/Escape) does not affect other error dialogs. | Unit test: trigger two error dialogs with different IDs, verify both are in `dialogs_`. Call `request_close()` on one, verify the other remains. |
| AC-004 | Delete confirmation uses `CustomDialog` triggered via `ctx.editor.open_dialog()`. No ad-hoc state members in `ScenePanel`. | Verify `show_delete_confirmation_`, `pending_deletion_ids_`, `pending_deletion_with_children_`, `pending_deletion_first_name_` are removed from `scene_panel.h`. Verify `open_dialog(CustomDialog(...))` is called in `execute_delete_entity()` when entities have children. |
| AC-005 | Delete confirmation shows correct dynamic text: "Delete X and its Y children?" for single entity with children, or "Delete Z entities? (Y have children...)" for multiple entities. | Unit test: mock the content function or inspect the stored dialog to verify text is correct for both single and multi-entity scenarios. Manual: attempt deletion with one entity (with children) and with multiple entities. |
| AC-006 | Delete button on confirmation dialog executes `DeleteEntityCommand`. Cancel button and Escape dismiss the dialog without deletion. | Unit test: open delete confirmation, simulate button clicks. Verify `DeleteEntityCommand` is executed on Delete. Verify `command_stack` is unchanged on Cancel/Escape. |
| AC-007 | Save-prompt uses `CustomDialog` with button callbacks. `SavePromptResult` enum is removed. `draw_save_prompt_modal()` is removed. | Inspect `editor.h` — verify `SavePromptResult` enum is removed. Verify `draw_save_prompt_modal()` declaration is removed. Verify `save_prompt_requested_`, `save_prompt_seen_` are removed. |
| AC-008 | Save-prompt callback: Save button saves the scene (if untitled, redirects to Save As file dialog) then continues the pending operation. | Manual: trigger New Scene on dirty untitled scene → click Save → verify Save As dialog opens. After save, verify New Scene is executed. Manual: trigger Quit on dirty scene with file path → click Save → verify scene is saved then editor exits. |
| AC-009 | Save-prompt callback: Don't Save button discards changes and continues the pending operation without saving. | Manual: trigger Open Scene on dirty scene → click Don't Save → verify file open dialog appears immediately. |
| AC-010 | Save-prompt callback: Cancel button aborts the pending operation. Scene remains unchanged. | Manual: trigger New Scene on dirty scene → click Cancel → verify no new scene created, dirty scene remains. |
| AC-011 | Save-prompt: pressing Escape behaves like Cancel (aborts the pending operation). | Manual: trigger Quit on dirty scene → press Escape → verify editor does not quit. |
| AC-012 | Save-prompt: pending_file_path_ (dead code) is removed from Editor. | Inspect `editor.h` — verify no `pending_file_path_` member exists. |
| AC-013 | Error modal state members (`error_modal_title_`, `error_modal_message_`, `show_error_modal_`) are removed from Editor. | Inspect `editor.h` — verify these members are removed. |
| AC-014 | `show_error_modal()` and `draw_error_modals()` declarations and implementations are removed from `editor.h` and `editor.cpp`. | Inspect both files — verify no declarations or definitions exist. |
| AC-015 | `draw_delete_confirmation_modal()` declaration and implementation are removed from `scene_panel.h` and `scene_panel.cpp`. | Inspect both files — verify no declarations or definitions exist. |
| AC-016 | The `"title###id"` pattern is applied to `ImGui::OpenPopup` and `ImGui::BeginPopupModal` calls in Phase 4 of `Editor::draw_ui()`. | Inspect `editor.cpp` Phase 4 — verify `dialog->title() + "###" + dialog->id()` is used for both `OpenPopup` and `BeginPopupModal`. |
| AC-017 | The dialog render loop no longer calls `opened_dialog_ids_.erase()` (the `"title###id"` pattern makes per-dialog OpenPopup tracking via `opened_dialog_ids_` unnecessary). | Inspect `editor.cpp` Phase 4 — verify `opened_dialog_ids_` is no longer used in the render loop. The `opened_dialog_ids_` member is retained for API backward-compatibility with SPEC-2026-007 but is not referenced in the rendering code. |
| AC-018 | `draw_pending_op_modal()` is simplified: it opens a CustomDialog via `open_dialog()` when `dirty_` is true and a `pending_op_` is active, instead of polling `draw_save_prompt_modal()`. | Inspect `editor.cpp` — verify `draw_pending_op_modal()` no longer calls `draw_save_prompt_modal()`. |
| AC-019 | All existing tests still pass. | Run `buddd_tests` — all previously passing tests continue to pass. |
| AC-020 | Zero new warnings from `src/editor/` and `tests/`. | Build with `cmake --build --preset debug` — verify zero new warnings. |
| AC-021 | Dialogs are headless-safe: the error modal's unique-ID generator does not call ImGui functions. | Unit test: create Editor without setup (headless), trigger an error path that calls `show_error_modal()` (or equivalent), verify no crash. |
| AC-022 | Behavioral equivalence: all three ported popups render identically to before the port (same text, same buttons, same behavior for Save/Don't Save/Cancel/Delete/Cancel/OK/Escape). | Run manual smoke test per E2E verification table — verify each ported popup's appearance and behavior matches the pre-port implementation. |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Run `buddd_tests`. Verify `[editor][dialog]` tagged tests pass — including new tests for error modals, delete confirmation via open_dialog, and save-prompt CustomDialog. |
| **Manual smoke test (display)** | Run `buddd edit`. 1) Trigger a load error (open corrupt .yaml file) — verify error modal appears with title "Load Error" and OK button. Dismiss with OK. 2) Trigger two errors in sequence — verify both stack, dismiss independently. 3) Select entity with children, press Delete — verify confirmation modal appears with correct text. Click Delete — entity is deleted. 4) Make scene dirty, click File > New — verify save-prompt appears. Test Save (untitled→Save As), Don't Save, Cancel. 5) Test Quit with dirty scene → save-prompt → Cancel → editor stays open. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero new warnings from `src/editor/` and `tests/`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | All three popups (error modals, delete confirmation, save-prompt) use the Dialog abstraction rendered in Phase 4. | No ad-hoc `ImGui::OpenPopup`/`BeginPopupModal` calls remain for these popups. Inspect `editor.cpp` and `scene_panel.cpp` — verify no raw popup calls for these three types. |
| SC-002 | The save-prompt no longer uses `SavePromptResult` or polling state machine. | `SavePromptResult` enum removed. `draw_save_prompt_modal()` removed. `save_prompt_requested_` and `save_prompt_seen_` removed. |
| SC-003 | All ad-hoc state members for the three popups are removed from `Editor` and `ScenePanel`. | Inspect headers: no `error_modal_title_`, `error_modal_message_`, `show_error_modal_`, `save_prompt_requested_`, `save_prompt_seen_`, `pending_file_path_`, `show_delete_confirmation_`, `pending_deletion_ids_`, `pending_deletion_with_children_`, `pending_deletion_first_name_`. |
| SC-004 | The `"title###id"` pattern is applied to the dialog render loop, preventing ImGui ID collisions. | Inspect Phase 4 of `draw_ui()` — verify `title() + "###" + id()` is used. |
| SC-005 | All existing scene-management and dialog tests pass. | Run `buddd_tests` — zero regressions. |

## Key entities

### Modified: `Editor` class (`src/editor/editor.h`, `src/editor/editor.cpp`)

| Change | From | To |
|---|---|---|
| Error modal state | `error_modal_title_`, `error_modal_message_`, `show_error_modal_` | Removed. Errors use `CustomDialog` with random unique ID. |
| Error modal methods | `show_error_modal()` — sets state booleans; `draw_error_modals()` — renders raw ImGui popup. | `show_error_modal()` becomes a private helper that calls `open_dialog(CustomDialog(...))` with a random unique ID. `draw_error_modals()` removed entirely (rendered by Phase 4 dialog loop). |
| Save-prompt state | `SavePromptResult` enum, `save_prompt_requested_`, `save_prompt_seen_` | Removed. |
| Save-prompt methods | `draw_save_prompt_modal()` returns `std::optional<SavePromptResult>`. | Removed. |
| Pending op dead state | `pending_file_path_` (declared but never assigned, only read in `execute_pending_op()` for OpenScene). | Removed. `execute_pending_op()` for OpenScene becomes a no-op or opens file dialog directly. |
| `draw_pending_op_modal()` | Polls `draw_save_prompt_modal()`, checks SavePromptResult, manages request/seen flags. | Simplified: when `dirty_ == true` and no save-prompt dialog is open yet, opens a `CustomDialog` via `open_dialog()`. Button callbacks capture `pending_op_` by value and execute actions directly. |
| Dialog render loop (Phase 4) | `ImGui::OpenPopup(dialog->title().c_str())` / `BeginPopupModal(dialog->title().c_str(), ...)`. | `ImGui::OpenPopup((dialog->title() + "###" + dialog->id()).c_str())` / `BeginPopupModal((dialog->title() + "###" + dialog->id()).c_str(), ...)`. |

### Modified: `ScenePanel` class (`src/editor/panels/scene_panel.h`, `src/editor/panels/scene_panel.cpp`)

| Change | From | To |
|---|---|---|
| Delete confirmation state | `show_delete_confirmation_`, `pending_deletion_ids_`, `pending_deletion_with_children_`, `pending_deletion_first_name_` | Removed. State captured in lambdas. |
| Delete confirmation method | `draw_delete_confirmation_modal()` — renders raw ImGui popup with OpenPopup/BeginPopupModal. | Removed. In `execute_delete_entity()`, when `with_children > 0`, call `ctx.editor.open_dialog(CustomDialog(...))` with content function and button callbacks that capture entity info. |
| `execute_delete_entity()` | Checks `with_children > 0` → sets state members → sets `show_delete_confirmation_ = true`. | Checks `with_children > 0` → calls `ctx.editor.open_dialog(CustomDialog(...))`. |
| `draw_ui()` | Calls `draw_delete_confirmation_modal(ctx)` at end. | No longer calls it. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Error modal triggered while another error is showing** | Both dialogs appear stacked (different unique IDs). The second dialog opens on top of the first. User dismisses each independently. |
| **Error modal triggered rapidly (e.g., 10 errors in one frame)** | Each gets its own unique ID. 10 dialogs stack on top of each other. No dedup — users must dismiss each one. |
| **Error modal with empty title or message** | ImGui handles empty strings. Title bar shows empty title. Body shows empty text. Still works. |
| **Delete confirmation triggered while already showing** | `open_dialog()` returns `false` due to ID-based dedup (if using a fixed ID like "confirm-delete"). No duplicate modals. |
| **Save-prompt triggered while already showing** | Same dedup: `open_dialog()` returns `false` for a dialog with the same "save-changes" ID. No duplicate save-prompts. |
| **Save-prompt: Save clicked on untitled, error on Save As** | Save As file dialog opens → user picks a path → `save_scene_as()` fails → error modal shown. `pending_op_` is cleared (operation aborted). |
| **Save-prompt: Save clicked on untitled, user cancels Save As** | User cancels the Save As file dialog → no save occurs → `pending_op_` is cleared (operation aborted). The `ShowSaveFileDialog` callback fires with `std::nullopt`. |
| **Save-prompt: Save clicked on scene with file path, save fails** | `save_scene()` returns error → error modal shown with "Save Error". `pending_op_` is cleared. |
| **delete confirmation: empty selection (deletion_ids_ empty)** | Should not happen — `execute_delete_entity()` is only called when selection is non-empty. If it does happen, no dialog is opened (nothing to delete). |
| **delete confirmation: entity deleted between confirmation opening and Delete click** | The `DeleteEntityCommand` captures IDs at dialog creation time. If the entity is deleted externally (unlikely given modal is blocking), the command may operate on stale IDs — existing behavior preserved. |
| **Save-prompt: dirty_ set to false while dialog is open** | Not possible — user interaction with save-prompt is the only path that clears dirty state during a pending operation. The dialog is blocking. |
| **Multiple pending_op_ triggers before dialog is dismissed** | Not possible — dialog is modal and no user operations can set `pending_op_` while a modal dialog is visible (keyboard shortcuts are gated by `WantCaptureKeyboard`, menu items disabled). |
| **Headless mode (no ImGui init)** | `draw_ui()` returns early due to `initialized_` guard. `show_error_modal()` creates a dialog but it's never rendered. `open_dialog()` makes no ImGui calls — safe. |

## Error cases

| Case | Expected behavior |
|---|---|
| **`open_dialog()` returns false due to dedup** | Duplicate dialog is silently not opened. No crash. Calling code (e.g., `execute_delete_entity()`) should handle this gracefully — existing behavior in delete confirmation remains (state members no longer needed). |
| **Save-prompt CustomDialog has no buttons (construction error)** | Dialog renders with content but no buttons. User can only dismiss with Escape. Framework auto-closes. This is a coding error — not expected in production. |
| **Error modal button callback throws** | Exception propagates. Same behavior as rest of editor (no special handling). |
| **Very long error message (> 512 chars)** | ImGui word-wraps text. Dialog auto-resizes (AlwaysAutoResize flag). No truncation. |
| **Delete confirmation with 200+ entities selected** | Text shows "Delete 200 entities? (N have children...)". Button callbacks capture the ID vector by value. Works correctly. |

## Permissions and security

- No changes to permissions or security posture.
- Dialog content is provided by editor code, not user input — no injection risk.
- File dialog callbacks use captured lambdas — same pattern as existing Platform integration.

## Observability

| Signal | Source | Level |
|---|---|---|
| **Error dialog opened** | In the new `show_error_modal()` helper, log the title and ID before calling `open_dialog()`. | `BUDDD_LOG_DEBUG("Error dialog: {} (id={})", title, id)` |
| **Error dialog dismissed** | Already covered by the Dialog framework's `"Dialog closed: {}"` log in the cleanup phase (from SPEC-2026-007). | `BUDDD_LOG_DEBUG("Dialog closed: {}"` |
| **Delete confirmation shown** | Log in `execute_delete_entity()` when `with_children > 0` and dialog is opened. | `BUDDD_LOG_DEBUG("Delete confirmation: {} entities ({} with children)", ids.size(), with_children)` |
| **Save-prompt shown** | Log when `draw_pending_op_modal()` opens the save-prompt CustomDialog. | `BUDDD_LOG_DEBUG("Save prompt: {} (dirty)", pending_op_name)` |
| **Save-prompt button action** | Each button callback logs which action was taken (Save/Discard/Cancel) and the pending op. | `BUDDD_LOG_INFO("Save prompt: Save (pending={})", pending_op_name)` |
| **Pending op cancellation** | Log when Cancel or Escape aborts a pending operation. | `BUDDD_LOG_INFO("Save prompt cancelled: pending_op cleared")` |

## Documentation impact

The following existing documentation must be updated as part of this feature:

| Document | Required changes |
|---|---|
| `docs/wiki/editor/scene-management.md` | Update lines 3, 60, 81, 130, 131, 186 that state save-prompt and error modals are "not ported" — they will be ported by this feature. Update Phase 4/5/7 documentation to reflect that all popups now use the Dialog abstraction. Update the save-prompt "State machine" section (line 62) to describe the callback-driven approach. |
| `docs/wiki/architecture/module-map.md` | Update the `Editor` class entry (line 365): remove references to removed members (`error_modal_title_`, `show_error_modal_`, `save_prompt_requested_`, `save_prompt_seen_`, `draw_error_modals`, `draw_save_prompt_modal`, `draw_pending_op_modal`, `pending_file_path_`). Update `ScenePanel` entry to remove delete-confirmation members. Document the `show_error_modal()` wrapper. Update dialog render loop description (Phase 4) to reflect `"title###id"` pattern and removal of `opened_dialog_ids_` usage. |
| `docs/wiki/editor/editor-panels.md` | Update line 15 ("Save-prompt, error modals, and delete-confirmation remain separate") and line 492 (Dialog Abstraction section with delete-confirmation mention) to reflect that all popups now use the Dialog abstraction. |
| `docs/wiki/architecture/overview.md` | Update lines 209 and 272 that describe save-prompt and error modals as ad-hoc — reflect that all popups now use the Dialog abstraction. |

### ADR and other spec references

No ADRs were found that reference the specific error-modal or save-prompt implementation details in a way that would require updating. The ADRs referenced in `docs/wiki/editor/scene-management.md` (ADR-027, ADR-029, ADR-014, ADR-001, ADR-019, ADR-026) describe architectural boundaries and UX decisions — none are contradicted by porting popups to the Dialog abstraction.

This feature does not modify the Dialog abstraction itself (`editor_dialog.h`), so the Dialog abstraction spec (SPEC-2026-007) is not affected. The Editor Dialog Abstraction spec's AC-22 requirement for `opened_dialog_ids_` is satisfied by retention (the member is kept, though its usage in the render loop changes).

## Out of scope

- Changes to the Dialog abstraction (`editor_dialog.h`) itself.
- Changes to engine APIs or build system.
- Changes to menu/panel architecture.
- Changes to `PendingOp` enum — it remains as-is.
- Changes to `Editor::update()` lifecycle — pending_op processing remains.
- Porting any other popups beyond the three listed (e.g., future property pickers, asset browsers).
- Non-blocking overlays / toasts.
- Dialog animation, resizing, or z-ordering changes.
- Window close button behavior changes (still uses `pending_op_ = PendingOp::Quit` flow).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The Dialog abstraction (`Dialog`, `CustomDialog`, `DialogButton`, `open_dialog()`, `dialogs_`, Phase 4 render loop, Escape handling, cleanup) is already implemented and working as specified in SPEC-2026-007. |
| A-02 | The `CustomDialog` framework auto-closes after any button callback returns — button callbacks do NOT need to call `request_close()`. |
| A-03 | A random unique ID for error modals can be generated using `std::time()` combined with a random component (e.g., `std::rand()` or a simple counter). The ID only needs to be unique among currently-open dialogs, not globally unique. |
| A-04 | The save-prompt CustomDialog uses a fixed ID ("save-changes") for dedup — only one save-prompt can be open at a time. This matches existing behavior. |
| A-05 | The delete confirmation CustomDialog uses a fixed ID ("confirm-delete") for dedup — only one can be open at a time. This matches existing behavior. |
| A-06 | The `opened_dialog_ids_` set in `Editor` is retained for API backward-compatibility with SPEC-2026-007 (which requires the member to exist in its AC-22). However, the render loop no longer uses `opened_dialog_ids_` for first-frame-only OpenPopup tracking — the `"title###id"` pattern makes OpenPopup per-frame safe. The member is preserved as a class member but is no longer referenced in the Phase 4 rendering code. |
| A-07 | `draw_pending_op_modal()` continues to be called in Phase 5 (after dialog rendering) to manage the pending-op lifecycle. It opens the save-prompt dialog when needed, or executes the pending op if the scene is clean. The actual dialog rendering happens in Phase 4 via the Dialog abstraction. |
| A-08 | The `show_error_modal()` helper is retained as a thin wrapper that creates a CustomDialog and calls `open_dialog()`, rather than inlining the CustomDialog construction at all 15+ call sites. This reduces diff noise. |
| A-09 | The `EngineContext` for async file dialog callbacks is captured at the time the callback is created. This matches the existing pattern in `draw_pending_op_modal()` where the `ctx` reference is captured by the save-prompt button callbacks during `draw_ui()`. The `EngineContext` reference is valid for the duration of `draw_ui()`, and file dialog callbacks fire during the same frame's event processing — so no dangling reference. |
| A-10 | `execute_pending_op()` for `PendingOp::OpenScene` currently reads the dead `pending_file_path_` (which is always `std::nullopt`), making it a no-op. For the Discard path, the OpenScene case directly opens the file dialog inline (not via `execute_pending_op()`). After cleanup, `execute_pending_op()` for OpenScene will remain a no-op or be removed entirely — the file dialog opening happens in the Discard/Save button callbacks, not in `execute_pending_op()`. |
| A-11 | Save-prompt button callbacks may capture `this` (Editor pointer) to access `engine_->platform()` for opening a file dialog on untitled scenes. This is safe because `engine_` is a `unique_ptr<EngineService>` set during `Editor::setup()` and never changed or nullified for the lifetime of the `Editor` object. The `Editor` object itself is stable during `draw_ui()` and its callbacks execute synchronously within the same frame. File dialog callbacks (fired during `SDL_PumpEvents`) execute on the main thread — the captured `Editor*` remains valid because the Editor outlives any single frame. |

## Open questions

| ID | Question | Priority | Resolution |
|---|---|---|---|
| Q-01 | **Should `show_error_modal()` be retained as a convenience wrapper, or should all 15+ call sites be inlined with direct `open_dialog(CustomDialog(...))` calls?** | Scope | **Retain as a wrapper** to minimize diff noise. The wrapper creates the CustomDialog with a random unique ID, title, message, and OK button. Each call site simply calls `show_error_modal("Title", "Message")` as before. |
| Q-02 | **Should `execute_pending_op()` for `PendingOp::OpenScene` be removed entirely (since `pending_file_path_` is dead and the file dialog is opened inline)?** | Technical | **Retain but simplify** — `execute_pending_op()` for OpenScene becomes a no-op (the file dialog is opened by the Discard/Save button callbacks, not by `execute_pending_op()`). The method is kept for the NewScene and Quit cases. |
| Q-03 | **Should the save-prompt CustomDialog be opened from `draw_pending_op_modal()` (Phase 5) or from a dedicated location?** | Scope | **Open from `draw_pending_op_modal()`** — it already manages the pending_op lifecycle. On detect of `dirty_ == true` with no dialog yet open, it calls `open_dialog()`. The dialog's rendering happens in Phase 4. This preserves the existing Phase ordering. |
| Q-04 | **Should `opened_dialog_ids_` be removed when switching to `"title###id"` pattern?** | Technical | **Retain the member but stop using it in the render loop.** The `"title###id"` pattern makes OpenPopup per-frame safe without tracking. However, `opened_dialog_ids_` is kept as a class member for API backward-compatibility with SPEC-2026-007 (AC-22). The render loop no longer references it — OpenPopup is called every frame using the stable ID. |

Q-03 resolution depends on whether `ctx` (the `EngineContext`) is available at the point where `draw_pending_op_modal()` runs. It is — `draw_pending_op_modal(ctx)` already receives `ctx`. However, the button callbacks for Save (which may open a file dialog) need to capture `engine_->platform()` and the pending op. Since the `Editor` object (`this`) is stable, and `engine_` is set during `setup()` and never changes, callbacks can capture `this` and the pending op value. This is safe and matches the existing pattern.
