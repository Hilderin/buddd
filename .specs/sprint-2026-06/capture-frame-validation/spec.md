# SPEC-009 — Capture-Frame Validation: Auto-set `--frame` and Error on Mismatch

**Amendment to SPEC-008.** This spec supersedes the `--capture` / `--frame` interaction rules in SPEC-008 and adds `CaptureSpec::effective_frame()`.

## Problem

The current `--capture` / `--frame` interaction has three gaps:

1. **Captures silently never fire.** Running `buddd run cube --capture 120:/tmp/out.png` without `--frame 120` runs the app interactively (no frame limit). The render loop does not terminate automatically, so frame 120 is never reached unless the user keeps the window open. In headless/CI mode, the loop runs forever and the process never exits.

2. **Frame 1 capture with `--frame 1` silently produces nothing.** The OpenGL driver quirk forces frame 1 to be captured at frame 2, but `--frame 1` limits the loop to exactly 1 frame. The capture is silently skipped — no error, no output file, no indication of why.

3. **Driver quirk constant is duplicated.** The render loop in `app.cpp` contains the inline expression `(spec.frame < 2) ? 2 : spec.frame`. There is no single named method that both the capture logic and validation can reference, making the codebase inconsistent and harder to maintain.

## Goals

- **G-01**: Add `CaptureSpec::effective_frame()` that returns the 1-based frame number adjusted for the OpenGL driver quirk (minimum frame 2).
- **G-02**: When `--capture` is specified without `--frame`, auto-set `--frame` to the maximum `effective_frame()` among all captures, ensuring the application always terminates.
- **G-03**: When `--frame N` is explicitly set and `N < max_capture_effective_frame`, print an error to stderr and exit with code 1 before creating any windows or rendering.
- **G-04**: Preserve existing driver quirk behavior (frame 1 silently maps to frame 2, no warning emitted).
- **G-05**: All changes are testable via `parse_running_args()` and `CaptureSpec::effective_frame()` unit tests.

## Non-goals

- No change to the driver quirk itself (frame 1 still captures frame 2 silently).
- No change to `run_app()` capture logic beyond the frame-limit interaction.
- No change to `RenderSystem`, `App` interface, or any engine module.
- No change to scene dispatch, window creation, or app lifecycle.
- No additional CLI flags or syntax changes beyond the existing `--frame` and `--capture`.
- No change to how `--capture` parse errors are handled (SPEC-008 error behavior is unchanged).
- No change to the `"Captured: <path>"` output message format.
- No new warning or info output for the auto-set frame limit.

## Actors

| Actor | Description |
|---|---|
| Developer | A human running the `buddd` CLI from a terminal. Uses `buddd run [<scene>] --capture N:path` to capture rendered output. |
| CI system | Automated script running `buddd run <scene> --capture N:path` expecting deterministic termination without needing to specify `--frame` separately. |
| CLI maintainer | A developer who maintains `app_config.h/.cpp` (CaptureSpec, parse_running_args) and `app.cpp` (run_app render loop). |

## User-visible behavior

### 1. `CaptureSpec::effective_frame()` method

`CaptureSpec` gains a public const method:

```
int effective_frame() const;
```

Returns the 1-based frame number after adjusting for the OpenGL driver quirk (minimum frame 2). The logic is: `(frame < 2) ? 2 : frame`.

This method replaces the inline `(spec.frame < 2) ? 2 : spec.frame` expression currently used in `app.cpp`.

### 2. Auto-set `--frame` when `--capture` is given without `--frame`

If `parse_running_args()` encounters `--capture` entries but **no** `--frame` was explicitly given, it automatically sets `RunningArgs::frame_limit` to the maximum `effective_frame()` among all `CaptureSpec` entries.

| Command | Auto-set `frame_limit` | Rationale |
|---|---|---|
| `--capture 120:/tmp/out.png` | `120` | `effective_frame(120) = 120` |
| `--capture 1:/tmp/out.png` | `2` | `effective_frame(1) = 2` |
| `--capture 1:/tmp/a.png --capture 50:/tmp/b.png` | `50` | `max(2, 50) = 50` |
| `--capture 1:/tmp/a.png --capture 2:/tmp/b.png` | `2` | `max(2, 2) = 2` |
| `--capture 3:/tmp/a.png --capture 3:/tmp/b.png` | `3` | `max(3, 3) = 3` (identical effective frames) |

