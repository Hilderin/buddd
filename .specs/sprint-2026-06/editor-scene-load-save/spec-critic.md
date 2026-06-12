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
| B-04 Window API | A-08 specifies `Window::set_title(std::string)` on `buddd::engine::Window` with SDL3 and Headless impls | ✅ Resolved |

### Re-review #2 (2026-06-12) — ImGuiFileDialog → SDL3 native file dialogs

**Scope**: Re-review focused on the file dialog mechanism change only. All 10 ACs, 6 user stories, edge cases, and error cases remain unchanged.

**Findings**:
- ✅ **No ImGuiFileDialog references remain** in spec.md (zero matches from grep).
- ✅ **Platform abstraction correctly replaces ImGuiFileDialog** in G-09, Actors, OS File Dialog section, and all affected assumptions (A-04, A-05, A-12).
- ✅ **Async SDL3 dialog flow** properly described with callback model, thread safety via Platform, and frame-bound result processing.
- ✅ **Edge/error cases updated** to reflect SDL3 dialog behavior (cancellation, empty path, init failure).
- ✅ **Documentation to update** correctly lists Platform dialog methods for module-map.md.
- ⚠️ **Minor ADR-019 ambiguity**: The spec describes the Platform API using `SDL_DialogFileFilter` (line 127), which could be misinterpreted as leaking SDL3 types into the editor. The stated intent (Platform "respects the ADR-019 architecture boundary") is clear, but the spec should clarify that Platform defines its own filter type to avoid confusion.

No new blocking issues introduced. All Definition of Ready criteria remain satisfied for the changed areas.

**Verdict: ACCEPTED** — no new blocking issues, one minor ambiguity flagged as a warning.

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

- **SDL_DialogFileFilter in Platform API description (ADR-019 concern)**: The spec at line 127 describes the Platform API using `SDL_DialogFileFilter{.name = "YAML Scene", .pattern = "yaml"}` notation. If this is the public Platform API signature, editor code would need to construct SDL3 types, violating ADR-019 (no SDL3 headers in `src/editor/`). While the spec's stated intent (Platform "respects the ADR-019 architecture boundary") and assumptions (A-04: SDL3 APIs "abstracted through the Platform interface") are clear, the spec should explicitly state that Platform defines its own filter type (e.g., `Platform::DialogFilter` or similar) so editor code never touches SDL3 types. This is a minor documentation clarity issue — the intent is correct but the wording is ambiguous.

- **Async dialog callback dispatch mechanism**: The spec (A-05, A-12) states the Platform abstraction queues the dialog result for the Editor to read on the next frame. The exact mechanism (lock-free queue, mutex-guarded vector, atomic flag + stored path) is not specified. This is acceptable at this level but should be resolved in the implementation contract to avoid data races.

## Required changes (resolved from previous reviews)

### All four from original review (resolved 2026-06-12)

All four required changes from the previous review have been implemented in the spec:

- [x] **OS window close button (X button) with dirty scene** — Added as edge case row (line 258) and G-05 updated to include X / Alt+F4.
- [x] **UX contradiction on initial dirty state** — Documentation to update section now flags UX spec AC-015 and Story 1 for correction (lines 327-334), ADR-029 noted for update, wiki north-star section marked for correction.
- [x] **Overwrite behavior clarified** — Edge case now reads: "Silent overwrite (no additional editor confirmation — ImGuiFileDialog may show platform-dependent warning independently)."
- [x] **Window::set_title exact API** — A-08 now specifies `Window::set_title(std::string title)` on `buddd::engine::Window` with concrete implementations for WindowSDL3 (SDL_SetWindowTitle) and WindowHeadless (no-op).

### Spec update — ImGuiFileDialog → SDL3 native dialogs (applied in this cycle)

The spec-author replaced all ImGuiFileDialog references with SDL3 native file dialogs via the Platform abstraction. Verified changes:

- [x] **G-09** — Updated to reference `Platform::show_open_file_dialog()` and `Platform::show_save_file_dialog()`
- [x] **Actors > Platform** — Updated to describe Platform abstraction wrapping SDL3, respecting ADR-019
- [x] **OS File Dialog section** — Rewritten for SDL3 async dialog flow with callbacks and window parent association
- [x] **Assumptions A-04, A-05, A-12** — Replaced ImGuiFileDialog-specific assumptions with SDL3 availability, async callback model, and thread safety
- [x] **Edge cases** — Updated for SDL3 dialog behavior (non-yaml filter, silent overwrite, cancel, init failure)
- [x] **Permissions** — Updated to reference SDL3 native dialog (via Platform)
- [x] **Documentation to update** — Updated to reference Platform dialog methods in module-map.md

## Suggested improvements

Optional ideas (not required):

- Add an explicit note about the `Editor::mark_dirty()` contract: document which Editor methods and external callers are expected to invoke it.
- Resolve the log channel tag from "TBD during implementation" to a concrete value (e.g., `"Editor:Scene"` consistent with the UX spec observability section).
- Consider adding the X-button close behavior to the Goals table as G-11 or as an explicit non-goal if deferred.
- In the Documentation to update section, explicitly call out that UX spec AC-015 and Story 1 need correction to match the clean-by-default decision, rather than just "Add note that F-01 implements AC-015."
