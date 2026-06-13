# Implementation Contract Review — CLI `buddd edit [<scene>]` — open editor with scene path

## Blocking issues

None. The implementation fully satisfies the spec and implementation contract.

## Warnings

- **Test T4 (`--capture` flag)** does not assert exit code — only checks that `"Unknown argument for edit"` is absent. This matches the contract's minimum assertion but a stronger assertion (exit code 0) would be more robust. Non-blocking because the test still validates the dispatch branch correctly.
- **Test T1** checks for `"Editor: layout file: buddd_editor.ini"` in stderr, which only appears when `BUDDD_HAS_DISPLAY=ON` (the default). A `BUDDD_HAS_DISPLAY=OFF` build would fail this test. Acceptable given the project default, but worth noting for future portability.

## Required changes

None.

## Suggested improvements

- T4 could add `REQUIRE(res.exit_code == 0)` to strengthen the assertion that the capture-with-flags path works end-to-end (currently only checks absence of error, not presence of success).
- T7 and T8 could check that exit code is *not* 1 (with `Scene file not found`), currently they only check absence of the error string. The combination of both checks would be slightly more defensive.
- Consider adding a test for the corrupt-YAML case (AC-009) in a future cycle — currently only listed as a manual/E2E test.

## Review verdict summary

The implementation was reviewed against the accepted spec (SPEC-035) and implementation contract (IMPL-035). All acceptance criteria are covered. The modified files are limited to the 5 allowed files: `src/cmd/main.cpp`, `src/cmd/apps/editor_app.h`, `src/cmd/apps/editor_app.cpp`, `src/cmd/commands/help_command.h`, `tests/cmd/cli_app_tests.cpp`. All 8 forbidden file categories remain untouched.

### What was verified

1. **editor_app.h**: Added `#include <optional>`, parameterised constructor `EditorApp(std::optional<std::string>)`, and `std::optional<std::string> scene_path_` member. ✅
2. **editor_app.cpp**: Constructor stores scene path in member initializer. `setup()` calls `editor_->open_scene(*scene_path_)` after successful `editor_->setup(ctx)`, logs "Editor: opening scene: {}" on entry and warns on failure without propagating error. ✅
3. **main.cpp**: 4-step edit dispatch (no arg → empty, YAML file → `is_regular_file()` validation, flag `-` prefix → flags only, else → unknown arg error+exit 1). Uses `std::filesystem::is_regular_file()` per spec. `flags_start=3` when scene path present. ✅
4. **help_command.h**: Updated `edit` line to "Open the editor (optionally with a scene file)". ✅
5. **Tests**: 8 integration tests covering all required scenarios (T1-T8). All pass. ✅
6. **Full test suite**: All 582 tests pass with zero regressions. ✅
7. **Build warnings**: Zero warnings from our code (`src/` and `tests/`). ✅
8. **Architecture boundaries**: No SDL3, OpenGL, or GLM headers in modified `src/cmd/` files. ✅
9. **Forbidden files**: `src/cmd/app.h`, `src/cmd/app.cpp`, `src/cmd/app_config.h`, `src/cmd/app_config.cpp`, `src/editor/editor.h`, `src/editor/editor.cpp`, `tests/test_helpers.h`, `CMakeLists.txt` — all confirmed unchanged. ✅

The implementation is complete, correct, and production-ready.
