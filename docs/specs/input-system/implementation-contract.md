# IMPL-013 — Input System: Keyboard & Mouse Abstraction

## Status

`Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|
| Approved by | Guillaume (human) |
| Date | 2026-05-30 |
| Time | (human-validation time) |

## Source spec

`docs/specs/input-system/spec.md` (SPEC-013), accepted (`docs/specs/input-system/spec-critic.md` verdict: `Accept` — both blocking issues resolved, spec ready for implementation-contract authoring).

Non-blocking warnings carried forward and addressed by this contract:

| Warning | Resolution in this contract |
|---|---|
| **W-01** — Missing include dependency in `InputSystem` class code example (no `#include "platform/platform.h"` for `Backend` enum) | Required implementation behavior specifies the exact include: `#include "platform/platform.h"` in `input_system.cpp` (the factory file) and forward declaration of `Backend` in `input_system.h`. |
| **W-02** — `InputInitFailed` added to `Error::Category` but never used in any error path | This contract uses `InputInitFailed` in the `InputSystem::create()` factory for the forward-compat error path, replacing `Unsupported` (see Required implementation behavior section 3). |
| **W-03** — AC-012/AC-013 verification of `begin_frame()` call is imprecise | Verification is through indirect observation: call `poll_events()` without events, then verify `is_pressed()` returns false for any key (confirms `begin_frame()` ran without crashing). Headless backend has an intentionally observable effect: `begin_frame()` is called and the platform returns `true`. |
| **W-04** — AC-015 "mapping must be complete" wording is ambiguous | Clarified as: "All defined `KeyCode` values match their corresponding `SDL_Scancode` numeric values — verified by `static_cast`, no mapping function needed." |
| **W-05** — AC-006–AC-009 use `SDL_PushEvent()` which requires `<SDL3/SDL.h>` in test files, exceeding AMEND-2026-001 scope | Resolved by **expanding AMEND-2026-001** to allow `SDL_PushEvent()` calls in test files. The constitutional amendment will be updated by the constitution-agent in a later workflow step. Test files include `<SDL3/SDL.h>` for both `SDL_SetHint()` and `SDL_PushEvent()` usage (see Required implementation behavior section 9). |

## Goal

Implement the Input System for the Buddd Engine, providing:

1. An abstract `InputSystem` interface in `src/engine/input/input_system.h` with pure virtual methods for keyboard and mouse state queries (`is_down`, `is_pressed`, `is_released`, `mouse_position`, `mouse_delta`, `mouse_wheel`, `is_mouse_down`, `is_mouse_pressed`, `is_mouse_released`).
2. A `KeyCode` enum class in `src/engine/input/key_code.h` (engine-level, `uint8_t`, ~65 values including A–Z, Digit0–9, Space, Escape, Enter, Tab, ShiftLeft/Right, ControlLeft/Right, AltLeft/Right, SuperLeft/Right, ArrowUp/Down/Left/Right, F1–F12, Backspace, Delete, Insert, CapsLock, Grave, Minus, Equals, BracketLeft/Right, Semicolon, Quote, Comma, Period, Slash, Backslash).
3. A `MouseButton` enum class in `input_system.h` (`uint8_t`, values: Left, Right, Middle, X1, X2).
4. A concrete SDL3 backend (`InputSystemSDL3`) that processes SDL keyboard, mouse-motion, mouse-button, and mouse-wheel events via a private `on_sdl_event()` method, converting SDL scancodes to `KeyCode` via `static_cast` with bounds check.
5. A concrete headless backend (`InputSystemHeadless`) returning zero/false defaults for all queries — enabling tests to run without a display.
6. Integration into the `Platform` abstraction: `Platform` gains `virtual auto input_system() -> InputSystem& = 0`; each concrete platform backend owns an embedded input backend.
7. A frame-based, double-buffered state model: `begin_frame()` copies current→previous state arrays and resets mouse delta/wheel accumulators.
8. Factory `InputSystem::create(Backend)` returning `Result<std::unique_ptr<InputSystem>>` (for standalone construction in tests/tools).
9. New `Error::Category::InputInitFailed` value in `error.h`.

## Non-goals

- No gamepad or controller support.
- No touch input.
- No text input or IME (SDL text events).
- No input remapping or rebinding — `KeyCode` values are fixed to physical key positions (SDL scancode mapping).
- No input action system (e.g., "Jump" action mapped to a key).
- No joystick or hotplug detection.
- No raw input or relative mouse mode.
- No clipboard events (`SDL_EVENT_CLIPBOARD_UPDATE`).
- No drop events (`SDL_EVENT_DROP_FILE`, etc.).
- No dynamic backend switching after Platform construction.
- No support for multiple mice or keyboards.
- No focus-aware input filtering.
- No repeat-key events.
- No `const` overload of `input_system()` on `Platform`.
- No changes to `src/engine/CMakeLists.txt` (new files are picked up by existing `GLOB_RECURSE`).
- No changes to `src/cmd/`, `src/editor/`, root `CMakeLists.txt`, `CMakePresets.json`, or any file outside `src/engine/` and `tests/input_tests.cpp`.
- AMEND-2026-001 expansion to allow `SDL_PushEvent()` in test files is **not** done by the Code Agent; it will be handled by the constitution-agent in a later workflow step.

## Relevant constitution rules

- **CONST-001-architecture-boundaries.md**: Enforces the architecture boundary — no SDL3 types appear in public headers (`key_code.h`, `input_system.h`). Only the concrete SDL3 backend files include `<SDL3/SDL.h>`. AMEND-2026-001 will be expanded by the constitution-agent to allow `SDL_PushEvent()` and related SDL3 API calls in test files. AC-018 explicitly verifies the boundary.
- **CONST-002-testing-policy.md**: Requires unit tests for all testable code. This contract specifies required tests (see Required tests section).

## Relevant ADRs

