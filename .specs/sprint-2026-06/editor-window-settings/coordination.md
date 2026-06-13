# Workflow Coordination: editor-window-settings

## Orchestrator

**Feature**: `editor-window-settings`
**Status**: completed
**Current step**: completed
**Notes**:

### Decision Log (from Grill-me)

- **Minimum valid size**: 400x300 (if saved w < 400 or h < 300, use defaults).
- **Position validation**: Use SDL3 multi-display API — window must be at least partially visible on at least one connected display.
- **Settings key convention**: `editor.window.*` (e.g. `editor.window.x`, `editor.window.y`, `editor.window.width`, `editor.window.height`, `editor.window.state`).
- **Window state type**: String (`"normal"`, `"maximized"`, `"minimized"`).
- **Minimized on startup**: Force state to `"normal"` but still use saved position/size if they validate. Do NOT discard valid position/size just because state was minimized.
- **Save timing**: On `Editor::shutdown()` (which already calls `save_all()`). No mid-session saves.
- **Load timing**: After `Editor::setup()` loads settings via `load_all()`, apply validated settings to the window (two-phase: window created with defaults, then repositioned/resized/restored).
- **Architecture**: Add `WindowState` enum, `position()`/`set_position()`, `state()`/`set_state()` to `Window` abstract class. Add display query methods to `Platform`. No SDL3 in Editor.

**Initial instructions**: Conserver dans user_project les settings de taille, position et state de l'éditeur. Au démarrage, utiliser ces settings après validation (taille trop petite → defaults, position hors écran → defaults, state minimized → normal). À la fermeture, sauvegarder les settings courants.

### Loop history
- **2026-06-13**: Loop #1 — implementation-contract-critic found a blocking issue: integration test file `tests/editor/settings_integration_tests.cpp` not in "Files allowed to change" list. Looping back to implementation-contract-author to fix.
- **2026-06-13**: Loop #2 — code review feedback: save should not update position/size when window is maximized or minimized (to preserve restore geometry). Fixed in editor.cpp. Also fixed pre-existing test failure in cli_app_tests.cpp. Re-running code-reviewer.

## spec-author

**Status**: completed
**Summary**:
Created SPEC-037 — Editor Window Geometry Persistence spec.
Covers persistence of window position/size/state to user_project_settings tier.
Defines new Window API (position, set_position, state, set_state, resize) and Platform API (display_count, display_bounds).
Includes validation algorithm (minimum 400×300 size, at-least-partially-visible position, minimized→normal on startup).
Defines 23 acceptance criteria, 6 success criteria, comprehensive edge/error cases, and E2E verification plan.
**Artifacts**:
- `.specs/sprint-2026-06/editor-window-settings/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Reviewed SPEC-037 — Editor Window Geometry Persistence.
All 12 Definition of Ready criteria satisfied.
No blocking issues found. Spec is accepted.
See warnings and suggestions in spec-critic.md.
**Artifacts**:
- `.specs/sprint-2026-06/editor-window-settings/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- Save algorithm pseudocode includes `save_all()` inside the if block, but execution order separates it as existing code step 3 — reconcile to avoid double-save confusion.
- `resize()` vs `on_resize()` naming distinction not explicitly documented — could confuse implementers.
- AC-016 verification (verify `set_position()` is NOT called) requires mock/spy pattern; consider simpler position-value check.
- `resize()` returns void (errors logged internally), but this design choice is not called out explicitly.
- `noexcept` not specified on new methods; existing Window/Platform methods use `noexcept`.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Loop-back fix: resolved the blocking issue by adding `tests/editor/settings_integration_tests.cpp` to the "Files allowed to change" list (item 13). Also addressed all non-blocking warnings: added out-of-range display_bounds tests (-1 and 999), added offscreen SDL3 resize immediate-cache-update test (AC-018 for WindowSDL3), and added SC-00X traceability column to all test case tables. Total test coverage: 5 SDL3 offscreen tests + 15 headless/unit tests + 3 editor headless test categories with SC traceability.
**Artifacts**:
- `.specs/sprint-2026-06/editor-window-settings/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review (loop #2) — blocking issue resolved. The implementation-contract-author added `tests/editor/settings_integration_tests.cpp` to the "Files allowed to change" list (item 13). Additionally addressed non-blocking warnings: out-of-range display_bounds tests, offscreen SDL3 resize immediate-cache-update test, and SC-00X traceability columns. No new issues found. Contract accepted.
**Artifacts**:
- `.specs/sprint-2026-06/editor-window-settings/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-13 15:21:47
**Warnings**:
<none>
**Notes**: Human confirmed the test plan (17 test cases across 3 new files + integration test extension) and approved implementation.

## code-implementer

