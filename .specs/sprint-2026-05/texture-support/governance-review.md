# Governance Review — 2D Texture Support (SPEC-017 / IMPL-017-001)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec ↔ Implementation Contract**: Fully consistent. All 21 ACs are covered in the contract. `create_texture` returns `Result<std::unique_ptr<Texture>>` in both. `bind()` is `const` with `mutable` unit counter in both. `set_texture` checks uniform name existence (not GLSL type) in both. Error categories align. Edge cases are handled identically.
- [x] **Implementation Contract ↔ Code**: All required behavior is faithfully implemented. `texture.h` matches the exact interface. `MaterialOpenGL::set_uniform` defers to `bind()`. `MaterialOpenGL::bind()` follows the exact contract sequence. `RenderDeviceOpenGL::draw()` calls `mat.bind()` as first operation. `TextureCreationFailed` error category exists. 13 headless tests + 1 OpenGL-only test cover all ACs. Demo registered and structured per contract.
- [x] **Spec-critic + Contract-critic blocking issues**: All resolved. Spec-critic had 3 blocking issues (const-correctness, set_texture type-checking contradiction, unique_ptr vs shared_ptr return) — all fixed. Contract-critic had 0 blocking issues.
- [x] **Code-review blocking issues**: None. All 6 code-review items are warnings (non-blocking).
- [ ] **Wiki wrapping mode inconsistency**: Wiki (glossary, data-flow.md, module-map.md) states `GL_REPEAT` for texture wrapping mode, but the spec (§4), implementation contract (§5), and actual code (`render_device_opengl.cpp:330-333`) all use `GL_CLAMP_TO_EDGE`. The wiki is inconsistent with the authoritative documents and the implementation. **This is a wiki documentation bug — not blocking (the spec/contract/code are correct and consistent), but it should be corrected for cross-document coherence.**

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)**: No violation. `texture.h` exposes no backend types (`grep` for `SDL_`, `gl`, `GL_`, `GLAD`, `stb_` returns no matches). `TextureOpenGL` and `TextureHeadless` are private headers inside `src/engine/render/`. Demo code uses only abstract `Texture`, `Material`, `RenderDevice` interfaces — no platform/graphics headers leak outside `src/engine/`.
- [x] **CONST-002 (Testing Policy)**: Satisfied. 13 headless test cases + 1 OpenGL-only test in `tests/texture_tests.cpp`. All acceptance criteria from AC-001 through AC-021 are covered. All existing tests continue to pass (no regressions confirmed by code-reviewer).
- [x] **CONST-003 (Documentation Policy)**: TODO/unwritten — not violated by this feature. The feature itself is well-documented.
- [x] **CONST-004 (Security Policy)**: TODO/unwritten — not violated by this feature. No security concerns introduced (no network, no privileges, no new parsing dependencies).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-001 (Result/Error Pattern)**: Respected. `create_texture` returns `Result<std::unique_ptr<Texture>>`. `set_texture` returns `Result<void>`. Draw methods remain `void` per ADR-003 carve-out.
- [x] **ADR-003 (Render Pipeline Architecture)**: Respected. `bind()` returns `void` (GPU state command, not fallible). Draw methods remain `void`. The `glUseProgram` ordering fix is implemented inside `bind()` without changing the draw method signature.
- [x] **ADR-010 (No Raw Pointers in Public API)**: Respected. Material takes `std::shared_ptr<Texture>` (not raw pointer). All public API signatures use `std::string_view`, `std::shared_ptr`, references, or `Result<T>`.
- [x] **ADR-012 (Navigable Object Graph / EngineService)**: Respected. Tests use `EngineService::create(Backend::Headless)`. The textured-cube demo accesses platform via `device.window().platform()`.
- [x] **ADR-011**: Empty file (pre-existing). The texture feature does not depend on it and does not make it worse. No action needed.
- [x] **ADR-agent decision confirmed**: No new ADR needed for this feature — all decisions (mutable keyword, deferred uniform caching, unique_ptr→shared_ptr conversion, TextureCreationFailed error category) follow existing patterns.

## Wiki alignment

Wiki reflects current state and does not become law:

- [ ] **glossary.md** — TextureOpenGL entry incorrectly states `GL_REPEAT` wrapping. The actual implementation uses `GL_CLAMP_TO_EDGE` (matching spec and contract). Must be corrected to `GL_CLAMP_TO_EDGE`.
- [ ] **data-flow.md** — Lines 179-180 show `GL_REPEAT` for texture parameterization. Must be corrected to `GL_CLAMP_TO_EDGE`.
- [ ] **module-map.md** — Line 147 (texture_opengl.cpp description) states `GL_REPEAT`. Must be corrected to `GL_CLAMP_TO_EDGE`.
- [x] **architecture/overview.md** — Correctly references textured-cube demo and texture classes. No contradictions.
- [x] **domain/business-rules.md** — Updated Error::Category listing to include `TextureCreationFailed`. Consistent with `error.h`.
- [x] **architecure/dependency-map.md** — Documents render→image dependency for `create_texture`. Accurate.

## Warnings

Non-blocking concerns for awareness:

1. **Wiki wrapping mode incorrect** — Three wiki files (glossary.md, data-flow.md, module-map.md) state `GL_REPEAT` but the spec, contract, and code all use `GL_CLAMP_TO_EDGE`. This is a factual error in the wiki that should be corrected for consistency, but it does not affect the correctness of the implementation.
2. **TextureOpenGL destructor may fire after GL context destruction** — If a `shared_ptr<Texture>` outlives the `RenderDevice`, `glDeleteTextures` is called on a defunct GL context (UB). This risk already exists for `MaterialOpenGL::program_` and is not new. Documented in spec-critic and contract-critic phases. Acceptable for v1; should be addressed in a future revision.
3. **Missing direct test for data size mismatch** — The validation exists in both backends (`render_device_opengl.cpp:310`, `render_device_headless.cpp:368`) but no test directly exercises it. Low risk.
4. **Missing tests for 1-channel and 3-channel texture creation** — Only 4-channel RGBA is tested headless. Low risk — the format mapping is straightforward.
5. **Tests 8/9/10 validate at Image::create level** — The zero-width/height/empty-data error paths are caught by `Image::create()` before reaching `create_texture`. The defense-in-depth validation in `create_texture` is therefore not directly tested. Acceptable.
6. **Headless draw debug log uses `"?"` instead of vertex count** — `render_device_headless.cpp` lines 399-400 and 417-418 have `/*vertex_count*/ "?"` — harmless cosmetic issue.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- Correct `GL_REPEAT` to `GL_CLAMP_TO_EDGE` in:
  - `docs/wiki/domain/glossary.md` (TextureOpenGL definition)
  - `docs/wiki/architecture/data-flow.md` (lines 179-180)
  - `docs/wiki/architecture/module-map.md` (line 147)
