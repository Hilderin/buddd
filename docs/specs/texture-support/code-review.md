# SPEC-017 / IMPL-017-001 Review — 2D Texture Support

## Blocking issues

- [x] (none — all issues are non-blocking)

## Warnings

Non-blocking concerns for awareness:

1. **Tests 8, 9, 10 test at Image::create level, not directly at create_texture level** — The zero-width, zero-height, and empty-data error paths are tested through `Image::create()` which catches these conditions before `create_texture` is reached. The `create_texture` validation in both backends (defense-in-depth) is therefore not directly exercised by these tests. The implementer acknowledged this in coordination.md. Acceptable because the validation code is present and correct.

2. **Missing direct test for data size mismatch** — The spec error case "Image data size mismatch" (`image.data().size() != image.width() * image.height() * channels`) is not directly tested. The validation exists in both backends (`render_device_opengl.cpp:310`, `render_device_headless.cpp:368`) but no test constructs an `Image` with mismatched data to exercise it. Low risk because the condition is validated identically in both backends.

3. **Missing tests for 1-channel (grayscale) and 3-channel (RGB) texture creation** — The OpenGL format mapping for 1-channel (`GL_R8`/`GL_RED`) and 3-channel (`GL_RGB8`/`GL_RGB`) is implemented but not covered by headless tests. Only 4-channel RGBA is tested (Test 1). Low risk — the format mapping is straightforward and the validation catches unsupported channels.

4. **Headless draw debug log has a syntax quirk** — `render_device_headless.cpp:399-400` and `:417-418` contain `/*vertex_count*/ "?"` (a commented-out parameter reference followed by a string literal). This is harmless but slightly inconsistent with the debug output pattern. Would be cleaner as `<< vertex_count` or a proper message.

5. **visual verification of AC-019 (textured-cube demo) not performed in review** — The textured-cube demo requires a real display (SDL OpenGL context) and could not be run in this environment (the SDL `dummy` driver does not support OpenGL). The demo compiles and the code structurally follows the scene-graph pattern. The `buddd capture cube` scenario (pre-existing) was verified and passes, confirming no rendering regression.

6. **`TextureOpenGL` destructor may fire after GL context destruction** — Already noted in spec-critic and contract-critic phases. If a `shared_ptr<Texture>` outlives the `RenderDevice`, calling `glDeleteTextures` on a destroyed context is UB. Not a new issue; documented risk.

## Required changes

None.

## Suggested improvements

Optional ideas (not required):

1. Add a direct test for `create_texture` data size mismatch (construct an `Image` with valid dimensions but wrong data size and assert `InvalidArgument`).
2. Add a test for 1-channel and 3-channel texture creation to verify the format mapping.
3. Add a `textured-cube` capture scenario to `capture_command.cpp` for future visual regression testing.
4. Fix the headless draw debug log to use the actual vertex/index count instead of `"?"`.
