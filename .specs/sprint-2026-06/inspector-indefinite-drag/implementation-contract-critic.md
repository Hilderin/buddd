# Implementation Contract Review — Inspector Indefinite Drag

**Re-review (2026-06-14)**: Loopback fix verified. Include paths in section 5a are now correct (`"engine_service.h"`, `"input/input_system.h"`, `"platform/platform.h"`). The previously blocking issue (DC-07 compile failure) is resolved. All 10 ACs remain covered by the 15 DCs. No new issues introduced. **Verdict: accepted** — contract is ready for implementation.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **DC-07 include paths will fail to compile** — RESOLVED in loopback fix. The contract (section 5a) specifies adding `#include "engine/engine_service.h"`, `#include "engine/input/input_system.h"`, `#include "engine/platform/platform.h"`. These paths are wrong. The `buddd_engine` target sets its public include directory to `${CMAKE_CURRENT_SOURCE_DIR}` which is `src/engine/`. The existing file already uses this convention (e.g., `#include "log/log.h"` resolves to `src/engine/log/log.h`). The correct includes should be:
  - `#include "engine_service.h"` (not `"engine/engine_service.h"`)
  - `#include "input/input_system.h"` (not `"engine/input/input_system.h"`)
  - `#include "platform/platform.h"` (not `"engine/platform/platform.h"`)
  
  As written, these includes would attempt to resolve to `src/engine/engine/engine_service.h` etc., which does not exist. Fixing this is necessary before the code can compile. (Affects DC-13.)

  **Resolution**: This is a contract-level issue — loop back to implementation-contract-author.

## Warnings

Non-blocking concerns for awareness:

- **Fragile line number references** — The contract uses absolute line numbers throughout (e.g., "after line 36", "between line 174 and line 176", "line 146-198"). These line numbers may not match the actual files if they have been edited since the contract was written. The surrounding descriptive text is clear enough for an experienced implementer, but the line numbers could mislead. Consider using descriptive markers (e.g., "after the `// ── State (double-buffered) ──` comment") instead.

- **`<unordered_map>` not explicitly included** — The existing `inspector_editors.cpp` uses `std::unordered_map` (line 84) without a direct `#include <unordered_map>`. It compiles because it is transitively included via `editor.h` → `scene/world.h` → other headers. The contract extends this usage with a new `std::unordered_map<const void*, DragState>` type. This works by transitive include, but an explicit `#include <unordered_map>` would be more robust. Pre-existing issue, not introduced by this contract.

- **`SDL_WarpMouseInWindow` return value ignored** — In SDL3, `SDL_WarpMouseInWindow` returns `bool` (true on success). The contract's `InputSystemSDL3::set_mouse_position` has `-> void` return type and ignores the SDL3 return value. This is acceptable (matching the spec's `-> void` interface) but means the implementation cannot detect or log warp failures.

- **`platform_sdl3.cpp` insertion point description** — The contract says "between line 174 and line 176" for the `set_sdl_window` call in `create_window()`. In the actual file, these line numbers correspond to the gap between the null-check closing brace and `SDL_SetWindowMinimumSize`. The conceptual location is correct, but the line number reference may shift.

## Required changes

Concrete, actionable changes requested:

1. **Fix include paths** in section 5a: Change `#include "engine/engine_service.h"` → `#include "engine_service.h"`, `#include "engine/input/input_system.h"` → `#include "input/input_system.h"`, `#include "engine/platform/platform.h"` → `#include "platform/platform.h"`.

## Suggested improvements

Optional ideas (not required):

- Add explicit `#include <unordered_map>` in `inspector_editors.cpp` to reduce reliance on transitive includes.
- Replace absolute line number references with descriptive markers throughout the contract.
- Add a note that `InputSystem::create(Backend)` factory (in `input_system.cpp`) will need a `set_sdl_window` wiring update if any test code creates `InputSystemSDL3` directly — though this is unlikely since the factory is the only creation path.
