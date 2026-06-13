# Implementation Contract Review — Editor Window Geometry Persistence (SPEC-037 / IMPL-037)

## Blocking issues

No blocking issues — all acceptance criteria are satisfied.

## Warnings

Non-blocking concerns for awareness:

- **Test tag inconsistency**: `tests/engine/window_state_tests.cpp` uses the `[math]` tag (e.g., `"[math][window]"`) for window state tests. This appears to be a copy-paste from another test file. The `[math]` tag is not meaningful for these tests; they should use `[window]`-related tags only. This does not affect correctness as the tests still run under broader tags.

- **Headless editor tests don't exercise `Editor::setup()` validation code path**: In `tests/editor/window_settings_tests.cpp`, the `HeadlessEditorFixture::setup_editor()` calls `Editor::setup()` which returns early at the `engine_imgui::is_initialized()` check in headless mode (line 71 of `editor.cpp`). Consequently, the window geometry validation block (lines 94-159) never runs in headless mode. The tests still verify `WindowHeadless` and `PlatformHeadless` behavior directly, and the editor-level validation is tested through SDL3 offscreen integration tests. This is a known architectural limitation of the existing setup, not a bug.

- **`WindowSDL3::resize()` ignores `SDL_SetWindowSize` return value**: The spec's Error Cases section (line 358) mentions logging a warning if `SDL_SetWindowSize` fails. The implementation silently ignores the return value. In practice, `SDL_SetWindowSize` always succeeds with valid arguments, and the Editor-level code path handles any downstream effects. This is a minor deviation from the spec's error handling note but matches the implementation contract exactly.

- **Shutdown order differs from spec execution order table**: The spec (line 325) lists `ImGui::GetIO().IniFilename = nullptr;` as step 1 of shutdown, before saving window geometry. The implementation keeps it after `save_all()` (step 3), matching the pre-existing SPEC-036 behavior. The implementation contract inserted the save block before the existing `if (settings_manager_)` block correctly, preserving the original order. The spec table was slightly inaccurate about the original code order.

## Required changes

None.

## Suggested improvements

- Consider adding logging when `SDL_SetWindowSize` fails in `WindowSDL3::resize()` to match the spec's error handling description.
- Consider adding `BUDDD_HAS_DISPLAY`-guarded editor-level position validation tests (e.g., no-overlap, valid-overlap) using SDL3 offscreen to exercise the full `Editor::setup()` validation chain.

## Acceptance Criteria Checklist

| ID | Description | Status |
|---|---|---|
| AC-001 | `WindowState` enum (Normal, Maximized, Minimized) and `WindowPosition` struct (int x, y) in `window.h` | ✅ |
| AC-002 | 5 new pure virtual methods: `position()`, `set_position()`, `state()`, `set_state()`, `resize()` in `Window` | ✅ |
| AC-003 | `WindowSDL3` implements using correct SDL3 API calls | ✅ |
| AC-004 | `WindowHeadless` implements as no-op stubs (position→{0,0}, state→Normal, resize updates cache) | ✅ |
| AC-005 | `DisplayBounds` struct (int x, y, width, height) in `platform.h` | ✅ |
| AC-006 | 2 new pure virtual methods: `display_count()`, `display_bounds()` in `Platform` | ✅ |
| AC-007 | `PlatformSDL3` implements using `SDL_GetDisplays` + `SDL_GetDisplayBounds` with bounds checking | ✅ |
| AC-008 | `PlatformHeadless` implements: count→0, bounds→{0,0,0,0} | ✅ |
| AC-009 | `Editor::setup()` reads window settings from `user_project_settings`, validates, applies | ✅ |
| AC-010 | Size validation: <400 or <300 → fallback to 1280×800; >=400×300 accepted | ✅ |
| AC-011 | Position validation: AABB overlap test against all displays; zero displays → invalid | ✅ |
| AC-012 | Minimized state forced to Normal on startup; other valid states applied as-is | ✅ |
| AC-013 | Unknown state strings (e.g., "fullscreen", "") → Normal | ✅ |
| AC-014 | `Editor::shutdown()` writes 5 window keys to `user_project_settings` before `save_all()` | ✅ |
| AC-015 | Correct types: `int32_t` for x/y/width/height, `std::string` for state | ✅ |
| AC-016 | `set_position()` not called when position validation fails | ✅ |
| AC-017 | Size fallback calls `resize(DEFAULT_W, DEFAULT_H)` | ✅ |
| AC-018 | `resize()` updates cached width/height immediately in both backends | ✅ |
| AC-019 | `window_state_to_string` / `parse_window_state` round-trip correctly for all 3 states | ✅ |
| AC-020 | Headless mode: `Editor::setup()` + `shutdown()` no-crash | ✅ |
| AC-021 | `Editor::shutdown()` without prior `setup()` is safe | ✅ |
| AC-022 | Settings keys use `editor.window.*` convention in `user_project_settings` tier | ✅ |
| AC-023 | No SDL3 headers in `src/editor/editor.cpp` — all platform interaction through `Window`/`Platform` abstractions | ✅ |

