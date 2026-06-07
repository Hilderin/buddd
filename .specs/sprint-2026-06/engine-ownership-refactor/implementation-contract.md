# IMPL-019-REFACTOR - Engine Ownership Model & App Lifecycle Refactoring

## Source spec

`.specs/sprint-2026-06/engine-ownership-refactor/spec.md`

## Goal

Refactor the engine's ownership model and App lifecycle: move Entity creation to `World::add_entity()`, remove `std::unique_ptr<Entity>` (store by value), consolidate AssetManager ownership into `EngineService`, extend `EngineContext` with all per-frame references (`RenderDevice&`, `World&`, `RenderSystem&`, `int frame`), replace `App::render(RenderDevice&, int)` with `App::on_render(EngineContext const&)` (default no-op), move `render_scene()` into `run_app()` before `on_render()`, remove `App::world()`, `App::is_running()`, `App::set_running()`, and `App::running_` (exit signalling via `ctx.request_exit()` only), and migrate all 13 apps with zero visual behavioural change.

## Non-goals

- No changes to `RenderSystem` public API (other than ownership transfer)
- No changes to `World` component system, entity hierarchy, or `Updatable` dispatch logic
- No changes to `EngineService` creation or `AssetManager` public API
- No changes to the capture pipeline, frame limiting, or exit code logic
- No apps are removed, renamed, or added
- No changes to the CMake build system or new dependencies
- No changes to test infrastructure (existing tests must pass without modification)
- No changes to `src/cmd/main.cpp` (dispatch logic unchanged)

## Relevant ADRs

- **ADR-014** (CLI App System) — Establishes the App lifecycle (`setup`/`render`/`shutdown`) and `run_app()` pattern. The ownership model changes but the lifecycle contract is preserved. Must be updated after implementation.
- **ADR-023** (Updatable Components) — Establishes current `EngineContext` fields and `App::setup(EngineService&)` pattern. Signatures change. Must be updated after implementation.
- **ADR-011** (Ownership/Nullability/Lifetime) — Raw pointers as private data members are acceptable for non-owning observers. Applied for `HotReloadGltfApp::asset_manager_` (non-owning `AssetManager*`).

## Files to inspect

- `src/engine/engine_context.h` — Current fields (EngineService&, Window&, float delta_time, exit_requested)
- `src/engine/scene/entity.h` — Current `static create(World&)` factory + private constructor
- `src/engine/scene/entity.cpp` — Current `Entity::create()` implementation
- `src/engine/scene/world.h` — Current `create_entity()` declaration
- `src/engine/scene/world.cpp` — Current `create_entity()` implementation and all internal callers
- `src/cmd/app.h` — Current App base class: setup(EngineService&), on_frame_begin(), render(RenderDevice&, int), shutdown(), world(), is_running(), set_running(), running_
- `src/cmd/app.cpp` — Current run_app() frame loop
- `src/engine/engine_service.h` — Confirm `assets()` accessor exists (line 27)
- `src/engine/render/render_system.h` — Constructor signature `RenderSystem(RenderDevice&, World&)`
- All 13 app headers and source files in `src/cmd/apps/` (listed below in Files allowed to change)

## Files allowed to change

Precise list of files ordered by change group:

### Engine layer (5 files)
1. `src/engine/engine_context.h`
2. `src/engine/scene/entity.h`
3. `src/engine/scene/entity.cpp`
4. `src/engine/scene/world.h`
5. `src/engine/scene/world.cpp`

### App framework (2 files)
6. `src/cmd/app.h`
7. `src/cmd/app.cpp`

### App subclasses (26 files: 13 header+source pairs)
8. `src/cmd/apps/run_app.h`
9. `src/cmd/apps/run_app.cpp`
10. `src/cmd/apps/triangle_app.h`
11. `src/cmd/apps/triangle_app.cpp`
12. `src/cmd/apps/cube_app.h`
13. `src/cmd/apps/cube_app.cpp`
14. `src/cmd/apps/multi_material_app.h`
15. `src/cmd/apps/multi_material_app.cpp`
16. `src/cmd/apps/cube_scene_app.h`
17. `src/cmd/apps/cube_scene_app.cpp`
18. `src/cmd/apps/textured_cube_app.h`
19. `src/cmd/apps/textured_cube_app.cpp`
20. `src/cmd/apps/free_camera_app.h`
21. `src/cmd/apps/free_camera_app.cpp`
22. `src/cmd/apps/phong_app.h`
23. `src/cmd/apps/phong_app.cpp`
24. `src/cmd/apps/asset_demo_app.h`
25. `src/cmd/apps/asset_demo_app.cpp`
26. `src/cmd/apps/hot_reload_app.h`
27. `src/cmd/apps/hot_reload_app.cpp`
28. `src/cmd/apps/gltf_demo_app.h`
29. `src/cmd/apps/gltf_demo_app.cpp`
30. `src/cmd/apps/gltf_helmet_app.h`
31. `src/cmd/apps/gltf_helmet_app.cpp`
32. `src/cmd/apps/hot_reload_gltf_app.h`
33. `src/cmd/apps/hot_reload_gltf_app.cpp`

## Files forbidden to change

- `src/engine/engine_service.h` — No changes needed; `assets()` accessor already exists
- `src/engine/engine_service.cpp` — No changes needed
- `src/cmd/main.cpp` — No changes needed; dispatch logic is unaffected
- `src/cmd/app_config.h` — No changes needed
- `src/cmd/app_config.cpp` — No changes needed
- `src/engine/scene/entity_id.h` — No changes needed
- `src/engine/scene/updatable.h` — No changes needed
- `src/engine/render/render_system.cpp` — No changes needed (render_scene() API unchanged)
- All files under `tests/`, `docs/`, `assets/` — No changes in this contract (documentation updates are handled by wiki-agent)
- Any file not listed in "Files allowed to change"

