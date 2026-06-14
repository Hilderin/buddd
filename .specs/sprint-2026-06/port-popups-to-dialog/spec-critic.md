# Spec Review — Port Remaining Popups to Dialog Abstraction

## Summary

The spec is well-structured, clearly defines the three port targets (error modals, delete confirmation, save-prompt), and provides detailed acceptance criteria, edge cases, and behavioral specifications. The callback-driven save-prompt approach, `"title###id"` fix, and dead code removal are all clearly described. The previous blocking issue (missing `## Documentation impact` section from Loop 1 review) has been resolved — the spec now lists 4 wiki pages requiring updates with specific line references. All previous warnings have also been addressed: AC-022 is now a proper behavioral-equivalence criterion, the stale `[NEEDS CLARIFICATION]` tag on Q-03 is removed, an explicit `engine_` lifetime assumption (A-11) has been added, and the `opened_dialog_ids_` retention/usage ambiguity is resolved via A-06 and AC-017.

All Definition of Ready criteria are satisfied. The spec is ready for implementation.

**Verdict**: Accepted — no blocking issues remaining.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Missing documentation impact section.** (RESOLVED in Loop 1 fix) The spec now has `## Documentation impact` (lines 275-290) listing 4 wiki pages needing updates with specific line references and required changes, plus an ADR/spec-check subsection confirming no ADR conflicts.

## Warnings

Non-blocking concerns for awareness:

- ~~**AC-022 is not a real acceptance criterion.**~~ **RESOLVED**: AC-022 is now a proper behavioral-equivalence criterion (line 187): "Behavioral equivalence: all three ported popups render identically to before the port" with a concrete manual smoke test verification step.

- ~~**`[NEEDS CLARIFICATION]` tag on Q-03 resolution could confuse readers.**~~ **RESOLVED**: The `[NEEDS CLARIFICATION]` tag has been removed from the Q-03 resolution area. The text now reads cleanly with its resolution description.

- ~~**Save-prompt button callback context is underspecified.**~~ **RESOLVED**: A new explicit assumption (A-11, lines 318-319) documents the `engine_` lifetime guarantee for save-prompt button callbacks, including the safety of capturing `this` and the fact that `engine_` is a `unique_ptr<EngineService>` set during `setup()` and never changed.

- ~~**`opened_dialog_ids_` ambiguity.**~~ **RESOLVED**: A-06 (line 313) and AC-017 (line 182) now explicitly state: retain the member but stop using it in the render loop. The `"title###id"` pattern makes per-frame OpenPopup safe without tracking. The member is preserved for API backward-compatibility with SPEC-2026-007 AC-22.

No new warnings.

## Required changes

All 5 required changes from Loop 1 review are now addressed:

1. ✅ **Added `## Documentation impact` section** (lines 275-290) listing 4 wiki pages with specific line references, required changes, and an ADR/spec-check subsection confirming no ADR conflicts.

2. ✅ **AC-022 converted** (line 187) from meta-commentary to a proper behavioral-equivalence acceptance criterion with explicit manual smoke test verification.

3. ✅ **Stale `[NEEDS CLARIFICATION]` tag removed** from Q-03 resolution area (line 329). The resolution text now reads cleanly.

4. ✅ **A-11 assumption added** (lines 318-319) explicitly documenting `engine_` lifetime guarantee for save-prompt button callbacks.

5. ✅ **`opened_dialog_ids_` ambiguity resolved**: A-06 (line 313) and AC-017 (line 182) mandate retention of the member but clarify it is no longer used in the render loop.

## Suggested improvements

Optional ideas (not required):

- The "Before/After" tables in User Stories 5 and 6 are excellent. Consider adding a similar "Before/After" for the `draw_pending_op_modal()` transformation (Story 5 covers it partially via `SavePromptResult` removal, but the controller logic change is implicit).

- The edge case table could add an entry for "Error modal triggered during an active save-prompt" — e.g., if save fails during the save-prompt Save flow, an error modal appears. The save-prompt dialog would still be open while the error modal stacks on top. This may be handled correctly already (different IDs, both are dialogs) but it's worth documenting as an edge case.

- Consider adding explicit test guidance for the headless dialog accumulation edge case: in headless mode, `draw_ui()` is guarded by `initialized_` so error dialogs created via `show_error_modal()` accumulate in `dialogs_` but are never rendered or cleaned up. The existing AC-021 test passes because it doesn't crash, but nothing prevents unbounded growth if `show_error_modal()` is called repeatedly in headless. This could be addressed by clearing `dialogs_` in `shutdown()` or by documenting the limitation.
