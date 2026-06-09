# Spec Review — Editor Foundation (Re-review: 08-Jun-2026 — EditorMenu/EditorPanel refactor)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] *(none — no blocking issues found)*

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- [x] **Data flow diagram ordering** — **RESOLVED**. The tree diagram in the "Two-phase update/render separation" section is clear and unambiguous.

- [ ] **NG-05 contradicts `run_app()` modification**: NG-05 states "No other changes to `run_app()` or engine core" but the spec explicitly modifies `src/cmd/app.cpp` to add `app.update(ctx)` in the render loop. G-09, AC-034, and the "Modified" file table all confirm this change. NG-05's wording should acknowledge the `app.cpp` modification (e.g., "(3) one call to `app.update(ctx)` added to `run_app()` in `src/cmd/app.cpp`").

- [ ] **File changes table misclassifies EditorApp**: The "Unchanged" table lists `src/cmd/apps/editor_app.h/.cpp` but the "Reason" column says "EditorApp overrides `update()` to call `editor_->update(ctx)`." If EditorApp overrides `update()`, the files are **modified**, not unchanged. This entry should be moved to the "Modified" table.

- [ ] **Line 65: "Disabled when no command is available" for File > Quit**: `QuitCommand` is always executable (unlike Undo/Redo which depend on stack state). The phrase "Disabled when no command is available to execute it" appears to be a copy-paste from the Undo/Redo descriptions. Quit should always be enabled.

- [ ] **AC-024 awkward parenthetical (unchanged)**: "verify command is undone (re-enables disabled criterion can be checked via menu state)" — the parenthetical is grammatically awkward. Intent remains clear.

- [ ] **Panel close button / per-panel state (unchanged)**: The spec says panels have a close button that hides them "for the current session" and that "on next launch they reappear." ImGui manages this via `ImGui::Begin()` behavior, but the mechanism (`p_open` parameter, or no parameter = no close button) is not explicitly stated. For speculative panels, `ImGui::Begin("Scene")` (without `p_open`) means no close button by default. If close buttons are desired, `ImGui::Begin("Scene", &p_open)` is needed. The spec should clarify whether the close button is intentionally included (and how the `bool` is managed).

- [ ] **QuitCommand undo is no-op (unchanged)**: `QuitCommand::undo()` is a no-op since the exit flag cannot be cleared. After executing Quit, the undo stack shows "Undo Quit" as enabled, but clicking it does nothing. Acceptable for v1 — documented in error cases.

- [ ] **ADR-027 Decision 2 consequence now outdated (unchanged)**: ADR-027 states "No changes to `run_app()` or the `App` base class are needed" and "the App base class and `run_app()` are unchanged." The spec now adds a virtual `App::update()` method and modifies `run_app()` to call it. This is a backward-compatible extension and does not violate ADR-027's core decision (editor reuses the App lifecycle), but the stated consequence is stale. Recommend: (a) amendment note to ADR-027, or (b) explicit acknowledgment in the spec as an evolution of ADR-014's render loop.

- [ ] **EditorApp's `update()` vs. `on_frame_begin()` overlap (unchanged)**: Both are per-frame hooks called each frame. `update()` runs after `update_updatables()` (editor logic after game logic). `on_frame_begin()` runs before `update_updatables()`. The ordering rationale is documented in the data flow diagram but not explicitly justified.

## Required changes

None.

## Suggested improvements

Optional ideas (not required):

- Clarify how `Editor::update()` accesses the `InputSystem` for shortcut checks (via the navigable object graph: `ctx.services.platform().input_system()`).
- Add a brief note about the `io.IniFilename` pointer lifetime: the string literal `"buddd_editor.ini"` has static storage duration, so the pointer remains valid for ImGui's entire lifetime.
- Consider adding an exit-request check after `app.update(ctx)` in `run_app()` (matching the existing pattern used after `on_frame_begin()` and `update_updatables()`). Currently, the Editor may render one extra frame after a Quit command before poll_events detects the exit.
- Add a brief "Consequences for ADR-027" subsection to acknowledge that the spec extends the `App` lifecycle (new `update()` method), which supersedes ADR-027 Decision 2's "no changes to App base class" statement in a backward-compatible way.

---

## Re-review summary

The spec was refactored from monolithic `Editor::draw_ui()` with per-panel helper methods (draw_scene_panel, etc.) to a clean abstraction-based architecture:

