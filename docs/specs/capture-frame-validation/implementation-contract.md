# IMPL-009 — Capture-Frame Validation: Auto-set `--frame` and Error on Mismatch

## Source spec

`docs/specs/capture-frame-validation/spec.md` (SPEC-009), accepted. The spec-critic review (`docs/specs/capture-frame-validation/spec-critic.md`) found no blocking issues. Verdict: accepted.

## Goal

Add a `CaptureSpec::effective_frame()` method that encapsulates the OpenGL driver quirk (minimum frame 2). Update `parse_running_args()` in `app_config.cpp` to (a) auto-set `frame_limit` to the max effective frame across captures when `--frame` is not explicitly given, and (b) return an error when an explicit `--frame N` is smaller than the max capture effective frame. Refactor `run_app()` in `app.cpp` to use `spec.effective_frame()` instead of the inline quirk expression. All changes are testable via unit tests on `parse_running_args()` and `CaptureSpec::effective_frame()`.

## Non-goals

- No change to the driver quirk itself (frame 1 still silently maps to frame 2, no warning emitted).
- No change to `run_app()` capture logic beyond replacing the inline quirk expression.
- No change to `RenderSystem`, `App` interface, or any engine module.
- No change to scene dispatch, window creation, or app lifecycle.
- No additional CLI flags or syntax changes beyond the existing `--frame` and `--capture`.
- No change to how `--capture` parse errors are handled (SPEC-008 error behavior unchanged).
- No change to the `"Captured: <path>"` output message format.
- No new warning or info output for the auto-set frame limit.
- No change to `main.cpp` — validation happens inside `parse_running_args()`.
- No change to `RunningArgs` struct layout or fields.

## Relevant constitution rules

- **CONST-002** (`docs/constitution/rules/CONST-002-testing-policy.md`): All testable code must have corresponding tests. New `CaptureSpec::effective_frame()` and the updated `parse_running_args()` validation logic must be covered by unit tests.

## Relevant ADRs

- **ADR-014** (`docs/adr/014-cli-app-system.md`): Documents the OpenGL driver quirk (frame 1 → frame 2) and the centralised render loop. The quirk is documented in section "Negative" bullet 6 (frame numbering dualism) and is referenced by the wiki business-rules.md. This contract preserves the quirk behavior exactly.
- **ADR-001** (`docs/adr/001-result-error-pattern.md`): `Result<T>` pattern for fallible APIs. The new validation in `parse_running_args()` returns errors via the existing `engine::Result<RunningArgs>` pattern.

## Files to inspect

Before editing, the Code Agent must read these files:

| File | What to look for |
|---|---|
| `src/cmd/app_config.h` | Existing `CaptureSpec` struct — must add `effective_frame()` method. Existing `parse_running_args()` declaration — signature unchanged. |
| `src/cmd/app_config.cpp` | Existing `parse_running_args()` implementation — must add frame-explicit tracking, auto-set, and validation. |
| `src/cmd/app.cpp` | `run_app()` implementation — line 120 contains the inline quirk expression `(spec.frame < 2) ? 2 : spec.frame` that must be replaced with `spec.effective_frame()`. |
| `tests/cli_app_tests.cpp` | Existing test structure, `run_buddd()` helper, tag conventions (`[cli][app]`). |
| `docs/specs/cli-app-system/implementation-contract.md` | Edge case table rows 928–929 (frame > frame_limit and --capture 1:path) — must be updated to reference the new validation. |

## Files allowed to change

Only the following files may be modified. One new test file may be created.

| # | File | Change |
|---|---|---|
| 1 | `src/cmd/app_config.h` | Add `[[nodiscard]] int effective_frame() const` method to `CaptureSpec` struct. |
| 2 | `src/cmd/app_config.cpp` | Add `bool frame_explicit = false` tracking; after parsing loop, add auto-set and validation logic. |
| 3 | `src/cmd/app.cpp` | Replace line 120 `(spec.frame < 2) ? 2 : spec.frame` with `spec.effective_frame()`. |
| 4 | `tests/capture_frame_tests.cpp` | **New file**: unit tests for `effective_frame()`, auto-set, and validation error/success cases. |
| 5 | `docs/specs/cli-app-system/implementation-contract.md` | Update edge case rows 928–929 to reflect that the validation is now in `parse_running_args()`. |

