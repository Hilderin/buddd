# SPEC-005 Code Review — Render Pipeline

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

The render pipeline implementation is thorough and closely follows the accepted spec (SPEC-005) and implementation contract (IMPL-005). All 26 acceptance criteria are satisfied. The build compiles with zero warnings, all 90 existing tests pass, and the architecture boundary is clean (no backend types leak into abstract headers), and headless files contain no SDL3/OpenGL references.

### What changed in this re-review

The `poll_events()` changes have been successfully applied:

1. **`src/engine/platform/platform.h`** — Added `virtual auto poll_events() -> bool = 0` pure virtual method to the `Platform` abstract class. No new includes needed (`bool` is a built-in type). The abstract interface remains backend-type-free.

2. **`src/engine/platform/platform_sdl3.h/cpp`** — Implements `poll_events()` by draining the SDL3 event queue via `SDL_PollEvent` and returning `false` on `SDL_EVENT_QUIT`, `true` otherwise. All non-quit events are discarded (input handling is future work).

3. **`src/engine/platform/platform_headless.h/cpp`** — Implements `poll_events()` by returning `true` unconditionally (headless: never quits).

4. **`src/cmd/main.cpp`** — Removed the `<SDL3/SDL.h>` include and all direct SDL3 event API calls (`SDL_PollEvent`, `SDL_Event`, `SDL_EVENT_QUIT`). Now uses `(*platform)->poll_events()` in both `run_test_mode()` and `run_interactive()`.

### Warning status changes

| Previous ID | Description | Status |
|---|---|---|
| **W-01** | `src/cmd/main.cpp` includes `<SDL3/SDL.h>` (CONST-001 violation) | **RESOLVED** — No SDL3/GL includes remain in `main.cpp`. Architecture boundary is fully preserved. |

All other warnings (W-02 through W-06) remain unchanged.

## Files reviewed

### New files (22) — unchanged from previous review

1. `src/engine/render/primitive_topology.h`
2. `src/engine/render/vertex_format.h`
3. `src/engine/render/shader.h`
4. `src/engine/render/shader_opengl.h`
5. `src/engine/render/shader_opengl.cpp`
6. `src/engine/render/shader_headless.h`
7. `src/engine/render/shader_headless.cpp`
8. `src/engine/render/material.h`
9. `src/engine/render/material_opengl.h`
10. `src/engine/render/material_opengl.cpp`
11. `src/engine/render/material_headless.h`
12. `src/engine/render/material_headless.cpp`
13. `src/engine/render/vertex_buffer.h`
14. `src/engine/render/vertex_buffer_opengl.h`
15. `src/engine/render/vertex_buffer_opengl.cpp`
16. `src/engine/render/vertex_buffer_headless.h`
17. `src/engine/render/vertex_buffer_headless.cpp`
18. `src/engine/render/index_buffer.h`
19. `src/engine/render/index_buffer_opengl.h`
20. `src/engine/render/index_buffer_opengl.cpp`
21. `src/engine/render/index_buffer_headless.h`
22. `src/engine/render/index_buffer_headless.cpp`

### Modified files — poll_events() changes (4)

23. `src/engine/platform/platform.h` — Added `virtual auto poll_events() -> bool = 0`
24. `src/engine/platform/platform_sdl3.h` — Added `auto poll_events() -> bool override;`
25. `src/engine/platform/platform_sdl3.cpp` — Implemented `poll_events()` using `SDL_PollEvent` loop
26. `src/engine/platform/platform_headless.h` — Added `auto poll_events() -> bool override;`
27. `src/engine/platform/platform_headless.cpp` — Implemented `poll_events()` returning `true`
28. `src/cmd/main.cpp` — Uses `(*platform)->poll_events()`; no SDL3 includes

### Other modified files (unchanged from previous review)