**What changed:**
1. `EditorMenu` abstract class — overlay elements drawn before dockspace (id, update, draw_ui)
2. `EditorPanel` abstract class — dockable panels drawn inside dockspace (id, title, update, draw_ui)
3. `MenuBar` → concrete `EditorMenu` subclass
4. `ScenePanel`, `PropertiesPanel`, `ConsolePanel`, `ProjectPanel`, `AssetsPanel` → concrete `EditorPanel` subclasses
5. `Editor` has `menus_` and `panels_` vectors + `add_menu()` / `add_panel()` registration
6. 4-phase rendering: menus → dockspace → panels → about popup
7. NG-04 updated: panels registered via `add_panel()` in `setup()`, no dynamic discovery
8. AC-011 through AC-015, AC-038, AC-039 added for the new abstractions

**Consistency checks:**
- ✅ **EditorMenu/EditorPanel abstractions well-defined**: Both have clear interfaces (`id()`, `update()`, `draw_ui()`, plus `title()` for panels). Lifecycle is documented. Ownership via `unique_ptr` is explicit.
- ✅ **4-phase rendering flow**: Phase 1 (menus before dockspace) → Phase 2 (DockSpaceOverViewport) → Phase 3 (panels inside dockspace) → Phase 4 (About popup). Tree diagram is clear.
- ✅ **Registration pattern**: `add_menu()` / `add_panel()` take `unique_ptr` and transfer ownership. No dynamic discovery. Consistent with NG-04.
- ✅ **AC numbering is consistent**: 39 ACs (AC-001 through AC-039), new ones appended at the end. No gaps or overlaps.
- ✅ **Command system unchanged**: Command/CommandStack/QuitCommand/ShowAboutCommand are untouched by the refactor. All existing ACs (AC-001 to AC-010) remain valid.
- ✅ **`App::update()` lifecycle unchanged**: G-09, AC-033–037 are preserved from the previous update.
- ✅ **Architecture boundary preserved**: AC-028/AC-029 enforce zero SDL3/OpenGL/GLM headers in `src/editor/`. No new engine dependencies introduced.
- ✅ **Headless safety preserved**: AC-031/032 guard ImGui-dependent code. CommandStack tests are independent of ImGui.
- ✅ **Backward compatibility**: 14 existing App subclasses unaffected (default empty `update()`). AC-037 explicitly verifies this.
- ✅ **Testability**: All ACs have explicit verification methods (inspect, unit test, manual, build check).

**Non-blocking issues found:**
- NG-05 contradicts the `run_app()` modification
- File changes table misclassifies EditorApp as "Unchanged"
- "Disabled when no command is available" for Quit is misleading
- Three pre-existing warnings remain unresolved (panel close button, AC-024 phrasing, ADR-027 staleness)

**Overall**: The refactored spec is clear, implementable, and internally consistent modulo the minor contradictions noted in warnings. The EditorMenu/EditorPanel abstractions are well-defined and the registration pattern is explicit. No blocking issues.

## Definition of Ready assessment

### Clarity & Completeness

| Criterion | Verdict |
|---|---|
| Scope is clearly defined (what is included and what is explicitly excluded) | ✅ Pass |
| Dependencies on other features, modules, or external systems are identified | ✅ Pass |
| Edge cases and error conditions are described | ✅ Pass |
| The expected behavior is unambiguous and testable | ✅ Pass |

**Notes**: Scope refactored with EditorMenu/EditorPanel abstractions but remains well-bounded. 10 non-goals still clear. Minor textual ambiguities noted in warnings.

### Verification

| Criterion | Verdict |
|---|---|
| The spec defines how the feature will be verified end-to-end | ✅ Pass |
| Acceptance criteria are specific, measurable, and verifiable | ✅ Pass |
| Success and failure states are described | ✅ Pass |

**Notes**: 39 acceptance criteria with explicit verification methods. Manual + headless test strategies documented.

### Documentation

| Criterion | Verdict |
|---|---|
| Interface changes (CLI flags, API signatures, config keys) are documented | ✅ Pass |
| Existing documentation that must be updated is listed | ✅ Pass |

**Notes**: Full API signatures for all new classes. Three wiki files listed as modified.

### Technical

| Criterion | Verdict |
|---|---|
| Technical constraints are identified (system APIs, libraries, build changes) | ✅ Pass |
| Risks or unknowns are surfaced | ✅ Pass |
| Performance or resource implications, if any, are noted | ✅ Pass |

**Notes**: New virtual methods in EditorMenu/EditorPanel add negligible overhead. No new engine dependencies.

### Overall verdict

**PASS** — All Definition of Ready criteria are satisfied. No blocking issues found. The refactored spec is ready for implementation-contract authoring. The three new warnings (NG-05 contradiction, EditorApp misclassification, Quit disabled text) should be resolved before final sign-off.
