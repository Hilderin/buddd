# Spec Review — capture-frame-validation (SPEC-009)

## Summary

Reviewed SPEC-009 (Capture-Frame Validation) against the Definition of Ready, SPEC-008 (parent), constitution rules, and wiki content. The spec is well-structured, internally consistent, and covers all three behaviors requested in the coordination.md: (1) auto-set `--frame` from captures, (2) error on too-small `--frame`, (3) extract driver quirk into `CaptureSpec::effective_frame()`. No blocking issues found. Two non-blocking warnings noted.

**Verdict: accepted**

## Positive aspects

- Clear distinction between Goals, Non-goals, Out of scope, and Assumptions.
- Comprehensive edge case table (12 rows) and error case table (3 rows).
- All 12 acceptance criteria are specific, measurable, and verifiable through unit/integration tests.
- Explicit error message format makes AC-005 straightforward to test.
- Auto-set silence (no new output message) is consistent with the Non-goals and Assumptions.
- Edge case `--frame 0` (explicit interactive) with captures is correctly handled as OK (no validation bypassed).
- Consistent with SPEC-008 — no contradictions found (only additions and refinements).
- Constitution rules are respected (no engine header violations, tests are planned).

## Definition of Ready check

| Criterion | Status | Notes |
|---|---|---|
| Scope clearly defined | ✅ | Goals, Non-goals, Out of scope cover all boundaries |
| Dependencies identified | ✅ | References SPEC-008, existing app_config.h/.cpp, app.cpp |
| Edge cases and errors described | ✅ | 12 edge cases + 3 error cases |
| Behavior unambiguous and testable | ✅ | Input/output tables, AC items with clear verification |
| E2E verification defined | ✅ | Unit tests (`cmd_tests.cpp`) + integration test |
| AC specific, measurable, verifiable | ✅ | 12 AC items with precise inputs/expected outputs |
| Success/failure states described | ✅ | SC-001/002/003 + error cases |
| Interface changes documented | ✅ | `effective_frame()` method, `parse_running_args()` logic |
| Existing documentation updates listed | ⚠️ | **Warning**: not explicitly listed (see Warnings) |
| Technical constraints identified | ✅ | OpenGL driver quirk, existing structs unchanged |
| Risks/unknowns surfaced | ✅ | Open Questions (Q-01 through Q-03) with resolutions |
| Performance/resource implications noted | ✅ | "pure computation", "no new file I/O or network" |

## Blocking issues

None.

## Warnings

- **DoR — Existing documentation updates not explicitly listed**: The spec does not include a section listing which existing documents (README, wiki, ADRs, other specs) must be updated. While the wiki's `business-rules.md` already contains a consistent `effective_frame` formula (line 102) and the spec functions as an amendment to SPEC-008, a formal list would improve traceability. This is a minor gap, not a blocker.

- **AC-011 relies on manual code inspection**: AC-011 ("Inspect `app.cpp` — the render loop calls `spec.effective_frame()` and does NOT contain the raw expression") requires a developer to manually verify the code change rather than running an automated test. This is acceptable for a refactoring AC but is inherently weaker than behavioral verification. Consider augmenting with a compile-time assertion or a grep-based CI lint rule that fails if the raw expression `(spec.frame < 2)` appears in `app.cpp`.

- **Minor consistency note**: AC-005 verifies the error message via sub-string matching (`"too small"` + `"need at least 2"`), while SC-002 uses a full-string `contains` check. These are compatible but at different specificity levels. No change required, but the implementation contract should be explicit about which approach tests must use.

## Required changes

None.

## Suggested improvements

- Add a `## Documentation updates` section listing which documents are affected (e.g., "SPEC-008 edge cases table: rows involving `--capture`/`--frame` interaction are superseded", "business-rules.md: Section 'Driver quirk' already consistent, no change needed").

## Re-review notes

This is the first review of SPEC-009. No prior review file existed.
