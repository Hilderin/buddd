# ADR-023-updatable-components - Updatable Components & EngineContext

## Status

Accepted

## Context

Every interactive scene in the Buddd Engine needed per-frame update logic — camera movement, input handling, animation, etc. Before this ADR, each app was responsible for manually iterating its updatable components inside `render()`:

```cpp
world_.each<FreeCameraMovement>([&](auto& entity, auto& cam) {
    cam.update(input, window, dt);
});
```

This had several problems:

1. **Boilerplate duplication** — Every interactive app duplicated the same iteration pattern. Adding a new updatable component required modifying the app's render loop.
2. **No framework-level dispatch** — The engine had no concept of "things that update every frame." Update logic was an ad-hoc convention, not a framework primitive.
3. **Tight coupling to concrete input/window types** — The `FreeCameraMovement::update()` signature required explicit `InputSystem&, Window&, float dt` parameters, making future extension (e.g., passing engine services) a breaking change across all call sites.
4. **No uniform per-frame context** — There was no standard object encapsulating all per-frame state (services, window, delta time). Each updatable component pulled what it needed from disparate sources.

A framework-level solution was needed: an `Updatable` interface that the `World` discovers automatically, and a per-frame `EngineContext` that bundles all state an updatable needs.

## Decision

### 1. Updatable interface (`src/engine/scene/updatable.h`)

A pure abstract class orthogonal to `Component`:

```cpp
class Updatable {
public:
    virtual ~Updatable() = default;
    virtual auto update(const EngineContext& ctx) -> void = 0;
};
```

- **Not a subclass of `Component`** — `Updatable` is a standalone interface. A component can inherit from both via multiple inheritance.
- **Returns `void`** — Exit signalling is done via `EngineContext::request_exit()` rather than a boolean return value, because multiple updatables may need to run per frame.
- **No short-circuit** — All updatables are always called each frame. Exit is checked after the full iteration loop.

### 2. EngineContext (`src/engine/engine_context.h`)

A struct bundling all per-frame state:

```cpp
struct EngineContext {
    EngineService& services;
    Window& window;
    float delta_time;

    void request_exit() const;
    [[nodiscard]] auto is_exit_requested() const -> bool;
};
```

- Passed as `const&` to every `Updatable::update()` call — single const reference, everything accessible.
- `exit_requested_` is `mutable` to allow `const&` propagation while still enabling state mutation.
- Unifies the parameter list: instead of threading `InputSystem&, Window&, float dt` separately, components receive one parameter that can grow without breaking call sites.

### 3. World auto-registration and cleanup

**Registration** — `World::add_component<T>()` auto-detects `Updatable` subclasses at compile time:

```cpp
if constexpr (std::is_base_of_v<Updatable, T>) {
    updatables_.push_back(static_cast<Updatable*>(ptr));
}
```

**Cleanup** — Two paths prevent dangling pointers:

- `flush_destroyed()` — Before the `EntityNode` `unique_ptr` is destroyed, iterate its `components_` and `std::erase` any `Updatable*` from `updatables_` via `dynamic_cast`.
- `remove_component<T>()` — Before erasing a component, `dynamic_cast` to `Updatable*` and `std::erase` from `updatables_`.

### 4. App::setup(EngineService&) — broadened signature

Changed from `App::setup(RenderDevice&)` to `App::setup(EngineService&)`. This gives apps access to the full engine service locator (platform, window, input, asset manager, render device, etc.) from setup time, rather than only the render device.

### 5. run_app auto-dispatch

`run_app()` in `src/cmd/app.cpp` creates an `EngineContext` each frame and calls `World::update_updatables(ctx)` before `app.render()`:

```cpp
be::EngineContext ctx{eng, eng.window(), eng.platform().delta_time()};
if (auto* app_world = app.world()) {
    app_world->update_updatables(ctx);
    if (ctx.is_exit_requested()) {
        app.set_running(false);
    }
}
```

- `App::world()` virtual method (default `nullptr`) lets apps expose their `World*`.
- `App::set_running(bool)` exposed publicly (was `protected` setter only).
- Exit is signalled via `EngineContext::request_exit()` rather than a `bool` return — all updatables run before the exit check, which avoids partial-update problems.

## Alternatives considered

### 1. Manual per-app iteration (status quo)

Continuing to have each app call `world_.each<FreeCameraMovement>` in `render()`.

**Rejected** — Boilerplate duplication across every interactive app. Adding a new updatable component requires modifying every app that uses it. No framework-level discoverability.

### 2. Virtual method on Component base class

Adding a virtual `update()` method directly to `Component`:

```cpp
class Component {
    virtual auto update(const EngineContext&) -> void {}
};
```

**Rejected** — Would bloat every component (including data-only ones like `Transform`, `MeshComponent`) with a vtable slot and a no-op virtual method. The `Updatable` interface is opt-in: only components that need per-frame updates inherit from it.

### 3. System-based ECS (entity-component-system)

Introducing a full system/phase architecture where systems are registered globally and iterated in order.

