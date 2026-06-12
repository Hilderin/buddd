# Governance Review — F-01 Editor Scene Load/Save Integration

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [ ] **UT-02 incomplete — missing titled/clean and titled/dirty scenarios** (from code-review). The window title format test covers only 2 of 4 required scenarios from the spec. The untitled cases are tested; the titled/clean and titled/dirty cases are not. `build_title_string()` is correct by code inspection and related tests (UT-06, UT-07) indirectly validate the titled path, but direct coverage is missing. Non-blocking — see Warnings.
- [ ] **Dead code in Editor class** (from code-review). Three items declared/defined but unused:
  - `show_save_prompt_modal_` — declared in `editor.h` per contract Step 3 item 13, never read or written in `editor.cpp`.
  - `save_prompt_result_` — declared in `editor.h` per contract Step 3 item 13, never read or written in `editor.cpp`.
  - `handle_dirty_before_op()` — declared and defined per contract Step 3 item 12, never called. The dirtiness check is handled inline.
  The implementation is functionally correct (the save-prompt state machine uses `pending_op_` directly), but the unused members/method represent a gap between the contract's prescribed member list and the implementation's actual behavior.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] ADR-027 (Editor Architecture) — Compliant. Editor is a static library. No SDL3/OpenGL/GLM headers in `src/editor/`. ✅ Verified by code-review.
- [x] ADR-019 (Architecture Boundaries) — Compliant. No SDL3/GL/glm in `src/editor/`. ✅ Verified by code-review grep.
- [x] ADR-026 (ImGui Integration) — Compliant. ImGuiFileDialog uses same ImGui context. Docking branch used. ✅
- [x] ADR-014 (CLI App System) — Compliant. Editor uses `run_app()` via `EditorApp`. ✅
- [x] ADR-011 (Ownership/Nullability) — Empty file; no constraints to violate. ✅
- [ ] **ADR-029 (Editor UX Decisions) — NOT updated per spec requirement.** The F-01 spec's "Documentation to update" section (line 333) explicitly requires updating `docs/adr/ADR-029-editor-ux-decisions.md` to document the clean-by-default decision (Q-04: untitled scenes start clean, not dirty). This was not done. The wiki-agent noted "ADR updates are out of scope." The code-review flagged it as a warning. The ADR exists but lacks the amendment/note to reflect the grill-me decision. See Required governance updates.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/editor/scene-management.md` — ✅ Fully rewritten by wiki-agent to reflect F-01 implementation: clean-by-default, dirty state tracking, save-prompt state machine, error modals, ImGuiFileDialog, OS close button interception, Window::set_title API. Correctly references current implementation, not north-star future vision.
- [x] `docs/wiki/architecture/module-map.md` — ✅ Updated by wiki-agent with ImGuiFileDialog dependency, Window::set_title(), Platform::set_on_close_request(), F-01 MenuBar callbacks, Editor scene management methods.
- [x] `docs/wiki/architecture/overview.md` — ✅ Updated with F-01 key behaviors and editor capabilities.
- [x] `docs/wiki/editor/editor-panels.md` — ✅ Updated with F-01 File menu items status.

All four wiki pages correctly reflect the F-01 implementation state as of June 2026.

## Warnings

Non-blocking concerns for awareness:

- **ADR-029 not updated per spec requirement**: The F-01 spec's "Documentation to update" section (line 333) requires updating `docs/adr/ADR-029-editor-ux-decisions.md` to document the clean-by-default decision (Q-04: untitled scenes start clean, not dirty). The wiki-agent explicitly opted out ("ADR updates are out of scope"). This should be addressed as a follow-up: either amend ADR-029 with a note about the clean-by-default decision, or create a new ADR capturing the grill-me Q-04 decision. The lack of this ADR update does not affect implementation correctness, but it leaves a gap in the governance record.

- **UX spec AC-015 and Story 1 not corrected**: The F-01 spec's "Documentation to update" section (line 334) flags that `.specs/sprint-2026-06/editor-ux-design/spec.md` AC-015 (line 790) and Story 1 (line 589) still state dirty-by-default (`"Untitled*"`) instead of clean-by-default. The spec flagged this correctly, but the actual correction was not applied. The north-star UX spec remains contradictory with the implemented F-01 behavior. Should be corrected when the UX spec is next revised.

- **UT-02 window title test coverage incomplete**: Only 2 of 4 spec-defined window title scenarios are tested (untitled/clean and untitled/dirty). The titled/clean (`"scene.yaml — Buddd Editor"`) and titled/dirty (`"scene.yaml* — Buddd Editor"`) scenarios lack direct test assertions. `build_title_string()` is correct by code inspection and related tests (UT-06, UT-07) cover the file-path-tracking path, but direct coverage is missing.

- **Dead code in Editor class**: `show_save_prompt_modal_`, `save_prompt_result_` declared but never used; `handle_dirty_before_op()` defined but never called. The contract specifies these members but the implementation's save-prompt state machine uses `pending_op_` directly. The unused members do not affect correctness but should be cleaned up.

- **CMake FetchContent deprecation warning**: `FetchContent_Populate(ImGuiFileDialog)` is deprecated per CMake policy CMP0169. Future CMake versions may break the build. Should be migrated to `FetchContent_MakeAvailable()` or the modern pattern.

- **Manual `mark_dirty()` fragility** (acknowledged design trade-off): Dirty state relies on panels and commands calling `Editor::mark_dirty()`. Any future code path that modifies the scene without calling it will silently lose dirty tracking. Already flagged by spec-critic and contract-critic.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- **ADR-029**: Add an amendment or note documenting the clean-by-default decision for untitled scenes (Q-04 from F-01 grill-me). This ensures the decision is recorded at the ADR authority level.
- **UX spec** (`.specs/sprint-2026-06/editor-ux-design/spec.md`): Correct AC-015 (line 790) and Story 1 (line 589) to show `"Untitled"` (not `"Untitled*"`) for the initial state of a new scene, matching the implemented clean-by-default behavior.