- **ADR-001** (`docs/adr/001-result-error-pattern.md`): Establishes `Result<T>` / `Error` as the project-wide error handling pattern. `InputSystem::create()` returns `Result<std::unique_ptr<InputSystem>>`. Query methods (non-fallible) return plain values.
- **ADR-003** (`docs/adr/003-render-pipeline-architecture.md`): Establishes the `Platform::poll_events()` pattern, which this contract extends by calling `begin_frame()` before the SDL_PollEvent loop and routing events to the input system.
- **ADR-007** (`docs/adr/007-release-dependency-build.md`): Establishes `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` for SDL3 FetchContent. No changes needed — `GLOB_RECURSE` picks up new source files without CMakeLists.txt modification.

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/error.h` | Current `Error::Category` enum — must add `InputInitFailed`. |
| `src/engine/platform/platform.h` | Current abstract `Platform` — must add `virtual auto input_system() -> InputSystem& = 0`. |
| `src/engine/platform/platform_sdl3.h` | Current SDL3 Platform header — must add `InputSystemSDL3` member and `input_system()` override. |
| `src/engine/platform/platform_sdl3.cpp` | Current SDL3 Platform impl — must update `poll_events()` to call `begin_frame()` and route events. |
| `src/engine/platform/platform_headless.h` | Current Headless Platform header — must add `InputSystemHeadless` member and `input_system()` override. |
| `src/engine/platform/platform_headless.cpp` | Current Headless Platform impl — must update `poll_events()` to call `begin_frame()`. |
| `tests/sdl3_backend_tests.cpp` | Existing test pattern (AMEND-2026-001 exception usage) for reference. |
| `tests/platform_abstraction_tests.cpp` | Existing test pattern for headless platform tests. |
| `src/engine/CMakeLists.txt` | Current CMake — confirms `GLOB_RECURSE` picks up new files automatically. |
| `docs/specs/platform-abstraction/implementation-contract.md` | Style reference (IMPL-002) for contract format and level of detail. |
| `docs/specs/render-pipeline/implementation-contract.md` | Style reference (IMPL-005) for contract format and level of detail. |

## Files allowed to change

### New files to create (8 files)

All paths are relative to the repository root.

| # | File | Description |
|---|---|---|
| 1 | `src/engine/input/key_code.h` | Public header — `KeyCode` enum class |
| 2 | `src/engine/input/input_system.h` | Public header — abstract `InputSystem`, `MouseButton` enum, `Backend` forward decl |
| 3 | `src/engine/input/input_system.cpp` | Factory implementation — `InputSystem::create(Backend)` |
| 4 | `src/engine/input/input_system_sdl3.h` | Private header — `InputSystemSDL3` concrete class |
| 5 | `src/engine/input/input_system_sdl3.cpp` | SDL3 backend — event processing, state management, scancode conversion via static_cast |
| 6 | `src/engine/input/input_system_headless.h` | Private header — `InputSystemHeadless` concrete class |
| 7 | `src/engine/input/input_system_headless.cpp` | Headless backend — stub implementations |
| 8 | `tests/input_tests.cpp` | Test file — all input system tests |

### Files to modify (6 files)

| # | File | Change description |
|---|---|---|
| 11 | `src/engine/error.h` | Add `InputInitFailed` to `Error::Category` enum and update `to_string()` |
| 12 | `src/engine/platform/platform.h` | Add `virtual auto input_system() -> InputSystem& = 0` |
| 13 | `src/engine/platform/platform_sdl3.h` | Add `InputSystemSDL3 input_system_` member, `input_system()` override declaration |
| 14 | `src/engine/platform/platform_sdl3.cpp` | Update `poll_events()` — call `input_system_.begin_frame()` before event loop, route non-quit events |
| 15 | `src/engine/platform/platform_headless.h` | Add `InputSystemHeadless input_system_` member, `input_system()` override declaration |
| 16 | `src/engine/platform/platform_headless.cpp` | Update `poll_events()` — call `input_system_.begin_frame()` at start |

Total: 8 new files + 6 modified files = 14 files changed.

## Files forbidden to change

- Any file outside `src/engine/` or `tests/` (except `tests/input_tests.cpp` which is explicitly created).
- `src/engine/version.h`, `src/engine/version.cpp`.
- `src/engine/CMakeLists.txt` — `GLOB_RECURSE` picks up new files automatically; no changes needed.
- Root `CMakeLists.txt`, `CMakePresets.json`.
- `src/cmd/` (any file).
- `src/editor/` (any file).
- `docs/` (any file not listed above — includes `docs/adr/`, `docs/constitution/`, `docs/wiki/`, all other spec/contract directories).
- `.clang-format`, `.vscode/` (any file).
- `AGENTS.md`, `opencode.json`, `SpecKit.md`.

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Namespace | `buddd::engine` for all public types. Concrete backend classes use same namespace (`buddd::engine`). |
| File naming | `snake_case` (lowercase ASCII letters, digits, underscores). |
| Directory naming | `snake_case`. Create `src/engine/input/`. |
| Class naming | PascalCase (e.g., `InputSystemSDL3`, `InputSystemHeadless`). |
| Enum naming | PascalCase for enum class names and values (e.g., `KeyCode::Escape`, `MouseButton::Left`). |
| Header guards | `#pragma once` (no `#ifndef` guards). |
| Function style | Trailing return type syntax (`auto foo() -> int`). |
| `[[nodiscard]]` | All query methods (non-void return) must be marked `[[nodiscard]]`. |
| Non-copyable, non-movable | `InputSystem` (abstract), all concrete backends, must have copy/move constructors and assignment operators `= delete`. |
| Formatting | `.clang-format` at repository root enforces LLVM style, 4-space indent, 100 column limit. |
| Includes | Standard library includes use `<>`; project includes use `""` relative to `src/engine/`. |
| Error handling | Factory methods return `Result<T>`. Use `make_error()` to return errors. |
| Observability | Use `std::cerr` for lifecycle events, matching SPEC-002/SPEC-005 conventions. |
| SDL3 includes | Use `#include <SDL3/SDL.h>` — inside `src/engine/` (private headers, `.cpp` files), and in `tests/input_tests.cpp` under `#ifdef BUDDD_HAS_DISPLAY` per expanded AMEND-2026-001. |
| No backend types in abstract headers | `key_code.h`, `input_system.h` must not include `<SDL3/` or any backend-specific header. |
| Test file guard | `tests/input_tests.cpp` conditionally compiled with `#ifdef BUDDD_HAS_DISPLAY` for SDL3 tests; headless tests are always compiled. |

## Required implementation behavior

### 1. `src/engine/input/key_code.h` — `KeyCode` enum

Must contain:

```cpp
#pragma once

#include <cstdint>

namespace buddd::engine {

/// Engine-level key code enum mapping physical key positions.
/// Values are fixed to SDL scancode positions (not language-dependent keycodes).
/// Underlying type is uint8_t (max 256 values, extensible).
enum class KeyCode : uint8_t {
    Unknown = 0,

    // Letters (values match SDL_Scancode: SDL_SCANCODE_A=4 ... SDL_SCANCODE_Z=29)
    A = 4,  B = 5,  C = 6,  D = 7,  E = 8,  F = 9,  G = 10,
    H = 11, I = 12, J = 13, K = 14, L = 15, M = 16,
    N = 17, O = 18, P = 19, Q = 20, R = 21, S = 22,
    T = 23, U = 24, V = 25, W = 26, X = 27, Y = 28, Z = 29,

    // Digits (top row, SDL_SCANCODE_1=30 ... SDL_SCANCODE_0=39)
    Digit1 = 30, Digit2 = 31, Digit3 = 32, Digit4 = 33, Digit5 = 34,
    Digit6 = 35, Digit7 = 36, Digit8 = 37, Digit9 = 38, Digit0 = 39,

    // Common control keys (SDL values)
    Enter     = 40,
    Escape    = 41,
    Backspace = 42,
    Tab       = 43,
    Space     = 44,

    // Punctuation and symbol keys (SDL values)
    Minus       = 45,  // - and _
    Equals      = 46,  // = and +
    BracketLeft = 47,  // [ and {
    BracketRight = 48, // ] and }
    Backslash   = 49,  // \ and |
    Semicolon   = 51,  // ; and :
    Quote       = 52,  // ' and "
    Grave       = 53,  // ` and ~ (key above Tab)
    Comma       = 54,  // , and <
    Period      = 55,  // . and >
    Slash       = 56,  // / and ?
    CapsLock    = 57,

    // Function keys (SDL_SCANCODE_F1=58 ... SDL_SCANCODE_F12=69)
    F1 = 58,  F2 = 59,  F3 = 60,  F4 = 61,  F5 = 62,
    F6 = 63,  F7 = 64,  F8 = 65,  F9 = 66,  F10 = 67,
    F11 = 68, F12 = 69,

    // Navigation & editing (SDL values)
    Delete = 76,
    Right  = 79, Left = 80, Down = 81, Up = 82,
    Insert = 93,

    // Modifier keys (left and right distinguished, SDL values)
    ControlLeft  = 224, ShiftLeft  = 225, AltLeft  = 226, SuperLeft  = 227,
    ControlRight = 228, ShiftRight = 229, AltRight = 230, SuperRight = 231,

    // Sentinel value for array sizing — must remain last.
    _Count
};

} // namespace buddd::engine
```

**Requirements:**
- Underlying type `uint8_t`.
- `_Count` is the last enumerator, used for sizing arrays (`static_cast<size_t>(KeyCode::_Count)`).
- `Unknown = 0` — zero-initialised memory maps to Unknown.
- No SDL3 types or includes in this file.
- `sizeof(KeyCode) == 1`.

### 2. `src/engine/input/input_system.h` — Abstract `InputSystem`, `MouseButton` enum

Must contain:

```cpp
#pragma once

