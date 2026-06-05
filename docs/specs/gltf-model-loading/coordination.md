# Workflow Coordination: gltf-model-loading

## Orchestrator

**Feature**: `gltf-model-loading`
**Status**: in-progress
**Current step**: human-approved
**Initial instructions**: Ajouter la possibilité de loader des modèles glTF depuis l'AssetManager, avec support du hot-reload. Télécharger des modèles de KhronosGroup (Box + DamagedHelmet) dans assets/models/[folder du model].

**Notes**:

**Loop 1 (spec-author)**: spec-critic flagged BL-001 (missing doc update list) and BL-002 (texture loading contradiction). Human resolved BL-002: use raw `shared_ptr<Texture>` owned by PbrMaterial, not `TextureAsset`. Spec-author fixing both.

**Loop 2 (implementation-contract)**: implementation-contract-critic flagged BL-001 (inconsistent error category for corrupt glTF). Spec and human resolved: add new `Error::Category::InvalidFormat` for corrupt files. Implementation-contract-author fixing contract.

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

**Status**: pending
**Summary**:
pending
**Artifacts**:
pending
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: pending
**Summary**:
pending
**Artifacts**:
- `docs/specs/gltf-model-loading/code-review.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## adr-agent

**Status**: pending
**Summary**:
pending
**Artifacts**:
pending
**Decisions needed**:
pending
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## constitution-agent

**Status**: pending
**Summary**:
pending
**Artifacts**:
pending
**Changes needed**:
pending
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: pending
**Summary**:
pending
**Artifacts**:
pending
**Changes made**:
pending
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: pending
**Summary**:
pending
**Artifacts**:
- `docs/specs/gltf-model-loading/governance-review.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
