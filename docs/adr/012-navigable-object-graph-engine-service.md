# ADR-012: Navigable Object Graph, EngineService, and Abstract Interface Extensions

## Status

`Accepted`

## Context

SPEC-016 introduced a refactoring of the relationships between `Platform`, `Window`, `RenderDevice`, and `InputSystem` to address several pain points in the Buddd Engine codebase:

1. **No back-links between components**: `Window` had no reference to its creating `Platform`. `RenderDevice` had no reference to the `Window` it was created from. This forced every consumer that needed `Platform` or `Window` services (event polling, input system, mouse capture) to receive separate references threaded through call chains.

2. **Redundant `Platform&` parameter in every demo function**: All four demo functions accepted both `Platform&` and `RenderDevice&`, even though `Platform` was only needed for event polling, delta time, and input — not for rendering. Adding a new demo or feature required threading yet another reference through the dispatch chain.

3. **Mouse capture blocked**: Adding `set_mouse_capture`/`is_mouse_captured` to `Window` required `Window` to be reachable from code paths that only held a `RenderDevice&`. Without a navigable graph, this would have required a `Window&` parameter on every demo function.

4. **Ad-hoc test construction**: Unit tests constructed `RenderDeviceHeadless` directly via `RenderDeviceHeadless(800, 600)`, bypassing the real construction chain. This meant tests never exercised `Platform::create_window()` or `RenderDevice::create(Window&)`, and the construction chain was untested outside production code.

5. **Diagnostics only on headless backend**: `frame_begin_count()`, `frame_end_count()`, and `draw_call_count()` were only available on `RenderDeviceHeadless`. Code paths that received a `RenderDevice&` (e.g., tests using `EngineService::device()`) could not access these diagnostics without downcasting.

### Precedent and constraints

- ADR-010 prohibits raw pointers in public API. All new back-links use non-owning references (`Platform&`, `Window&`), not raw pointers.
- CONST-001 prohibits SDL3/OpenGL/GLM headers outside `src/engine/`. The navigable graph is implemented entirely within engine abstractions.
- ADR-003 established that draw methods are `void` (not `Result<void>`) and that `Platform::poll_events()` exists on the platform abstraction. No changes to those decisions.

### Alternatives considered per decision

| Alternative | Verdict |
|---|---|
| **Keep manual reference threading** — every function that needs Platform, Window, or InputSystem receives separate references. | **Rejected.** Does not scale. Every new feature that touches one of these components requires modifying function signatures up and down the call chain. Mouse capture would have required adding `Window&` to all four demo signatures. |
| **Use a global singleton** to hold Platform/Window/RenderDevice references. | **Rejected.** Violates the project's existing design ethos (no global state, explicit construction). Makes testing harder (tests would need to set up global state). Hides dependencies. |
| **Make EngineService a DI container / registry** with dynamic lookup by type. | **Rejected.** Over-engineering for the current scope (3 components, < 4 consumers). Direct accessors on EngineService are simpler, type-safe, and auditable. A registry can be introduced later if needed. |
| **Keep diagnostics headless-only** and use `dynamic_cast` in tests. | **Rejected.** Fragile (casts can fail at runtime), couples test code to concrete types, and the pattern would be copied into every new test. Virtual methods on the base class with default 0 implementations are zero-cost when not overridden by OpenGL backends. |

## Decisions

### Decision 1: Establish a navigable object graph with non-owning back-references

`Window` stores a non-owning `Platform&` reference (passed via constructor, stored in the base class). `RenderDevice` stores a non-owning `Window&` reference (passed via constructor, stored in each backend). Together, these create a fully navigable graph:

```
RenderDevice  ──>  Window  ──>  Platform  ──>  InputSystem
```

- **New module dependency**: `window/` now depends on `platform/` (forward declaration of `Platform` in `window.h`; non-owning reference).
- **New module dependency**: `render/` now depends on `window/` (forward declaration of `Window` in `render_device.h`; pure virtual accessor `window() -> Window&`).
- All back-references are non-owning (`T&`, not `T*`), compliant with ADR-010.
- Lifecycle invariants are unchanged: `Platform` must outlive `Window` must outlive `RenderDevice`.

### Decision 2: Introduce `EngineService` as the lifecycle owner of the component chain

A new class `EngineService` (in `src/engine/engine_service.h/.cpp`) owns the entire `Platform` → `Window` → `RenderDevice` chain via `std::unique_ptr`. It is the single entry point for engine lifecycle:

