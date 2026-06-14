# SPEC-2026-06-RDFBO - RenderDevice FBO / Render-to-Texture

## Problem

`RenderDevice` currently has no framebuffer object (FBO) abstraction. All rendering goes to the default window framebuffer:

- `render_scene()` always draws to whatever framebuffer is bound (always the default window framebuffer).
- `create_texture()` requires CPU-side `Image` data — there is no way to create a storage-only render texture.
- `read_pixels()` hardcodes `GL_BACK` — it cannot read from a custom FBO.
- There is no depth/stencil renderbuffer abstraction — only a window-surface depth buffer exists.

This blocks the editor's Viewport panel (F-07), which needs offscreen rendering into a texture that can be displayed in an ImGui panel. The viewport must render the 3D scene through the editor's own camera into a texture, not the default framebuffer.

## Goals

1.  Introduce a `FrameBuffer` abstraction class that encapsulates a color attachment (texture) and a depth attachment (renderbuffer).
2.  Add `create_render_texture(w, h)` to `RenderDevice` — creates a storage-only texture (no initial CPU data) suitable as a color attachment.
3.  Add `create_frame_buffer(w, h)` to `RenderDevice` — creates a complete FBO with color + depth attachments.
4.  Add `read_pixels(FrameBuffer&)` overload to `RenderDevice` — reads pixel data from a custom FBO (backward-compatible — the existing parameterless overload is unchanged).
5.  Support `resize(w, h)` on `FrameBuffer` — recreates attachments at a new size, re-checks completeness.
6.  Add a `render_scene(FrameBuffer&)` overload — binds the FBO, renders into it, unbinds it, leaving the original `render_scene()` unchanged.
7.  Headless backend handles all FBO operations as safe no-ops (no crash, no real GL calls).
8.  All existing tests continue to pass with zero new compiler warnings.

## Non-goals

- No stencil attachment.
- No depth texture (renderbuffer is sufficient for MVP viewport).
- No multisampling (MSAA) support.
- No FBO pool or caching.
- No editor viewport panel (comes in a separate feature, F-07).
- No `RenderDevice::begin_frame(FrameBuffer*)` override — the explicit `bind/unbind` pattern is the intended API.
- No changes to `begin_frame()` / `end_frame()` frame pipeline.

## Actors

| Actor | Description |
|---|---|
| **Engine developer** | Writes application/editor code that uses `RenderDevice` and `FrameBuffer`. |
| **Editor** | Client code that will create an FBO per viewport, pass it to `render_scene(*fbo)`, and display `color_texture()` in an ImGui panel. |
| **RenderSystem** | Currently renders to the bound framebuffer. Will gain a `render_scene(FrameBuffer&)` overload that internally binds the FBO, renders, then unbinds. |
| **Headless test runner** | CI environment with no display. Must not crash on FBO API calls. |

## User-visible behavior

From the perspective of code consuming the engine API:

1. A new public class `FrameBuffer` exists in `buddd::engine`, movable via `std::unique_ptr`, not copyable.
2. `RenderDevice` gains:
   - `create_render_texture(w, h) → Result<unique_ptr<Texture>>`
   - `create_frame_buffer(w, h) → Result<unique_ptr<FrameBuffer>>`
    - `read_pixels(FrameBuffer&) → Result<ImageBuffer>`
3. `FrameBuffer` provides:
   - `bind()` — binds to `GL_FRAMEBUFFER`, saves previous FBO for restore
   - `unbind()` — restores the previously bound FBO
   - `resize(w, h)` — recreates attachments, returns error on failure
   - `color_texture() → Texture&` — access to the color attachment for display/sampling
   - Destructor — cleans up all GL resources
4. `RenderSystem::render_scene()` gains a separate overload by reference:
   ```cpp
   auto render_scene() -> void;                       // unchanged — renders to currently bound framebuffer
   auto render_scene(FrameBuffer& target) -> void;    // new — binds FBO, renders into it, unbinds
   ```
   - The parameterless overload renders to the currently bound framebuffer (unchanged — backward compatible).
   - The `FrameBuffer&` overload binds the FBO before rendering and unbinds it after, drawing the scene into the FBO's attachments.
