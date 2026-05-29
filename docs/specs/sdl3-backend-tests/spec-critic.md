# SPEC-003 — SDL3 Backend Tests

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

### Previously reported (2nd review — all resolved)

- [x] **CONST-001 violation** — RESOLVED. Amendment AMEND-2026-001 has been ratified and appended to `docs/constitution/rules/CONST-001-architecture-boundaries.md`. The amendment adds a narrow exception allowing SDL3 test files conditionally compiled with `BUDDD_HAS_DISPLAY=ON` to include `<SDL3/SDL.h>` for setting video driver hints. The spec correctly references this amendment in the Permissions and security section.

- [x] **SPEC-002 AC-015 contradiction** — RESOLVED. SPEC-003 explicitly states (line 215): "This spec (SPEC-003) narrows that rule for SDL3 test files as described above. SPEC-002 remains authoritative for all non-test code and for test files not conditionally compiled with BUDDD_HAS_DISPLAY." The constitution amendment (highest authority) explicitly permits the exception, and SPEC-003 acknowledges and documents the carve-out.

- [x] **Non-goal vs root CMakeLists.txt contradiction** — RESOLVED. The non-goals correctly state: "The root `CMakeLists.txt` is modified only to add the `option(BUDDD_HAS_DISPLAY ...)` definition. No other changes." This is consistent with AC-011, A-05, and Q-03.

- [x] **AC-011 vs A-05/Q-03 (wrong file for option definition)** — RESOLVED. AC-011 now states the option is defined in the root `CMakeLists.txt`. This matches A-05 and Q-03. All three agree.

### Previously reported (2nd review warnings — all addressed)

- [x] **W-01 — `target_compile_definitions` not explicitly specified** — RESOLVED. AC-017 explicitly states the `target_compile_definitions(buddd_tests PRIVATE BUDDD_HAS_DISPLAY)` call.

- [x] **W-02 — Offscreen fallback mechanism is underspecified** — SUPERSEDED. The simplified video driver strategy (no fallback, no retry) replaced the earlier two-tier fallback algorithm. The new approach is simpler and better specified: platform/window tests use `"dummy"`; render device/frame cycle tests use `"offscreen"`. No fallback logic. The spec documents this clearly in the "Video driver strategy by test type" section (lines 116–123), AC-002, and all edge/error cases.

- [x] **W-03 — Tag naming conventions suggested but not required** — RESOLVED. AC-018 now requires the `[sdl3]` tag on all new test cases.

- [x] **W-04 — `SDL_SetHint` return value handling for offscreen not covered** — RESOLVED. The simplified approach uses the same hint key for both drivers. The Error cases section covers `SDL_SetHint` returning `false` for the general case.

- [x] **W-05 — Render device tests under dummy driver may need `[!mayfail]`** — NOT ADOPTED (acceptable). With the simplified approach, render device tests use `"offscreen"` (not `"dummy"`), so this concern is moot. The spec documents expected failure transparently.

- [x] **W-06 — No guidance for multi-test file pattern** — NOT ADOPTED (acceptable). Acceptable for a single-file test spec.

### Previously found issues (2nd review — now resolved in this spec version)

- [x] **Out of scope section contradicts non-goals on root CMakeLists.txt** — RESOLVED. The Out of scope section (lines 228–238) no longer says "No changes to the root `CMakeLists.txt`". It now correctly lists only `CMakePresets.json`. The non-goals and Out of scope sections are now aligned.

- [x] **Filename pattern mismatch with constitution amendment** — RESOLVED. Assumption A-11 (line 254) explicitly documents that `tests/sdl3_backend_test.cpp` is covered by AMEND-2026-001's "or similar" clause in the glob pattern `tests/*_sdl3*.cpp`.

## New findings (3rd review)

### No new blocking issues

This review found **zero new blocking issues**. The spec is internally consistent, testable, and aligned with the constitution and related specs.

### Minor observations (non-blocking)

- **CI workflow verification phrasing (lines 62–63)**: The spec states the CI job "verifies that: — No SDL3 test code is compiled ... — T-13 is no longer present in the test binary." These are outcomes of the `BUDDD_HAS_DISPLAY=OFF` configuration, not explicit verification steps in the CI YAML. The implementation contract's CI workflow (build + test steps) implicitly confirms these via successful compilation and test pass. This is acceptable at the spec level but may benefit from clarification in the implementation contract (e.g., adding a `grep` or `nm` check step). Not a spec issue.

- **CI workflow compiler selection**: The spec's CI description (line 58) says "standard dependencies (C++26 compiler, CMake, Ninja, SDL3 via `FetchContent`)." The implementation contract's CI YAML installs `g++-14` via `apt` but does not set `CXX=g++-14` or configure CMake with `-DCMAKE_CXX_COMPILER=g++-14`. On `ubuntu-latest`, `g++` defaults to an older GCC. The implementer should ensure the installed compiler is actually selected. This is an implementation contract detail, not a spec issue.

## Required changes

**None.** All previously identified blocking issues and inconsistencies have been resolved in this version of the spec. No further changes are required.

## Suggested improvements

Optional ideas (not required):

- **Consider adding an AC for CI workflow file existence** — The spec includes a GitHub Actions CI workflow in its goals (line 39) and describes it in detail (lines 53–65), but no AC explicitly verifies that `.github/workflows/ci.yml` exists. Adding an AC would close the gap between "goal" and "verifiable criterion." This is a minor completeness suggestion.
- **Add subsystem tags convention** — A-07 suggests `[sdl3]` + subsystem tag (`[platform]`, `[window]`, `[render]`), but AC-018 only requires `[sdl3]`. Consider making subsystem tags a soft convention in one of the assumptions or adding a note.

## Re-review summary

| Check | Outcome |
|---|---|
| All previous blocking issues (4) | All resolved and verified against current spec |
| Previous warnings (6) | All addressed or superseded by simplified design |
| Previously found issues (2 from 2nd review) | Both resolved in current spec version |
| Simplified video driver strategy | Well-specified, consistent across all sections. No fallback/retry logic. Clear driver-per-test-type assignment. |
| T-13 rework (removed from platform_abstraction_test, absorbed into sdl3_backend_test) | Correctly handled. AC-015, non-goals, A-08 all confirm. Implementation contract shows exact removal. |
| CI workflow specification | Clearly described. Single job with `BUDDD_HAS_DISPLAY=OFF`. Consistent with non-goals. ~ |
| Contradictions with constitution | None. AMEND-2026-001 ratified. Spec correctly references it. A-11 addresses filename coverage. |
| Contradictions with SPEC-002 | None. AC-015 narrowing is documented. |
| Contradictions with wiki | No wiki content on this topic found. No contradictions. |
| New issues introduced | **Zero.** All changes are consistent and well-integrated. |
| Acceptance criteria testability | All 18 ACs (AC-001 through AC-018) are specific, measurable, and verifiable by inspection or test execution. |
| Edge/error case coverage | Comprehensive. Covers driver unavailability, OpenGL absence, mixed `BUDDD_HAS_DISPLAY` states, lifecycle violations, `SDL_SetHint` idempotency. |
| Verdict | **Accepted** — the spec is clear, complete, consistent, and ready for implementation. |

## Change log

| Review | Verdict | Key findings |
|---|---|---|
| 1st | `Rejected` | 4 blocking issues (CONST-001, SPEC-002 contradiction, non-goal conflict, AC-011 mismatch) |
| 2nd | `Accepted with warnings` | All blocking issues resolved; 2 non-blocking issues found |
| 3rd (this) | `Accepted` | All previous issues resolved; no new issues; simplified design is well-specified |
