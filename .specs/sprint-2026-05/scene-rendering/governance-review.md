# Governance Review — Scene-Based Rendering (SPEC-011 / IMPL-011)

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec API vs Contract API vs Code API**: All public API signatures (`Component::entity()`, `Component::on_attach()`, `World::each<T>()`, `World::register_camera/unregister_camera/active_camera`, `CameraComponent`, `MeshRenderer`, `RenderSystem`) are consistent across spec, contract, and source code. Checks: AC-001–AC-030 all pass. No contradictions.

- [x] **Spec says `static_assert`, code uses `requires` clause**: The spec and contract specify `static_assert(std::is_base_of_v<Component, T>)` inside `World::each<T>()`. The implementation uses `requires std::is_base_of_v<Component, T>` as a template constraint. Code-review D-01 documents this deviation and correctly assesses it as **necessary and acceptable** — the `requires` clause is SFINAE-friendly, enabling the compile-time SFINAE check in tests.

- [x] **Spec says `Quat::from_axis_angle`, actual API is `Quat::angle_axis`**: Spec pseudo-code used a non-existent API name. The code-review correctly identified and documented this (D-02). The implementation uses the actual API `Quat::angle_axis(angle, axis)`. Acceptable correction.

- [x] **Spec pseudo-code incorrect for `MeshRenderer` constructor**: Spec line 486 shows `entity.add_component<be::MeshRenderer>(std::move(cube.model))` where `cube.model` is of type `Model`, but `MeshRenderer` constructor takes `std::shared_ptr<Model>`. The code-review documented this as D-03. The implementation correctly wraps with `std::make_shared<be::Model>(std::move(cube.model))`. Acceptable correction.

- [x] **`Component` member access**: Spec pseudo-code places `world_` and `entity_id_` as `private`. Contract specifies `protected` with explicit rationale (CameraComponent destructor needs access). Code implements as `protected`. Contract-correct. Acceptable spec oversight corrected in contract.

- [ ] **SPEC-011 spec vs IMPL-011 contract: `demo_command.cpp` registration contradiction**: SPEC-011 spec (line 454) explicitly states "The demo is registered in `src/cmd/commands/demo_command.cpp` as a new subcommand: `"cube-scene"`". IMPL-011 contract section "Files forbidden to change" (line 130) states `demo_command.cpp` is not listed in "Files allowed to change" and must not be modified. The code-review (W-01) correctly identified this contradiction. **Resolution needed**: Either the spec must be relaxed (the `"cube-scene"` subcommand is optional) or the contract must be amended to allow the one-line change to `demo_command.cpp`. Currently the `cube_scene_demo` exists and compiles but is unreachable from the CLI — SC-003 cannot be verified via CLI.

- [x] **Contract contains 7 stale `std::reference_wrapper` patterns** (contract-critic W-01 through W-07): The contract's documentation sections, test instructions, and checklist still contain references to `.get()`, `#include <functional>`, and `reference_wrapper` patterns that were superseded by the switch to `std::optional<CameraComponent&>`. These are non-blocking (the actual pseudo-code in sections 7–10 is correct) but the contract document itself is inconsistent.

- [x] **Contract-critic W-08, W-09**: The contract's `camera_component.h` and `render_system.h` pseudo-code included unnecessary `#include <memory>` headers. The actual source code correctly omits these includes (verified: `camera_component.h` includes only `math/camera.h` and `scene/component.h`; `render_system.h` includes no standard headers at all). So the stale contract patterns did not leak into the code.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries) — No violations**:
  - `CameraComponent` in `scene/` depends only on `math/` and `scene/`. ✓
  - `MeshRenderer` in `render/` depends on `scene/` for `Component`. The `render -> scene` dependency is unidirectional and explicitly noted. ✓
  - `RenderSystem` forward-declares `World` and `RenderDevice` — no backend headers in public interface. ✓
  - `cube_scene_demo.cpp` uses only abstract engine types — no SDL3, GL, or backend headers. ✓
  - No backend-specific types leak through `camera_component.h`, `mesh_renderer.h`, or `render_system.h`. ✓
  - `render_system.cpp`, `camera_component.cpp` include only engine abstractions. ✓

- [x] **CONST-002 (Testing Policy) — No violations**:
  - All 30 acceptance criteria (AC-001 through AC-030) have corresponding tests in `tests/scene_rendering_tests.cpp`. ✓
  - All 212 tests pass (29 scene-rendering tests + 183 pre-existing tests). ✓
  - No regressions. ✓

