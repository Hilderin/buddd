# SPEC-019-REFACTOR - Engine Ownership Model & App Lifecycle Refactoring

## Problem

The current engine ownership model has accumulated ownership inconsistencies and unnecessary indirections that increase boilerplate and risk of misuse:

1. **Entity creation is non-idiomatic**: `Entity::create(*world_)` is a factory pattern for a 16-byte handle. Entity should be created via `world.add_entity()` — the world is the natural factory for entities that belong to it.
2. **`std::unique_ptr<Entity>` is wasteful**: Entity is 16 bytes (pointer + EntityId), trivially copyable. Apps wrap it in `unique_ptr<Entity>`, adding heap indirection with zero benefit.
3. **Redundant AssetManager instances**: Each app creates its own `AssetManager::create(device, "assets")`, while `EngineService::assets()` already owns one. This duplicates state and means `poll_file_events()` and `clear()` must be called on the app's private instance, not the shared engine one.
4. **EngineContext is incomplete**: It stores only `EngineService&`, `Window&`, and `float delta_time`. Apps that need `RenderDevice&`, `World&`, or `RenderSystem&` must obtain them through other means (storing their own copies, navigating the object graph).
5. **App::render() signature is rigid**: Takes `RenderDevice&` and `int frame` separately. Adding more per-frame parameters requires changing all override signatures.
6. **World/RenderSystem ownership is scattered**: Each app independently creates and owns its own `World` and `RenderSystem`. The `run_app()` function has no access to these, forcing apps to expose `world()` so `run_app()` can dispatch Updatable components through it.
7. **render_scene() is called inside apps**: All World-using apps must call `render_system_->render_scene()` at the end of their `render()` method. This is boilerplate that should be owned by the frame loop.
8. **`App::world()` accessor leaks ownership**: Used only by `run_app()` to dispatch Updatable updates. If `run_app()` owned the World, this accessor would be unnecessary.

## Goals

1. Eliminate `Entity::create()` static factory — replace with `World::add_entity()`
2. Remove all `std::unique_ptr<Entity>` — store Entity handles by value in all apps
3. Move AssetManager ownership exclusively to EngineService — apps use `ctx.services.assets()`
4. EngineContext gains all per-frame references: `RenderDevice&`, `World&`, `RenderSystem&`, `int frame`
5. `App::setup()` takes `EngineContext const&` instead of `EngineService&`
6. `App::render()` is replaced by `App::on_render(EngineContext const&)` — a virtual with default no-op
7. `run_app()` creates and owns World + RenderSystem per app
8. `run_app()` calls `render_system.render_scene()` automatically before `app.on_render()`
9. Remove `App::world()` accessor — no longer needed
10. All 13 existing apps are migrated with zero visual behavioural change

## Non-goals

- No new rendering features, scenes, or demo apps
- No apps are removed or renamed
- No changes to the `RenderSystem` public API (other than its ownership transfer)
- No changes to the `World` component system, entity hierarchy, or Updatable dispatch logic
- No changes to `EngineService` creation, `AssetManager` public API, or asset loading behaviour
- No changes to the capture pipeline, frame limiting, or exit code logic
- No changes to the `buddd::engine` public API beyond `Entity::create()` removal and `World::add_entity()` addition

## Actors

| Actor | Role |
|---|---|
| **Engine developer** | Maintains the engine library (`src/engine/`). Writes the refactored `EngineContext`, `World` (rename), `Entity` (remove static factory). |
| **App developer** | Maintains the 13 app subclasses in `src/cmd/apps/`. Migrates each app to the new lifecycle. |
| **Framework integrator** | Maintains `run_app()` in `src/cmd/app.cpp`. Changes the frame loop and ownership model. |
| **Test engineer** | Runs unit tests and visual capture comparisons to verify no behavioural regressions. |

## User-visible behavior

All user-visible behavior is **preserved unchanged**:

- All 13 apps produce identical visual output (pixel-identical captures)
- All CLI flags (`--frame`, `--capture`, `--scene`, etc.) behave identically
- All exit codes remain identical
- All interactive controls (WASD, mouse look, Escape) behave identically
- All hot-reload mechanics (file swapping, `poll_file_events()` timing) behave identically
- Frame timing and delta_time remain unaffected

The only user-visible change is internal code quality: no change in observable behaviour.

## User stories

### Story 1 - Entity creation from World (Priority: P1)

**As an** app developer,
**I want** to create entities via `world.add_entity()` instead of `Entity::create(world)`,
**so that** the World is the natural factory for entities that belong to it.

**Given** a World instance,
**When** I call `world.add_entity()`,
**Then** an Entity handle is returned, and the entity exists in the World.

**Given** a World instance,
**When** I call `Entity::create(world)`,
**Then** the code does not compile (method removed).

