# Code Review — Input System (SPEC-013 / IMPL-013)

## Blocking issues

No blocking issues found.

## Warnings

- **W-01: T-02 placed inside `#ifdef BUDDD_HAS_DISPLAY` (contract deviation).**
  The implementation contract lists T-02 (`"Factory creates SDL3 InputSystem"`) under "Headless tests (always runnable)" with tags `[headless][input]`. The implementation places it inside the `#ifdef BUDDD_HAS_DISPLAY` block alongside SDL3 integration tests. This is because the test requires `dynamic_cast<InputSystemSDL3*>` which needs the `InputSystemSDL3` type from `input/input_system_sdl3.h` (which includes `<SDL3/SDL.h>`). In the primary build configuration (`BUDDD_HAS_DISPLAY=ON`) the test runs and passes. In a hypothetical `BUDDD_HAS_DISPLAY=OFF` build, it would be excluded. This is a pragmatic, non-blocking deviation from the contract's test table.

- **W-02: `Platform::input_system()` lacks `[[nodiscard]]` (carried forward from contract-critic W-04).**
  The abstract `Platform::input_system()` returns `InputSystem&` but is not marked `[[nodiscard]]`. This is consistent with the contract's own code block (section 10 of IMPL-013) which also omits `[[nodiscard]]`. The contract's conventions table says "All query methods (non-void return) must be marked `[[nodiscard]]`", so this is a minor inconsistency with stated convention.

## Required changes

None.

## Suggested improvements

- **S-01: Consider adding `[[nodiscard]]` to `Platform::input_system()`** to match the contract's own convention that all non-void-returning query methods should be `[[nodiscard]]`.
- **S-02: Consider adding `#include <cstdint>` explicitly in `input_system.h`** for `uint8_t` usage in `MouseButton`, rather than relying on transitive inclusion from `key_code.h` (contract-critic W-05).

## Detailed findings

### Architecture and constitution compliance

| Check | Result | Notes |
|---|---|---|
| No SDL3 types in public headers (`key_code.h`, `input_system.h`) | ✅ PASS | `grep -E '(SDL_|SDL3)'` returns no matches |
| No SDL3 references in headless files | ✅ PASS | `grep -E 'SDL_'` returns no matches |
| Forbidden files unchanged | ✅ PASS | Only 6 modified + 8 new files from the allowed list |
| Constitution compliance (CONST-001) | ✅ PASS | Architecture boundary maintained |

### Code correctness

| Check | Result | Notes |
|---|---|---|
| KeyCode values match SDL_Scancode | ✅ PASS | A=4 through SuperRight=231, verified by reading source |
| `sizeof(KeyCode) == 1` | ✅ PASS | T-08 static_assert + REQUIRE |
| Factory returns correct types | ✅ PASS | T-01, T-02 dynamic_cast checks |
| Unknown backend returns InputInitFailed | ✅ PASS | T-04 verifies |
| Double-buffered state transitions | ✅ PASS | T-14 (SDL3) and T-07 (headless no-op) |
| begin_frame() resets delta/wheel | ✅ PASS | T-11, T-13 verify |
| Mouse button pressed/released | ✅ PASS | T-12 for all 5 buttons |
| Rapid down+up in same frame | ✅ PASS | T-16: both events processed, no net change |
| Static_cast round-trip | ✅ PASS | T-17 for representative keys |
| Headless returns defaults | ✅ PASS | T-03 for all queries |
| Platform integration | ✅ PASS | T-05 (Headless), T-15 (SDL3) |

### Test coverage

| Test ID | Name | Status |
|---|---|---|
| T-01 | Factory creates Headless InputSystem | ✅ PASS |
| T-02 | Factory creates SDL3 InputSystem | ✅ PASS (inside `#ifdef BUDDD_HAS_DISPLAY`, see W-01) |
| T-03 | Headless InputSystem returns defaults | ✅ PASS |
| T-04 | Factory unknown backend returns InputInitFailed | ✅ PASS |
| T-05 | Headless Platform input_system() returns valid ref | ✅ PASS |
| T-06 | Headless begin_frame() does not crash | ✅ PASS |
| T-07 | Double-buffered state transitions (standalone) | ✅ PASS |
| T-08 | sizeof(KeyCode) == 1 | ✅ PASS |
| T-09 | MouseButton values exist | ✅ PASS |
| T-10 | SDL3 key down/up detected via InputSystem | ✅ PASS |
| T-11 | SDL3 mouse motion updates position and delta | ✅ PASS |
| T-12 | SDL3 mouse button pressed/released | ✅ PASS |
| T-13 | SDL3 mouse wheel accumulates correctly | ✅ PASS |
| T-14 | SDL3 multi-frame key transitions | ✅ PASS |
| T-15 | SDL3 Platform provides valid InputSystem reference | ✅ PASS |
| T-16 | SDL3 rapid down+up in same frame | ✅ PASS |
| T-17 | KeyCode static_cast round-trip | ✅ PASS |

### Build and warnings

| Check | Result | Notes |
|---|---|---|
| Build succeeds | ✅ PASS | `cmake --build --preset debug` — no errors |
| No input-system-related compilation warnings | ✅ PASS | Build output checked |
| Full test suite passes | ✅ PASS | 230 test cases, 12359 assertions, all pass |
| New files discovered by GLOB_RECURSE | ✅ PASS | No CMakeLists.txt changes needed |

### Frame-based state model verification

The `begin_frame()` implementation correctly:
1. Copies `current_keys_` → `previous_keys_` (array assignment)
2. Copies `current_mouse_buttons_` → `previous_mouse_buttons_` (array assignment)
3. Resets `delta_x_`, `delta_y_` to 0.0f
4. Resets `wheel_x_`, `wheel_y_` to 0.0f
5. Does NOT reset `mouse_x_`, `mouse_y_` (position persists)
6. Does NOT clear `current_keys_` or `current_mouse_buttons_` (held state persists)

The `is_pressed`/`is_released` semantics use the standard `current && !previous` / `!current && previous` pattern. Verified by T-14 (multi-frame key transitions) and T-16 (rapid down+up in same frame).

### Event routing in Platform

- `PlatformSDL3::poll_events()`: calls `input_system_.begin_frame()` before SDL_PollEvent loop, routes non-quit events to `input_system_.on_sdl_event(event)`. ✅
- `PlatformHeadless::poll_events()`: calls `input_system_.begin_frame()` before returning true. ✅

### Friend declarations

The implementation adds `friend class PlatformSDL3;` and `friend class PlatformHeadless;` to the concrete backend classes. These are necessary because the Platform backends embed the input system as members (value semantics), and the constructors of the backends are private (for factory access). The contract's code blocks did not show these friend declarations, but they are architecturally necessary and consistent with the embedded member pattern described in the spec and contract. The implementer documented this correctly in coordination.md warnings.