## Files forbidden to change

- `src/cmd/main.cpp` — no changes needed; validation happens inside `parse_running_args()`.
- `src/cmd/app.h` — no change to `App` interface or `run_app()` declaration.
- `src/cmd/apps/` — no App subclass changes.
- `src/engine/` — no engine changes.
- `src/cmd/CMakeLists.txt` — no build system changes.
- `src/cmd/commands/` — no command changes.
- `src/cmd/demo/` — no demo helper changes.

## Existing conventions to follow

1. **`[[nodiscard]]`**: `effective_frame()` and all `Result<T>`-returning functions are `[[nodiscard]]`.
2. **Include guard**: `#pragma once` in all headers.
3. **Namespace**: `buddd::cmd` for `CaptureSpec`/`RunningArgs`/`parse_running_args()`.
4. **Trailing return type**: `auto foo() -> int` style for all new functions.
5. **Error formatting**: Use `be::make_error(be::Error::Category::InvalidArgument, ...)` with a string message matching the exact format from SPEC-009.
6. **Error message format**: `"Error: --frame N is too small for captures (need at least M)"` — exact string, no i18n.
7. **Test style**: Catch2 test framework. Use `run_buddd()` helper for subprocess tests (existing pattern) and direct function calls for unit tests.
8. **Tagging**: Use `[cli][app]` for existing integration tests; use `[cli][capture]` for new capture-specific tests.

## Required implementation behavior

### 1. Add `CaptureSpec::effective_frame()` to `app_config.h`

```cpp
struct CaptureSpec {
    int frame;          // 1-based
    std::string path;

    /// Returns the effective 1-based frame number after applying the OpenGL
    /// driver quirk. The minimum effective frame is 2 (frame 1 on some OpenGL
    /// drivers returns the clear colour, so it is silently bumped to frame 2).
    [[nodiscard]] int effective_frame() const {
        return (frame < 2) ? 2 : frame;
    }
};
```

**Requirements:**
- Method is `const`, `[[nodiscard]]`, inline in the header (no `.cpp` change needed).
- Logic: `(frame < 2) ? 2 : frame`.
- No side effects, no I/O, no exceptions.
- No change to existing fields `frame` and `path`.

### 2. Update `parse_running_args()` in `app_config.cpp`

**Tracking whether --frame was explicitly given:**

Add a local `bool frame_explicit = false;` before the parsing loop. When `--frame` is parsed (existing `if (arg == "--frame")` block), set `frame_explicit = true` at the same time as setting `args.frame_limit`.

**After the parsing loop**, add validation logic:

```cpp
// Post-parse validation for capture-frame interaction
if (!args.captures.empty()) {
    // Compute max effective frame across all captures
    int max_effective = 0;
    for (const auto& spec : args.captures) {
        int eff = spec.effective_frame();
        if (eff > max_effective)
            max_effective = eff;
    }

    if (!frame_explicit) {
        // Auto-set frame_limit to max effective frame
        args.frame_limit = max_effective;
    } else if (args.frame_limit > 0 && args.frame_limit < max_effective) {
        // Error: explicit --frame is too small
        std::string msg = "Error: --frame "
            + std::to_string(args.frame_limit)
            + " is too small for captures (need at least "
            + std::to_string(max_effective) + ")";
        return be::make_error(
            be::Error::Category::InvalidArgument, std::move(msg));
    }
    // If frame_explicit && args.frame_limit == 0 (explicit interactive):
    // no validation, proceed with frame_limit == 0.
    // If frame_explicit && args.frame_limit >= max_effective: OK, use as-is.
}
```