**Deferred** — Too complex for V1. The `Updatable` approach covers 100% of current use cases (camera movement, input handling) with minimal machinery. A system-based architecture can be layered on top of `Updatable` later if phases, ordering, or parallel iteration become necessary.

### 4. Boolean return value for short-circuit exit

The original spec defined `update() -> bool` with short-circuit (first `false` stops iteration).

**Rejected during implementation** — Short-circuit means some updatables are skipped if an earlier one requests exit. This is surprising and introduces partial-update problems (e.g., camera movement runs but animation doesn't, leaving the scene in an inconsistent state). The `request_exit()` + post-iteration check pattern ensures all updatables run every frame regardless of exit signalling.

### 5. Weak_ptr / shared_ptr for updatable registry

Using `std::weak_ptr<Updatable>` or `std::shared_ptr<Updatable>` in `updatables_` instead of raw pointers.

**Rejected** — Components are owned by `EntityNode::components_` (vector of `unique_ptr<Component>`). Adding another ownership layer (shared_ptr) would complicate lifetime management and introduce reference cycles. The raw pointer registry is valid because cleanup is explicit in `flush_destroyed()` and `remove_component<T>()`, matching ADR-011's guidance on raw pointers for non-owning observers.

## Consequences

### Positive

- **Zero-boilerplate update** — Apps no longer iterate updatable components manually. Adding a new `Updatable` subclass and attaching it to any entity "just works" — `World` discovers it at add-time, `run_app` dispatches it every frame.
- **EngineContext is extensible** — New per-frame state (e.g., frame number, timing statistics, debug overlay) can be added to `EngineContext` without changing any `Updatable::update()` signature.
- **Orthogonal to Component** — Not all updatables need to be ECS components (though the current use case is component-based). The interface is standalone.
- **Safe cleanup** — Explicit `dynamic_cast` + `std::erase` in both destroy paths guarantees no dangling pointers, validated by spec AC-034 and EC-010.
- **App::setup(EngineService&)** — Apps now have access to the full engine service graph at setup time (asset manager, input system, platform), enabling richer initialisation without adding more parameters.

### Negative

- **All updatables run even after exit request** — If one updatable calls `request_exit()`, remaining updatables still execute for that frame. This is intentional (avoids partial-update inconsistency) but means exit is deferred by at most one frame.
- **Dynamic_cast in cleanup paths** — `flush_destroyed()` and `remove_component<T>()` use `dynamic_cast<Updatable*>` per component, adding RTTI overhead on entity destruction. This is acceptable because entity destruction is infrequent (not a per-frame hot path).
- **App::setup() signature change cascaded to all 12+ apps** — Every app subclass had to update its `setup()` signature from `(RenderDevice&)` to `(EngineService&)`. This was a mechanical but wide-reaching change.
- **Raw pointer registry in World** — `std::vector<Updatable*>` introduces lifetime coupling between World and its components. The safety hinges on the correctness of the cleanup logic in both destroy paths. An incorrect implementation would cause use-after-free.

### Risks

- **Low** — The `Updatable` system is additive and does not change the `Component` base class. Existing code (non-updatable components, static scenes) is unaffected.
- **Medium** — The raw pointer registry is safe only because cleanup is enforced in both `flush_destroyed()` and `remove_component<T>()`. Any future component-removal path must also clean up the updatables_ vector. This is a maintenance hazard that should be documented at the declaration site.

## Compliance

- `src/engine/scene/updatable.h` MUST declare `Updatable` as a pure abstract class with `virtual auto update(const EngineContext& ctx) -> void = 0` and a virtual destructor.
- `src/engine/engine_context.h` MUST define `EngineContext` with `EngineService& services`, `Window& window`, `float delta_time`, `request_exit()`, and `is_exit_requested()`.
- `src/engine/scene/world.h` MUST auto-register `Updatable` subclasses in `add_component<T>()` via `if constexpr (std::is_base_of_v<Updatable, T>)`.
- `src/engine/scene/world.h/world.cpp` MUST clean up `Updatable*` pointers in both `flush_destroyed()` and `remove_component<T>()` before component destructors run.
- `World::update_updatables(const EngineContext&)` MUST iterate all registered updatables and MUST NOT short-circuit on exit request.
- `App::setup()` MUST accept `EngineService&` (not `RenderDevice&`).
- `run_app()` MUST create `EngineContext` each frame and call `World::update_updatables(ctx)` before `app.render()`.

## Related documents

- SPEC-023 — Console Timestamps, FreeCameraMovement Refactoring & Helmet Investigation (`.specs/sprint-2026-06/console-timestamps-and-helmet-investigation/spec.md`)
- ADR-011 — No raw pointers in public API (raw pointer precedent; `updatables_` follows the non-owning observer pattern)
- ADR-019 — Architecture Boundaries (engine_context.h is stdlib-only, compliant)
- Wiki: `docs/wiki/architecture/module-map.md` — Documents `Updatable`, `EngineContext`, `FreeCameraMovement`
- Wiki: `docs/wiki/architecture/data-flow.md` — Documents `Updatable` auto-update step in `run_app` render loop

---

*Derived from SPEC-023 and decisions settled during implementation.*
