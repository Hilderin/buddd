# IMPL-012 — Depth Buffer Support for OpenGL Renderer

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|
| Approved by | Guillaume |
| Date | 2026-05-30 |
| Time | (approved via question tool — see conversation) |

## Source spec

`docs/specs/depth-handling/spec.md` (SPEC-012), accepted. The spec-critic review (`docs/specs/depth-handling/spec-critic.md`) identified one non-blocking issue that was resolved in the accepted spec: AC-010 now explicitly requires a debug-build-only `glGetError()` check after depth state setup, with logging via `std::cerr`. No blocking issues remain.

## Goal

Add depth buffer support to the OpenGL renderer backend by: (1) requesting a 24-bit depth buffer via `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` in `render_device.cpp` before context creation, (2) enabling `GL_DEPTH_TEST` with `GL_LESS` comparison in the `RenderDeviceOpenGL` constructor, (3) adding `GL_DEPTH_BUFFER_BIT` to the per-frame `glClear()` call in `begin_frame()`, and (4) adding debug-build-only observability logging for depth allocation, depth test enablement, and a `glGetError()` check after depth state setup. The existing demos (`cube_demo`, `cube_scene_demo`) render with correct front-face occlusion as a result. The abstract `RenderDevice` interface, its header, and the headless backend remain completely unchanged.

## Non-goals

- No changes to `render_device.h`, `render_device_opengl.h`, `render_device_headless.h`, or `render_device_headless.cpp`.
- No new files created — implementation is confined to two existing files (`render_device.cpp`, `render_device_opengl.cpp`).
- No new public API, no new virtual methods, no new parameters on existing methods.
- No changes to `RenderSystem`, `CameraComponent`, `MeshRenderer`, `World`, `Entity`, `Shader`, `Material`, `Model`, `ImageBuffer`, or any demo source files.
- No backface culling (`glEnable(GL_CULL_FACE)`), stencil buffer, depth readback, depth texture access, or framebuffer objects.
- No configurable depth buffer size — the 24-bit value is hardcoded.
- No `glClearDepth()` call — the OpenGL default of 1.0 is used.
- No headless backend changes — headless has no GPU state and no depth concept.
- No new test files — depth-specific behavior is verified by static inspection, existing SDL3 backend tests, and visual demo verification. This is justified because: (i) depth testing requires a display-backed OpenGL context and cannot be tested via the headless backend, (ii) the existing `sdl3_backend_tests.cpp` already exercises the modified code path (context creation + frame cycle) through the offscreen driver, and (iii) the debug-build logging provides runtime observability for manual verification.

## Relevant constitution rules

- **CONST-001** (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): All changes are confined to `src/engine/render/` — inside the engine boundary. No SDL3/OpenGL types leak outside. The abstract `RenderDevice` interface is unchanged. Compliant.
- **CONST-002** (`docs/constitution/rules/CONST-002-testing-policy.md`): All new behavior is either (a) verified by static inspection (AC-001 through AC-004, AC-011, AC-014), (b) verified by existing SDL3 backend tests exercising the same code path, or (c) verified by manual visual demo verification (AC-008, AC-009). Depth testing requires a display and cannot be tested via the headless backend — this is an inherent limitation of graphics-level changes. The spec was accepted with this justification.
- **CONST-003** (`docs/constitution/rules/CONST-003-documentation-policy.md`): Not applicable — no documentation files are modified.
- **CONST-004** (`docs/constitution/rules/CONST-004-security-policy.md`): Not applicable — no elevated privileges, no I/O, no new dependencies.

## Relevant ADRs