**Requirements:**
- `bool frame_explicit` starts as `false`, set to `true` ONLY when `--frame` is successfully parsed.
- The auto-set (when `!frame_explicit` and captures non-empty) silently sets `args.frame_limit = max_effective` — no output message.
- The error message format must match EXACTLY: `"Error: --frame N is too small for captures (need at least M)"`, where `N` is the user-supplied value and `M` is `max_effective`.
- When `--frame 0` is explicitly given (explicit interactive), skip the validation (no error).
- When `frame_explicit` is true but there are no captures, skip the validation (no-op for frame_limit).
- The validation must happen AFTER all args are parsed (not incrementally).
- Return type unchanged: `engine::Result<RunningArgs>`.

### 3. Refactor `run_app()` in `app.cpp`

In `src/cmd/app.cpp`, line 120, replace:
```cpp
int effective_frame = (spec.frame < 2) ? 2 : spec.frame;
```
with:
```cpp
int effective_frame = spec.effective_frame();
```

**Requirements:**
- No other changes to `run_app()`.
- Include `app_config.h` is already included (via `app.h` which includes `app_config.h`).
- The behavior is identical — the same logic, just using the named method.

### 4. Update `docs/specs/cli-app-system/implementation-contract.md`

In the edge case table, update rows 928–929:

**Row 928 (currently: "Frame-limited run with --capture where frame > frame_limit"):**
- After IMPL-009, this scenario cannot arise silently. If `--frame` is explicit and `frame < max_effective`, `parse_running_args()` returns an error (exit 1, run never starts). If `--frame` is auto-set (no explicit `--frame`), `frame_limit = max_effective`, so the capture always fires. Update the row to: "Impossible after IMPL-009: explicit `--frame < max_effective` is now an error; auto-set guarantees capture always fires."

**Row 929 (currently: `--capture 1:path` (frame 1)):**
- The current text is: "Driver quirk: `run_app()` forces minimum frame to 2; frame 1 is silently skipped and frame 2 is captured instead. No warning."
- Update to: "Driver quirk: `CaptureSpec::effective_frame()` returns 2 for frame 1. If `--frame` is auto-set, `frame_limit` becomes 2. If `--frame` is explicitly 1, `parse_running_args()` returns error: `'Error: --frame 1 is too small for captures (need at least 2)'`."

## Required tests

### Unit tests for `CaptureSpec::effective_frame()` (direct function calls)

| ID | Test Name | Verification |
|---|---|---|
| EF-01 | `effective_frame(0) returns 2` | `CaptureSpec{0, ""}.effective_frame() == 2` |
| EF-02 | `effective_frame(1) returns 2` | `CaptureSpec{1, ""}.effective_frame() == 2` |
| EF-03 | `effective_frame(2) returns 2` | `CaptureSpec{2, ""}.effective_frame() == 2` |
| EF-04 | `effective_frame(3) returns 3` | `CaptureSpec{3, ""}.effective_frame() == 3` |
| EF-05 | `effective_frame(120) returns 120` | `CaptureSpec{120, ""}.effective_frame() == 120` |

### Unit tests for `parse_running_args()` capture-frame validation

These tests call `parse_running_args()` directly (no display needed). Each test constructs a `std::vector<const char*>` arg vector and calls `parse_running_args()`, then checks the result (success or error).

