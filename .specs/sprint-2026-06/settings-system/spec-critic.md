# SPEC-036 spec-critic — Settings System (MVP1 Skeleton)

## Re-review (13-Jun-2026)

**Outcome**: accepted

All 5 previously blocking/warning issues have been resolved. The E2E verification now explicitly requires `BUDDD_HAS_DISPLAY=ON` (display-backed). The dangling pointer issue is addressed via a documented persistent `std::string` member. The ADR-019 note is simplified. AC-027 covers observer cleanup. AC-019 references AC-008 for clean-store coverage. All Definition of Ready criteria are satisfied.

### Previously blocking/warning issues
- [x] Blocking: Underspecified headless E2E test — resolved. Now uses `BUDDD_HAS_DISPLAY=ON` display-backed testing (lines 146-147, 165). No headless mock required.
- [x] Warning: IniFilename dangling pointer — resolved. Spec now documents persistent `std::string` member (lines 326-329, 358).
- [x] Warning: Contradictory ADR-019 note — resolved. Simplified to clear "No update needed" statement (lines 412-413).
- [x] Warning: Missing AC for observer cleanup — resolved. AC-027 added (line 151).
- [x] Warning: AC-019 clean-store gap — resolved. AC-019 now references AC-008 for clean-store no-op coverage (line 143).

### Remaining issues
- none

## Re-review 2 (13-Jun-2026) — After design changes

**Outcome**: accepted

The spec was substantially reworked with these changes:
1. TypeRegistry integration — `SettingsStore` now takes `SerializationContext`
2. `migrate_ini()` removed entirely
3. `layout_ini_path()` merged to single method returning `const std::string&`
4. `os_user_config_dir()` moved to `src/engine/util/`
5. New `editor_data_root()` / `editor_user_data_root()` utilities
6. Editor integration uses `SerializationContext{engine_->assets()}`
7. ACs renumbered (32 items)

All 5 previously resolved issues remain resolved. The new design changes are sound — TypeRegistry integration follows the pattern established by ADR-028 (existing `MockAssetManager` + `SerializationContext` in unit tests at `tests/engine/component_registry_tests.cpp`), `editor_data_root()` utilities centralise path logic, and the `os_user_config_dir()` move enables reuse. AC-032 (unregistered type test) is clear and testable. All Definition of Ready criteria remain satisfied. No new blocking issues introduced.

### Previously resolved issues (unchanged)
- [x] Blocking: Underspecified headless E2E test — resolved (display-backed via `BUDDD_HAS_DISPLAY=ON`)
- [x] Warning: IniFilename dangling pointer — resolved (persistent `std::string` member)
- [x] Warning: Contradictory ADR-019 note — resolved (simplified)
- [x] Missing AC for observer cleanup — resolved (AC-025)
- [x] Clean-store not-written behavior — resolved (AC-019 references AC-008)

### New issues
- none

## Summary

This spec is well-structured, thorough, and covers all Definition of Ready criteria. It defines clear API surfaces (`SettingsStore`, `SettingsManager`, `os_user_config_dir()`, `editor_data_root()`, `editor_user_data_root()`), 32 acceptance criteria with verification methods, path resolution per OS, edge cases, error conditions, and lifecycle integration with `Editor::setup()`/`shutdown()`. After the substantial rework on 13-Jun-2026, the spec correctly integrates TypeRegistry via `SerializationContext`, removes the deprecated `migrate_ini()` path, merges `layout_ini_path()` into a single persistent-`std::string` method, and extracts reusable path utilities into `src/engine/util/`. All Definition of Ready criteria are satisfied. No new blocking issues. The spec is accepted.

## Satisfied criteria