### Story 2 - Entity by value (Priority: P1)

**As an** app developer,
**I want** to store Entity handles by value instead of `std::unique_ptr<Entity>`,
**so that** I avoid unnecessary heap allocation for a 16-byte handle.

**Given** an app that previously stored `std::unique_ptr<Entity> entity_`,
**When** it is migrated,
**Then** it stores `Entity entity_;` directly as a value member.

### Story 3 - Shared AssetManager (Priority: P1)

**As an** app developer,
**I want** to use the EngineService's AssetManager instead of creating my own,
**so that** there is a single source of truth for asset lifecycle management.

**Given** an app that previously created `std::unique_ptr<AssetManager> asset_manager_`,
**When** it is migrated,
**Then** it accesses the shared AssetManager via `ctx.services.assets()`.

### Story 4 - Complete EngineContext (Priority: P1)

**As an** app developer,
**I want** EngineContext to contain all per-frame references I need,
**so that** I don't need to store separate member variables for World, RenderSystem, etc.

**Given** the frame loop,
**When** EngineContext is constructed,
**Then** it contains `EngineService&`, `Window&`, `RenderDevice&`, `World&`, `RenderSystem&`, `float delta_time`, and `int frame`.

### Story 5 - Simplified App lifecycle (Priority: P1)

**As an** app developer,
**I want** a simpler App API where `setup()` and `on_render()` both receive `EngineContext const&`,
**and** `render_scene()` is called automatically by the frame loop,
**so that** my app code is simpler and I don't need to manually invoke scene rendering.

**Given** an app that previously called `render_system_->render_scene()` in `render()`,
**When** it is migrated,
**Then** that call is removed from the app, and `run_app()` calls it automatically.

### Story 6 - World/RenderSystem owned by run_app (Priority: P2)

**As an** framework integrator,
**I want** `run_app()` to create and own the World and RenderSystem,
**so that** the frame loop has direct access to them without going through the app.

**Given** `run_app()`,
**When** it starts,
**Then** it creates a `World` and `RenderSystem` on behalf of the app and passes references via `EngineContext`.

### Story 7 - Visual regression verification (Priority: P2)

**As a** test engineer,
**I want** to verify that all 13 apps produce pixel-identical captures before and after the refactoring,
**so that** I can be confident no rendering behaviour changed.

**Given** the refactored codebase,
**When** I run each app with its standard capture arguments,
**Then** the output image matches the baseline (pre-refactoring) capture pixel-for-pixel.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `Entity::create(World&)` is removed; `World::add_entity()` is added with equivalent behaviour | Code search shows `Entity::create` removed; test compiles with `world.add_entity()` |
| AC-002 | No `std::unique_ptr<buddd::engine::Entity>` exists in any app | Code search across `src/cmd/apps/` for `unique_ptr.*Entity` returns zero matches |
| AC-003 | No private `asset_manager_` member exists in any app | Code search across `src/cmd/apps/` for `asset_manager_` returns zero matches (excluding engine library) |
| AC-004 | `EngineContext` contains `RenderDevice&`, `World&`, `RenderSystem&`, `int frame` | Static assert or header check confirms new fields exist and are populated in frame loop |
| AC-005 | `App::setup()` signature is `auto setup(EngineContext const& ctx) -> Result<void>` | All 13 apps compile with new signature |
| AC-006 | `App::render()` is removed; `App::on_render(EngineContext const&)` exists with default no-op | All 13 apps compile; `RunApp` does not override `on_render()` and compiles |
| AC-007 | `App::world()` is removed | Code search for `world() noexcept` in `App` class and overrides returns zero matches |
| AC-008 | `run_app()` creates a `World` and `RenderSystem` per app | Trace shows World and RenderSystem constructed in `run_app()` body |
| AC-009 | `run_app()` calls `render_system.render_scene()` before `app.on_render()` | Frame loop order confirmed: `render_scene()` before `on_render()` |
| AC-010 | All 13 apps compile without errors | `cmake --build` succeeds with no warnings related to the refactored API |
| AC-011 | Unit tests pass | All existing unit tests pass with no regressions |
| AC-012 | Visual captures for all 13 apps match pre-refactoring baselines | Pixel-level comparison of before/after capture images for each app |
| AC-013 | Apps that call `poll_file_events()` do so via `ctx.services.assets().poll_file_events()` | **HotReloadApp**: texture swap occurs at frame 30; capture at frame 30 shows pre-swap texture; capture at frame 60 shows post-swap texture. **HotReloadGltfApp**: model scale change (1.0 → 2.0) at frame 30; capture at frame 30 shows pre-swap scale 1.0; capture at frame 60 shows post-swap scale 2.0 |
| AC-014 | Apps that previously used `frame` parameter in `render()` now get it from `ctx.frame` | `GltfDemoApp`, `HotReloadApp`, `HotReloadGltfApp` compile and behaviour matches |
| AC-015 | `RunApp` (empty window) works with World/RenderSystem always created | RunApp completes its full frame loop without crashing; World is empty (no entities); `render_scene()` on empty World is a no-op (no assertions fire, no crashes); window opens and closes normally via Escape → `request_exit()` |