29. `src/engine/error.h`
30. `src/engine/render/render_device.h`
31. `src/engine/render/render_device_opengl.h`
32. `src/engine/render/render_device_opengl.cpp`
33. `src/engine/render/render_device_headless.h`
34. `src/engine/render/render_device_headless.cpp`

### Documentation files (updated alongside code)

35. `.opencode/agents/orchestrator.md` — Workflow sequencing improvement
36. `docs/constitution/rules/CONST-001-architecture-boundaries.md` — AMEND-2026-002 marked SUPERSEDED; poll_events() eliminated the need for a CONST-001 exception
37. `docs/wiki/architecture/module-map.md` — Render pipeline module map entries
38. `docs/wiki/architecture/overview.md` — Links to SPEC-005/IMPL-005
39. `docs/wiki/decisions/adr-index.md` — SPEC-005/IMPL-005 entries
40. `docs/wiki/domain/glossary.md` — Render pipeline glossary terms

## Acceptance criteria verification

| ID | Description | Status | Notes |
|---|---|---|---|
| AC-001 | `ShaderType` enum with `Vertex` and `Fragment` | ✅ | In `shader.h` |
| AC-002 | Abstract `Shader` class, virtual destructor, non-copyable, non-movable | ✅ | In `shader.h` |
| AC-003 | Abstract `Material` class with 6 `set_uniform` overloads + `has_uniform` | ✅ | In `material.h` |
| AC-004 | `VertexAttributeType` enum with all 11 values | ✅ | In `vertex_format.h` |
| AC-005 | `VertexAttribute` struct with correct fields | ✅ | In `vertex_format.h` |
| AC-006 | `VertexFormat` struct with stride + attributes | ✅ | In `vertex_format.h` |
| AC-007 | Abstract `VertexBuffer` class | ✅ | In `vertex_buffer.h` |
| AC-008 | `IndexType` enum with `Uint16` and `Uint32` | ✅ | In `index_buffer.h` |
| AC-009 | Abstract `IndexBuffer` class | ✅ | In `index_buffer.h` |
| AC-010 | `PrimitiveTopology` enum with all 5 values | ✅ | In `primitive_topology.h` |
| AC-011 | `RenderDevice` gains 4 factory methods | ✅ | In `render_device.h` |
| AC-012 | `RenderDevice` gains `draw` and `draw_indexed` (void return) | ✅ | In `render_device.h` |
| AC-013 | `ShaderOpenGL` + `ShaderHeadless` exist | ✅ | Separate files |
| AC-014 | `MaterialOpenGL` + `MaterialHeadless` exist | ✅ | Separate files |
| AC-015 | `VertexBufferOpenGL` + `VertexBufferHeadless` exist | ✅ | Separate files |
| AC-016 | `IndexBufferOpenGL` + `IndexBufferHeadless` exist | ✅ | Separate files |
| AC-017 | OpenGL 4.5 Core DSA APIs used | ✅ | `glCreateVertexArrays`, `glNamedBufferStorage`, `glVertexArrayAttribFormat`, etc. |
| AC-018 | Headless files have no SDL3/GL headers | ✅ | Verified via grep |
| AC-019 | `set_uniform` with unknown name returns error | ✅ | Both OpenGL and Headless |
| AC-020 | `has_uniform` returns true/false correctly | ✅ | Both backends |
| AC-021 | Invalid shader source returns error | ✅ | OpenGL: GLSL compiler error; Headless: `#error` marker |
| AC-022 | Shader linking failure returns error | ✅ | OpenGL: actual link error; Headless: I/O mismatch simulation |
| AC-023 | Empty vertex data returns error | ✅ | `InvalidArgument` |
| AC-024 | Empty index data returns error | ✅ | `InvalidArgument` |
| AC-025 | All 4 abstract classes non-copyable, non-movable | ✅ | All have `= delete` on copy/move |
| AC-026 | "First triangle" demo compiles and runs | ✅ | `main.cpp` implements interactive + test modes |

## Review checks — poll_events() changes

