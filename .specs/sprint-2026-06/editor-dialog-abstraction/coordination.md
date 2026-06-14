# Workflow Coordination: editor-dialog-abstraction

## Orchestrator

**Feature**: `editor-dialog-abstraction`
**Status**: completed
**Current step**: completed
**Initial instructions**: Introduce a reusable Dialog abstraction for the Buddd Editor. Currently, every popup/modal (save-prompt, error modals, About, delete confirmation) uses ad-hoc booleans and inline ImGui OpenPopup/BeginPopupModal calls scattered across Editor and panel classes. The proposal is to create a Dialog class with:

- Title + content render callback
- ID-based deduplication (option A: same ID → skip/focus existing)
- Self-registration into Editor (added to a dialogs_ vector)
- Self-removal on close (Escape, OK, Cancel)
- Reusable for any temporary/top-level UI element (modals, popups, overlays)

**Notes**: Initial design discussion completed. Option A (ID-based dedup) confirmed. Proceeding to grill-me step.
Loop 1 → spec-author: fix spec-critic blocking issues (B-01/B-02/B-03: Close button callback pattern ambiguous). Human chose constructor-based pattern. Re-invoking spec-author.
Loop 2 → contract-author: fix contract-critic B-01 (contradictory logging approaches). Both resolved.
Human validation: approved by Hilderin on 2026-06-13.

## Decision Log

| # | Decision | Rationale |
|---|---|---|
| D-01 | **Dialog IDs for dedup**: Each Dialog has an `id` (string). If a dialog with the same ID already exists in the editor, the open request is ignored (or the existing dialog is focused). | Prevents duplicate popups of the same type (About, save-prompt, etc.). |
| D-02 | **Separate `dialogs_` vector** in Editor, not merged into menus_ or panels_. | Dialogs have different semantics (modal, always-on-top, transient). Merging would require refactoring existing menu/panel interfaces with no benefit. |
| D-03 | **Dialog API**: Abstract `Dialog` base class with `id()`, `title()`, `draw_content()`, `request_close()`, `should_close()`. Concrete `CustomDialog` for simple cases (takes `content_fn`, `vector<DialogButton>`, `on_close` callback). Complex dialogs (save-prompt) inherit `Dialog` directly. | Balances reusability with flexibility. |
| D-04 | **Escape/X close behavior**: Handled per-dialog. `CustomDialog::on_close` callback fires when dialog closes via non-button means. | Allows each dialog to define its Escape/X semantics. |
| D-05 | **No explicit result type**: Button callbacks execute actions directly and call `request_close()`. | Simplifies API; avoids result-polling pattern. |
| D-06 | **Framework renders buttons after `draw_content()`**: CustomDialog handles button layout. | Eliminates boilerplate for standard button bars. |
| D-07 | **Escape on stacked dialogs**: Only the topmost dialog receives `handle_escape()`. Editor calls `handle_escape()` on the last dialog in `dialogs_` when Escape is pressed. | Prevents closing multiple stacked dialogs at once. |
| D-08 | **`handle_escape()` virtual method**: Default calls `request_close()`. Custom subclasses override for custom behavior (e.g., Cancel action). `CustomDialog::handle_escape()` fires `on_close` then `request_close()`. | Clean separation from `draw_content()`. |

## spec-author

**Status**: completed
**Summary**:
Fixed 3 blocking spec-critic issues (B-01/B-02/B-03) and addressed warnings W-01/W-02/W-03 in `spec.md`. Changes: (1) Replaced ambiguous Close button callback patterns with a single constructor-based approach where the framework auto-closes after any button click — button callbacks do NOT call `request_close()`. (2) Removed all references to `set_button_callback()` and direct `buttons_` access — buttons are configured exclusively via constructor. (3) Cleaned up rendering loop comment (removed `dynamic_cast` ambiguity). (4) Locked in `std::unordered_set<std::string>` for OpenPopup tracking (removed alternative `has_opened_once_` flag). (5) Updated NG-08, User Stories 1/4, AC-11/22, Key entities, Assumptions A-05/A-06, and Open Questions Q-03/Q-04 to reflect the new auto-close behavior.
**Artifacts**:
- `.specs/sprint-2026-06/editor-dialog-abstraction/spec.md`
**Questions for human**:
- none
**Warnings**:
- none
**Blocking issues**:
- none