## E2E Verification

- **Method**: Run each of the 13 apps with `--capture` at key frames and compare output images pixel-by-pixel to pre-refactoring baselines (stored in `tests/captures/baselines/`).
- **Secondary**: Full build with `cmake --build --preset debug` and all unit tests pass.
- **Tertiary**: Manual interactive testing of `FreeCameraApp`, `PhongApp`, `GltfHelmetApp` (WASD, mouse look, Escape exit).

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All 13 apps compile without any new warnings or errors |
| SC-002 | All existing unit tests pass (zero regressions) |
| SC-003 | Visual captures for all 13 apps are pixel-identical to pre-refactoring baselines |
| SC-004 | Zero `unique_ptr<Entity>` instances remain in app code |
| SC-005 | Zero private `AssetManager` instances remain in app code |
| SC-006 | `App::world()` method is removed from the base class and all subclasses |
| SC-007 | `Entity::create()` is removed from the public API |

## Target Architecture

### New `EngineContext`

```cpp
struct EngineContext {
    EngineService& services;
    Window& window;
    RenderDevice& device;      // NEW: convenience access
    World& world;              // NEW: owned by run_app
    RenderSystem& render_system; // NEW: owned by run_app
    float delta_time;
    int frame;                 // NEW: moved from render() parameter

    void request_exit() const { exit_requested_ = true; }
    [[nodiscard]] auto is_exit_requested() const -> bool { return exit_requested_; }

    mutable bool exit_requested_ = false;
};
```

### New `App` base class

```cpp
class App {
public:
    virtual ~App() = default;

    [[nodiscard]] virtual auto config() const -> AppConfig = 0;

    /// Takes EngineContext (which provides access to EngineService, device, etc.)
    [[nodiscard]] virtual auto setup(EngineContext const& ctx)
        -> Result<void> = 0;

    /// Called before Updatable dispatch. Provides ctx.frame and ctx.services.assets(). Default no-op.
    virtual auto on_frame_begin(EngineContext const& ctx) -> void {}

    /// Called once per frame after render_scene().
    /// Replaces render(RenderDevice&, int). Default no-op.
    virtual auto on_render(EngineContext const& ctx) -> void {}

    virtual auto shutdown() -> void {}
};
```

**Defaults note**: Both `on_frame_begin()` and `on_render()` have default no-op implementations. Apps that do NOT need per-frame state updates do NOT need to override `on_frame_begin()`. Apps that do NOT need custom rendering (beyond the automatic `render_scene()`) do NOT need to override `on_render()`.

**Removals from `App`**:
- `render(RenderDevice&, int)` — replaced by `on_render(EngineContext const&)`
- `world()` — no longer needed (World is owned by `run_app()`)
- `is_running()` / `set_running()` / `running_` member — exit signalling moved to `EngineContext::request_exit()` / `is_exit_requested()`

### New `run_app()` frame loop

```
1. app.config() → AppConfig
2. EngineService::create(backend, config) → EngineService
3. print "Window opened: WxH"
4. Create World (always — empty World is ~1KB, render_scene on empty World is a no-op)
5. Create RenderSystem(device, world) (always — paired with World)
6. app.setup(ctx) where ctx = {services, window, device, world, render_system, delta_time, frame}
   — the app receives references to the world/render_system that run_app owns
7. print start message
8. Loop:
   a. poll_events() — if false, break
   b. if ctx.is_exit_requested(), break
   c. device.begin_frame()
   d. app.on_frame_begin(ctx)   ← ctx provides ctx.frame and ctx.services.assets()
   e. World::update_updatables(EngineContext{complete ctx})
   f. if ctx.is_exit_requested(), break
   g. render_system.render_scene()     ← MOVED HERE (was in apps)
   h. app.on_render(EngineContext{complete ctx})  ← was app.render()
   i. capture injection (if frame matches)
   j. device.end_frame()
   k. ++frame
9. print completion/abort
10. app.shutdown()
11. print "Window closed, shutting down."
12. return exit code
```

**Key ownership changes**:
- `run_app()` creates `World` and `RenderSystem` via `std::unique_ptr` (or by value if movable)
- These are passed by reference through `EngineContext` to the app
- `render_system.render_scene()` is called automatically before `on_render()`
- All apps receive a valid World and RenderSystem — empty World apps simply ignore them

### Handling apps without World/RenderSystem