- **ADR-001** (`docs/adr/001-result-error-pattern.md`): Not directly affected — the depth changes do not introduce any new fallible operations. `SDL_GL_SetAttribute` return values are not checked (consistent with pre-existing pattern in the same file).
- **ADR-003** (`docs/adr/003-render-pipeline-architecture.md`): Not affected — draw methods remain `void` and nothing changes in the draw path.
- **ADR-004** (`docs/adr/004-demo-system-architecture.md`): Not affected — demos are not modified. The cube demos benefit from depth testing automatically because `RenderDeviceOpenGL` now enables it during construction.

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/render/render_device.cpp` (lines 20–41) | Understand the exact OpenGL context creation flow — where `SDL_GL_SetAttribute` calls are made, where the debug flag is conditionally set, and where `SDL_GL_CreateContext` is called. Depth attribute must be inserted after all existing attributes and before context creation. |
| `src/engine/render/render_device_opengl.cpp` (lines 80–93) | Understand the current constructor (lines 80–81, empty body) and `begin_frame()` (lines 87–93, especially line 92 `glClear(GL_COLOR_BUFFER_BIT)`). These are the two modification sites. |
| `src/engine/render/render_device.h` | Confirm the abstract interface is NOT modified — verification by `diff`. |
| `src/engine/render/render_device_opengl.h` | Confirm the OpenGL backend header is NOT modified — no new members or methods. |
| `src/engine/render/render_device_headless.h` | Confirm the headless backend header is NOT modified. |
| `src/engine/render/render_device_headless.cpp` | Confirm the headless backend implementation is NOT modified. |
| `tests/sdl3_backend_tests.cpp` | Read existing SDL3 offscreen tests — these will continue to pass and serve as integration tests for the modified code path. |
| `tests/render_device_tests.cpp` | Read existing headless render device tests — these must continue to pass unchanged. |

## Files allowed to change

Exactly two files, with only additive changes (no existing statements are removed, though empty braces `{}` may be expanded to multi-line blocks):

| # | File | Change summary |
|---|---|---|
| 1 | `src/engine/render/render_device.cpp` | Insert `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` and a debug-build-only `std::cerr` log line after the last existing `SDL_GL_SetAttribute` (`#endif` of the debug flag block), before `SDL_GL_CreateContext`. |
| 2 | `src/engine/render/render_device_opengl.cpp` | (a) In constructor: add `glEnable(GL_DEPTH_TEST)`, `glDepthFunc(GL_LESS)`, debug-build-only `std::cerr` log, and debug-build-only `glGetError()` check. (b) In `begin_frame()`: change `glClear(GL_COLOR_BUFFER_BIT)` to `glClear(GL_COLOR_BUFFER_BIT \| GL_DEPTH_BUFFER_BIT)`. |

## Files forbidden to change

- `src/engine/render/render_device.h` — abstract interface must be untouched.
- `src/engine/render/render_device_opengl.h` — no new members, no new methods.
- `src/engine/render/render_device_headless.h` — headless backend unchanged.
- `src/engine/render/render_device_headless.cpp` — headless backend unchanged.
- `src/engine/render/` (all other files) — no other render module files may be touched.
- `src/cmd/`, `tests/` (all files) — no demo, command, or test file changes.
- `src/engine/CMakeLists.txt`, `tests/CMakeLists.txt` — GLOB_RECURSE handles all source files.
- `docs/adr/`, `docs/constitution/`, `docs/wiki/` — no documentation changes.
- Root configuration files (`CMakeLists.txt`, `CMakePresets.json`, `.clang-format`, `AGENTS.md`, `opencode.json`).

## Existing conventions to follow

| Convention | Rule |
|---|---|
| `#ifndef NDEBUG` for debug-only code | Used in `render_device.cpp` line 26 for the debug flag, and in `render_device_opengl.cpp` lines 286, 322 for draw call logging. All new debug-only logging must use the same guard. |
| `std::cerr` for observability | Existing pattern throughout the render backend. All new logging must use `std::cerr`. |
| `SDL_GL_SetAttribute` return value not checked | Pre-existing pattern in `render_device.cpp` — none of the existing `SDL_GL_SetAttribute` calls check the return value. The new depth attribute call must follow the same pattern (no return value check). |
| OpenGL state set in constructor | Consistent with one-time setup pattern (no other state is set in the constructor currently, but the constructor is the natural location for one-time OpenGL state). |
| `glGetError()` cleared before check | In `read_pixels()` (line 349), `glGetError()` is called first to clear any prior error, then the actual check follows. The new debug-only check in the constructor should use the same pattern. |
| Bitwise OR for `glClear()` mask | Existing code uses `GL_COLOR_BUFFER_BIT` alone. New code uses `GL_COLOR_BUFFER_BIT \| GL_DEPTH_BUFFER_BIT`. |
| Include order | Standard library headers (`<cstdint>`, `<iostream>`, etc.) before project headers (`"render_device_opengl.h"`). No new includes needed — `SDL3/SDL_opengl.h` is already included by `render_device_opengl.cpp`. |
| No `GLenum` to `std::string` conversion helper dependency | The `to_hex_string` helper exists in `render_device_opengl.cpp`'s anonymous namespace but is not needed for the simple error check. Print the raw integer value for the error code, consistent with minimal debug output. |

