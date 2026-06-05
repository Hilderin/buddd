# Workflow Coordination: lighting

## Orchestrator

**Feature**: lighting
**Status**: completed
**Current step**: completed
**Initial instructions**: Implémenter un système de lumière Phong — LightComponent (directionnelle, ponctuelle), shaders Phong, normales dans le format de vertex, mise à jour du RenderSystem pour collecter les lumières et passer les uniforms associés.
**Notes**:
- Human a choisi "Système de lumière (Phong)" comme prochaine étape après avoir eu les options : lumière, quick-win cube-scene, audio, asset pipeline, skybox.
- Scout a fourni une analyse détaillée du pipeline de rendu existant :
  - RenderSystem actuel : itère les MeshRenderer, calcule MVP, set `u_mvp`, draw
  - Shaders actuels : vertex avec position(loc0) + color(loc1), fragment sort color direct, pas de lumière
  - Material supporte déjà set_uniform(Vec3) et set_uniform(Mat4)
  - Pas de code lié à la lumière nulle part
  - Spécifications existantes (SPEC-005, SPEC-011, SPEC-017) listent le lighting comme hors-scope
- Réponses aux questions du spec-author (2026-05-31) :
  - Q-01: Camera::position() existe ✅ (vérifié dans camera.h)
  - Q-02: MaterialHeadless a get_uniform_mat4() mais pas get_uniform_vec3/Vec4 — à ajouter
  - Q-03: Démo interactive (free-camera) ✅
  - Q-04: Utiliser les textures pour la couleur diffuse, pas la couleur per-vertex. Un tint uniforme global sur le Material est une option intéressante.
- Loop 1 (spec-critic → spec-author, 2026-05-31) : Résolution des blocking issues B-01 à B-05
  - Décision B-01/B-05: Utiliser des flattened arrays (`u_light_positions_or_dir[MAX_LIGHTS]`, `u_light_colours[MAX_LIGHTS]`, `u_light_ranges[MAX_LIGHTS]`) — plus simple, évite la dépendance au support struct-array dans set_uniform
  - Décision B-02: Ambient global hors de la boucle for (standard Phong, compatible avec 0 lights)
  - B-03: Signatures const à corriger
  - B-04: Renumbering des ACs séquentiellement
  - W-01 à W-08: À adresser (assumptions, LightData, normalize, etc.)
- Loop 2 (impl-contract-critic → impl-contract-author, 2026-05-31) : Résolution du blocking issue B-01
  - B-01: extract_uniform_names() ne parse pas les uniforms GLSL avec valeurs par défaut (`uniform type name = default_value;`)
- Design discussion (2026-05-31) : Réarchitecture complète suite au feedback humain
  - Vertex standard unique: `Vertex` struct (72B) dans `src/engine/render/vertex.h` — position+color+normal+texcoord+tangent+texcoord2, utilisé par tous les maillages
  - Renommage: `lit` → `phong` partout
  - Architecture: `PhongMaterial` (Material subclass) dans `src/engine/render/phong/` — shaders + material encapsulés
  - Composants lumière séparés: DirectionalLightComponent, PointLightComponent, SpotLightComponent
  - Refactoring: `extract_uniform_names()` → `src/engine/render/glsl_util.h/.cpp` mutualisé
  - Demo renommée: `buddd demo phong`

## spec-author

