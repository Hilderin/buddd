# SPEC-007 — CLI Command Evolution: Demo System & Empty Run

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

### Previously resolved (from 1st review)

- [x] **B-01 (title case)** — RESOLVED. Window title now consistently uses lowercase `triangle` throughout (Story 1 line 293, AC-007 line 395), matching the case-preservation rule (line 117).
- [x] **B-02 (CONST-002 / tests optional)** — RESOLVED. The test implications section now explicitly states "New tests (required by CONST-002 — all unconditionally testable paths must have tests)" (line 498). The "optional" label is removed.
- [x] **B-03 (argv contract)** — RESOLVED. `DemoCommand::run()` now passes `argc - 2, argv + 2` to per-demo functions (line 587), so `argv[0]` is the demo name as documented (line 239). The note at line 589 confirms this.

### New in this review

- [x] **B-04 — Edge case table and Assumption A-10 contradict the headless backend behavior**

  The spec introduces compile-time backend selection:
  - `BUDDD_HAS_DISPLAY=ON` → `be::Backend::SDL3`
  - `BUDDD_HAS_DISPLAY=OFF` → `be::Backend::Headless` (lines 115, 168)

  The spec's own behavior descriptions confirm the headless backend *works*:
  - RunCommand: "or indefinitely (headless backend, `poll_events()` always returns `true`)" (line 167)
  - DemoCommand: "headless backend never returns `false` from `poll_events()`" (line 122)
  - AC-007: "Works with both SDL3 and headless backends" (line 395)
  - Test table: `buddd demo triangle` "Always runs (headless backend on CI)" (line 507)

  However, the edge case table says the **opposite**:

  | Line | Edge case | Says |
  |---|---|---|
  | 433 | `buddd run` with no display (`BUDDD_HAS_DISPLAY=OFF`) | "Platform creation fails at runtime; error printed to stderr; exits non-zero." |
  | 434 | `buddd demo triangle` with no display | "Same — platform creation fails; error to stderr; exits non-zero." |

  These edge cases describe the **old** SPEC-006 behavior (SDL3 backend always used, fails without a display). Under the updated spec, `BUDDD_HAS_DISPLAY=OFF` selects the headless backend, which explicitly **succeeds** at platform creation (confirmed by `src/engine/platform/platform.cpp` line 23–27).

  Furthermore, **Assumption A-10** (line 544) states:
  > "When the CLI has no display (`BUDDD_HAS_DISPLAY=OFF` or no SDL3 at runtime), commands that try to open a window fail at the `Platform::create()` or `create_window()` stage with an engine error."

  This is also a leftover from SPEC-006. When `BUDDD_HAS_DISPLAY=OFF`, the headless backend is used and `Platform::create(Backend::Headless)` succeeds. The "no SDL3 at runtime" part is still valid for the `BUDDD_HAS_DISPLAY=ON` case, but the "`BUDDD_HAS_DISPLAY=OFF`" part is now incorrect.

  **Impact**: An implementer who follows the edge cases or A-10 will expect platform creation to fail in headless mode, but the body text and AC-007 say it succeeds. This is a self-contradiction that makes the spec ambiguous about the expected headless behavior.

  **Required resolution**: Update **all three locations** to be consistent with the compile-time backend selection:
  - Edge case 433: Change to `"Uses headless backend. poll_events() always returns true — runs until killed by timeout."`
  - Edge case 434: Change to `"Uses headless backend. Demo runs 120 frames (~2s) and completes normally."`
  - Assumption A-10: Remove the `BUDDD_HAS_DISPLAY=OFF` case from the failure scenario (keep only "no SDL3 at runtime" for the `BUDDD_HAS_DISPLAY=ON` case), or clarify that platform creation succeeds with the headless backend.

---

## Warnings

Non-blocking concerns for awareness:

### Previously resolved (from 1st review)

- [x] **W-01 (framebuffer clear undefined)** — RESOLVED. The spec now documents that `begin_frame()` clears the colour buffer to black as an implementation detail of the OpenGL backend (line 173).
- [x] **W-02 (extra-args warning ordering)** — RESOLVED. The implementation pseudocode now prints the extra-arguments warning **after** resource creation (steps 5 → 6, lines 583–586).
- [x] **W-03 (run extra_arg missing)** — RESOLVED. Added to edge case table (line 437).
- [x] **W-04 (RunCommand.h comment)** — RESOLVED. Required implementation behavior now includes `run_command.h` doc comment update (lines 594–596).
- [x] **W-05 (case sensitivity note)** — RESOLVED. Added "Demo names are case-sensitive." to both usage messages (lines 146, 161).
- [x] **W-06 (title/message casing)** — RESOLVED. All titles and diagnostic messages now use lowercase `triangle` consistently.

### New in this review

- [x] **W-07 — AC-018 verification command (`grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/`) produces false positives for `be::Backend::SDL3`**

  The current verification command for AC-018 (line 407) and AC-019/AC-023/AC-024 references:
  ```
  Run `grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/` — zero matches.
  ```

  In the updated codebase, this grep will match:
  - `src/cmd/commands/run_command.cpp`: `return be::Backend::SDL3;`
  - `src/cmd/commands/demo_command.cpp`: `return be::Backend::SDL3;`
  - `src/cmd/CMakeLists.txt`: `message(STATUS "buddd: BUDDD_HAS_DISPLAY=ON (SDL3 backend)")`

  None of these are CONST-001 violations — they use the engine's `Backend` enum or are build system strings. The regex is a heuristic that worked when the SDL3 string only appeared in `#include` directives, but now it produces false positives. The verification should be refined to exclude `be::Backend::SDL3` and build system strings, or changed to only check for `#include` lines (e.g., `grep -rnE '^#include.*(SDL3|GL/|glad|glm)' src/cmd/`).

  **Impact**: An implementer who runs the verification as documented will see matches and may incorrectly conclude CONST-001 is violated. A reviewer familiar with the heuristic will know it's a false positive, but the spec should be precise.

