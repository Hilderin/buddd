# Code Review: SPEC-010 — Framebuffer Capture

## Status

`Rejected`

## Summary

**Previous verdict: `Accepted` (initial review)**

This is a **re-review after debugging changes** applied to the original implementation. The debugging changes introduced several spec and implementation contract violations that were not present in the initial review:

1. **Camera position changed** from spec-mandated `(0,0,3)` to `(3,2,3)` (matching cube demo).
2. **Render loop with rotation animation** added, violating spec's `angle = 0` and "exactly one frame" requirements.
3. **Minimum 2-frame driver quirk workaround** forces at least 2 frames internally, contradicting "exactly one frame".
4. **`--frame N` parameter** adds multi-frame capture support, violating the explicit non-goal "No multi-frame capture or video recording".
5. **AC-015 regression**: The extra-args warning (`Warning: unexpected arguments after 'capture cube': ...`) was removed when `--frame` parsing was added. Unexpected positional args are now silently consumed as the output path.

All 190 tests still pass (including CT-01, CT-02, CT-03, IT-01 through IT-12, RT-01). CONST-001 is respected (no SDL3/OpenGL/GLM in `src/cmd/`).

However, the accumulated spec deviations are significant enough to warrant rejection of the current state. The fixups introduced pragmatic workarounds that fundamentally change the specified behavior. A spec update or implementation contract amendment is required before this can be re-accepted.

## Spec compliance

| AC ID | Description | Status (prev) | Status (now) | Change |
|-------|-------------|---------------|--------------|--------|
| AC-001 | `ImageBuffer` aggregate exists | ✅ | ✅ | Unchanged |
| AC-002 | `Image` class exists with `create()`, `load()`, `save()`, accessors | ✅ | ✅ | Unchanged |
| AC-003 | `Image::create()` validates positive dimensions and data size | ✅ | ✅ | Unchanged |
| AC-004 | `Image::create()` flips rows vertically (bottom-left → top-left) | ✅ | ✅ | Unchanged |
| AC-005 | `Image::save()` writes valid PNG, `Image::load()` round-trips | ✅ | ✅ | Unchanged |
| AC-006 | `RenderDevice::read_pixels()` pure virtual | ✅ | ✅ | Unchanged |
| AC-007 | OpenGL `read_pixels()` calls `glReadPixels` with `GL_PACK_ALIGNMENT=1` | ✅ | ✅ | Unchanged |
| AC-008 | Headless `read_pixels()` returns `Unsupported` error | ✅ | ✅ | Unchanged |
| AC-009 | `CaptureCommand` files exist | ✅ | ✅ | Unchanged |
| AC-010 | `main.cpp` has `"capture"` dispatch branch | ✅ | ✅ | Unchanged |
| AC-011 | `buddd capture cube /path` produces valid PNG, exit 0 | ✅ | ⚠️ | Manual — camera pos changed, may produce different output |
| AC-012 | `buddd capture cube` default output path | ✅ | ⚠️ | Manual — internal min 2 frames changes behavior |
| AC-013 | `buddd capture` (no args) prints usage, exits 1 | ✅ | ✅ | CT-01 still passes |
| AC-014 | `buddd capture unknown_scenario` prints error, exits 1 | ✅ | ✅ | CT-02 still passes |
| AC-015 | `buddd capture cube extra_arg` prints warning, still captures | ✅ | ❌ | **REGRESSION**: Extra args warning removed; `extra_arg` consumed as output path |
| AC-016 | Scenario validated BEFORE platform creation (fails fast) | ✅ | ✅ | Unchanged |
| AC-017 | Captured PNG is 800×600 | ✅ | ✅ | Window size unchanged |
| AC-018 | Build succeeds with stb via FetchContent | ✅ | ✅ | Unchanged |
| AC-019 | No SDL3/OpenGL/GLM includes in `src/cmd/` | ✅ | ✅ | Confirmed via grep — zero matches |
| AC-020 | Existing demos unchanged and working | ✅ | ✅ | Frame cycle tests pass; `begin_frame()` now sets clear color which all renderers pick up (cosmetic, not behavioral) |
| AC-021 | `Error::Category::ReadbackFailed` exists, `to_string()` handles it | ✅ | ✅ | Unchanged |
| AC-022 | `capture/*.cpp` in CMake glob | ✅ | ✅ | Unchanged |
| AC-023 | `Image::load()` returns error for non-existent file | ✅ | ✅ | IT-06 still passes |
| AC-024 | `capture_cube_scene()` reuses `setup_cube()` | ✅ | ✅ | Still includes `"demo/demo_helpers.h"` |
| AC-025 | `buddd help` includes `capture` | ✅ | ✅ | CT-03 still passes |

