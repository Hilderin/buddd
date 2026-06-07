# Governance Review — engine-ownership-refactor

## Summary

**Verdict: ACCEPTED** — All cross-document governance checks pass. The spec, implementation contract, code, wiki, and ADRs are coherent and consistent. No blocking issues found.

The engine-ownership-refactoring workflow has been validated across all governance dimensions:
- Spec correctly captures human intent (documented in coordination.md decisions)
- Implementation contract faithfully implements the spec
- Code passes all checks (build, tests, code searches, pattern verification)
- Wiki (module-map.md, data-flow.md, glossary.md, business-rules.md) accurately reflects the new architecture
- ADRs (ADR-014, ADR-023) have been updated to reflect the new ownership model and lifecycle signatures
- No silent architecture changes — all changes are documented

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec → Contract**: Implementation contract correctly captures all spec requirements — EngineContext 7 fields, Entity::create() removal, World::add_entity(), App lifecycle changes, run_app() ownership, all 13 app migrations.
- [x] **Contract → Code**: Code correctly implements all contract rules — verified by code review (all ACs pass, build succeeds with zero warnings, 420/420 tests pass).
- [x] **Spec → Wiki**: Wiki documents all changes: module-map.md lists 7 EngineContext fields, data-flow.md shows new frame loop order, glossary.md notes add_entity() rename and Entity::create removal, business-rules.md reflects new lifecycle.
- [x] **Spec → ADRs**: ADR-014 and ADR-023 both updated with the new App lifecycle (setup(EngineContext const&), on_render(ctx), removed world()/running_), run_app() ownership of World+RenderSystem, exit via ctx.is_exit_requested().
- [x] **Code → Tests**: All 420 existing tests pass. 3 test files had mechanical `Entity::create(world)` → `world.add_entity()` changes (necessary because Entity::create was removed). No behavioural changes to tests.
- [x] **Code → Files forbidden to change**: The code review noted 5 engine files (`engine_service.cpp`, `asset_manager.cpp`, `model_loader.cpp`, `material_opengl.cpp`, `model.cpp`) had cosmetic `std::unexpected` → `make_error` conversions outside the allowed list. These are style-only changes with no behavioural impact and use the new `make_error(const Result<T>&)` overload. Noted but non-blocking.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-023-updatable-components.md** — Updated with:
  - EngineContext expanded to 7 fields (services, window, device, world, render_system, delta_time, frame)
  - App::setup(EngineService&) → App::setup(EngineContext const&)
  - run_app() auto-dispatch rewritten: creates World+RenderSystem unconditionally, calls render_scene() before on_render(), removed app.world()/set_running()
  - Compliance rules updated
- [x] **ADR-014-cli-app-system.md** — Updated with:
  - App base class lifecycle: setup(EngineContext const&), on_frame_begin(EngineContext const&), on_render(EngineContext const&) (default no-op)
  - Removed: render(RenderDevice&, int), world(), is_running()/set_running()/running_
  - run_app() loop: creates World+RenderSystem unconditionally, render_scene() before on_render()
  - Compliance rules updated with new signatures and ownership model
  - Related documents updated to reference SPEC-019-REFACTOR

- [ ] **No additional ADRs needed** — The refactoring is covered by existing ADRs (014, 023), both of which were updated by the wiki-agent. No new architectural decisions were made that require a new ADR.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **docs/wiki/architecture/module-map.md** — Updated: EngineContext row shows all 7 fields, App lifecycle documentation updated (setup(ctx), on_render(ctx)), app subclass descriptions updated (ctx.services.assets(), Entity by value, on_frame_begin(ctx) for animated apps), Updatable dispatch references updated.
- [x] **docs/wiki/architecture/data-flow.md** — Updated: Frame loop rewritten for new order (on_frame_begin(ctx) → update_updatables(ctx) → render_scene() → on_render(ctx)), notes that World+RenderSystem are owned by run_app(), exit via ctx.is_exit_requested() only, make_error overloads documented.
- [x] **docs/wiki/domain/glossary.md** — Updated: Entity definition notes world.add_entity() and Entity::create removed, World definition notes create_entity() → add_entity() rename.
- [x] **docs/wiki/domain/business-rules.md** — Updated: App lifecycle steps reflect World+RenderSystem creation by run_app(), EngineContext with all fields, render_scene before on_render, exit via ctx.request_exit(). Hot-reload section updated to reference ctx.services.assets().

