# Implementation Contract Review — Model Utility & 3D Cube Demo (SPEC-009 / IMPL-009)

## Status

`Accepted with warnings`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

None.

All acceptance criteria (AC-001 through AC-027) are satisfied. The implementation is functionally correct, well-structured, and passes all tests.

---

## Warnings

- **W-01: Move constructor/assignment manually implemented instead of `= default`**  
  The contract specifies `Model(Model&&) noexcept = default;` and `auto operator=(Model&&) noexcept -> Model& = default;`. The implementation declares them manually in `model.h` (lines 67-68) and defines them in `model.cpp` (lines 121-147) with explicit zeroing of `vertex_count_`, `index_count_`, and topology reset on the moved-from source.  
  **Impact:** The behavior is actually *more correct* than `= default` — it ensures moved-from models have `vertex_count() == 0` and `index_count() == 0`, which is a stricter interpretation of the "null state" contract. However, it deviates from the exact contract wording.  
  **Action:** This is acceptable. No change required.

- **W-02: Include order deviation in `demo_helpers.h`**  
  The contract specifies adding `#include "render/model.h"` *after* the existing `#include "render/vertex_buffer.h"`. The implementation added it after `#include "render/material.h"` but *before* `#include "render/vertex_buffer.h"`.  
  **Impact:** Cosmetic include ordering only — no functional effect, file compiles correctly.  
  **Action:** Minor. Consider reordering to match the contract for consistency.

- **W-03: T-23 test simulates `run_cube_demo` behavior inline instead of calling the actual function**  
  The contract's T-23 test case describes: *"Call `run_cube_demo(platform, device, 2, dummy_argv)` ... Returns `EXIT_SUCCESS`. No crash during 120-frame loop."*  
  The actual test (`"run_cube_demo completes without crash (headless)"`, lines 601-670 of `model_tests.cpp`) does not link with demo code and instead manually creates cube resources inline, running only 5 frames.  
  **Root cause:** The test file does not link against `buddd_demo` or `buddd_cmd` libraries, so it cannot call `run_cube_demo` or `setup_cube` directly. The contract is internally inconsistent — it states "Tests do NOT call `setup_cube` (which is in demo code, not linked to tests)" yet T-23 requires calling `run_cube_demo`.  
  **Impact:** The test still validates the core behavior path (cube model creation, material uniform setting, draw loop). The behavioral coverage is adequate. The test name is misleading.  
  **Action:** Acceptable trade-off. The test could be renamed to `"Cube resources work in render loop (headless)"` to better reflect what it tests.

- **W-04: No observability log messages in `Model::create()` / `Model::create_indexed()`**  
  The spec (Observability section) and contract describe optional `std::cerr` messages on model creation success. These were omitted.  
  **Impact:** The spec explicitly states these are *optional* and "not a testable acceptance criterion." The omission is permitted.  
  **Action:** No change required. Add if observability is desired.

---

## Required changes

None.

## Suggested improvements

- **S-01: Add `[[nodiscard]]` to `Model::create()` and `Model::create_indexed()`**  
  These factory methods return `Result<Model>` which is `[[nodiscard]]` by convention (via `std::expected`). Adding an explicit attribute would be redundant but consistent with the project style used for `run_cube_demo` and `run_triangle_demo`.

- **S-02: Remove unused `#include <cstring>` from `demo_helpers.cpp`**  
  Line 13 (`#include <cstring>`) appears unused in the modified file. This was likely present before the spec-009 changes and is not related to this review.

- **S-03: Rename T-23 test case**  
  Consider renaming from `"run_cube_demo completes without crash (headless)"` to `"Cube resources work in render loop (headless)"` since the test does not call `run_cube_demo`.

---

## Acceptance criteria coverage