No output message is emitted for the auto-set — it is silent.

### 3. Error when `--frame` is explicitly set but too small

If the user explicitly provides both `--frame N` and one or more `--capture` entries, and `N < max_capture_effective_frame`, then `parse_running_args()` returns an error with the exact message:

```
Error: --frame N is too small for captures (need at least M)
```

Where `N` is the user-supplied frame limit and `M` is `max(entry.effective_frame() for entry in captures)`.

The caller (`main.cpp`):
- Prints this error to **stderr**.
- Exits with code **1**.
- Does **not** create any window, platform, or render device.
- Does **not** produce any output files.

No error is raised when `N >= max_capture_effective_frame`, or when `N == 0` (interactive with explicit `--frame 0`), or when `--frame` was not explicitly given (auto-set path applies instead).

| Command | `N` | `max_effective` | Result |
|---|---|---|---|
| `--frame 1 --capture 1:/tmp/out.png` | 1 | 2 | **Error**: "too small (need at least 2)" |
| `--frame 50 --capture 120:/tmp/out.png` | 50 | 120 | **Error**: "too small (need at least 120)" |
| `--frame 2 --capture 1:/tmp/out.png` | 2 | 2 | OK (2 >= 2) |
| `--frame 120 --capture 120:/tmp/out.png` | 120 | 120 | OK |
| `--frame 200 --capture 120:/tmp/out.png` | 200 | 120 | OK (200 >= 120) |
| `--frame 0 --capture 120:/tmp/out.png` | 0 | 120 | OK (0 = interactive / no limit) |

### 4. Existing driver quirk preserved

- `--capture 1:path` captures frame 2 (effective frame = 2). No warning is emitted.
- This is unchanged from SPEC-008.

### 5. Observability of the auto-set

The existing stderr message `"Scene started: <scene> (<frame_limit> frames)"` already reflects the (possibly auto-set) `frame_limit`. No additional observability is added.

## Key entities

### Updated `CaptureSpec` (`src/cmd/app_config.h`)

Only the `effective_frame()` method is added. No fields are changed, no struct is renamed.

```cpp
struct CaptureSpec {
    int frame;          // 1-based frame number (as provided by the user)
    std::string path;   // output PNG file path

    /// Returns the effective 1-based frame number after applying the OpenGL
    /// driver quirk. The minimum effective frame is 2 (frame 1 on some OpenGL
    /// drivers returns the clear colour, so it is silently bumped to frame 2).
    [[nodiscard]] int effective_frame() const {
        return (frame < 2) ? 2 : frame;
    }
};
```

### Updated `parse_running_args()` logic (in `app_config.cpp`)

The function must distinguish between "user explicitly passed `--frame`" and "user did not pass `--frame`". The default `frame_limit = 0` is shared with the explicit `--frame 0` case, so a boolean flag or sentinel is needed internally.

The validation rules are:

1. Parse all flags (same parsing as SPEC-008 for `--frame N`, `--capture N:path`).
2. If `--frame` was **not** explicitly given:
   - If there are captures: set `frame_limit = max(entry.effective_frame() for entry in captures)`.
   - If there are no captures: `frame_limit` stays `0` (interactive).
3. If `--frame` **was** explicitly given:
   - If `frame_limit > 0` (a numeric limit) and `frame_limit < max_capture_effective_frame`: return error.
   - If `frame_limit == 0` (explicit interactive): no validation, proceed.
   - Otherwise: use the user-supplied `frame_limit` as-is.

Return `RunningArgs` on success, or an `engine::Error` on validation failure.

### Unchanged entities

- `RunningArgs` struct (no field changes).
- `App` interface (no changes).
- `run_app()` signature and implementation (uses `RunningArgs.frame_limit` as before, which may now be auto-set).
- All `App` subclasses (no changes).

## User stories

### Story 1 — Auto-terminate when capturing without `--frame` (Priority: P1)

