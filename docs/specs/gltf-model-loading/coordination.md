# Workflow Coordination: gltf-model-loading

## Orchestrator

**Feature**: `gltf-model-loading`
**Status**: completed
**Current step**: completed
**Initial instructions**: Ajouter la possibilité de loader des modèles glTF depuis l'AssetManager, avec support du hot-reload. Télécharger des modèles de KhronosGroup (Box + DamagedHelmet) dans assets/models/[folder du model].

**Notes**:

**Loop 1 (spec-author)**: spec-critic flagged BL-001 (missing doc update list) and BL-002 (texture loading contradiction). Human resolved BL-002: use raw `shared_ptr<Texture>` owned by PbrMaterial, not `TextureAsset`. Spec-author fixing both.

**Loop 2 (implementation-contract)**: implementation-contract-critic flagged BL-001 (inconsistent error category for corrupt glTF). Spec and human resolved: add new `Error::Category::InvalidFormat` for corrupt files. Implementation-contract-author fixing contract.

**Loop 3 (code-implementer)**: code-reviewer flagged BL-001 (magenta fallback texture never applied — `ensure_texture` lambda defined but not called) and BL-002 (13 missing required tests). Code-implementer fixing both.

**Loop 4 (rendering fix)**: User reported black screen — caused by PBR shader with no scene lights and ultra-low ambient (0.03). Fixed: (1) added ambient fallback + directional fallback light in shader when u_light_count == 0, (2) increased ambient from 0.03→0.1, (3) added DirectionalLightComponent in gltf_demo_app and hot_reload_gltf_app.

**Loop 5 (hot_reload_gltf_app fix)**: Model rendered all (5,5,13) — camera was looking in wrong direction (yaw=180° without look_at). Fixed camera to use look_at(0,0,0) with orbit animation. Also fixed double `make_full_path` in `handle_source_change` for ModelAsset (was causing YAML parse error → hot-reload failure).

**Loop 6 (hot_reload_gltf_app rewrite)**: Rewrote hot_reload_gltf_app to swap YAML settings.scale (1.0 → 2.0 → 0.5) instead of loading heavy DamagedHelmet model (10s load). App now demonstrates hot-reload by changing box size at frames 30 and 70, with entity lifecycle management (destroy/recreate).

### Decision Log

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| 1 | glTF library | tinygltf via FetchContent | Header-only, minimal footprint, C++11, covers glTF 2.0. Assimp was initially considered but rejected as too heavy. |
| 2 | Metadata approach | YAML wrapper (`type: Model`) | Uniform with existing Texture/Material asset pattern. Extensible (version, settings). |
| 3 | glTF material mapping | PbrMaterial with embedded shaders | New self-contained PBR material like PhongMaterial. Created automatically by AssetManager during glTF loading. |
| 4 | V1 scope | Meshes + PBR materials + ModelNode hierarchy + .glb/.gltf + hot-reload | No animations, cameras, lights, Draco compression in V1. |
| 5 | Hierarchy handling | ModelNode tree (not flattened) | Each glTF mesh node becomes a `ModelNode` with local transform + optional Model. Preserves hierarchy for future animation work. |
| 6 | Hot-reload strategy | In-place update via friend AssetManager | AssetManager is friend of Model. Private method replaces GPU buffers and materials on hot-reload, like Texture::replace_gl_handle(). |
| 7 | YAML schema | `type: Model`, `version: 1`, `source: path/to/model.gltf`, `settings: { scale: 1.0 }` | Simple, extensible. scale applied to all vertices. |
| 8 | Test models | Box + DamagedHelmet, committed in repo | Box (~few KB) for minimal test, DamagedHelmet (~7MB) for PBR with textures. |
| 9 | Demo app | `gltf_demo_app` | Loads model from AssetManager, traverses ModelNode tree, creates ECS entities with MeshRenderer + LocalTransform, continuous Y rotation. |
| 10 | Hot-reload test app | `hot_reload_gltf_app` | Like hot_reload_app for materials: two model variants, file swap at frame N, validates hot-reload. |
| 11 | Missing texture fallback | Magenta (1x1) | Consistent with RenderDevice::fallback_material(). Visible immediately. |
| 12 | Shader strategy | Embedded GLSL PBR shaders in PbrMaterial | Same pattern as PhongMaterial. No external shader files needed for PBR. |
| 13 | Edge cases | Error on corrupt glTF, empty ModelNode if no mesh, Error if missing position attribute | Uses existing Result pattern throughout. |

### Definition of Ready Check

#### Clarity & Completeness
- [x] Scope is clearly defined (what is included and what is explicitly excluded)
- [x] Dependencies on other features, modules, or external systems are identified
- [x] Edge cases and error conditions are described
- [x] The expected behavior is unambiguous and testable

