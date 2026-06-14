# Spec Review — Scene Panel Right-Click Selection

## Blocking issues

Items that must be resolved before the artifact can be accepted.

*None — no blocking issues identified.*

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **AC-12/AC-13 dependency on F-04 context menu implementation**: AC-12 and AC-13 verify that "Delete"/"Rename" menu items are enabled/disabled after right-click. These rely on the context menu implementation from F-04. The spec assumes F-04 behavior (menu items read `selection().empty()` for enabled state) which is correct, but the dependency could be made more explicit in the acceptance criteria section.
- **Unspecified modifier combinations (e.g., Alt+right-click)**: NG-07 says only plain, Ctrl, and Shift are handled for right-click. The pseudocode falls through to Replace for unhandled modifiers. This is implicitly correct, but the spec does not explicitly document what happens with other modifiers (Alt, GUI/Super, etc.). A brief statement like "Other modifiers are ignored — behavior degrades to plain right-click (Replace)" would preempt implementation questions.

## Required changes

Concrete, actionable changes requested:

*None — no required changes.*

## Suggested improvements

Optional ideas (not required):

- **Anchor verification in ACs**: The user-visible behavior table specifies anchor behavior for all right-click combinations (plain→anchor set, Ctrl→anchor unchanged, Shift→anchor unchanged). Consider adding explicit anchor verification to one or two acceptance criteria (e.g., "AC-17: Plain right-click sets anchor to the clicked entity. AC-18: Ctrl+right-click does not change the anchor.") to make this testable at the AC level rather than only in the behavior table.
- **Consider documenting the modifier fallthrough**: As noted in warnings, a brief note about Alt/Super/other modifier behavior would eliminate ambiguity for implementers.

## Definition of Ready Assessment

### Clarity & Completeness

| Criterion | Verdict | Rationale |
|-----------|---------|-----------|
| Scope is clearly defined (what is included and what is explicitly excluded) | ✅ Pass | Goals (G-01 through G-08) and Non-goals (NG-01 through NG-09) clearly delineate scope. "Out of scope" section reinforces exclusions. |
| Dependencies on other features, modules, or external systems are identified | ✅ Pass | F-03 (Entity Selection API), F-04 (Context menu, Entity Operations), and ImGui APIs are all referenced. Assumptions A-01 through A-10 confirm API compatibility. |
| Edge cases and error conditions are described | ✅ Pass | 14 edge cases (empty selection, multi-select, inline rename active, confirmation dialog open, rapid clicks, 10K+ entities, etc.) and 3 error cases (destroyed entity, missing tree node, invalid EntityId). |
| The expected behavior is unambiguous and testable | ✅ Pass | Clear user-visible behavior table, pseudocode implementation, 16 user stories with GIVEN/WHEN/THEN, and 16 acceptance criteria. |

### Verification

| Criterion | Verdict | Rationale |
|-----------|---------|-----------|
| The spec defines how the feature will be verified end-to-end | ✅ Pass | E2E Verification section covers manual smoke test and CI build verification. |
| Acceptance criteria are specific, measurable, and verifiable | ✅ Pass | All 16 ACs specify a verification method (Manual, Run tests, Build). Each describes concrete steps and expected outcomes. |
| Success and failure states are described | ✅ Pass | Success criteria (SC-001 through SC-004) define measurable outcomes. Error cases define failure behavior. |

### Documentation

| Criterion | Verdict | Rationale |
|-----------|---------|-----------|
| Interface changes (CLI flags, API signatures, config keys) are documented | ✅ Pass | No interface changes required. NG-06, NG-09 explicitly state no changes to EditorSelection API, World, Entity, or Editor classes. |
| Existing documentation that must be updated is listed | ✅ Pass | "Impact on Existing Specs" section precisely identifies F-03 NG-10 (remove) and F-04 AC-32 (update text). Both verified against actual spec files. |

### Technical

| Criterion | Verdict | Rationale |
|-----------|---------|-----------|
| Technical constraints are identified (system APIs, libraries, build changes) | ✅ Pass | ImGui APIs (IsItemHovered, IsMouseReleased, GetIO, OpenPopup), EditorSelection API (select, contains, set_selection, anchor), and scene_panel.cpp helpers (collect_range) are all identified. |
| Risks or unknowns are surfaced | ✅ Pass | Open questions Q-01, Q-02, Q-03 with documented resolutions. 10 assumptions covering API behavior, ImGui conventions, and code structure. |
| Performance or resource implications, if any, are noted | ✅ Pass | Edge case table documents O(1) for clear+select and O(n) for range select with 10K+ entities. |