## Required implementation behavior

### 1. `src/engine/render/render_device.cpp` — Insert depth buffer attribute

**Location:** After the closing `#endif` of the debug-context-flag block (currently line 28), replacing the blank line at line 29, before the `SDL_GL_CreateContext` call (currently line 30).

The current code reads:

```cpp
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    auto* gl_context = SDL_GL_CreateContext(sdl_window);
```

**Change:** Insert after the `#endif` (and a blank line), before `SDL_GL_CreateContext`:

```cpp
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    // Request a 24-bit depth buffer for correct 3D occlusion.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifndef NDEBUG
    std::cerr << "Depth buffer requested: 24-bit\n";
#endif

    auto* gl_context = SDL_GL_CreateContext(sdl_window);
```

**Requirements:**
- The `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` call must be **unconditional** (not guarded by `#ifndef NDEBUG`). The depth buffer must be allocated in all build configurations.
- The log line `"Depth buffer requested: 24-bit\n"` must be guarded by `#ifndef NDEBUG` (debug builds only).
- No return value check on `SDL_GL_SetAttribute` — consistent with the other attribute calls in the same function.
- The headless branch (lines 15–18) must remain completely untouched — no `SDL_GL_SetAttribute` calls are added to the headless path.

### 2. `src/engine/render/render_device_opengl.cpp` — Constructor: enable depth testing

**Location:** The `RenderDeviceOpenGL` constructor, currently an empty body at line 81.

The current code reads:

```cpp
RenderDeviceOpenGL::RenderDeviceOpenGL(SDL_Window* window, SDL_GLContext context)
    : window_(window), context_(context) {}
```

**Change:** Replace with:

```cpp
RenderDeviceOpenGL::RenderDeviceOpenGL(SDL_Window* window, SDL_GLContext context)
    : window_(window), context_(context)
{
    // Enable hardware depth testing — fragments closer to the camera
    // (smaller Z after perspective divide and viewport depth-range
    // transform, in window coordinates [0,1]) occlude farther ones.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

#ifndef NDEBUG
    std::cerr << "Depth testing enabled (GL_LESS)\n";

    // Check for OpenGL errors after depth state setup (debug builds only).
    // Clear any prior error first, then check.
    glGetError();
    GLenum depth_err = glGetError();
    if (depth_err != GL_NO_ERROR) {
        std::cerr << "Warning: OpenGL error during depth state setup: "
                  << depth_err << "\n";
    }
#endif
}
```

**Requirements:**
- `glEnable(GL_DEPTH_TEST)` must precede `glDepthFunc(GL_LESS)` (order does not matter functionally, but `glEnable` first is conventional).
- `glDepthFunc(GL_LESS)` must be set explicitly — do NOT rely on the driver default. The spec requires explicitness.
- The log line `"Depth testing enabled (GL_LESS)\n"` must be guarded by `#ifndef NDEBUG`.
- The `glGetError()` check must be guarded by `#ifndef NDEBUG`. It must follow the existing pattern of calling `glGetError()` once to clear any prior error, then calling it again to get the actual check value.
- The error message must be printed to `std::cerr`. Use the raw `GLenum` integer value — the `to_hex_string` helper from the anonymous namespace exists but printing the raw integer is simpler and avoids coupling to the helper.
- No `glDepthMask(GL_TRUE)` call — depth writing is enabled by default.
- No `glDepthRange()` call — the default near/far range of [0,1] is correct.

### 3. `src/engine/render/render_device_opengl.cpp` — `begin_frame()`: add depth buffer clear

**Location:** Line 92 in `begin_frame()`.

The current code reads:

```cpp
    glClear(GL_COLOR_BUFFER_BIT);
```

**Change:** Replace with:

