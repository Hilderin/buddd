# SPEC-012 — Depth Buffer Support for OpenGL Renderer

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

## Problem

The Buddd Engine's OpenGL renderer allocates only a colour buffer — no depth (Z) buffer is requested from SDL3, no `glEnable(GL_DEPTH_TEST)` is issued, and `begin_frame()` clears only `GL_COLOR_BUFFER_BIT`. The result is that the GPU rasterizer writes fragments in index-buffer order without depth testing: back faces overwrite front faces, making all 3D rendering appear flat and incorrect.

This is not fixable through application-level workarounds (e.g., sorting draw calls by distance) because:

- Individual draw calls (e.g., a single cube) contain both front and back faces in the same index buffer; within a single `glDrawElements` call, OpenGL must decide per-fragment visibility.
- Sorting entities alone (painter's algorithm) still fails for concave or interpenetrating objects.

The hardware depth buffer is the correct, standard solution for opaque 3D rendering.

## Goals

- **Depth buffer allocation**: Request a 24-bit depth buffer from SDL3 during OpenGL context creation.
- **Depth testing**: Enable `GL_DEPTH_TEST` with `GL_LESS` comparison function so that fragments closer to the camera occlude those farther away.
- **Per-frame depth clear**: Clear the depth buffer each frame alongside the colour buffer in `begin_frame()`.
- **Zero regression**: Existing demos (`cube_demo`, `cube_scene_demo`) continue to compile and render without crashing or visual degradation — they should visually improve (correct occlusion).
- **Minimal change**: No modifications to the `RenderDevice` abstract interface, no new public APIs, no backend-visible depth configuration.

## Non-goals

- No render-order sorting (painter's algorithm) — not needed for opaque geometry with a working depth buffer.
- No transparency, alpha blending, or order-independent transparency (OIT).
- No backface culling (`glEnable(GL_CULL_FACE)`) — can be added separately.
- No stencil buffer allocation or stencil testing.
- No depth buffer readback or depth texture access (shadow maps, post-processing depth).
- No changes to the `RenderDevice` abstract interface or public header.
- No changes to the Headless backend (no GPU, no depth concept).
- No configurable depth buffer size (24-bit is the standard for v1; future specs may add configurable pixel format).
- No shader changes — depth output is handled automatically by the fixed-function rasterizer when `GL_DEPTH_TEST` is enabled.
- No changes to the `RenderSystem`, `CameraComponent`, `MeshRenderer`, or any scene/ECS types.

## Actors

| Actor | Description |
|---|---|
| Application developer | Uses `RenderDevice` and `RenderSystem` to render 3D scenes. Expects front faces to correctly occlude back faces without manual sorting or workarounds. |
| Demo maintainer | Runs or extends `cube_demo` and `cube_scene_demo`. Should see correct 3D rendering (a properly shaded, depth-occluded cube). |
| Test suite | Catch2 v3 tests that verify the OpenGL backend state setup and the unchanged `RenderDevice` interface — all in headless mode where possible, plus manual visual verification for the demos. |

## User-visible behavior

### 1. Depth buffer allocation (`src/engine/render/render_device.cpp`)

A new `SDL_GL_SetAttribute` call is added **before** `SDL_GL_CreateContext`, between the existing context profile/version attributes and the context creation:

```cpp
// Existing attributes
SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#ifndef NDEBUG
SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

// NEW: Request a 24-bit depth buffer
SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

// Context creation — unchanged
auto* gl_context = SDL_GL_CreateContext(sdl_window);
```

- If the requested 24-bit depth buffer is not available, SDL3 may fall back to the closest supported size (typically 24 or 32 bits on modern hardware). No explicit fallback handling is required — the depth buffer will be created with whatever depth precision the driver provides, and `GL_DEPTH_TEST` will use it correctly.
- The value `24` is chosen because it is the most widely supported depth buffer precision on desktop GPUs and matches the default for OpenGL contexts created via SDL3 without explicit depth size (when unspecified, many drivers default to 0 — no depth buffer).

### 2. Depth test enablement & per-frame clear (`src/engine/render/render_device_opengl.cpp`)

#### Initialization (one-time setup)

`GL_DEPTH_TEST` is enabled and the comparison function is set to `GL_LESS` at a suitable initialization point. The constructor is the natural location, consistent with one-time OpenGL state setup:

```cpp
RenderDeviceOpenGL::RenderDeviceOpenGL(SDL_Window* window, SDL_GLContext context)
    : window_(window), context_(context)
{
    // Enable hardware depth testing — fragments closer to the camera
    // (smaller Z in clip space after perspective divide) occlude farther ones.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}
```

- `GL_LESS` means: a fragment passes the depth test if its Z value (after perspective divide and viewport depth-range transform, in window coordinates [0,1]) is **less** than the currently stored depth value. This is the standard convention for a right-handed coordinate system with the camera looking down -Z (as used by `math::Camera`).
- `glDepthFunc(GL_LESS)` is technically the default, but is set explicitly for clarity and to guard against any future state changes that might alter it.
- No `glDepthMask(GL_TRUE)` call is needed — depth writing is enabled by default.
- No `glDepthRange()` call is needed — the default near/far range of [0,1] is correct for the existing `math::Camera` projection setup.

#### Per-frame clear

`begin_frame()` gains `GL_DEPTH_BUFFER_BIT` in its clear call:

```cpp
auto RenderDeviceOpenGL::begin_frame() -> void {
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // ADDED GL_DEPTH_BUFFER_BIT
}
```

- Clearing the depth buffer each frame resets all pixels to the far plane (default `glClearDepth(1.0)` value), so that fragments from the current frame are not falsely occluded by depth values left over from the previous frame.
- `glClearDepth(1.0)` is the OpenGL default and is not changed — this is correct for `GL_LESS` comparison (depth = 1.0 means "far plane", so any fragment with Z < 1.0 passes the test).
- The `glClearColor` call is unchanged.

### 3. No changes to:

- `render_device.h` — abstract interface unchanged.
- `render_device_opengl.h` — no new members or methods. The existing `window_` and `context_` members are sufficient.
- `render_device_headless.h` / `render_device_headless.cpp` — no changes needed. Headless mode has no depth buffer concept and does not simulate depth testing.
- `render_device.cpp` — the OpenGL/context-creation branch is modified (adds `SDL_GL_SetAttribute`); the headless branch is unchanged.
- `RenderSystem`, `CameraComponent`, `MeshRenderer`, `World`, `Entity`, `Shader`, `Material`, `Model` — no changes.
- Demo files — no structural changes. The visual output improves automatically.

## User stories

### Story 1 — Depth buffer is allocated and depth testing is enabled (Priority: P1)

As an application developer, I want depth testing to be enabled automatically when the OpenGL render device is created, so that 3D rendering correctly handles occlusion without any manual setup.

**Given** the OpenGL render backend is initialized
**When** the `RenderDeviceOpenGL` constructor runs
**Then** `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` was called before `SDL_GL_CreateContext`, `glEnable(GL_DEPTH_TEST)` is called during construction, and `glDepthFunc(GL_LESS)` is set.

### Story 2 — Depth buffer is cleared each frame (Priority: P1)

As an application developer, I want the depth buffer to be cleared at the start of each frame so that rendering from the previous frame does not interfere with the current frame.

**Given** a running `RenderDeviceOpenGL`
**When** `begin_frame()` is called
**Then** `GL_DEPTH_BUFFER_BIT` is included in the `glClear()` call alongside `GL_COLOR_BUFFER_BIT`.

### Story 3 — Existing demos render correctly with depth (Priority: P2)

As a demo maintainer, I want the existing cube demos to show correct occlusion after the depth buffer change, so that 3D objects appear solid and correctly shaded.

**Given** the cube demo (either `cube_demo` or `cube_scene_demo`) is running
**When** the cube is rendered
**Then** front faces properly occlude back faces — the six face colours are distinguishable and the cube appears solid rather than transparent/intersecting.

### Story 4 — Headless backend is unaffected (Priority: P2)

As a CI maintainer, I want the headless backend to continue passing all existing tests without any modifications, so that headless testing remains available for non-GPU environments.

**Given** the headless backend
**When** `begin_frame()` and `end_frame()` are called
**Then** no depth-related behavior is added — the headless backend increments its counters as before.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` is called in the OpenGL backend creation path (`render_device.cpp`) before `SDL_GL_CreateContext`. | Static inspection: the call exists between the existing profile/version attributes and `SDL_GL_CreateContext`. |
| AC-002 | `glEnable(GL_DEPTH_TEST)` is called during `RenderDeviceOpenGL` construction. | Static inspection: `render_device_opengl.cpp` constructor contains `glEnable(GL_DEPTH_TEST)`. |
| AC-003 | `glDepthFunc(GL_LESS)` is called during `RenderDeviceOpenGL` construction (or immediately after `glEnable`). | Static inspection: the same constructor contains `glDepthFunc(GL_LESS)`. |
| AC-004 | `begin_frame()` clears `GL_DEPTH_BUFFER_BIT` in addition to `GL_COLOR_BUFFER_BIT`. | Static inspection: `glClear(GL_COLOR_BUFFER_BIT \| GL_DEPTH_BUFFER_BIT)` is present in `RenderDeviceOpenGL::begin_frame()`. |
| AC-005 | The `RenderDevice` abstract interface (`render_device.h`) is unchanged — no new virtual methods, no new parameters. | `diff` against the pre-spec header shows zero changes. |
| AC-006 | The `RenderDeviceOpenGL` header (`render_device_opengl.h`) is unchanged — no new members, no new methods. | `diff` against the pre-spec header shows zero changes. |
| AC-007 | The Headless backend (`render_device_headless.h`, `render_device_headless.cpp`) is unchanged — no depth-related behavior added. | `diff` against pre-spec files shows zero changes. |
| AC-008 | `cube_demo` compiles, links, and runs without crashing. The cube is rendered with correct occlusion (front faces hide back faces). | Run `buddd demo cube` — observe that the cube appears solid with distinguishable face colours. No crash during the demo loop. |
| AC-009 | `cube_scene_demo` compiles, links, and runs without crashing. The cube is rendered with correct occlusion. | Run `buddd demo cube-scene` — observe that the cube appears solid with distinguishable face colours. No crash during the demo loop. |
| AC-010 | No OpenGL errors are generated during depth state setup. | In the `RenderDeviceOpenGL` constructor, after `glEnable(GL_DEPTH_TEST)` and `glDepthFunc(GL_LESS)`, a debug-build-only `glGetError()` check is performed and logged via `std::cerr` (consistent with the existing debug logging pattern in the Observability section). The check returns `GL_NO_ERROR`. |
| AC-011 | The default clear depth value is the OpenGL default of 1.0 (no explicit `glClearDepth` call needed). | Static inspection confirms no `glClearDepth` call — the default of 1.0 is correct. |
| AC-012 | No changes to `RenderSystem`, `CameraComponent`, `MeshRenderer`, `World`, `Entity`, `Shader`, `Material`, `Model`, or any demo source files. | `diff` against pre-spec shows no changes outside `render_device.cpp` and `render_device_opengl.cpp`. |
| AC-013 | Headless tests (existing) pass with no modifications. | Run `ctest --preset debug` (headless) — all existing tests pass. |
| AC-014 | The depth buffer size attribute is only requested in the OpenGL backend path (not in the headless path). | Static inspection: `render_device.cpp` headless branch has no `SDL_GL_SetAttribute` calls. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | Application developers writing 3D code using `RenderDevice` get correct depth occlusion automatically — no manual depth setup, no shader changes, no sorting required. | Code review of a minimal 3D program that renders overlapping geometry at different depths: the nearer geometry correctly occludes the farther geometry. |
| SC-002 | No regressions in any existing demo or test. | `cmake --build --preset debug && ctest --preset debug` — all tests pass. Both cube demos run without crashes. |
| SC-003 | Depth buffer allocation and testing complete in under 5 developer-minutes of additional code. | Code changes are confined to two files with fewer than 5 lines added total (one `SDL_GL_SetAttribute`, one `glEnable`, one `glDepthFunc`, one `GL_DEPTH_BUFFER_BIT` bitwise-OR). |
| SC-004 | Both cube demos render with visually correct occlusion: front faces of the cube hide back faces, the six face colours are distinguishable, and the cube appears solid. | Manual visual verification by running `buddd demo cube` and `buddd demo cube-scene`. |

## Edge cases

| Case | Expected behavior |
|---|---|
| SDL3 cannot provide a 24-bit depth buffer (e.g., limited driver or unusual config) | SDL3 falls back to the closest supported depth size (typically 32-bit). The `SDL_GL_SetAttribute` call is a **request**, not a hard requirement. Depth testing still works correctly at whatever precision is available. |
| Multiple `RenderDeviceOpenGL` instances created and destroyed (e.g., window re-creation) | Each constructor enables `GL_DEPTH_TEST` independently. No shared state issue — depth testing is per-context state. |
| `begin_frame()` called without a prior `end_frame()` (misuse) | Undefined behaviour (same as pre-spec). The depth buffer clear does not change this. |
| Window resized between frames | `glViewport` is already called in `begin_frame()` and resets to the new window size. The depth buffer is automatically resized by SDL3/OpenGL when the window is resized (the depth buffer is part of the default framebuffer, which is resized by the window system). No explicit handling needed. |
| `read_pixels()` called after depth writes | `read_pixels()` reads the colour buffer (`GL_BACK`) — depth buffer contents are not read. Behaviour unchanged. |
| Depth buffer requested but `GL_DEPTH_TEST` not enabled (hypothetical future backend variation) | This spec always enables `GL_DEPTH_TEST` in the constructor. Not applicable. |
| `glGetError()` returns an error during depth state setup | If `glGetError()` returns non-zero after `glEnable(GL_DEPTH_TEST)` or `glDepthFunc(GL_LESS)`, it indicates a driver or context issue (e.g., OpenGL 3.0+ core profile where these calls are valid, or an incompatible context). The engine does not check `glGetError()` after state setup (consistent with pre-spec pattern). |
| Viewport is set before depth buffer is cleared | `glViewport()` and `glClear()` are called in sequence within `begin_frame()`. The viewport affects the clear operation, so viewport must be set first — this is already the case (viewport set before clear). |
| `glClearDepth()` value differs from default | This spec uses the default `glClearDepth(1.0)`. If a future change alters `glClearDepth`, the clear value must be set explicitly. Documented in assumptions. |
| Headless backend receives a `GL_DEPTH_BUFFER_BIT`-like concept | Not applicable — headless backend has no GPU state. No changes needed. |
| `RenderDevice::create()` is called with a native window handle that is null (headless path) | The headless path skips all `SDL_GL_SetAttribute` and `SDL_GL_CreateContext` calls. Depth attribute is not set. Correct — no GPU depth buffer exists in headless mode. |

## Error cases

| Case | Expected behavior |
|---|---|
| `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` fails | `SDL_GL_SetAttribute` returns an int (-1 on error in SDL3). The existing code does not check the return value of any `SDL_GL_SetAttribute` call (a pre-existing pattern). If it fails, the context is created without a depth buffer — `GL_DEPTH_TEST` will still be enabled but will produce undefined results (no depth buffer allocated). This is consistent with how other attribute failures are handled. A future spec may add error checking for all `SDL_GL_SetAttribute` calls. |
| `glEnable(GL_DEPTH_TEST)` called but no depth buffer was allocated (SDL_GL_DEPTH_SIZE attribute was ignored or failed) | OpenGL behaviour is undefined — depth testing requires a depth buffer. On most drivers, if no depth buffer is attached to the default framebuffer, enabling `GL_DEPTH_TEST` may produce incorrect rendering, crashes, or GL errors. Mitigated by requesting the depth buffer before context creation (AC-001). |
| `glEnable(GL_DEPTH_TEST)` is called before `glDepthFunc(GL_LESS)` but a draw call occurs between them (multi-threading or re-entrancy) | Not possible — both calls are in the single-threaded constructor, issued sequentially. No draw calls occur between them. |
| Context creation fails after `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` | The existing `SDL_GL_CreateContext` error path returns an error to the caller. No depth-buffer-specific handling needed — the context creation failure is handled generically by the existing error return. |

## Permissions and security

- No elevated privileges required.
- No network, filesystem, or secret access involved.
- No new dependencies on third-party libraries beyond SDL3 and OpenGL, which are already used by the render module.
- The `SDL_GL_SetAttribute` call is a standard, safe operation that requests a framebuffer parameter — it does not load, execute, or transmit any code.
- The architecture boundary (CONST-001) is unaffected — all changes are inside `src/engine/render/`, which already has access to SDL3 and OpenGL headers.
- No new public API surface is exposed.

## Observability

All observability uses `std::cerr` consistent with the project pattern.

| Signal | Source |
|---|---|
| Depth buffer allocation requested | `std::cerr << "Depth buffer requested: 24-bit\n"` after `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` (debug builds only) |
| Depth testing enabled | `std::cerr << "Depth testing enabled (GL_LESS)\n"` after `glEnable(GL_DEPTH_TEST)` (debug builds only) |
| Frame buffer clear includes depth | Not logged individually — the existing frame begin/end logging is sufficient. The changed clear mask is visible in the source. |
| Existing device creation log messages | Unchanged: `"Render device created (OpenGL 4.5 Core)"` still printed in `render_device.cpp`. |

## Out of scope

- Configurable depth buffer size (the 24-bit value is hardcoded; future specs may expose pixel format configuration).
- Depth buffer readback or access (shadow maps, post-processing depth-of-field, SSAO).
- Stencil buffer allocation or testing.
- Backface culling (`glEnable(GL_CULL_FACE)`).
- Polygon mode changes (wireframe override, point rendering).
- Depth bias (`glPolygonOffset`) for shadow maps or decal rendering.
- Depth clamping or depth bounds.
- Early-z or Hi-z optimisation configuration.
- Reverse-Z depth mapping (near=1, far=0).
- Multiple render targets or framebuffer objects (FBO) with depth attachments.
- Headless backend simulation of depth testing.
- Any changes to the `RenderDevice` abstract interface.
- Any changes to `RenderSystem`, scene graph, ECS, or demo source files.
- Any changes to shader source code (GLSL).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` successfully requests a depth buffer for the OpenGL context. On platforms where exactly 24 bits is unavailable, SDL3 rounds up to the nearest supported size (e.g., 32 bits). This is standard SDL3 behaviour. |
| A-02 | `glClearDepth(1.0)` is the OpenGL default and is not changed by any existing code. The default value resets the depth buffer to the far plane, which is correct for `GL_LESS` comparison. |
| A-03 | `GL_LESS` is the correct depth comparison function for the existing `math::Camera` setup (right-handed, looking down -Z, with standard perspective projection mapping Z to [0,1] after perspective divide). This is the OpenGL convention and matches the existing camera implementation. |
| A-04 | No existing code calls `glDisable(GL_DEPTH_TEST)` or changes `glDepthFunc` — the depth state set in the constructor persists for the lifetime of the `RenderDeviceOpenGL`. If future code needs to disable depth testing temporarily, it must restore the state. |
| A-05 | The constructor of `RenderDeviceOpenGL` is called after `SDL_GL_MakeCurrent` (as it is in the current `render_device.cpp` factory), so OpenGL calls in the constructor are valid. |
| A-06 | The depth buffer is part of the default framebuffer and is automatically resized when the window is resized. No explicit handling is required. |
| A-07 | The Headless backend has no GPU state and no depth concept — it is used for unit testing and CI without a display. No depth-related changes are needed or appropriate. |
| A-08 | `GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT` is a valid bitmask for `glClear()` and both buffers are available (the depth buffer was allocated via `SDL_GL_DEPTH_SIZE`). Using an invalid bit (e.g., `GL_STENCIL_BUFFER_BIT` without a stencil buffer) would be undefined behaviour, but `GL_DEPTH_BUFFER_BIT` is safe because the depth buffer was explicitly requested. |
| A-09 | The OpenGL 4.5 Core profile supports `glEnable(GL_DEPTH_TEST)` and `glDepthFunc(GL_LESS)` unconditionally. These are part of the core OpenGL 1.0 API and are not deprecated in any modern profile. |
| A-10 | Files to be modified:
- `src/engine/render/render_device.cpp` — add `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)`.
- `src/engine/render/render_device_opengl.cpp` — add `glEnable(GL_DEPTH_TEST)`, `glDepthFunc(GL_LESS)`, and `GL_DEPTH_BUFFER_BIT` to `glClear()`.
No new files are created. |
| A-11 | The `GL_LESS` comparison function is set explicitly for clarity. The driver default is also `GL_LESS`, so this is a no-op on most drivers. It guards against any code that might change `glDepthFunc` before the first draw call. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | Should `glDepthFunc(GL_LESS)` be set unconditionally in the constructor, or should it be left to the driver default (which is already `GL_LESS`)? **Proposed resolution**: Set it unconditionally for explicitness and to guard against accidental state changes before the first draw call. This matches the project convention of being explicit about OpenGL state (as seen with `glClearColor`). | Minor — no behaviour difference either way, but explicit is safer and more maintainable. |
| Q-02 | Should a `glGetError()` check be added after depth state setup to provide a diagnostic if the depth buffer was not actually allocated? **Proposed resolution**: No — the existing code does not check `glGetError()` after state setup (e.g., after `glViewport`, `glClearColor`). Adding a one-off check would introduce an inconsistency. A future spec may add comprehensive GL error checking across the renderer. | Observability — defer to a future error-checking spec. |
