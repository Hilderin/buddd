# Spec Review — SPEC-018: Phong Lighting System

## Blocking issues

Items that must be resolved before the artifact can be accepted.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

- [x] **B-04** — `collect_light` lambda never populates `ld.spot_direction` for spot lights: the direction is received as a parameter but only used for directional lights (`type_w == 0.0f`). The `spot_direction` field in `LightData` remains zero-initialized, causing undefined behaviour in the shader. **RESOLVED** ✓ — lines 599-602 now store `ld.spot_direction = {direction.x, direction.y, direction.z, 0.0f}` when `type_w == 2.0f`.

## Warnings

Non-blocking concerns for awareness:

- SPEC-017 is still `Draft` — SPEC-018 depends on its texture system. If SPEC-017 changes, SPEC-018 must be updated.
- Demo texture `assets/brick.png` may not exist — procedural checkerboard fallback recommended (A-20).
- AC-015 description still omits `spot_direction` from its field list, even though the `LightData` struct definition (line 266-273) correctly includes it. Minor documentation gap — consider updating AC-015 for completeness.
- W-03 tension: "no modification to existing demos" vs "Standard Vertex used by ALL meshes" persists (unchanged from prior review).

## Required changes

None. All blocking issues resolved.

## Suggested improvements

- Update AC-015 description to include `spot_direction` in the field list.

---

## B-04 verification

B-04 was the only blocking issue from the previous review. Verdict:

- **`collect_light` lambda**: lines 596-602 now correctly handle `type_w == 2.0f` by storing direction in `ld.spot_direction`. ✅ **Resolved**
- **Fragment shader**: uses `u_light_spot_directions[i]` for cone falloff (line 470). ✅ **Consistent**
- **RenderSystem**: sets `u_light_spot_directions[i]` from `ld.spot_direction` (line 678). ✅ **Consistent**
- **LightData struct**: includes `spot_direction` field (line 270). ✅ **Consistent**

## Final verdict

**Accept** — B-04 is resolved, no new blocking issues introduced. The spec is internally consistent and ready for implementation.