World and RenderSystem are created unconditionally for ALL apps. Four apps (`RunApp`, `TriangleApp`, `CubeApp`, `MultiMaterialApp`) do not use them, but this is harmless:

- An empty World (no entities) is valid and cheap (~1KB).
- `render_scene()` on an empty World is a no-op.
- The apps simply do not call `world.add_entity()` or override `on_render()` (default no-op).
- All apps receive valid references via `EngineContext` — no special-casing in the frame loop.

## Changes per Component

### EngineService (src/engine/engine_service.h/.cpp)

**No changes needed.** `EngineService` already owns `AssetManager` and exposes it via `.assets()`. The refactoring simply removes the redundant app-level AssetManager instances.

### EngineContext (src/engine/engine_context.h)

**Changes**:
- Add `RenderDevice& device` field
- Add `World& world` field
- Add `RenderSystem& render_system` field
- Add `int frame` field

**New fields** are listed after existing `Window& window` for logical grouping (core services → rendering → scene → frame state).

### World (src/engine/scene/world.h/.cpp)

**Changes**:
- Add `auto add_entity() -> Entity;` — same implementation as `create_entity()` currently
- Keep `create_entity()` as an alias during transition OR remove it immediately
- **Decision from grill-me**: Remove `create_entity()`, rename to `add_entity()`

No other changes to World.

### Entity (src/engine/scene/entity.h/.cpp)

**Changes**:
- Remove `static auto create(World& world) -> Entity;`
- The private constructor `Entity(World& world, EntityId id)` remains (used by World as friend)
- No changes to the Entity handle class itself

### App (src/cmd/app.h)

**Changes**:
- Remove `render(RenderDevice&, int)` pure virtual
- Add `on_render(EngineContext const&)` with default no-op implementation
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Change `on_frame_begin()` to `on_frame_begin(EngineContext const& ctx)` — provides `ctx.frame` and `ctx.services.assets()`
- Remove `world()` accessor
- Remove `is_running()`, `set_running()`, and `running_` member — exit signalling is entirely through `ctx.request_exit()` / `ctx.is_exit_requested()`

### run_app() (src/cmd/app.cpp)

**Changes (substantial)**:
- Create `World` and `RenderSystem` before calling `app.setup()`
- Construct full `EngineContext` with all 7 fields
- Pass `EngineContext const&` to `app.setup()`
- Move `render_system.render_scene()` call to after `update_updatables()` and before `app.on_render()`
- Remove the `app.world()` check — World is always available via `ctx.world`
- Update `EngineContext` construction to include all new fields

### App-specific changes

#### 1. RunApp (src/cmd/apps/run_app.h)
- Remove `render()` override (it was empty) — `on_render()` default no-op suffices
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- No members to change — has no World/RenderSystem/Entity

#### 2. TriangleApp (src/cmd/apps/triangle_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)` — access `ctx.device` instead of separate parameter
- Replace `render(RenderDevice&, int)` with `on_render(EngineContext const&)` — access `ctx.device` for `model_.draw(ctx.device)`
- No World/RenderSystem changes

#### 3. CubeApp (src/cmd/apps/cube_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Replace `render(RenderDevice&, int)` with `on_render(EngineContext const&)`
- No World/RenderSystem changes

#### 4. MultiMaterialApp (src/cmd/apps/multi_material_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Replace `render(RenderDevice&, int)` with `on_render(EngineContext const&)`
- No World/RenderSystem changes

#### 5. CubeSceneApp (src/cmd/apps/cube_scene_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `world_`, `render_system_`, `entity_` unique_ptr members
- Store `Entity entity_;` by value
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- Remove `render_system_->render_scene()` call from `on_render()`
- Since entity transforms are updated in `on_render()`, the rotation is applied before `render_scene()` in the same frame (render_scene runs first, then on_render). **This is a behavioural concern** — see Edge Cases section.
- Move transform update logic to `on_frame_begin()` so it executes before `render_scene()`.

#### 6. TexturedCubeApp (src/cmd/apps/textured_cube_app.h/.cpp)
- Same pattern as CubeSceneApp
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `world_`, `render_system_`, `entity_` unique_ptr
- Store `Entity entity_;` by value
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- Move transform update from `on_render()` to `on_frame_begin()`

#### 7. FreeCameraApp (src/cmd/apps/free_camera_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `world_`, `render_system_`, `cube_entity_` unique_ptr
- Store `Entity cube_entity_;` by value (keep `camera_entity_` by value, already is)
- Remove override of `world()` — no longer exists
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- Override `on_frame_begin()` to perform cube animation (if any) — currently `render()` is just `render_scene()` which is automatic
- In this app, `render()` is already just `render_system_->render_scene()`, so it can become a pure default — no `on_render()` override needed at all