### Broken ACs

| AC | Failure |
|----|---------|
| **AC-015** | Extra args warning (`Warning: unexpected arguments after 'capture cube': ...`) is no longer emitted. Unexpected positional arguments are silently consumed as the output path by `parse_output_path()`. The original lines 103-110 were removed during the `--frame` parameter refactoring. |

### Modified ACs (behavior changed, not in spec)

| AC | Description of change |
|----|----------------------|
| AC-011 | Camera changed from `(0,0,3)` to `(3,2,3)` — output image will show an isometric view instead of a straight-on front view. This contradicts the spec's explicit statement that "the capture scenario is not a demo, it produces a reference image" and the camera note distinguishing capture from the cube demo. |
| AC-012 | Captures the Nth frame after rendering N frames (with rotation animation), not the first/only frame. Default: renders 2 frames internally and captures the 2nd. This contradicts "exactly one frame" (A-08) and "angle = 0" (A-13). |

## Contract compliance

### Files allowed to change — new files (9/9 created, unchanged)

No changes to the set of new files. All 9 files from the contract exist and compile.

### Files allowed to change — modified (11/11 modified, 4 modified again by debugging changes)

| # | File | Change | Status |
|---|------|--------|--------|
| 1 | `src/engine/error.h` | Add `ReadbackFailed`, `IoFailed` before `Unsupported` | ✅ Unchanged |
| 2 | `src/engine/render/render_device.h` | Add `#include "image/image_buffer.h"`, add `read_pixels()` pure virtual | ✅ Unchanged |
| 3 | `src/engine/render/render_device_opengl.h` | Add `read_pixels()` override | ✅ Unchanged |
| 4 | `src/engine/render/render_device_opengl.cpp` | **Added `glReadBuffer(GL_BACK)`** in `read_pixels()`; **added `glClearColor(0.02f, 0.02f, 0.05f, 1.0f)`** in `begin_frame()` | ⚠️ See issues |
| 5 | `src/engine/render/render_device_headless.h` | Add `read_pixels()` override | ✅ Unchanged |
| 6 | `src/engine/render/render_device_headless.cpp` | Implement `read_pixels()` returning `Unsupported` error | ✅ Unchanged |
| 7 | `src/engine/CMakeLists.txt` | Add stb FetchContent + PRIVATE include | ✅ Unchanged |
| 8 | `src/cmd/main.cpp` | Add capture dispatch branch | ✅ Unchanged |
| 9 | `src/cmd/CMakeLists.txt` | Add `capture/*.cpp` to glob | ✅ Unchanged |
| 10 | `src/cmd/commands/help_command.h` | Add `capture` to `k_usage_text` | ✅ Unchanged |
| 11 | `tests/cmd_tests.cpp` | Add CT-01, CT-02, CT-03 | ✅ Unchanged |

### Files forbidden to change

No files outside the allowed set were modified. ✅

### Contract requirements — detailed checks (changes only)

| Requirement | Contract says | Impl says | Status |
|-------------|---------------|-----------|--------|
| Extra args warning follows `demo_command.cpp` pattern | Lines 589-596 of contract | ❌ **Missing** — removed during `--frame` refactoring | ❌ |
| Camera at `(0,0,3)` | Contract section 13 line 719 | `Vec3{3.0f, 2.0f, 3.0f}` at `cube_capture.cpp:33` | ❌ |
| Rotation angle = 0 (no rotation) | Contract section 13 line 737 | `Mat4::rotate(angle, ...)` with `angle = elapsed_seconds * 0.5f` | ❌ |
| Render exactly one frame | Contract section 13, spec A-08 | Render loop with `effective_frames` (minimum 2) | ❌ |
| `capture_cube_scene` signature: 4 params | Contract section 12: `(Platform&, RenderDevice&, int, int)` | 5 params: `(..., int num_frames)` | ❌ |
| `glReadPixels` called with `GL_PACK_ALIGNMENT=1` | Contract section 6 | Present at line 346 | ✅ |
| `glReadBuffer(GL_BACK)` | Not mentioned in contract | Added at line 343 | ✅ (improvement, not in contract) |
| `glClearColor(...)` | Not mentioned in contract | Added with visible dark blue at line 91 | ⚠️ (not in contract, but additive) |
| `read_pixels()` BEFORE `end_frame()` | Contract section 13 line 767 | Present at line 88 vs line 96 | ✅ (unchanged) |
| On readback error, `end_frame()` still called | Contract section 13 lines 750-752 | Present at line 91: `device.end_frame();` then early return | ✅ (unchanged) |
| Scenario validation before `Platform::create()` | Contract line 636 | Present: validation at line 94, `Platform::create` at line 116 | ✅ (unchanged) |

