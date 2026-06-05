# IMPL-007 Critic Review — CLI Command Evolution: Demo System & Empty Run

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

IMPL-007 has been updated to address all previous blocking issues and warnings from the first review. The contract now faithfully translates SPEC-007 into precise, compilable code with correct dereference patterns, minimal includes, and correct API understanding.

**Changes verified since last review:**
1. Backend selection via `BUDDD_HAS_DISPLAY` compile-time define using `constexpr` IIFE — correct C++ ✓
2. Demo name validation before resource creation — matches actual code ✓
3. CMakeLists.txt propagates `BUDDD_HAS_DISPLAY` to the `buddd` target — present and correct ✓
4. CLI tests no longer guarded by `BUDDD_HAS_DISPLAY` — works with headless backend on CI ✓
5. Done criteria updated for backend selection items — complete and verifiable ✓

**Remaining issue**: The CONST-001 compliance grep pattern in the contract still uses the bare regex `(SDL3|GL/|glad|glm)` which produces false positives against `be::Backend::SDL3` enum values. The source spec (SPEC-007) was already fixed for this (W-07 in spec-critic), but the fix was not propagated to the contract. This is a warning-level documentation inaccuracy — the enum values are not CONST-001 violations, but the verification command as documented will incorrectly report matches.

No blocking issues remain.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

### Previously resolved (from 1st review)

- [x] **B-01: `run_triangle_demo(*platform, *device, ...)` uses single-dereference instead of double-dereference**

  **Status**: RESOLVED. The contract now correctly shows `**platform` and `**device` (line 285). The actual source code (`src/cmd/commands/demo_command.cpp` line 99) confirms the fix: `return buddd::cmd::demo::run_triangle_demo(**platform, **device, argc - 2, argv + 2);`. ✓

### New in this review

None.

## Warnings

Non-blocking concerns for awareness:

### Previously resolved (from 1st review)

- [x] **W-01: Unnecessary `#include "demos/demo_helpers.h"` in `demo_command.cpp`**

  **Status**: RESOLVED. `demo_command.cpp` no longer includes `demo_helpers.h`. The actual source includes only `"demo_command.h"` and `"demo/triangle_demo.h"`. ✓

- [x] **W-02: Misleading comment about `WindowConfig::title` type**

  **Status**: RESOLVED. The contract's comment now correctly states: `// WindowConfig::title is std::string, so concatenation creates a temporary that is copied into the config.` (line 66-67). This matches the actual definition in `src/engine/window/window.h` where `WindowConfig::title` is `std::string`. ✓

- [x] **W-03: Spec-internal contradiction propagated to contract**

  **Status**: RESOLVED. The source spec's Required implementation behavior section was corrected to use `**platform` and `**device` (spec-critic item resolved). The contract now matches. ✓

### New in this review

- [x] **W-04: CONST-001 compliance grep pattern not updated to match spec fix**

  **Description**: The contract's CONST-001 compliance section (line 585) and Done criteria AC-018 (line 798) use the bare regex:
  ```
  grep -rnE '(SDL3|GL/|glad|glm)' src/cmd/
  ```
  This grep returns **non-zero matches** against the actual source code:
  ```
  src/cmd/commands/run_command.cpp:19:    return be::Backend::SDL3;
  src/cmd/commands/demo_command.cpp:22:    return be::Backend::SDL3;
  src/cmd/CMakeLists.txt:15:    message(STATUS "buddd: BUDDD_HAS_DISPLAY=ON (SDL3 backend)")
  ```
  (plus build artifacts in `CMakeFiles/`)

  None of these are CONST-001 violations — `be::Backend::SDL3` is an engine abstraction enum value, and the CMake messages are build system strings. However, the contract claims "zero matches" which is factually incorrect, and an implementer running this grep would incorrectly believe CONST-001 is violated.

  The source spec (SPEC-007) was already fixed for this exact issue (spec-critic W-07, resolved) by refining the regex to:
  ```
  grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/
  ```
  This refined regex correctly returns zero matches against both source code and build artifacts.

  **Fix**: Update both the CONST-001 compliance section (line 585) and Done criteria AC-018 (line 798) to use `#include.*(SDL3|GL/|glad|glm)` instead of the bare `(SDL3|GL/|glad|glm)`. This brings the contract in line with the already-fixed spec.

  **Impact**: Low (documentation inaccuracy, does not affect code correctness or implementability).

## Required changes

Concrete, actionable changes requested:

1. [x] **Fix W-04**: Updated the CONST-001 compliance section and Done criteria AC-018 to use `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` instead of the bare regex.

## Suggested improvements

Optional ideas (not required):

- None new in this review.

## Detailed analysis

### 1. Backend selection via `constexpr` IIFE — correctness

The contract uses:
```cpp
constexpr auto k_demo_backend = [] {
#ifdef BUDDD_HAS_DISPLAY
    return be::Backend::SDL3;
#else
    return be::Backend::Headless;
#endif
}();
```

This is a C++17-style immediately-invoked constexpr lambda. Since `be::Backend` is an `enum class` with enumerators `SDL3` and `Headless`, the return value is a valid constant expression. The preprocessor resolves `#ifdef` before constexpr evaluation, so the active branch is always a simple return statement. ✅ Correct C++.

The same pattern is used for `k_run_backend` in `run_command.cpp`. ✅

### 2. `#ifdef BUDDD_HAS_DISPLAY` behavior when not defined