## spec-critic

**Status**: completed
**Summary**:
Loop 2 re-review: All 3 previous blocking issues (B-01/B-02/B-03 — Close button pattern ambiguous, missing `set_button_callback()` API, private `buttons_` access) are verified as resolved. All 3 warnings (W-01/W-02/W-03 — rendering comment, dual OpenPopup tracking, lifecycle discussion) are also resolved. Two minor new warnings identified (W-04: `DialogButton` struct missing `shortcut` field inconsistent with prose; W-05: render loop code snippet omits `opened_dialog_ids_` tracking logic) — these are non-blocking. The spec satisfies all Definition of Ready criteria. Spec is **accepted**.
**Artifacts**:
- `.specs/sprint-2026-06/editor-dialog-abstraction/spec-critic.md`
**Questions for human**:
- none
**Warnings**:
- W-04: `DialogButton` struct (lines 56-60) is missing the `shortcut` field referenced in line 115 and Q-02. Add `std::optional<ImGuiKey> shortcut = std::nullopt;` to struct.
- W-05: Render loop code snippet (lines 144-155) shows unconditional `ImGui::OpenPopup()` but prose (lines 171-175) describes conditional tracking via `opened_dialog_ids_`. Update snippet to match prose.
**Blocking issues**:
- none

## implementation-contract-author

**Status**: completed

**Summary**:
Fixed contract-critic B-01 (contradictory logging approaches): selected the inline-logging-in-`erase_if` predicate approach, applied it consistently in Section 3c, removed all alternatives from Section 3e, and removed the unused `closed_count` variable. Also addressed W-02 (tightened UT-04 to a single approach using a draw_content counter), W-03 (replaced line-number references with descriptive anchors), and W-04 (Done criteria now matches the actual code in Section 3c).
**Artifacts**:
- `.specs/sprint-2026-06/editor-dialog-abstraction/implementation-contract.md`
**Questions for human**:
- none
**Warnings**:
- none
**Blocking issues**:
- none

## implementation-contract-critic

**Status**: completed
(Loop 1 re-review: contract-author fixed B-01, W-02, W-03, W-04 — all resolved, no new issues)
**Summary**:
Loop 2 re-review: All blocking issues and warnings from the previous review cycle are verified as resolved. B-01 (contradictory logging approaches) is now a single definitive inline-in-`erase_if` approach. W-01 (dead `closed_count` variable) removed. W-02 (UT-04 imprecise description) tightened with a single approach. W-03 (fragile line-number references) replaced with descriptive anchors. W-04 (Done criteria inconsistency) resolved — Section 3c code and Done criteria now match. No new issues introduced. All other criteria checks pass (allowed/forbidden files, tests, conventions, architecture decisions, spec/wiki consistency). Contract is **accepted**.
**Artifacts**:
- `.specs/sprint-2026-06/editor-dialog-abstraction/implementation-contract-critic.md`
**Questions for human**:
- none
**Warnings**:
- none
**Blocking issues**:
- none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-13 21:51:17
**Warnings**:
<none>
**Notes**: Approved. Dialog abstraction with constructor-based CustomDialog, ID dedup, auto-close after button click, Escape on topmost only.

## code-implementer

**Status**: completed
**Summary**:
Implemented the Editor Dialog Abstraction: created `src/editor/editor_dialog.h` with `Dialog` (abstract base), `CustomDialog` (concrete), and `DialogButton` (struct). Modified `editor.h` to add `open_dialog()`, `dialogs_`, `opened_dialog_ids_`, and remove `show_about_`/`draw_about_popup()`. Modified `editor.cpp` to implement `open_dialog()` with ID-based dedup, replace Phase 4 with a general dialog rendering loop with `OpenPopup` tracking and Escape dispatch, and migrate the About popup to a `CustomDialog`. Added 14 unit tests in `tests/editor/editor_tests.cpp` covering all contract requirements.