| Check | Result | Details |
|---|---|---|
| **1. No SDL3 or GL includes in main.cpp** | ✅ PASS | `grep -E '(SDL_|GL_)' src/cmd/main.cpp` — zero matches. Only engine abstraction headers are included. |
| **2. Build compiles with zero warnings** | ✅ PASS | `cmake --build --preset debug` — success, zero warnings. |
| **3. All 90 tests pass** | ✅ PASS | `ctest --preset debug` — 90/90 tests passed. |
| **4. Architecture boundary preserved** | ✅ PASS | `platform.h` has no backend types. `poll_events()` returns `bool` (built-in type). No new includes added to `platform.h`. |
| **5. `poll_events()` returns false on quit** | ✅ PASS | `platform_sdl3.cpp`: drains event queue, returns `false` on `SDL_EVENT_QUIT`, `true` otherwise. |
| **6. `poll_events()` always returns true** | ✅ PASS | `platform_headless.cpp`: unconditionally returns `true`. No SDL3/GL includes. |
| **7. W-01 resolved** | ✅ PASS | No SDL3 headers or APIs remain in `main.cpp`. The previous CONST-001 violation is fully eliminated. |

## Issues

### (Resolved) W-01: `src/cmd/main.cpp` no longer includes SDL3 headers

**Status**: ✅ **RESOLVED**

The previous review flagged that `main.cpp` included `<SDL3/SDL.h>` and directly called `SDL_PollEvent`, `SDL_Event`, and `SDL_EVENT_QUIT`, violating CONST-001 (Architecture Boundaries).

This has been fixed in the current changes:
- `main.cpp` now uses `(*platform)->poll_events()` via the abstract `Platform` interface.
- No `<SDL3/SDL.h>` or other graphics library headers are included.
- The architecture boundary is fully preserved.

The superseded exception `AMEND-2026-002` in `CONST-001-architecture-boundaries.md` documents the original proposal and why it was rejected in favor of the `poll_events()` approach.

### (Non-blocking) W-02: No render pipeline unit tests exist

**File**: `tests/` (no render pipeline test files exist)

CONST-002 (Testing Policy) requires: "All testable code added or modified in this project must have corresponding unit tests." The implementation contract specifies 28 required tests (RP-T-01 through RP-T-28) but explicitly states "The implementation-author does NOT create test files" — deferring them to a test-author.

As of this review, `tests/` contains no render pipeline test files. The contract's "Required tests" section and the human's approval of the contract defer this, rendering it a process/timing gap rather than an implementation flaw.

**Recommendation**: A test-author must create the required tests (RP-T-01 through RP-T-28) before the feature is considered fully verified. Without them, the test suite only validates existing platform/math functionality, not the new render pipeline.

### (Non-blocking) W-03: OpenGL backend headers use `<SDL3/SDL_opengl.h>` instead of `<GL/gl.h>`

**Files**:
- `src/engine/render/shader_opengl.h` (line 5)
- `src/engine/render/material_opengl.h` (line 5)
- `src/engine/render/vertex_buffer_opengl.h` (line 5)
- `src/engine/render/index_buffer_opengl.h` (line 5)
- `src/engine/render/render_device_opengl.cpp` (uses `<SDL3/SDL_opengl.h>`)

The implementation contract's conventions table specifies: "OpenGL includes in backend files → Use `<GL/gl.h>` for OpenGL types. Provided by `find_package(OpenGL REQUIRED)`."

All OpenGL backend files use `<SDL3/SDL_opengl.h>` instead. Both headers provide identical GL type declarations, and the SDL3 variant is fully functional as it merely wraps the system GL header. However, this deviates from the contract's specified include convention and bypasses CMake's `find_package(OpenGL)` dependency tracking.

**Recommended fix**: Replace `#include <SDL3/SDL_opengl.h>` with `#include <GL/gl.h>` in all five OpenGL backend files to match the contract convention. Alternatively, update the contract/conventions list if SDL3's OpenGL header is the preferred approach.

