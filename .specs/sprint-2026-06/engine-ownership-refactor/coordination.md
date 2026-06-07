# Workflow Coordination: engine-ownership-refactor

## Orchestrator

**Feature**: engine-ownership-refactor
**Status**: in-progress
**Current step**: code-implementer
**Initial instructions**: Refactor EngineContext, App lifecycle, and ownership model:
- Entity creation via `world.add_entity()` (was `Entity::create(*world_)`)
- Remove `unique_ptr<Entity>` pattern (Entity is 16B handle, store by value)
- Move AssetManager ownership fully into EngineService (apps use `engine.assets()`)
- run_app creates and owns World + RenderSystem
- EngineContext gains: RenderDevice&, World&, RenderSystem&, int frame
- App::setup(EngineContext const&) — replaces setup(EngineService&)
- App::on_render(EngineContext const&) — replaces render(RenderDevice&, int), default no-op
- run_app calls render_system.render_scene() automatically before app.on_render()
- Migrate all 13 apps to the new model

**Notes**:
- Workflow started: 2026-06-06
- Feature name: engine-ownership-refactor
- Sprint: sprint-2026-06
- Priorité: haute, à faire maintenant
- Answers from grill-me:
  - world.add_entity() name confirmed
  - Entity by value (remove unique_ptr<Entity>) confirmed
  - AssetManager: uniquement dans EngineService (Option B)
  - EngineContext structure avec services, window, device, world, render_system, delta_time, frame
  - setup(EngineContext const&) et on_render(EngineContext const&) confirmés
  - run_app: render_scene AVANT app.on_render()
  - Tous les 13 apps sont migrés
  - Aucun app supprimé
  - Vérification: build + tests + captures visuelles

## Decision Log

### AssetManager ownership
- **Date**: 2026-06-06
- **Decision**: AssetManager only in EngineService. Apps use `ctx.services.assets()`.
- **Rationale**: EngineService already creates an AssetManager. Apps duplicating it is redundant.

### Entity API
- **Date**: 2026-06-06
- **Decision**: `world.add_entity()` as the new name for entity creation. No more `unique_ptr<Entity>`.
- **Rationale**: Entity is 16 bytes, trivially copyable. `unique_ptr<Entity>` adds indirection with no benefit.

### EngineContext structure
- **Date**: 2026-06-06
- **Decision**: EngineContext contains EngineService&, Window&, RenderDevice&, World&, RenderSystem&, float delta_time, int frame.
- **Rationale**: Single per-frame context passed to all lifecycle methods. Removes need for separate parameters.

### App lifecycle
- **Date**: 2026-06-06
- **Decision**: `setup(EngineContext const&)`, `on_render(EngineContext const&)` (default no-op, optional). run_app owns render_scene() call.
- **Rationale**: Apps shouldn't need to call render_scene() themselves. Custom rendering is the exception, not the rule.

### Frame loop order
- **Date**: 2026-06-06
- **Decision**: poll_events → begin_frame → on_frame_begin() → update_updatables(ctx) → render_system.render_scene() → on_render(ctx) → capture → end_frame
- **Rationale**: Standard scene render happens first, custom rendering overlays on top.

### Scope
- **Date**: 2026-06-06
- **Decision**: All 13 apps migrated. No apps removed. Verification: build + unit tests + visual captures.

### on_frame_begin signature
- **Date**: 2026-06-06
- **Decision**: `on_frame_begin(EngineContext const& ctx)` — add EngineContext parameter.
- **Rationale**: Apps need `ctx.frame` (for animation) and `ctx.services.assets()` (for hot-reload polling). Storing refs from setup() is more verbose and error-prone.

### World/RenderSystem creation
- **Date**: 2026-06-06
- **Decision**: run_app() always creates World + RenderSystem for all apps unconditionally.
- **Rationale**: Empty World is cheap (~1KB). Simplifies the frame loop. render_scene() on empty World is a no-op.

### Exit handling
- **Date**: 2026-06-06
- **Decision**: Remove `app.running_` / `set_running()`. Use only `ctx.request_exit()` / `ctx.is_exit_requested()`.
- **Rationale**: EngineContext is available everywhere (setup, on_frame_begin, on_render). run_app() can check ctx directly after update_updatables().

### shutdown() signature
- **Date**: 2026-06-06
- **Decision**: Keep `shutdown()` without parameters.
- **Rationale**: No app needs context during shutdown. Simpler.

### run_app() namespace
- **Date**: 2026-06-06
- **Decision**: Keep `run_app()` in `buddd::cmd` namespace.
- **Rationale**: run_app() is a CLI framework detail. Moving would break callers.

### HotReloadGltfApp frame counter
- **Date**: 2026-06-06
- **Decision**: Use `ctx.frame` directly from `on_frame_begin(ctx)`. Remove `frame_count_` member.
- **Rationale**: ctx.frame is the canonical source of frame number.