#include "error.h"
#include "input/key_code.h"

#include <memory>
#include <utility>

namespace buddd::engine {

enum class Backend;  // Forward-declared from platform/platform.h

enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    X1,   // Forward / thumb button 1
    X2    // Back / thumb button 2
};

class InputSystem {
public:
    /// Creates an InputSystem with the given backend.
    /// Returns InputInitFailed for unknown backends (forward-compat).
    [[nodiscard]] static auto create(Backend backend) -> Result<std::unique_ptr<InputSystem>>;

    virtual ~InputSystem() = default;

    // ── Frame lifecycle ──
    /// Must be called once per frame before any queries.
    /// Copies current state to previous (for pressed/released detection),
    /// and resets accumulated frame values (mouse delta, wheel).
    /// Called automatically by the owning Platform's poll_events().
    virtual auto begin_frame() -> void = 0;

    // ── Keyboard state ──
    [[nodiscard]] virtual auto is_down(KeyCode key) const noexcept -> bool = 0;
    [[nodiscard]] virtual auto is_pressed(KeyCode key) const noexcept -> bool = 0;
    [[nodiscard]] virtual auto is_released(KeyCode key) const noexcept -> bool = 0;

    // ── Mouse state ──
    /// Absolute mouse position in window coordinates (origin top-left).
    [[nodiscard]] virtual auto mouse_position() const noexcept -> std::pair<float, float> = 0;
    /// Relative mouse motion accumulated since the last begin_frame() call.
    [[nodiscard]] virtual auto mouse_delta() const noexcept -> std::pair<float, float> = 0;
    /// Mouse wheel scroll accumulated since the last begin_frame() call.
    /// Positive Y = scroll up / away from user (natural scrolling).
    [[nodiscard]] virtual auto mouse_wheel() const noexcept -> std::pair<float, float> = 0;

    [[nodiscard]] virtual auto is_mouse_down(MouseButton button) const noexcept -> bool = 0;
    [[nodiscard]] virtual auto is_mouse_pressed(MouseButton button) const noexcept -> bool = 0;
    [[nodiscard]] virtual auto is_mouse_released(MouseButton button) const noexcept -> bool = 0;

    InputSystem(const InputSystem&) = delete;
    auto operator=(const InputSystem&) -> InputSystem& = delete;
    InputSystem(InputSystem&&) = delete;
    auto operator=(InputSystem&&) -> InputSystem& = delete;

protected:
    InputSystem() = default;
};

} // namespace buddd::engine
```

**Requirements:**
- Forward-declares `enum class Backend` (defined in `platform/platform.h`). Do NOT include `platform/platform.h` in this header — the forward declaration is sufficient for the factory declaration.
- `InputSystem` is abstract with exactly 11 pure virtual methods.
- All query methods are `[[nodiscard]]`.
- Non-copyable, non-movable.
- Protected default constructor.

### 3. `src/engine/input/input_system.cpp` — Factory implementation

Must contain:

```cpp
#include "input_system.h"
#include "input_system_sdl3.h"
#include "input_system_headless.h"

#include "platform/platform.h"   // for Backend enum definition

#include <iostream>

namespace buddd::engine {

auto InputSystem::create(Backend backend) -> Result<std::unique_ptr<InputSystem>> {
    switch (backend) {
        case Backend::SDL3: {
            std::cerr << "InputSystem backend: SDL3\n";
            return std::unique_ptr<InputSystem>(new InputSystemSDL3());
        }
        case Backend::Headless: {
            std::cerr << "InputSystem backend: Headless\n";
            return std::unique_ptr<InputSystem>(new InputSystemHeadless());
        }
    }
    return make_error(Error::Category::InputInitFailed, "Unknown backend");
}

} // namespace buddd::engine
```

**Requirements:**
- Must `#include "platform/platform.h"` for the full `Backend` enum definition.
- Both `InputSystemSDL3` and `InputSystemHeadless` constructors must be reachable (they declare `friend auto InputSystem::create(Backend) -> Result<std::unique_ptr<InputSystem>>;`).
- Observability: print `"InputSystem backend: SDL3\n"` or `"InputSystem backend: Headless\n"`.
- Returns `InputInitFailed` (not `InitFailed`) for unknown backend values.

### 4. `src/engine/input/input_system_sdl3.h` — SDL3 backend header

Must contain:

```cpp
#pragma once

#include "input_system.h"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>

namespace buddd::engine {

class InputSystemSDL3 final : public InputSystem {
public:
    ~InputSystemSDL3() override = default;

    // ── InputSystem overrides ──
    auto begin_frame() -> void override;

    [[nodiscard]] auto is_down(KeyCode key) const noexcept -> bool override;
    [[nodiscard]] auto is_pressed(KeyCode key) const noexcept -> bool override;
    [[nodiscard]] auto is_released(KeyCode key) const noexcept -> bool override;

    [[nodiscard]] auto mouse_position() const noexcept -> std::pair<float, float> override;
    [[nodiscard]] auto mouse_delta() const noexcept -> std::pair<float, float> override;
    [[nodiscard]] auto mouse_wheel() const noexcept -> std::pair<float, float> override;

    [[nodiscard]] auto is_mouse_down(MouseButton button) const noexcept -> bool override;
    [[nodiscard]] auto is_mouse_pressed(MouseButton button) const noexcept -> bool override;
    [[nodiscard]] auto is_mouse_released(MouseButton button) const noexcept -> bool override;

    InputSystemSDL3(const InputSystemSDL3&) = delete;
    auto operator=(const InputSystemSDL3&) -> InputSystemSDL3& = delete;
    InputSystemSDL3(InputSystemSDL3&&) = delete;
    auto operator=(InputSystemSDL3&&) -> InputSystemSDL3& = delete;

private:
    friend auto InputSystem::create(Backend) -> Result<std::unique_ptr<InputSystem>>;
    InputSystemSDL3() = default;

    /// Called by PlatformSDL3::poll_events() for each non-quit SDL event.
    /// Processes keyboard, mouse-motion, mouse-button, and mouse-wheel events.
    /// Not part of the abstract InputSystem interface.
    void on_sdl_event(const SDL_Event& event);

    // ── State (double-buffered) ──
    static constexpr size_t kKeyCount = static_cast<size_t>(KeyCode::_Count);
    static constexpr size_t kMouseButtonCount = 5;

    std::array<bool, kKeyCount> current_keys_{};
    std::array<bool, kKeyCount> previous_keys_{};

    std::array<bool, kMouseButtonCount> current_mouse_buttons_{};
    std::array<bool, kMouseButtonCount> previous_mouse_buttons_{};

    float mouse_x_{0.0f};
    float mouse_y_{0.0f};
    float delta_x_{0.0f};
    float delta_y_{0.0f};
    float wheel_x_{0.0f};
    float wheel_y_{0.0f};
};

} // namespace buddd::engine
```

**Requirements:**
- `on_sdl_event()` is private and non-virtual — called only by `PlatformSDL3::poll_events()`.
- State arrays are `std::array<bool, N>` with value-initialisation (`{}`) to zero/false.
- `kKeyCount` = `static_cast<size_t>(KeyCode::_Count)`.
- `kMouseButtonCount` = 5 (Left=0, Right=1, Middle=2, X1=3, X2=4).
- All member fields are initialised to zero/false in the member initialiser list (default member initialisers as shown).
- Private default constructor, `friend` for factory.

