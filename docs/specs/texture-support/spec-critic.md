# Spec Review — SPEC-017: 2D Texture Support for the Render Pipeline (Re-review)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

None. All previously identified blocking issues have been resolved.

### Previously resolved

- [x] **BLOCKING-1: `const Material&` / `bind()` const‑correctness** — RESOLVED. `bind()` is now declared `const` with `mutable next_available_unit_`. The `draw()` code examples correctly use `static_cast<const MaterialOpenGL&>(material)` and call `const bind()`, which compiles fine against the existing `const Material&` draw signatures.
- [x] **BLOCKING-2: `set_texture` type‑checking contradiction** — RESOLVED. The spec now consistently states that `set_texture` checks only uniform name existence, not GLSL type. Section 3 prose, the edge-cases table, and AC-008 all agree.
- [x] **BLOCKING-3: `shared_ptr` vs `unique_ptr` factory return** — RESOLVED. `create_texture` now returns `Result<std::unique_ptr<Texture>>`, matching all other `RenderDevice` factory methods. The spec explicitly notes that callers can wrap in `shared_ptr` when assigning to a material.

## Warnings

Non-blocking concerns for awareness:

- **`set_uniform` deferral is a behavioural change.** The spec explicitly acknowledges this (line 208) and claims no existing caller depends on immediate `glUniform*` side effects. This is a reasonable claim, but the implementation must verify every call site (`RenderSystem::render()`, `cube_demo`, `triangle_demo`, `free_camera_demo`, and all tests) to confirm the deferral is safe. The spec has added AC-015 to confirm `set_uniform` still validates uniform existence immediately (via `glGetUniformLocation`), which mitigates the risk.

- **`TextureOpenGL` destructor may fire after GL context destruction.** The spec returns `unique_ptr<Texture>`, but callers can wrap it in `shared_ptr` for material assignment. If a `shared_ptr<Texture>` outlives the `RenderDevice` (e.g., stored globally or in a component), `glDeleteTextures` will be called on a defunct GL context, causing UB. The spec does not document this lifetime precondition. Not a blocker for v1, but worth documenting in a future revision.

- **AC-019 includes a visual verification criterion** ("Visual output shows a textured rotating cube"). This follows precedent from SPEC-005 AC-026 (also has a visual criterion), so it is consistent. The verification table cannot be automated in headless CI; it is a manual/visual check.

## Required changes

None. No blocking issues remain.

## Suggested improvements

Optional ideas (not required):

- **Minor `const` inconsistency in prose vs code block.** Line 103 says `create_texture` takes an `Image&` reference, but the code block on line 99 shows `const Image&`. The code block is authoritative; the prose is trivially inaccurate. Consider updating line 103 to say `const Image&` for consistency.

- The `CONST-001` citation (line 462) could be more precise: `CONST-001-architecture-boundaries.md`. This is consistent with how all other specs cite it, but the full filename would be more navigable.

- The `Texture` class is non-movable but is returned via `unique_ptr` (which uses move semantics internally). This is fine because the concrete `TextureOpenGL`/`TextureHeadless` objects are moved through the `unique_ptr`, not the abstract base. No issue — just noting the design is sound.