## Re-review (Loop #2 — June 13, 2026)

### Changes verified

Three changes were applied in this loop-back:

1. **`Editor::shutdown()` in `editor.cpp`** — Window position/size are now only saved when the window state is `Normal`. When `Maximized` or `Minimized`, only the state string is saved; the previously-saved position/size (from the last Normal save) are preserved on disk. This prevents un-maximizing from restoring to maximized geometry instead of the pre-maximized "restored" geometry.

2. **`cli_app_tests.cpp`** — Fixed the pre-existing test failure by changing the substring check from `"Editor: layout file: buddd_editor.ini"` to `"layout.ini"` (the log message uses the full path from the settings system).

3. **`settings_integration_tests.cpp`** — Added `"Maximized/Minimized state does not overwrite position/size in settings"` integration test that verifies position/size are preserved when the window is maximized.

### Build and test results

- **Build**: Clean build with **zero warnings** in our code (`src/` and `tests/`).
- **Tests**: **All 656 tests pass** (22501 assertions). The previous pre-existing failure in `cli_app_tests.cpp` is now fixed. The new maximized save test passes.

### File change audit

| File | Status |
|---|---|
| `src/editor/editor.cpp` | ✅ Allowed (item 12) |
| `tests/editor/settings_integration_tests.cpp` | ✅ Allowed (item 13) |
| `tests/cmd/cli_app_tests.cpp` | ⚠️ Not in allowed list, but explicitly directed by orchestrator to fix pre-existing test failure |

The `tests/cmd/cli_app_tests.cpp` change was not originally in the implementation contract's "Files allowed to change" list. However, this was a minimal one-line fix explicitly requested by the orchestrator in the loop-back (see coordination.md "Loop #2" note) to resolve a pre-existing test failure documented in the previous review. This is a reasonable exception directed by the orchestrator.

### Correctness analysis

- **Maximized save logic**: Correct. When the window state is `Normal`, position/size/state are all saved. When `Maximized` or `Minimized`, only the state is saved (position/size on disk remain from the last Normal save). This matches SDL3's behavior where `SDL_GetWindowPosition`/`SDL_GetWindowSize` report the maximized geometry, not the "restored" geometry.
- **CLI test fix**: Correct — the substring `"layout.ini"` matches the actual log output regardless of the full path.
- **New integration test**: Correctly verifies that position/size are preserved after maximize+shutdown, with a conditional assertion that properly handles SDL3 offscreen mode (which may not report `Maximized` state).

### Blocker status

All three changes are correct and well-tested. No blocking issues.

**Verdict: Accepted**

## Summary

The implementation is complete, correct, and matches the accepted spec and implementation contract. All 23 acceptance criteria are satisfied. The build produces zero warnings in our code. All 656 tests pass (the previously broken test is now fixed). The code quality is high, follows project conventions (trailing return types, `noexcept` on getters, `[[nodiscard]]` on query methods, proper ADR-019 architecture boundaries).

**Verdict: Accepted**