#### 8. PhongApp (src/cmd/apps/phong_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `world_`, `render_system_` unique_ptr
- Remove `camera_entity_`, `pointA_entity_`, `pointB_entity_` unique_ptr — store by value
- Remove override of `world()` — no longer exists
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- Move orbiting light position updates from `on_render()` to `on_frame_begin()`
- Remove `render_system_->render_scene()` call from `on_render()` — it's now automatic

#### 9. AssetDemoApp (src/cmd/apps/asset_demo_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `asset_manager_` unique_ptr — use `ctx.services.assets()`
- Remove `world_`, `render_system_`, `entity_` unique_ptr
- Store `Entity entity_;` by value
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- Move transform update from `on_render()` to `on_frame_begin()`
- In `on_frame_begin()`, call `ctx.services.assets().poll_file_events()` instead of `asset_manager_->poll_file_events()`
- Remove `render_system_->render_scene()` call from `on_render()`

#### 10. HotReloadApp (src/cmd/apps/hot_reload_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `asset_manager_` unique_ptr — use `ctx.services.assets()`
- Remove `world_`, `render_system_`, `entity_` unique_ptr
- Store `Entity entity_;` by value
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- In `on_frame_begin()`, call `ctx.services.assets().poll_file_events()` instead of `asset_manager_->poll_file_events()`
- Move texture-swap logic from `render()` (at frame 30) to `on_frame_begin()`
- Move entity rotation update from `render()` to `on_frame_begin()`
- Note: The texture swap calls `poll_file_events()` immediately after the swap. Since `on_frame_begin()` already called `poll_file_events()` at the start of the frame, the extra call in the swap is needed to trigger immediate reload. This logic must be preserved.

#### 11. GltfDemoApp (src/cmd/apps/gltf_demo_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `asset_manager_` unique_ptr — use `ctx.services.assets()`
- Remove `world_`, `render_system_`, `camera_entity_` unique_ptr
- Store `Entity camera_entity_;` by value
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- Move camera animation (Y-rotation) from `on_render()` to `on_frame_begin(ctx)` — use `ctx.frame` for orbit calculation
- Remove `render_system_->render_scene()` call
- In `setup()`, access AssetManager via `ctx.services.assets()`

#### 12. GltfHelmetApp (src/cmd/apps/gltf_helmet_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `asset_manager_` unique_ptr — use `ctx.services.assets()`
- Remove `world_`, `render_system_`, `camera_entity_` unique_ptr
- Store `Entity camera_entity_;` by value
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- Remove override of `world()` — no longer exists
- `render()` was already just `render_system_->render_scene()` — no `on_render()` override needed
- In `setup()`, access AssetManager via `ctx.services.assets()`

#### 13. HotReloadGltfApp (src/cmd/apps/hot_reload_gltf_app.h/.cpp)
- Change `setup(EngineService&)` to `setup(EngineContext const&)`
- Remove `asset_manager_` unique_ptr — store `AssetManager& asset_manager_` as a reference member, obtained from `ctx.services.assets()` during `setup()`
- Remove `frame_count_` member — use `ctx.frame` directly in `on_frame_begin(ctx)`
- Remove `world_`, `render_system_` unique_ptr
- Keep `model_entities_` as `std::vector<Entity>` by value (already correct)
- Use `ctx.world.add_entity()` instead of `Entity::create(*world_)`
- In `reload_model()`, access the AssetManager via the stored `asset_manager_` reference
- In `on_frame_begin()`, call `ctx.services.assets().poll_file_events()` instead of `asset_manager_->poll_file_events()` (or via stored reference)
- In `on_frame_begin()`, call `reload_model()` based on `ctx.frame` check
- Move camera animation from `render()` to `on_frame_begin()`
- Remove `render_system_->render_scene()` call

## Dependencies

### Modified files (engine library)

| File | Change |
|---|---|
| `src/engine/engine_context.h` | Add `RenderDevice&`, `World&`, `RenderSystem&`, `int frame` fields |
| `src/engine/scene/world.h` | Rename `create_entity()` → `add_entity()` |
| `src/engine/scene/world.cpp` | Rename implementation |
| `src/engine/scene/entity.h` | Remove `static create(World&)` |

### Modified files (app framework)

| File | Change |
|---|---|
| `src/cmd/app.h` | New `App` base class (signatures + remove `world()`) |
| `src/cmd/app.cpp` | New `run_app()`: World/RenderSystem ownership, new frame loop |

### Modified files (apps)

