# Implementation Contract Review — capture-frame-validation (IMPL-009) — Re-review #2

## Summary

The contract-author has resolved all issues from the previous review. The blocking issue (Row 928 behavior description) is fixed — the contract now correctly states row 928 must be updated. All three warnings are addressed: test file pinned to `tests/capture_frame_tests.cpp`, AP test patterns removed, integration tests use `temp_filename()` with unique prefixes. Full re-validation against SPEC-009, CONST-002, wiki, and IMPL-008 finds no new issues. **Verdict: accepted.**

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Row 928 behavior description is incorrect/unchanged when it should change**: Contract Section 4 claims row 928 ("Frame-limited run with `--capture` where frame > frame_limit") needs no update — that the description "Capture spec stored but never matched; no output file created (no error)" is still accurate. This is **wrong**. After IMPL-009:
  - If `--frame` is explicit and `frame_limit < max_effective` → `parse_running_args()` returns an error (exit 1), the run never starts.
  - If `--frame` is NOT explicit (auto-set) → `frame_limit` = `max_effective`, so the capture frame can **never** exceed `frame_limit`.
  - There is **no case** where a capture silently produces nothing after IMPL-009.
  - **Fix**: Row 928 must be updated to describe the new behavior: either an error (explicit frame too small) or the auto-set guarantee (frame_limit >= capture frame). Suggested text: "When `--frame` is explicit and `N < max_effective`, `parse_running_args()` returns error; exit 1. When `--frame` is auto-set (no explicit `--frame`), `frame_limit` is set to `max_effective`, so capture always fires."

**Resolution in re-review**: Section 4 now correctly describes row 928 as needing update to "Impossible after IMPL-009: explicit `--frame < max_effective` is now an error; auto-set guarantees capture always fires." Row 929 also updated to describe the new behavior. **Blocking issue resolved.** ✅

## Warnings

Non-blocking concerns for awareness:

- [x] **Ambiguous test file location**: File #4 in "Files allowed to change" says `tests/cli_app_tests.cpp` **or** new file `tests/capture_frame_tests.cpp`. This ambiguity could lead to test file fragmentation. Recommend specifying a single location (e.g., `tests/capture_frame_tests.cpp`) and updating the done criteria accordingly.

  **Resolution**: Now pinned to a single file: `tests/capture_frame_tests.cpp` (new file). ✅

- [x] **Non-existent AP test patterns referenced**: The contract says "Use the same pattern as AP-01 through AP-10 from IMPL-008" for direct function call tests, but those tests do not exist in the current codebase. The Code Agent may be confused.

  **Resolution**: All AP test pattern references removed. Tests are now fully specified inline (EF-01..EF-05, CF-01..CF-13). ✅

- [x] **Integration test output path collision**: CI-01 and CI-02 both write to `/tmp/out.png`. Could collide in parallel runs.

  **Resolution**: Both now use `temp_filename()` with unique prefixes (`buddd_capture_ci01`, `buddd_capture_ci02`). ✅

- **Spec-critic warning about AC-005 testing specificity**: The spec-critic noted that AC-005 uses substring match while SC-002 uses full-string contains. The contract tests (CF-04, CF-05) follow the substring pattern. Not a blocking issue; the tests are correct either way.

## Required changes

Concrete, actionable changes requested:

1. [x] **Update row 928** in `docs/specs/cli-app-system/implementation-contract.md` to reflect the new IMPL-009 behavior (blocking issue above).
2. [x] **Specify a single test file** for new unit tests to avoid inconsistency.
3. [x] **Add unique temp paths** for CI-01 and CI-02 integration tests to avoid collisions.

## Suggested improvements

Optional ideas (not required):

- Consider adding a test for the edge case `--frame 3 --capture 1:path --capture 2:path` (explicit frame >= all effective frames, mixed effective frames) to complement CF-13 which only tests auto-set.
- The contract could explicitly state that `test_helpers.h` must be included by the new test file for `run_buddd()` and `temp_filename()` helpers.

## Re-validation findings (re-review #2)

All previous issues resolved. Full re-validation against SPEC-009, CONST-002, wiki business-rules.md, ADR-014, and IMPL-008 finds:

### Checks passed
- **Allowed files**: 5 specific files, not too broad ✅
- **Forbidden files**: 6 entries, covers all relevant paths ✅
- **Tests**: EF-01..EF-05, CF-01..CF-13, CI-01..CI-02 — comprehensive coverage linked to SPEC-009 AC items ✅
- **Conventions**: 8 explicit conventions (nodiscard, pragma once, namespace, trailing return, error format, test style, tagging) ✅
- **Architecture decisions**: All explicit; no hidden decisions left to Code Agent ✅
- **New dependencies**: None ✅
- **Migration/Data impact**: None ✅
- **Security impact**: Documented ✅
- **Documentation impact**: Covers IMPL-008 edge table and wiki verification ✅
- **ADR impact**: ADR-014 and ADR-001 referenced correctly ✅
- **Constitution impact**: CONST-002 satisfied ✅
- **Consistency with SPEC-009**: All behaviors match (auto-set, error, effective_frame, edge cases, error format, AC linkage) ✅
- **Consistency with wiki**: Driver quirk formula preserved ✅
- **Consistency with CONST-002**: All testable code has corresponding tests ✅