- [x] **W-08 — Line 514 misleadingly describes paths as "display-dependent"**

  Line 514 states:
  > "Display-dependent paths (demo mode with window, run mode with window) are guarded by `BUDDD_HAS_DISPLAY` and tested at the integration level."

  With the compile-time backend selection, these paths are **not** display-dependent anymore — they use the headless backend on CI and work without a display. The implementation contract's tests confirm this (the `buddd demo triangle` test runs unconditionally, not guarded by `BUDDD_HAS_DISPLAY`). The phrase "display-dependent" is misleading and contradicts the test table (line 507) which says "Always runs (headless backend on CI)."

  **Impact**: Readers may incorrectly assume these paths are conditionally compiled or skipped on CI, when in fact they now run with the headless backend.

---

## Required changes

Concrete, actionable changes requested:

### Re-review items (new in this review)

- [x] **B-04**: Updated edge case rows 433, 434 and Assumption A-10 to reflect that `BUDDD_HAS_DISPLAY=OFF` uses the headless backend which succeeds at platform creation.
- [x] **W-07**: Refined AC-018's verification regex to `#include.*(SDL3|GL/|glad|glm)` to avoid false positives from `be::Backend::SDL3` enum values.
- [x] **W-08**: Updated line 514 to describe paths as "backend-sensitive" rather than "display-dependent".

### Previously resolved items (verified in this review)

- [x] B-01: Title case — now consistent (all lowercase `triangle`)
- [x] B-02: CONST-002 compliance — tests now explicitly required, not optional
- [x] B-03: argv contract — dispatch passes `argc - 2, argv + 2`, doc comment matches
- [x] W-01: Framebuffer clear — documented as `begin_frame()` implementation detail
- [x] W-02: Extra-args warning ordering — now after resource creation
- [x] W-03: `run extra_arg` edge case — added
- [x] W-04: `run_command.h` comment update — documented
- [x] W-05: Case-sensitivity note — added to both usage messages
- [x] W-06: Title/message casing — now consistent

---

## Suggested improvements

Optional ideas (not required):

1. **Headless edge case for `buddd run` timeout in tests**: The spec's AC-011 and AC-012 tests rely on "run with timeout" but don't specify what happens with the headless backend where the loop runs indefinitely. The implementation contract handles this with a timeout/pkill mechanism. Consider documenting this timeout strategy in the spec's test implications section for clarity.

2. **`buddd demo triangle` test on SDL3 backend**: The test implications table (line 507) says `buddd demo triangle` "Always runs (headless backend on CI)". This is the CI path. But there's no mention of an SDL3-specific test for the demo triangle completion. If SDL3 visual verification is desired, consider adding a note about manual testing for the SDL3 path.

3. **Two-node supersession chain**: SPEC-007 supersedes SPEC-006, which superseded SPEC-001. Consider adding a note in the Supersedes section: "SPEC-007 inherits SPEC-006's supersession of SPEC-001 regarding CLI behavior (no-args default, --version flag removal)." This closes the chain cleanly for readers.

4. **CMake status messages in grep false positives**: The `CMakeLists.txt` line `message(STATUS "buddd: BUDDD_HAS_DISPLAY=ON (SDL3 backend)")` will also match the AC-018 grep. This is harmless (CMake file, not C++ source) but if the grep is run across ALL files in `src/cmd/`, it will be flagged. Consider restricting AC-018 verification to `*.h` and `*.cpp` files only.

---

## Re-review summary

| Check | Outcome |
|---|---|
| **Previous B-01 (title case)** | RESOLVED ✓ |
| **Previous B-02 (CONST-002)** | RESOLVED ✓ |
| **Previous B-03 (argv contract)** | RESOLVED ✓ |
| **New B-04 (edge cases contradict headless)** | **BLOCKING** — edge cases 433, 434 and A-10 describe old behavior where SDL3 always fails without display; new compile-time backend selection uses headless which succeeds |
| Internal consistency | 1 new blocking issue (B-04) — body spec says headless works, edge cases say it fails |
| All previous 6 warnings | All RESOLVED ✓ |
| New W-07 (AC-018 grep false positives) | Warning — `be::Backend::SDL3` enum values will match the grep |
| New W-08 (display-dependent claim) | Warning — line 514 misleadingly calls paths "display-dependent" |
| Acceptance criteria testability | ✓ All ACs testable (some manual) |
| Edge/error case coverage | Mostly thorough but B-04 shows edge cases 433–434 are stale |
| Constitution rules preserved | CONST-001 ✓ (enum values are engine abstraction, not headers). CONST-002 ✓ (tests required). |
| Contradictions with SPEC-006 | ✓ None found. Supersession table is thorough and correct. |
| Verdict | **Rejected** — one new blocking issue (B-04) plus two new warnings (W-07, W-08). Previous 3 blocking issues and 6 warnings all successfully resolved. |

---

## Change log

| Review | Verdict | Key findings |
|---|---|---|
| 1st | `Rejected` | 3 blocking issues (B-01 title case, B-02 CONST-002, B-03 argv contract). 6 warnings. |
| 2nd (this) | `Rejected` | Previous 3 blocking issues **RESOLVED**. Previous 6 warnings **RESOLVED**. **New** 1 blocking issue (B-04: edge cases contradict headless backend). **New** 2 warnings (W-07: AC-018 grep false positives, W-08: misleading "display-dependent" claim). |
