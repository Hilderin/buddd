# Implementation contract critique: IMPL-010 — Framebuffer Capture

## Status

`Accepted` *(resolved 2026-05-30)*

Allowed values: `Accepted`, `Accepted with warnings`, `Changes Requested`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Changes Requested` or any blocking issue remains.

## Summary

This is a thorough, detailed implementation contract that demonstrates a clear understanding of the existing architecture, conventions, and patterns. The contract is well-structured with explicit file-by-file instructions, include order, error handling, test specifications, and edge cases. All issues identified in the initial review have been resolved (see below).

## Positive aspects

- **Exceptional level of detail**: At 1026 lines, every file change is specified with exact code, include order, and rationale. The Code Agent has very little ambiguity about what to implement.
- **Correct stb implementation pattern verified**: The pinned stb commit (`31c1ad37456438565541f4919958214b6e762fb4`) has the implementation block **outside** the include guard in both `stb_image.h` and `stb_image_write.h`. The double-include pattern (first from `image.h` without `IMPLEMENTATION`, second from `image.cpp` with `IMPLEMENTATION`) works correctly for this version — the include guard skips the declarations on the second pass, while the `#ifdef STB_IMAGE_IMPLEMENTATION` block outside the guard is processed. The contract author has correctly verified this.
- **Correct `read_pixels()` ordering**: The contract explicitly calls `read_pixels()` BEFORE `end_frame()` (line 749) with error-handling cleanup that still calls `end_frame()` (line 752-753). This is critical for OpenGL buffer swap correctness.
- **Fails-fast validation**: Scenario name is validated before `Platform::create()` (lines 550-556), matching the `DemoCommand` pattern and enabling headless CI testing (AC-016, CT-02).
- **Comprehensive error handling**: Every fallible operation in `CaptureCommand::run()` has error handling that prints to stderr and returns `EXIT_FAILURE`. No fallible operation is ignored.
- **Correct `setup_cube()` reuse**: The contract correctly handles the fact that `setup_cube()` calls `std::exit()` on failure (not `Result<T>`), matching lines 210-212 of `demo_helpers.cpp`.
- **Constitution compliance**: The contract respects CONST-001 (no SDL3/OpenGL/GLM in `src/cmd/`) and CONST-002 (tests for all testable code).
- **Clear test linkage to ACs**: Each acceptance criterion is mapped to specific test IDs (IT-01 through IT-11, RT-01, CT-01 through CT-03), making verification unambiguous.
- **Good handling of `std::unexpected` pattern**: The headless `read_pixels()` returns `make_error(Error::Category::Unsupported, ...)` which correctly creates a `std::unexpected<Error>` compatible with `Result<T>`.

## Issues

### Blocking issues (all resolved)

- [x] **B-01: Test file naming mismatch — RESOLVED**
  All test files renamed from `*_test.cpp` to `*_tests.cpp` to match the existing CMake glob pattern (`image_test.cpp` → `image_tests.cpp`, `render_device_test.cpp` → `render_device_tests.cpp`). All references throughout the contract updated.

- [x] **B-02: Unnecessary stb includes in public header — RESOLVED**
  stb includes removed from `image.h`. The stb headers are now included only in `image.cpp` (the canonical single-header pattern). The `stb_SOURCE_DIR` include directory changed from `PUBLIC` to `PRIVATE`. The forward declaration of `ImageBuffer` in `image.h` is sufficient since `create()` takes `const ImageBuffer&` (reference — incomplete type is allowed).

### Non-blocking issues (all resolved)

- [x] **N-01: `to_hex_string` format — RESOLVED**
  The contract now specifies the format explicitly: `"0x"` followed by 4 lowercase hex digits, implemented as a free function in an anonymous namespace in `render_device_opengl.cpp`.

- [x] **N-02: Event polling in cube capture — RESOLVED**
  A single `platform.poll_events()` call added before `device.begin_frame()` to improve window manager compatibility. The requirement text updated accordingly.