**Status**: completed
**Summary**:
Implemented Editor Window Geometry Persistence (SPEC-037 / IMPL-037) across engine and editor layers:
- Added WindowState enum, WindowPosition struct, 5 new pure virtual methods to Window abstract class (position, set_position, state, set_state, resize) with SDL3 and Headless implementations
- Added DisplayBounds struct, 2 new pure virtual methods to Platform (display_count, display_bounds) with SDL3 and Headless implementations
- Created window_utils.h/.cpp with state↔string conversion helpers
- Integrated load/validate/save logic in Editor::setup() and Editor::shutdown() with log messages
- All 654 tests pass (1 pre-existing failure unrelated to this feature)
**Artifacts**:
- Modified: src/engine/window/window.h, src/engine/window/window_sdl3.h, src/engine/window/window_sdl3.cpp, src/engine/window/window_headless.h, src/engine/window/window_headless.cpp
- Modified: src/engine/platform/platform.h, src/engine/platform/platform_sdl3.h, src/engine/platform/platform_sdl3.cpp, src/engine/platform/platform_headless.h, src/engine/platform/platform_headless.cpp
- Modified: src/editor/editor.cpp, tests/editor/settings_integration_tests.cpp
- Created: src/engine/window/window_utils.h, src/engine/window/window_utils.cpp
- Created: tests/engine/window_state_tests.cpp, tests/engine/platform_display_tests.cpp, tests/editor/window_settings_tests.cpp
**Questions for human**:
none
**Warnings**:
- Pre-existing test failure in tests/cmd/cli_app_tests.cpp:113 ("buddd edit --frame 2 opens editor and exits") — not related to this feature, was already failing before implementation
- Visual verification skipped: feature has no rendered/visual output — window geometry persistence is non-visual
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**: Re-review (Loop #3) completed. Final fix replaces the "skip save when maximized" approach with a geometry cache: Editor tracks last-known Normal position/size via update() and always saves the cached values on shutdown. This ensures correct restore geometry even when the user always quits while maximized. Pre-existing cli_app_tests.cpp test also fixed. All 656 tests pass, zero warnings. No blocking issues.
**Artifacts**:
- `.specs/sprint-2026-06/editor-window-settings/code-review.md`
**Questions for human**:
none
**Warnings**:
- `tests/cmd/cli_app_tests.cpp` is not in the implementation contract's "Files allowed to change" list, but the change was explicitly directed by the orchestrator to fix a pre-existing test failure. This is a reasonable exception.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated the operational wiki to reflect the Editor Window Geometry Persistence feature (SPEC-037). Edited settings-system.md with the first-consumer section describing `editor.window.*` keys, window geometry save/load lifecycle, and validation algorithm. Updated architecture/overview.md directory listing to mention WindowState, WindowPosition, DisplayBounds, and new APIs. Updated module-map.md platform and window sections with detailed new method descriptions including window_utils.h/.cpp.
**Artifacts**:
- `docs/wiki/editor/settings-system.md` (modified)
- `docs/wiki/architecture/overview.md` (modified)
- `docs/wiki/architecture/module-map.md` (modified)
**Changes made**:
- `settings-system.md`: Added `(window geometry)` to user_project_settings scope; expanded Editor lifecycle table with window geometry save/load phases; added "First consumer: Editor Window Geometry" section with settings keys table, validation/save/geometry-tracking descriptions, and associated API summary.
- `architecture/overview.md`: Updated `platform/` and `window/` directory entries to mention WindowState, WindowPosition, DisplayBounds, window_utils, and new position/state/resize/display APIs.
- `architecture/module-map.md`: Updated Platform submodule table to document DisplayBounds, display_count(), display_bounds(); updated Window submodule table to document WindowState, WindowPosition, all five new virtual methods, and window_utils.h/.cpp.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance validation completed for SPEC-037 / IMPL-037.
All 7 relevant ADRs (ADR-019, ADR-027, ADR-029, ADR-016, ADR-009, ADR-020, ADR-012) are respected.
SPEC-036 (settings system) is used correctly as the first consumer.
Wiki updated correctly. No blocking issues. Verdict: Accepted.
**Artifacts**:
- `.specs/sprint-2026-06/editor-window-settings/governance-review.md`
**Questions for human**:
none
**Warnings**:
- `tests/cmd/cli_app_tests.cpp` was modified per orchestrator directive but was not in the implementation contract's allowed list — reasonable exception.
- `WindowSDL3::resize()` does not log on `SDL_SetWindowSize` failure as suggested by spec's error cases (implementation contract didn't require it).
- Headless editor tests don't exercise the `Editor::setup()` validation block (known architectural limitation — tested via SDL3 offscreen integration tests).
- `[math]` tag copy-paste artifact in `window_state_tests.cpp` test cases.
**Blocking issues**:
none