| File | App | Changes |
|---|---|---|
| `src/cmd/apps/run_app.h` | RunApp | New signatures, remove empty `render()` |
| `src/cmd/apps/triangle_app.h` | TriangleApp | New signatures |
| `src/cmd/apps/triangle_app.cpp` | TriangleApp | New signatures, ctx.device access |
| `src/cmd/apps/cube_app.h` | CubeApp | New signatures |
| `src/cmd/apps/cube_app.cpp` | CubeApp | New signatures, ctx.device access |
| `src/cmd/apps/cube_scene_app.h` | CubeSceneApp | Remove members, new signatures, Entity by value |
| `src/cmd/apps/cube_scene_app.cpp` | CubeSceneApp | Major restructure |
| `src/cmd/apps/textured_cube_app.h` | TexturedCubeApp | Remove members, new signatures, Entity by value |
| `src/cmd/apps/textured_cube_app.cpp` | TexturedCubeApp | Major restructure |
| `src/cmd/apps/free_camera_app.h` | FreeCameraApp | Remove members, new signatures |
| `src/cmd/apps/free_camera_app.cpp` | FreeCameraApp | Major restructure |
| `src/cmd/apps/phong_app.h` | PhongApp | Remove members, Entity by value, new signatures |
| `src/cmd/apps/phong_app.cpp` | PhongApp | Major restructure |
| `src/cmd/apps/asset_demo_app.h` | AssetDemoApp | Remove AssetManager+World+RenderSystem+Entity, new signatures |
| `src/cmd/apps/asset_demo_app.cpp` | AssetDemoApp | Major restructure |
| `src/cmd/apps/hot_reload_app.h` | HotReloadApp | Remove AssetManager+World+RenderSystem+Entity, new signatures |
| `src/cmd/apps/hot_reload_app.cpp` | HotReloadApp | Major restructure |
| `src/cmd/apps/multi_material_app.h` | MultiMaterialApp | New signatures |
| `src/cmd/apps/multi_material_app.cpp` | MultiMaterialApp | New signatures, ctx.device access |
| `src/cmd/apps/gltf_demo_app.h` | GltfDemoApp | Remove AssetManager+World+RenderSystem+Entity |
| `src/cmd/apps/gltf_demo_app.cpp` | GltfDemoApp | Major restructure |
| `src/cmd/apps/gltf_helmet_app.h` | GltfHelmetApp | Remove AssetManager+World+RenderSystem+Entity |
| `src/cmd/apps/gltf_helmet_app.cpp` | GltfHelmetApp | Major restructure |
| `src/cmd/apps/hot_reload_gltf_app.h` | HotReloadGltfApp | Remove AssetManager+World+RenderSystem |
| `src/cmd/apps/hot_reload_gltf_app.cpp` | HotReloadGltfApp | Major restructure |

## Related Documents

This spec refactors architecture established by:

- **ADR-023 (Updatable Components)**: Establishes the current `EngineContext` fields, `App::setup(EngineService&)` pattern, and `render(RenderDevice&, int)` lifecycle.
- **ADR-014 (CLI App system)**: Establishes the `App` lifecycle (`setup`/`render`/`shutdown`) and the `run_app()` pattern.

Both ADRs will need to be updated to reflect the new ownership model and lifecycle signatures.

## Documentation Updates

After implementation, the following documentation files must be updated to reflect the new ownership model:

| File | Reason |
|---|---|
| `docs/wiki/architecture/module-map.md` | Describes `EngineContext`, `App` lifecycle, `run_app()` frame loop, and app subclasses — all change |
| `docs/wiki/architecture/data-flow.md` | Describes `EngineService` lifecycle and render loop order — `render_scene()` moves to `run_app()` |
| `docs/wiki/domain/glossary.md` | Defines `Entity`, `World`, `RenderSystem`, `AssetManager` — rename `create_entity()` → `add_entity()` |
| `docs/adr/ADR-023-updatable-components.md` | Establishes current `EngineContext` fields and `App::setup(EngineService&)` — signatures change |
| `docs/adr/ADR-014-cli-app-system.md` | Establishes the `App` lifecycle and `run_app()` pattern — ownership model changes |

## Recommended migration order

Implement apps in this order to build confidence incrementally (each step verified by compilation):

1. **RunApp** — simplest, no World/RenderSystem/Entity usage. Verifies new base class signatures compile.
2. **TriangleApp** — adds minimal `on_render()` override with `ctx.device`. Verifies custom rendering works.
3. **CubeApp** — same pattern as TriangleApp. Verifies multiple independent apps migrate cleanly.
4. **MultiMaterialApp** — same pattern. Verifies no hidden assumptions in the base refactoring.
5. **CubeSceneApp** — first World-using app. Entity by value, `on_frame_begin()` for transforms. Verifies core ownership change.
6. **TexturedCubeApp** — same pattern as CubeSceneApp. Verifies the pattern is reproducible.
7. **CubeSceneApp → FreeCameraApp** — adds camera management but no custom rendering. Verifies `on_frame_begin()` works for camera updates.
8. **PhongApp** — multiple entities, orbiting lights. Verifies per-frame state updates for multiple objects.
9. **GltfHelmetApp** — simple GLTF loader with automatic scene rendering. Verifies no `on_render()` override compiles.
10. **GltfDemoApp** — GLTF with camera animation. Verifies `ctx.frame` usage in `on_frame_begin()`.
11. **HotReloadGltfApp** — AssetManager reference, model reload on frame trigger. Verifies `AssetManager&` stored from `setup()`.
12. **AssetDemoApp** — AssetManager from ctx, `poll_file_events()` in frame loop. Verifies shared AssetManager access.
13. **HotReloadApp** — most complex: texture swap, `poll_file_events()`, frame-30 timing. Verifies complete system integration.

