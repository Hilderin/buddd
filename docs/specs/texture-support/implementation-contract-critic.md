# Implementation Contract Review — 2D Texture Support

## Blocking issues

Items that must be resolved before the artifact can be accepted.

**None.** The contract faithfully implements all 21 ACs from SPEC-017, follows all existing architectural conventions, handles all edge cases, and provides clear implementation instructions with no blockers.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

1. **TextureOpenGL lifetime vs GL context destruction (carried forward from spec-critic)** — `shared_ptr<Texture>` stored in `Material::texture_map_` can outlive the `RenderDevice`/GL context. If `TextureOpenGL` destructor fires after context destruction, `glDeleteTextures` is invalid. This risk already exists for `MaterialOpenGL::program_` (shared_ptr in Model), so the contract does not introduce new risk, but the lifetime precondition should be documented in a future update.

2. **`bind()` does not reuse the existing `location_cache_` for uniform application** — The contract's `bind()` calls `glGetUniformLocation` directly in the loop (line 346) instead of using the cached locations in `location_cache_`. This is correct but slightly wasteful. A future optimization could query the cache first and fall back to `glGetUniformLocation`. Not blocking — correctness is preserved.

3. **`has_texture` semantics mismatch between spec prose and AC-008 is now resolved** — The contract correctly follows the resolved interpretation (`has_texture` checks uniform existence, not whether a texture was set), consistent with `has_uniform` semantics. No action needed.

## Required changes

Concrete, actionable changes requested:

**None.** The contract is complete and ready for implementation.

## Suggested improvements

Optional ideas (not required):

1. **Forward-declare `Image` in `render_device.h` instead of including `image/image.h`** — The contract offers both options ("include or forward-declare"). Forward-declaration is preferred: it is consistent with the existing pattern in `render_device.h` (see forward declarations for `Shader`, `Material`, `VertexBuffer`, etc.) and avoids adding a new include dependency. Consider making this explicit.

2. **Prefer `std::holds_alternative`/if-else chain over `std::visit`** for the `bind()` uniform dispatch — The contract's note about the compiler-level C++26 requirement is valid, but `std::visit` with the current variant types in a `bind()` hot path may generate more code. The existing `MaterialHeadless::get_uniform_mat4` already uses `std::holds_alternative`. Consistency with the existing codebase pattern is preferable.

3. **Test 3/4 setup detail** — The test description says "Create a material with known_uniforms containing u_tex". The Code Agent should create minimal but non-empty shaders (headless backend rejects empty shader source) and either embed `uniform sampler2D u_tex;` in the shader source or pass `known_uniforms = {"u_tex"}` as the third argument to `create_material`. Consider making this explicit in the test setup description.
