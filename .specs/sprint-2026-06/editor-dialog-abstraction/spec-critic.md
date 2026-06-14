# Spec Review — Editor Dialog Abstraction (SPEC-2026-007)

## Re-review summary (Loop 2)

**Verdict: ACCEPTED** — All 3 previous blocking issues (B-01, B-02, B-03) and 3 warnings (W-01, W-02, W-03) are verified as fixed. No new blocking issues found. Minor inconsistencies remain (detailed in new warnings below) but do not affect clarity, testability, or implementability. The spec satisfies all Definition of Ready criteria.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01 — Ambiguous Close button callback pattern (unsettled specification)**: The spec presents **four different code patterns** for wiring the Close button's `request_close()` call, and never settles on a single definitive approach:
  1. Lines 190-200: Empty/comment-only callback body with unresolved questions inline.
  2. Lines 203-218: Raw pointer capture via separate `about_ptr` variable + direct `buttons_` access.
  3. Lines 222-227: `auto* raw = dlg.get(); [raw]() { raw->request_close(); }` using `set_button_callback(0, ...)`.
  4. Lines 231-235: `this` capture during `make_unique` construction (discusses safety but shows no concrete code).

  The section titled "Actual pattern used in final spec" (lines 231-235) does **not** provide executable code — only a prose safety argument. The implementer cannot determine which pattern is intended. This violates DoR criterion "The expected behavior is unambiguous and testable."

  **Resolution**: Spec updated to single constructor-based approach. Framework auto-closes after any button click; button callbacks do NOT call `request_close()`. All conflicting patterns removed.
  **Re-review (Loop 2)**: ✅ Verified — NG-08, button behavior (lines 113-115), About migration (lines 190-198), Assumption A-06, Q-03/Q-04 all consistently describe the single approach.

- [x] **B-02 — Missing API: `set_button_callback`**: The recommended code pattern at line 227 (`raw->set_button_callback(0, [raw]() { raw->request_close(); })`) calls a method `set_button_callback()` that is **not declared** in the `CustomDialog` API specification (lines 87-108). The only documented way to supply buttons is through the constructor's `std::vector<DialogButton> buttons` parameter. No mutator method is shown. Either `set_button_callback()` must be added to the API, or the pattern must use the constructor-based approach.

  **Resolution**: All references to `set_button_callback()` removed from spec. Buttons are configured exclusively via constructor.
  **Re-review (Loop 2)**: ✅ Verified — zero occurrences of `set_button_callback` remain in spec.

- [x] **B-03 — Private member access in recommended pattern**: The alternative pattern shown at lines 216-218 (`about->buttons_[0].callback = [about_ptr]() { about_ptr->request_close(); }`) accesses `buttons_`, which is declared `private` in the `CustomDialog` class (line 106). This code will not compile from external callers (e.g., the Editor or a menu callback). The spec must either make `buttons_` accessible (public or via an accessor) or remove this pattern.

  **Resolution**: All direct `buttons_` access patterns removed. Buttons are only configured via constructor.
  **Re-review (Loop 2)**: ✅ Verified — `buttons_` only appears as a private member declaration (line 106). No external access patterns remain.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.
**Loop 2**: All 3 previous blocking issues resolved — no new blocking issues found.

## Warnings

Non-blocking concerns for awareness:

### Previously reported (Loop 1) — resolved in spec update

- [x] **W-01 — Rendering code comment ambiguous about button rendering strategy**: Lines 150-154 show a rendering loop comment: `// If CustomDialog has buttons, render them (Editor calls render if dynamic_cast succeeds, or buttons rendered inside draw_content for subclass)`. This suggests two unresolved approaches. However, the `CustomDialog` specification (lines 265-266) clearly states that `draw_content()` renders both content and buttons. The comment is misleading and should be cleaned up to reflect the single intended approach (buttons are rendered inside `draw_content()`, no `dynamic_cast` needed).
  **Verification (Loop 2)**: ✅ Resolved — line 151 now reads `// Buttons are rendered inside CustomDialog::draw_content() — no dynamic_cast needed.`