### (Non-blocking) W-04: Headless `draw`/`draw_indexed` debug output uses literal `"?"` instead of parameter values

**File**: `src/engine/render/render_device_headless.cpp`, lines 222 and 236

```cpp
std::cerr << "Draw (Headless, " << /*vertex_count*/ "?"
          << ")\n";
```

The debug output (under `#ifndef NDEBUG`) prints `"?"` as a literal string instead of the actual `vertex_count` or `index_count` parameter value. This matches the contract's placeholder code but produces uninformative debug output.

**Recommended fix**: Replace `"?"` with the actual parameter value, e.g.:
```cpp
std::cerr << "Draw (Headless, " << vertex_count << " vertices)\n";
```

### (Non-blocking) W-05: `VertexBufferOpenGL` stores unused `byte_size_` field

**File**: `src/engine/render/vertex_buffer_opengl.h`, line 28

The private field `size_t byte_size_` is stored in the constructor but never accessed via any getter or used internally. Depending on compiler warning settings (`-Wunused-private-field`), this may generate warnings.

**Suggested improvement**: Remove the field or expose it via a getter (`byte_size()`). The contract shows the same pattern, so this is a design choice rather than a bug.

### (Non-blocking) W-06: `IndexBufferOpenGL` has both `type()` and `index_type()` returning the same value

**File**: `src/engine/render/index_buffer_opengl.h`, lines 14, 17

The class exposes both `type()` (from the abstract `IndexBuffer` interface) and `index_type()` (backend-specific). Both return the same `IndexType` value. This is redundant but matches the contract's explicit specification.

**Suggested improvement**: Remove `index_type()` and have the draw method in `render_device_opengl.cpp` use `ib.type()` instead of `ib.index_type()`.

## Blocking issues checklist

- [x] (none)

## Non-blocking issues / Warnings

- ~~W-01: `src/cmd/main.cpp` includes `<SDL3/SDL.h>` outside `src/engine/`~~ ✅ **RESOLVED** — `main.cpp` now uses `(*platform)->poll_events()`, no SDL3/GL includes remain.
- W-02: No render pipeline tests exist (CONST-002 gap, but deferred to test-author per contract)
- W-03: OpenGL backend headers use `<SDL3/SDL_opengl.h>` instead of contract-specified `<GL/gl.h>`
- W-04: Headless draw debug output prints `"?"` instead of actual vertex/index count
- W-05: `VertexBufferOpenGL::byte_size_` is stored but unused
- W-06: Redundant `index_type()` method in `IndexBufferOpenGL`

## Required changes

**None blocking.** The following are recommended before the feature is considered fully complete:

1. **W-02**: Add render pipeline tests (RP-T-01 through RP-T-28) per the contract's Required tests section.
2. **W-03**: Consider replacing `<SDL3/SDL_opengl.h>` with `<GL/gl.h>` in OpenGL backend headers, or update the conventions document.
3. **W-04**: Fix the headless draw debug output to print actual vertex/index counts.

## Suggested improvements

1. **W-05**: Add a `byte_size()` getter to `VertexBufferOpenGL` (or remove the unused field).
2. **W-06**: Remove redundant `index_type()` from `IndexBufferOpenGL` and use `type()` from the abstract interface instead.
3. Consider extracting `setup_triangle()` in `main.cpp` into a reusable demo helper (currently duplicated pattern across test/interactive modes is reasonable for simplicity but could be centralized).

## Verdict

`Accepted`

The implementation faithfully implements SPEC-005 and IMPL-005. All 26 acceptance criteria are satisfied. The `poll_events()` changes successfully eliminate the previous CONST-001 architecture boundary violation (W-01) without introducing any new issues. The build compiles with zero warnings, all 90 existing tests pass, and the architecture boundary is correctly enforced in both the render pipeline abstractions and the platform event abstraction.

The remaining warnings (W-02 through W-06) are non-blocking and should be addressed in follow-up work.
