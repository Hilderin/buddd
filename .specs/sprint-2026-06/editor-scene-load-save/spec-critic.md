# Spec Review — F-01 Editor Scene Load/Save Integration

**Spec file:** `.specs/sprint-2026-06/editor-scene-load-save/spec.md`
**Reviewer:** spec-critic
**Date:** 2026-06-12

## Summary

The F-01 spec is well-structured, thorough, and covers the core save/open workflow comprehensively. It includes detailed state transitions, user stories, acceptance criteria, edge/error cases, assumptions, and observability logging. The spec successfully addresses the Definition of Ready in most areas.

All four previously blocking issues have been resolved in the latest spec revision. The spec is now compliant with the Definition of Ready and ready for implementation.

**Verdict: ACCEPTED** — all four blocking issues resolved, no new issues introduced.

### Re-review (2026-06-12) — verification of 4 resolved blocking issues

| Issue | Resolution | Status |
|-------|-----------|--------|
| B-01 UX contradiction | Documentation section now flags UX spec AC-015 and Story 1 for correction; ADR-029 and wiki noted for update | ✅ Resolved |
| B-02 OS window close | Edge case added (line 258); G-05 includes X / Alt+F4; behavior matches File > Quit save-prompt | ✅ Resolved |
| B-03 Overwrite ambiguity | Edge case states "Silent overwrite" explicitly — editor does not add extra confirmation | ✅ Resolved |
| B-04 Window API | A-08 specifies `Window::set_title(std::string title)` on `buddd::engine::Window` with SDL3 and Headless impls | ✅ Resolved |

No new blocking issues introduced. All Definition of Ready criteria remain satisfied.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01 — Contradiction with accepted UX spec on initial dirty state of untitled scenes**
  The F-01 spec (Q-04, G-06, state transition table) defines that a new untitled scene starts **clean** (`dirty_ = false`, title `"Untitled — Buddd Editor"`). However, the accepted north-star UX spec (`.specs/sprint-2026-06/editor-ux-design/spec.md`) states in AC-015: *"File > New Scene creates an empty scene … shows 'Untitled*' tab title"* and Story 1 says *"The Scene tab title shows 'Untitled*' (dirty indicator)."* The grill-me step resolved this (Q-04: clean by default), but the UX spec was not updated. This must be reconciled:
  - Either update UX spec AC-015 and Story 1 to match the decision (clean by default), or
  - Update F-01 spec to match UX spec (dirty by default).
  The F-01 "Documentation to update" section also lists updating the UX spec but does not flag this required correction to AC-015.

- [x] **B-02 — Missing edge case: OS window close button (X button) with dirty scene**
  The spec covers `File > Quit` (G-05) with dirty-check and save prompt. It does **not** specify what happens when the user closes the editor via the OS window decoration (X button, Alt+F4, etc.) while the scene is dirty. The accepted UX spec explicitly covers this (EC-10: *"Same save prompt as File > Quit. If user cancels, window close is aborted."*). The F-01 spec must either add this edge case or explicitly exclude it (with rationale). Without this, the promise that "Every unsaved change is lost when the editor closes" (Problem statement) may be misinterpreted as current behavior vs. the desired F-01 behavior.

- [x] **B-03 — Ambiguous overwrite behavior when saving to an existing file**
  Edge case table row: *"Save to a path where file already exists: Overwrite without confirmation (standard OS dialog behavior — ImGuiFileDialog may or may not warn depending on platform)."* This is contradictory — "without confirmation" is contradicted by "may or may not warn." The spec must clarify the **expected editor behavior** (does the editor expect silent overwrite, or does it rely on the dialog's platform-dependent warning?) and whether the editor should do anything if ImGuiFileDialog does not warn on the current platform.

- [x] **B-04 — Ambiguous Window title API contract**
  Assumption A-08: *"The `Window` class has a `set_title(const std::string&)` method (or equivalent)."* The "(or equivalent)" introduces ambiguity about the exact API. The implementation contract needs a precise contract (method name, signature, namespace). If `Window::set_title()` does not exist, the fallback strategy must be specified.

## Warnings

Non-blocking concerns for awareness:

- **Contradiction between F-01 and existing wiki page**: The wiki page `docs/wiki/editor/scene-management.md` (Untitled Scene Behavior section) states *"The tab title shows 'Untitled*' (dirty by default)"*, which contradicts F-01's clean-by-default decision. This is acknowledged in the Documentation to update section and the wiki is marked as "north-star future vision", but it should be confirmed that the wiki update corrects this to match the grill-me decision.

- **Manual dirty tracking is fragile**: G-06 states `mark_dirty()` is a manual call that panels and commands must invoke. Any code path that modifies the scene without calling `mark_dirty()` will silently lose dirty tracking. This is an architectural trade-off acknowledged by the spec, but it creates a maintenance burden — future panels may forget to call it.

- **Log channel tag TBD**: Line 306 says the log channel tag is "TBD during implementation." This is a minor documentation gap that should be resolved before the implementation contract is written.

- **Single `dirty_` boolean on Editor class**: The spec acknowledges (NG-08) that multi-tab dirty tracking is out of scope. However, the single `dirty_` bool on `Editor` will require refactoring when Prefab tabs (F-17) add independent per-tab dirty state. This is documented but worth early awareness for the Editor class design.

- **Test AC-04 (`[.display]` tag?)**: AC-04 verification says "Unit test: construct Editor, mark dirty, verify title contains `*`, save, verify `*` removed." If `set_title()` interacts with a display-backed `Window`, this test may need a display. The E2E section says AC-04 is a unit test, but its dependency on `Window::set_title` (which may require display) needs clarification.

## Required changes (all resolved)

All four required changes from the previous review have been implemented in the spec:

- [x] **OS window close button (X button) with dirty scene** — Added as edge case row (line 258) and G-05 updated to include X / Alt+F4.
- [x] **UX contradiction on initial dirty state** — Documentation to update section now flags UX spec AC-015 and Story 1 for correction (lines 327-334), ADR-029 noted for update, wiki north-star section marked for correction.
- [x] **Overwrite behavior clarified** — Edge case now reads: "Silent overwrite (no additional editor confirmation — ImGuiFileDialog may show platform-dependent warning independently)."
- [x] **Window::set_title exact API** — A-08 now specifies `Window::set_title(std::string title)` on `buddd::engine::Window` with concrete implementations for WindowSDL3 (SDL_SetWindowTitle) and WindowHeadless (no-op).

## Suggested improvements

Optional ideas (not required):

- Add an explicit note about the `Editor::mark_dirty()` contract: document which Editor methods and external callers are expected to invoke it.
- Resolve the log channel tag from "TBD during implementation" to a concrete value (e.g., `"Editor:Scene"` consistent with the UX spec observability section).
- Consider adding the X-button close behavior to the Goals table as G-11 or as an explicit non-goal if deferred.
- In the Documentation to update section, explicitly call out that UX spec AC-015 and Story 1 need correction to match the clean-by-default decision, rather than just "Add note that F-01 implements AC-015."