When `BUDDD_HAS_DISPLAY` is **not defined** (CI build):
- `#ifdef` evaluates to false → `#else` branch → `return be::Backend::Headless;`
- The binary uses the headless backend for all commands
- Platform creation succeeds, `poll_events()` always returns `true`

When `BUDDD_HAS_DISPLAY` **is defined** (display available):
- `#ifdef` evaluates to true → `return be::Backend::SDL3;`
- The binary uses the SDL3 backend
- Window can be displayed, `poll_events()` returns `false` on window close ✅

### 3. Demo name validation order

`demo_command.cpp` validates the demo name **before** creating resources:
```cpp
// Line 51-55: Validate demo name BEFORE creating resources
if (demo_name != "triangle") {
    std::fprintf(stderr, "Unknown demo: '%s'\n\n", argv[2]);
    std::fwrite(k_demo_usage.data(), 1, k_demo_usage.size(), stderr);
    return EXIT_FAILURE;
}

// Line 58: Create platform, window, and render device
auto platform = be::Platform::create(k_demo_backend);
```

This matches the contract and the spec's requirement to "fail fast on CI without display." ✅

### 4. Test coverage

All unconditionally testable paths have corresponding `[cli]` tests in `tests/version_test.cpp`:

| Test | Present in code? | Required by |
|------|:---:|:---:|
| `buddd demo` with no name → usage + exit 1 | ✅ Line 160 | CONST-002 |
| `buddd demo unknownname` → error + exit 1 | ✅ Line 170 | CONST-002 |
| `buddd test` is unknown command → error + exit 1 | ✅ Line 180 | CONST-002 / AC-016 |
| `buddd demo triangle` completes | ✅ Line 220 | AC-007 |
| Help text checks for `demo` not `test` | ✅ Lines 128, 155 | AC-014 |
| `buddd` no-args → window message | ✅ Line 190 | AC-012 |
| `buddd version` → correct string | ✅ Line 113 | AC-013 |
| `buddd unknowncommand` → error + exit 1 | ✅ Line 133 | AC-015 |
| `buddd version extra_arg` → still version | ✅ Line 142 | AC-020 |
| `buddd help extra_arg` → still help | ✅ Line 149 | AC-021 |

### 5. File structure and include path correctness

| Include in code | Resolution | Correct? |
|---|---|---|
| `demo_command.h` from `commands/demo_command.cpp` | Same directory `commands/` | ✅ |
| `demo/triangle_demo.h` from `commands/demo_command.cpp` | Via `-Isrc/cmd` → `src/cmd/demo/triangle_demo.h` | ✅ |
| `demo/demo_helpers.h` from `demo/triangle_demo.cpp` | Via `-Isrc/cmd` → `src/cmd/demo/demo_helpers.h` | ✅ |
| `demo_helpers.h` from `demo/demo_helpers.cpp` | Same directory `demo/` (sibling) | ✅ |
| `platform/platform.h` from any `src/cmd/` file | Via engine include dir → `src/engine/platform/platform.h` | ✅ |
| `render/render_device.h` from any `src/cmd/` file | Via engine include dir → `src/engine/render/render_device.h` | ✅ |

### 6. CONST-001 preservation

No file under `src/cmd/` includes any SDL3, OpenGL, or GLM header. The architecture boundary is preserved. The use of `be::Backend::SDL3` in `demo_command.cpp` and `run_command.cpp` is an engine abstraction enum value, not a library header include — this is exactly what CONST-001 permits. ✅

(The grep verification command should be refined to avoid false positives from the enum values — see W-04.)

### 7. Edge case completeness

The contract's edge case table (lines 690-714) correctly reflects the headless backend behavior (updated per spec-critic B-04 resolution). Key headless-specific edge cases:

| Edge case | Contract says | Correct? |
|---|---|---|
| `buddd run` with no display | "Uses headless backend... runs until killed by timeout" | ✅ |
| `buddd demo triangle` with no display | "Uses headless backend... runs normally" | ✅ |
| `buddd run` window closed | "exits immediately" | ✅ |
| `buddd demo triangle` early abort | "Abort message, exits 0" | ✅ |

### 8. Done criteria completeness

All 36 Done criteria map to spec ACs (1-24), SCs (1-3), and additional verification items (25-36). Each is verifiable. The backend selection criteria are covered by:
- ✅ AC-029: BUDDD_HAS_DISPLAY propagation
- ✅ AC-030: Backend selection in commands (compile-time)
- ✅ AC-031: Demo name validated before resources
- ✅ AC-036: CI without display (headless build + tests)

## Review summary

| Category | Count |
|---|---|
| Blocking issues (new) | 0 |
| Warnings (new) | 1 (W-04: CONST-001 grep pattern) |
| Previously resolved blocking | 1 (B-01) — all ✅ |
| Previously resolved warnings | 3 (W-01, W-02, W-03) — all ✅ |
| Required changes | 2 (both for W-04) |
| Verdict | `Accepted with warnings` |

## Change log

| Review | Verdict | Key findings |
|---|---|---|
| 1st | `Rejected` | 1 blocking issue (B-01: single-dereference bug). 3 warnings (W-01: unnecessary include, W-02: misleading comment, W-03: spec contradiction). |
| 2nd (this) | `Accepted with warnings` | Previous B-01 **RESOLVED** ✅. Previous W-01, W-02, W-03 **RESOLVED** ✅. **New** 1 warning (W-04: CONST-001 grep pattern not updated to match spec fix). No blocking issues. |