As a developer, I want to run `buddd run cube --capture 120:/tmp/out.png` without specifying `--frame` and have the app terminate automatically after frame 120 is rendered and captured.

**Given** the `buddd` binary is compiled
**When** I run `buddd run cube --capture 120:/tmp/out.png`
**Then** the app runs for 120 frames (auto-set frame limit), captures frame 120, and exits with code 0.

### Story 2 — Captures with frame 1 auto-terminate at frame 2 (Priority: P1)

As a developer, I want `buddd run cube --capture 1:/tmp/out.png` to terminate automatically and capture frame 2 (driver quirk), so that I don't have to know about the driver issue.

**Given** the `buddd` binary is compiled
**When** I run `buddd run cube --capture 1:/tmp/out.png`
**Then** the app runs for 2 frames (auto-set frame limit = 2), captures frame 2 to `/tmp/out.png`, and exits with code 0.

### Story 3 — Error when `--frame` is too small for captures (Priority: P1)

As a developer, I want a clear error message if I accidentally set `--frame 1 --capture 1:path`, so that I know why the capture didn't fire.

**Given** the `buddd` binary is compiled
**When** I run `buddd run cube --frame 1 --capture 1:/tmp/out.png`
**Then** an error message is printed to stderr and the process exits with code 1 without creating any window.

### Story 4 — Explicit `--frame` at or above max effective is OK (Priority: P2)

As a developer, I want to explicitly set a frame limit that covers all captures and have it work as expected.

**Given** the `buddd` binary is compiled
**When** I run `buddd run cube --frame 120 --capture 120:/tmp/out.png`
**Then** the app runs for 120 frames, captures frame 120, and exits with code 0.

**When** I run `buddd run cube --frame 200 --capture 120:/tmp/out.png`
**Then** the app runs for 200 frames, captures frame 120, and exits with code 0.

### Story 5 — Multiple captures with auto-set frame limit (Priority: P2)

As a developer, I want to capture multiple frames in one run without manually calculating the required frame limit.

**Given** the `buddd` binary is compiled
**When** I run `buddd run cube --capture 50:/tmp/a.png --capture 200:/tmp/b.png`
**Then** the app runs for 200 frames (auto-set), captures frames 50 and 200, and exits with code 0.

### Story 6 — `CaptureSpec::effective_frame()` is correct (Priority: P2)

As a developer, I want the `effective_frame()` method to return the right value for all valid frame numbers.

**Given** a `CaptureSpec` with `frame = 1`
**When** I call `effective_frame()`
**Then** it returns `2`.

**Given** a `CaptureSpec` with `frame = 2`
**When** I call `effective_frame()`
**Then** it returns `2`.