- [x] **W-02 — Dual OpenPopup tracking strategies without selection**: The spec describes two alternative implementations for tracking first-frame `OpenPopup` — `std::unordered_set<std::string>` (lines 171-176) and a `bool has_opened_once_` flag on `Dialog` (line 177). AC-22 accepts either ("or equivalent"), but the spec should select one concrete approach for the implementation contract to avoid ambiguity during coding.
  **Verification (Loop 2)**: ✅ Resolved — only `std::unordered_set<std::string> opened_dialog_ids_` is described. AC-22 now specifically requires this member. No alternative flag remains.

- [x] **W-03 — Raw pointer lifecycle is safe but the spec's discussion is confusing**: The `auto* raw = dlg.get(); [raw]() { raw->request_close(); }` pattern is safe (the dialog remains alive in `dialogs_` while callbacks fire during `draw_ui()`), but the spec spends 40+ lines iterating through patterns and doubts without reaching a clean conclusion. This erodes implementer confidence.
  **Verification (Loop 2)**: ✅ Resolved — lifecycle discussion removed. Migration code uses clean `[this]` capture. Assumption A-06 provides a concise safety note.

### New findings (Loop 2)

- **W-04 — `DialogButton` struct missing `shortcut` field referenced in spec body**: Line 115 states "Buttons may optionally specify a shortcut key (stored but not auto-bound...)" and Q-02 resolves to "Store the shortcut field but do NOT auto-bind in this pass." However, the `DialogButton` struct (lines 56-60) does not include a `shortcut` field — only `label`, `label_id`, and `callback`. The key entities table (lines 232-238) also omits it. An implementer following the struct definition would not add the field, while the prose promises it. The struct should include `std::optional<ImGuiKey> shortcut = std::nullopt;` to match the described behavior.

- **W-05 — Render loop code snippet doesn't reflect `opened_dialog_ids_` tracking**: The rendering loop code (lines 144-155) calls `ImGui::OpenPopup(dialog->id().c_str())` unconditionally every frame. The prose (lines 171-175) and AC-08 clearly describe conditional tracking via `opened_dialog_ids_` (call only on first frame after open). While the prose is authoritative and an implementer reading the full spec will follow it, the code snippet should be updated to reflect the conditional logic to avoid confusion. For example, the loop should show a check like `if (opened_dialog_ids_.contains(dialog->id())) { ImGui::OpenPopup(...); opened_dialog_ids_.erase(...); }`.

## Required changes

### Previously required (Loop 1) — all completed

- [x] **Settle on one definitive Close button callback pattern**: ✅ Done — constructor-based approach selected, framework auto-closes.
- [x] **Remove `set_button_callback()` or add it to API**: ✅ Done — all references removed, buttons are constructor-only.
- [x] **Resolve the rendering comment ambiguity**: ✅ Done — comment now reads `no dynamic_cast needed`.

### Remaining (Loop 2) — optional clean-up items

The following are non-blocking corrections that would improve spec consistency (all are minor and do not affect implementability):

- **Add `shortcut` field to `DialogButton` struct**: Line 115 and Q-02 expect an `std::optional<ImGuiKey> shortcut` field, but the struct definition (lines 56-60) and key entities table (lines 232-238) omit it. Add `std::optional<ImGuiKey> shortcut = std::nullopt;` to the struct and document it in the key entities table.

- **Update render loop code snippet to reflect `opened_dialog_ids_` tracking**: The snippet at lines 144-155 shows unconditional `ImGui::OpenPopup()`. Update it to show the conditional check described in the prose (lines 171-175) so code and prose agree.

## Suggested improvements

Optional ideas (not required):

- (Loop 1) The lifecycle discussion at lines 220-235 could be simplified to a brief 2-line note. ✅ Resolved — now handled by Assumption A-06.
- (Loop 1) Consider making `buttons_` protected or adding a `button_count()`/`set_button_callback()` API for post-construction customization. ✅ Resolved — buttons are constructor-only, no post-construction customization needed.
- (Loop 2) Consider adding `opened_dialog_ids_` to the "Editor integration" code snippet at lines 123-131 alongside `dialogs_` and `open_dialog()` for completeness. Currently it's only in the key entities table and AC-22, not in the member listing.