## Backward Compatibility

The refactoring is **source-breaking** (requires changes to App subclasses) but **behaviour-preserving**:

- All 13 apps produce identical visual output
- All capture timing and file output is identical
- All frame counts, exit codes, and interactive behaviour is identical
- All hot-reload mechanics work at the same frame numbers with the same results

**What is preserved**:
- `EngineService` public API (`.assets()`, `.device()`, `.window()`, `.platform()`)
- `RenderSystem` public API (`render()`, `render_scene()`)
- `World` public API (except `create_entity` → `add_entity` rename)
- `Entity` handle semantics (16-byte handle, copyable)
- `App::config()`, `App::shutdown()` signatures (unchanged)
- `run_app()` return value behaviour
- Backend selection (SDL3 vs Headless)
- Capture pipeline and frame limiting

## Edge Cases

### 1. Frame ordering: transform updates must happen before render_scene

**Problem**: In the old model, apps updated entity transforms inside `render()` and then called `render_system_->render_scene()` at the end. In the new model, `render_scene()` is called by `run_app()` before `on_render()`. This means if an app updates transforms in `on_render()`, the scene renders with stale transforms.

**Solution**: Apps that update per-frame state (entity transforms, camera positions, light positions) must move those updates to `on_frame_begin()`, which is called before `render_scene()`.

**Affected apps**: All World-using apps with per-frame animation:
- `CubeSceneApp` — entity rotation (move to `on_frame_begin()`)
- `TexturedCubeApp` — entity rotation (move to `on_frame_begin()`)
- `PhongApp` — orbiting light positions (move to `on_frame_begin()`)
- `GltfDemoApp` — camera orbit (move to `on_frame_begin()`)
- `HotReloadApp` — entity rotation + texture swap (move to `on_frame_begin()`)
- `HotReloadGltfApp` — camera animation (move to `on_frame_begin()`)
- `FreeCameraApp` — no per-frame animation, `render()` was only `render_scene()`, so no `on_render()` needed

### 2. AssetManager shared state: poll_file_events() and clear()

**Problem**: Previously, each app had its own AssetManager. Now they share the EngineService's AssetManager. If two apps could run in the same process (they don't currently, but theoretically), they'd share asset state. More practically, the shared AssetManager's `poll_file_events()` and `clear()` are now called on the shared instance.

**Solution**: Since only one app runs at a time, this is safe. Apps that call `poll_file_events()` or `clear()` do so via `ctx.services.assets()`. The EngineService's AssetManager must stay alive for the full lifetime of the app (which it does — EngineService outlives the app).

### 3. Entity validity after World ownership moves

**Risk**: Entity handles store a `World*` pointer internally. If World ownership moves to `run_app()` and is allocated differently, existing Entity handles remain valid as long as the World is not moved or destroyed while handles reference it.