5. The intended FBO usage pattern is:
   ```cpp
   auto fbo = device.create_frame_buffer(1024, 768);
   render_system.render_scene(*fbo);  // by reference, no pointer
   // fbo->color_texture() is now ready for ImGui::Image() display
   ```

## User stories

### Story 1 — Create and render to an offscreen FBO (Priority: P1)

As an engine developer, I want to create an FBO, bind it, render a scene into it, and unbind it so that I can produce offscreen renderings for the editor viewport.

**Given** a `RenderDevice` with an OpenGL backend
**When** I call `device.create_frame_buffer(64, 64)` and the result is successful
**Then** the returned `FrameBuffer` is complete and usable for rendering

**Given** a complete `FrameBuffer` of size 64×64
**When** I call `render_system.render_scene(*fbo)` to render a simple colored mesh into the FBO
**Then** the color attachment texture contains the rendered output

### Story 2 — Read pixels from an FBO (Priority: P1)

As an engine developer, I want to read pixel data from a custom FBO so that I can capture or verify offscreen renderings.

**Given** a complete `FrameBuffer` with rendered content
**When** I call `device.read_pixels(*fbo)`
**Then** I receive an `ImageBuffer` whose pixel data matches what was rendered into the FBO

**Given** a `RenderDevice` in headless mode
**When** I call `device.read_pixels(*fbo)`
**Then** I receive an error with category `Unsupported` and message containing "no display"

### Story 3 — Resize an FBO (Priority: P2)

As an engine developer, I want to resize an FBO so that it adapts to viewport size changes without recreating the object.

**Given** a complete `FrameBuffer` of size 64×64
**When** I call `fbo->resize(128, 128)`
**Then** the FBO remains complete at the new size 128×128

**Given** a complete `FrameBuffer`
**When** I call `fbo->resize(0, 0)`
**Then** I receive an error and the FBO remains in its previous valid state

### Story 4 — Headless backend safety (Priority: P1)

As a CI maintainer, I want the headless backend to handle all FBO operations without crashing so that the test suite can validate the API surface.

**Given** a `RenderDevice` in headless mode
**When** I call `device.create_frame_buffer(64, 64)`
**Then** the result is successful (no crash), returning a valid no-op `FrameBuffer`

**Given** a no-op `FrameBuffer` from the headless backend
**When** I call `bind()`, `unbind()`, or `resize(128, 128)` on it
**Then** none of these operations crash


## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `create_frame_buffer(w, h)` returns a complete, usable `FrameBuffer` for any valid positive dimensions. | Integration test: create 64×64 FBO, check `glCheckFramebufferStatus` returns `GL_FRAMEBUFFER_COMPLETE`. |
| AC-002 | `FrameBuffer::bind()` binds the FBO; subsequent draw calls render into the FBO's attachments. | Integration test: render a colored mesh into bound FBO, `read_pixels(*fbo)` returns correct RGBA values. |
| AC-003 | `FrameBuffer::unbind()` restores the previously bound framebuffer. | Integration test: bind FBO, unbind, verify that the default framebuffer is restored (subsequent rendering goes to window). |
| AC-004 | `FrameBuffer::resize(w, h)` recreates attachments at new size; FBO remains complete. | Integration test: create 64×64 FBO, resize to 128×128, verify completeness and new dimensions via `color_texture().width()/height()`. |
| AC-005 | `read_pixels(FrameBuffer&)` on headless backend returns an error. | Headless unit test: `FrameBuffer_Headless_ReadPixelsFails` asserts error. |
| AC-006 | Headless backend: `create_frame_buffer`, `bind`, `unbind`, `resize`, and `destroy` all succeed without crashing. | Headless unit test: `FrameBuffer_Headless_CreateDestroy` and `FrameBuffer_Headless_Resize` assert no crash. |
| AC-007 | `create_frame_buffer(0, y)` or `(x, 0)` returns an error on both backends. | Unit test: `FrameBuffer_Headless_ZeroSize` asserts error. |
| AC-008 | `render_scene(FrameBuffer&)` renders into the specified FBO. | Integration test: create FBO, call `render_system.render_scene(*fbo)`, read back pixels with `read_pixels(*fbo)` and verify a colored mesh was rendered into the FBO. |
| AC-009 | Zero new compiler warnings. | Build with `-Wall -Wextra -Wpedantic` or equivalent; warning count must not increase. |
| AC-010 | All existing tests pass. | Run `ctest --preset debug` before and after changes; no regressions. |