- [x] **CONST-003 (Documentation Policy) — Not applicable**: Rule is still TODO. No violation possible.

- [x] **CONST-004 (Security Policy) — Not applicable**: Rule is still TODO. No violation possible.

- [x] **Principles.md — Partial alignment concern**: The principle "Governance documents must not contradict each other" is strained by the `demo_command.cpp` contradiction between spec and contract (see cross-document coherence above). This is not a formal constitution rule violation (principles are guidelines, not rules) but should be resolved.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-005 (`std::optional<T&>`)**: Fully followed. `World::active_camera()` returns `std::optional<CameraComponent&>`. Internal storage is `std::optional<CameraComponent&>`. No `std::reference_wrapper` used. C++26 baseline confirmed.

- [x] **ADR-006 (dynamic_cast)**: Fully followed. `World::each<T>()` uses `dynamic_cast<T*>` for type matching, consistent with `get_component<T>()` and `remove_component<T>()`.

- [x] **ADR-010 (No raw pointers in public API)**: Fully followed.
  - `active_camera()` returns `std::optional<CameraComponent&>` — not a raw pointer. ✓
  - `register_camera(CameraComponent&)`, `unregister_camera(const CameraComponent&)` use references. ✓
  - `CameraComponent::camera()` returns `math::Camera&`. ✓
  - `MeshRenderer::model()` returns `Model&`. ✓
  - `Component::entity()` returns `Entity` by value. ✓
  - `RenderDevice* device_` and `World* world_` are private members — exempt per ADR-010 §Exception 3 (private non-owning observer pointers). ✓
  - `const char* const* argv` in `run_cube_scene_demo` — exempt per ADR-010 §Exception 1 (C string literal interop). ✓
  - No `const_cast` used anywhere. ✓

- [x] **ADR-001 (Result/Error)**: Followed. `set_uniform()` returns `Result<void>`. `CameraComponent` constructor and `on_attach()` are infallible. `World::each<T>()` returns `size_t`.

- [x] **ADR-003 (Draw returns void)**: Followed. `RenderSystem::render()` does NOT error-check `Model::draw()` calls — confirmed in `render_system.cpp` line 40: `mr.model().draw(*device_)` is called without error handling.

- [x] **SPEC-008 amendment**: SPEC-011 explicitly supersedes SPEC-008 on `on_attach()` lifecycle hooks (spec lines 43–52). The amendment is:
  - Explicitly documented in "Relationship to SPEC-008" section. ✓
  - Targeted and justified (minimal hook needed for CameraComponent auto-registration). ✓
  - All other SPEC-008 contracts (non-copyable/non-movable Component, `vector<unique_ptr<Component>>` storage, `dynamic_cast` dispatch, deferred destruction) remain unchanged. ✓
  - No new ADR needed — the supersession is documented in the spec itself. ✓

- [x] **ADR-010 adoption**: ADR-010 (`docs/adr/010-no-raw-pointers-in-public-api.md`) was created concurrently and is `Accepted`. The spec-critic confirms ADR-010 compliance. The spec references it directly.

## Wiki alignment

Wiki reflects current state and does not become law:

- [ ] **`docs/wiki/architecture/module-map.md` is out of date**: The module map does not mention:
  - `camera_component.h` / `camera_component.cpp` (new file in `scene/`)
  - `mesh_renderer.h` / `mesh_renderer.cpp` (new file in `render/`)
  - `render_system.h` / `render_system.cpp` (new file in `render/`)
  - `cube_scene_demo.h` / `cube_scene_demo.cpp` (new file in `cmd/demo/`)
  - The `render -> scene` dependency introduced by `MeshRenderer` and `RenderSystem`
  - The `"cube-scene"` subcommand (or the fact that it was intended but not registered; see cross-document coherence)
  
  The IMPL-011 contract itself (section "Documentation impact", line 894) acknowledges this: "docs/wiki/architecture/module-map.md — should be updated to note the render -> scene dependency introduced by MeshRenderer and RenderSystem."

  **Required action**: Update `docs/wiki/architecture/module-map.md` with the new files and the `render -> scene` dependency arrow.

- [x] No wiki content contradicts the spec, contract, or code. The gap is one of omission, not commission.

- [x] The wiki does not "become law" — it reflects current state AT the point of implementation completion. The out-of-date sections should be updated as part of the feature completion process.