**Given** a `CaptureSpec` with `frame = 120`
**When** I call `effective_frame()`
**Then** it returns `120`.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `CaptureSpec` has a `[[nodiscard]] int effective_frame() const` method that returns `(frame < 2) ? 2 : frame`. | Compile and call with frame=0, 1, 2, 3, 120: returns 2 for <2, frame for >=2. |
| AC-002 | `parse_running_args()` without `--frame` and with one `--capture 120:/tmp/out.png` sets `frame_limit` to 120. | Call `parse_running_args()` with `["--capture", "120:/tmp/out.png"]` starting at index 0; verify `result.frame_limit == 120`. |
| AC-003 | `parse_running_args()` without `--frame` and with multiple `--capture` sets `frame_limit` to the max effective frame. | Call with `["--capture", "1:/tmp/a.png", "--capture", "50:/tmp/b.png"]`; verify `result.frame_limit == 50`. |
| AC-004 | `parse_running_args()` without `--frame` and `--capture 1:path` sets `frame_limit` to 2 (effective frame). | Call with `["--capture", "1:/tmp/out.png"]`; verify `result.frame_limit == 2`. |
| AC-005 | `parse_running_args()` with `--frame` explicitly given and `frame < max_effective` returns an error with the expected message. | Call with `["--frame", "1", "--capture", "1:/tmp/out.png"]`; verify error message contains `"too small"` and `"need at least 2"`. |
| AC-006 | `parse_running_args()` with `--frame` explicitly given and `frame >= max_effective` succeeds. | Call with `["--frame", "120", "--capture", "120:/tmp/out.png"]`; verify `result.frame_limit == 120`. |
| AC-007 | `parse_running_args()` with `--frame` explicitly given and `--capture 1:path` where `N=2` (>= max_effective=2) succeeds. | Call with `["--frame", "2", "--capture", "1:/tmp/out.png"]`; verify success. |
| AC-008 | `parse_running_args()` with `--frame 0` (explicit interactive) and captures succeeds without error. | Call with `["--frame", "0", "--capture", "120:/tmp/out.png"]`; verify `result.frame_limit == 0`. |
| AC-009 | `parse_running_args()` without any `--frame` or `--capture` keeps `frame_limit` as 0 (interactive). | Call with `[]` (no flags); verify `result.frame_limit == 0` and `result.captures.empty()`. |
| AC-010 | `parse_running_args()` with `--frame` explicitly given and no `--capture` uses the user-supplied frame limit. | Call with `["--frame", "60"]`; verify `result.frame_limit == 60`. |
| AC-011 | The effective-frame adjustment in `run_app()` uses the new `CaptureSpec::effective_frame()` method instead of the inline expression. | Inspect `app.cpp` — the render loop calls `spec.effective_frame()` and does NOT contain the raw expression `(spec.frame < 2) ? 2 : spec.frame`. |
| AC-012 | `--capture 1:path` still captures frame 2 (driver quirk preserved). No warning emitted. | Run `buddd run cube --capture 1:/tmp/ac12.png`; verify that `/tmp/ac12.png` is a valid PNG (contents of frame 2). stderr contains no warning about frame adjustment. |

## E2E Verification

- **Method**: Unit tests for `CaptureSpec::effective_frame()` and `parse_running_args()` validation/auto-set logic in `tests/cmd_tests.cpp` (tagged `[cli]`). Integration test: run `buddd run cube --capture 1:/tmp/e2e.png` and verify the PNG is created and the process exits with code 0.

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | `buddd run cube --capture 120:/tmp/out.png` (without `--frame`) terminates automatically and produces a valid PNG of frame 120. | Run command; verify exit code 0, verify `/tmp/out.png` is a valid PNG. |
| SC-002 | `buddd run cube --frame 1 --capture 1:/tmp/out.png` prints a clear error and exits with code 1 without creating a window. | Run command; verify stderr contains `"Error: --frame 1 is too small for captures (need at least 2)"`; verify exit code 1; verify no `Window opened` message. |
| SC-003 | All existing CLI tests (version, help, unknown command, capture without frame number) still pass. | Run the test suite; existing `[cli]` tests pass unchanged. |

## Edge cases

| Case | Expected behavior |
|---|---|
| `--capture` without `--frame` and captures list is empty | Impossible by construction (no `--capture` flags → captures list is empty). No auto-set. Default `frame_limit = 0` (interactive). |
| `--capture` with `--frame N` where `N` equals `max_effective_frame` exactly | OK. No error. Frame limit is exactly enough to cover all captures. |
| `--capture 1:path` (frame 1) with auto-set | Auto-sets `frame_limit = 2`. Render loop runs 2 frames. Frame 2 captured. |
| `--capture` with all same frame number | `max_effective_frame` is that frame's effective value. Auto-set uses it. No deduplication needed. |
| `--frame N` plus `--capture` where `N` is zero (explicit interactive) | OK — frame limit 0 means "no limit". No error, no auto-set. Captures fire when reached. |
| `--frame N` plus `--capture` where `N` is less than 1 (invalid `--frame`) | SPEC-008 error behavior takes precedence (invalid frame value error, exit 1). The capture-frame validation is never reached. |
| Multiple `--capture` with mixed effective frames | Auto-set uses `max(entry.effective_frame() for entry in captures)`. |
| `--capture 1:path --capture 1:path` (duplicate path) | Both stored; auto-set uses `max(2,2) = 2`. Render loop runs 2 frames. Both are processed. |
| `--capture` path is a directory | SPEC-008 error behavior unchanged: `Image::save()` fails, error on stderr, continues. |
| No `--capture`, no `--frame` | Unchanged from SPEC-008: `frame_limit = 0`, interactive. |

