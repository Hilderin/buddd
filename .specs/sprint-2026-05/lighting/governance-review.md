# Governance Review — Phong Lighting System (SPEC-018)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec ↔ Contract: LightData struct fields** — The spec's AC-015 field listing omits `spot_direction`, though the spec's own `LightData` struct definition (spec lines 266–273) correctly includes it. The implementation contract added `spot_direction` and renamed `inner_cone`/`outer_cone` to `inner_cone_cos`/`outer_cone_cos`. This is a **spec gap fix**, not a contradiction — the contract is more precise. The code matches the contract. Non-blocking.
- [x] **Spec ↔ Contract: `u_material_specular` type** — Spec declares it as `vec3`, the contract and implementation use `vec4` with `.rgb` extraction. Functionally identical rendering. Non-blocking.
- [x] **Contract ↔ Code: Light component constructors** — The implementation removed `explicit` from parameterised constructors (all params have defaults, making `explicit` ambiguous). Semantically equivalent. Non-blocking.
- [x] **Contract ↔ Code: OpenGL backend uniform extraction** — Contract specified `render_device_opengl.cpp` should parse shader source via `glsl_util`, but `ShaderOpenGL` has no `source()` method (forbidden to modify). The `#include` is present but unused; `MaterialOpenGL` uses `glGetUniformLocation` at call time. Acceptable per contract's own caveat.
- [x] **Code ↔ Tests: All 37 ACs covered** — 33 test cases map directly to ACs; remaining 4 verified by code review or covered indirectly. 299 assertions, all pass.
- [x] **Spec ↔ Wiki: Wiki accurately reflects spec** — 5 wiki pages updated with Phong lighting details, module maps, data flow, business rules, and testing tables. No contradictions found.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)** — No SDL3, OpenGL, or GLM types exposed in public headers. All new `scene/` headers include only `math/*.h` and `scene/component.h`. `phong_demo.h` forward-declares `RenderDevice`. Backend types stay in `src/engine/render/`. ✅ No violation.
- [x] **CONST-002 (Testing Policy)** — All testable code has corresponding unit tests. 33 lighting tests (299 assertions) and 273 existing tests (12,780 assertions) all pass. ✅ No violation.
- [x] **CONST-003 (Documentation Policy)** — Rule is still `TODO` (unrelated to lighting system). Not violated.
- [x] **CONST-004 (Security Policy)** — Rule is still `TODO` (unrelated to lighting system). Not violated.
- [x] **Engineering Principles** — Explicit contracts preferred (SPEC-018 and IMPL-018-002 are detailed). Small scoped changes (no modifications to existing interfaces). Existing conventions followed (accessor patterns, namespace conventions, PIMPL). Testable requirements (all ACs testable in headless mode). Governance documents do not contradict each other. ✅ All principles respected.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-013 (Standard Vertex Format)** — Created by `adr-agent`, documents the 72B `Vertex` struct decision with rationale, alternatives, and compliance rules. Consistent with existing ADRs (ADR-003, ADR-010, ADR-002).
- [x] **ADR-003 (Render Pipeline Architecture)** — `Model::draw()` returns `void`. ✅ Respected.
- [x] **ADR-005 (Optional Ref Component API)** — Light component accessor pairs match the CameraComponent pattern. ✅ Respected.
- [x] **ADR-010 (No Raw Pointers in Public API)** — `PhongMaterial` uses `unique_ptr<Impl>`, references (`RenderDevice&`), `shared_ptr<Texture>`. ✅ Respected.
- [x] **ADR-009 (Test File Naming Convention)** — `tests/lighting_tests.cpp` follows `*_tests.cpp` pattern. ✅ Respected.

No new ADRs required beyond ADR-013. The remaining 5 design decisions (PhongMaterial self-contained construction, three separate light components, glsl_util refactoring, flattened uniform arrays, has_uniform sentinel) are implementation-level patterns that follow established conventions.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`architecture/overview.md`** — Updated with light components in scene/ directory, vertex.h/glsl_util/light_data.h/phong/ in render/, phong demo in key behaviors, light component lifecycle notes. ✅
- [x] **`architecture/module-map.md`** — Updated with directional/point/spot light components, vertex.h, glsl_util, light_data.h, phong/ submodule table, updated material_headless/render_system/demo_helpers entries, phong_demo entry. ✅
- [x] **`architecture/data-flow.md`** — Added Phong rendering lifecycle section with full render() flow, normal matrix computation, light type encoding, shader fragment flow, array uniform naming convention. ✅
- [x] **`domain/business-rules.md`** — Added comprehensive Phong Lighting Rules section covering standard vertex, light components, light count limit, PhongMaterial rules, shader rules, light type encoding, accessor patterns, glsl_util rules, MaterialHeadless diagnostic accessors. ✅
- [x] **`engineering/testing.md`** — Added Phong lighting tests section with AC-to-test mapping table (32 tests), SPEC-018/IMPL-018-002 references. ✅

The wiki captures current operational understanding without becoming constitutional law. No constitution updates are needed.

## Warnings

Non-blocking concerns for awareness:

- **SPEC-017 still Draft** — SPEC-018 depends on SPEC-017's texture system (`Material::set_texture()`, `Texture`, `Image::load`). If SPEC-017 changes, SPEC-018 must be reviewed for compatibility.
- **`-Wunused-result` warnings in `render_system.cpp`** — `set_uniform()` return values after the initial `u_mvp` check are intentionally ignored (per spec pattern), but trigger `-Wunused-result` warnings. Consider `(void)` casts for polish.
- **Unlit shader `a_color` type mismatch** — `Vertex::color` is `Vec4` (16B) but unlit shaders declare `vec3 a_color` (12B). Works in practice because OpenGL reads by attribute location, but technically diverges.
- **`u_material_specular` type discrepancy between spec (`vec3`) and contract/code (`vec4`)** — Functionally identical via `.rgb` extraction in the fragment shader. Consider a spec amendment for alignment.
- **AC-015 spec field listing omits `spot_direction`** — The spec's `LightData` struct definition (lines 266-273) correctly includes `spot_direction`, but AC-015's field list omits it. Consider updating AC-015 for documentation completeness.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- None. All governance documents are consistent. Wiki has been updated. ADR-013 has been created. No constitution rules were violated or need amendment.