```cpp
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

**Requirements:**
- The `glClearColor` call (line 91) must remain unchanged.
- The `glViewport` call (line 90) must remain unchanged.
- The `GL_DEPTH_BUFFER_BIT` bit is OR-ed with `GL_COLOR_BUFFER_BIT` using the `|` operator.
- No `glClearDepth()` call is needed — the default `glClearDepth(1.0)` is correct.
- No additional logging is needed — the existing frame begin/end logging is sufficient.
- The `glViewport()` call precedes `glClear()` — this ordering must be preserved (viewport affects the clear operation).

### 4. No changes anywhere else

- No includes need to change — `SDL3/SDL_opengl.h` is already included in `render_device_opengl.cpp` (line 8) and provides `glEnable`, `glDepthFunc`, `glGetError`, `GL_DEPTH_TEST`, `GL_LESS`, `GL_DEPTH_BUFFER_BIT`, `GL_NO_ERROR`. `SDL3/SDL.h` is already included in `render_device.cpp` (line 6) and provides `SDL_GL_SetAttribute`, `SDL_GL_DEPTH_SIZE`.
- No changes to `render_device_opengl.h` — no new members, no new methods, no new includes.
- No changes to `render_device.h` — the abstract interface is unchanged.
- No changes to `render_device_headless.h` or `render_device_headless.cpp`.

## Required tests

No new test files are created. The spec explicitly lists "No new files are created" in Assumption A-10. Depth-specific behavior is verified as follows:

### Existing tests that MUST continue to pass

| Test file | Test case | What it verifies |
|---|---|---|
| `tests/sdl3_backend_tests.cpp` | `"SDL3 RenderDevice creation"` | `RenderDevice::create()` with SDL3 offscreen backend succeeds — exercises the modified `render_device.cpp` path including the new `SDL_GL_SetAttribute` call. |
| `tests/sdl3_backend_tests.cpp` | `"SDL3 frame cycle completes"` | `begin_frame()` + `end_frame()` with SDL3 backend — exercises the modified `begin_frame()` with `GL_DEPTH_BUFFER_BIT`. |
| `tests/render_device_tests.cpp` | `"Headless read_pixels returns Unsupported error"` | Headless backend is unchanged — must still pass. |
| All other existing tests | All test cases | No regressions — all existing tests must pass with `ctest --preset debug`. |

### Verification by static inspection (code review)

| AC ID | What to inspect |
|---|---|
| AC-001 | `render_device.cpp` contains `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` between the last existing attribute (`SDL_GL_CONTEXT_FLAGS` or its `#endif`) and `SDL_GL_CreateContext`. |
| AC-002 | `render_device_opengl.cpp` constructor contains `glEnable(GL_DEPTH_TEST)`. |
| AC-003 | `render_device_opengl.cpp` constructor contains `glDepthFunc(GL_LESS)` (after `glEnable`). |
| AC-004 | `begin_frame()` in `render_device_opengl.cpp` contains `glClear(GL_COLOR_BUFFER_BIT \| GL_DEPTH_BUFFER_BIT)`. |
| AC-010 | Constructor contains `#ifndef NDEBUG`-guarded `glGetError()` check that logs to `std::cerr` on error. |
| AC-011 | No `glClearDepth()` call anywhere in the modified files — confirmed by `diff`. |
| AC-014 | Headless branch in `render_device.cpp` has no `SDL_GL_SetAttribute` calls — confirmed by `diff`. |

### Verification by demo execution (manual)

| AC ID | Command | Expected outcome |
|---|---|---|
| AC-008 | `buddd demo cube` | Demo runs without crashing. The cube appears solid with distinguishable face colours (front faces occlude back faces). |
| AC-009 | `buddd demo cube-scene` | Demo runs without crashing. The cube appears solid with distinguishable face colours. |

### Verification by `diff`

| AC ID | What to verify |
|---|---|
| AC-005 | `diff` of `render_device.h` against the git baseline: zero changes. |
| AC-006 | `diff` of `render_device_opengl.h` against the git baseline: zero changes. |
| AC-007 | `diff` of `render_device_headless.h` and `render_device_headless.cpp`: zero changes. |
| AC-012 | `diff` of all non-modified files: zero changes outside `render_device.cpp` and `render_device_opengl.cpp`. |

### Debug-build verification

When running any SDL3-backed demo or test in a debug build, the following should appear on `std::cerr` during initialization:
- `"Depth buffer requested: 24-bit\n"` (from `render_device.cpp`)
- `"Depth testing enabled (GL_LESS)\n"` (from constructor)
- No `"Warning: OpenGL error during depth state setup: ..."` message (indicates `glGetError()` returned `GL_NO_ERROR`).

These lines confirm the debug-build-only logging is functioning correctly.

## Edge cases

All edge cases from the spec (see `docs/specs/depth-handling/spec.md#edge-cases`) must be handled as follows:

