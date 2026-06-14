# Test Report: <feature-name>

## Test Summary

**Total tests**: <N>
**Passed**: <N>
**Failed**: <N>
**Skipped**: <N>

**Build**: clean (zero errors, zero warnings in `src/` and `tests/`)

---

## Unit Tests

<Pass / Fail — summary only, no per-test listing>

---

## Integration / E2E Tests

| Scenario | Method | Result | Evidence |
|---|---|---|---|
| <scenario name> | `buddd capture <scene>` + vision analysis | PASS / FAIL | <path to screenshot> |

### Visual analysis notes
<For each E2E capture, notes from vision_analyze_image — what was checked, whether it matches spec expectations>

---

## Regression Checks

| App / Module | Check performed | Result | Evidence |
|---|---|---|---|
| <app name> | E2E capture + compare / log inspection / test suite | PASS / FAIL | <path or description> |

<Any regressions detected, or "No regressions detected">

---

## Manual Tests Required

<If any tests cannot be automated, list them here with step-by-step instructions for the human. If none, write "none".>

---

## Issues Found

### Blocking
- [ ] <issue description>

### Non-blocking
- <warnings or suggestions>