**Mitigation**: `World` is non-movable (deleted move constructor/assignment). It is owned by `run_app()` via `std::unique_ptr<World>`. The World is destroyed after `app.shutdown()` completes. Since no Entity handle outlives the World (they're all destroyed or go out of scope in `shutdown()`), this is safe.

### 4. Apps that render_scene() directly without World/RenderSystem

**Risk**: `RunApp`, `TriangleApp`, `CubeApp`, `MultiMaterialApp` don't use World/RenderSystem. Calling `render_scene()` on an empty World is harmless (no-op). The apps don't override `on_render()` (or do so only to call `model_.draw(device)`), so the frame loop works correctly.

### 5. HotReloadApp frame-30 texture swap timing

**Risk**: In the old model, `render()` was called once per frame, and the texture swap + `poll_file_events()` happened at the start of frame 30's render. In the new model, `on_frame_begin()` runs before `render_scene()`, so the swap/poll must happen in `on_frame_begin()` to have the reloaded texture visible in the same frame's render.

**Solution**: Move the frame-30 texture swap logic from `render()` to `on_frame_begin()`. The extra `poll_file_events()` call after the swap (for immediate reload) remains in `on_frame_begin()`. This ensures the reloaded texture is available for the same frame's `render_scene()`.

**Verification**: The before/after captures at frames 30 and 60 must match exactly.

### 6. GltfDemoApp camera animation uses `ctx.frame`

**Risk**: `GltfDemoApp::render()` uses the `frame` parameter to calculate the orbit position. In the new model, this moves to `on_frame_begin(ctx)` where `ctx.frame` is available directly.

**Solution**: `on_frame_begin(ctx)` receives `EngineContext const&` which includes `ctx.frame`. Use `ctx.frame` directly for the orbit animation calculation in `on_frame_begin(ctx)`, which runs before `render_scene()`. This ensures the camera position is correct for the current frame's scene render.

## Error cases

| Case | Expected behaviour |
|---|---|---|
| App that previously used `world()` override no longer has it | Compilation error at all override sites |
| App that creates `AssetManager::create()` still compiles | Must be migrated to using `ctx.services.assets()` |
| App that uses `Entity::create()` still compiles | `Entity::create()` is removed — compilation error |
| App that overrides `render()` with new `on_render()` | Compiles only if signature matches `on_render(EngineContext const&)` |
| App that doesn't use World but `RenderSystem` is created | Empty world, no-op render_scene — harmless |
| `run_app()` with app that throws in `setup()` after World is created | `shutdown()` is called; World is destroyed when `run_app()` returns (unique_ptr destructor) |
| `render_system.render_scene()` fails (e.g., missing camera) | Currently `render_scene()` logs a warning internally and continues. If it throws (unexpected exception), the exception propagates and `run_app()` exits (existing behaviour preserved). The spec does not add new error handling for this path. |
| Frame loop breaks before `device.begin_frame()` (e.g., `poll_events()` returns false or exit is requested before `begin_frame`) | `device.end_frame()` is NOT called because `begin_frame()` was never invoked. This is safe — the device API requires `end_frame()` only after a successful `begin_frame()`. All early-exit paths occur before `begin_frame()` in the loop. |

## Permissions and security

No changes to the security or permission model. This is a pure refactoring of internal ownership. The `EngineContext` is a stack-allocated struct of references passed through the call stack — no new persistence or external access is introduced.

## Observability

No changes to logging or observability. The same `BUDDD_LOG_INFO`, `BUDDD_LOG_ERROR`, and `BUDDD_LOG_TAG` macros are used. Lifecycle messages remain identical.

## Out of scope

- Changes to `RenderSystem` rendering pipeline
- Changes to World internals (entity storage, component dispatch, hierarchy)
- Changes to AssetManager caching, hot-reload, or file-watching mechanics
- Adding or removing apps
- Changing the CMake build system or adding new dependencies
- Refactoring the test suite or adding new tests beyond verification
- Changes to `EngineService` creation or destruction order
- Changes to the CLI argument parsing or command dispatch

## Assumptions

1. World and RenderSystem are cheap to construct empty, so `run_app()` always creates them regardless of app type. If this becomes a performance concern, a `needs_world()` flag can be added later.
2. The `RenderSystem` is non-owning (stores pointers to `RenderDevice&` and `World&`), so it can be safely destroyed when the frame loop ends, before the World and RenderDevice are destroyed.
3. Entity handles are stable across frames as long as the World is alive and the entity is not destroyed. Since World ownership moves to `run_app()`, this is safe.
4. `on_frame_begin(EngineContext const& ctx)` is the correct hook for per-frame state updates. Apps can override it to update transforms, animations, or other state that must be current before `render_scene()`. `ctx.frame` provides the current frame number for animation calculations, and `ctx.services.assets()` provides access to the shared AssetManager for hot-reload polling.
6. The `AssetManager::create()` factory is not called by any code outside the apps (no internal engine code creates AssetManagers). This assumption must be verified during implementation.
7. The `RenderSystem::render_scene()` method is safe to call on an empty World (no entities, no camera). It should either render nothing or produce a cleared framebuffer.

## Resolved design decisions

All prior open questions have been resolved by human decision:

1. **`App::shutdown()` signature**: Kept without parameters (no `EngineContext`). No app needs context during shutdown. Simpler.

2. **`run_app()` namespace**: Stays in `buddd::cmd`. It is a CLI framework detail; moving would break callers.

3. **HotReloadGltfApp frame counter**: Uses `ctx.frame` directly from `on_frame_begin(ctx)`. Removes the `frame_count_` member. `ctx.frame` is the canonical source.

4. **`setup()` failure handling**: `shutdown()` is called systematically even if `setup()` fails after World/RenderSystem creation. World/RenderSystem exist during `shutdown()`, destroyed via `unique_ptr` after. This is the current pattern — preserved.

5. **HotReloadGltfApp::reload_model() AssetManager access**: Stores `AssetManager&` as a reference member, obtained from `ctx.services.assets()` in `setup()`. Non-owning, safe because EngineService outlives the app.