| ID | Test Name | Tags | Input | Expected |
|---|---|---|---|---|
| CF-01 | `auto-set frame_limit from single capture` | `[cli][capture]` | `["--capture", "120:/tmp/out.png"]` from start=0 | `result.frame_limit == 120, result.captures.size() == 1` |
| CF-02 | `auto-set frame_limit from frame-1 capture (effective_frame=2)` | `[cli][capture]` | `["--capture", "1:/tmp/out.png"]` | `result.frame_limit == 2` |
| CF-03 | `auto-set frame_limit from max of multiple captures` | `[cli][capture]` | `["--capture", "1:/tmp/a.png", "--capture", "50:/tmp/b.png"]` | `result.frame_limit == 50` |
| CF-04 | `error when explicit --frame < max_effective` | `[cli][capture]` | `["--frame", "1", "--capture", "1:/tmp/out.png"]` | Returns error; `be::to_string(error)` contains `"too small"` and `"need at least 2"` |
| CF-05 | `error when explicit --frame 50 with capture 120` | `[cli][capture]` | `["--frame", "50", "--capture", "120:/tmp/out.png"]` | Returns error; message contains `"need at least 120"` |
| CF-06 | `success when explicit --frame equals max_effective` | `[cli][capture]` | `["--frame", "120", "--capture", "120:/tmp/out.png"]` | `result.frame_limit == 120, success` |
| CF-07 | `success when explicit --frame exceeds max_effective` | `[cli][capture]` | `["--frame", "200", "--capture", "120:/tmp/out.png"]` | `result.frame_limit == 200, success` |
| CF-08 | `success when explicit --frame >= effective_frame for capture 1` | `[cli][capture]` | `["--frame", "2", "--capture", "1:/tmp/out.png"]` | `result.frame_limit == 2, success` |
| CF-09 | `success when --frame 0 (explicit interactive) with captures` | `[cli][capture]` | `["--frame", "0", "--capture", "120:/tmp/out.png"]` | `result.frame_limit == 0, success` |
| CF-10 | `no auto-set when no captures present` | `[cli][capture]` | `[]` (no flags) | `result.frame_limit == 0, result.captures.empty()` |
| CF-11 | `explicit --frame without captures uses user value` | `[cli][capture]` | `["--frame", "60"]` | `result.frame_limit == 60, captures empty` |
| CF-12 | `auto-set with both captures having same effective frame (both 3)` | `[cli][capture]` | `["--capture", "3:/tmp/a.png", "--capture", "3:/tmp/b.png"]` | `result.frame_limit == 3` |
| CF-13 | `auto-set with captures having mixed effective frames (1 and 2)` | `[cli][capture]` | `["--capture", "1:/tmp/a.png", "--capture", "2:/tmp/b.png"]` | `result.frame_limit == 2` |

### Test linkage to acceptance criteria

| AC ID | Test(s) |
|---|---|
| AC-001 (effective_frame returns correct value) | EF-01 through EF-05 |
| AC-002 (auto-set from single capture) | CF-01 |
| AC-003 (auto-set from multiple captures) | CF-03 |
| AC-004 (auto-set frame 1 → 2) | CF-02 |
| AC-005 (error when --frame too small) | CF-04, CF-05 |
| AC-006 (success when --frame >= max_effective) | CF-06, CF-07 |
| AC-007 (success with --frame 2 + capture 1) | CF-08 |
| AC-008 (--frame 0 + captures succeeds) | CF-09 |
| AC-009 (no flags → default 0) | CF-10 |
| AC-010 (--frame without captures unchanged) | CF-11 |
| AC-011 (run_app uses effective_frame()) | Code inspection of app.cpp (manual) |
| AC-012 (driver quirk preserved) | EF-02, EF-03, CF-02 (effective_frame returns 2, auto-set works) |

### E2E / Integration verification

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| CI-01 | auto-set frame from capture (success) | `[cli][capture]` | Use `temp_filename("buddd_capture_ci01")` for output path. Run `buddd run cube --capture 120:<path>` with headless backend; verify exit code 0. |
| CI-02 | explicit --frame too small (error) | `[cli][capture]` | Use `temp_filename("buddd_capture_ci02")` for output path. Run `buddd run cube --frame 1 --capture 1:<path>`; verify stderr contains `"too small"`, exit code 1. |

## Edge cases

All edge cases from SPEC-009 must be handled:

| Case | Expected behavior |
|---|---|
| `--capture` without `--frame` and no captures | Impossible (no `--capture` → captures empty). No auto-set. Default `frame_limit = 0`. |
| `--capture` with `--frame N` where N equals max_effective exactly | OK. No error. |
| `--capture 1:path` with auto-set | Auto-sets `frame_limit = 2`. Render loop runs 2 frames. Frame 2 captured. |
| `--capture` with all same frame number | `max_effective_frame` is that frame's effective value. Auto-set uses it. |
| `--frame 0` (explicit interactive) plus `--capture` | OK — frame limit 0 means "no limit". No error, no auto-set. Captures fire when reached. |
| `--frame N` (explicit) plus `--capture` where N is invalid (< 1) | SPEC-008 error behavior takes precedence (invalid frame error). Capture-frame validation never reached. |
| Multiple `--capture` with mixed effective frames | Auto-set uses `max(effective_frame() for each entry)`. |
| `--capture 1:path --capture 1:path` (duplicate paths) | Both stored; max = 2. Auto-set `frame_limit = 2`. Both processed. |
| `--capture` path is a directory | SPEC-008 error behavior unchanged (`Image::save()` fails, prints error, continues). |
| No `--capture`, no `--frame` | Unchanged: `frame_limit = 0`, interactive. |
| Existing `--capture` parse errors (invalid frame, missing N:) | Unchanged from SPEC-008. Error takes precedence over capture-frame validation. |
| `--frame` explicit but no captures | No validation. `frame_limit` used as-is. |

