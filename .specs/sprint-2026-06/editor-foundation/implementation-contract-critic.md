# Implementation Contract Review — Editor Foundation

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] *(none — no blocking issues found)*

## Warnings

Non-blocking concerns for awareness:

- **AC-030 grep does not include `src/cmd/apps/`**: The E2E verification table (line 1425) and Step 15 verification items (items 3–6) only run the SDL3/OpenGL/GLM grep on `src/editor/` and its subdirectories, but AC-030 requires "No SDL3, OpenGL, or GLM headers are included from any file under `src/editor/` **or `src/cmd/apps/`**." The grep should also cover `src/cmd/apps/`. Both AC-029 (src/editor/ only) and AC-030 (src/editor/ + src/cmd/apps/) are referenced together, which is insufficient. The contract should add `grep -rnE '#include.*(SDL3|GL/|glm/)' src/cmd/apps/` to Step 15's verification checklist.

- **AC-043 verified via code review instead of unit test**: The spec's AC-043 verification column says "Unit test: create ShortcutRegistry, call process() twice with same key held down; action fires only once." The contract's test table lists "Code review" for AC-043 (line 1398). The implementation does use `is_pressed()` which guarantees edge-triggered behavior, but the contract does not require a dedicated unit test for the "fires only once per press" contract. This is a minor deviation from the spec.

- **ADR-027 consequence staleness not fully acknowledged**: The contract's ADR impact section (line 1477) states "ADR-027 (Editor Architecture): This contract implements the editor architecture defined in ADR-027. No changes to the ADR." However, ADR-027 Decision 2 states "no changes to App base class or run_app()" which the contract contradicts by adding `App::update()` to the base class and `app.update(ctx)` to `run_app()`. The spec-critic already flagged this as an unresolved warning. The contract should at least acknowledge this is a backward-compatible extension beyond ADR-027's original scope, rather than stating "No changes to the ADR" without qualification.

- **Step 5 (QuitCommand) include discussion is confusing**: The step discusses forward-declaring `EngineContext` vs including `"engine_context.h"` — the comment (line 375–376) reads as an internal note to the spec author, not as clear implementation guidance. The final code correctly uses `#include "engine_context.h"`, but the surrounding text could confuse an implementer. Consider cleaning up the commentary.

- **Logging for command execution not explicitly mentioned in implementation steps**: The spec's Observability section (lines 721–726) requires logging for command execution (`BUDDD_LOG_DEBUG`), undo/redo, About dialog, and shortcut suppression (`BUDDD_LOG_TRACE`). The contract's `Editor::setup()` (Step 12) logs the ini file path at INFO level, but the implementation steps for `Editor::update()` and `draw_about_popup()` do not explicitly mention logging for command execution, undo/redo, or shortcut suppression. MenuBar's note (line 655) leaves logging to the implementer's choice. The logging patterns from the spec should be explicitly included in the implementation steps to ensure observability requirements are met. This is not blocking because the implementer can infer the logging from context, but it would be clearer to call it out.

## Required changes

None — the contract is complete and implementable. All spec goals and acceptance criteria are covered.

## Suggested improvements

Optional ideas (not required):

- **Consider adding a single `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/ src/cmd/apps/` command** in Step 15's verification checklist to cover both AC-029 and AC-030 in one pass, with zero expected matches.
- **Clarify the Step 5 include commentary** to simply say `#include "engine_context.h"` (the forward-declaration discussion is unnecessary — inline code needs the full definition).
- **Add explicit BUDDD_LOG_DEBUG calls** for command execution and undo/redo in the `Editor::update()` step, matching the Observability section of the spec.
- **Consider rewording the ADR impact section** for ADR-027 to read: "This contract extends the editor architecture defined in ADR-027 with a new `App::update()` method. ADR-027's Decision 2 ('no changes to App base class') is superseded for this feature in a backward-compatible way (default empty impl, no existing subclasses affected)."

## Coverage summary

| Criterion | Verdict |
|---|---|
| All spec goals (G-01 to G-09) covered | ✅ Pass |
| All 43 ACs have corresponding verification | ✅ Pass |
| Steps ordered logically with dependencies respected | ✅ Pass |
| Each step independently testable | ✅ Pass |
| EditorMenu/EditorPanel abstractions properly implemented | ✅ Pass |
| ShortcutRegistry properly covered | ✅ Pass |
| MenuBar callback pattern correct (set_on_about) | ✅ Pass |
| No SDL3/OpenGL/GLM leaks (ADR-019) | ✅ Pass (minor: src/cmd/apps/ grep missing) |
| Architecture boundary respected | ✅ Pass |
| Headless safety for tests | ✅ Pass |
| File changes match between spec and contract | ✅ Pass |
| Spec-critic warnings addressed or acknowledged | ✅ Pass (ADR-027 staleness remains unaddressed) |

## Re-review summary

This is the first review of the implementation contract. The contract faithfully implements all spec requirements with clear, sequential implementation steps. No blocking issues found. Five warnings are flagged: (1) AC-030 grep missing `src/cmd/apps/`, (2) AC-043 uses code review instead of unit test, (3) ADR-027 consequence staleness not acknowledged, (4) Step 5 include commentary is messy, (5) Observability logging from spec not explicitly called out in steps. These are all minor and should not block the workflow.
