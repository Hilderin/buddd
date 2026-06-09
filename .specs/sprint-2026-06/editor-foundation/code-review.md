# Implementation Code Review — Editor Foundation

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] **11 warnings from `[[nodiscard]]` return values being ignored in our code**: The build produces 11 warnings in `src/` and `tests/` about ignoring the `[[nodiscard]]` return value of `CommandStack::undo()` and `CommandStack::redo()`. Per review rules, "the build produces zero warnings in our code (`src/` and `tests/`)" is a hard requirement. All 14 existing demo scenes compile cleanly, but the new code does not:

  | File | Line | Warning |
  |---|---|---|
  | `src/editor/panels/menu_bar.h` | 44 | `undo()` return ignored |
  | `src/editor/panels/menu_bar.h` | 47 | `redo()` return ignored |
  | `src/editor/editor.cpp` | 71 | `undo()` return ignored (Ctrl+Z handler) |
  | `src/editor/editor.cpp` | 74 | `redo()` return ignored (Ctrl+Shift+Z handler) |
  | `src/editor/editor.cpp` | 77 | `redo()` return ignored (Ctrl+Y handler) |
  | `tests/editor_tests.cpp` | 79, 98, 100, 109, 127, 142 | `undo()` return ignored |

  **Root cause**: `CommandStack::undo()` and `CommandStack::redo()` are declared with `[[nodiscard]]`, but several call sites intentionally ignore the return value because the menu/button enabled state already gates the call. This is by design (the implementer notes this), but the warnings violate the zero-warnings policy.

  **Fix options**:
  1. Cast to `(void)` at call sites (e.g., `(void)command_stack_.undo();`) — preserves `[[nodiscard]]` for other callers.
  2. Remove `[[nodiscard]]` from `undo()` and `redo()` since they are action methods, not query methods (per ADR-011 convention about `[[nodiscard]]` on `Result<T>` and boolean queries only).
  3. Use `std::ignore = command_stack_.undo();`

  **Recommended**: Option 1 (cast to `(void)`) — minimal change, keeps the attribute for potential future callers who genuinely need the result.

## Warnings

Non-blocking concerns for awareness:

- **Observability logging not implemented**: The spec's Observability section (spec.md lines 721-726) requires `BUDDD_LOG_DEBUG` for command execution, undo/redo, and About dialog, and `BUDDD_LOG_TRACE` for shortcut suppression. The implementation only logs the ini file path (INFO level). No command/undo/redo/shortcut logging is present. This reduces debuggability but does not affect correctness.

- **`#include <imgui_internal.h>` in editor.cpp**: Required for `ImGuiDockNode*` access in the default layout check. While functional, it depends on an ImGui internal header. Consider using `ImGui::DockBuilderGetNode()` return value more indirectly if stability is a concern. Acceptable for v1.

- **`(void)input;` in editor.cpp:65**: The `input` variable is fetched but only used in a comment. The reference is not actually consumed (shortcut lambdas capture `ctx` directly, and `process()` receives `input` as a parameter at runtime). Minor code smell.

- **ADR-027 staleness**: The contract acknowledges this extends ADR-027 with `App::update()` in backward-compatible way but does not formally flag it. Not a code issue.

## Required changes

1. **Fix all 11 `[[nodiscard]]` warnings** — either cast to `(void)` or remove `[[nodiscard]]` from `undo()`/`redo()`. After fix, verify `cmake --build --preset debug` produces zero warnings from `src/` and `tests/`.

## Suggested improvements

Optional ideas (not required):

- **Add observability logging**: Add `BUDDD_LOG_DEBUG("Editor: executing command: {}", cmd->name())` in `CommandStack::execute()`, `BUDDD_LOG_DEBUG("Editor: undo '{}'", name)` in `undo()`, and `BUDDD_LOG_DEBUG("Editor: redo '{}'", name)` in `redo()`. Add `BUDDD_LOG_TRACE` for shortcut suppression in `ShortcutRegistry::process()`. Matches spec Observability section.

- **Add unit test for AC-043 (edge-triggered shortcut)**: The spec requires "Unit test: create ShortcutRegistry, call process() twice with same key held down; action fires only once." Currently verified only by code review (`is_pressed()` used). Adding a dedicated test would strengthen coverage.

- **Minor code cleanup**: Remove the unused `(void)input;` pattern in `editor.cpp` setup — the `input` variable is not needed (it's fetched inside `update()` when `process()` is called).

## Coverage summary

| Criterion | Verdict |
|---|---|
| All 15 new files created as specified | ✅ Pass |
| All 7 modified files changed as specified | ✅ Pass |
| Forbidden files untouched | ✅ Pass |
| Architecture boundary (ADR-019): no SDL3/GL/glm in src/editor/ or src/cmd/apps/ | ✅ Pass |
| Namespace: `buddd::editor` for all editor code | ✅ Pass |
| Command/CommandStack implementation matches spec | ✅ Pass |
| ShortcutRegistry: bind/process, WantCaptureKeyboard gate, is_pressed() edge-triggered | ✅ Pass |
| EditorMenu/EditorPanel abstract classes | ✅ Pass |
| MenuBar: CommandStack&, set_on_about(), File/Edit/Help menus | ✅ Pass |
| 5 concrete panels with 100x100 min size | ✅ Pass |
| Editor::update()/draw_ui() two-phase split, 4-phase rendering | ✅ Pass |
| App::update() virtual method + EditorApp override | ✅ Pass |
| DockBuilder default layout for 5 panels | ✅ Pass |
| All 438 tests pass (21479 assertions) | ✅ Pass |
| Full build succeeds | ❌ Blocked (11 warnings) |