### HotReloadGltfApp reload_model AssetManager access
- **Date**: 2026-06-06
- **Decision**: Store `AssetManager&` reference member (obtained from `ctx.services.assets()` in setup()).
- **Rationale**: Non-owning, EngineService outlives the app. Simplest approach.

### setup() failure handling
- **Date**: 2026-06-06
- **Decision**: `shutdown()` is called systematically even if `setup()` fails after World/RenderSystem creation.
- **Rationale**: Current behavior preserved. World/RenderSystem exist during shutdown(), destroyed via unique_ptr after.

## spec-author

**Status**: completed
**Summary**:
Addressed all 5 blocking issues from spec-critic and incorporated all human decisions from loop-back:
- Resolved all 5 [NEEDS CLARIFICATION] items (shutdown no-params, run_app in buddd::cmd, HotReloadGltfApp uses ctx.frame, setup() failure calls shutdown(), AssetManager& ref member)
- Fixed AC-013 with exact frame numbers (30/60 texture swap for HotReloadApp, scale swap for HotReloadGltfApp)
- Fixed AC-015 with concrete criteria (no crash, empty World, no-op render_scene, no assertions)
- Added Documentation Updates section listing 5 affected files (wiki + ADRs)
- Added Related Documents section (ADR-023, ADR-014)
- Added Recommended migration order (RunApp → ... → HotReloadApp)
- Added render_scene() failure and early-exit error cases
- Added default no-op clarification for on_frame_begin() and on_render()
- Removed optional trace-logging suggestion (out of scope)
- Updated HotReloadGltfApp section with AssetManager& ref and ctx.frame
**Artifacts**:
- `.specs/sprint-2026-06/engine-ownership-refactor/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review (2026-06-06): All 5 previous blocking issues are RESOLVED. DoR criteria satisfied. Spec is accepted.
- Documentation Updates section added (5 files: 3 wiki + 2 ADR)
- All 5 [NEEDS CLARIFICATION] items resolved with human decisions
- Error handling for render_scene() failure documented
- AC-013 now has exact frame numbers (30/60)
- AC-015 now has concrete criteria (no crash, empty World, no-op, no assertions)
- Migration order added, ADR references added, trace-logging removed
1 new minor warning: HotReloadGltfApp AssetManager& reference member cannot be directly initialized in setup() — C++ mechanics need adjustment (use pointer instead). This is non-blocking.
**Artifacts**:
- `.specs/sprint-2026-06/engine-ownership-refactor/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- HotReloadGltfApp AssetManager& reference member: declared as `AssetManager&` but this cannot be initialized in setup() — C++ reference members must be initialized in the constructor initializer list. Implementation contract should use `AssetManager*` (raw pointer), `std::reference_wrapper`, or pass the reference through a changed construction pattern. Non-blocking — design intent is clear.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Wrote implementation contract covering:
- EngineContext with 7 fields (services, window, device, world, render_system, delta_time, frame)
- World::create_entity() → add_entity(), Entity::create() removed
- App base class: setup(EngineContext const&), on_render(EngineContext const&) default no-op, on_frame_begin(EngineContext const&), removed world()/running_/set_running()
- run_app() creates World+RenderSystem unconditionally, render_scene() before on_render(), exit via ctx.is_exit_requested() only
- All 13 apps migrated with per-app rules (7 apps move transforms to on_frame_begin, HotReloadGltfApp uses AssetManager* pointer + World& params for reload_model/create_entities)
- 17-item Done criteria checklist

**Loop-back fixes applied** (from implementation-contract-critic):
1. Fixed section 6.5: changed "Remove `#include <chrono>`" to "Keep `#include <chrono>`" (start_time_ requires it)
2. Fixed line 56: "23 header+source pairs" → "26 files (13 header+source pairs)"
3. Fixed Done criteria #10: grep allows matches in `hot_reload_gltf_app.*` only
4. Fixed section 6.6: added explicit note that `<chrono>` is kept (same as CubeSceneApp)
**Artifacts**:
- `.specs/sprint-2026-06/engine-ownership-refactor/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- HotReloadGltfApp uses `AssetManager*` (raw pointer, not reference) set in setup() per spec-critic warning resolution. The spec's original `AssetManager&` would not compile in setup().
- HotReloadGltfApp's reload_model() and create_entities() now accept a `World&` parameter since the world_ member was removed. This is a slight expansion beyond the spec's "remove world_" instruction for correctness.
- The `ctx.is_exit_requested()` check after `on_frame_begin()` and after `update_updatables()` requires calling `end_frame()` before breaking the frame loop, since `begin_frame()` was already called.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review (2026-06-06): All 4 previous issues are RESOLVED. Contract is accepted:
1. CubeSceneApp `#include <chrono>`: now KEPT (not removed) — resolved ✓
2. File count: "26 files (13 header+source pairs)" — resolved ✓
3. Done criteria #10: grep for `asset_manager_` allows matches in `hot_reload_gltf_app.*` only — resolved ✓
4. TexturedCubeApp: explicitly says `<chrono>` is kept — resolved ✓

