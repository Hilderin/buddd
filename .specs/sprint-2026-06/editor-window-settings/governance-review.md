# Governance Review — Editor Window Geometry Persistence (SPEC-037 / IMPL-037)

**Overall verdict: Accepted** ✅

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec vs Contract — `save_all()` call**: The spec's save algorithm pseudocode (line 301) included `save_all()` inside the save block. The spec-critic flagged this as a double-call risk. The contract explicitly notes that `save_all()` must NOT be called inside the save block — it is called by the existing code that follows. The implementation correctly calls `save_all()` only once in the existing `if (settings_manager_)` block after the save block. Resolved. ✅
- [x] **Contract vs Code — Maximized save logic**: The original implementation (loop #1) always saved the current window position/size regardless of state. The code-reviewer (loop #2) identified that saving maximized geometry would restore the wrong "restored" position/size. The solution evolved through two loops:
  - Loop #2: Skip position/size save when maximized or minimized (only save state).
  - Loop #3: Track last-known Normal geometry in `Editor::update()` and always save cached Normal values on shutdown.
  The final implementation (loop #3) correctly caches Normal geometry and always saves cached values, ensuring correct restore geometry even when the user always quits maximized. ✅
- [x] **Spec execution order vs Code — Shutdown step order**: The spec (line 325) lists `ImGui::GetIO().IniFilename = nullptr;` as step 1 of shutdown, before window geometry save. The implementation keeps it after `save_all()` (step 3), matching the pre-existing SPEC-036 behavior. The implementation contract correctly inserted the save block before the existing `if (settings_manager_)` block, preserving original order. Minor spec table inaccuracy, no functional issue. ✅
- [x] **Contract vs Code — `cli_app_tests.cpp` change**: The implementation contract's "Files forbidden to change" did not include `tests/cmd/cli_app_tests.cpp`, but the orchestrator explicitly directed this minimal one-line fix to resolve a pre-existing test failure. Reasonable orchestrator-directed exception. ✅
- [x] **Spec error handling vs Code — `SDL_SetWindowSize` return value**: The spec's Error Cases section (line 358) mentions logging a warning if `SDL_SetWindowSize` fails. The implementation silently ignores the return value. The implementation contract did not require error handling for this case. Minor spec/code discrepancy but intentionally designed per the contract. ✅

## ADR alignment

- [x] **ADR-019 (Architecture Boundaries)** — ✅ No SDL3, OpenGL, or GLM headers in `src/editor/`. All platform interaction goes through abstract `Window` and `Platform` interfaces. Verified: `grep -rn '#include.*SDL' src/editor/` returns zero matches. The narrow test exception (AMEND-2026-001) is properly followed in `tests/` files under `#ifdef BUDDD_HAS_DISPLAY`.
- [x] **ADR-027 (Editor Architecture)** — ✅ The Editor class follows the established pattern: separate static library (`buddd_editor`), direct member variables (not PIMPL), no SDL3 headers in `src/editor/`, lifecycle via `EditorApp` subclass of `App`. The cached geometry fields added to `editor.h` are consistent with ADR-027 Decision 4 (direct member variables).
- [x] **ADR-029 (Editor UX Decisions)** — ✅ No contradictions. Window geometry persistence is orthogonal to the UX model (tabs, panels, play mode). The feature is a transparent infrastructure feature with no visible UI, consistent with ADR-029's scope.
- [x] **ADR-016 (yaml-cpp kept private)** — ✅ yaml-cpp remains a PRIVATE dependency of `buddd_engine`. No yaml-cpp headers are exposed in `src/editor/` or any public engine headers. The settings system uses the pimpl pattern in `SettingsStore` to keep YAML types internal.
- [x] **ADR-009 (Test File Naming Convention)** — ✅ All new test files use the `_tests.cpp` suffix: `window_state_tests.cpp`, `platform_display_tests.cpp`, `window_settings_tests.cpp`.
- [x] **ADR-020 (Custom Logging System)** — ✅ All log messages use `BUDDD_LOG_TAG("Editor")`, `BUDDD_LOG_INFO`, and `BUDDD_LOG_WARN` macros with `std::format`-style formatting.
- [x] **ADR-012 (Navigable Object Graph)** — ✅ The implementation correctly uses `EngineService` through `engine_` to access `platform().display_count()` and `platform().display_bounds()`, consistent with the navigable object graph pattern.
- [x] **SPEC-036 (Settings System)** — ✅ SPEC-037 is the first concrete consumer of the settings infrastructure. All `editor.window.*` keys use the `user_project_settings` tier, matching SPEC-036's API. The `int32_t` and `std::string` types match `SettingsStore`'s explicit template instantiations.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/editor/settings-system.md`** — ✅ Updated with window geometry section. Correctly documents the three tiers, the `editor.window.*` keys, the save/load lifecycle, geometry tracking, and ADR-019 compliance note.
- [x] **`docs/wiki/architecture/overview.md`** — ✅ Updated directory listing to mention `WindowState`, `WindowPosition`, `DisplayBounds`, `window_utils.h/.cpp`, and new APIs in `platform/` and `window/` sections.
- [x] **`docs/wiki/architecture/module-map.md`** — ✅ Updated Platform and Window submodule tables with detailed documentation of all new methods, structs, and enum values.
- [x] **No wiki contradictions with spec/code** — ✅ All wiki descriptions match the spec, implementation contract, and actual code behavior. The wiki correctly describes the Normal-geometry caching strategy.

## Warnings

Non-blocking concerns for awareness:

1. **`tests/cmd/cli_app_tests.cpp` outlier** — The implementation contract's "Files forbidden to change" list did not include this file, but the orchestrator directed the fix for a pre-existing test failure. This is a reasonable exception but should be noted for process compliance.
2. **`WindowSDL3::resize()` ignores `SDL_SetWindowSize` failure** — The spec's Error Cases section recommends logging a warning on failure. The implementation contract did not require this, and in practice the SDL3 call always succeeds with valid arguments. Minor deviation from spec.
3. **Headless editor tests don't exercise `Editor::setup()` validation block** — The `HeadlessEditorFixture::setup_editor()` calls `Editor::setup()` which returns early at the `engine_imgui::is_initialized()` check in headless mode. Consequently, the window geometry validation block never runs in headless tests. This is a known architectural limitation, not a bug, and the validation is tested through SDL3 offscreen integration tests.
4. **`[math]` test tag in `window_state_tests.cpp`** — Some test cases use the `[math]` tag which appears to be a copy-paste artifact. The `[math]` tag is not meaningful for window state tests. Does not affect correctness but should be cleaned up.

## Required governance updates

No new ADRs required. All changes are within existing ADR boundaries. No governance documents need amendment.

## Workflow gate compliance

| Gate | Status |
|---|---|
| Spec author | ✅ completed |
| Spec critic | ✅ completed (accepted) |
| Implementation contract author | ✅ completed |
| Implementation contract critic | ✅ completed (accepted) |
| Human validation | ✅ approved (Hilderin, 2026-06-13) |
| Code implementer | ✅ completed |
| Code reviewer | ✅ completed (loop #3, accepted) |
| Wiki agent | ✅ completed |
| Governance reviewer | ✅ completed (accepted) |

## Summary

SPEC-037 / IMPL-037 (Editor Window Geometry Persistence) is a well-documented, well-architected feature that respects all existing ADRs. The cross-document coherence is strong — all contradictions found during review were resolved through the loop-back process (save algorithm, maximized save logic, integration test file permissions). The implementation correctly adds 5 new pure virtual methods to `Window`, 2 to `Platform`, state conversion helpers, and save/load/validation logic in `Editor::setup()` and `Editor::shutdown()`. All 23 acceptance criteria are satisfied, 656 tests pass with zero warnings, and the architecture boundary (ADR-019) is preserved. The wiki has been updated to reflect the current state.

**Verdict: Accepted** — no blocking issues remain.
