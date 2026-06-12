# Code Review — F-03 Entity Selection with Multi-Select

## Blocking issues

- [ ] None — all acceptance criteria, done criteria, and review checks are satisfied.

## Warnings

Non-blocking concerns for awareness:

- **Test temp file leak**: `Editor::open_scene() clears selection on success` test (line 467) creates a temp file via `mkstemp` (`/tmp/buddd_f03_XXXXXX`), then saves the test scene to `<name>.yaml`. The original `mkstemp`-created file (without `.yaml`) is never cleaned up. While `/tmp` is typically ephemeral, this leaves a small debris file on disk. Not a blocking issue but worth fixing: either use the mkstemp path directly as the save target, or clean up both files in the test teardown.

- **Spec documentation inaccuracy**: The spec's description of `snapshot()` says it returns "a `Selection` copy of the current state (including anchor)" — but `Selection` is a pure set-of-`EntityId`s value object; the anchor is stored separately in `EditorSelection` and is NOT included in the `Selection` object. The implementation correctly does not include anchor in `snapshot()`. This doc inaccuracy has no behavioral impact.

- **Anchor asymmetry (documented)**: `clear()` clears the anchor; `set_selection({})` (empty span) leaves the anchor unchanged. This is intentional per spec/contract and is correctly implemented, but may surprise future callers who expect `set_selection({})` to behave like `clear()`.

## Required changes

None.

## Suggested improvements

Optional ideas (not required):

- **Add a const overload of `Editor::selection() const -> EditorSelection const&`**: The spec-critic suggested this for const-correct callers. Not needed in F-03 (no const consumers yet), but it would prevent a future API break when const use sites emerge.

## Review summary

### Scope compliance ✅

All changes are limited to the five allowed files:
- `src/editor/editor_selection.h` — **created** (Selection, EditorSelection, SelectionModifier, std::hash<EntityId>)
- `src/editor/editor.h` — **modified** (include, selection_ member, selection() accessor)
- `src/editor/editor.cpp` — **modified** (selection() impl, new_scene/ open_scene clear, Ctrl+A shortcut)
- `src/editor/panels/scene_panel.h` — **modified** (click handling, highlighting, empty-area clear, collect_range/ collect_all)
- `tests/editor/f03_entity_selection_tests.cpp` — **created** (21 test cases, 123 assertions)

No files under `src/engine/`, no forbidden editor files (`editor_context.h`, `command.h`, `properties_panel.h`, etc.), and no `CMakeLists.txt` were touched.

### Spec compliance (32 ACs) ✅

| AC | Description | Status | Evidence |
|---|---|---|---|
| AC-01–07 | Selection value class (contains, size, empty, first, copy, add/remove/clear, ==, iteration) | ✅ | Unit tested |
| AC-08–16 | EditorSelection mutations (Replace, Toggle, clear, set_selection, snapshot, restore, anchor, callbacks) | ✅ | Unit tested |
| AC-17–19 | Editor integration (selection() accessor, new_scene clears, open_scene clears) | ✅ | Unit tested |
| AC-20–30 | ScenePanel click/Shift/Ctrl/Ctrl+A/empty-area/highlight/WantCaptureKeyboard | ✅ | Code review confirms implementation matches spec |
| AC-31 | All existing tests pass | ✅ | 558 tests, 22134 assertions — all pass |
| AC-32 | Zero new warnings | ✅ | Build produces zero warnings |

### Contract compliance (8 DCs) ✅

- **DC-01**: `editor_selection.h` — all required types, inline implementations, `EntityId::none()` guard, callback model, `noexcept`, `[[nodiscard]]`. ✅
- **DC-02**: `editor.h` — include, member, accessor. ✅
- **DC-03**: `editor.cpp` — accessor, new_scene/ open_scene clear, Ctrl+A with WantCaptureKeyboard gate and tree traversal. ✅
- **DC-04**: `scene_panel.h` — highlighting, click handling (plain/Ctrl/Shift), empty-area with modifier guard, collect_range/collect_all helpers. ✅
- **DC-05**: `f03_entity_selection_tests.cpp` — 21 test cases covering all specified ACs. ✅
- **DC-06**: Build with zero warnings. ✅
- **DC-07**: All existing tests pass. ✅
- **DC-08**: No changes to forbidden files. ✅

### Blocking issues from contract-critic ✅

- **B-01 (std::minmax dangling reference)**: RESOLVED. `collect_range` uses manual `(std::min)`/`(std::max)` on named `size_t` variables with an explanatory comment (lines 127–128 of scene_panel.h). ✅
- **B-02 (empty-area click modifier guard)**: RESOLVED. Empty-area click is guarded by `!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift` (lines 95–99 of scene_panel.h). ✅

### EntityId::none() guard ✅

- `EditorSelection::select()`: returns immediately if `id == EntityId::none()` (line 181 of editor_selection.h).
- `EditorSelection::set_selection()`: silently skips `EntityId::none()` entries (line 207 of editor_selection.h).
- Tested at lines 161–180 of f03_entity_selection_tests.cpp.

### Architecture boundary ✅

- `editor_selection.h` only includes `scene/entity_id.h` and standard library headers.
- No SDL3, OpenGL, or GLM headers in any `src/editor/` file changed or created.

### Build & tests ✅

- `cmake --build --preset debug`: succeeds with **zero warnings** from `src/editor/` or `tests/`.
- `./build/debug/tests/buddd_tests`: **558 test cases, 22134 assertions — all pass**.
- `./build/debug/tests/buddd_tests "[editor][selection]"`: **21 test cases, 123 assertions — all pass**.

### Verdict: Accepted