**Fix**: `OpenPopup` was using `dialog->id()` ("about") while `BeginPopupModal` used `dialog->title()` ("About Buddd Editor") — mismatched ImGui popup IDs caused the About dialog to never appear. Fixed by using `dialog->title()` for both.
**Artifacts**:
- `src/editor/editor_dialog.h` (new)
- `src/editor/editor.h` (modified)
- `src/editor/editor.cpp` (modified)
- `tests/editor/editor_tests.cpp` (modified)
**Questions for human**:
- none
**Warnings**:
- The display-dependent integration tests (IT-01 through IT-04) that require `draw_ui()` with ImGui rendering could not be executed because offscreen SDL3 mode does not provide a working ImGui context for the test harness. These scenarios are covered by direct logic tests (open_dialog return values, dialog method calls) and manual smoke testing.
**Blocking issues**:
- none

## code-reviewer

**Status**: completed
(Re-review after code fixes: OpenPopup/erase_if fixes — all verified, About dialog confirmed working)
**Artifacts**:
- `.specs/sprint-2026-06/editor-dialog-abstraction/code-review.md`
**Questions for human**:
- none
**Warnings**:
- UT-01 lifecycle test cannot fully exercise the draw_ui → request_close → removal path because draw_ui() requires an ImGui runtime context. The removal logic is structurally verified via code analysis.
- UT-06/UT-12 button auto-close contract is tested via handle_escape() as a proxy (same callback-then-close pattern) rather than via ImGui::Button() simulation. The contract acknowledges this limitation.
- UT-04 OpenPopup tracking is verified via the opened_dialog_ids_ mechanism and dedup behavior rather than direct ImGui::OpenPopup observation (requires ImGui context).
**Blocking issues**:
- none

## wiki-agent

**Status**: completed
**Summary**:
Updated three wiki pages to reflect the Editor Dialog Abstraction implementation. Added `editor_dialog.h` to the module map, documented the dialog abstraction in editor-panels.md, and documented Phase 4 as the dialog rendering phase in scene-management.md with a note that save-prompt/error modals remain separate.
**Artifacts**:
- `docs/wiki/architecture/module-map.md` (modified)
- `docs/wiki/editor/editor-panels.md` (modified)
- `docs/wiki/editor/scene-management.md` (modified)
**Changes made**:
- `module-map.md`: Added `editor_dialog.h` to Editor UI abstractions table with Dialog/CustomDialog/DialogButton roles. Updated `editor.h` entry to document `open_dialog()`, `dialogs_`, `opened_dialog_ids_`, and Phase 4 dialog rendering.
- `editor-panels.md`: Added dialog abstraction bullet to status header and v1 foundation section. Added spec reference to Related specs. Updated Last reviewed date.
- `scene-management.md`: Added dialog abstraction to status header. Updated save-prompt and error modal references to note About popup uses CustomDialog and other modals remain separate. Documented Phase 4 as dialog rendering phase in F-01 foundation section. Added spec reference. Updated Last reviewed date.
**Questions for human**:
- none
**Warnings**:
- none
**Blocking issues**:
- none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance validation passed. All 22 acceptance criteria are verified complete. The spec, implementation contract, code, and wiki are coherent with each other and with ADR-027/ADR-029/ADR-026. No ADRs are violated. Wiki updates correctly reflect the implementation (Phase 4 dialog rendering, `editor_dialog.h` with Dialog/CustomDialog/DialogButton, open_dialog/dialogs_/opened_dialog_ids_). All workflow gates were followed: spec-critic → contract-critic → human validation → implement → code-review → wiki → governance. One minor spec struct definition inconsistency (DialogButton missing `shortcut` field in struct but present in prose) is non-blocking — all downstream artifacts (contract, code, wiki) agree on the correct definition. No blocking governance issues found.
**Artifacts**:
- `.specs/sprint-2026-06/editor-dialog-abstraction/governance-review.md`
**Questions for human**:
- none
**Warnings**:
- The spec's DialogButton struct definition omits the `shortcut` field that the spec prose, contract, code, and wiki all include. Spec-critic flagged this as W-04 (non-blocking). Optional cleanup if the spec struct is updated.
**Blocking issues**:
- none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