## Error cases

| Case | Expected behavior |
|---|---|
| `--frame N < max_capture_effective_frame` (N >= 1) | `parse_running_args()` returns error: `"Error: --frame N is too small for captures (need at least M)"`. Exit code 1. No window created. |
| `--frame 1 --capture 1:path` | Specific instance of above. Error: `"Error: --frame 1 is too small for captures (need at least 2)"`. Exit code 1. |
| Existing `--capture` parse errors (invalid frame, missing N:) | Unchanged from SPEC-008. Error takes precedence over capture-frame validation. |

## Permissions and security

- No changes to permissions or security model from SPEC-008.
- The `effective_frame()` method is a pure computation with no side effects.
- The auto-set and validation logic operate entirely on parsed CLI arguments before any platform/rendering code runs.
- No new file I/O or network access.

## Observability

| Signal | Source | Type of change |
|---|---|---|
| `"Error: --frame N is too small for captures (need at least M)"` | stderr | **New.** Printed when validation fails, before any window/rendering. |
| `"Scene started: <scene> (N frames)"` | stderr | **Updated (implicitly).** The `N` may now be auto-set from captures. No format change. |
| All other observability | stderr/stdout | **Unchanged** from SPEC-008. |

## Out of scope

- Any changes to the OpenGL driver quirk itself.
- Any changes to `run_app()` capture logic beyond frame-limit interaction.
- Any changes to `RenderSystem`, `App` interface, or engine modules.
- Any new CLI flags or syntax.
- Any new warning when the driver quirk adjusts frame 1 to frame 2.
- Any UI or interactive feedback (pop-ups, dialogs).
- Any changes to help text (the `--frame` description already states "default: interactive"; no need to mention auto-set).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `parse_running_args()` can distinguish "user explicitly passed `--frame`" from "user did not pass `--frame`" using an internal boolean or sentinel (e.g., `bool frame_explicit = false` toggled to `true` when `--frame` is encountered). The `RunningArgs` struct itself does not need a new field — this is an internal implementation detail. |
| A-02 | When `--frame 0` is explicitly given (interactive/no limit) together with `--capture`, no validation error is raised. The user intends to run interactively with captures firing whenever the frame count reaches the capture point. The app still runs until the window is closed or ESC is pressed (for interactive scenes). |
| A-03 | In headless mode (`BUDDD_HAS_DISPLAY=OFF`), `poll_events()` never returns false. The auto-set frame limit is the only mechanism for termination when captures are requested without an explicit `--frame`. |
| A-04 | The `effective_frame()` method is `const` and `[[nodiscard]]`, and has no observable side effects. |
| A-05 | Existing SPEC-008 AC items for `parse_running_args()` (AC-005, AC-006, AC-025, AC-026) remain valid for inputs not involving capture-frame validation. |
| A-06 | The existing SPEC-008 Assumption A-11 ("minimum capture frame workaround matches existing behavior") is preserved. |
| A-07 | The test framework (Catch2, tests in `tests/cmd_tests.cpp`) supports parameterized or multiple test cases covering the new validation logic. |
| A-08 | The auto-set frame limit only applies when captures are present AND `--frame` is absent. If the user explicitly passes `--frame 60` and captures, the user-supplied limit is used (subject to validation). |
| A-09 | The error message format `"Error: --frame N is too small for captures (need at least M)"` is fixed (no i18n, no templating). |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | Should the auto-set frame limit be logged or visible to the user? | **No.** The auto-set is silent. The existing `"Scene started"` message already reflects the frame count. |
| Q-02 | What if `--capture` specifies frames beyond what the scene should render (e.g., `--capture 5000:path` for a 120-frame scene)? | **No validation.** The frame limit is auto-set to 5000, the loop runs 5000 frames (repetitive rendering of the same animation). This matches SPEC-008 behavior where `--frame` could be set arbitrarily high. |
| Q-03 | Should `--capture 0:path` (frame 0, invalid per SPEC-008) interact with capture-frame validation? | **No.** SPEC-008 already errors on frame 0 as invalid input. The capture-frame validation is only reached when captures parse successfully. |
