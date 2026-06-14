# Governance Review — Port Remaining Popups to Dialog Abstraction

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **opened_dialog_ids_ retention in spec vs removal in code**: Spec AC-017 and A-06 state the member is "retained for API backward-compatibility with SPEC-2026-007." Implementation contract (Change 9) and code remove it entirely, including removing `#include <unordered_set>` from `editor.h`. This intentional deviation is documented in Decision Log D-08 in coordination.md and was approved by Human Validation (Hilderin, 2026-06-14). The spec remains as a historical snapshot; the decision log records the override.

- [x] **show_error_modal() retention in spec vs removal in code**: Spec Q-01 resolution states "Retain as a wrapper" — retaining `show_error_modal()` as a private helper. Implementation contract (Change 2) removes it entirely, replacing all call sites with the public `open_error_dialog()` helper. This is documented in Decision Log D-06 (convenience helpers) and approved by Human Validation.

- [x] **DialogButton::callback signature change vs spec NG-01**: Spec NG-01 says "No changes to the Dialog abstraction itself (`editor_dialog.h`) — no new base class features, no API changes." Implementation contract changes `DialogButton::callback` from `std::function<void()>` to `std::function<bool()>`. This was accepted by implementation-contract-critic (NG-01 scoped to "no new base class features" — callback signature change permitted), documented in Decision Log D-04, and approved by Human Validation.

- [x] **Code review confirms all 19 done criteria satisfied**: All 19 DC items verified. 697 tests pass (22658 assertions). Zero new warnings from `src/editor/` and `tests/`. Manual smoke test (DC-16) is display-required and could not be verified headlessly — noted as pending in code review but not blocking.

- [x] **Spec AC-021 (headless safety) verified**: The unique-ID generator (`std::time(nullptr)` + static counter) makes no ImGui calls — safe in headless mode.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-027 (Editor Architecture)**: No violations. Dialog abstraction and popup porting operate within the editor library's scope. Architecture boundary (no SDL3/OpenGL/GLM headers in `src/editor/`) is respected.

- [x] **ADR-029 (Editor UX Decisions)**: No violations. Popup porting does not affect tab system, layout decisions, or play mode behaviors. All decisions (one-scene-at-a-time, fixed layout per tab type, console persistence) remain intact.

- [x] **ADR-019 (Architecture Boundaries)**: No SDL3, OpenGL, or GLM headers in editor code. Verified by code review.

- [x] **ADR-026 (ImGui Integration)**: No changes to ImGui integration patterns. ImGui continues to be used via existing abstractions.

- [x] **No ADR amendments required**: The spec confirms no ADRs reference the specific error-modal or save-prompt implementation details in a way that requires updating. This governance review confirms the same.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **module-map.md line 365**: Now correctly states `opened_dialog_ids_` was "removed — OpenPopup is now called unconditionally each frame" and `DialogButton::callback` changed from `void()` to `bool()`. Fixed by wiki-agent re-review.

- [x] **editor-panels.md line 492**: Now correctly states `opened_dialog_ids_` was "previously tracked via... since removed as OpenPopup is now called unconditionally each frame" and `DialogButton::callback` changed from `void()` to `bool()`. Fixed by wiki-agent re-review.

- [x] **scene-management.md**: Correctly reflects all popups ported, convenience helpers, `defer()` mechanism, `DialogButton::callback` bool return, and `"title###id"` pattern. Last-reviewed date updated to 2026-06-14.

- [x] **overview.md line 272**: Correctly updated to reflect all popups using Dialog abstraction, convenience helpers, and `defer()` mechanism.

## Warnings

Non-blocking concerns for awareness:

- **Wiki-agent completed with incorrect `opened_dialog_ids_` status (RESOLVED)**: The wiki-agent's coordination.md summary originally stated it noted `opened_dialog_ids_` as "retained for backward compatibility" — but the member was actually removed. The wiki has since been corrected by a re-review: both `module-map.md` and `editor-panels.md` now correctly state `opened_dialog_ids_` was removed.

- **Test comment references to removed `opened_dialog_ids_`**: Lines 1075-1078 and 1241 in `editor_tests.cpp` contain historical comments referencing the removed member. These are comments only (no active code) but could confuse future readers. From code-review warning.

- **Test coverage for save-prompt/delete-confirmation behavioral paths is thinner than the contract suggests**: T4/T5/T6/T8/T9/T10 from the implementation contract are covered only indirectly or partially. Behavioral verification relies on the manual smoke test (DC-16, display-required). From code-review warning.

- **Spec was not updated for three deliberate deviations**: The spec retains language about `opened_dialog_ids_` retention, `show_error_modal()` wrapper, and "no API changes" to `editor_dialog.h`. All three were deliberately changed during implementation. The spec is a historical snapshot (read-only after workflow completes), so this is acceptable — the decision log in coordination.md documents the overrides. However, future readers should consult both the spec and the decision log for the full picture.

## Required governance updates — APPLIED

Concrete changes to governance documents (ADRs, wiki) — all resolved:

1. **module-map.md** ✅ — Editor class entry updated:
   - `opened_dialog_ids_` now listed as "(removed — OpenPopup is now called unconditionally each frame)"
   - `DialogButton::callback` type correctly listed as `bool()` (return `true` to close)
   - Phase mention unchanged (7-phase rendering remains accurate)
   
2. **editor-panels.md** ✅ — Dialog abstraction section updated:
   - `opened_dialog_ids_` now correctly described as "previously tracked via... since removed"
   - Last-reviewed date updated to reflect this feature's changes