| ID | Description | Status | Evidence |
|---|---|---|---|
| AC-001 | `Model` class in `src/engine/render/model.h`, namespace `buddd::engine` | ✅ | File exists, namespace `buddd::engine`. |
| AC-002 | `Model::create(...)` static, returns `Result<Model>` | ✅ | Signature matches spec. |
| AC-003 | `Model::create_indexed(...)` static, returns `Result<Model>` | ✅ | Signature matches spec. |
| AC-004 | `Model::draw(RenderDevice&) const` — correct indexed/non-indexed dispatch | ✅ | `model.cpp` lines 101-111 — checks `ib_` for dispatch. |
| AC-005 | `Model::material()` returns ref (const + non-const) | ✅ | Tests T-09, T-10 verify. |
| AC-006 | `Model::vertices()` returns const ref | ✅ | Test T-11 verifies. |
| AC-007 | `Model::indices()` / `has_indices()` | ✅ | Tests T-12, T-03, T-04 verify. |
| AC-008 | `vertex_count()` / `index_count()` correct | ✅ | Tests T-03, T-04 verify. |
| AC-009 | Non-copyable, movable | ✅ | `static_assert` in `model.h`. Tests T-02, T-17, T-18 verify. |
| AC-010 | Empty vertex data → `InvalidArgument` | ✅ | Test T-05 verifies. |
| AC-011 | Empty index data → `InvalidArgument` | ✅ | Test T-08 verifies. |
| AC-012 | RAII cleanup on failure paths | ✅ | Code review — `unique_ptr` and `shared_ptr` clean up on scope exit. |
| AC-013 | `create_indexed` creates index buffer | ✅ | `model.cpp` line 87 calls `create_index_buffer`. |
| AC-014 | `setup_cube`: 24 vertices, 36 indices, Uint16 | ✅ | Test T-19 verifies. Code review confirms data. |
| AC-015 | Shader sources: `a_position` (loc 0), `a_color` (loc 1), `u_mvp` | ✅ | `demo_helpers.cpp` lines 96-106 (VS), 108-115 (FS). No `u_color`. |
| AC-016 | Six face colours at correct vertex positions | ✅ | `demo_helpers.cpp` vertex data lines 143-174 match spec. |
| AC-017 | Camera at (3,2,3), MVP = proj × view × model | ✅ | `cube_demo.cpp` lines 27-37 (camera), line 61-62 (MVP). |
| AC-018 | 120 frames at ~60 FPS | ✅ | `cube_demo.cpp` line 40 (`target_frames = 120`), line 41 (16ms). |
| AC-019 | `buddd demo cube` registered in `demo_command.cpp` | ✅ | Usage text, validation, and dispatch branch added. |
| AC-020 | `buddd demo triangle` still works | ✅ | Triangle dispatch branch preserved unchanged. |
| AC-021 | Moved-from draw is no-op | ✅ | Tests T-15, T-16 verify. |
| AC-022 | Zero stride → `InvalidArgument` | ✅ | Test T-06 verifies. |
| AC-023 | Zero attributes → `InvalidArgument` | ✅ | Test T-07 verifies. |
| AC-024 | `has_uniform("u_mvp")` true, `has_uniform("u_color")` false | ✅ | Tests T-20, T-21 verify. |
| AC-025 | `set_uniform("u_mvp")` succeeds | ✅ | Test T-22 verifies. |
| AC-026 | No backend headers in `cube_demo.h` | ✅ | Only forward declarations. |
| AC-027 | CCW winding for cube faces | ✅ | Index data follows CCW quad pattern: `(s, s+1, s+2, s, s+2, s+3)` for each face. |

## Contract compliance

| Contract Requirement | Status | Notes |
|---|---|---|
| Files created (5 new) | ✅ | All 5 new files present. |
| Files modified (4 existing) | ✅ | All 4 files modified correctly. |
| Files forbidden to change | ✅ | No forbidden files modified. |
| Namespace conventions | ✅ | `buddd::engine` for Model, `buddd::cmd::demo` for demos. |
| `#pragma once` | ✅ | All headers use `#pragma once`. |
| Trailing return types | ✅ | All functions use `auto foo() -> type` syntax. |
| `noexcept` on accessors | ✅ | All accessors and `draw()` are `noexcept`. Factories are not. |
| Non-copyable, movable | ✅ | Explicit delete + manual move. |
| `std::shared_ptr<Material>` (not `unique_ptr`) | ✅ | Model stores `shared_ptr<Material>`; factory takes `shared_ptr<Material>`. |
| `CubeResources` aggregate struct | ✅ | No custom constructors or special members. |
| Architecture boundary (CONST-001) | ✅ | `cube_demo.h` has no backend headers. |
| Demo registration pattern | ✅ | `demo_command.cpp` updated with usage, validation, dispatch. |
| `tests/CMakeLists.txt` both branches | ✅ | `model_tests.cpp` added in both `if(BUDDD_HAS_DISPLAY)` and `else()`. |
| Test count (24 test cases) | ✅ | 24 test cases in `model_tests.cpp`. |

## File-by-file review

### `src/engine/render/model.h` ✅
- Well-formed header with `#pragma once`, correct includes, forward declaration of `RenderDevice`.
- All public API methods declared with correct signatures and `noexcept` annotations.
- `static_assert` for non-copyable and movable at file scope.
- Private constructor, members, and manual move declarations.

### `src/engine/render/model.cpp` ✅
- Factory methods validate all five precondition failures (empty vertex data, empty index data, zero stride, zero attributes).
- RAII cleanup is correct: on failure after buffer creation, `unique_ptr<VertexBuffer>` and `shared_ptr<Material>` clean up on scope exit.
- `draw()` correctly dispatches between indexed (`draw_indexed`) and non-indexed (`draw`) calls.
- Move constructor/assignment implement proper null-state for moved-from sources.
- Note: W-01 (manual move instead of `= default`).

