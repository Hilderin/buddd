# Spec critique: SPEC-010 — Framebuffer Capture

## Status

`Accepted` *(resolved 2026-05-30)*

## Summary

This is a well-structured spec that demonstrates a clear understanding of the existing architecture, CONST-001 boundary rules, and the project's established patterns. The spec correctly reuses `setup_cube()` from SPEC-009, follows the Command pattern from SPEC-006/SPEC-007, and maintains the engine/CLI separation. All issues identified in the initial review have been resolved (see below).

## Positive aspects

- **Excellent architecture boundary hygiene**: AC-019 (zero SDL3/OpenGL/GLM headers in `src/cmd/`) and the detailed CONST-001 compliance table (lines 353–364) show careful attention to the constitution.
- **Correct reuse of existing code**: The capture scenario reuses `setup_cube()` from SPEC-009 without modifying it, honouring the non-goal of not changing existing demos or `demo_helpers`.
- **Consistent command pattern**: `CaptureCommand` follows the same `.h`/`.cpp` pair, `run(int, const char* const*) -> int` signature, and extra-args-warning pattern established in SPEC-006/SPEC-007.
- **Good error categorisation**: The new `ReadbackFailed` category is appropriately scoped (only for `glReadPixels` failures, not for `Image` I/O), and the distinction from `Unsupported` (headless backend) is clear.
- **Deterministic capture**: Using rotation angle = 0 and a fixed camera position `(0, 0, 3)` for the cube capture produces reproducible output — a deliberate and well-documented choice.
- **Comprehensive edge case table**: Lines 534–553 cover many scenarios (uppercase scenario, trailing whitespace, no display, permission errors, zero dimensions, empty data, etc.).
- **Explicit open questions**: Q-01 through Q-07 are listed honestly, making it clear which design decisions are not yet finalised.
- **Fails-fast validation**: Scenario name is validated before creating platform resources (line 193), matching the `DemoCommand` pattern.

## Issues

### Blocking issues (all resolved)

- [x] **B-01: Untagged dependency (`GIT_TAG master`) — RESOLVED**
  Pinned to specific commit hash `31c1ad37456438565541f4919958214b6e762fb4` (latest HEAD as of April 2026). stb does not use traditional version tags, so a commit hash is the appropriate pin. The CMakeLists.txt example in the spec has been updated.