- **Factory method**: `EngineService::create(Backend, WindowConfig) -> Result<std::unique_ptr<EngineService>>`.
- **Accessors**: `.platform()`, `.window()`, `.device()` return references valid for the service's lifetime.
- **Member declaration order** (`platform_`, `window_`, `device_`) guarantees correct destruction ordering: `RenderDevice` first, then `Window`, then `Platform`.
- Used by both tests and production code (`demo_command.cpp`).
- Replaces direct `RenderDeviceHeadless(width, height)` constructions in tests.

### Decision 3: Add diagnostic virtual methods to `RenderDevice` base class

`RenderDevice` gains three virtual methods with default `0` implementations:

```cpp
virtual auto frame_begin_count() const noexcept -> int { return 0; }
virtual auto frame_end_count() const noexcept -> int { return 0; }
virtual auto draw_call_count() const noexcept -> int { return 0; }
```

`RenderDeviceHeadless` overrides these to return real counters. `RenderDeviceOpenGL` inherits the default `0` implementations. This enables code that holds a `RenderDevice&` (e.g., tests using `EngineService::device()`) to access diagnostics without downcasting.

### Decision 4: Add mouse capture to `Window` abstract interface

`Window` gains two pure virtual methods:

```cpp
virtual auto set_mouse_capture(bool captured) -> void = 0;
virtual auto is_mouse_captured() const noexcept -> bool = 0;
```

- **SDL3 backend**: Calls `SDL_SetWindowRelativeMouseMode` / caches state.
- **Headless backend**: No-op; `is_mouse_captured()` returns `false`.

This is the first input/hardware feature on the `Window` abstract interface. It is motivated by the free camera demo's need for relative mouse mode, but the interface is general-purpose.

## Consequences

### Positive

- **Simpler demo API**: Demo functions now accept only `RenderDevice&` (and `argc`/`argv`). `Platform` is accessible via `device.window().platform()`.
- **No reference threading**: Any code with a `RenderDevice&` can reach `Window`, `Platform`, and `InputSystem` without additional parameters.
- **Mouse capture unblocked**: The free camera demo can call `device.window().set_mouse_capture(true)` without needing a separate `Window&` parameter.
- **Test construction chain exercised**: Tests now use `EngineService::create()`, which exercises `Platform::create_window()` and `RenderDevice::create(Window&)`.
- **Zero-cost diagnostics**: Default `0` virtual methods on the base class add no overhead to OpenGL backends (they inherit the default). Headless backends override with real counters.
- **No raw pointers**: All back-references use `T&`, compliant with ADR-010.
- **Explicit destruction ordering**: EngineService's member declaration order guarantees correct teardown without documentation reliance.

### Negative

- **New module dependencies**: `window/` now includes a forward declaration of `Platform`; `render/` now includes a forward declaration of `Window`. These are forward declarations only (no header includes), but they create measurable coupling between modules that were previously independent.
- **Window becomes a "god accessor"**: `Window` now provides access to `Platform` (and via it, `InputSystem`), in addition to its own metrics and mouse capture. There is a risk that `Window` becomes a dumping ground for unrelated accessors. Future accessors should be added to the appropriate component (Platform, InputSystem) rather than to Window.
- **EngineService duplicates existing factory logic**: `demo_command.cpp` already constructed Platform → Window → RenderDevice inline. EngineService formalises this pattern but adds a new public class that must be maintained.
- **Virtual diagnostic methods are a leaky abstraction**: The three counter methods (`frame_begin_count`, `frame_end_count`, `draw_call_count`) are inherently test-only concerns. Adding them to the abstract `RenderDevice` interface pollutes the API with non-production methods. The default-0 approach minimises the pollution but does not eliminate it.

### Preserved invariants

- CONST-001: No SDL3/OpenGL/GLM headers outside `src/engine/`.
- ADR-010: No raw pointers in public API (all back-links are `T&`).
- ADR-003: Draw methods remain `void`; `Platform::poll_events()` unchanged.
- Lifecycle rules: `Platform` outlives `Window` outlives `RenderDevice`.
- Demo dispatch (if/else in `DemoCommand::run()`) unchanged, per ADR-004.

## Related documents

- SPEC-016 (`.specs/sprint-2026-05/architecture-refactor-device-window-platform/spec.md`): Spec-level documentation of the refactoring.
- ADR-010 (`docs/adr/010-no-raw-pointers-in-public-api.md`): Raw pointer prohibition — all back-references use `T&`.
- ADR-003 (`docs/adr/003-render-pipeline-architecture.md`): Draw method exception and `Platform::poll_events()` — unchanged.
- ADR-004 (`docs/adr/004-demo-system-architecture.md`): Demo system architecture — demo dispatch pattern unchanged.
- CONST-001 (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): Architecture boundary preserved.
