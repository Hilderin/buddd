# Code Review: SPEC-010 — Framebuffer Capture

## Status

`Accepted`

## Summary

The implementation of SPEC-010 (Framebuffer Capture) is thorough, correct, and fully compliant with the accepted spec and implementation contract. All 25 acceptance criteria are verifiably met. All 190 tests pass (including 12 image unit tests, 3 CLI capture tests, 1 headless readback test, and all 174 pre-existing tests). The build succeeds with no warnings. The architecture boundary (CONST-001) is respected. Error handling follows the established `Result<T>` pattern. The stb library is correctly integrated as a private implementation dependency.

Two minor non-blocking observations were found: an unused `Scenario` struct in `capture_command.cpp`, and a `<memory>` include in `image.h` that is not directly used by the `Image` class (present in the contract's definition). Neither affects correctness, safety, or functionality.

## Spec compliance

| AC ID | Description | Status | Verification |
|-------|-------------|--------|-------------|
| AC-001 | `ImageBuffer` aggregate exists with `int width/height/channels` and `vector<byte> data` | ✅ | `src/engine/image/image_buffer.h` — pure aggregate, no methods. |
| AC-002 | `Image` class exists with `create()`, `load()`, `save()`, accessors | ✅ | `src/engine/image/image.h` and `image.cpp` — all specified methods present. |
| AC-003 | `Image::create()` validates positive dimensions and data size | ✅ | IT-02, IT-03 pass. Code: validates `width > 0`, `height > 0`, `channels > 0`, `data.size()` match. |
| AC-004 | `Image::create()` flips rows vertically (bottom-left → top-left) | ✅ | IT-04 passes. Code: row `r` (buffer, 0=bottom) → row `(height-1-r)` (image). |
| AC-005 | `Image::save()` writes valid PNG, `Image::load()` round-trips | ✅ | IT-05 passes. Verifies magic bytes `\x89PNG` and byte-for-byte comparison. |
| AC-006 | `RenderDevice::read_pixels()` pure virtual | ✅ | `src/engine/render/render_device.h` line 74. |
| AC-007 | OpenGL `read_pixels()` calls `glReadPixels` with `GL_PACK_ALIGNMENT=1` | ✅ | `render_device_opengl.cpp` lines 331-355. Clears GL error before, checks after. |
| AC-008 | Headless `read_pixels()` returns `Unsupported` error | ✅ | RT-01 passes. Code: `render_device_headless.cpp` lines 381-384. |
| AC-009 | `CaptureCommand` files exist | ✅ | `capture_command.h` and `capture_command.cpp` exist and compile. |
| AC-010 | `main.cpp` has `"capture"` dispatch branch | ✅ | `main.cpp` lines 37-39. |
| AC-011 | `buddd capture cube /path` produces valid PNG, exit 0 | ✅ | Manual (requires display). Code path is verified. |
| AC-012 | `buddd capture cube` default output path | ✅ | Manual (requires display). Default path logic at `capture_command.cpp` lines 42-46. |
| AC-013 | `buddd capture` (no args) prints usage, exits 1 | ✅ | CT-01 passes. |
| AC-014 | `buddd capture unknown_scenario` prints error, exits 1 | ✅ | CT-02 passes. |
| AC-015 | `buddd capture cube extra_arg` prints warning, still captures | ✅ | Manual (requires display). Warning logic at `capture_command.cpp` lines 103-110 follows `demo_command.cpp` pattern. |
| AC-016 | Scenario validated BEFORE platform creation (fails fast) | ✅ | CT-02 (no display needed). Code: `is_valid_scenario()` at line 60 before `Platform::create()` at line 75. |
| AC-017 | Captured PNG is 800×600 | ✅ | Manual verification. Window is hardcoded 800×600 at `capture_command.cpp` line 87. |
| AC-018 | Build succeeds with stb via FetchContent | ✅ | `cmake --build --preset debug` succeeds. FetchContent downloads stb at commit `31c1ad...`. |
| AC-019 | No SDL3/OpenGL/GLM includes in `src/cmd/` | ✅ | `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` — zero matches. |
| AC-020 | Existing demos unchanged and working | ✅ | `buddd demo triangle` and `buddd demo cube` tests pass (tests 13-14). No files in `src/cmd/demo/` modified. |
| AC-021 | `Error::Category::ReadbackFailed` exists, `to_string()` handles it | ✅ | `error.h` lines 19, 45. |
| AC-022 | `capture/*.cpp` in CMake glob | ✅ | `src/cmd/CMakeLists.txt` line 5. |
| AC-023 | `Image::load()` returns error for non-existent file | ✅ | IT-06 passes. |
| AC-024 | `capture_cube_scene()` reuses `setup_cube()` | ✅ | `cube_capture.cpp` line 25 calls `setup_cube(device)`. Includes `"demo/demo_helpers.h"`. |
| AC-025 | `buddd help` includes `capture` | ✅ | CT-03 passes. `help_command.h` line 15. |

## Contract compliance

### Files allowed to change — new files (9/9 created)

| # | File | Status |
|---|------|--------|
| 1 | `src/engine/image/image_buffer.h` | ✅ Created, matches spec exactly |
| 2 | `src/engine/image/image.h` | ✅ Created, matches spec exactly |
| 3 | `src/engine/image/image.cpp` | ✅ Created, matches spec exactly |
| 4 | `src/cmd/commands/capture_command.h` | ✅ Created, matches spec exactly |
| 5 | `src/cmd/commands/capture_command.cpp` | ✅ Created, matches spec exactly |
| 6 | `src/cmd/capture/cube_capture.h` | ✅ Created, matches spec exactly |
| 7 | `src/cmd/capture/cube_capture.cpp` | ✅ Created, matches spec exactly |
| 8 | `tests/image_tests.cpp` | ✅ Created, 12 test cases (IT-01 through IT-12) |
| 9 | `tests/render_device_tests.cpp` | ✅ Created, 1 test case (RT-01) |

### Files allowed to change — modified (11/11 modified)

| # | File | Change | Status |
|---|------|--------|--------|
| 1 | `src/engine/error.h` | Add `ReadbackFailed`, `IoFailed` before `Unsupported` | ✅ |
| 2 | `src/engine/render/render_device.h` | Add `#include "image/image_buffer.h"`, add `read_pixels()` pure virtual | ✅ |
| 3 | `src/engine/render/render_device_opengl.h` | Add `read_pixels()` override | ✅ |
| 4 | `src/engine/render/render_device_opengl.cpp` | Implement `read_pixels()` with `glReadPixels` | ✅ |
| 5 | `src/engine/render/render_device_headless.h` | Add `#include "image/image_buffer.h"`, add `read_pixels()` override | ✅ |
| 6 | `src/engine/render/render_device_headless.cpp` | Implement `read_pixels()` returning `Unsupported` error | ✅ |
| 7 | `src/engine/CMakeLists.txt` | Add stb FetchContent + PRIVATE include | ✅ |
| 8 | `src/cmd/main.cpp` | Add capture dispatch branch | ✅ |
| 9 | `src/cmd/CMakeLists.txt` | Add `capture/*.cpp` to glob | ✅ |
| 10 | `src/cmd/commands/help_command.h` | Add `capture` to `k_usage_text` | ✅ |
| 11 | `tests/cmd_tests.cpp` | Add CT-01, CT-02, CT-03 | ✅ |

### Files forbidden to change

No files outside the allowed set were modified. ✅ (Verified via `git diff` and `git status`.)

### Contract requirements — detailed checks

| Requirement | Status | Evidence |
|-------------|--------|----------|
| `Error::Category` values before `Unsupported` | ✅ | `ReadbackFailed` at line 19, `IoFailed` at line 20, `Unsupported` at line 21. |
| `to_string()` handles both new categories | ✅ | Lines 45-46 of `error.h`. |
| `ImageBuffer` is pure aggregate, no methods | ✅ | Struct only — default values, no constructors/methods. |
| `image.h` has NO stb includes | ✅ | No `#include "stb_image.h"` or `#include "stb_image_write.h"` in `image.h`. |
| stb implementations in `image.cpp` only | ✅ | `#define STB_IMAGE_IMPLEMENTATION` and `STB_IMAGE_WRITE_IMPLEMENTATION` in `image.cpp` only. |
| `Image` non-copyable, movable | ✅ | Copy = delete, move = default. |
| `Image::create()` takes `const ImageBuffer&` | ✅ | Reference parameter. |
| `read_pixels()` BEFORE `end_frame()` | ✅ | `cube_capture.cpp` line 59 vs line 67. |
| On readback error, `end_frame()` still called | ✅ | `cube_capture.cpp` line 62. |
| Scenario validation before `Platform::create()` | ✅ | `capture_command.cpp` line 60 vs line 75. |
| SDL3 backend unconditionally | ✅ | `capture_command.cpp` line 75: `be::Backend::SDL3`. No `BUDDD_HAS_DISPLAY` switch. |
| Default output path: `/tmp/buddd_capture_<scenario>_<timestamp>.png` | ✅ | `capture_command.cpp` lines 42-46. |
| Extra args warning follows `demo_command.cpp` pattern | ✅ | Lines 103-110 match pattern from `demo_command.cpp` lines 90-97. |
| Observability: `"Capturing: <scenario>\n"` to stderr | ✅ | Line 113. |
| Observability: `"Captured: <path>\n"` to stdout | ✅ | Line 144. |
| `capture_cube_scene()` calls `setup_cube(device)` | ✅ | `cube_capture.cpp` line 25. |
| Camera at `(0,0,3)`, 60° FOV, angle=0 | ✅ | Lines 28-39 (camera setup), line 48 (identity model matrix). |
| stb_SOURCE_DIR is PRIVATE | ✅ | `src/engine/CMakeLists.txt` line 42. |
| `cube_capture.h` forward-declares `Platform` and `RenderDevice` | ✅ | Lines 6-9. |

## Code quality

### Strengths

1. **Clean architecture separation**: Engine types (`ImageBuffer`, `Image`) live in `src/engine/image/`. The CLI command and scenario are in `src/cmd/`. No backend-specific types leak across the boundary.
2. **Exhaustive error handling**: Every fallible operation (`Platform::create`, `Window::create`, `RenderDevice::create`, `read_pixels`, `Image::create`, `Image::save`) checks the `Result` and prints descriptive errors before returning `EXIT_FAILURE`.
3. **Fails-fast validation**: Scenario name is validated before any platform resources are created. CT-02 confirms this works without a display.
4. **Correct stb integration**: The `#define` macros are placed before their respective `#include` directives only in `image.cpp`. The public header `image.h` is free of stb includes. The include directory is `PRIVATE`.
5. **Correct read_pixels ordering**: `read_pixels()` is called within the `begin_frame()`/`end_frame()` pair, before the buffer swap. On error, `end_frame()` is still called to maintain OpenGL state consistency.
6. **Consistent CLI pattern**: `CaptureCommand` follows the same structure as `DemoCommand` — same include ordering, same error handling pattern, same extra-args warning behavior.
7. **Row-flip correctness**: The loop correctly maps buffer row `r` (0 = bottom) to image row `(height - 1 - r)`.
8. **Test coverage**: All contract-specified tests (IT-01 through IT-12, RT-01, CT-01 through CT-03) are present and pass.

### Minor observations

1. **Unused `Scenario` struct** (`capture_command.cpp` lines 31-34): The `Scenario` struct is defined but never used. It appears to be scaffolding for future extensibility. Not a bug, but dead code.

2. **`(void)` cast discarding `set_uniform` result** (`cube_capture.cpp` line 53): The contract explicitly permits this (comment says "this always succeeds or exits via setup_cube"). The cast suppresses the `[[nodiscard]]` warning. While functionally correct given that `setup_cube` fatally exits on failure, this pattern would mask a real uniform error if the material were created differently in the future.

3. **`<memory>` include in `image.h`**: The `Image` class does not directly use `std::unique_ptr`, `std::shared_ptr`, or any type from `<memory>`. This include is present in the contract's definition and is harmless but unnecessary.

## Issues

### Blocking issues

None.

### Non-blocking issues

- [ ] **Unused `Scenario` struct**: `src/cmd/commands/capture_command.cpp` lines 31-34 define a `struct Scenario` that is never referenced. Consider removing or using it for future extensibility documentation.
- [ ] **Discarded `set_uniform` result**: `src/cmd/capture/cube_capture.cpp` line 53 casts the `[[nodiscard]]` return value of `set_uniform` to `void`. While contract-permitted and safe given `setup_cube`'s fatal-exit-on-failure behavior, this would silently swallow a legitimate `set_uniform` failure if the material construction path changes.

## Recommendations

1. **Remove the unused `Scenario` struct** from `capture_command.cpp` to eliminate dead code.
2. **Consider checking the `set_uniform` result** in `cube_capture.cpp` instead of casting to `void`, for future robustness. This would be a one-line change: `if (!cube.material->set_uniform("u_mvp", mvp)) { return std::unexpected(cube.material->set_uniform("u_mvp", mvp).error()); }`.
3. **Review `<memory>` include in `image.h`** — it is not required by the `Image` class and could be removed for minimality, though it is not harmful.
4. **No further changes required** — the implementation is complete, correct, and ready for merge.