## Existing conventions to follow

- Forward declarations at top of headers instead of `#include` where possible
- `buddd::engine::` and `buddd::cmd::` namespace nesting conventions
- `be = buddd::engine` alias in .cpp files where already used
- `BUDDD_LOG_TAG()` macro at top of .cpp files where already used
- `Result<T>` / `Error` error handling pattern (`std::expected`)
- `std::unique_ptr<T>` for owning pointers, raw pointers for non-owning observers (ADR-011)
- `auto` return types with trailing return type syntax
- `[[nodiscard]]` where result must not be discarded
- `noexcept` on move constructors and trivial operations
- Member variable naming: trailing underscore (e.g., `entity_`)
- `EngineContext` fields are references or value types, passed as `const&`

## Required implementation behavior

### 1. EngineContext (`src/engine/engine_context.h`)

**Add forward declarations** before the `EngineContext` struct:
```cpp
class RenderDevice;
class World;
class RenderSystem;
```

**Add four new fields** after `Window& window` and before `float delta_time`:
```cpp
RenderDevice& device;
World& world;
RenderSystem& render_system;
int frame;           // 0-based, incremented by run_app()
```

The final field order MUST be: `services`, `window`, `device`, `world`, `render_system`, `delta_time`, `frame`.

Keep `request_exit()`, `is_exit_requested()`, and `mutable bool exit_requested_` unchanged.

### 2. World rename (`src/engine/scene/world.h` + `src/engine/scene/world.cpp`)

**world.h line 33**: Replace `auto create_entity() -> Entity;` with `auto add_entity() -> Entity;`

**world.cpp line 29**: Replace `World::create_entity()` with `World::add_entity()`. Keep the entire implementation body identical — only the function name changes.

### 3. Entity remove static factory (`src/engine/scene/entity.h` + `src/engine/scene/entity.cpp`)

**entity.h line 64**: Remove the line `static auto create(World& world) -> Entity;`

**entity.cpp lines 55-57**: Remove the entire `Entity::create()` function body:
```cpp
auto Entity::create(World& world) -> Entity {
    return world.create_entity();
}
```
Keep the `Entity(World& world, EntityId id) noexcept` private constructor (line 7-10 of entity.cpp).

### 4. App base class (`src/cmd/app.h`)

**Remove includes/forward-decls**: Remove the forward declaration of `RenderDevice` (world is no longer needed as a forward). Add forward declarations for `EngineContext`.

**Change `setup()` signature** (line 31): From `auto setup(buddd::engine::EngineService& engine) -> buddd::engine::Result<void>` to `auto setup(buddd::engine::EngineContext const& ctx) -> buddd::engine::Result<void>`.

**Change `on_frame_begin()` signature** (line 36): From `auto on_frame_begin() -> void` to `auto on_frame_begin(buddd::engine::EngineContext const& ctx) -> void`.

**Replace `render()`** (lines 38-40): Remove the `render(buddd::engine::RenderDevice&, int)` pure virtual. Add after `on_frame_begin()`:
```cpp
/// Called once per frame after render_scene().
/// Replaces render(RenderDevice&, int). Default no-op.
virtual auto on_render(buddd::engine::EngineContext const& ctx) -> void {}
```

**Remove world() accessor** (line 50): Remove the entire `world()` declaration.

**Remove is_running()/set_running()** (lines 46-53): Remove both methods and the `running_` member.

**Update class comment**: Change `/// Lifecycle: config() -> setup() -> render() x N -> shutdown().` to `/// Lifecycle: config() -> setup() -> on_frame_begin() x N -> on_render() x N -> shutdown().`

**Update run_app() comment** (lines 61-72): Remove references to `app.world()` and `app.set_running()`. Document the new frame loop order.

### 5. run_app() frame loop (`src/cmd/app.cpp`)

**Add includes**: Ensure `#include "scene/world.h"` and `#include "render/render_system.h"` are present.

**Remove unused includes**: `#include "scene/updatable.h"` may still be needed by world.h, keep it.

**Change the body of `run_app()`** completely. The new flow is:

```cpp
auto buddd::cmd::run_app(App& app, const RunningArgs& args) -> int {
    // 1. Get AppConfig
    auto cfg = app.config();

    // 2. Create EngineService
    auto engine = be::EngineService::create(k_app_backend, {cfg.title, cfg.width, cfg.height});
    if (!engine) {
        BUDDD_LOG_ERROR("FATAL: {}", be::to_string(engine.error()));
        return EXIT_FAILURE;
    }
    auto& eng = **engine;

    BUDDD_LOG_INFO("Window opened: {}x{}", eng.window().width(), eng.window().height());

    // 3. Create World + RenderSystem (always, unconditionally)
    auto world = std::make_unique<be::World>();
    auto render_system = std::make_unique<be::RenderSystem>(eng.device(), *world);

    // 4. Setup app
    be::EngineContext setup_ctx{
        eng, eng.window(), eng.device(), *world, *render_system,
        eng.platform().delta_time(), 0
    };
    auto setup_result = app.setup(setup_ctx);
    if (!setup_result) {
        BUDDD_LOG_ERROR("{}", be::to_string(setup_result.error()));
        app.shutdown();
        return EXIT_FAILURE;
    }

    // 5. Print start message
    bool has_limit = args.frame_limit > 0;
    if (has_limit) {
        BUDDD_LOG_INFO("Scene started: {} ({} frames)", cfg.title, args.frame_limit);
    } else {
        BUDDD_LOG_INFO("Scene started: {} (interactive)", cfg.title);
    }

    // 6. Render loop
    bool any_capture_success = false;
    bool any_capture_failure = false;
    int frame = 0;
    bool aborted_by_user = false;

    while (true) {
        // Frame limit check
        if (has_limit && frame >= args.frame_limit)
            break;

        // Event polling
        if (!eng.platform().poll_events()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user");
            break;
        }

        // Begin frame
        eng.device().begin_frame();

        // Construct per-frame EngineContext
        be::EngineContext ctx{
            eng, eng.window(), eng.device(), *world, *render_system,
            eng.platform().delta_time(), frame
        };

        // Frame start hook (hot-reload polling, transform updates, etc.)
        app.on_frame_begin(ctx);

        // Exit check after on_frame_begin
        if (ctx.is_exit_requested()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user (frame {})", frame + 1);
            eng.device().end_frame();  // Must end frame before breaking
            break;
        }

        // ── Updatable auto-dispatch ──
        world->update_updatables(ctx);
        if (ctx.is_exit_requested()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user (frame {})", frame + 1);
            eng.device().end_frame();
            break;
        }

        // ── Automatic scene render ──
        render_system->render_scene();

        // ── Custom rendering (optional, default no-op) ──
        app.on_render(ctx);

        // Capture: read_pixels BEFORE end_frame()
        bool did_read_pixels = false;
        be::Result<be::ImageBuffer> pixel_buffer =
            be::make_error(be::Error::Category::Unknown, "no capture needed");

        for (const auto& spec : args.captures) {
            int effective_frame = spec.effective_frame();
            if (effective_frame == frame + 1) {
                if (!did_read_pixels) {
                    pixel_buffer = eng.device().read_pixels();
                    did_read_pixels = true;
                }
                if (pixel_buffer) {
                    auto image = be::Image::create(*pixel_buffer);
                    if (image) {
                        auto save_result = image->save(spec.path);
                        if (save_result) {
                            any_capture_success = true;
                            BUDDD_LOG_INFO("Captured: {}", spec.path);
                        } else {
                            any_capture_failure = true;
                            BUDDD_LOG_ERROR("{}", be::to_string(save_result.error()));
                        }
                    } else {
                        any_capture_failure = true;
                        BUDDD_LOG_ERROR("{}", be::to_string(image.error()));
                    }
                } else {
                    any_capture_failure = true;
                    BUDDD_LOG_ERROR("{}", be::to_string(pixel_buffer.error()));
                }
            }
        }

        // End frame
        eng.device().end_frame();

        ++frame;
    }

    // 7. Print completion or abort
    if (!aborted_by_user) {
        BUDDD_LOG_INFO("Scene complete: {} ({} frames rendered)", cfg.title, frame);
    }

    // 8. Shutdown
    app.shutdown();

    BUDDD_LOG_INFO("Window closed, shutting down.");

    // 9. Exit code based on capture success
    bool has_captures = !args.captures.empty();
    if (has_captures && !any_capture_success && any_capture_failure)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
```

Key structural rules for the new loop:
- World and RenderSystem are created unconditionally as `std::unique_ptr` (non-movable, stored on heap)
- `EngineContext` is constructed with all 7 fields once per frame before `on_frame_begin()`
- `EngineContext` is also constructed once before `setup()` for the initialisation call (with `frame = 0`)
- `on_frame_begin(ctx)` is called before `update_updatables(ctx)` — frame start hook
- After `on_frame_begin()`, check `ctx.is_exit_requested()` and break if true (must call `end_frame()` first)
- `update_updatables()` is called with the complete `EngineContext` (provides all services, window, device, delta_time)
- After `update_updatables()`, check `ctx.is_exit_requested()` again
- `render_scene()` is called BEFORE `on_render()`
- `app.on_render(ctx)` replaces `app.render(device, frame)` — receives the full context
- `app.setup()` receives an EngineContext — synchronous exit signalling available during setup if needed
- Exit check: only `ctx.is_exit_requested()`. The old `app.is_running()`/`app.set_running()`/`app.running_` are removed.

### 6. Per-app migration rules

#### 6.1 RunApp (`src/cmd/apps/run_app.h`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)` (signature change, body remains empty `return {};`)
- Remove `render(RenderDevice&, int)` override entirely — `on_render()` default no-op suffices

The entire class body after changes should look like:
```cpp
class RunApp : public App {
public:
    auto config() const -> AppConfig override {
        return {};
    }
    [[nodiscard]] auto setup(buddd::engine::EngineContext const&) -> buddd::engine::Result<void> override {
        return {};
    }
};
```

**Source file** (`run_app.cpp`): No changes needed (already empty).

#### 6.2 TriangleApp (`src/cmd/apps/triangle_app.h` + `.cpp`)

**Header:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- Remove `render(RenderDevice&, int)` — add `on_render(EngineContext const& ctx)`
- Remove forward declaration of `EngineService` (no longer needed)
- Add forward declaration of `EngineContext` if needed (via `app.h` which includes engine_context.h transitively — verify)