| Edge case | Required behavior |
|---|---|
| SDL3 cannot provide a 24-bit depth buffer | No explicit fallback handling. `SDL_GL_SetAttribute` is a *request* — if 24-bit is not available, SDL3 falls back to the closest supported size (e.g., 32-bit). Depth testing works at whatever precision is available. Consistent with pre-existing pattern (no return value checked). |
| Multiple `RenderDeviceOpenGL` instances created and destroyed | Each constructor independently enables `GL_DEPTH_TEST` and sets `GL_LESS`. Depth testing is per-context state — no shared-state issues. |
| `begin_frame()` called without a prior `end_frame()` | Undefined behaviour (same as pre-spec). The depth buffer clear does not change this. |
| Window resized between frames | `glViewport` is already called in `begin_frame()` and resets to the new window size. The depth buffer is part of the default framebuffer and is auto-resized by SDL3/OpenGL when the window is resized. |
| `read_pixels()` called after depth writes | `read_pixels()` reads the colour buffer (`GL_BACK`), not the depth buffer. Behaviour unchanged. |
| `glGetError()` returns an error during depth state setup | In debug builds, a warning is printed to `std::cerr` with the error code. In release builds, no check is performed. The existing `read_pixels()` error checking is unaffected. |
| Viewport is set before depth buffer is cleared | `glViewport()` is called before `glClear()` in `begin_frame()` — this order is correct (viewport affects the clear operation). The depth clear does not change this requirement. |
| `glClearDepth()` value differs from default | This spec uses the default `glClearDepth(1.0)`. No `glClearDepth` call is added. |
| Headless backend receives a `GL_DEPTH_BUFFER_BIT`-like concept | Not applicable — headless backend has no GPU state. No changes. |
| `RenderDevice::create()` called with a null native handle (headless path) | The headless path skips all `SDL_GL_SetAttribute` and `SDL_GL_CreateContext` calls. No depth attribute is set. Correct — no GPU depth buffer exists in headless mode. |

## Security impact

None. The changes are limited to standard, safe OpenGL and SDL3 API calls:
- `SDL_GL_SetAttribute` requests a framebuffer parameter — it does not load, execute, or transmit code.
- `glEnable(GL_DEPTH_TEST)` enables a fixed-function rasterizer state.
- `glDepthFunc(GL_LESS)` sets a comparison function — no I/O, no execution of external code.
- `glGetError()` is a read-only query.
- `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)` clears framebuffer memory — no data is exposed or transmitted.

No elevated privileges, no network access, no filesystem access, no secrets. The architecture boundary (CONST-001) is unaffected — all changes are inside `src/engine/render/`.

## Data and migration impact

None. No schema changes, data migrations, seed data, or persistent state. The depth buffer is an ephemeral GPU resource allocated per-context and destroyed when the context is destroyed.

## API compatibility impact

None. The abstract `RenderDevice` interface (`render_device.h`) is unchanged — no new virtual methods, no new parameters, no new return types. The `RenderDeviceOpenGL` header is unchanged. All existing consumers (`RenderSystem`, demos, tests) continue to compile and work without modification. The depth testing is transparently enabled for all OpenGL rendering — existing draw calls automatically get depth occlusion.

## Documentation impact

None. No README, wiki, or API documentation files are modified. The spec (`docs/specs/depth-handling/spec.md`) serves as the primary reference.

## ADR impact

None. This implementation follows existing ADR decisions:
- ADR-001 (`Result<T>` pattern) — not affected (no new fallible operations introduced).
- ADR-003 (draw returns `void` exception) — not affected (draw path unchanged).
- No new ADR is required.

## Constitution impact

None. The implementation respects:
- **CONST-001** — all changes are within `src/engine/render/`, no SDL3/OpenGL types leak outside.
- **CONST-002** — all new behavior is verifiable by static inspection (ACs), existing tests (SDLS3 backend tests exercise the modified paths), or manual visual verification (demos). The spec was accepted with this justification.
- **CONST-003** — no documentation changes.
- **CONST-004** — no security impact.

## Done criteria

The implementation is complete when ALL of the following are satisfied:

### Build and compilation
- [ ] `cmake --build --preset debug` succeeds with no errors or warnings related to the modified code.
- [ ] `cmake --build --preset release` succeeds (verifies that `#ifndef NDEBUG` guards are correct and release build does not reference debug-only code paths incorrectly).