#### Verification
- [x] The spec defines how the feature will be verified end-to-end (gltf_demo_app + hot_reload_gltf_app + headless tests)
- [x] Acceptance criteria are specific, measurable, and verifiable
- [x] Success and failure states are described

#### Documentation
- [ ] Interface changes (API signatures, YAML schemas) are documented — *to be captured in spec*
- [ ] Existing documentation that must be updated is listed — *to be captured in spec*

#### Technical
- [x] Technical constraints are identified (tinygltf, GLSL PBR shaders, GLM math)
- [x] Risks or unknowns are surfaced
- [x] Performance or resource implications, if any, are noted

## spec-author

**Status**: completed
**Summary**:
Fixed BL-001 (added "Documents requiring updates" section listing 4 documents) and BL-002 (resolved texture loading contradiction: textures are raw shared_ptr<Texture> owned by PbrMaterial, not TextureAsset). Updated Goals, Section 4, Section 7.
**Artifacts**:
- `docs/specs/gltf-model-loading/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: accepted
**Summary**:
Re-review after BL-001 and BL-002 fixes. BL-001 resolved: "Documents requiring updates" section added listing 4 documents (asset-manager/spec.md, module-map.md, glossary.md, new tinygltf ADR). BL-002 resolved: all texture loading references now consistently state textures are loaded directly from image data as `shared_ptr<Texture>`, not as `TextureAsset`. All Definition of Ready criteria satisfied. No new blocking issues found. Spec is accepted and ready for implementation.
**Artifacts**:
- `docs/specs/gltf-model-loading/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- W-001: RenderDevice::create_texture() API compatibility for embedded glTF textures is unverified (Assumptions A-07/A-08).
- W-002: AC-015 title says "compiles and renders" but verification only checks known uniforms.
- W-003: doubleSided flag is stored but its rendering effect (face culling) is unspecified.
- W-004: No license/attribution documented for Khronos test models (Box, DamagedHelmet).
- W-005: Hot-reload handler extension for .gltf/.glb file changes within AssetManager is assumed but not explicitly confirmed in scope.
- W-006: AC-017 requires pixel-level headless texture inspection — capability not confirmed.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Added `src/engine/render/model_utils.h` with `add_model_to_world()` free function to the implementation contract: added new file to the "files to create" table, added section 12 in Required implementation behavior with full API signature and traversal logic (adapted to use `World`/`Entity` instead of the spec's `Registry`/`entt::entity` to match actual engine types), added 3 new test cases and 4 new Done Criteria. Updated gltf_demo_app description to note it uses the utility.
**Artifacts**:
- `docs/specs/gltf-model-loading/implementation-contract.md`
**Questions for human**:
- The spec (Section 3a) uses `Registry&` and `entt::entity` — these don't match the engine's custom ECS (`World`, `Entity`, `Entity::none()`). The contract corrects the signature to use actual engine types. Please confirm this adaptation is correct.
**Warnings**:
- W-001: `Image::create(const ImageBuffer&)` flips rows vertically (bottom-left → top-left). glTF embedded textures may be top-left origin. If the rendered result appears flipped, the Image flipping logic may need adjustment (existing behavior for all engine images).
- W-002: Spec AC-015 title says "compiles and renders" but verification only checks known uniforms (spec-critic W-002). This contract uses the spec's verification method (known uniforms check). The title mismatch is noted but not blocking.
- W-003: `doubleSided` flag is stored but NOT applied to rendering in V1. Face culling remains at backend default. This is consistent with spec Appendix A (shader doesn't use the flag) but may be unexpected.
- W-004: 72-byte Vertex format means un-needed fields (tangent, texcoord2) consume memory per vertex. This matches ADR-013's exception for glTF models loaded via AssetManager (standard Vertex is required).
**Blocking issues**:
none

## implementation-contract-critic

**Status**: accepted
**Summary**:
Re-review after BL-001 fix. BL-001 resolved: `InvalidFormat` used consistently across contract (preamble, enum addition, implementation behavior at line 343, Test #10 at line 609, DC-12 at line 726, modified files at line 112). No new blocking issues found. 6 warnings remain (5 carried forward + 1 new: W-006 about .bin edge case IoFailed/InvalidFormat mismatch). Contract is accepted — ready for code implementation.
**Artifacts**:
- `docs/specs/gltf-model-loading/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- W-001: ModelLoadResult::materials vector appears unused after construction — may confuse implementer.
- W-002: Several spec error cases lack explicit test coverage (YAML syntax error, missing source field, missing glTF source file, out-of-bounds accessor indices).
- W-003: Spec-critic warnings W-001 through W-006 remain unaddressed — the code implementer should be aware of these risks.
- W-004: No explicit test for normal texture slot population (Test #6 only checks "materials have textures" generically).
- W-005: tinygltf exception safety not documented — relevant if compiling with -fno-exceptions.
- W-006: Edge case table (line 648) says missing .bin → IoFailed, but implementation behavior (line 343) wraps ALL tinygltf parse failures as InvalidFormat. Minor inconsistency — implementation behavior is prescriptive and takes precedence.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-05 12:08 EDT
**Warnings**:
none
**Notes**: Approved to proceed with implementation. All 28 ACs, model_utils.h utility, InvalidFormat error category, box + damaged-helmet test models, 3 demo apps, and tinygltf via FetchContent confirmed.

## code-implementer

**Status**: completed (loop 1)
**Summary**:
Fixed BL-001 (magenta fallback texture now applied — `ensure_texture` lambda moved before first use and called after each texture slot assignment in `create_pbr_material()`). Fixed BL-002 (added 11 missing test cases for AC-011/012/017/018/019/020/021/024/025/027/028). Fixed W-001 (use-after-move of `load_result->root` in `asset_manager.cpp` — moved vertex count and children count before the move). Fixed debug log crash in `model_loader.cpp` (out-of-bounds access when material_idx < 0). Fixed `read_attribute` buffer resize bug (used `num_components` instead of `expected_components` causing VEC3→VEC4 expansion crash). Fixed uint32 index handling bug (first Uint32 primitive data was not appended). Made `replace_root()` private with `friend AssetManager` access.
**Artifacts**:
- `src/engine/asset/model_loader.cpp` — magenta fallback applied, read_attribute resize fix, uint32 index append fix, debug log bounds fix
- `src/engine/asset/asset_manager.cpp` — fixed use-after-move in load_model debug log
- `src/engine/asset/model_asset.h` — `replace_root()` moved to private section
- `src/engine/render/vertex_buffer_headless.h` — added `data()` accessor for test pixel inspection
- `src/engine/render/index_buffer_headless.h` — added `data()` accessor for test pixel inspection
- `tests/model_asset_tests.cpp` — added all 11 missing test cases
- `tests/assets/models/missing-position/*` — test glTF without POSITION attribute
- `tests/assets/models/corrupt/*` — corrupt glTF test file
- `tests/assets/models/missing-texture/*` — glTF with bogus texture URI
- `tests/assets/models/scale2x/*` — YAML with scale 2.0
- `tests/assets/models/transform-only/*` — transform-only node test
- `tests/assets/models/unsupported-mode/*` — POINTS mode test
- `tests/assets/models/color-vec3/*` — COLOR_0 VEC3 test
- `tests/assets/models/missing-normal/*` — missing NORMAL test
- `tests/assets/models/uint32-indices/*` — Uint32 indices test
**Questions for human**:
none
**Warnings**:
- Same as before: tinygltf stb conflict, v2.9.7 instead of v2.10.0, add_model_to_world takes non-const reference.
- The `replace_root()` was moved from public to private (with friend AssetManager) — this is a security fix matching the spec's intent.
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**: Final review. All 348 tests pass (0 failures). All 28 ACs now pass. BL-001 (magenta fallback) fixed — `ensure_texture` called after each texture slot. BL-002 resolved — all 21 DC-12 required tests present (11 added). Additional fixes verified: (1) PBR ambient 0.03→0.15 in shader, (2) DirectionalLightComponent in both demo apps, (3) camera look_at+orbit in hot_reload_gltf_app, (4) double make_full_path bug fixed. Visual verification: gltf_demo_app renders Box model with texture binding; hot_reload_gltf_app hot-reloads successfully. Feature accepted.
**Artifacts**:
- `docs/specs/gltf-model-loading/code-review.md`
**Questions for human**:
none
**Warnings**:
- W-001: `add_model_to_world()` takes non-const `ModelNode&` (differs from spec's `const`) because Model is move-only. This is intentional but the spec should be updated.
- W-002: Embedded glTF textures decoded twice — performance issue, not blocking.
- W-003: tinygltf v2.9.7 vs spec's v2.10.0 (v2.10.0 tag does not exist upstream).
- W-004: 6 edge-case tests from the contract's full test table remain unimplemented (P2/P3, not in DC-12), including add_model_to_world hierarchy check (P1).
- W-005: PBR tone-mapping makes texture detail dim — known V1 quality limitation.
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
Created ADR-018 documenting the tinygltf v2.9.7 dependency decision for glTF 2.0 model loading. Covers: rationale (header-only, error-code-based, FetchContent integration, full glTF 2.0 spec coverage), alternatives considered (assimp, cgltf, manual parsing, vendored header), custom stb_image callback to avoid ODR conflicts, PRIVATE linkage to preserve abstraction boundaries, and exception-safety analysis (tinygltf uses error codes — no exception wrappers needed, unlike yaml-cpp).
**Artifacts**:
- `docs/adr/018-tinygltf-dependency.md`
**Decisions needed**:
none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## constitution-agent

**Status**: completed
**Summary**:
Reviewed the glTF model loading feature (spec, implementation contract, code-reviewer output) against all existing constitution rules (CONST-001–CONST-004) and the charter. Verified architecture boundaries are respected: tinygltf is PRIVATE to `src/engine/`, no GL/types leak outside render layer, no tinygltf types in public headers. The feature follows established FetchContent dependency patterns (per ADR-016, ADR-007). All new code has corresponding tests (348 tests passing, per code-reviewer). No new constitution rules or amendments are needed.
**Artifacts**:
none
**Changes needed**:
none
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated the operational wiki to document the new glTF model loading feature. Added entries for ModelAsset, ModelNode, PbrMaterial, PbrMaterialData, ModelLoader, and model_utils.h in both the module map and glossary. Added PBR submodule section alongside existing Phong submodule. Added gltf_demo_app and hot_reload_gltf_app to the app subclasses and scene list. Added model_asset_tests.cpp to the test file reference.
**Artifacts**:
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/domain/glossary.md`
**Changes made**:
- **module-map.md**: Added ModelAsset, ModelLoader entries to asset/ table; added ModelNode, model_utils.h entries to render/ table; added PBR submodule section with pbr_shaders.h, pbr_material.h, pbr_material.cpp; added gltf_demo_app and hot_reload_gltf_app to app subclasses; added gltf-demo and hot-reload-gltf to available scenes list; added model_asset_tests.cpp to tests table; added glTF spec and ADR-018 references.
- **glossary.md**: Added ModelNode term to Model utility section; added PbrMaterial and PbrMaterialData to Render pipeline terms; added ModelAsset and ModelLoader to Asset system terms; updated Asset to list ModelAsset as a concrete subclass; updated YAML asset metadata to mention Model schema; added glTF spec and ADR-018 references.
**Questions for human**:
none
**Warnings**:
- The spec reference in both files still uses placeholder "SPEC-NNNN" because the actual spec number is not yet assigned. This should be updated when the spec is finalised.
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance review: all 3 blocking issues resolved ✅. BL-001 (asset-manager spec non-goals) updated to reflect glTF support. BL-002 (wiki module-map and glossary) updated by wiki-agent with all new types. BL-003 (spec tinygltf version mismatch) fixed to v2.9.7 matching ADR-018 and code. Also fixed add_model_to_world() signature in spec. No constitution violations. ADR-018 (tinygltf) exists and is correct. Feature is fully governance-compliant.

**Artifacts**:
- `docs/specs/gltf-model-loading/governance-review.md`
**Questions for human**:
none
**Warnings**:
- W-001: Spec Appendix C uses tinygltf v2.10.0 but actual upstream tag is v2.9.7 (ADR-018 and code use v2.9.7). Spec must be updated to match.
- W-002: `add_model_to_world()` signature in spec shows `const ModelNode&` but implementation uses non-const (Model is move-only). Spec should be updated.
- W-003: 6 edge-case tests from the contract's full table not implemented (P2/P3, none in DC-12, non-blocking).
- W-004: Embedded glTF textures decoded twice — performance issue only.
- W-005: Unsupported primitive mode: spec line 452 says "return error" but line 588 says "skipped with warning" — the latter is correct per code.
- W-006: Khronos test model licenses/attribution not documented in spec.
**Blocking issues**:
- [ ] BL-001: Asset-manager spec (`docs/specs/asset-manager/spec.md`) non-goals (line 62) and out-of-scope (line 1113) still state "No glTF/glb model loading (models remain programmatic for V1)" — this contradicts the new glTF feature and was listed as a required update in the glTF spec's "Documents requiring updates" section.
- [ ] BL-002: Wiki module-map and glossary not yet updated. `docs/wiki/architecture/module-map.md` missing entries for ModelAsset, ModelNode, PbrMaterial, ModelLoader, pbr_shaders.h, model_utils.h. `docs/wiki/domain/glossary.md` missing terms for ModelAsset, ModelNode, PbrMaterial, PbrMaterialData. Wiki-agent status is pending.
- [ ] BL-003: Spec Appendix C (line 1032) pins tinygltf to `v2.10.0` which does not exist upstream — actual is `v2.9.7`. The spec must be updated to match ADR-018 and the actual implementation.

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