## E2E Verification

**Method**: Automated integration test suite, split into two tiers:

1. **Headless unit tests** (always run, no display required):
   - `FrameBuffer_Headless_CreateDestroy` — create + destroy doesn't crash
   - `FrameBuffer_Headless_Resize` — resize doesn't crash
   - `FrameBuffer_Headless_ReadPixelsFails` — read_pixels returns error
   - `FrameBuffer_Headless_ZeroSize` — create with 0 size returns error

2. **OpenGL integration test** (conditional on `BUDDD_HAS_DISPLAY=ON`):
   - `FrameBuffer_OpenGL_RenderAndReadback` — create 64×64 FBO, render a simple colored mesh (e.g., a full-screen quad or triangle), read pixels back, verify they match expected color values.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | FBO creation for a 1920×1080 framebuffer completes in under 5 ms (OpenGL backend). |
| SC-002 | FBO bind/unbind overhead is negligible (< 0.01 ms per pair). |
| SC-003 | Headless FBO creation and destruction have no measurable performance impact (no real GL calls). |
| SC-004 | All headless FBO tests pass in CI (no display required). |

## Edge cases

| Case | Expected behavior |
|---|---|
| Zero width or height | `create_frame_buffer(0, 128)` / `create_frame_buffer(128, 0)` → error. `resize(0, 128)` / `resize(128, 0)` → error, FBO unchanged. |
| One dimension zero | Same as zero dimensions — error returned. |
| Very large dimensions | Platform-dependent. If `glTextureStorage2D` fails (e.g., exceeds `GL_MAX_TEXTURE_SIZE`), the creation returns an error. |
| Resize to same dimensions | Should succeed without error; attachments are recreated at the same size. |
| Double resize | `resize(A)`, then `resize(B)` → second resize succeeds, FBO at size B. |
| Bind → resize → unbind | Resizing while bound leaves the FBO in a valid state; binding after resize works correctly. |
| Destroy after resize | Destructor cleans up the current (post-resize) attachments — no double-deletion or leak. |
| Multiple FBOs created sequentially | Each is independent; bind/unbind stack is per-context. |
| `color_texture()` used after destruction | Undefined behavior (use-after-free) — callers must ensure the `FrameBuffer` outlives usage of its texture. |

## Error cases

| Condition | Error category | Message |
|---|---|---|
| `create_frame_buffer(0, h)` or `(w, 0)` | `InvalidArgument` | "FrameBuffer dimensions must be positive" |
| `create_frame_buffer(w, h)` — FBO incomplete | `ResourceCreationFailed` | "Framebuffer is incomplete (status: 0xXXXX)" |
| `resize(0, h)` or `(w, 0)` | `InvalidArgument` | "FrameBuffer dimensions must be positive" |
| `resize()` — FBO incomplete after resize | `ResourceCreationFailed` | "Framebuffer is incomplete after resize (status: 0xXXXX)" |
| `read_pixels(FrameBuffer&)` in headless backend | `Unsupported` | "read_pixels with FBO is not supported in headless mode" |
| `read_pixels()` (parameterless) in headless backend (existing behavior) | `Unsupported` | "read_pixels is not supported in headless mode" |
| `create_render_texture(w, h)` — texture creation fails (e.g., exceeds max size) | `TextureCreationFailed` | Backend-specific error message |

