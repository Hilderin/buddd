# Implementation Contract Review — Window Resize for All Apps (Re-review 2)

*Previous review: rejected (missing includes in test 2). Updated contract now addresses all issues.*

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] Missing includes in test 2 (`RenderDeviceHeadless::size reflects on_resize`): `tests/window_resize_tests.cpp` uses `EngineService::create()` and `eng.device().size()` but is missing `#include "engine_service.h"` and `#include "render/render_device.h"`. Will not compile as specified. *(Resolved: includes now present at top of test file, shared across all test cases.)*

## Warnings

Non-blocking concerns for awareness:

- [x] Line-number comment in section 2 says "after line 16 (auto height()...)" but height() is on line 15; native_handle() is on line 16. *(Resolved: now correctly says "after line 15".)*
- [x] WindowSDL3::~WindowSDL3() lacks null check on SDL_GetWindowID() — potential dangling pointer if ID lookup fails. *(Resolved: `if (id != 0)` guard added.)*
- [x] Include order in platform_sdl3.h: `<SDL3/SDL.h>` placed after `<cstdint>` contradicts convention #10 (library before system). *(Resolved: `<SDL3/SDL.h>` now placed before `<cstdint>`.)*
- [x] Deferred "final size" INFO logging per spec Observability — acknowledged by contract-author as requiring a debounce mechanism not yet implemented. *(Acknowledged; noted as deferred in contract. Not a contract issue.)*
- [x] create_window() log message deviates from spec ("remains unchanged" → changed to include "resizable" and windowID). *(Resolved: documented in "Notable deviations from spec.")*
- [x] Registration responsibility differs from spec: spec says WindowSDL3 registers in its constructor, contract does it in PlatformSDL3::create_window(). *(Resolved: documented in "Notable deviations from spec.")*
- [ ] AC-006 automated SDL event injection test not implemented (spec-level inconsistency: AC-006 requires automated, but E2E section lists SDL3 as manual). *(Spec-level issue, not a contract issue — contract faithfully follows the E2E section's manual testing approach. Remains as awareness for spec-author.)*

## Required changes

None. All previously required changes are resolved.

## Suggested improvements

- Consider implementing the final-size INFO logging (debounced) as a follow-up; the contract's event-level DEBUG logging is sufficient for the initial implementation.
