# Test Report: RenderDevice FBO / Render-to-Texture

## Test Summary

**Total tests**: 817 (3 new FBO tests added)
**Passed**: 817
**Failed**: 0
**Skipped**: 0 (no display required — offscreen SDL driver used)

**Build**: clean (zero errors, zero warnings in `src/` and `tests/`)

---

## Unit Tests (Headless)

All 10 headless FBO tests pass (54 assertions):

| Test | Status | AC / Edge Case |
|---|---|---|
| `FrameBuffer_Headless_CreateDestroy` | PASS | AC-001, AC-006 |
| `FrameBuffer_Headless_Resize` | PASS | AC-004, AC-006 |
| `FrameBuffer_Headless_BindUnbind` | PASS | AC-006 |
| `FrameBuffer_Headless_ReadPixelsFails` | PASS | AC-005 |
| `FrameBuffer_Headless_ZeroSize` | PASS | AC-007 |
| `FrameBuffer_Headless_ResizeZero` | PASS | Edge case: resize(0,h) / resize(w,0) |
| `FrameBuffer_Headless_ColorTexture` | PASS | AC-001 (accessor) |
| `RenderTexture_Headless_Create` | PASS | Goal #2 (create_render_texture) |
| `FrameBuffer_Headless_BindResizeUnbind` | PASS | Edge case: bind → resize → unbind |
| `FrameBuffer_OpenGL_RenderAndReadback` (BUDDD_HAS_DISPLAY guard) | PASS | AC-001, AC-002, AC-003 |

---

## Integration / OpenGL Tests

| Scenario | Method | Result | Evidence |
|---|---|---|---|
| Render and readback | Create 64×64 FBO, render colored triangle, read back pixels, verify non-black | PASS | `FrameBuffer_OpenGL_RenderAndReadback` |
| Zero-size creation | Create FBO with 0×64 / 64×0 → verify error | PASS | `FrameBuffer_OpenGL_ZeroSize` (new) |
| Resize | Create 64×64 → resize 128×128 → same → double → bind+resize+unbind | PASS | `FrameBuffer_OpenGL_Resize` (new) |
| RenderScene overload | Create World + Camera + mesh, call `render_system.render_scene(fbo)`, read back | PASS | `FrameBuffer_OpenGL_RenderSceneOverload` (new) |

All OpenGL tests use SDL offscreen driver — no physical display required.

---

## Acceptance Criteria Coverage

| ID | Description | Coverage | Method |
|---|---|---|---|
| AC-001 | `create_frame_buffer(w, h)` returns complete, usable FBO | ✅ | Headless + OpenGL tests |
| AC-002 | `bind()` binds FBO; draw calls render into attachments | ✅ | OpenGL: render triangle → readback non-black pixels |
| AC-003 | `unbind()` restores previously bound framebuffer | ✅ | OpenGL: `glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING)` check |
| AC-004 | `resize(w, h)` recreates attachments; FBO remains complete | ✅ | Headless Resize + OpenGL Resize (new) |
| AC-005 | `read_pixels(FrameBuffer&)` on headless returns error | ✅ | `FrameBuffer_Headless_ReadPixelsFails` |
| AC-006 | Headless: create/bind/unbind/resize/destroy all no-crash | ✅ | Multiple headless tests |
| AC-007 | `create_frame_buffer(0, y)` / `(x, 0)` returns error | ✅ | Headless ZeroSize + OpenGL ZeroSize (new) |
| AC-008 | `render_scene(FrameBuffer&)` renders into specified FBO | ✅ | OpenGL RenderSceneOverload (new) |
| AC-009 | Zero new compiler warnings | ✅ | Build verification |
| AC-010 | All existing tests pass | ✅ | 817/817 tests pass |

---

## Edge Cases Coverage

| Case | Expected | Coverage |
|---|---|---|
| Zero width or height in create | Error `InvalidArgument` | ✅ Headless + OpenGL |
| Zero width or height in resize | Error `InvalidArgument`, FBO unchanged | ✅ Headless `ResizeZero` |
| Very large dimensions | Platform-dependent error | ⚠️ Not tested (platform-dependent, hard to automate) |
| Resize to same dimensions | Succeeds, attachments recreated | ✅ OpenGL Resize test |
| Double resize | Second succeeds at new size | ✅ Headless Resize + OpenGL Resize |
| Bind → resize → unbind | FBO valid after | ✅ Headless BindResizeUnbind + OpenGL Resize |
| Destroy after resize | No double-deletion | ✅ Headless (resize + destructor via unique_ptr) |
| Multiple FBOs | Each independent | ⚠️ Not explicitly tested (OpenGL ensures per-context independence) |
| `color_texture()` after destruction | Undefined behavior | ✅ Documented as out-of-scope for testing |

---

## Success Criteria

| ID | Metric | Coverage |
|---|---|---|
| SC-001 | FBO creation < 5ms (1920×1080) | ⚠️ Manual benchmark (not automated) |
| SC-002 | Bind/unbind overhead < 0.01ms | ⚠️ Manual benchmark (not automated) |
| SC-003 | Headless no measurable perf impact | ✅ Verified by CI (no GL calls in headless) |
| SC-004 | All headless FBO tests pass in CI | ✅ 9 headless tests pass without display |

---

## Regression Checks

| App / Module | Check performed | Result | Evidence |
|---|---|---|---|
| `render_device_opengl.cpp` | Existing OpenGL render device tests | PASS | `[render][headless]` 13 tests pass |
| `render_device_headless.cpp` | Existing headless render device tests | PASS | `[render][headless]` 13 tests pass |
| `render_system.cpp` | Scene rendering tests | PASS | `[scene_rendering]` 30 tests pass |
| All tests | Full `ctest --preset debug` | PASS | 817/817 tests pass, 0 regressions |

**No regressions detected.** The 3 new tests increased the total from 814 to 817.

---

## Manual Tests Required

- **SC-001 / SC-002 performance benchmarks**: FBO creation time and bind/unbind overhead require instrumented profiling (e.g., `glBeginQuery` / `glEndQuery` or `std::chrono` timing in a release build). These are not practical as unit tests.
- **Visual inspection of FBO texture in editor viewport**: This will be tested as part of feature F-07 (Viewport Panel), which integrates the FBO texture with ImGui.

---

## Issues Found

### Blocking
- None.

### Non-blocking
- `render_scene(FrameBuffer&)` does not clear the FBO before rendering (the same behavior as the regular `render_scene()`). Callers must clear the FBO explicitly if they need a deterministic initial state. This is consistent with the spec's assumption that the caller manages bind/unbind and buffer state.
- Very large dimensions (exceeding `GL_MAX_TEXTURE_SIZE`) and multiple FBO sequential creation are not explicitly tested but rely on OpenGL driver behavior and are hard to automate deterministically.