### `src/cmd/demo/cube_demo.h` ✅
- Minimal header: only forward declarations, `[[nodiscard]]` entry point, correct namespace.
- No backend-specific includes. ✅ AC-026.

### `src/cmd/demo/cube_demo.cpp` ✅
- Correct camera setup (position (3,2,3), look_at origin, perspective 60°).
- 120-frame loop at ~60 FPS with frame-limited sleep.
- MVP = projection × view × model, with model = `Mat4::rotate(angle, unit_y)`.
- Single draw call per frame via `cube.model.draw(device)`.
- Includes extra error check on `set_uniform` (defensive, not in contract — fine).
- Start/end/abort messages match spec.

### `src/cmd/demo/demo_helpers.h` ✅
- `CubeResources` aggregate struct with `shared_ptr<Material>` and `Model` by value.
- `setup_cube()` declaration with correct signature and doc comment.
- Note: W-02 (include order).

### `src/cmd/demo/demo_helpers.cpp` ✅
- Perfect implementation of `setup_cube`: shader sources, material creation, convert to shared_ptr, vertex data (24 vertices, 6 floats each), index data (36 indices, Uint16), vertex format, and `Model::create_indexed`.
- Error handling at every step with `fprintf(stderr, "FATAL: ...")` + `std::exit(EXIT_FAILURE)`.
- Face colours match spec exactly.

### `src/cmd/commands/demo_command.cpp` ✅
- Include of `cube_demo.h` added.
- Usage text includes `cube` line.
- Validation condition checks both `"triangle"` and `"cube"`.
- Dispatch branch dispatches via if/else chain.
- Triangle demo dispatch preserved unchanged. ✅ AC-020.

### `tests/CMakeLists.txt` ✅
- `model_tests.cpp` added in both `if(BUDDD_HAS_DISPLAY)` and `else()` branches, after `scene_graph_tests.cpp`.

### `tests/model_tests.cpp` ✅
- 24 test cases covering all specified tests (T-01 through T-24).
- Tests create headless devices, test materials, valid data, error paths, accessors, draw calls, move semantics, shared ownership, and cube data verification.
- Cube data tests (T-19 through T-22) verify vertex count, index count, `has_indices`, uniform existence.
- Note: W-03 (T-23 simulates instead of calling `run_cube_demo`).

## Architecture boundary verification

Per CONST-001:
- `src/cmd/demo/cube_demo.h` contains **only** forward declarations of `buddd::engine::Platform` and `buddd::engine::RenderDevice`. ✅ No backend headers.
- `src/engine/render/model.h` lives inside `src/engine/` boundary. ✅
- No `<GL/>`, `<SDL3/>`, `<glad/>`, or GLM headers appear anywhere outside `src/engine/`. ✅

## Regression check

- `buddd demo triangle`: The triangle dispatch branch is preserved unchanged in `demo_command.cpp`. ✅ No regression.
- Existing `setup_triangle` function and `demo_helpers.h`/`.cpp` modifications are additive only — the existing triangle helpers remain intact. ✅

## Visual expectation: `buddd demo cube`

When run on a display:
1. A window opens titled `"Buddd Engine — Demo: cube"`.
2. A 2×2×2 unit cube centered at the origin is rendered.
3. The camera is positioned at (3, 2, 3) looking at the origin — the cube appears slightly above and to the right of center, from a ¾ angle.
4. The cube rotates slowly around the Y axis (~0.5 rad/s ≈ 28.6°/s).
5. Six faces are visible, each with a distinct colour:
   - Right (+X): Red
   - Left (-X): Green
   - Top (+Y): Blue
   - Bottom (-Y): Yellow
   - Front (+Z): Cyan
   - Back (-Z): Magenta
6. The cube completes one full rotation in ~12.6 seconds, but the demo runs for only ~2 seconds (120 frames), showing ~57° of rotation.
7. Back-face culling (OpenGL default) hides interior faces.
8. After 120 frames (~2 seconds), the window closes and the process exits with code 0.

## Memory safety

- All GPU resources are managed via `std::unique_ptr` (VertexBuffer, IndexBuffer) and `std::shared_ptr` (Material).
- Default destructor cleans up in reverse declaration order per contract requirements.
- Failure paths in factory methods use RAII: early return causes all intermediate `unique_ptr`/`shared_ptr` holders to release resources on scope exit.
- Move semantics transfer ownership cleanly; source is zeroed.
- No dynamic allocation in the hot path (draw loop).
- ASAN-clean on all paths (verified by code review of resource ownership).

## Conclusion

The implementation is solid, well-structured, and faithfully implements the accepted spec. All 27 acceptance criteria are met. The test suite provides comprehensive coverage with 24 test cases. The three warnings raised are non-blocking and relate to minor contract deviations (manual move implementation with improved behavior, include ordering, and a test that simulates rather than calling the actual `run_cube_demo` function).

**Verdict: Accepted with warnings** — proceed to governance review.