### 5. `src/engine/input/input_system_sdl3.cpp` — SDL3 backend implementation

**`begin_frame()` algorithm:**

```cpp
auto InputSystemSDL3::begin_frame() -> void {
    // Copy current→previous for all keys
    previous_keys_ = current_keys_;

    // Copy current→previous for all mouse buttons
    previous_mouse_buttons_ = current_mouse_buttons_;

    // Reset accumulated frame values
    delta_x_ = 0.0f;
    delta_y_ = 0.0f;
    wheel_x_ = 0.0f;
    wheel_y_ = 0.0f;

    // NOTE: mouse position (mouse_x_, mouse_y_) is NOT reset — it persists.
    // NOTE: current key/button state is NOT cleared — held keys stay held.
}
```

**`is_down`, `is_pressed`, `is_released` implementations:**

```cpp
auto InputSystemSDL3::is_down(KeyCode key) const noexcept -> bool {
    auto idx = static_cast<size_t>(key);
    if (idx >= kKeyCount) return false;
    return current_keys_[idx];
}

auto InputSystemSDL3::is_pressed(KeyCode key) const noexcept -> bool {
    auto idx = static_cast<size_t>(key);
    if (idx >= kKeyCount) return false;
    return current_keys_[idx] && !previous_keys_[idx];
}

auto InputSystemSDL3::is_released(KeyCode key) const noexcept -> bool {
    auto idx = static_cast<size_t>(key);
    if (idx >= kKeyCount) return false;
    return !current_keys_[idx] && previous_keys_[idx];
}
```

**Mouse query implementations:**

```cpp
auto InputSystemSDL3::mouse_position() const noexcept -> std::pair<float, float> {
    return {mouse_x_, mouse_y_};
}

auto InputSystemSDL3::mouse_delta() const noexcept -> std::pair<float, float> {
    return {delta_x_, delta_y_};
}

auto InputSystemSDL3::mouse_wheel() const noexcept -> std::pair<float, float> {
    return {wheel_x_, wheel_y_};
}
```

**Mouse button queries (same pattern as keyboard):**

```cpp
auto InputSystemSDL3::is_mouse_down(MouseButton button) const noexcept -> bool {
    auto idx = static_cast<size_t>(button);
    if (idx >= kMouseButtonCount) return false;
    return current_mouse_buttons_[idx];
}

auto InputSystemSDL3::is_mouse_pressed(MouseButton button) const noexcept -> bool {
    auto idx = static_cast<size_t>(button);
    if (idx >= kMouseButtonCount) return false;
    return current_mouse_buttons_[idx] && !previous_mouse_buttons_[idx];
}

auto InputSystemSDL3::is_mouse_released(MouseButton button) const noexcept -> bool {
    auto idx = static_cast<size_t>(button);
    if (idx >= kMouseButtonCount) return false;
    return !current_mouse_buttons_[idx] && previous_mouse_buttons_[idx];
}
```

**`on_sdl_event()` implementation:**

```cpp
void InputSystemSDL3::on_sdl_event(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN: {
            // Ignore key repeats (SDL_EVENT_KEY_DOWN with repeat flag, though
            // SDL3 sends SDL_EVENT_KEY_DOWN for every physical press/repeat).
            // We track physical state, so we update on every key-down event.
            // SDL3 does not set a 'repeat' field on KEY_DOWN in the same way
            // as SDL2; we process every KEY_DOWN as a physical down.
            // Convert via static_cast — KeyCode values match SDL_Scancode values.
            auto scancode = event.key.scancode;
            if (scancode > SDL_SCANCODE_UNKNOWN && scancode < static_cast<SDL_Scancode>(KeyCode::_Count)) {
                auto idx = static_cast<size_t>(scancode);
                current_keys_[idx] = true;
            } else {
                // Unknown scancode — ignore (debug builds print warning below)
            }
            break;
        }
        case SDL_EVENT_KEY_UP: {
            auto scancode = event.key.scancode;
            if (scancode > SDL_SCANCODE_UNKNOWN && scancode < static_cast<SDL_Scancode>(KeyCode::_Count)) {
                auto idx = static_cast<size_t>(scancode);
                current_keys_[idx] = false;
            } else {
                // Unknown scancode — ignore (debug builds print warning below)
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            mouse_x_ = static_cast<float>(event.motion.x);
            mouse_y_ = static_cast<float>(event.motion.y);
            delta_x_ += event.motion.xrel;
            delta_y_ += event.motion.yrel;
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            auto button = sdl_button_to_mouse_button(event.button.button);
            auto idx = static_cast<size_t>(button);
            if (idx < kMouseButtonCount) {
                current_mouse_buttons_[idx] = true;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            auto button = sdl_button_to_mouse_button(event.button.button);
            auto idx = static_cast<size_t>(button);
            if (idx < kMouseButtonCount) {
                current_mouse_buttons_[idx] = false;
            }
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL: {
            wheel_x_ += event.wheel.x;
            wheel_y_ += event.wheel.y;
            break;
        }
        default:
            break;  // Ignore unhandled event types
    }
}
```

**SDL scancode → KeyCode conversion** (no separate mapping function):

No separate `sdl_scancode_to_key_code()` function exists. `KeyCode` enum values match `SDL_Scancode` values numerically. Conversion is done inline in `on_sdl_event()` via `static_cast<KeyCode>(scancode)` with a bounds check — no lookup table or switch statement needed.

The bounds check pattern used in both `SDL_EVENT_KEY_DOWN` and `SDL_EVENT_KEY_UP`:

```cpp
auto scancode = event.key.scancode;
if (scancode > SDL_SCANCODE_UNKNOWN && scancode < static_cast<SDL_Scancode>(KeyCode::_Count)) {
    auto idx = static_cast<size_t>(scancode);
    current_keys_[idx] = <true|false>;
}
```

Unknown scancodes (those at or below `SDL_SCANCODE_UNKNOWN` or at or above `KeyCode::_Count`) are silently ignored. Debug builds additionally print a warning to `std::cerr` (see Observability section below).

**SDL mouse button → MouseButton mapping** (private helper in anonymous namespace):

```cpp
auto sdl_button_to_mouse_button(uint8_t sdl_button) -> MouseButton {
    switch (sdl_button) {
        case SDL_BUTTON_LEFT:   return MouseButton::Left;
        case SDL_BUTTON_RIGHT:  return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_X1:     return MouseButton::X1;
        case SDL_BUTTON_X2:     return MouseButton::X2;
        default:                return MouseButton::Left;  // Default to Left for unknown
    }
}
```

**Observability (in debug builds):** Unrecognised scancodes (those failing the bounds check) print `"InputSystemSDL3: unrecognised scancode <N>\n"` to `std::cerr`, guarded by `#ifndef NDEBUG`, inside the `else` branch of the inline bounds check.

### 6. `src/engine/input/input_system_headless.h` — Headless backend header

Must contain:

```cpp
#pragma once

#include "input_system.h"

namespace buddd::engine {

class InputSystemHeadless final : public InputSystem {
public:
    ~InputSystemHeadless() override = default;

    auto begin_frame() -> void override;

    [[nodiscard]] auto is_down(KeyCode) const noexcept -> bool override;
    [[nodiscard]] auto is_pressed(KeyCode) const noexcept -> bool override;
    [[nodiscard]] auto is_released(KeyCode) const noexcept -> bool override;

    [[nodiscard]] auto mouse_position() const noexcept -> std::pair<float, float> override;
    [[nodiscard]] auto mouse_delta() const noexcept -> std::pair<float, float> override;
    [[nodiscard]] auto mouse_wheel() const noexcept -> std::pair<float, float> override;

    [[nodiscard]] auto is_mouse_down(MouseButton) const noexcept -> bool override;
    [[nodiscard]] auto is_mouse_pressed(MouseButton) const noexcept -> bool override;
    [[nodiscard]] auto is_mouse_released(MouseButton) const noexcept -> bool override;

    InputSystemHeadless(const InputSystemHeadless&) = delete;
    auto operator=(const InputSystemHeadless&) -> InputSystemHeadless& = delete;
    InputSystemHeadless(InputSystemHeadless&&) = delete;
    auto operator=(InputSystemHeadless&&) -> InputSystemHeadless& = delete;

private:
    friend auto InputSystem::create(Backend) -> Result<std::unique_ptr<InputSystem>>;
    InputSystemHeadless() = default;
};

} // namespace buddd::engine
```

