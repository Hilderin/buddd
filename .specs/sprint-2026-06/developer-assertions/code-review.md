# Code Review — Developer Assertions

## Summary

The implementation of the Developer Assertions system (IMPL-023) has been reviewed against the accepted spec (SPEC-023), the implementation contract, existing ADRs, wiki conventions, and codebase conventions. All 20 acceptance criteria are satisfied. The feature builds cleanly in both debug and release modes, passes all 12 assertion test cases (24 assertions) in both builds, and does not regress any of the existing 419 test cases (debug) / 413 passing tests (release, 6 pre-existing failures unrelated). No blocking issues found.

## Files reviewed

### New files
- `src/engine/debug/debug_break.h` — Inline `buddd::engine::debug_break()` with platform intrinsics
- `src/engine/debug/assert.h` — `format_assertion_failure_message()`, `handle_assertion_failure()`, five macros
- `src/engine/debug/assert.cpp` — Implementation of formatting and handler
- `tests/assertion_tests.cpp` — 12 test cases covering all acceptance criteria

### Modified files
- `src/engine/log/log.h` — Added `LogLevel::Fatal`, `BUDDD_LOG_FATAL`, `BUDDD_LOG_TAGGED_FATAL`
- `src/engine/log/console_sink.cpp` — Added `case LogLevel::Fatal: return "FATAL"`
- `src/engine/engine_service.cpp` — 4 `BUDDD_ASSERT` insertion points
- `src/engine/scene/entity.cpp` — 1 `BUDDD_ASSERT` insertion point
- `src/engine/render/render_device_opengl.cpp` — 2 `BUDDD_FAIL_MSG` default cases

### Forbidden files (verified unchanged)
- `src/cmd/app_config.cpp`, `src/cmd/app_config.h` — Not modified
- `src/cmd/main.cpp` — Not modified
- `tests/log_helpers.h` — Not modified
- `tests/CMakeLists.txt`, `src/engine/CMakeLists.txt` — Not modified
- `docs/` — No changes (handled by wiki-agent)

## Acceptance criteria verification

| ID | Description | Status | Evidence |
|---|---|---|---|
| AC-001 | `LogLevel::Fatal` > `Error` in enum | ✅ Pass | `Fatal` value is 5, `Error` is 4. Test T-A1. |
| AC-002 | `Fatal` is last/highest enumerator | ✅ Pass | Test T-A12 confirms value 5. |
| AC-003 | Console uses `[FATAL]` prefix | ✅ Pass | `console_sink.cpp` line 14. Test T-A5. |
| AC-004 | `debug_break()` in `debug_break.h` | ✅ Pass | Code review + T-A2 compiles. |
| AC-005 | Correct platform intrinsics | ✅ Pass | `__builtin_trap()` for GCC/Clang, `__debugbreak()` for MSVC. |
| AC-006 | No-op in `NDEBUG` builds | ✅ Pass | `#ifndef NDEBUG` guard in `debug_break.h`. |
| AC-007 | `BUDDD_ASSERT` eval + log+break+abort (debug) | ✅ Pass | Macro structure in `assert.h`. Test T-A8. |
| AC-008 | `BUDDD_ASSERT` no eval in release | ✅ Pass | `((void)0)` in release. Test T-A9. |
| AC-009 | `BUDDD_ASSERT_MSG` with custom message | ✅ Pass | Macro passes `std::format` result. Test T-A4. |
| AC-010 | `BUDDD_VERIFY` eval + abort (debug) | ✅ Pass | Tests T-A6, T-A7. |
| AC-011 | `BUDDD_VERIFY` eval + log-only (release) | ✅ Pass | Release path logs directly, no `handle_assertion_failure`. |
| AC-012 | `BUDDD_FAIL` unconditional | ✅ Pass | Test T-A10. No `#ifndef` guard. |
| AC-013 | `BUDDD_FAIL_MSG` with formatted message | ✅ Pass | Test T-A10. |
| AC-014 | `handle_assertion_failure` signature | ✅ Pass | Code review matches AC-014 exactly. |
| AC-015 | Formatted report format | ✅ Pass | Tests T-A3, T-A4 verify all fields. |
| AC-016 | Fixed `"Assert"` tag | ✅ Pass | Hardcoded in all macros (code review). |
| AC-017 | `do { } while(false)` wrapper | ✅ Pass | All 5 macros use it (code review). |
| AC-018 | No double evaluation | ✅ Pass | Tests T-A7, T-A8. |
| AC-019 | Zero external dependencies | ✅ Pass | Only `format`, `string`, `string_view`, `optional`, `cstdlib` + own headers. |
| AC-020 | log → break → abort sequence | ✅ Pass | `handle_assertion_failure` calls `log()`, `debug_break()`, `abort()` in order. |