- [x] Scope is clearly defined (what is included and what is explicitly excluded) — Goals and Non-goals sections are explicit.
- [x] Dependencies on other features, modules, or external systems are identified — yaml-cpp, std::filesystem, ImGui INI, ADR-019, existing Editor class.
- [x] Edge cases and error conditions are described — 13 edge cases and 6 error cases with tables.
- [x] The expected behavior is unambiguous and testable — All 26 ACs have verification methods; user stories use Given/When/Then.
- [x] Acceptance criteria are specific, measurable, and verifiable — Each AC describes exact expected behavior and verification technique (unit test, integration test, code review).
- [x] Success and failure states are described — 6 success criteria with metrics, error cases with categories and behaviors.
- [x] Interface changes (API signatures, config keys, file paths) are documented — Complete C++ API signatures for all three new types, path resolution table, Editor integration points.
- [x] The spec defines how the feature will be verified end-to-end — E2E integration test with display-backed Editor lifecycle (`BUDDD_HAS_DISPLAY=ON`), plus 21 headless-compatible unit tests for `SettingsStore` and `SettingsManager`.
- [x] Existing documentation that must be updated is listed — ADR-019 reference, editor-foundation spec, wiki pages mentioned.
- [x] Technical constraints are identified — yaml-cpp availability, std::filesystem (C++17), OS-standard config dirs, ADR-019.
- [x] Risks or unknowns are surfaced — Assumptions table with 8 items, all open questions resolved with human.
- [x] Performance or resource implications are noted — Load once, save on shutdown/on-demand, dirty-store optimization.

## Issues found

### Blocking Issues

#### Underspecified headless E2E test — blocking
- **Severity**: blocking
- **Location**: Lines 152-164 (E2E Verification), AC-022 (line 146), AC-023 (line 147)
- **Description**: The spec states that the E2E integration test "does NOT require a display (headless engine + manual mock for ImGui) and can run in CI." However, the current `Editor::setup()` (editor.cpp, line 66) returns `InitFailed` if `engine_imgui::is_initialized()` returns false, which is always the case in headless mode. The term "manual mock for ImGui" is not defined anywhere in the spec. Without specifying how the Editor's ImGui dependency is satisfied in headless mode, AC-022 and AC-023 are not proven testable, and the E2E verification method is underspecified.
- **Recommendation**: Either (a) specify how the ImGui mock works (e.g., a test seam that bypasses the is_initialized check, or a test-only `engine_imgui::force_initialized()` helper), (b) acknowledge that AC-022/AC-023 require display-backed testing, or (c) redefine AC-022/AC-023 to test `SettingsManager` directly rather than through the full Editor lifecycle.

### Warnings (non-blocking)

#### IniFilename dangling pointer risk
- **Severity**: non-blocking
- **Location**: Line 355 (Changes in `Editor::setup()`)
- **Description**: The spec writes `ImGui::GetIO().IniFilename = settings_manager_->layout_ini_path().string().c_str();`. Since `path::string()` returns a temporary `std::string`, the `.c_str()` pointer becomes dangling after the expression completes. `ImGui::GetIO().IniFilename` is a `const char*` that must remain valid for the lifetime of the ImGui context.
- **Recommendation**: The Editor (or SettingsManager) should store the ini path as a persistent `std::string` member, and set `IniFilename` to a pointer to that persistent string. Update the spec to reflect this correct pattern, or leave it as an implementation detail for the implementer.

#### Contradictory documentation update note
- **Severity**: non-blocking
- **Location**: Lines 406-408 ("Existing documentation that must be updated")
- **Description**: The section says "Update if [AMEND-2026-001 needs to be referenced]" and then immediately concludes "does NOT require an amendment." This reads as if it's debating whether to update ADR-019 or not, and the final answer is "no update needed." The wording is confusing — it should be a clear statement either way.
- **Recommendation**: Simplify to: "No update needed. The settings module sits in `src/engine/` and is already within the architecture boundary defined by ADR-019."

#### Missing AC for observer cleanup
- **Severity**: non-blocking
- **Location**: Line 188 (edge case: "Observer is unregistered (lifecycle)")
- **Description**: The edge case table describes correct behavior for `Connection` destruction (auto-unregister), but there is no acceptance criterion that tests this behavior. AC-012 and AC-013 test observer invocation but not cleanup. Without a test, `Connection` cleanup is only implicitly verified.
- **Recommendation**: Consider adding AC-027: "Destroying a `Connection` object unregisters its observer, so `set()` on the observed key no longer triggers the callback." with unit test verification.

#### Clean-store not-written behavior not tested in AC-019
- **Severity**: non-blocking
- **Location**: AC-019 (line 143)
- **Description**: AC-019 tests that "save_all() saves all dirty stores" but does not test that clean stores are NOT written (as specified in Story 3, lines 88-90). This is a minor gap — the behavior is defined but not verified by an AC.
- **Recommendation**: Either split AC-019 into two parts (dirty stores are saved, clean stores are not) or note that AC-008 covers the per-store no-op behavior for clean stores, which combined with AC-019 implicitly covers `save_all()`.