- [x] **B-02: `Image::save()` channel-coverage gap — RESOLVED**
  The spec now documents that `Image::save()` supports 1–4 channels (preserving the Image's channel count). stb_image_write natively handles 1/2/3/4 channels. The channel-coverage text has been updated accordingly.

- [x] **B-03: Cross-directory dependency `render_device.h` → `image_buffer.h` — RESOLVED**
  The spec now explicitly documents that `render_device.h` must `#include "image/image_buffer.h"` and acknowledges this as an acceptable cross-directory dependency within `src/engine/`.

### Non-blocking issues (all resolved)

- [x] **N-01: Backend-selection pattern — RESOLVED**
  The spec now includes a rationale for SDL3 unconditional selection: capturing requires a display, and headless mode `read_pixels()` unconditionally returns an error. On headless builds, `Platform::create(SDL3)` fails at runtime with a clear error message. This has been confirmed with the human.

- [x] **N-02: Test implications section — RESOLVED**
  Added a full "Test implications" section following SPEC-007's pattern, listing new test files (`tests/image_test.cpp`), existing test file updates (`tests/cmd_tests.cpp`), display-dependent tests, and test hierarchy.

- [x] **N-03: Open questions — RESOLVED**
  All 7 open questions (Q-01 through Q-07) have been resolved with the human and are now documented as "RESOLVED" in the spec with their final decisions.

- [x] **N-04: Image copy/move semantics — RESOLVED**
  The `Image` class declaration in the spec now explicitly documents non-copyable (`= delete`) and movable (`= default`) semantics, matching the `Model` pattern from SPEC-009.

- [x] **N-05: AC-005 / AC-022 overlap — RESOLVED**
  Merged: AC-005 now explicitly covers "save produces valid PNG loadable by Image::load() and identifiable by external tools". AC-022 replaced with a glob test (CMake `capture/*.cpp` inclusion).

- [x] **N-06: AC-007 display dependency — RESOLVED**
  AC-007 verification now marked as "requires display/OpenGL context".

- [x] **N-07: Missing edge case (save to directory) — RESOLVED**
  Added edge case for `Image::save()` with an existing directory path (returns `IoFailed`).

- [x] **N-08: Missing OOM edge cases — RESOLVED**
  Added OOM edge cases for `Image::create()`, `Image::load()`, and `Image::save()`.

- [x] **N-09: `setup_cube` failure in error table — RESOLVED**
  Added `setup_cube()` failure to the error table, noting it calls `std::exit(EXIT_FAILURE)` per SPEC-009.

- [x] **N-10: stb include pattern in header — RESOLVED**
  The spec now documents that `image.h` includes stb headers **without** the `IMPLEMENTATION` define to prevent ODR violations.

- [x] **N-11: Error::Category update — RESOLVED**
  Both `ReadbackFailed` and `IoFailed` categories are now specified, with `to_string()` update noted.

- [x] **N-12: `capture/*.cpp` glob — RESOLVED**
  AC-022 now explicitly covers the `capture/*.cpp` glob in `src/cmd/CMakeLists.txt`.

## Open questions for the human

- **Q-01**: The backend selection for `CaptureCommand` (SDL3 unconditionally vs compile-time `BUDDD_HAS_DISPLAY` pattern). Which approach is preferred? Using SDL3 unconditionally will fail at `Platform::create()` on headless builds, whereas adopting the compile-time pattern allows graceful headless operation (with `read_pixels()` returning an error, which is already implemented).

- **Q-02**: Should GIT_TAG for stb be pinned to a specific tag (and if so, which one — e.g., `0.41` or a specific commit hash)? The current `master` branch usage breaks reproducible builds.

- **Q-03**: Should `Image::save()` explicitly handle overwriting existing files (current default), or should it fail if the target already exists (safer)? The default is set to overwrite silently (Q-06 default), but this should be confirmed.

- **Q-04**: Is a new `IoFailed` error category preferable to reusing `InitFailed` for image file I/O errors (Q-03)? Using `InitFailed` for "permission denied" or "disk full" is semantically misleading.

- **Q-05**: Should the default output path be configurable via an environment variable (Q-04)? This adds scope but improves developer ergonomics.

- **Q-06**: Should `read_pixels()` be explicitly documented as having undefined behaviour if called outside `begin_frame()`/`end_frame()` (it already is, but should this be a precondition UB like `draw()` per ADR-003)?

- **Q-07**: Should `Image::load()` and `Image::save()` use `std::filesystem::path` instead of `std::string_view` for path arguments, for cross-platform path handling? The spec currently uses `std::string_view` everywhere.

## Recommendations

1. **Pin stb to a specific tag** — Replace `GIT_TAG master` with a fixed tag (e.g., `GIT_TAG 0.41` or a commit hash). This aligns with SPEC-002 and SPEC-005 conventions.

2. **Document the `render_device.h` → `image_buffer.h` include** — Either add the include explicitly in the spec or forward-declare and verify toolchain compatibility.

3. **Adopt a consistent backend-selection strategy** — Either use the compile-time `BUDDD_HAS_DISPLAY` pattern (matching SPEC-007) or document the rationale for divergence. If the compile-time pattern is adopted, the headless backend's existing `read_pixels()` error path handles the "capture requires a display" case gracefully.

4. **Add a "Test implications" section** — Follow SPEC-007's example: list which existing tests must be updated (e.g., `help` command output test), where new engine-layer tests live (e.g., `tests/image_test.cpp`, `tests/render_device_test.cpp`), and which tests are conditional on display availability.

5. **Resolve all open questions before implementation** — Q-01 through Q-07 should be answered and either integrated into the spec or explicitly marked as resolved (following the `[RESOLVED]` pattern from SPEC-005 and SPEC-007).

6. **Document `Image` copy/move semantics** — Add explicit `= delete` / `= default` declarations in the class (or in the spec text) to mirror how `Model` documents this in SPEC-009.

7. **Add the missing edge cases** — Saving to an existing directory; OOM during image processing; 1- and 2-channel images in `Image::save()`.

8. **Clarify `Image` public header stb include pattern** — Document that `image.h` includes stb headers **without** the `IMPLEMENTATION` define to prevent ODR violations.

9. **Merge or distinguish AC-005 and AC-022** — If they are genuinely different tests (AC-005: save produces valid PNG for external tools; AC-022: our loader handles our output), make the distinction explicit.