- [x] **N-03: SDL3 unconditional backend — ACKNOWLEDGED**
  This was resolved as Q-01 in the accepted spec (SPEC-010). The behavior is intentional and confirmed with the human.

- [x] **N-04: POSIX-only `/tmp/` — ACKNOWLEDGED**
  Documented in spec assumption A-06. Cross-platform path handling is explicitly out of scope for this contract.

- [x] **N-05: `Image::create()` may throw — ACKNOWLEDGED**
  Documented in the edge case table. This is consistent with the spec and is an accepted design limitation for OOM scenarios.

- [x] **N-06: Missing save-to-directory test — RESOLVED**
  Added IT-12: `"Image::save returns error when path is a directory"` to the image test suite.

- [x] **N-07: Memory ownership model — ACKNOWLEDGED**
  The contract's code keeps all resources as local `unique_ptr`s in `CaptureCommand::run()`, ensuring references remain valid for the scope of the scenario call.

- [x] **N-08: `GL_PACK_ALIGNMENT` reset — ACKNOWLEDGED**
  Consistent with existing codebase patterns (no state restoration elsewhere).

## Open questions for the human

1. **Test file naming**: The contract uses `*_test.cpp` while the CMake glob expects `*_tests.cpp`. Which fix is preferred?
   - (a) Rename to `image_tests.cpp` / `render_device_tests.cpp` (consistent with existing convention)
   - (b) Update `tests/CMakeLists.txt` glob to also match `*_test.cpp`

2. **stb in public header**: Should stb includes be removed from `image.h` (fixing the architectural leakage) or is the current approach acceptable given that stb is a well-known, stable library? Removing them is architecturally cleaner but removing them is a simple change.

3. **Event polling in cube capture**: Should a single `platform.poll_events()` be added before `begin_frame()` for window manager compatibility, or is the current no-polling approach acceptable?

4. **Backend selection divergence**: Is the unconditional SDL3 approach for `CaptureCommand` confirmed as the desired behavior, noting that it diverges from `DemoCommand`'s compile-time pattern? This was resolved as Q-01 in the spec but is worth reconfirming.

## Recommendations

### Required changes (must fix before acceptance)

1. **Fix test file naming**: Rename `tests/image_test.cpp` → `tests/image_tests.cpp` and `tests/render_device_test.cpp` → `tests/render_device_tests.cpp` to match the existing CMake glob pattern `*_tests.cpp`. Update all references throughout the contract.

2. **Remove stb includes from `image.h`**: Delete lines 234-237 from the `image.h` specification. The stb includes provide no value in the public header and leak the dependency. Simplify the `image.cpp` include pattern to the canonical stb pattern (define `IMPLEMENTATION` then include — no prior include needed). Change `${stb_SOURCE_DIR}` from `PUBLIC` to `PRIVATE`.

### Strongly suggested improvements

3. **Specify `to_hex_string` format**: Add a brief specification for the helper, e.g.: "Formats a `GLenum` as `0x` followed by 4 lowercase hex digits. Implement as a free function in an anonymous namespace in `render_device_opengl.cpp`."

4. **Add `poll_events()` call**: Insert a single `platform.poll_events()` call in `cube_capture.cpp` before `device.begin_frame()` to improve window-manager compatibility.

5. **Add test for save-to-directory**: Add IT-12 to verify `Image::save()` returns `IoFailed` when the path is an existing directory.

### Nice-to-have improvements

6. **Document `Image::create()` throwing**: Add a note in the `Image::create()` documentation that it may throw `std::bad_alloc` if the pixel data allocation fails (matching the edge case table).

7. **Consider using `std::filesystem::temp_directory_path()`**: If cross-platform compatibility is a concern, replace the hardcoded `/tmp/` with a call to `std::filesystem::temp_directory_path()` for the default output path. This is out of scope for this contract but worth noting for future iterations.