### 7. `src/engine/input/input_system_headless.cpp` — Headless backend implementation

```cpp
#include "input_system_headless.h"

namespace buddd::engine {

auto InputSystemHeadless::begin_frame() -> void {
    // No-op: no state to update in headless mode.
}

auto InputSystemHeadless::is_down(KeyCode) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::is_pressed(KeyCode) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::is_released(KeyCode) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::mouse_position() const noexcept -> std::pair<float, float> {
    return {0.0f, 0.0f};
}

auto InputSystemHeadless::mouse_delta() const noexcept -> std::pair<float, float> {
    return {0.0f, 0.0f};
}

auto InputSystemHeadless::mouse_wheel() const noexcept -> std::pair<float, float> {
    return {0.0f, 0.0f};
}

auto InputSystemHeadless::is_mouse_down(MouseButton) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::is_mouse_pressed(MouseButton) const noexcept -> bool {
    return false;
}

auto InputSystemHeadless::is_mouse_released(MouseButton) const noexcept -> bool {
    return false;
}

} // namespace buddd::engine
```

**Requirements:**
- NO `#include <SDL3/SDL.h>` in any headless file.
- All query methods return zero/false defaults unconditionally.

### 8. `src/engine/error.h` — Add `InputInitFailed`

**Add to `Error::Category` enum** (before `Unsupported`, preserving alphabetical order or adding at the end before `Unsupported` as per project convention):

```cpp
enum class Category {
    InitFailed,
    WindowCreationFailed,
    RenderDeviceCreationFailed,
    ShaderCompilationFailed,
    LinkingFailed,
    ResourceCreationFailed,
    InvalidArgument,
    UniformNotFound,
    ReadbackFailed,
    IoFailed,
    InputInitFailed,      // <-- NEW: Input system initialisation failure
    Unsupported,
    Unknown
};
```

**Add to `to_string()` switch:**

```cpp
case Error::Category::InputInitFailed: category_str = "InputInitFailed"; break;
```

**Requirements:**
- `InputInitFailed` is inserted before `Unsupported` (at the end of the non-sentinel values).
- All existing values remain unchanged.
- `to_string()` handles the new value.

### 9. `tests/input_tests.cpp` — Direct `SDL_PushEvent()` usage (via expanded AMEND-2026-001)

This section replaces the previously planned engine-side test helper approach. Following the human decision, AMEND-2026-001 will be **expanded** (by the constitution-agent in a later step) to allow SDL3 API calls in test files when testing SDL3-dependent engine functionality.

**Test file approach for synthetic events:**

Synthetic SDL events are injected by calling `SDL_PushEvent()` directly in `tests/input_tests.cpp`, using `<SDL3/SDL.h>` included in the `#ifdef BUDDD_HAS_DISPLAY` block. The test file defines local helper functions for constructing and pushing events, including a `KeyCode`→`SDL_Scancode` conversion function using `static_cast`.

**KeyCode→SDL_Scancode conversion (local to `tests/input_tests.cpp`):**

```cpp
namespace {

/// Convert KeyCode to SDL_Scancode.
/// Values match 1:1 — this is a simple cast.
/// No lookup table needed because KeyCode values equal SDL_Scancode values.
auto key_code_to_sdl_scancode(KeyCode key) -> SDL_Scancode {
    auto sc = static_cast<SDL_Scancode>(key);
    // Verify the KeyCode value is valid (not Unknown or _Count)
    REQUIRE(sc > SDL_SCANCODE_UNKNOWN);
    return sc;
}

auto mouse_button_to_sdl(MouseButton button) -> uint8_t {
    switch (button) {
        case MouseButton::Left:   return SDL_BUTTON_LEFT;
        case MouseButton::Right:  return SDL_BUTTON_RIGHT;
        case MouseButton::Middle: return SDL_BUTTON_MIDDLE;
        case MouseButton::X1:     return SDL_BUTTON_X1;
        case MouseButton::X2:     return SDL_BUTTON_X2;
    }
    return SDL_BUTTON_LEFT;
}

} // anonymous namespace
```

**Synthetic event helpers (local to `tests/input_tests.cpp`, `#ifdef BUDDD_HAS_DISPLAY` block):**

```cpp
static auto push_key_event(KeyCode key, bool down) -> bool {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = key_code_to_sdl_scancode(key);
    event.key.down = down;
    event.key.repeat = false;
    return SDL_PushEvent(&event);
}

static auto push_mouse_motion_event(float x, float y, float xrel, float yrel) -> bool {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = x;
    event.motion.y = y;
    event.motion.xrel = xrel;
    event.motion.yrel = yrel;
    event.motion.which = 0;
    event.motion.timestamp = 0;
    return SDL_PushEvent(&event);
}

static auto push_mouse_button_event(MouseButton button, bool down) -> bool {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.button = mouse_button_to_sdl(button);
    event.button.down = down;
    event.button.x = 0;
    event.button.y = 0;
    event.button.which = 0;
    event.button.timestamp = 0;
    return SDL_PushEvent(&event);
}

static auto push_mouse_wheel_event(float x, float y) -> bool {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.x = x;
    event.wheel.y = y;
    event.wheel.which = 0;
    event.wheel.timestamp = 0;
    return SDL_PushEvent(&event);
}
```

**Requirements:**
- The `key_code_to_sdl_scancode` function uses `static_cast` — no table to maintain or sync.
- Local helper functions use `static` linkage (internal to the translation unit).
- Test files include `<SDL3/SDL.h>` in the `#ifdef BUDDD_HAS_DISPLAY` block for both `SDL_SetHint()` and the local push functions.
- AMEND-2026-001 expansion (handled by the constitution-agent) will formally permit SDL3 API calls in test files for testing SDL3-dependent engine functionality.

### 10. `src/engine/platform/platform.h` — Add `virtual auto input_system() -> InputSystem& = 0`

Add a forward declaration of `InputSystem` before the `Platform` class definition, and add the pure virtual method:

**Forward declaration (add before `class Platform`):**
```cpp
class InputSystem;
```

**New method (add after `poll_events()`):**
```cpp
    /// Returns a reference to the input system owned by this platform.
    /// The reference remains valid for the lifetime of the Platform.
    virtual auto input_system() -> InputSystem& = 0;
```

**Exact file after modification (`platform.h`):**

```cpp
#pragma once

#include "error.h"

#include <memory>

namespace buddd::engine {

enum class Backend {
    SDL3,
    Headless
};

class Window;
struct WindowConfig;
class InputSystem;   // <-- NEW: forward declaration

class Platform {
public:
    [[nodiscard]] static auto create(Backend backend) -> Result<std::unique_ptr<Platform>>;

    virtual ~Platform() = default;

    [[nodiscard]] virtual auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> = 0;

    /// Polls the platform event queue.
    /// Returns false if the user requested to quit (e.g., window close button),
    /// true otherwise. In headless mode, always returns true.
    virtual auto poll_events() -> bool = 0;

    /// Returns a reference to the input system owned by this platform.
    /// The reference remains valid for the lifetime of the Platform.
    virtual auto input_system() -> InputSystem& = 0;   // <-- NEW

    Platform(const Platform&) = delete;
    auto operator=(const Platform&) -> Platform& = delete;
    Platform(Platform&&) = delete;
    auto operator=(Platform&&) -> Platform& = delete;

protected:
    Platform() = default;
};

} // namespace buddd::engine
```

