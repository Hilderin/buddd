# CONST-001-architecture-boundaries - Architecture Boundaries

## Rule

No code outside `src/engine/` may include platform, graphics, or windowing
library headers (e.g., `<SDL3/`, `<GL/`, `<glad/>`). All access to
platform-level and graphics APIs must go through the engine's abstraction
layer (`Platform`, `Window`, `RenderDevice` and their equivalents for future
subsystems).

Violations must be caught during code review. Automated enforcement via a
compile-time header guard or CI linting is encouraged where feasible.

## Rationale

Direct inclusion of platform or graphics library headers outside
`src/engine/` couples consumers to specific library versions and platform
APIs, makes unit tests dependent on display servers and GPU availability,
and increases the cost of swapping backends. The abstraction layer ensures
that code outside the engine core remains platform-independent and testable
without a display.

## Enforcement

Blocking. Code that violates this boundary must not be merged.

## Exceptions

None. New platform or graphics library dependencies must be added inside
`src/engine/` following the same abstraction pattern.