### File modifications — exact changes verified by `diff`
- [ ] `src/engine/render/render_device.cpp` — a new `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` call is present in the OpenGL backend path, positioned between the last existing `SDL_GL_SetAttribute` (the debug flag `#endif`) and `SDL_GL_CreateContext`. A debug-build-only `std::cerr << "Depth buffer requested: 24-bit\n"` follows the attribute call. Verified by static inspection.
- [ ] `src/engine/render/render_device_opengl.cpp` — constructor now contains `glEnable(GL_DEPTH_TEST)`, `glDepthFunc(GL_LESS)`, and debug-build-only `std::cerr` log + `glGetError()` check. Verified by static inspection.
- [ ] `src/engine/render/render_device_opengl.cpp` — `begin_frame()` contains `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`. Verified by static inspection.

### Zero changes to non-allowed files
- [ ] `git diff -- src/engine/render/render_device.h` — zero changes. Verified by `diff`.
- [ ] `git diff -- src/engine/render/render_device_opengl.h` — zero changes. Verified by `diff`.
- [ ] `git diff -- src/engine/render/render_device_headless.h` — zero changes. Verified by `diff`.
- [ ] `git diff -- src/engine/render/render_device_headless.cpp` — zero changes. Verified by `diff`.

### Test results
- [ ] `ctest --preset debug` — all existing tests pass (headless and SDL3 backend tests).
- [ ] `ctest --preset release` — all existing tests pass (verifies no debug-conditional test failures).
- [ ] Headless-specific tests (`[render][headless]`) pass — the `RenderDeviceHeadless` is untouched.

### Debug-build observability (manual verification)
- [ ] When running any SDL3-backed binary in debug mode (e.g., `buddd demo triangle`), the output on `std::cerr` during initialization includes:
  - `"Depth buffer requested: 24-bit\n"`
  - `"Depth testing enabled (GL_LESS)\n"`
  - No `"Warning: OpenGL error during depth state setup: ..."` message.
- [ ] In release build, the above log lines are absent (confirmed by inspecting `#ifndef NDEBUG` guards).

### Demo verification (manual, requires display)
- [ ] `buddd demo cube` — runs without crashing. The cube appears solid with correct occlusion (front faces hide back faces, six face colours distinguishable).
- [ ] `buddd demo cube-scene` — runs without crashing. The cube appears solid with correct occlusion.
- [ ] `buddd demo triangle` — still renders the triangle correctly (regression check).

### Acceptance criteria coverage
- [ ] **AC-001**: `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` present in the OpenGL backend path before `SDL_GL_CreateContext`. ✓
- [ ] **AC-002**: `glEnable(GL_DEPTH_TEST)` in constructor. ✓
- [ ] **AC-003**: `glDepthFunc(GL_LESS)` in constructor. ✓
- [ ] **AC-004**: `GL_DEPTH_BUFFER_BIT` in `glClear()` in `begin_frame()`. ✓
- [ ] **AC-005**: `render_device.h` unchanged. ✓
- [ ] **AC-006**: `render_device_opengl.h` unchanged. ✓
- [ ] **AC-007**: Headless backend files unchanged. ✓
- [ ] **AC-008**: `cube_demo` compiles, links, runs without crash, correct occlusion. ✓
- [ ] **AC-009**: `cube_scene_demo` compiles, links, runs without crash, correct occlusion. ✓
- [ ] **AC-010**: Debug-build-only `glGetError()` check after depth state setup, logged via `std::cerr` if error. ✓
- [ ] **AC-011**: No `glClearDepth()` call — default 1.0 used. ✓
- [ ] **AC-012**: No changes to `RenderSystem`, `CameraComponent`, `MeshRenderer`, `World`, `Entity`, `Shader`, `Material`, `Model`, or any demo source files. ✓
- [ ] **AC-013**: Headless tests pass with no modifications. ✓
- [ ] **AC-014**: Depth buffer size attribute only requested in OpenGL backend path (not headless). ✓

### Code quality
- [ ] All new code follows existing conventions (`#ifndef NDEBUG` guards, `std::cerr` logging, trailing return type style).
- [ ] No new dependencies introduced.
- [ ] No new includes needed (SDL3 and OpenGL headers already included by the modified files).
- [ ] No `glClearDepth()` call present in any modified file.
- [ ] No `glDepthMask(GL_TRUE)` call present in any modified file.
- [ ] No `glDepthRange()` call present in any modified file.
- [ ] The headless branch in `render_device.cpp` has no `SDL_GL_SetAttribute` calls.