### 11. `src/engine/platform/platform_sdl3.h` — Add InputSystemSDL3 member

Add the include and the member:

```cpp
#pragma once

#include "platform.h"
#include "input/input_system_sdl3.h"   // <-- NEW include

namespace buddd::engine {

class PlatformSDL3 final : public Platform {
public:
    ~PlatformSDL3() override;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;
    auto poll_events() -> bool override;
    auto input_system() -> InputSystem& override;   // <-- NEW override declaration

    PlatformSDL3(const PlatformSDL3&) = delete;
    auto operator=(const PlatformSDL3&) -> PlatformSDL3& = delete;
    PlatformSDL3(PlatformSDL3&&) = delete;
    auto operator=(PlatformSDL3&&) -> PlatformSDL3& = delete;

private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformSDL3() = default;

    InputSystemSDL3 input_system_;   // <-- NEW embedded member
};

} // namespace buddd::engine
```

### 12. `src/engine/platform/platform_sdl3.cpp` — Update `poll_events()`

Replace the existing `poll_events()` implementation:

```cpp
auto PlatformSDL3::poll_events() -> bool {
    // 1. Begin the input frame (copies current→previous, resets delta/wheel)
    input_system_.begin_frame();

    // 2. Process all pending SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
        // Route non-quit events to the input system
        input_system_.on_sdl_event(event);
    }
    return true;
}
```

**Requirements:**
- `input_system_.begin_frame()` is called BEFORE the `SDL_PollEvent` loop.
- Non-quit events are routed to `input_system_.on_sdl_event(event)`.
- `on_sdl_event()` is private on `InputSystemSDL3` — this is fine because `PlatformSDL3` accesses it through its own `InputSystemSDL3` member directly (member access, not through the abstract interface).
- No new includes needed — `platform_sdl3.h` now includes `input/input_system_sdl3.h`, which includes `<SDL3/SDL.h>`.

### 13. `src/engine/platform/platform_headless.h` — Add InputSystemHeadless member

```cpp
#pragma once

#include "platform.h"
#include "input/input_system_headless.h"   // <-- NEW include

namespace buddd::engine {

class PlatformHeadless final : public Platform {
public:
    ~PlatformHeadless() override = default;

    auto create_window(const WindowConfig& config) -> Result<std::unique_ptr<Window>> override;
    auto poll_events() -> bool override;
    auto input_system() -> InputSystem& override;   // <-- NEW override declaration

    PlatformHeadless(const PlatformHeadless&) = delete;
    auto operator=(const PlatformHeadless&) -> PlatformHeadless& = delete;
    PlatformHeadless(PlatformHeadless&&) = delete;
    auto operator=(PlatformHeadless&&) -> PlatformHeadless& = delete;

private:
    friend auto Platform::create(Backend) -> Result<std::unique_ptr<Platform>>;
    PlatformHeadless() = default;

    InputSystemHeadless input_system_;   // <-- NEW embedded member
};

} // namespace buddd::engine
```

### 14. `src/engine/platform/platform_headless.cpp` — Update `poll_events()`

Replace the existing `poll_events()`:

```cpp
auto PlatformHeadless::poll_events() -> bool {
    input_system_.begin_frame();   // <-- NEW: begin the input frame
    return true;                   // Headless: never quits.
}
```

**Requirements:**
- `begin_frame()` is called before returning `true`.
- No SDL3 includes needed in headless files.

## Required tests

All tests live in `tests/input_tests.cpp`. The test file has two sections:

- **Headless tests** (always compiled, no display required) — no `BUDDD_HAS_DISPLAY` guard.
- **SDL3 integration tests** (conditionally compiled with `#ifdef BUDDD_HAS_DISPLAY`).

Within the `#ifdef BUDDD_HAS_DISPLAY` block, `<SDL3/SDL.h>` is included for both `SDL_SetHint()` (video driver setup) and `SDL_PushEvent()` (synthetic event injection), per the expanded AMEND-2026-001 (to be updated by the constitution-agent). Synthetic event injection uses local helper functions defined in the test file that call `SDL_PushEvent()` directly.

### Headless tests (always runnable)

| ID | Test name | Tags | Verification | AC linkage |
|---|---|---|---|---|
| T-01 | `"Factory creates Headless InputSystem"` | `[headless][input]` | `InputSystem::create(Backend::Headless)` returns engaged `Result`. `dynamic_cast<InputSystemHeadless*>(result->get())` is non-null. | AC-014 |
| T-02 | `"Factory creates SDL3 InputSystem"` | `[headless][input]` | `InputSystem::create(Backend::SDL3)` returns engaged `Result`. `dynamic_cast<InputSystemSDL3*>(result->get())` is non-null. | AC-014 |
| T-03 | `"Headless InputSystem returns defaults"` | `[headless][input]` | Create headless InputSystem. Call `begin_frame()`. Verify all query methods return `false` or `(0.0f, 0.0f)`. Check all `KeyCode` values and all `MouseButton` values. | AC-010, AC-016 |
| T-04 | `"Factory unknown backend returns InputInitFailed"` | `[headless][input]` | Calling `InputSystem::create(static_cast<Backend>(999))` returns `make_error` with `Error::Category::InputInitFailed`. | AC-014 (error path) |
| T-05 | `"Headless Platform input_system() returns valid ref"` | `[headless][platform][input]` | Create `Platform::create(Headless)`. Call `platform->input_system()`. Verify it returns an `InputSystem&`. Call `poll_events()`. All input queries return defaults. | AC-011, AC-013 |
| T-06 | `"Headless begin_frame() does not crash"` | `[headless][input]` | Create headless platform. Call `poll_events()` twice. Verify no crash. | AC-013 |
| T-07 | `"Double-buffered state transitions (standalone)"` | `[headless][input]` | Create headless InputSystem via factory (or use SDL3 backend via factory in standalone mode — headless backend always returns false so this test uses direct state inspection or SDL3 backend with synthetic events; see SDL3 tests below). *For headless: just verify that begin_frame() does not crash.* | AC-016 |
| T-08 | `"sizeof(KeyCode) == 1"` | `[headless][input]` | `static_assert(sizeof(KeyCode) == 1)` compiles. | AC-001 |
| T-09 | `"MouseButton values exist"` | `[headless][input]` | All five `MouseButton` values compile and can be compared. | AC-002 |

### SDL3 integration tests (conditional: `#ifdef BUDDD_HAS_DISPLAY`)

These tests require the `BUDDD_HAS_DISPLAY` CMake option. They use the expanded AMEND-2026-001 exception to include `<SDL3/SDL.h>` for both `SDL_SetHint()` (offscreen video driver) and `SDL_PushEvent()` (synthetic event injection via local helper functions).

