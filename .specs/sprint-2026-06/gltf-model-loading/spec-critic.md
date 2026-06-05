# Spec Review — glTF Model Loading

## Re-review summary (2026-06-05)

Re-reviewed `.specs/sprint-2026-06/gltf-model-loading/spec.md` after fixes for BL-001 and BL-002.

**BL-001**: RESOLVED. A "Documents requiring updates" section (lines 727-735) lists all 4 required documents: `.specs/sprint-2026-06/asset-manager/spec.md`, `docs/wiki/architecture/module-map.md`, `docs/wiki/domain/glossary.md`, and a new tinygltf ADR in `docs/adr/`.

**BL-002**: RESOLVED. All texture loading references are consistent:
- Goal (line 27): "loaded directly from image data ... Not cached as TextureAsset"
- Section 7 (line 332): "loaded directly from image data, not as TextureAsset instances"
- Flow step e (line 209): "Load textures → shared_ptr<Texture>"
- Section 4 step 3 (lines 280-285): "direct Image::load + RenderDevice::create_texture"

All Definition of Ready criteria are now satisfied. No new blocking issues found. The spec is ready for implementation.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **BL-001: Existing documentation requiring updates is not listed** — (RESOLVED) The spec now has a "Documents requiring updates" section (lines 727-735) listing all required documents.
  - `.specs/sprint-2026-06/asset-manager/spec.md`
  - `docs/wiki/architecture/module-map.md`
  - `docs/wiki/domain/glossary.md`
  - `docs/adr/` — new tinygltf ADR

- [x] **BL-002: Contradictory texture loading strategy** — (RESOLVED) All references now consistently state that textures are loaded directly from image data as `shared_ptr<Texture>`, not as `TextureAsset`.

## Warnings

Non-blocking concerns for awareness:

- **W-001: `RenderDevice::create_texture()` API compatibility unverified** — Assumptions A-07 and A-08 state that `RenderDevice::create_texture()` accepts raw pixel data (not just `Image` objects loaded from files) or that `Image` can be constructed from raw pixel data. The existing specs (texture-support spec) show `create_texture(const Image&)` taking only an `Image`. If `Image` has no constructor/factory from raw pixel data (width, height, channels, data pointer), the embedded texture loading path for glTF buffer views cannot be implemented as specified. This should be verified against the actual `Image` class API before implementation.

- **W-002: AC-015 title/description mismatch** — The acceptance criteria title says "PbrMaterial with embedded shaders compiles **and renders**" but the verification method only checks "known uniforms include standard PBR uniforms." If "renders" implies GPU output, it cannot be verified in headless mode. The description should be aligned with the verification (e.g., remove "and renders" or add a separate GPU test).

- **W-003: `doubleSided` flag stored but effect unspecified** — The spec correctly reads `doubleSided` from glTF and stores it in `PbrMaterialData::double_sided`. However, the spec does not specify how this flag affects rendering (e.g., face culling mode). The shader code in Appendix A also does not use it. The spec should document whether double-sided rendering is implemented in V1 or deferred (and if deferred, note that the flag is stored but not applied).

- **W-004: No license/attribution for Khronos test models** — The spec commits Khronos Box and DamagedHelmet as test models in the repo. The spec does not mention license compliance or attribution requirements for these third-party assets. The models are under the Khronos Group's sample model license (generally permissive, but attribution may be required). This should be documented.

- **W-005: Hot-reload dependency on FileWatcher extension** — The spec assumes the existing `poll_file_events()` mechanism will detect `.gltf`/`.glb` changes automatically. The FileWatcher watches the `assets/` directory recursively, so `.gltf`/`.glb` modifications should be detected. However, the hot-reload handler in the AssetManager must be extended to process model file changes (it currently only handles YAML, image, and shader file changes). The spec should confirm this extension is within scope and/or explicitly document it.

- **W-006: Headless texture inspection capability** — AC-017 requires verifying "texture is not null and has 1×1 magenta content" (pixel-level inspection). This depends on the headless texture backend supporting pixel-level readback. If `TextureHeadless` stores pixel data accessibly, this is fine; if not, the AC cannot be verified in headless mode. The spec should confirm headless texture inspection is possible.

## Required changes

Concrete, actionable changes requested:

1. ~~**Add a "Documents requiring updates" section** to the spec~~ — **DONE**. Section added at lines 727-735 listing all 4 documents.

2. ~~**Resolve the texture loading contradiction**~~ — **DONE**. All references now consistently state textures are direct `shared_ptr<Texture>` not `TextureAsset`.

## Suggested improvements

Optional ideas (not required):

- Consider documenting how `TEXCOORD_1` (texcoord2) is handled in the PBR shader. Currently the vertex shader declares `a_texcoord2` (location 5) as "Reserved, not used in V1." Fragments of this coordinate system from the lighting spec carry over, but the PBR shader doesn't use it. This is fine, but a note would help future implementers.
- Consider adding a test AC for the YAML `settings.scale` with negative values (mirroring) as documented in the Error cases table (line 636) but not covered by a dedicated AC.
- Consider clarifying that the `ModelNode` tree is traversed depth-first in the hierarchy building algorithm (the pseudo-code implicitly does this via recursion, but it's not explicitly stated).
- The hot-reload E2E description (line 583) says "modifies the source .gltf file programmatically" — it may be worth noting that this requires the running user to have write permissions to the assets directory, and that headless testing can bypass this via synthetic `FileEvent` injection (as described in AC-021).
