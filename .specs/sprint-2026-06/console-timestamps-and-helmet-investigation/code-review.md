# Implementation Review — Console Timestamps, FreeCameraMovement Refactoring & Helmet Investigation

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] **Wiki `module-map.md` documents wrong Updatable interface signature** — The entry at line 122 states:
  ```
  `Updatable` ... Provides `virtual update(const InputSystem&, Window&, float dt) -> bool`
  ```
  But the actual implementation is `virtual auto update(const EngineContext& ctx) -> void`. The wiki must be updated to match the implemented interface (and should also mention the `EngineContext` struct and the `request_exit()` exit mechanism).

## Warnings

Non-blocking concerns for awareness:

- **Architectural deviation: `EngineContext` struct and `Updatable::update()` interface changed from accepted spec** — The spec and contract specify:
  ```cpp
  virtual auto update(const InputSystem& input, Window& window, float dt) -> bool = 0;
  ```
  The implementation changes this to:
  ```cpp
  virtual auto update(const EngineContext& ctx) -> void = 0;
  ```
  with exit via `ctx.request_exit()` instead of returning `false`. While functionally correct and arguably cleaner, this is a meaningful architectural change that was not in the accepted spec or contract. A new ADR documenting this design decision may be warranted (the ADR agent should evaluate).

- **`App::setup()` signature change cascaded to all 12+ apps** — The contract only explicitly listed `free_camera_app`, `phong_app`, and `gltf_helmet_app` for refactoring, but changing the base class `setup()` signature from `RenderDevice&` to `EngineService&` necessarily requires updating all derived classes. This was correctly done but is broader than the spec's listed scope.

- **`run_app()` restructured to use `EngineService::create()`** — The contract specified manual Platform/Window/RenderDevice creation in `run_app()`, but the implementation replaces it with `EngineService::create()`. This aligns with the wiki's documented EngineService lifecycle pattern and is a valid improvement, but it was not in the contract.

- **No short-circuit in `update_updatables()`** — The spec's AC-006 requires that `update_updatables()` returns `false` on the first `false` return (short-circuit). The new implementation iterates all updatables every frame (no short-circuit), deferring the exit check after all complete. This is a minor behavioral difference from the spec.

- **`free_camera_app` initial camera position check** — The spec says initial camera position is (0, 2, 5) but the contract says (0, 2, 5). The implementation correctly uses (0, 2, 5). Verified.

## Required changes

Concrete, actionable changes requested:

- [ ] Update `docs/wiki/architecture/module-map.md` line 122 to document the actual `Updatable` interface signature: `update(const EngineContext&) -> void`.

## Suggested improvements

Optional ideas (not required):

- Consider adding a short-circuit in `World::update_updatables()` if an Updatable requests exit, to match the spec's original AC-006 short-circuit semantics:
  ```cpp
  void World::update_updatables(const EngineContext& ctx) {
      for (auto* upd : updatables_) {
          upd->update(ctx);
          if (ctx.is_exit_requested()) break; // short-circuit
      }
  }
  ```

## Review summary

**Build**: ✅ Passes (cmake --build, no errors)
**Tests**: ✅ All 420 tests pass (21421 assertions)
**Spec AC compliance**: ⚠️ Functional ACs satisfied (timestamps format, camera controls, app refactoring, scene dispatch, cleanup safety). Interface-level ACs (AC-004, AC-006, AC-011) differ due to the EngineContext architectural change.
**Files allowed/forbidden**: ✅ Allowed files modified, forbidden files untouched (`model_loader.cpp`, PBR shaders, platform/window/input unchanged).
**Documentation updated**: ✅ ADR-020 updated, wiki logging.md updated (but module-map.md has wrong Updatable signature).
**New files**: All created files match the contract's intent (free_camera_movement.h/.cpp, updatable.h, gltf_helmet_app.h/.cpp, engine_context.h).