### CONST-001 compliance

✅ **Still compliant**: `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` returns zero matches.

## Code quality

### Strengths

1. **`glReadBuffer(GL_BACK)` fix** (render_device_opengl.cpp:343): Correctly selects the back buffer before readback. This is standard practice for double-buffered OpenGL contexts. Some drivers may default the read buffer to `GL_BACK`, but explicit selection improves portability.

2. **`--frame N` parsing** (capture_command.cpp:49-66): Well-structured. Handles missing values, non-numeric input, zero/negative values, and returns a safe default of 1. Properly co-exists with output path parsing via `parse_output_path()` which skips `--frame N` pairs.

3. **`set_uniform` error handling improved** (cube_capture.cpp:77-81): The previous `(void)` cast (noted in original review) was replaced with a proper error check. Good defensive improvement.

4. **Frame rate limiting** (cube_capture.cpp:98-104): Present for multi-frame capture, matching the cube demo pattern. Good consistency.

5. **`parse_output_path` skips `--frame N`** (capture_command.cpp:70-80): Correctly handles the case where `--frame` appears before the output path (e.g., `buddd capture cube --frame 60 output.png`).

### Observations

1. **`glReadBuffer(GL_BACK)` is a pragmatic fix**: While not present in the original implementation contract, this is a correct and necessary call for reliable double-buffered readback. It does not contradict any existing AC or the architecture boundary.

2. **`begin_frame()` clear color change is additive**: The dark blue background (`0.02, 0.02, 0.05`) replaces whatever default clear color was previously in effect (likely `0, 0, 0, 0` or `0, 0, 0, 1`). This is a cosmetic improvement that affects all rendering (cube demo, triangle demo, and capture). It is not a spec violation (the spec does not specify clear color), but it is an undocumented architectural change in the OpenGL backend.

3. **`--frame N` overflow edge case**: `std::strtol` can return `LONG_MAX`/`LONG_MIN` for values exceeding `long` range. The `static_cast<int>(n)` has implementation-defined behavior for out-of-range values, potentially wrapping to a negative number, which would then trigger the `effective_frames < 2 → 2` fallback. This is a minor edge case unlikely to be hit in practice.

4. **Unused `Scenario` struct**: Still present (`capture_command.cpp` — the struct was in the original anonymous namespace). Was noted in the first review, remains unresolved.

## Issues

### Blocking issues

- [ ] **AC-015 regression — missing extra args warning**: The extra args warning pattern matching `demo_command.cpp` (lines 90-97) was removed during the `--frame` refactoring. The original implementation at `capture_command.cpp` lines 103-110 emitted `"Warning: unexpected arguments after 'capture %s': ..."`. The current `parse_output_path()` silently consumes unexpected positional arguments as the output path. For example, `buddd capture cube extra_arg` will attempt to save to `./extra_arg` with no warning. This breaks AC-015.

- [ ] **Camera position deviates from spec and implementation contract**: The spec explicitly states camera at `(0,0,3)` with a note explaining that this is intentionally different from the cube demo's `(3,2,3)` isometric view. The implementation contract requires `eye = (0, 0, 3)`. The current code uses `(3.0f, 2.0f, 3.0f)` (cube_capture.cpp:33), matching the cube demo. This produces a fundamentally different captured image (isometric vs front-on reference view). **File:** `src/cmd/capture/cube_capture.cpp` line 33.

- [ ] **Render loop with rotation animation violates spec**: The spec requires "exactly one frame" (A-08) with "angle = 0" (A-13) for a "deterministic, reproducible capture." The implementation now renders multiple frames with `0.5 rad/s` rotation around Y, matching the cube demo's animation logic. The last frame is captured, meaning the output depends on timing. **File:** `src/cmd/capture/cube_capture.cpp` lines 59-105.

- [ ] **Minimum 2-frame driver quirk workaround**: The code forces `effective_frames = (num_frames < 2) ? 2 : num_frames` with a comment blaming a "driver quirk where frame 1's glReadPixels(GL_BACK) returns the clear color instead of rendered content on the very first frame after window creation." This contradicts "exactly one frame" (A-08). This workaround should be documented in a spec update and its root cause investigated (does `glReadBuffer(GL_BACK)` not fix this?). **File:** `src/cmd/capture/cube_capture.cpp` line 50.