No new issues found. Contract is thorough, precise, and ready for human validation.
**Artifacts**:
- `.specs/sprint-2026-06/engine-ownership-refactor/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-06 21:30 EDT
**Warnings**:
<none>
**Notes**: Approved with all reviews passing. Ready to implement.

## code-implementer

**Status**: completed
**Summary**:
Implemented the full engine ownership refactoring across all 33 files:
- EngineContext: added RenderDevice&, World&, RenderSystem&, int frame fields (7 fields total)
- World: renamed create_entity() → add_entity() in both declaration and implementation
- Entity: kept static create() as backward-compatible shim delegating to world.add_entity() (required by internal engine code in model_utils.h and 103+ test callers)
- App base class: new signatures setup(EngineContext const&), on_frame_begin(EngineContext const&), on_render(EngineContext const&) default no-op. Removed render(), world(), is_running(), set_running(), running_
- run_app(): creates World+RenderSystem unconditionally, new frame loop with render_scene() before on_render(), exit via ctx.is_exit_requested()
- All 13 apps migrated: removed owned World/RenderSystem/AssetManager/unique_ptr Entity members, Entity by value, on_frame_begin(ctx) for per-frame updates, ctx.world.add_entity(), ctx.services.assets() for shared AssetManager
- HotReloadGltfApp: AssetManager* pointer, World& params for reload_model/create_entities
**Artifacts**:
- `src/engine/engine_context.h` — added 4 new fields + forward declarations
- `src/engine/scene/entity.h` — added backward-compatible create() shim
- `src/engine/scene/entity.cpp` — create() now delegates to world.add_entity()
- `src/engine/scene/world.h` — create_entity() → add_entity()
- `src/engine/scene/world.cpp` — same rename
- `src/cmd/app.h` — new lifecycle signatures, removed old methods
- `src/cmd/app.cpp` — new run_app() frame loop with World+RenderSystem ownership
- `src/cmd/apps/run_app.h` — new signatures, no render()
- `src/cmd/apps/triangle_app.h` + `.cpp` — new signatures, on_render, engine_context.h include
- `src/cmd/apps/cube_app.h` + `.cpp` — new signatures, on_render, engine_context.h include
- `src/cmd/apps/multi_material_app.h` + `.cpp` — new signatures, on_render, engine_context.h include
- `src/cmd/apps/cube_scene_app.h` + `.cpp` — major restructure, Entity by value, on_frame_begin for rotation
- `src/cmd/apps/textured_cube_app.h` + `.cpp` — major restructure, Entity by value, on_frame_begin for rotation
- `src/cmd/apps/free_camera_app.h` + `.cpp` — removed world() override, Entity by value
- `src/cmd/apps/phong_app.h` + `.cpp` — Entity by value, on_frame_begin for orbiting lights
- `src/cmd/apps/asset_demo_app.h` + `.cpp` — removed asset_manager_, Entity by value, ctx.services.assets()
- `src/cmd/apps/hot_reload_app.h` + `.cpp` — removed asset_manager_, Entity by value, on_frame_begin for texture swap+rotation
- `src/cmd/apps/gltf_demo_app.h` + `.cpp` — removed asset_manager_, Entity by value, on_frame_begin for camera orbit
- `src/cmd/apps/gltf_helmet_app.h` + `.cpp` — removed asset_manager_, Entity by value, no overrides needed
- `src/cmd/apps/hot_reload_gltf_app.h` + `.cpp` — AssetManager* pointer, World& params for reload/create_entities, ctx.frame, on_frame_begin for camera
**Questions for human**:
none
**Warnings**:
- Entity::create(World&) kept as backward-compatible shim (delegates to world.add_entity()). Required by engine-internal code in `src/engine/render/model_utils.h` and 103+ test callers. Removing it would require modifying files outside the contract's allowed list.
- Release build linker error is pre-existing (fails on clean master too), not caused by this refactoring.
**Blocking issues**:
none

## code-reviewer

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- `.specs/sprint-2026-06/engine-ownership-refactor/code-review.md`
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## wiki-agent

**Status**: pending
**Summary**:
<none>
**Artifacts**:
<none>
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## governance-reviewer

**Status**: pending
**Summary**:
<none>
**Artifacts**:
- `.specs/sprint-2026-06/engine-ownership-refactor/governance-review.md`
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
