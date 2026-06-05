# Implementation Contract Review — capture-frame-validation (IMPL-009 / SPEC-009)

## Summary

The implementation successfully implements all 12 acceptance criteria (AC-001 through AC-012) from SPEC-009 and follows the implementation contract IMPL-009 precisely. All modified files are within the allowed scope (plus a necessary structural change to `tests/CMakeLists.txt` to compile the new test file). The build succeeds, all 366 tests pass (18 new + 348 existing), and the binary behaves correctly in manual render tests. No blocking issues found.

## What was verified

- **Build**: `cmake --build --preset debug` succeeds with no errors.
- **Unit tests**: `./build/debug/tests/buddd_tests [capture]` — 18 test cases, 36 assertions, all pass.
- **Full test suite**: `./build/debug/tests/buddd_tests` — 366 test cases, 13263 assertions, all pass.
- **Existing CLI tests**: `./build/debug/tests/buddd_tests [cli]` — 36 test cases, 86 assertions, all pass.
- **Render test (auto-set)**: `./build/debug/src/cmd/buddd run cube --capture 1:/tmp/review_capture1.png` exits 0, produces valid 1024×768 PNG of frame 2. (Auto-sets `frame_limit=2`, driver quirk preserved.)
- **Render test (error)**: `./build/debug/src/cmd/buddd run cube --frame 1 --capture 1:/tmp/review_error.png` exits 1 with error `"Error: --frame 1 is too small for captures (need at least 2)"`.
- **Render test (negative frame)**: `--frame -1` still correctly errors with `"non-negative integer"` message.
- **Code inspection**: No inline quirk expression `(spec.frame < 2) ? 2 : spec.frame` remains in `app.cpp` or `app_config.cpp` (AC-011 satisfied).
- **Constitution**: CONST-002 satisfied — all new testable code has corresponding unit tests.
- **Forbidden files**: No modifications to `main.cpp`, `app.h`, `src/cmd/apps/`, `src/engine/`, or any other forbidden file.

## Files modified

| File | Change | Status |
|---|---|---|
| `src/cmd/app_config.h` | Added `[[nodiscard]] int effective_frame() const` to `CaptureSpec`. Updated doc comment for `--frame` to say `>= 0`. | ✓ |
| `src/cmd/app_config.cpp` | Added `bool frame_explicit` tracking; auto-set logic; validation error return. | ✓ |
| `src/cmd/app.cpp` | Replaced inline `(spec.frame < 2) ? 2 : spec.frame` with `spec.effective_frame()`. | ✓ |
| `tests/capture_frame_tests.cpp` | **New file**: 18 test cases (EF-01–EF-05, CF-01–CF-13). | ✓ |
| `tests/CMakeLists.txt` | Added `app_config.cpp` to test executable; added `src/cmd` include directory. | ⚠️ (necessary, see warnings) |
| `.specs/sprint-2026-06/cli-app-system/implementation-contract.md` | Updated edge case rows 928–929. | ✓ |

## Acceptance criteria coverage

| AC ID | Description | Verified by | Status |
|---|---|---|---|
| AC-001 | `effective_frame()` returns `(frame < 2) ? 2 : frame` | EF-01–EF-05 | ✓ |
| AC-002 | Auto-set from single capture `120:/tmp/out.png` | CF-01 | ✓ |
| AC-003 | Auto-set from multiple captures (max) | CF-03 | ✓ |
| AC-004 | Auto-set from `--capture 1:path` → `frame_limit=2` | CF-02 | ✓ |
| AC-005 | Error when `--frame < max_effective` | CF-04, CF-05 | ✓ |
| AC-006 | Success when `--frame >= max_effective` | CF-06, CF-07 | ✓ |
| AC-007 | Success with `--frame 2 --capture 1:path` | CF-08 | ✓ |
| AC-008 | `--frame 0` with captures succeeds | CF-09, render test | ✓ |
| AC-009 | No flags → default `frame_limit=0` | CF-10 | ✓ |
| AC-010 | `--frame` without captures unchanged | CF-11 | ✓ |
| AC-011 | `run_app()` uses `effective_frame()`, not inline expression | Code inspection, `grep` | ✓ |
| AC-012 | Driver quirk preserved (frame 1 → frame 2) | EF-02, EF-03, CF-02, render test | ✓ |

## Blocking issues

None.

## Warnings

1. **`tests/CMakeLists.txt` modification not in allowed file list**: The IMPL-009 allowed file list did not include `tests/CMakeLists.txt`, but a minor change was necessary to compile the new test file (`app_config.cpp` must be linked into the test binary, and the `src/cmd` include directory must be available). The change is small and correct. This is a non-blocking scope expansion.

2. **Existing behavior change for `--frame 0`**: The `--frame` validation was relaxed from `n < 1` (reject 0) to `n < 0` (allow 0), and the error message changed from "positive integer" to "non-negative integer". This is intentional per SPEC-009 AC-008, but it represents a behavioral change from SPEC-008 which previously rejected `--frame 0`. The implementer noted this in their report.

## Required changes

None.

## Suggested improvements

None.

## Verdict

**Accepted** — All acceptance criteria are satisfied, all tests pass, the constitution is respected, and the implementation follows the contract precisely.
