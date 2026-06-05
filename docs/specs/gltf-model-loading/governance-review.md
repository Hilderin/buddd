# Governance Review — glTF Model Loading

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **BL-001: Asset-manager spec non-goals not updated** — Fixed: updated `docs/specs/asset-manager/spec.md` non-goals and out-of-scope references to reflect that glTF model loading is now supported via `ModelAsset`.

- [x] **BL-002: Wiki not updated** — Fixed: wiki-agent updated `docs/wiki/architecture/module-map.md` and `docs/wiki/domain/glossary.md` with entries for ModelAsset, ModelNode, PbrMaterial, PbrMaterialData, ModelLoader, model_utils.h, pbr/ submodule, and demo apps.

- [x] **BL-003: Spec tinygltf version mismatch** — Fixed: spec Appendix C, assumption A-01, and all references changed from `v2.10.0` to `v2.9.7` to match ADR-018 and actual implementation. Also fixed `add_model_to_world()` signature to match implementation (non-const `ModelNode&`, `World&`/`Entity` instead of `Registry&`/`entt::entity`).

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [ ] No violations found. All checks pass:
  - **CONST-001 (Architecture Boundaries)**: tinygltf is a PRIVATE dependency, no tinygltf types appear in public headers (confirmed by code review DC-20). No GL types leak outside the render layer. `PbrMaterial` follows the existing `PhongMaterial` pattern. `ModelNode` is in `render/` (contains `Model`), `ModelAsset` is in `asset/`. ✓
  - **CONST-002 (Testing Policy)**: All 348 tests pass (0 failures). 21 DC-12 required tests present. All 28 ACs pass. ✓
  - **CONST-003**: Placeholder (TODO). Not violated by this feature.
  - **CONST-004**: Placeholder (TODO). Not violated by this feature.
  - **Charter/Principles**: All principles respected — existing conventions followed (FetchContent, PIMPL, embedded shaders), existing document update list provided, testable requirements throughout.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-018 (tinygltf-dependency)**: Created by the adr-agent, status `Accepted`. Documents the tinygltf v2.9.7 dependency decision, FetchContent integration, PRIVATE linkage, custom image loader callback, stb_image ODR avoidance, and CONST-001 compliance. ✓
- [x] No additional ADRs required. The friend-`AssetManager` pattern for `ModelAsset::replace_root()` is documented as Decision #6 in coordination.md, and the spec + implementation contract cover it adequately. No new ADR needed per contract note.

## Wiki alignment

Wiki reflects current state and does not become law:

- [ ] **BL-002 (carried from above)**: Wiki modules and glossary must be updated to reflect new types. The wiki is documentation, not law, but must reflect the current state of the codebase. See BL-002 above.

## Warnings

Non-blocking concerns for awareness:

- **W-001: Spec v2.10.0 vs ADR/code v2.9.7** — The Git tag `v2.10.0` does not exist upstream. Spec Appendix C should use `v2.9.7` to match ADR-018 and the actual implementation.
- **W-002: `add_model_to_world()` signature differs from spec** — The spec (line 292-296) declares `add_model_to_world(Registry& registry, const ModelNode& node, ...)` with `const ModelNode&`. The implementation uses non-const `ModelNode&` because `Model` is move-only. The spec should be updated to match the real signature. (Code review W-001)
- **W-003: 6 edge-case tests not implemented** — The implementation contract's full test table (33 tests) is missing 6 edge-case tests (nos. 26-30, 33) including `add_model_to_world` hierarchy preservation (P1). None are in DC-12, so non-blocking.
- **W-004: Double texture decode** — Embedded glTF images decoded twice (once by custom callback, once by the loader). Performance issue only. (Code review W-002)
- **W-005: Unsupported primitive mode in spec but code handles it** — Minor: spec line 452 says unsupported modes "return error" but line 588 says they are "skipped with warning." The implementation contract and code correct to "skipped with warning." The spec's own description at line 588 is the correct intent.
- **W-006: Licenses for Khronos test models not documented** — Spec-critic W-004 noted that Khronos Box and DamagedHelmet models need license/attribution documentation. This remains unaddressed.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **`docs/specs/asset-manager/spec.md`**: Update non-goals (line 62) and out-of-scope (line 1113) to remove glTF as a non-goal, reflecting that `ModelAsset` is now supported. **(Required for BL-001)**
- **`docs/specs/gltf-model-loading/spec.md`**: Update Appendix C tinygltf Git tag from `v2.10.0` to `v2.9.7` to match ADR-018 and actual implementation. Update `add_model_to_world()` signature to show non-const `ModelNode&`.
- **`docs/wiki/architecture/module-map.md`**: Add entries for `ModelAsset`, `ModelNode`, `PbrMaterial`, `ModelLoader`, `pbr_shaders.h`, `model_utils.h`, and the `pbr/` subdirectory. **(Required for BL-002)**
- **`docs/wiki/domain/glossary.md`**: Add terms `ModelAsset`, `ModelNode`, `PbrMaterial`, `PbrMaterialData`. **(Required for BL-002)**