## Warnings

Non-blocking concerns for awareness:

1. **`demo_command.cpp` not updated for `"cube-scene"` subcommand**: The spec says to register it, the contract forbids modifying the file. The cube_scene_demo exists and compiles but is unreachable from the CLI. Escalate to Orchestrator to resolve spec-contract contradiction. This blocks SC-003 (manual CLI verification) but does not block the core functionality.

2. **Contract document retains stale patterns**: 7 stale `reference_wrapper` patterns remain in the IMPL-011 contract (documentation sections, test instructions, checklist). These do not affect the code (which is correct) but create a misleading paper trail. A cleanup pass on the contract is recommended.

3. **Dead-code SFINAE test in test file**: The code-review (W-03) identified a no-op lambda test (lines 672–678 of `tests/scene_rendering_tests.cpp`) that always returns `false` and tests nothing. The real SFINAE check (lines 682–692) is correct. The dead code should be removed for clarity.

4. **Wiki not updated**: As noted above, the module-map.md wiki page does not reflect the new files or the `render -> scene` dependency. This is a documentation debt item.

5. **Spec correction needed**: The spec contains two inaccuracies in pseudo-code: (a) `Quat::from_axis_angle` should be `Quat::angle_axis`, (b) `MeshRenderer` constructor takes `shared_ptr<Model>` but the spec passes a bare `Model`. These were corrected in the implementation but the spec document itself is not fully accurate.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **Wiki update**: Update `docs/wiki/architecture/module-map.md` to add:
  - `camera_component.h` / `camera_component.cpp` to the scene submodule table
  - `mesh_renderer.h` / `mesh_renderer.cpp` to the render submodule table
  - `render_system.h` / `render_system.cpp` to the render submodule table
  - `cube_scene_demo.h` / `cube_scene_demo.cpp` to the demo files table
  - A note about the `render -> scene` dependency direction
  - Update the demo subcommand list to include `cube-scene` if the spec-contract contradiction is resolved

- **Spec correction**: Update SPEC-011 spec.md to fix:
  - `Quat::from_axis_angle` → `Quat::angle_axis` (line 508)
  - `entity.add_component<be::MeshRenderer>(std::move(cube.model))` → `entity.add_component<be::MeshRenderer>(std::make_shared<be::Model>(std::move(cube.model)))` (line 486, or document that `MeshRenderer` constructor should accept `Model` directly)

- **Contract cleanup**: Address contract-critic warnings W-01 through W-09 in IMPL-011 contract.md:
  - W-01: Remove stale `#include <functional>` from "Files to inspect" table
  - W-02: Remove stale `Add #include <functional>` instruction
  - W-03: Update stale `.get()` pattern in test description
  - W-04: Update stale `->get()` pattern in test setup notes
  - W-05: Fix non-compiling example code
  - W-06: Update stale `.get()` in edge case description
  - W-07: Remove stale `#include <functional>` in Done criteria
  - W-08: Remove unnecessary `#include <memory>` from `camera_component.h` pseudo-code (already done in actual code)
  - W-09: Remove unnecessary `#include <memory>` from `render_system.h` pseudo-code (already done in actual code)

- **Resolve spec-contract contradiction on `demo_command.cpp`**: Either:
  - (a) Amend the contract to allow modifying `demo_command.cpp` (one include + two lines of dispatch code), OR
  - (b) Update the spec to remove the requirement to register `"cube-scene"` as a CLI subcommand (making the demo reachable only through direct function call)

- **ADR-010 compliance**: ADR-010 §Compliance says "The `docs/wiki/architecture.md` SHALL be updated to document this convention". Confirm that `docs/wiki/architecture.md` (or `module-map.md`) is updated to document the no-raw-pointers convention. This is a pre-existing ADR compliance gap, not specific to this feature.

## Review history

| Date | Verdict | Summary |
|------|---------|---------|
| 2026-05-30 | `Accepted with warnings` | No constitution violations. All ADRs followed (005, 006, 010, 001, 003). SPEC-008 supersession properly documented. CONST-001 and CONST-002 respected. Three categories of warnings identified: (1) spec-contract contradiction on `demo_command.cpp` — `cube_scene_demo` compiles but is unreachable from CLI; (2) wiki outdated — new files and dependencies not documented; (3) contract document retains stale patterns from pre-`optional<T&>` era. Cross-document coherence is otherwise strong — all 30 ACs pass and code matches accepted contract. |
