# Spec Review — Scene Panel — Entity Tree

**Re-review (12-Jun-2026): ACCEPTED.** The previous blocking issue — missing "Documentation impact" section — has been resolved (new lines 284–293 list `docs/wiki/editor/editor-panels.md` and `docs/wiki/editor/scene-management.md`). All 12 Definition of Ready criteria are satisfied. No remaining blocking issues.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Missing list of existing documentation to update** — The spec does not explicitly list which wiki pages or other documents need updating to reflect the interface changes. The Definition of Ready criterion requires this to be listed. At minimum, `docs/wiki/editor/editor-panels.md` needs updating for the `EditorContext` signature change in panel/menu base classes. `docs/wiki/editor/scene-management.md` may also need a review.
  - **Resolved (re-review 12-Jun-2026):** A "Documentation impact" section was added (lines 284–293) listing `docs/wiki/editor/editor-panels.md` and `docs/wiki/editor/scene-management.md` with specific reasons for each update.

## Warnings

Non-blocking concerns for awareness:

- AC-01 through AC-06 describe unit tests that verify `ImGui::TreeNodeEx` was called with specific arguments. The spec does not identify the test infrastructure or framework that will support capturing/counting ImGui calls (e.g., imgui_test_engine, manual wrappers, ImGui item store queries). This is acceptable as an implementation detail but may cause estimation surprises.
- The spec does not mention the `#include` additions needed in existing files (`editor_panel.h`, `editor_menu.h`, `menu_bar.h`, all 5 panel headers) to pull in the new `editor_context.h` header. Minor omission — easily resolved during implementation.
- SC-004 asserts a performance target of `< 0.1 ms per frame for < 1000 entities` but does not specify measurement methodology (release vs debug build, profiling tool, platform). This is acceptable for a success criterion but may be hard to verify objectively without a defined benchmark.

## Required changes

Concrete, actionable changes requested:

1. Add a section (e.g., "Documentation impact" or "Existing documentation to update") that explicitly lists every existing document that must be updated when this spec is implemented. At minimum: `docs/wiki/editor/editor-panels.md` (signature change for panel/menu base classes). Consider whether `docs/wiki/editor/scene-management.md` or other wiki pages also need updates.

**Re-review verdict (12-Jun-2026):** ✅ All required changes have been implemented. The spec now includes a "Documentation impact" section (lines 284–293) listing `docs/wiki/editor/editor-panels.md` and `docs/wiki/editor/scene-management.md` with specific update reasons.

## Suggested improvements

Optional ideas (not required):

- Consider mentioning the test infrastructure approach for ImGui call verification (e.g., "using manual push/pop capture wrappers" or "using imgui_test_engine hooks") to de-risk AC-01 through AC-06.
- Add a brief note about expected `#include` additions in affected files to help implementers.
- Consider clarifying the SC-004 performance measurement context (profile build, approximate platform) to make the metric more objectively verifiable.

---

### Criteria assessment

| Criterion | Status |
|---|---|
| Scope is clearly defined (included / excluded) | ✅ Satisfied |
| Dependencies on other features, modules, or external systems are identified | ✅ Satisfied |
| Edge cases and error conditions are described | ✅ Satisfied |
| The expected behavior is unambiguous and testable | ✅ Satisfied |
| How the feature will be verified end-to-end | ✅ Satisfied |
| Acceptance criteria are specific, measurable, and verifiable | ✅ Satisfied |
| Success and failure states are described | ✅ Satisfied |
| Interface changes (CLI flags, API signatures, config keys) are documented | ✅ Satisfied |
| Existing documentation that must be updated is listed | ✅ Resolved |
| Technical constraints are identified (system APIs, libraries, build changes) | ✅ Satisfied |
| Risks or unknowns are surfaced | ✅ Satisfied |
| Performance or resource implications, if any, are noted | ✅ Satisfied |