**Status**: completed
**Summary**:
Complete rearchitecture (2026-05-31) based on design review feedback. Final fixes (2026-05-31):
- B-01: Added `u_light_spot_directions[MAX_LIGHTS]` to fragment shader + cone falloff via `spot_cone_attenuation()`
- B-02: Added `spot_direction` to `LightData`, `u_light_spot_directions[i]` to RenderSystem pseudocode, and `u_light_spot_directions` to AC-009
- B-03: Restored SPEC-011's `u_mvp` error-handling pattern (check Result, log warning, skip entity)
- B-04: Added `spot_direction` storage in `collect_light` lambda for spot lights
**Artifacts**:
- `.specs/sprint-2026-05/lighting/spec.md`
**Questions for human**:
none
**Warnings**:
- SPEC-017 is still `Draft` — SPEC-018 depends on its texture system. If SPEC-017 changes, SPEC-018 must be updated.
- Demo texture `assets/brick.png` may not exist — procedural checkerboard fallback recommended (added assumption A-20).
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**: Final re-review of SPEC-018 (2026-05-31). B-04 verified as resolved ✓ — `collect_light` lambda now stores `direction` in `ld.spot_direction` when `type_w == 2.0f`. No new blocking issues found. Verdict: **Accept**.
**Artifacts**:
- `.specs/sprint-2026-05/lighting/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- SPEC-017 is still `Draft` — SPEC-018 depends on its texture system. If SPEC-017 changes, SPEC-018 must be updated.
- Demo texture `assets/brick.png` may not exist — procedural checkerboard fallback recommended (A-20).
- AC-015 description still omits `spot_direction` from its field list — minor documentation gap.
- W-03 tension: "no modification to existing demos" vs "Standard Vertex used by ALL meshes" persists.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Complete rewrite of the implementation contract to match the rearchitected SPEC-018. Covers: (1) Standard Vertex struct (72B, 6 attributes) in `vertex.h`; (2) glsl_util shared utility module for uniform name extraction/normalization; (3) Three separate light components (Directional, Point, Spot) instead of a unified one; (4) Phong module (`phong/`) with PhongMaterial + embedded GLSL shaders; (5) LightData struct with inner/outer cone cosines + spot_direction; (6) RenderSystem extended to collect all 3 light types (max 8); (7) MaterialHeadless diagnostic accessors + array subscript normalization; (8) Demo helpers updated to use Vertex struct; (9) `buddd demo phong` interactive demo with orbiting point light + directional fill. All existing demos remain unchanged.
**Artifacts**:
- `.specs/sprint-2026-05/lighting/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- The spec's fragment shader (lines 408-409) uses `u_light_inner_cones[MAX_LIGHTS]` and `u_light_outer_cones[MAX_LIGHTS]` as separate float arrays, while the user's "Key architecture changes" describes `u_light_spot_cones[8]` as Vec2. This contract follows the user's Vec2 approach as it is more recent and GPU-efficient. If the spec version should be used instead, update the shader uniforms and LightData accordingly.
- Spot direction handling: the spec acknowledges spot direction must be passed separately but the fragment shader in the spec doesn't include `u_light_spot_directions`. This contract adds it explicitly (per Q-05 resolution and user's "Key architecture changes").
- `u_material_ambient` is specified as `float` in this contract (user instruction) but the spec uses `vec3`. The float approach is simpler; if Vec3 is preferred, update the shader and RenderSystem code.
- SPEC-017 is still `Draft` — this contract depends on its texture system (`Image::load`, `device.create_texture`, `material.set_texture`). The phong demo includes a procedural checkerboard fallback.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**: Final re-review (2026-05-31) of the completely rewritten implementation contract matching SPEC-018 rearchitecture. The contract is precise, implementable, and covers all 37 spec ACs (explicitly or via Done Criteria). Key findings: (1) LightData adds `spot_direction` field not in spec's AC-015 — fixes a spec deficiency (shader needs it for `u_light_spot_directions`). (2) `u_material_specular` uses `vec4` vs spec's `vec3` — behavior identical via `.rgb` extraction. (3) AC-033 (`set_transforms` deep test) not explicitly mapped in AC table, though implied by AC-008/009. None are blocking. Verdict: **Accept**.
**Artifacts**:
- `.specs/sprint-2026-05/lighting/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- LightData struct diverges from spec AC-015 (adds `spot_direction`, renames fields) — necessary fix but contradicts published spec.
- `u_material_specular` is `vec4` in contract vs `vec3` in spec — functionally equivalent, minor type difference.
- AC-033 (`set_transforms` deep test) not explicitly mapped in AC-to-test table — consider adding.
- SPEC-017 is still Draft — lighting demo depends on its texture system.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-05-31 14:26 EDT
**Notes**: Architecture validée : Vertex standard (72B), PhongMaterial dans src/engine/render/phong/, trois composants lumière (Directional/Point/Spot), glsl_util partagé, demo interactive phong. Approuvé pour implémentation.

## code-implementer

**Status**: completed
**Summary**:
Implemented the full Phong lighting system per IMPL-018-002: standard Vertex struct (72B) in vertex.h; shared glsl_util module with extract_uniform_names and normalize_uniform_name; three light components (Directional/Point/Spot) deriving Component; LightData struct with k_max_lights=8; phong/ module with PhongMaterial (Material subclass) and embedded GLSL 450 shaders; MaterialHeadless diagnostic accessors and array subscript normalization; RenderSystem extended with light collection and lighting uniform setting; demo_helpers updated to use Vertex; new `buddd demo phong` interactive demo; full test suite in tests/lighting_tests.cpp (32 test cases, all passing).
**Artifacts**:
- Created: vertex.h, glsl_util.h/.cpp, light_data.h, directional_light_component.h/.cpp, point_light_component.h/.cpp, spot_light_component.h/.cpp, phong_shaders.h, phong_material.h/.cpp, phong_demo.h/.cpp, lighting_tests.cpp
- Modified: material_headless.h/.cpp, render_device_headless.cpp, render_device_opengl.cpp, render_system.cpp, demo_helpers.cpp, demo_command.cpp
**Questions for human**:
none
**Warnings**:
- `ShaderOpenGL` has no `source()` method (forbidden to modify), so OpenGL backend skips shader-source-based uniform extraction. The `known_uniforms` parameter is accepted but not stored. This is fine because `MaterialOpenGL` uses `glGetUniformLocation` at call time.
- Light components removed the `explicit` keyword from parameterized constructors and removed `= default` default constructors to resolve ambiguous constructor calls (all params have defaults).
- `PhongMaterial` uses PIMPL and delegates all uniform calls to an inner `Material`. Added `inner_material()` accessor for test diagnostics.
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Code review complete. All 33 lighting tests pass (299 assertions), all 273 existing tests pass (12780 assertions, no regressions). Build succeeds with zero errors. Architecture boundaries respected (CONST-001), draw returns void (ADR-003), no raw pointers in public API (ADR-010). The implementation matches SPEC-018 and IMPL-018-002 closely with minor non-blocking divergences (u_material_specular vec4 vs vec3, LightData field naming). Verdict: Accept.
**Artifacts**:
- `.specs/sprint-2026-05/lighting/code-review.md`
**Questions for human**:
none
**Warnings**:
- `-Wunused-result` warnings in `render_system.cpp` for intentionally ignored `set_uniform()` return values — pattern matches spec but generates noisy compiler output.
- Unlit demo shaders read `a_color` as `vec3` while `Vertex` stores it as `Vec4` — functional but a type mismatch. Documented in spec §1.
- Phong demo is interactive-only and cannot be visually captured in headless mode; manual visual verification required.
**Blocking issues**:
none
**Bug fix (post-review, 2026-05-31)**: Crash fix in `render_device_opengl.cpp` — `draw()` and `draw_indexed()` used `static_cast<const MaterialOpenGL&>(material).bind()` which crashes when material is a `PhongMaterial` (not a `MaterialOpenGL`). Fixed by using virtual dispatch: `material.bind()`. All 273 tests pass. Demo runs >300 frames without crash.

**Re-verification (2026-05-31 2nd pass)**: Confirmed the fix is correct: `Material::bind()` is pure virtual, overridden by `MaterialOpenGL`, `PhongMaterial`, and `MaterialHeadless`; the old `static_cast` was invalid UB for non-`MaterialOpenGL` types. All 273 tests pass (12780 assertions, 100%). No regressions. Fix accepted.

## adr-agent

**Status**: completed
**Summary**:
Assessed 6 key architectural decisions from the Phong Lighting System implementation. Determined that the Standard Vertex Format (single 72B Vertex struct for all meshes) is a cross-cutting architecture decision warranting an ADR. Created ADR-013 documenting the rationale, alternatives, fixed attribute locations, and consequences. The other 5 decisions (PhongMaterial self-contained construction, three separate light components, glsl_util refactoring, flattened uniform arrays, has_uniform sentinel) are implementation-level patterns that follow established conventions and do not warrant standalone ADRs.
**Artifacts**:
- `docs/adr/013-standard-vertex-format.md`
**Decisions needed**:
none — ADR-013 has been written documenting the Standard Vertex Format decision.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## constitution-agent

**Status**: completed
**Summary**:
Reviewed all 4 existing constitution rules (CONST-001 through CONST-004) against the Phong Lighting System implementation. CONST-001 architecture boundary is respected (no SDL3/OpenGL/GLM types in scene/ headers). CONST-002 testing policy is satisfied (32 lighting tests pass). No violations found. No new constitutional rules needed — the standard Vertex format, light component design, and Phong module encapsulation are spec-level decisions documented in SPEC-018, adequately enforced by existing specs and code review.
**Artifacts**:
- none
**Changes needed**:
none — existing constitution rules sufficiently cover the lighting system. The standard Vertex format is a spec-level invariant (SPEC-018), not a cross-cutting constitutional matter; it can legitimately evolve (e.g., for PBR) without constitutional amendment. Light component architecture and Phong module patterns are design choices, not project-wide invariants.
**Questions for human**:
none
**Warnings**:
- CONST-003 (Documentation Policy) and CONST-004 (Security Policy) remain TODOs — unrelated to lighting but may need attention in the future.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated 5 wiki pages to reflect the newly implemented Phong Lighting System (SPEC-018). Added Phong module (phong/), three light components (Directional/Point/Spot), standard Vertex struct, glsl_util, LightData, and Phong demo to architecture overview, module map, data flow, business rules, and testing docs.
**Artifacts**:
- `docs/wiki/architecture/overview.md`
- `docs/wiki/architecture/module-map.md`
- `docs/wiki/architecture/data-flow.md`
- `docs/wiki/domain/business-rules.md`
- `docs/wiki/engineering/testing.md`
**Changes made**:
- `architecture/overview.md`: Added light components to scene/ directory listing; added vertex.h, glsl_util, light_data.h, phong/ subdirectory to render/ listing; added phong demo to key behaviors; added light component lifecycle note to scene graph behaviors.
- `architecture/module-map.md`: Added directional/point/spot light components to scene/ table; added vertex.h, glsl_util.h/.cpp, light_data.h to render/ table; added phong/ submodule table with phong_shaders.h and phong_material; updated material_headless/render_system/demo_helpers entries; added phong_demo to demo files; added lighting_tests.cpp to test files; updated demo list in subcommand behavior.
- `architecture/data-flow.md`: Added Phong rendering lifecycle section with full render() flow, normal matrix computation, light type encoding, shader fragment flow, array uniform naming convention.
- `domain/business-rules.md`: Added comprehensive Phong Lighting Rules section covering standard vertex, light components, light count limit, PhongMaterial rules, shader rules, light type encoding, accessor patterns, glsl_util rules, MaterialHeadless diagnostic accessors.
- `engineering/testing.md`: Added Phong lighting tests section with AC-to-test mapping table (32 tests); added SPEC-018/IMPL-018-002 references.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Final cross-document governance validation of the Phong Lighting System (SPEC-018). All documents are coherent: spec ↔ contract ↔ code ↔ tests. Constitution rules (CONST-001, CONST-002) are respected. ADR-013 (Standard Vertex Format) has been created and aligns with existing ADRs. Wiki has been updated (5 pages). No blocking issues found. Verdict: **Accept**.
**Artifacts**:
- `.specs/sprint-2026-05/lighting/governance-review.md`
**Questions for human**:
none
**Warnings**:
- SPEC-017 is still Draft — SPEC-018 depends on its texture system.
- `-Wunused-result` warnings in `render_system.cpp` for intentionally ignored `set_uniform()` return values.
- Unlit shader `a_color` type mismatch (vec3 vs Vec4) — documented, works in practice.
- `u_material_specular` type discrepancy between spec (vec3) and code (vec4).
- AC-015 field listing omits `spot_direction` though the spec's LightData struct includes it.
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