**Source** (`triangle_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Change `engine.device()` to `ctx.device` inside setup
- `render(RenderDevice& device, int)` → `on_render(EngineContext const& ctx)`
- Use `ctx.device` instead of the separate `device` parameter in on_render

#### 6.3 CubeApp (`src/cmd/apps/cube_app.h` + `.cpp`)

Same pattern as TriangleApp (section 6.2).

#### 6.4 MultiMaterialApp (`src/cmd/apps/multi_material_app.h` + `.cpp`)

Same pattern as TriangleApp (section 6.2).

#### 6.5 CubeSceneApp (`src/cmd/apps/cube_scene_app.h` + `.cpp`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- `render(RenderDevice&, int)` → `on_render(EngineContext const& ctx)` (no-op body)
- Add `on_frame_begin(EngineContext const& ctx)` for transform rotation
- **Remove members**: `world_`, `render_system_`, `entity_` (all unique_ptr)
- **Add member**: `Entity entity_;` (by value)
- Remove includes for `scene/entity.h`, `scene/world.h`, `render/render_system.h` if no longer needed (keep what header-only types require)
- Keep `#include <chrono>` (start_time_ stays and requires `<chrono>`)
- **Do NOT remove `#include <chrono>`** — `start_time_` is `std::chrono::steady_clock::time_point` which requires `<chrono>`. No transitive include provides it.
- Remove `#include <memory>` (no more unique_ptr members)

**Source changes** (`cube_scene_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Remove `world_ = std::make_unique<be::World>();` — world is owned by run_app()
- `be::Entity::create(*world_)` → `ctx.world.add_entity()`
- Use `ctx.device` instead of `engine.device()`
- Remove `render_system_ = std::make_unique<be::RenderSystem>(device, *world_);` — render_system is owned by run_app()
- `entity_ = std::make_unique<be::Entity>(std::move(entity));` → `entity_ = entity;` (Entity is 16-byte handle, copyable)
- **New**: `on_frame_begin(EngineContext const& ctx)` — move the transform rotation from old `render()`:
  ```cpp
  auto CubeSceneApp::on_frame_begin(EngineContext const& ctx) -> void {
      auto elapsed = std::chrono::steady_clock::now() - start_time_;
      float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
      float angle = elapsed_seconds * 0.5f;
      entity_.transform().rotation =
          be::math::Quat::angle_axis(angle, be::math::Vec3::unit_y());
  }
  ```
- `render()` → `on_render(EngineContext const&)` with empty body (just `{}`) — `render_scene()` is now automatic
- Remove `#include "render/render_system.h"` if no longer needed

#### 6.6 TexturedCubeApp (`src/cmd/apps/textured_cube_app.h` + `.cpp`)

Same changes as CubeSceneApp (section 6.5):
- Setup signature change, remove owned members, Entity by value
- `Entity::create(*world_)` → `ctx.world.add_entity()`
- Move transform rotation to `on_frame_begin(ctx)` 
- `on_render()` is no-op (just `{}`)
- **Do NOT remove `#include <chrono>`** — `start_time_` requires it (same as CubeSceneApp)

#### 6.7 FreeCameraApp (`src/cmd/apps/free_camera_app.h` + `.cpp`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- `render(RenderDevice&, int)` → remove entirely (render was just render_scene, now automatic)
- Remove `world()` override
- **Remove members**: `world_`, `render_system_`, `cube_entity_` (unique_ptr)
- **Change** `cube_entity_` from `std::unique_ptr<be::Entity>` to `be::Entity cube_entity_;` (by value)
- Keep `camera_entity_` as value (already is)
- Remove `#include <memory>` if no longer needed
- Remove `#include "scene/world.h"` if no longer needed

**Source changes** (`free_camera_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Remove `world_ = std::make_unique<be::World>();`
- `be::Entity::create(*world_)` → `ctx.world.add_entity()` (for camera_entity_ and cube_entity)
- Use `ctx.device` instead of `engine.device()`
- Remove `render_system_ = std::make_unique<be::RenderSystem>(device, *world_);`
- `cube_entity_ = std::make_unique<be::Entity>(std::move(cube_entity));` → `cube_entity_ = cube_entity;`
- Remove entire `render()` method body — no `on_render()` override, no `on_frame_begin()` override (camera auto-updated by Updatable system)

#### 6.8 PhongApp (`src/cmd/apps/phong_app.h` + `.cpp`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- `render(RenderDevice&, int)` → `on_render(EngineContext const& ctx)` (no-op body)
- Add `on_frame_begin(EngineContext const& ctx)` for orbiting light updates
- Remove `world()` override
- **Remove members**: `world_`, `render_system_` (unique_ptr)
- **Change** `camera_entity_`, `pointA_entity_`, `pointB_entity_` from `std::unique_ptr<be::Entity>` to `be::Entity` (by value)
- Remove `#include <memory>` if no longer needed

**Source changes** (`phong_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Remove `world_ = std::make_unique<be::World>();`
- All `be::Entity::create(*world_)` → `ctx.world.add_entity()`
- All `std::make_unique<be::Entity>(be::Entity::create(*world_))` → `ctx.world.add_entity()` directly assign to value member
- All `pointA_entity_->...` → `pointA_entity_.` (value semantics)
- All `pointB_entity_->...` → `pointB_entity_.` (value semantics)
- All `camera_entity_->...` → `camera_entity_.` (value semantics)
- Use `ctx.device` instead of `engine.device()`
- Remove `render_system_ = std::make_unique<be::RenderSystem>(device, *world_);`
- **New** `on_frame_begin(EngineContext const& ctx)` — move orbiting light position updates from old `render()`:
  ```cpp
  auto PhongApp::on_frame_begin(EngineContext const& ctx) -> void {
      auto now = std::chrono::steady_clock::now();
      float elapsed = std::chrono::duration<float>(now - start_time_).count();
      float t = elapsed;
      float orbit_r = 6.0f;
      float orbit_y = 2.5f;
      pointA_entity_.transform().position = be::math::Vec3{
          orbit_r * std::cos(t * 0.8f),
          orbit_y + 0.8f * std::sin(t * 1.2f),
          orbit_r * std::sin(t * 0.8f)
      };
      pointB_entity_.transform().position = be::math::Vec3{
          orbit_r * 0.7f * std::cos(t * 0.6f + 1.57f),
          orbit_y - 0.5f + 1.2f * std::sin(t * 0.9f + 0.5f),
          orbit_r * 0.7f * std::sin(t * 0.6f + 1.57f)
      };
  }
  ```
- `render()` → `on_render(EngineContext const&)` with empty body (render_scene is automatic)
- Remove `#include "render/render_system.h"` if no longer needed

#### 6.9 AssetDemoApp (`src/cmd/apps/asset_demo_app.h` + `.cpp`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- `render(RenderDevice&, int)` → `on_render(EngineContext const& ctx)` (no-op body)
- `on_frame_begin()` → `on_frame_begin(EngineContext const& ctx)`
- **Remove members**: `asset_manager_`, `world_`, `render_system_`, `entity_` (all unique_ptr)
- **Add member**: `Entity entity_;` (by value)
- Remove forward declaration of `AssetManager`
- Remove `#include <memory>`

**Source changes** (`asset_demo_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Use `ctx.device` instead of `engine.device()`
- **Remove** `auto am = be::AssetManager::create(device, "assets");` block entirely
- **Remove** `asset_manager_ = std::move(*am);`
- `be::AssetManager::create(...)` is not called — use shared instance via `ctx.services.assets()`
- Change material loading: `asset_manager_->create<be::MaterialAsset>(...)` → `ctx.services.assets().create<be::MaterialAsset>(...)`
- Remove `world_ = std::make_unique<be::World>();`
- `be::Entity::create(*world_)` → `ctx.world.add_entity()`
- Use `ctx.device` for device access
- Remove `render_system_ = std::make_unique<be::RenderSystem>(device, *world_);`
- `entity_ = std::make_unique<be::Entity>(std::move(entity));` → `entity_ = entity;`
- `on_frame_begin()` → `on_frame_begin(EngineContext const& ctx)`:
  ```cpp
  void on_frame_begin(EngineContext const& ctx) {
      ctx.services.assets().poll_file_events();
  }
  ```
  (Old body was `asset_manager_->poll_file_events();`)
- **New** `on_frame_begin(ctx)` — move transform rotation from old `render()`:
  ```cpp
  auto elapsed = std::chrono::steady_clock::now() - start_time_;
  float elapsed_seconds = std::chrono::duration<float>(elapsed).count();
  float angle = elapsed_seconds * 0.5f;
  entity_.transform().rotation =
      be::math::Quat::angle_axis(angle, be::math::Vec3::unit_y());
  ```
- Keep `on_frame_begin()` and merge both responsibilities (poll + rotation update) in one override
- `render()` → `on_render(EngineContext const&)` with empty body (render_scene automatic)

#### 6.10 HotReloadApp (`src/cmd/apps/hot_reload_app.h` + `.cpp`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- `render(RenderDevice&, int)` → `on_render(EngineContext const& ctx)` (no-op body)
- `on_frame_begin()` → `on_frame_begin(EngineContext const& ctx)`
- **Remove members**: `asset_manager_`, `world_`, `render_system_`, `entity_` (all unique_ptr)
- **Add member**: `Entity entity_;` (by value)
- Remove forward declaration of `AssetManager`
- Remove `#include <memory>`

**Source changes** (`hot_reload_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Use `ctx.device` instead of `engine.device()`
- **Remove** `auto am = be::AssetManager::create(device, "assets");` block
- **Remove** `asset_manager_` from setup entirely
- Change material loading: `(*am)->create<be::MaterialAsset>(...)` → `ctx.services.assets().create<be::MaterialAsset>(...)`
- Remove `asset_manager_ = std::move(*am);`
- Remove `world_ = std::make_unique<be::World>();`
- `be::Entity::create(*world_)` → `ctx.world.add_entity()`
- Remove `render_system_ = std::make_unique<be::RenderSystem>(device, *world_);`
- `entity_ = std::make_unique<be::Entity>(std::move(entity));` → `entity_ = entity;`
- `on_frame_begin()` → `on_frame_begin(EngineContext const& ctx)`:
  - Change `asset_manager_->poll_file_events()` to `ctx.services.assets().poll_file_events()`
- **Move from old `render()` to `on_frame_begin(ctx)`**:
  - Frame-30 texture swap logic (at `ctx.frame == 30`):
    ```cpp
    if (ctx.frame == 30) {
        BUDDD_LOG_INFO("Frame 30: swapping texture...");
        std::filesystem::copy(
            "assets/textures/hot_reload_b.png",
            "assets/textures/hot_reload_live.png",
            std::filesystem::copy_options::overwrite_existing);
        ctx.services.assets().poll_file_events();  // immediate reload
        BUDDD_LOG_INFO("Texture swapped and poll_file_events() called.");
    }
    ```
  - Entity rotation update (using `ctx.frame` for angle):
    ```cpp
    float angle_deg = static_cast<float>(ctx.frame) * 3.0f;
    float angle_rad = be::math::radians(angle_deg);
    entity_.transform().rotation = be::math::Quat::angle_axis(angle_rad, be::math::Vec3::unit_y());
    ```
- `render()` → `on_render(EngineContext const& ctx)` with empty body

#### 6.11 GltfDemoApp (`src/cmd/apps/gltf_demo_app.h` + `.cpp`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- `render(RenderDevice&, int)` → `on_render(EngineContext const& ctx)` (no-op body)
- Add `on_frame_begin(EngineContext const& ctx)` for camera animation
- **Remove members**: `asset_manager_`, `world_`, `camera_entity_`, `render_system_` (all unique_ptr)
- **Change** `camera_entity_` from `std::unique_ptr<be::Entity>` to `be::Entity` (by value)
- Remove forward declarations of `EngineService`, `RenderDevice`, `World`, `RenderSystem`, `Entity`, `AssetManager`
- Remove `#include <memory>`

**Source changes** (`gltf_demo_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Use `ctx.device` instead of `engine.device()`
- **Remove** `auto am_result = be::AssetManager::create(device, base_path);` block
- **Remove** `asset_manager_` entirely
- Change model loading: `asset_manager_->create<be::ModelAsset>(...)` → `ctx.services.assets().create<be::ModelAsset>(...)`
- Remove `world_ = std::make_unique<be::World>();`
- All `be::Entity::create(*world_)` → `ctx.world.add_entity()`
- `camera_entity_ = std::make_unique<be::Entity>(be::Entity::create(*world_));` → `camera_entity_ = ctx.world.add_entity();`
- `be::add_model_to_world(*world_, root)` → `be::add_model_to_world(ctx.world, root)` (uses ctx.world)
- Remove `render_system_ = std::make_unique<be::RenderSystem>(device, *world_);`
- **New** `on_frame_begin(EngineContext const& ctx)` — move camera Y-rotation orbit from old `render()`:
  ```cpp
  auto GltfDemoApp::on_frame_begin(EngineContext const& ctx) -> void {
      auto& cam = camera_entity_.get_component<be::CameraComponent>()->camera();
      float angle = static_cast<float>(ctx.frame) * 0.02f;
      cam.set_position(be::math::Vec3{
          3.0f * std::sin(angle),
          1.0f,
          3.0f * std::cos(angle)
      });
      cam.look_at(be::math::Vec3{0.0f, 0.0f, 0.0f});
  }
  ```
- `render()` → `on_render(EngineContext const&)` with empty body

#### 6.12 GltfHelmetApp (`src/cmd/apps/gltf_helmet_app.h` + `.cpp`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- `render(RenderDevice&, int)` → remove entirely (render was just render_scene, now automatic)
- Remove `world()` override
- **Remove members**: `asset_manager_`, `world_`, `camera_entity_`, `render_system_` (all unique_ptr)
- **Change** `camera_entity_` from `std::unique_ptr<be::Entity>` to `be::Entity` (by value)
- Remove forward declarations of `EngineService`, `RenderDevice`, `World`, `RenderSystem`, `Entity`, `AssetManager`
- Remove `#include <memory>`

**Source changes** (`gltf_helmet_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Use `ctx.device` instead of `engine.device()`
- **Remove** `auto am_result = be::AssetManager::create(device, base_path);` block
- **Remove** `asset_manager_` entirely
- Change model loading: `asset_manager_->create<be::ModelAsset>(...)` → `ctx.services.assets().create<be::ModelAsset>(...)`
- Remove `world_ = std::make_unique<be::World>();`
- All `be::Entity::create(*world_)` → `ctx.world.add_entity()`
- `camera_entity_ = std::make_unique<be::Entity>(be::Entity::create(*world_));` → `camera_entity_ = ctx.world.add_entity();`
- `be::add_model_to_world(*world_, root)` → `be::add_model_to_world(ctx.world, root)`
- Remove `render_system_ = std::make_unique<be::RenderSystem>(device, *world_);`
- Remove entire `render()` method body — no `on_render()` override needed
- No `on_frame_begin()` override needed (camera auto-updated by Updatable system)

#### 6.13 HotReloadGltfApp (`src/cmd/apps/hot_reload_gltf_app.h` + `.cpp`)

**Header changes:**
- `setup(EngineService&)` → `setup(EngineContext const& ctx)`
- `render(RenderDevice&, int)` → `on_render(EngineContext const& ctx)` (no-op body)
- `on_frame_begin()` → `on_frame_begin(EngineContext const& ctx)`
- **Remove members**: `asset_manager_` (unique_ptr), `world_` (unique_ptr), `render_system_` (unique_ptr), `frame_count_`
- **Change** `asset_manager_` to `AssetManager* asset_manager_ = nullptr;` (non-owning raw pointer, set in setup())
- **Remove** `world_` and `render_system_` unique_ptr members
- **Keep** `model_entities_` as `std::vector<buddd::engine::Entity>` (already value)
- `create_entities()` changes signature: `auto create_entities(buddd::engine::ModelNode& node, buddd::engine::World& world) -> void;`
- `reload_model()` changes signature: `auto reload_model(buddd::engine::World& world) -> void;`
- Remove forward declaration of `RenderDevice` (no longer used)
- Remove `#include <memory>`

**Source changes** (`hot_reload_gltf_app.cpp`):
- `setup(EngineService& engine)` → `setup(EngineContext const& ctx)`
- Use `ctx.device` instead of `engine.device()`
- **Remove** `auto am_result = be::AssetManager::create(device, "assets");` block
- **Change** `asset_manager_ = std::move(*am_result);` (unique_ptr assignment) to `asset_manager_ = &ctx.services.assets();` (pointer assignment)
- Remove `world_ = std::make_unique<be::World>();`
- All `be::Entity::create(*world_)` → `ctx.world.add_entity()` — includes setup inline entities and create_entities
- Remove `render_system_ = std::make_unique<be::RenderSystem>(device, *world_);`
- In `setup()`: `reload_model()` → `reload_model(ctx.world)` (pass World&)
- In `setup()`: `ctx.world` used for all Entity creation
- `on_frame_begin()` → `on_frame_begin(EngineContext const& ctx)`:
  - Replace `frame_count_ == 30` with `ctx.frame == 30`
  - Replace `asset_manager_->poll_file_events();` with `ctx.services.assets().poll_file_events();`
  - Replace `reload_model()` with `reload_model(ctx.world)` (pass World&)
  - Remove `++frame_count_;`
- `reload_model()` → `reload_model(buddd::engine::World& world)`:
  - All `world_->` calls become `world.` (value, not pointer)
  - `asset_manager_->clear()` → `asset_manager_->clear()` unchanged (pointer stays)
  - `asset_manager_->create<be::ModelAsset>(...)` → `asset_manager_->create<be::ModelAsset>(...)` unchanged
  - `create_entities(...)` call passes `world` as second argument
- `create_entities(buddd::engine::ModelNode& node)` → `create_entities(buddd::engine::ModelNode& node, buddd::engine::World& world)`:
  - `be::Entity::create(*world_)` → `world.add_entity()`
- `render()` → `on_render(EngineContext const& ctx)` with empty body (render_scene automatic)
  - **Move camera animation from old render() to on_frame_begin(ctx)**:
    ```cpp
    // In on_frame_begin(ctx):
    if (auto cam_opt = ctx.world.active_camera()) {
        auto& cam = cam_opt->camera();
        float a = static_cast<float>(ctx.frame) * 0.02f;
        cam.set_position({3.0f * std::sin(a), 1.0f, 3.0f * std::cos(a)});
        cam.look_at({0.0f, 0.0f, 0.0f});
    }
    ```

## Required tests

All existing unit tests MUST pass without modification. The refactoring is behaviour-preserving, so no new unit tests are required.

### Unit tests
- Run existing test suite: `ctest --preset debug` or `./build/debug/tests/buddd_tests`
- All existing tests must pass without changes (AC-011)

### E2E / Integration verification
- Build: `cmake --build --preset debug` must succeed with zero warnings (AC-010)
- Code search across `src/cmd/apps/` for `unique_ptr.*Entity` returns zero matches (AC-002)
- Code search across `src/cmd/apps/` for `asset_manager_` returns zero matches (AC-003)
- Code search for `Entity::create` returns zero matches in engine public API (AC-001)
- Code search for `world() noexcept` in App class and overrides returns zero matches (AC-007)
- Code search for `set_running` / `is_running` / `running_` in `src/cmd/app.h` returns zero matches
- Visual capture comparison: run each of the 13 apps with its standard capture arguments and compare output images pixel-for-pixel to pre-refactoring baselines (stored in `tests/captures/baselines/`) (AC-012)
- Special capture tests for HotReloadApp: capture at frame 30 shows pre-swap texture, capture at frame 60 shows post-swap texture (AC-013)
- Special capture tests for HotReloadGltfApp: capture at frame 30 shows scale 1.0, capture at frame 60 shows scale 2.0 (AC-013)
- RunApp with `--frame 10` completes without crashing (AC-015)

## Edge cases

1. **Frame ordering (7 apps)**: CubeSceneApp, TexturedCubeApp, PhongApp, GltfDemoApp, HotReloadApp, HotReloadGltfApp, and AssetDemoApp all had per-frame state updates (transform rotations, light orbits, camera animations) inside the old `render()` method. These must be moved to `on_frame_begin(ctx)` so they execute BEFORE `render_scene()`. The spec identifies these 7 apps explicitly.

2. **HotReloadApp frame-30 timing**: The texture swap + immediate `poll_file_events()` was in `render()` at frame 30. It must move to `on_frame_begin(ctx)` at `ctx.frame == 30`. The extra `poll_file_events()` call after the file copy must be preserved. Both the `on_frame_begin()` start-of-frame poll AND the frame-30 immediate poll must exist.

3. **HotReloadGltfApp `AssetManager*` pointer**: MUST use `AssetManager* asset_manager_` (raw pointer, not reference). Initialize to `nullptr`, set in `setup()` from `&ctx.services.assets()`. Dereference as `asset_manager_->clear()` etc. This is the spec-critic warning resolution.

4. **HotReloadGltfApp `reload_model()`/`create_entities()` World parameter**: These private methods previously used the member `world_`. Since `world_` is removed, they must accept a `World&` parameter. `reload_model(World& world)` and `create_entities(ModelNode& node, World& world)`.

5. **FreeCameraApp and GltfHelmetApp**: These apps had `render()` that was ONLY `render_system_->render_scene()` and nothing else. They MUST NOT override `on_render()` — the default no-op is correct. They also MUST NOT override `on_frame_begin()` — camera is auto-updated by the Updatable system via `World::update_updatables()`.

6. **RunApp with empty World**: World is always created unconditionally. `render_scene()` on an empty World is a no-op. Must not crash, assert, or log warnings.

7. **Early exit before `begin_frame()`**: If `poll_events()` returns false before `begin_frame()`, `end_frame()` must NOT be called. The new loop handles this correctly (exit check happens before `begin_frame()` or after `ctx.is_exit_requested()` with explicit `end_frame()` call).

8. **EngineContext construction in run_app()**: Constructed TWICE per frame — once before `on_frame_begin()`/`update_updatables()`/etc. and once before `setup()`. The setup-time ctx uses `frame = 0`.

9. **`ctx.is_exit_requested()` after `update_updatables()`**: After `update_updatables()` completes, `ctx.is_exit_requested()` must be checked. If true, the loop MUST call `end_frame()` before breaking (because `begin_frame()` was already called).

10. **App::setup() failure handling**: If `setup()` returns an error, `shutdown()` is called. World and RenderSystem (created before setup) will be destroyed by their `unique_ptr` after `run_app()` returns. This is the existing pattern preserved.

## Security impact

None. The refactoring is a pure internal ownership change. `EngineContext` is a stack-allocated struct of references passed through the call stack with no new persistence or external access.

## Data and migration impact

None. No schema changes, data migrations, seed data, or data loss risks. This is a compile-time refactoring with zero behavioural change.

## API compatibility impact

**Source-breaking** (requires changes to App subclasses), **behaviour-preserving**:

- `Entity::create(World&)` removed — code using it will not compile
- `World::create_entity()` renamed to `World::add_entity()`
- `App::setup(EngineService&)` → `App::setup(EngineContext const&)` — all subclasses affected
- `App::render(RenderDevice&, int)` removed — replaced by `App::on_render(EngineContext const&)` with default no-op
- `App::on_frame_begin()` → `App::on_frame_begin(EngineContext const&)` — signature change
- `App::world()` removed — code overriding it will not compile
- `App::is_running()`, `App::set_running()`, `App::running_` removed — use `ctx.request_exit()` / `ctx.is_exit_requested()`
- `EngineContext` gains 4 new fields — existing `EngineContext` construction will not compile (needs more arguments)
- `AssetManager::create()` can still be called but is no longer needed in any app — all apps use `ctx.services.assets()`

## Documentation impact

- Wiki: `docs/wiki/architecture/module-map.md` — Update `EngineContext` section (line 17) to list new fields. Update `App` lifecycle description (line 279). Update per-app descriptions for ownership changes.
- Wiki: `docs/wiki/architecture/data-flow.md` — Update frame loop section (lines 164-185) to reflect new order: `on_frame_begin(ctx)` → `update_updatables(ctx)` → `render_scene()` → `on_render(ctx)`. Remove references to `app.world()` and `app.set_running()`.
- Wiki: `docs/wiki/domain/glossary.md` — Update `Entity` definition: `create_entity()` → `add_entity()`.
- ADR: `docs/adr/ADR-023-updatable-components.md` — Update section 2 (EngineContext fields, section 5 (run_app auto-dispatch — remove app.world()), section 4 (App::setup signature now takes EngineContext const&).
- ADR: `docs/adr/ADR-014-cli-app-system.md` — Update to reflect new App signatures and ownership model.

These documentation updates are the responsibility of the wiki-agent, not the code-implementer. The code-implementer must not modify documentation files.

## ADR impact

This implementation warrants updating **ADR-023** and **ADR-014** to reflect the new ownership model and lifecycle signatures. The updates are the responsibility of the wiki-agent (documented in the spec's Documentation Updates section). The code-implementer must NOT modify ADR files.

## Done criteria

All of the following MUST be verifiable after implementation:

1. [ ] `src/engine/engine_context.h` compiles with 7 fields: `services`, `window`, `device`, `world`, `render_system`, `delta_time`, `frame`
2. [ ] `src/engine/scene/entity.h` has no `static create(World&)` declaration
3. [ ] `src/engine/scene/entity.cpp` has no `Entity::create()` function body
4. [ ] `src/engine/scene/world.h` declares `auto add_entity() -> Entity;` (no `create_entity`)
5. [ ] `src/engine/scene/world.cpp` implements `World::add_entity()` (identical body to old `create_entity`)
6. [ ] `src/cmd/app.h` has `setup(EngineContext const&)`, `on_frame_begin(EngineContext const&)`, `on_render(EngineContext const&)` (default no-op), no `render()`, no `world()`, no `running_`/`is_running()`/`set_running()`
7. [ ] `src/cmd/app.cpp` creates `World` + `RenderSystem` before `app.setup()`, calls `render_scene()` before `app.on_render()`, uses `ctx.is_exit_requested()` for exit (no `app.is_running()`)
8. [ ] All 13 apps compile with new signatures
9. [ ] `grep -r 'unique_ptr.*Entity' src/cmd/apps/` returns zero matches
10. [ ] `grep -r 'asset_manager_' src/cmd/apps/` returns matches only in `hot_reload_gltf_app.*` files (HotReloadGltfApp keeps `AssetManager* asset_manager_` as a non-owning pointer; no other app has `asset_manager_`)
11. [ ] `grep -r 'Entity::create' src/engine/` returns zero matches (except entity.h private constructor comment)
12. [ ] `grep -r 'world()' src/cmd/app.h` returns zero matches
13. [ ] `cmake --build --preset debug` succeeds with zero warnings
14. [ ] All existing unit tests pass
15. [ ] HotReloadGltfApp stores `AssetManager* asset_manager_` (not `AssetManager&`), initialized to `nullptr`, set from `&ctx.services.assets()`
16. [ ] HotReloadGltfApp `reload_model()` and `create_entities()` accept `World&` parameter
17. [ ] 7 apps with per-frame animation (CubeSceneApp, TexturedCubeApp, PhongApp, GltfDemoApp, HotReloadApp, HotReloadGltfApp, AssetDemoApp) have transform/light/camera updates in `on_frame_begin(ctx)` not `on_render(ctx)`
