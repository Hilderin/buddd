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

---

## Amendment AMEND-2026-001 — SDL3 Test File Exception

### Status
Ratified

### Proposed change
Add a narrow exception to CONST-001 allowing SDL3 test files to include
`<SDL3/SDL.h>` under specific, tightly-scoped conditions.

### Current rule affected
CONST-001 — Architecture Boundaries (this file).

### Change description
Add the following exception to the rule:

> **Exception:** SDL3 test files (`tests/*_sdl3*.cpp` or similar) that are
> conditionally compiled with the `BUDDD_HAS_DISPLAY` CMake option set to `ON`
> may include `<SDL3/SDL.h>` only for the purpose of setting video driver hints
> (e.g., `SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy")`) before calling
> `Platform::create()`. This exception is narrow: it applies only to test files
> that test the SDL3 backend, only to `<SDL3/SDL.h>`, and only for setting
> video driver hints. All other platform, graphics, or windowing library headers
> remain prohibited outside `src/engine/`.

### Reason
SDL3 backend tests need to configure the video driver before engine
initialisation (e.g., selecting the `"dummy"` driver so tests run on a
headless CI runner). This cannot be done through the engine abstraction layer
because the hint must be set *before* `Platform::create()` is called.
A narrow, auditable exception is preferable to workarounds such as environment
variables, platform-specific initialisation paths in engine code, or skipping
SDL3 backend tests entirely.

### Impact
- **Positive:** SDL3 backend tests can run on headless CI without a display
  server, and the video driver selection is explicit in test code.
- **Negative:** Introduces a controlled exception to a previously absolute
  boundary. The risk is mitigated by the narrow scope (single header, single
  purpose, conditional compilation).

### Migration plan
No migration required. The exception applies only to new or modified test files
targeting the SDL3 backend. Existing code outside `tests/` is unaffected.

### Risks
- Test files could misuse the exception to include other SDL3 headers or use
  SDL3 APIs beyond hint-setting. Mitigation: code review must verify that
  `#include <SDL3/SDL.h>` is used only for `SDL_SetHint()` and is guarded by
  `#ifdef BUDDD_HAS_DISPLAY`. Automated enforcement (e.g., CI lint) is
  recommended.
- Future backend test files (e.g., for GLFW, Vulkan) might request similar
  exceptions. Each such request must be evaluated on its own merits; this
  amendment does not set a precedent for automatic approval.

### Ratification
- **Date:** 2026-05-29
- **Approving authority:** Guillaume (user)
- **Process:** Explicit human ratification of the amendment proposal.