## Permissions and security

- No user-facing permissions apply — this is a graphics engine API.
- No file I/O or network access is involved.
- No new data access controls are required.
- The FBO and its textures live entirely in GPU memory, managed by OpenGL.

## Observability

| Event | Log level | Message |
|---|---|---|
| FBO creation | `INFO` | "FrameBuffer created ({}x{})" |
| FBO destruction | `DEBUG` | "FrameBuffer destroyed" |
| FBO resize | `INFO` | "FrameBuffer resized ({}x{} -> {}x{})" |
| FBO bind | `TRACE` | "FrameBuffer bound (id={})" |
| FBO unbind | `TRACE` | "Default framebuffer restored" |
| FBO incomplete error | `ERROR` | "Framebuffer is incomplete (status: 0xXXXX)" |
| Render texture creation | `INFO` | "Render texture created (OpenGL, {}x{})" |

All logging goes through the project's existing `BUDDD_LOG_TAG` / `BUDDD_LOG_*` macros.

No new metrics or counters are required beyond what the existing `RenderDevice` diagnostics provide.

## Out of scope

- Stencil attachment.
- Depth texture (renderbuffer is sufficient for MVP).
- Multisampling (MSAA).
- FBO pool/cache.
- Editor viewport panel (separate feature F-07).
- `RenderDevice::begin_frame(FrameBuffer*)` or `begin_frame(FrameBuffer&)` overload.
- Changes to `begin_frame()` / `end_frame()` pipeline integration.
- Color attachment format customization (always RGBA8).
- Depth attachment format customization (always DEPTH_COMPONENT24).

## Assumptions

| Assumption | Rationale |
|---|---|
| `FrameBuffer` is used via `std::unique_ptr` (movable, not copyable). | Matches existing engine resource ownership patterns (`Texture`, `Material`, etc.). |
| Color attachment is always RGBA8. | Sufficient for viewport display; can be extended later if needed. |
| Depth attachment is always a renderbuffer (GL_DEPTH_COMPONENT24), not a texture. | Sufficient for occlusion in the viewport. Shadow mapping would require a depth texture, but that is out of scope. |
| `FrameBuffer::bind()` saves the previous FBO and `unbind()` restores it. | Supports nested/scoped FBO usage patterns. Implementation uses `glGetIntegerv(GL_FRAMEBUFFER_BINDING, ...)`. |
| Headless `FrameBuffer` is a no-op with no real GL state. | Follows the existing headless pattern (`TextureHeadless`, `ShaderHeadless`, etc.). |
| The existing `read_pixels()` (no parameters) is unchanged — the new overload is `read_pixels(FrameBuffer&)` for reading from a specific FBO. | Backward compatible: existing callers continue to work without modification; new code passes an FBO reference. |
| The `render_scene(FrameBuffer&)` overload is separate from the parameterless call; no default parameter. | Existing callers of `render_scene()` continue to render to the currently bound framebuffer unchanged. New callers pass an FBO reference explicitly. This matches the existing `read_pixels()` / `read_pixels(FrameBuffer&)` pattern. |
| `glCheckFramebufferStatus` is called once at creation and once after each `resize()`. | Catches driver/hardware issues early. Not called on every `bind()`. |
| Error checking follows the existing `Result<T>` / `Error` pattern. | Consistent with ADR-001. |
| FBO memory usage at 1080p: ~14 MB (1920×1080×4 bytes color + 1920×1080×3 bytes depth). Based on 8.3 MB color + ~5.5 MB depth. | Acceptable for MVP; no pooling needed. |
| `create_render_texture` uses `GL_LINEAR` filtering and `GL_CLAMP_TO_EDGE` wrap. | Standard settings for render targets displayed in ImGui. |

## Open questions

*None.*