## Build verification

| Check | Result |
|---|---|
| `cmake --build --preset debug` | ✅ Succeeds, no warnings |
| `cmake --build --preset release` | ✅ Succeeds, no warnings |

## Test results

| Suite | Result |
|---|---|
| Debug: `[assertion]` tests (12 cases, 24 assertions) | ✅ All passed |
| Debug: Full suite (419 test cases, 21415 assertions) | ✅ All passed |
| Release: `[assertion]` tests (12 cases, 24 assertions) | ✅ All passed |
| Release: Full suite (419 cases, 21410 assertions) | ✅ 413 passed, 6 pre-existing failures (unrelated — `cli_app_tests`, `cmd_tests`, `logging_tests`) |

## Visual verification

Not required. This feature does not produce rendered/visual output. It adds assertion infrastructure (macros, logging extension, formatting).

## Contract compliance verification

- `debug_break.h`: Correct platform intrinsics, `#ifndef NDEBUG` guard, `inline`, `buddd::engine` namespace ✅
- `assert.h`: `snake_case` functions, all 5 macros, `do { } while(false)`, `__VA_OPT__(,)`, `"Assert"` tag hardcoded, `[[nodiscard]]` on formatter, `[[noreturn]]` on handler ✅
- `assert.cpp`: Multi-line format matches spec; `log()`, `debug_break()`, `abort()` sequence ✅
- `log.h`: `LogLevel::Fatal` after `Error`, value 5, `BUDDD_LOG_FATAL` + `BUDDD_LOG_TAGGED_FATAL` added ✅
- `console_sink.cpp`: `case LogLevel::Fatal: return "FATAL"` added ✅
- 8 assertion insertion points across 3 files ✅
- No `BUDDD_TESTING` involvement ✅
- No forbidden files modified ✅

## Blocking issues

None.

## Warnings

- **Release build pre-existing failures**: 6 test cases in `cli_app_tests.cpp`, `cmd_tests.cpp`, and `logging_tests.cpp` fail in release builds. These are pre-existing and unrelated to the assertion feature.
- **`BUDDD_LOG_FATAL` / `BUDDD_LOG_TAGGED_FATAL` not directly tested**: No test exercises these macros directly. The existing T-A5 tests the Fatal-level log path via `Logger::instance().log()`, which covers the runtime behavior but not macro expansion. Acceptable given the macros are mechanical renames of the existing ERROR pattern.
- **`experiments-spec-driven-dev.md` modification**: This file at the repo root has an unrelated change (adding a note about eliminating separate critic files). Not part of this feature implementation but present in the working tree.

## Suggested improvements

- **`format_assertion_failure_message` noexcept**: Consider adding `noexcept` since the function performs pure string formatting with no side effects or allocations that could throw (beyond `std::format` itself). Currently has no explicit `noexcept` specification.
- **Remove redundant `is_enabled()` check in `BUDDD_VERIFY` release path**: The outer `is_enabled()` call in the release path of `BUDDD_VERIFY` is redundant since `Logger::log()` internally calls `is_enabled()` again. Not a bug, but adds unnecessary code.