| ID | Test name | Tags | Verification | AC linkage |
|---|---|---|---|---|
| T-10 | `"SDL3 key down/up detected via InputSystem"` | `[sdl3][input]` | `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`. Create `Platform::create(Backend::SDL3)`. Call `test::push_key_event(KeyCode::Escape, true)`. Call `platform->poll_events()`. Verify `platform->input_system().is_down(KeyCode::Escape) == true` and `is_pressed(KeyCode::Escape) == true`. Push key-up event. Call `poll_events()`. Verify `is_down(KeyCode::Escape) == false` and `is_released(KeyCode::Escape) == true`. | AC-006 |
| T-11 | `"SDL3 mouse motion updates position and delta"` | `[sdl3][input]` | Setup as above. Push motion event (x=100, y=200, xrel=10, yrel=-5). Call `poll_events()`. Verify `mouse_position()` → `(100, 200)`, `mouse_delta()` → `(10, -5)`. Call `poll_events()` again (no events). Verify `mouse_delta()` → `(0, 0)` (reset by begin_frame). | AC-007 |
| T-12 | `"SDL3 mouse button pressed/released"` | `[sdl3][input]` | Setup as above. For each `MouseButton` value, push button-down, call `poll_events()`, verify `is_mouse_pressed(button)==true` and `is_mouse_down(button)==true`. Push button-up, call `poll_events()`, verify `is_mouse_released(button)==true` and `is_mouse_down(button)==false`. | AC-008 |
| T-13 | `"SDL3 mouse wheel accumulates correctly"` | `[sdl3][input]` | Setup as above. Push wheel event (x=1, y=2). Call `poll_events()`. Verify `mouse_wheel()` → `(1, 2)`. Call `poll_events()` again (no events). Verify `mouse_wheel()` → `(0, 0)`. Push two wheel events of (1, 2) before a single `poll_events()` call. Verify `mouse_wheel()` returns `(2, 4)`. | AC-009 |
| T-14 | `"SDL3 multi-frame key transitions"` | `[sdl3][input]` | Setup as above. Simulate key-down for `KeyCode::W`. Call `poll_events()`. Verify `is_pressed(W)==true`, `is_down(W)==true`, `is_released(W)==false`. Call `poll_events()` again (no new events). Verify `is_pressed(W)==false`, `is_down(W)==true`, `is_released(W)==false`. Push key-up event, call `poll_events()`. Verify `is_released(W)==true`, `is_down(W)==false`, `is_pressed(W)==false`. | AC-016 |
| T-15 | `"SDL3 Platform provides valid InputSystem reference"` | `[sdl3][input]` | Setup as above. Call `platform->input_system()`. Verify it returns a non-null `InputSystem&` (the reference binds successfully). | AC-012 |
| T-16 | `"SDL3 rapid down+up in same frame"` | `[sdl3][input]` | Setup as above. Push key-down for `KeyCode::Space`, then immediately push key-up for `KeyCode::Space`. Call `poll_events()`. Verify `is_down(Space)==false`, `is_pressed(Space)==false`, `is_released(Space)==false` (no net change). | Edge case from spec |
| T-17 | `"KeyCode static_cast round-trip for representative keys"` | `[sdl3][input]` | Setup as above. Test `static_cast` round-trip for a representative set of keys (e.g., A, Z, Digit0, Digit9, Space, Escape, ArrowUp, F12, Grave). For each `KeyCode k`, verify `static_cast<KeyCode>(static_cast<SDL_Scancode>(k)) == k` and that the scancode value falls within the valid range. No lookup table needed — the static_cast-based compile-time assertion in AC-015 covers all values. | AC-015 |

### Test file structure

```cpp
// tests/input_tests.cpp

#include "error.h"
#include "platform/platform.h"
#include "input/input_system.h"
#include "input/key_code.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace buddd::engine;

// ===== Headless tests (always compiled) =====

TEST_CASE("Factory creates Headless InputSystem", "[headless][input]") { ... }
TEST_CASE("Factory creates SDL3 InputSystem", "[headless][input]") { ... }
// ... all headless tests above ...

// ===== SDL3 integration tests (conditional) =====

#ifdef BUDDD_HAS_DISPLAY

#include <SDL3/SDL.h>       // For SDL_SetHint + SDL_PushEvent (per expanded AMEND-2026-001)

// Local helper functions for synthetic event injection (see section 9 of Required implementation behavior)
// These call SDL_PushEvent() directly with the expanded constitutional exception.
static auto push_key_event(KeyCode key, bool down) -> bool { /* ... */ }
static auto push_mouse_motion_event(float x, float y, float xrel, float yrel) -> bool { /* ... */ }
static auto push_mouse_button_event(MouseButton button, bool down) -> bool { /* ... */ }
static auto push_mouse_wheel_event(float x, float y) -> bool { /* ... */ }

TEST_CASE("SDL3 key down/up detected via InputSystem", "[sdl3][input]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    // ... test body using local push_key_event() etc.
}

// ... all SDL3 tests above ...

#endif // BUDDD_HAS_DISPLAY
```

**Requirements:**
- All test cases use `REQUIRE`/`REQUIRE_FALSE` (not `CHECK`).
- Headless tests are always compiled and passing.
- SDL3 tests are guarded by `#ifdef BUDDD_HAS_DISPLAY`.
- The `<SDL3/SDL.h>` include in the `#ifdef` block is used for both `SDL_SetHint()` and `SDL_PushEvent()`, per the expanded AMEND-2026-001 (handled by constitution-agent).

## Edge cases

| Case | Expected behaviour |
|---|---|
| Rapid key press and release between two `begin_frame()` calls (down and up in same frame) | Both events are processed within the same frame: `current[key]` is set to `true` by down, then `false` by up. Result: `is_down=false`, `is_pressed=false`, `is_released=false` (no net change). |
| Key held down for 5 consecutive frames | Frame 1: key-down event → `is_pressed=true`, `is_down=true`. Frames 2–5: no events → `is_pressed=false`, `is_down=true`. Frame N+1: key-up event → `is_released=true`, `is_down=false`. |
| Mouse moved but no `begin_frame()` before next `poll_events()` | `begin_frame()` is always called at the start of `poll_events()`. Application pattern is `poll_events()` → queries → update+render. Multiple `poll_events()` per frame resets delta/wheel mid-frame (discouraged but safe). |
| Key held when window loses focus | SDL sends key-up events for all held keys on focus loss. The input system correctly receives these and clears state. |
| Mouse position queried before any motion event | Returns `(0.0f, 0.0f)` (initial default). |
| Scroll wheel with horizontal axis | `mouse_wheel()` returns `(x, y)` where both axes are accumulated. Horizontal scroll (trackpad two-finger swipe) is supported. |
| Unknown SDL scancode received | Ignored (not mapped to any `KeyCode`). The bounds check in the `static_cast` conversion rejects scancode values at or below `SDL_SCANCODE_UNKNOWN` or at or above `KeyCode::_Count`. Debug builds print warning to `std::cerr`. |
| More than 256 distinct key values needed | `KeyCode` is `uint8_t`, max 256 values. Current definition uses ~53 values. Future expansion requires changing underlying type to `uint16_t` (non-breaking for static library). |
| Repeated calls to `is_pressed` within the same frame | All return the same value within a frame (`current`/`previous` do not change mid-frame). |
| `MouseButton` cast from integer out of range | Behaviour is undefined (standard C++ enum class out-of-range cast). Application code should not cast. |
| Platform destructor runs while `InputSystem&` is still held | Use-after-free bug. Caller must ensure `InputSystem&` is not accessed after owning Platform is destroyed. |
| `is_down()` called before first `begin_frame()` | Safe: `current`/`previous` both initialised to zero in constructor. Returns `false`. |
| Mouse motion event with negative coordinates | Both `position` and `delta` can be negative. Application should clamp if needed. |
| `begin_frame()` called twice without event processing between | Safe but events between the two calls are lost. delta/wheel are zeroed twice. |

## Security impact

None. No elevated privileges required. The InputSystem is a passive state tracker — it does not capture or record input beyond the current frame's state arrays. The headless backend never processes real input events, making it safe for CI environments without display or input hardware. No network or filesystem access is involved.

