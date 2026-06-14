# Implementation Contract Review — RenderDevice FBO / Render-to-Texture

## Summary

**Verdict: ACCEPTED** — Both blocking issues from the previous review are resolved. The contract is now ready for implementation.

The contract author fixed the 2 blocking issues:
1. **AC-003 test added**: The OpenGL integration test now verifies that `unbind()` restores the default framebuffer (lines 593–603 — saves default FBO, binds, verifies binding changed, unbinds, verifies default restored via `glGetIntegerv`).
2. **Viewport save/restore added**: `FrameBufferOpenGL` now saves the viewport via `glGetIntegerv(GL_VIEWPORT, previous_viewport_)` in `bind()` and restores it via `glViewport(previous_viewport_[0], ...)` in `unbind()`. The `previous_viewport_[4]` member is declared at line 189.

Non-blocking warnings were also addressed:
- `create_render_texture` direct test added (Test 8: `RenderTexture_Headless_Create`)
- `bind → resize → unbind` edge case test added (Test 9: `FrameBuffer_Headless_BindResizeUnbind`)

No new issues introduced by the fixes.

---

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **AC-003 not tested**: The spec's AC-003 requires that "Integration test: bind FBO, unbind, verify that the default framebuffer is restored (subsequent rendering goes to window)." The contract's OpenGL integration test (`FrameBuffer_OpenGL_RenderAndReadback`) does not test that `unbind()` correctly restores the previously bound framebuffer. Add a step to the integration test that binds the FBO, unbinds, and verifies the default framebuffer is active (e.g., by checking the FBO binding via `glGetIntegerv` or by issuing a draw to the default framebuffer and reading back).
    *Resolved*: Integration test step 3 (lines 593–603) now saves default FBO via `glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING)`, binds FBO and verifies binding changed via `REQUIRE(bound_fbo != default_fbo)`, unbinds and verifies default restored via `REQUIRE(after_unbind == default_fbo)`.

- [x] **Viewport not restored on `unbind()`**: `FrameBufferOpenGL::bind()` calls `glViewport(0, 0, width_, height_)` to set the viewport to FBO dimensions, but `FrameBufferOpenGL::unbind()` only restores `GL_FRAMEBUFFER_BINDING` — it does **not** restore the previous viewport. After `unbind()`, any subsequent rendering to the default framebuffer (e.g., `app.on_render()`, `render_ui()` for ImGui) will use the FBO-sized viewport instead of the window-sized viewport. The fix: save the viewport in `bind()` (via `glGetIntegerv(GL_VIEWPORT, ...)`) and restore it in `unbind()`. Update the `FrameBufferOpenGL` class to store `GLint previous_viewport_[4]`.
    *Resolved*: `FrameBufferOpenGL` now has `GLint previous_viewport_[4]` member (line 189). `bind()` saves via `glGetIntegerv(GL_VIEWPORT, previous_viewport_)` (line 225). `unbind()` restores via `glViewport(previous_viewport_[0], previous_viewport_[1], previous_viewport_[2], previous_viewport_[3])` (line 231). DC-002 explicitly requires this behavior.

---

## Warnings

Non-blocking concerns for awareness:

- **FBO not explicitly cleared before rendering**: `render_scene(FrameBuffer&)` binds the FBO and calls `render_scene()` (which does not clear). The FBO's initial contents from `glTextureStorage2D` are undefined. The integration test is structured to use a full-coverage triangle (all pixels overwritten), so undefined initial data does not cause test flakiness. However, for general use, the `render_scene(FrameBuffer&)` overload should clear the FBO before rendering (at least `GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT`). Consider adding a `clear()` method to `FrameBuffer` or including a clear in the overload.

- **`create_render_texture` has no direct unit test**: The spec lists `create_render_texture(w, h)` as a public API on `RenderDevice` (Goal #2). It is only tested indirectly via `create_frame_buffer`. While the FBO creation internally creates a texture via the same path, a dedicated headless test (e.g., `RenderTexture_Headless_Create`) would ensure the standalone API is tested. Consider adding one.

- **`Bind → resize → unbind` edge case not tested**: The spec lists this as an edge case (line 171). The contract describes the correct behavior but does not include a test. Consider adding a headless or OpenGL test for this scenario.

- **`Texture::width()` returns `int`, `FrameBuffer::width()` returns `uint32_t`**: Minor API inconsistency. Both are internally consistent but callers may need casts when comparing values.

- **`FrameBuffer` self-forward-declaration**: The abstract class header includes `class FrameBuffer;` before `class FrameBuffer {` (line 114 of the contract). This is harmless but unnecessary.

---

## Required changes

All previously requested changes have been implemented:

1. ✅ Add an integration test step for AC-003 (verify unbind restoration).
2. ✅ Add viewport save/restore to `FrameBufferOpenGL::bind()` / `unbind()`.

---

## Suggested improvements

Optional ideas (not required):

1. Add a `clear()` method to `FrameBuffer` (or clear inside `render_scene(FrameBuffer&)`) for deterministic FBO initial state.
2. Add a direct headless test for `create_render_texture` as a standalone API.
3. Add a test for `bind() → resize() → unbind()` edge case.
4. Consider whether `FrameBuffer::width()`/`height()` should return `int` to match `Texture`'s convention, or keep `uint32_t` and document the mismatch.

---

## Definition of Ready check

Per the DoR criteria at `docs/wiki/engineering/definition-of-ready.md`:

| Criterion | Status | Notes |
|---|---|---|
| Scope clearly defined (included + explicitly excluded) | ✅ | Non-goals section is comprehensive and matches spec |
| Dependencies identified | ✅ | Files to inspect, ADRs, build system noted |
| Edge cases and error conditions described | ✅ | 11 edge cases, 7 error conditions documented |
| Expected behavior unambiguous and testable | ✅ | Detailed implementation specs with code listings |
| Verification defined (E2E) | ✅ | Headless unit tests + OpenGL integration test |
| Acceptance criteria specific, measurable, verifiable | ✅ | Each AC maps to specific test(s) — AC-003 now tested via glGetIntegerv verification in integration test |
| Success and failure states described | ✅ | Error categories and messages match spec error table |
| Interface changes documented | ✅ | Full API signatures listed |
| Documentation to update listed | ✅ | Wiki pages, README, ADRs all covered |
| Technical constraints identified | ✅ | DSA OpenGL, build system (GLOB), conditional compilation |
| Risks or unknowns surfaced | ✅ | Architecture decisions documented |
| Performance implications noted | ✅ | ~14MB at 1080p, bind performance noted |

The contract satisfies all DoR criteria. All blocking issues are resolved.
