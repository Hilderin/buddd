# Implementation Contract Review — Phong Lighting System (Rearchitected)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

None.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **LightData struct diverges from spec AC-015**: The spec's `LightData` (AC-015) defines 5 fields: `position_or_dir`, `colour`, `range`, `inner_cone`, `outer_cone`. The contract adds a `spot_direction` (`Vec4`) field and renames `inner_cone`/`outer_cone` to `inner_cone_cos`/`outer_cone_cos`. This is a necessary fix — the spec's LightData was incomplete (the shader declares `u_light_spot_directions[MAX_LIGHTS]` but the spec's struct had no storage for it). The contract correctly addresses this. However, it technically contradicts the accepted spec's AC-015 field listing. Consider a spec amendment to AC-015 to add `spot_direction` and update field names.

- **`u_material_specular` type differs from spec**: The spec declares `u_material_specular` as `vec3`, the contract uses `vec4`. The contract's RenderSystem sets it as `Vec4{1,1,1,1}` vs the spec's `Vec3{1,1,1}`. The shader extracts `.rgb` so rendering is identical. This is a minor deviation. Consider aligning with either the spec (`vec3`) or the contract (`vec4`) for consistency.

- **AC-033 (`set_transforms` deep test) not explicitly mapped in AC-to-test table**: The spec's AC-033 requires: "call setter, verify all three uniforms match expected values." The contract's AC-008 only verifies the header declares `set_transforms`, not that it correctly sets `u_model`/`u_mvp`/`u_normal_mat`. The test can be added under AC-008 or as a separate AC-033 entry in the test table. This is a minor gap — a diligent implementer will naturally add the test, but the contract should explicitly call for it.

- **spot_direction uniforms set even for directional/point lights**: The contract's RenderSystem sets `u_light_spot_directions[i]` for ALL light types (directional and point lights get a zeroed `spot_direction`). This wastes uniform slots but is harmless. If performance is a concern, consider only setting spot directions when there are active spot lights.

## Required changes

None — no blocking issues. Warnings above are for awareness only.

## Suggested improvements

- **LightData field ordering**: Current order (`position_or_dir`, `colour`, `range`, `spot_direction`, `inner_cone_cos`, `outer_cone_cos`) works with alignof(Vec4)=4. But if `GLM_FORCE_ALIGNED` is ever enabled, 12 bytes of padding will appear between `range` and `spot_direction`, making `sizeof(LightData) = 80` instead of 60, and the static_assert will fail. Consider reordering to: `position_or_dir`, `colour`, `spot_direction`, `range`, `inner_cone_cos`, `outer_cone_cos` (all Vec4s first, then floats) to avoid alignment issues across all ABI configurations.

- **Consider adding a `static_assert(alignof(LightData) == 4, ...)`** to document the alignment assumption.

- **The test table references TOL = 1e-5f for float comparisons** — this is fine for headless tests but may be too tight for GPU-backed tests in the future.

## Review summary

**Verdict**: **Accept**

The contract is precise, complete, and implementable. All 37 spec ACs are covered either explicitly in the AC-to-test table or via Done Criteria. The contract correctly fixes two spec deficiencies: (1) adding `spot_direction` to `LightData` for the separate `u_light_spot_directions` uniform array, and (2) using `vec4` for `u_material_specular` (with `.rgb` extraction in the shader). No blocking issues remain.

| Check | Result |
|-------|--------|
| Standard Vertex (72B, 6 attributes) | ✅ Correct layout, offsets, `static_assert` |
| PhongMaterial (self-contained, RenderDevice&) | ✅ Correct signature, convenience setters |
| Three light components (Dir/Point/Spot) | ✅ Correct accessor patterns |
| glsl_util (4 GLSL patterns) | ✅ Correct parsing order, bugfix for old code |
| LightData (6 fields, k_max_lights=8) | ⚠️ Adds spot_direction (fixes spec gap) |
| RenderSystem extended | ✅ Light collection + uniform setting |
| MaterialHeadless diagnostic accessors | ✅ vec3, vec4, float, int |
| Demo (Vertex struct + free-camera) | ✅ Interactive orbitting point light |
| Backward compatibility | ✅ `has_uniform("u_model")` gate |
| CONST-001 compliance | ✅ No backend types in public headers |
| ADR-010 compliance | ✅ No raw pointers, references only |
| All 37 spec ACs | ✅ Covered (explicitly or via DC) |