Architecture boundary (CONST-001) is maintained: public headers expose zero SDL3 types. Only `input_system_sdl3.h`, `input_system_sdl3.cpp` include `<SDL3/SDL.h>` within `src/engine/`. Test files also include `<SDL3/SDL.h>` under the `#ifdef BUDDD_HAS_DISPLAY` guard, per the expanded AMEND-2026-001.

## Data and migration impact

None. No persistent state, database, or file format is introduced. No schema changes.

## API compatibility impact

The following public API surface is introduced:

```cpp
namespace buddd::engine {

// KeyCode enum
enum class KeyCode : uint8_t { Unknown, A...Z, Digit0...9, Space, Escape, Enter, Tab,
    Backspace, Delete, Insert, CapsLock, ShiftLeft/Right, ControlLeft/Right,
    AltLeft/Right, SuperLeft/Right, ArrowUp/Down/Left/Right, F1...F12,
    Grave, Minus, Equals, BracketLeft/Right, Semicolon, Quote, Comma, Period,
    Slash, Backslash, _Count };

// MouseButton enum
enum class MouseButton : uint8_t { Left, Right, Middle, X1, X2 };

// InputSystem (abstract)
class InputSystem {
    static auto create(Backend) -> Result<std::unique_ptr<InputSystem>>;
    virtual auto begin_frame() -> void = 0;
    virtual auto is_down(KeyCode) const noexcept -> bool = 0;
    virtual auto is_pressed(KeyCode) const noexcept -> bool = 0;
    virtual auto is_released(KeyCode) const noexcept -> bool = 0;
    virtual auto mouse_position() const noexcept -> std::pair<float,float> = 0;
    virtual auto mouse_delta() const noexcept -> std::pair<float,float> = 0;
    virtual auto mouse_wheel() const noexcept -> std::pair<float,float> = 0;
    virtual auto is_mouse_down(MouseButton) const noexcept -> bool = 0;
    virtual auto is_mouse_pressed(MouseButton) const noexcept -> bool = 0;
    virtual auto is_mouse_released(MouseButton) const noexcept -> bool = 0;
    // non-copyable, non-movable
};

// Platform gains:
virtual auto input_system() -> InputSystem& = 0;

// Error gains:
Error::Category::InputInitFailed

} // namespace buddd::engine
```

**Backward compatibility**: The Platform abstraction gains a new pure virtual method (`input_system()`), which is a breaking change for any existing `Platform` subclass. However, there are currently only two subclasses (`PlatformSDL3`, `PlatformHeadless`), both modified in this contract. No other code subclasses `Platform`.

Once accepted, changing any of the following constitutes a breaking change:
- Namespace, class name, or enum value name.
- Function signature, return type, or parameter type.
- Adding or removing virtual methods.
- Changing `KeyCode` underlying type.
- Changing `MouseButton` values.

## Documentation impact

- The wiki documentation (`docs/wiki/architecture/overview.md`) should be updated to include the new `src/engine/input/` directory in the engine library structure diagram.
- The wiki architecture boundary section should note the new InputSystem abstraction and mention that AMEND-2026-001 was expanded to allow SDL3 API calls in test files for testing SDL3-dependent engine functionality.
- No README, constitution, or ADR files are modified.

## ADR impact

None. No new architectural decision requires an ADR. The patterns used (abstract interface + concrete backends, static factory, double-buffered state, `poll_events()` integration) are established in ADR-003 and existing implementation contracts.

The human chose to expand AMEND-2026-001 to allow `SDL_PushEvent()` and related SDL3 API calls in test files. This expansion will be implemented by the constitution-agent in a later workflow step.

## Constitution impact

AMEND-2026-001 must be expanded to broaden its exception from "SDL3 includes only for setting video driver hints" to "SDL3 API calls in test files when testing SDL3-dependent engine functionality". This expansion covers `SDL_PushEvent()`, `SDL_SetHint()`, and any other SDL3 API calls needed to set up or exercise SDL3 backends in conditional (`#ifdef BUDDD_HAS_DISPLAY`) tests. The expansion will be implemented by the constitution-agent in a later workflow step — the Code Agent does NOT modify the constitution.

## Done criteria

The implementation is complete when all of the following are true:

1. **Files exist**: All 8 new files listed in "Files allowed to change" exist with correct content matching the required implementation behavior.

2. **Files modified**: All 6 modified files (`error.h`, `platform.h`, `platform_sdl3.h`, `platform_sdl3.cpp`, `platform_headless.h`, `platform_headless.cpp`) have the exact changes described in this contract.

3. **Build succeeds**: `cmake --preset debug && cmake --build --preset debug` exits 0 with zero warnings related to the input system source files on the reference compiler.

4. **Architecture boundary verified**: Running `grep -E '(SDL_|SDL3)' src/engine/input/key_code.h src/engine/input/input_system.h` returns zero matches. No SDL3 types leak into public headers.

5. **No SDL3 in headless files**: Grep for `SDL_` in `*_headless.*` files returns nothing.

6. **`sizeof(KeyCode) == 1`**: A `static_assert(sizeof(KeyCode) == 1)` compiles.

7. **`KeyCode::_Count` is last**: The enum has `_Count` as its final enumerator with value > 0.

8. **`InputSystem` is abstract**: A `static_assert` test verifies `InputSystem` has pure virtual methods (cannot be instantiated directly).

9. **`InputSystem` is non-copyable and non-movable**: `static_assert(!std::is_copy_constructible_v<InputSystem>)` passes.

10. **`InputInitFailed` is in `Error::Category`**: The new category compiles and `to_string()` handles it.

11. **Headless backend works**: `Platform::create(Headless)` returns a valid platform. `platform->input_system()` returns a valid reference. All query methods return `false` or `(0.0f, 0.0f)`.

12. **SDL3 backend compiles**: `Platform::create(SDL3)` compiles (runtime depends on display availability).

13. **Factory test (headless)**: `InputSystem::create(Backend::Headless)` returns engaged `Result`.

14. **Factory test (SDL3)**: `InputSystem::create(Backend::SDL3)` returns engaged `Result`.

15. **Test file compiles**: `tests/input_tests.cpp` compiles for both headless and SDL3 (conditional) builds.

16. **AMEND-2026-001 expansion note**: The constitutional amendment expansion is handled separately by the constitution-agent. The Code Agent does NOT modify `docs/constitution/rules/CONST-001-architecture-boundaries.md`. Verify via `git diff docs/constitution/rules/CONST-001-architecture-boundaries.md` — no changes by Code Agent.

17. **Forbidden files unchanged**: `git diff` does not show changes to `src/engine/version.h`, `src/engine/version.cpp`, root `CMakeLists.txt`, `CMakePresets.json`, any file in `src/cmd/`, `src/editor/`, or `docs/constitution/`.

## Verification commands (copy-paste ready)

```bash
# Configure and build
cmake --preset debug
cmake --build --preset debug

# Verify architecture boundary (no leaks in public headers)
grep -E '(SDL_|SDL3)' src/engine/input/key_code.h src/engine/input/input_system.h
# Expected: zero matches

# Verify no SDL3 in headless files
grep -E 'SDL_' src/engine/input/input_system_headless.h src/engine/input/input_system_headless.cpp
# Expected: zero matches

# Verify forbidden files are unchanged
git diff --name-only
# Should NOT include: version.h, version.cpp, root CMakeLists.txt, CMakePresets.json,
# anything in src/cmd/, src/editor/, docs/constitution/

# Verify constitutional amendment is unchanged by Code Agent (constitution-agent handles expansion)
git diff docs/constitution/rules/CONST-001-architecture-boundaries.md
# Expected: no changes (empty diff) — Code Agent does NOT modify constitution.
```
