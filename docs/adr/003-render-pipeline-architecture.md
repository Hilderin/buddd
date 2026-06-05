# ADR-003: Render Pipeline Architecture — Explicit Exceptions to Established Patterns

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

SPEC-005 / IMPL-005 introduced the render pipeline abstractions (Shader, Material, VertexBuffer, IndexBuffer) to the Buddd Engine. The implementation followed the established Platform/Window/RenderDevice abstraction pattern.

One decision contradicted existing project-wide conventions; one potential contradiction was avoided by adding an abstraction.

### 1. Draw methods return `void` instead of `Result<void>`

ADR-001 establishes that *all* public API functions that can fail return `Result<T>`. The new `draw()` and `draw_indexed()` methods on `RenderDevice` return `void` instead.

These methods are on a performance-sensitive hot path — called every frame, potentially hundreds or thousands of times. Per-frame error checking imposes:
- Branching overhead at every call site (checking `.has_value()` or propagating error).
- Code bloat from error-return paths in inline functions.
- An interface that silently encourages callers to ignore errors, since hot-path error checking is rarely done in practice.

Instead, the design adopts the convention from real-time graphics APIs (OpenGL, Vulkan, DirectX): draw calls are **immediate commands with precondition-based contracts**. Precondition violations (invalid topology, out-of-bounds vertex access, unlinked material, draw outside `begin_frame()`/`end_frame()`) are **undefined behaviour** — the caller must ensure correct state before drawing. This is a deliberate, documented exception to ADR-001.

### 2. `Platform::poll_events()` added instead of allowing SDL3 includes in `main.cpp`

The interactive render mode in `src/cmd/main.cpp` needs to pump events to keep the window responsive and detect close requests. The initial implementation contract proposal authorized `#include <SDL3/SDL.h>` in `main.cpp`, which would have violated CONST-001.

Instead, the accepted approach adds `Platform::poll_events() -> bool` to the engine abstraction layer. This method:
- **SDL3 backend**: Calls `SDL_PollEvent` internally. If the event is `SDL_EVENT_QUIT`, returns `false` (signal to stop). All other events are discarded (input handling is future work).
- **Headless backend**: Always returns `true` (no quit signal, no events to process).

This avoids a CONST-001 exception entirely — `src/cmd/main.cpp` uses only the abstract `Platform` interface for event polling, with no SDL3 includes outside `src/engine/`.

## Decision

### Decision 1: Draw methods are `void`, not `Result<void>`

`RenderDevice::draw()` and `RenderDevice::draw_indexed()` return `void`. Precondition violations are undefined behaviour. This exception to ADR-001 is limited to:

- **Only** `draw()` and `draw_indexed()` on `RenderDevice`.
- **Only** because they are hot-path rendering commands where per-call error checking is impractical and graphics API convention universally uses precondition-based contracts for draw calls.

All other fallible render pipeline methods (`create_shader`, `create_material`, `create_vertex_buffer`, `create_index_buffer`, `set_uniform`) continue to return `Result<T>` per ADR-001.

### Decision 2: `Platform::poll_events() -> bool` added to prevent CONST-001 violation

Instead of carving out a CONST-001 exception, `Platform` gains a new pure virtual method:

```cpp
/// Polls the platform event queue.
/// Returns false if the user requested to quit (e.g., window close button),
/// true otherwise. In headless mode, always returns true.
virtual auto poll_events() -> bool = 0;
```

This keeps `src/cmd/main.cpp` clean — it uses only the abstract platform interface. No SDL3 headers leak outside `src/engine/`. This is consistent with the architecture boundary and follows the established abstraction pattern.

## Alternatives considered

### For Decision 1 (draw returns `Result<void>`)

| Alternative | Verdict |
|---|---|
| Return `Result<void>` from draw calls | **Rejected.** Every draw call site would need `.has_value()` / `.error()` handling. In practice, callers would ignore the result (`.ignore()`), adding noise with no safety benefit. The hot-path cost of branches, error storage, and code bloat is non-negligible for draw calls that may execute tens of thousands of times per frame. |
| Return `Result<void>` in debug builds, `void` in release | **Rejected.** Inconsistent API across build configurations leads to confusing code and untested error paths. The precondition-violation-UB design is honest: the caller must ensure state, regardless of build mode. |
| Use a separate `CheckedRenderDevice` wrapper | **Rejected.** Adds complexity (wrapper type, dynamic dispatch or template) for marginal benefit. Precondition violations in draw calls are programming bugs, not runtime errors to recover from. |

### For Decision 2 (SDL3 include in `main.cpp` or `Platform::poll_events()`)

| Alternative | Verdict |
|---|---|
| Add `Platform::poll_events()` returning `bool` | **Accepted** (chosen approach). Minimal API surface (single method, single return value). Does not require a full event abstraction — just a quit signal for now. Headless backend trivially returns `true`. Consistent with the existing abstraction pattern. |
| Allow `#include <SDL3/SDL.h>` in `main.cpp` | **Rejected** (initially proposed in the contract, revised after human review). Would create a second CONST-001 exception. The `poll_events()` approach is architecturally cleaner and adds minimal scope. |
| Put the render loop + event pump in `src/engine/` | **Rejected.** The run loop is an application-level concern, not an engine concern. The engine provides primitives; the application decides the loop structure. |
| Add `Window::poll_events()` instead of `Platform::poll_events()` | **Rejected.** Event polling is a platform-level concern (the platform owns the event queue), not a window-level concern. Putting it on `Platform` is semantically correct. |

## Consequences

### Positive

- **Draw calls are zero-overhead**: No branching for error checking, no error storage, no code bloat from error-return paths in the hot path.
- **Honest API contract**: Draw call precondition violations are UB, matching the mental model of graphics programmers and real-world GPU API conventions.
- **Main loop is explicit**: `src/cmd/main.cpp` owns the event pump and the render loop, making the program flow visible and modifiable by application developers.
- **Minimal scope creep**: The render pipeline implementation did not need to design a full event abstraction to have a working demo.
- **Exception is narrow and auditable**: Both exceptions are explicitly scoped and can be enforced by code review.

### Negative

- **ADR-001 has a carveout**: The project's error-handling convention is no longer universal. Developers must remember that draw methods are an exception and not use this as a precedent to make other hot-path methods return `void`.
- **CONST-001 remains intact**: No new exception was needed for `main.cpp` because event polling is abstracted through `Platform::poll_events()`. The only CONST-001 exception remains AMEND-2026-001 (SDL3 test file hint-setting).
- **`main.cpp` is testable without SDL3**: Because `main.cpp` uses only the abstract `Platform` interface, it compiles and links without SDL3. Headless builds work.
- **No exception creep risk for CONST-001**: The architecture boundary was preserved without a new exception.

### Precedent for future exceptions

These two exceptions do **not** establish a pattern for automatic approval of similar exceptions. Each future exception request must:

1. Demonstrate that the convention (ADR-001's `Result<T>` or CONST-001's boundary) creates a genuine problem for the specific use case.
2. Show that all reasonable alternatives within the convention were considered and rejected.
3. Define narrow, auditable scope for the exception.
4. Be documented in an ADR with rationale and consequences.

## References

- ADR-001 (`docs/adr/001-result-error-pattern.md`): The error-handling convention that Decision 1 carves out from.
- CONST-001 (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): The architecture boundary that Decision 2 preserves by adding `Platform::poll_events()`.
- SPEC-005 (`.specs/sprint-2026-05/render-pipeline/spec.md`): Spec-level documentation of the render pipeline.
- IMPL-005 (`.specs/sprint-2026-05/render-pipeline/implementation-contract.md`): Contract-level implementation details.