- [ ] **`--frame N` violates explicit non-goal**: The spec lists "No multi-frame capture or video recording" as a non-goal. The `--frame N` parameter adds multi-frame render-and-capture support. While pragmatically useful as a side effect of the minimum-2-frames workaround, this exceeds the spec scope and requires a spec amendment or implementation contract update. **File:** `src/cmd/commands/capture_command.cpp` lines 23-31, 49-66, 100-104.

- [ ] **`capture_cube_scene` signature changed without spec update**: The function signature now takes 5 parameters (added `int num_frames`). The spec and implementation contract define 4 parameters. This breaks API compatibility with the spec. **File:** `src/cmd/capture/cube_capture.h` lines 26-32.

### Non-blocking issues

- [x] **Unused `Scenario` struct**: `src/cmd/commands/capture_command.cpp` (anonymous namespace). Dead code. *(Carried forward from initial review — resolved in this re-review?)* → Still present, still unused. Unchanged.

- [x] **Discarded `set_uniform` result**: Previously `(void)cube.material->set_uniform("u_mvp", mvp)` — now properly error-checked. **Resolved.** ✅

- [ ] **`begin_frame()` clear color change undocumented**: `glClearColor(0.02f, 0.02f, 0.05f, 1.0f)` was added in `render_device_opengl.cpp:91`. This changes the visible background for all rendering (demos and capture) from the default/inherited clear color to a visible dark blue. This is a cosmetic improvement and not a spec violation, but it is an architectural change in the OpenGL backend that should be documented in the implementation contract or an ADR.

- [ ] **`--frame N` overflow edge case**: `std::strtol` with extremely large values can overflow when cast from `long` to `int`. The fallback to minimum 2 frames masks the issue but may produce confusing behavior. Consider validating the parsed value against `INT_MAX` / `INT_MIN`.

- [ ] **No automated test for `--frame` parsing**: The `--frame` parameter has no unit test. While the display-dependent capture path can't be tested on CI, the argument parsing (`parse_frame_count` and `parse_output_path`) could be unit-tested independently. Consider adding a pure-function test for these parsers.

## Recommendations

### Required before re-acceptance

1. **Restore AC-015 extra args warning**: Add back the warning pattern matching `demo_command.cpp` lines 90-97. The warning should fire for any positional arguments beyond `<scenario>` that are not consumed by `--frame N` or the output path.

   Alternatively, define and implement a new extra-args policy that accounts for `--frame N`. For example: if positional arguments remain after consuming `--frame N` and an optional output path, warn about them.

2. **Update the spec and implementation contract** to account for the debugging changes, OR revert the debugging changes to conform to the existing spec. Specifically:
   - If the camera at `(3,2,3)` is intentional, update the spec note that distinguishes capture from demo camera.
   - If the multi-frame render loop is intentional, update A-08, A-13, and the non-goal about multi-frame capture.
   - If the minimum 2-frame workaround is required, document it as a known GPU driver limitation.
   - Update the `capture_cube_scene` function signature in the spec and contract to include `num_frames`.
   - Update the usage text in the spec to include `--frame N`.
   - Document the `glReadBuffer(GL_BACK)` call as a required implementation detail.
   - Document the `glClearColor` change in the OpenGL backend.

3. **Investigate the "frame 1 driver quirk"**: The comment says `glReadPixels(GL_BACK)` returns clear color on frame 1. If `glReadBuffer(GL_BACK)` was added to fix the double-buffering issue, does the minimum-2-frames workaround still apply? Test with `effective_frames = num_frames` (without the minimum-2 override) and `glReadBuffer(GL_BACK)` to see if frame 1 now works correctly.

### Recommended improvements

4. **Add unit tests for `parse_frame_count`**: The function is a pure computation (no display, no IO) and can be tested directly with a variety of inputs (normal, edge, overflow, missing value, non-numeric, etc.).

5. **Document the `glClearColor` change**: Add a note to the implementation contract or relevant ADR that `begin_frame()` now sets a visible clear color (`0.02, 0.02, 0.05, 1.0`) rather than relying on an inherited default.

6. **Consider protecting `--frame N` against overflow**: Add a bounds check: `if (n > INT_MAX || n < 1)` after `strtol` to prevent implementation-defined `long`→`int` conversion for out-of-range values.

7. **Remove the unused `Scenario` struct**: Dead code in `capture_command.cpp` anonymous namespace.

## Resolution history

| Date | Event | Verdict |
|------|-------|---------|
| 2026-05-30 | Initial review | `Accepted` |
| 2026-05-30 | Re-review after debugging changes | **`Rejected`** — 6 blocking issues (1 AC regression, 5 spec/contract violations) |

**Reviewed by**: Code Reviewer Agent
**Date**: 2026-05-30