## Security impact

- No changes to permissions or security model from SPEC-008.
- `effective_frame()` is a pure computation with no side effects.
- The auto-set and validation logic operate entirely on parsed CLI arguments before any platform/rendering code runs.
- No new file I/O or network access.

## Data and migration impact

None. No schema changes, data migrations, seed data, or persistent state.

## API compatibility impact

- **Backward-compatible**: `CaptureSpec` gains a new public const method `effective_frame()`. Existing code that reads `CaptureSpec::frame` is unaffected.
- **Backward-compatible**: `parse_running_args()` signature unchanged. New behavior activates automatically.
- **Behavioral change**: Calling `parse_running_args()` with `--capture` and without `--frame` now sets `frame_limit` to the max effective frame (previously left as 0).
- **Behavioral change**: Calling `parse_running_args()` with `--frame N` and `--capture` where N < max effective frame now returns an error (previously succeeded silently, and the capture would never fire).

## Documentation impact

- `docs/specs/cli-app-system/implementation-contract.md`: Update edge case rows 928–929 to reflect new validation.
- Wiki (`docs/wiki/domain/business-rules.md`): Already documents the driver quirk (lines 100–102). The wiki-agent should verify consistency after implementation, but no change is required — the wiki's "effective_frame" description is already accurate (it documents the same formula).
- No README changes required.

## ADR impact

- **ADR-014** (CLI App System): The driver quirk documentation in ADR-014 is still accurate. No changes needed. The `effective_frame()` method formalises what was previously an inline expression in `run_app()`, consistent with the ADR's intent.
- No new ADR is required. All design decisions are resolved in SPEC-009.

## Constitution impact

No constitution changes are required. CONST-002 (testing policy) is satisfied by the test requirements above.

## Done criteria

The contract is done when ALL of the following are satisfied:

### Code changes
- [ ] `src/cmd/app_config.h`: `CaptureSpec` has `[[nodiscard]] int effective_frame() const` method with logic `(frame < 2) ? 2 : frame`.
- [ ] `src/cmd/app_config.cpp`: `parse_running_args()` tracks `frame_explicit`, auto-sets `frame_limit` when `--frame` absent and captures present, and returns error when explicit `--frame < max_effective` and `frame_limit > 0`.
- [ ] `src/cmd/app.cpp`: The render loop in `run_app()` calls `spec.effective_frame()` instead of the inline expression `(spec.frame < 2) ? 2 : spec.frame`.
- [ ] `src/cmd/app_config.cpp` does NOT contain the raw expression `(spec.frame < 2) ? 2 : spec.frame` — only the method `effective_frame()` is used for quirk logic.
- [ ] `src/cmd/app.cpp` does NOT contain the raw expression `(spec.frame < 2) ? 2 : spec.frame` — only `spec.effective_frame()` is used.

### Tests
- [ ] Unit tests EF-01 through EF-05 pass (effective_frame values).
- [ ] Unit tests CF-01 through CF-13 pass (auto-set, validation errors, edge cases).
- [ ] Integration tests CI-01 and CI-02 pass (or are verified as correct).
- [ ] All existing tests still pass unchanged.

### Documentation
- [ ] `docs/specs/cli-app-system/implementation-contract.md` edge case rows 928–929 updated to reflect new validation behavior.

### Build
- [ ] `cmake --build --preset debug` succeeds with no errors.