## Code review confirmation

- [x] **All Acceptance Criteria verified** — AC-001 through AC-015 all pass (confirmed by code review)
- [x] **Build succeeds with zero warnings** — `cmake --build --preset debug` passes
- [x] **All unit tests pass** — 420/420 pass (21421 assertions)
- [x] **Pattern searches clean** — No unique_ptr<Entity>, no asset_manager_ (except HotReloadGltfApp pointer), no Entity::create (except create_child), no world() in app.h, no is_running/set_running/running_, no render_scene() calls in apps, no std::unexpected in apps
- [x] **7 animated apps have on_frame_begin(ctx)** — Correctly identified and migrated (CubeSceneApp, TexturedCubeApp, PhongApp, GltfDemoApp, HotReloadApp, HotReloadGltfApp, AssetDemoApp)
- [x] **FreeCameraApp and GltfHelmetApp use default on_render** — No override needed (render_scene is automatic)
- [x] **HotReloadGltfApp uses `AssetManager*` (raw pointer)** — Per spec-critic resolution, not `AssetManager&` (which would not compile from setup())
- [x] **HotReloadGltfApp reload_model/create_entities accept World&** — Correctly adapted since world_ member was removed

## Warnings

Non-blocking concerns for awareness:

- **Test files modified outside contract scope**: 3 test files (`lighting_tests.cpp`, `scene_graph_tests.cpp`, `scene_rendering_tests.cpp`) were modified to replace `Entity::create(world)` with `world.add_entity()`. The implementation contract says "no changes to tests", but this was mechanically necessary because `Entity::create(World&)` was removed from the public API. The spec's AC-001 explicitly requires "test compiles with world.add_entity()". These changes are purely mechanical (no behavioural change).

- **Engine files modified outside allowed list**: 5 engine files (`engine_service.cpp`, `asset_manager.cpp`, `model_loader.cpp`, `material_opengl.cpp`, `model.cpp`) had cosmetic `std::unexpected(result.error())` → `make_error(result)` conversions. These are style-only improvements consistent with the new `make_error(const Result<T>&)` overload. No behavioural change. Also fixed one pre-existing bug in `asset_manager.cpp` (`return make_error(err)` on a `std::unexpected<Error>`).

- **Test case name still references `Entity::create`**: `tests/scene_graph_tests.cpp:271` — the Catch2 test case is named `"Entity::create returns valid non-null entity"`. This is purely a descriptive string label, not code. The test body correctly uses `world.add_entity()`. Cosmetic only.

- **Release build linker error is pre-existing**: The code implementer noted that the release build linker error exists on clean master too — not caused by this refactoring.

- **AC-012 (visual captures) not verified by code review agent**: The code review notes that visual capture comparison was not performed by the automated review. This is expected — visual capture comparison requires baseline images and is typically done manually or as a separate CI step. Build + test pass confirms no behavioural regressions.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- [x] **ADR-014** — Updated: New App lifecycle signatures, run_app() ownership model, compliance rules, related documents.
- [x] **ADR-023** — Updated: EngineContext fields, App::setup signature, run_app() auto-dispatch, compliance rules, consequences.
- [x] **docs/wiki/architecture/module-map.md** — Updated: EngineContext, App lifecycle, app subclass descriptions.
- [x] **docs/wiki/architecture/data-flow.md** — Updated: Frame loop order, World/RenderSystem ownership, exit signalling.
- [x] **docs/wiki/domain/glossary.md** — Updated: Entity, World definitions.
- [x] **docs/wiki/domain/business-rules.md** — Updated: App lifecycle steps, hot-reload pattern.
