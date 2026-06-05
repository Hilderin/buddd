# SPEC-013 — Input System: Keyboard & Mouse Abstraction

## Status

`Draft`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> Human validated on 2026-05-30. See coordination.md ## Human Validation for details.

| | |
|---|---|
| Approved by | human |
| Date | 2026-05-30 |

## Problem

The Buddd Engine has no input system. The existing `Platform::poll_events()` handles only `SDL_EVENT_QUIT` — all other SDL events are discarded with the comment *"input handling is future work"*. This means:

- Applications built on the engine cannot receive keyboard or mouse input.
- Every interactive feature (camera controls, menu navigation, gameplay) is blocked.
- There is no engine-level abstraction for input state, so any workaround would require including SDL3 headers outside `src/engine/`, violating CONST-001.
- There is no headless input stub, making it impossible to test input-dependent features in CI or headless environments.
- The demo loop (`buddd run`) runs without user interaction, and future demos need input to be truly interactive.

## Goals

- Define an abstract `InputSystem` interface in `src/engine/input/` with pure virtual methods for keyboard and mouse state queries.
- Provide a concrete SDL3 backend (`InputSystemSDL3`) that processes SDL keyboard, mouse-motion, mouse-button, and mouse-wheel events routed from `PlatformSDL3::poll_events()`.
- Provide a concrete headless backend (`InputSystemHeadless`) that returns default (zero / false) values for all queries, enabling tests to run without a display.
- Integrate the InputSystem into the Platform abstraction: `Platform` gains `virtual auto input_system() -> InputSystem& = 0`; each concrete Platform backend owns a matching InputSystem backend instance.
- Define an engine-level `KeyCode` enum in a public header — no SDL scancodes or keycodes exposed in public API (CONST-001 compliance).
- Define a frame-based state model with double-buffered key/button arrays, supporting `is_down()`, `is_pressed()`, and `is_released()` queries per frame.
- Support mouse position (absolute window coordinates), mouse delta (relative motion accumulated since last `begin_frame()`), mouse wheel (accumulated scroll since last `begin_frame()`), and mouse button state with the same pressed/released transition detection.
- Follow the established project pattern: abstract interface + concrete SDL3/Headless backends + static factory, non-copyable/non-movable, `Result<T>` for fallible operations.

## Non-goals

- No gamepad or controller support — reserved for a future feature.
- No touch input — reserved for a future feature.
- No text input (SDL text events) — reserved for a future feature.
- No input remapping or rebinding — `KeyCode` values are fixed to physical key positions (SDL scancode mapping).
- No input action system (e.g., "Jump" action mapped to a key) — that is an application-level concern.
- No joystick or hotplug detection — deferred to future gamepad support.
- No raw input or relative mouse mode — the input system provides window-relative absolute position plus accumulated delta.
- No clipboard events (`SDL_EVENT_CLIPBOARD_UPDATE`) — not required in v1.
- No drop events (`SDL_EVENT_DROP_FILE`, etc.) — deferred.
- No dynamic backend switching — the input backend is fixed by the Platform backend for the Platform lifetime.
- No per-frame event accumulation beyond the current frame — mouse delta and wheel are reset to zero at the start of each frame by `begin_frame()`.
- No support for multiple mice or keyboards — only the primary pointing device and keyboard are tracked.
- No focus-aware input filtering — input state is updated for all events received by the platform, regardless of window focus.
- No repeat-key events — the input system tracks physical key state (down/up), not typed characters. Key repeat is handled at the application level if needed.
- No `const` overload of `input_system()` on `Platform` — only a mutable reference is provided, because `begin_frame()` mutates state. A const accessor can be added later if needed.

## Actors

| Actor | Description |
|---|---|
| Engine developer | Adds engine features that depend on input — camera controls, debug overlay interaction, menu systems. Calls `platform.input_system().is_down()`, etc. through the abstract Platform API. |
| Application developer | Builds games or tools on top of the engine. Uses `Platform::input_system()` to read input state in their game loop. Never sees SDL scancodes or backend-specific types. |
| Test suite | Catch2 v3 tests that exercise input-dependent features in headless mode. Creates a headless Platform, calls `poll_events()` and `input_system()` queries, and verifies that all queries return zero/false defaults. May also construct a standalone `InputSystem` via the factory for unit testing the state model. |

## Key entities

### `KeyCode` enum

Defined in a dedicated public header `src/engine/input/key_code.h`. Underlying type is `uint8_t` (max 256 values, extensible). KeyCode values match SDL_Scancode values — conversion is `static_cast` with bounds check. Unrecognised scancodes (or values >= `_Count`) map to `KeyCode::Unknown`.

```cpp
namespace buddd::engine {

enum class KeyCode : uint8_t {
    Unknown = 0,

    // Letters (4-29 — matches SDL_SCANCODE_A through SDL_SCANCODE_Z)
    A = 4, B = 5, C = 6, D = 7, E = 8, F = 9, G = 10,
    H = 11, I = 12, J = 13, K = 14, L = 15, M = 16,
    N = 17, O = 18, P = 19, Q = 20, R = 21, S = 22,
    T = 23, U = 24, V = 25, W = 26, X = 27, Y = 28, Z = 29,

    // Digits (30-39 — matches SDL_SCANCODE_1 through SDL_SCANCODE_0)
    Digit1 = 30, Digit2 = 31, Digit3 = 32, Digit4 = 33, Digit5 = 34,
    Digit6 = 35, Digit7 = 36, Digit8 = 37, Digit9 = 38, Digit0 = 39,

    // Common keys (40-57)
    Enter = 40, Escape = 41, Backspace = 42, Tab = 43, Space = 44,
    Minus = 45, Equals = 46,
    BracketLeft = 47, BracketRight = 48, Backslash = 49,
    // Note: SDL_SCANCODE_NONUSHASH = 50, not included
    Semicolon = 51, Quote = 52, Grave = 53,
    Comma = 54, Period = 55, Slash = 56,
    CapsLock = 57,

    // Function keys (58-69)
    F1 = 58, F2 = 59, F3 = 60, F4 = 61, F5 = 62,
    F6 = 63, F7 = 64, F8 = 65, F9 = 66, F10 = 67,
    F11 = 68, F12 = 69,

    // Navigation & editing
    Delete = 76,      // SDL_SCANCODE_DELETE
    Right = 79,       // SDL_SCANCODE_RIGHT
    Left = 80,        // SDL_SCANCODE_LEFT
    Down = 81,        // SDL_SCANCODE_DOWN
    Up = 82,          // SDL_SCANCODE_UP
    Insert = 93,      // SDL_SCANCODE_INSERT

    // Modifiers (224-231)
    ControlLeft = 224, ShiftLeft = 225, AltLeft = 226, SuperLeft = 227,
    ControlRight = 228, ShiftRight = 229, AltRight = 230, SuperRight = 231,

    _Count,  // Not a valid SDL scancode; used for array sizing
};

} // namespace buddd::engine
```

### `MouseButton` enum

Defined in the public header `src/engine/input/input_system.h`. Underlying type is `uint8_t`.

```cpp
namespace buddd::engine {

enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    X1,       // Forward / thumb button 1
    X2        // Back / thumb button 2
};

} // namespace buddd::engine
```

## User-visible behavior

### 1. Abstract `InputSystem` class (`src/engine/input/input_system.h`)

```cpp
#pragma once

#include "error.h"
#include "input/key_code.h"

#include <memory>
#include <utility>

namespace buddd::engine {

enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    X1,
    X2
};

class InputSystem {
public:
    /// Creates an InputSystem with the given backend.
    /// For standalone testing, Backend::Headless is the only useful choice
    /// (SDL3 is constructed internally by PlatformSDL3).
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

### 2. State model (double-buffered)

The InputSystem maintains two state buffers per key and per mouse button plus accumulated frame values:

| Variable | Description |
|---|---|
| `current[key]` | Whether the key is held down this frame |
| `previous[key]` | Whether the key was held down at the end of the previous frame |
| `current_mouse[button]` | Whether the mouse button is held down this frame |
| `previous_mouse[button]` | Whether the mouse button was held down last frame |
| `position_x, position_y` | Absolute mouse position (set by mouse-motion events) |
| `delta_x, delta_y` | Accumulated mouse delta since last `begin_frame()` |
| `wheel_x, wheel_y` | Accumulated mouse wheel since last `begin_frame()` |

**`begin_frame()` contract:**

1. `previous[key] = current[key]` for all keys.
2. `previous_mouse[button] = current_mouse[button]` for all mouse buttons.
3. `delta_x = 0; delta_y = 0;` (reset accumulated delta).
4. `wheel_x = 0; wheel_y = 0;` (reset accumulated wheel).
5. `position` is **not** reset — it persists independently.

**Event processing** (inside the concrete SDL3 backend, triggered during `poll_events()`):
- Key down → `current[key] = true`
- Key up → `current[key] = false`
- Mouse motion → `position_x = event.x; position_y = event.y; delta_x += event.xrel; delta_y += event.yrel;`
- Mouse button down → `current_mouse[button] = true`
- Mouse button up → `current_mouse[button] = false`
- Mouse wheel → `wheel_x += event.x; wheel_y += event.y;`

**Query semantics:**

| Query | Formula | Description |
|---|---|---|
| `is_down(key)` | `current[key]` | True if the key is currently held down |
| `is_pressed(key)` | `current[key] && !previous[key]` | True on the frame the key was first pressed |
| `is_released(key)` | `!current[key] && previous[key]` | True on the frame the key was released |
| `is_mouse_down(button)` | `current_mouse[button]` | True if the button is currently held down |
| `is_mouse_pressed(button)` | `current_mouse[button] && !previous_mouse[button]` | True on the frame the button was first pressed |
| `is_mouse_released(button)` | `!current_mouse[button] && previous_mouse[button]` | True on the frame the button was released |
| `mouse_position()` | `(position_x, position_y)` | Current absolute mouse position |
| `mouse_delta()` | `(delta_x, delta_y)` | Accumulated mouse motion since last `begin_frame()` |
| `mouse_wheel()` | `(wheel_x, wheel_y)` | Accumulated scroll since last `begin_frame()` |

**Key behaviour:** Unlike mouse delta and wheel, key and button state is **not cleared** in `begin_frame()`. If a key is held down for 5 consecutive frames:
- Frame 1: key down event → `current[key]=true`, `previous[key]=false` → `is_pressed=true`, `is_down=true`
- Frames 2–5: no events for that key → `current[key]=true`, `previous[key]=true` (snapshotted each frame) → `is_pressed=false`, `is_down=true`
- Frame N+1: key up event → `current[key]=false`, `previous[key]=true` → `is_released=true`, `is_down=false`

This gives the standard "just pressed" and "just released" semantics found in game engines.

### 3. Platform integration

`Platform` (abstract, `src/engine/platform/platform.h`) gains:

```cpp
virtual auto input_system() -> InputSystem& = 0;
```

`PlatformSDL3` (`src/engine/platform/platform_sdl3.h`):
- Owns a member `InputSystemSDL3 input_system_;` (embedded, not heap-allocated through the factory).
- `poll_events()` is updated to:
  1. Call `input_system_.begin_frame()` before processing events.
  2. For each non-quit SDL event, dispatch to `InputSystemSDL3::on_sdl_event(event)` — a private, non-virtual method that processes `SDL_EVENT_KEY_DOWN`, `SDL_EVENT_KEY_UP`, `SDL_EVENT_MOUSE_MOTION`, `SDL_EVENT_MOUSE_BUTTON_DOWN`, `SDL_EVENT_MOUSE_BUTTON_UP`, and `SDL_EVENT_MOUSE_WHEEL`.
- `input_system()` returns a reference to the embedded `input_system_` member.

`PlatformHeadless` (`src/engine/platform/platform_headless.h`):
- Owns a member `InputSystemHeadless input_system_;` (embedded).
- `poll_events()` calls `input_system_.begin_frame()` and returns `true`.
- `input_system()` returns a reference to the embedded `input_system_` member.
- All input queries return zero/false defaults (no events are ever processed).

### 4. Concrete backend: `InputSystemSDL3`

Private header (`src/engine/input/input_system_sdl3.h`) and implementation (`src/engine/input/input_system_sdl3.cpp`).

- Inherits `InputSystem`.
- Has a private method `on_sdl_event(const SDL_Event&)` that is **not** part of the abstract interface.
- Maintains the double-buffered state arrays (as `std::array<bool, 256>` or similar fixed-size storage for keys, and fixed-size for mouse buttons).
- Converts SDL scancodes to `KeyCode` via `static_cast<KeyCode>(scancode)` with a bounds check against `KeyCode::_Count`. Scancodes with no corresponding `KeyCode` value (or value >= `_Count`) map to `KeyCode::Unknown`.
- Tracks mouse position, delta, and wheel from SDL events.
- `friend auto InputSystem::create(Backend) -> Result<std::unique_ptr<InputSystem>>;` — allows the factory to instantiate it.

### 5. Concrete backend: `InputSystemHeadless`

Private header (`src/engine/input/input_system_headless.h`) and implementation (`src/engine/input/input_system_headless.cpp`).

- Inherits `InputSystem`.
- `begin_frame()` is a no-op (no state to update).
- All query methods return zero/false defaults.
- `friend auto InputSystem::create(Backend) -> Result<std::unique_ptr<InputSystem>>;`.

### 6. Factory

`InputSystem::create(Backend)` in `src/engine/input/input_system.cpp`:
- `Backend::SDL3` → returns `std::unique_ptr<InputSystem>(new InputSystemSDL3())`.
- `Backend::Headless` → returns `std::unique_ptr<InputSystem>(new InputSystemHeadless())`.
- The factory uses `friend` access to construct private concrete classes.

This factory is available for standalone construction (e.g., unit tests). In production, each Platform backend creates its InputSystem directly via embedding (not via the factory), so the factory is primarily for tooling and test scenarios.

### 7. Application usage pattern

```cpp
// Game loop (conceptual)
while (platform.poll_events()) {
    // begin_frame() is called INSIDE poll_events(), before event processing.

    auto& input = platform.input_system();

    if (input.is_down(KeyCode::W)) {
        // Move forward
    }
    if (input.is_pressed(KeyCode::Space)) {
        // Jump
    }
    if (input.is_released(KeyCode::Escape)) {
        // Open menu
    }

    auto [mx, my] = input.mouse_position();
    auto [dx, dy] = input.mouse_delta();

    if (input.is_mouse_down(MouseButton::Left)) {
        // Drag
    }

    // ... update, render ...
}
```

## User stories

### Story 1 — Query keyboard state in the game loop (Priority: P1)

As an application developer, I want to check whether a key is held, just pressed, or just released each frame, so that I can implement character movement, jump-once, and menu-toggle behaviours.

**Given** a running engine with an SDL3 platform
**When** I hold the `W` key for 3 frames, then release it
**Then** the first frame reports `is_down(W) = true` and `is_pressed(W) = true`; frames 2–3 report `is_down(W) = true` and `is_pressed(W) = false`; the frame after release reports `is_down(W) = false`, `is_released(W) = true`, and `is_pressed(W) = false`.

**Given** a running engine with a headless platform
**When** I query any key state
**Then** all queries return `false` (keys are never pressed in headless mode).

### Story 2 — Query mouse state (Priority: P1)

As an application developer, I want to read the mouse cursor position, relative motion, scroll wheel, and button state, so that I can implement camera look, object selection, and scrollable lists.

**Given** a running engine with an SDL3 platform
**When** the user moves the mouse and clicks the left button
**Then** `mouse_position()` returns the current cursor position, `mouse_delta()` returns the accumulated relative motion since the last frame, and `is_mouse_pressed(MouseButton::Left)` is true on the frame the button goes down.

**Given** the user scrolls the mouse wheel up
**Then** `mouse_wheel()` returns a pair `(0.0f, >0.0f)` (positive Y representing upward scroll) on the frame the scroll occurs.

**Given** a headless platform
**When** any mouse query is called
**Then** position is `(0.0f, 0.0f)`, delta is `(0.0f, 0.0f)`, wheel is `(0.0f, 0.0f)`, and all button queries return `false`.

### Story 3 — InputSystem accessed through Platform (Priority: P1)

As an engine developer, I want the InputSystem to be available through the Platform abstraction, so that I do not need to create or manage a separate input object.

**Given** a `Platform&` reference (either SDL3 or headless)
**When** I call `platform.input_system()`
**Then** I receive a valid `InputSystem&` reference whose lifetime is tied to the Platform.

### Story 4 — Frame-based state resets correctly (Priority: P2)

As an application developer, I want mouse delta and wheel to reset each frame, so that accumulated motion does not carry over incorrectly.

**Given** an SDL3 platform where the user moves the mouse by 10 pixels on frame 1 and does not move it on frame 2
**When** I query `mouse_delta()` on frame 2
**Then** the delta is `(0.0f, 0.0f)` — the previous frame's motion does not leak into frame 2.

**Given** the same scenario for the scroll wheel
**Then** `mouse_wheel()` returns `(0.0f, 0.0f)` on frame 2.

### Story 5 — Platform owns InputSystem lifecycle (Priority: P2)

As an engine developer, I want the InputSystem to be automatically created and destroyed with its owning Platform, so that there is no additional resource management burden.

**Given** a `Platform::create(Backend::SDL3)` call
**When** the Platform is constructed
**Then** the InputSystem is fully initialised and accessible via `input_system()`, without any additional initialisation step.

**Given** the Platform is destroyed
**Then** the InputSystem is destroyed with it and no resources leak.

### Story 6 — KeyCode values match SDL scancodes (Priority: P3)

As an engine developer, I want `KeyCode` enum values to match `SDL_Scancode` values so that conversion is a simple `static_cast` with bounds check, eliminating the need for a lookup table.

**Given** an SDL3 platform
**When** I press the physical Escape key
**Then** `input_system().is_down(KeyCode::Escape)` returns `true`.
**When** I press the physical `A` key
**Then** `input_system().is_down(KeyCode::A)` returns `true`.
**When** I press the physical left Shift key
**Then** `input_system().is_down(KeyCode::ShiftLeft)` returns `true`.

### Story 7 — Headless input system returns defaults for all queries (Priority: P1)

As a test author, I want the InputSystem in headless mode to return sensible defaults, so that tests that incidentally read input state do not need special handling.

**Given** a headless Platform
**When** I call any input query method
**Then** the result is `false` for boolean queries, `(0.0f, 0.0f)` for position/delta/wheel queries, and `KeyCode::Unknown` comparisons never match.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | A public header `src/engine/input/key_code.h` exists defining `enum class KeyCode : uint8_t` in namespace `buddd::engine` with values matching SDL_Scancode for: A–Z (4–29), Digit1–Digit0 (30–39), Enter, Escape, Backspace, Tab, Space, Minus, Equals, BracketLeft, BracketRight, Backslash, Semicolon, Quote, Grave, Comma, Period, Slash, CapsLock (40–57), F1–F12 (58–69), Delete (76), Right/Left/Down/Up (79–82), Insert (93), ControlLeft/ShiftLeft/AltLeft/SuperLeft (224–227), and ControlRight/ShiftRight/AltRight/SuperRight (228–231). Includes `Unknown = 0` and `_Count` for array sizing. | File compiles; `KeyCode` enumeration has the specified values; `sizeof(KeyCode) == 1`; `static_cast<uint8_t>(KeyCode::A) == 4` and other SDL-matching values verified via static assertion. |
| AC-002 | A public header `src/engine/input/input_system.h` exists defining `enum class MouseButton : uint8_t` with values Left, Right, Middle, X1, X2, and an abstract `InputSystem` class in namespace `buddd::engine`. | File compiles; `InputSystem` is abstract (has at least one pure virtual method); `MouseButton` values are as specified. |
| AC-003 | `InputSystem` has a static `create(Backend)` factory returning `Result<std::unique_ptr<InputSystem>>`, a virtual destructor, and is non-copyable and non-movable. | File compiles; `static_assert(!std::is_copy_constructible_v<InputSystem>)` passes; factory signature matches. |
| AC-004 | `InputSystem` has pure virtual methods: `begin_frame() -> void`, `is_down(KeyCode) const noexcept -> bool`, `is_pressed(KeyCode) const noexcept -> bool`, `is_released(KeyCode) const noexcept -> bool`, `mouse_position() const noexcept -> std::pair<float,float>`, `mouse_delta() const noexcept -> std::pair<float,float>`, `mouse_wheel() const noexcept -> std::pair<float,float>`, `is_mouse_down(MouseButton) const noexcept -> bool`, `is_mouse_pressed(MouseButton) const noexcept -> bool`, `is_mouse_released(MouseButton) const noexcept -> bool`. All query methods are marked `[[nodiscard]]`. | File compiles; signatures match specification; removal of any `= 0` causes a compile error in a derived class. |
| AC-005 | `InputSystemSDL3` (concrete class inheriting `InputSystem`) exists in a private header `src/engine/input/input_system_sdl3.h`. It implements all pure virtual methods. It has a private `on_sdl_event(const SDL_Event&)` method. | File compiles; class is non-copyable, non-movable; `on_sdl_event` is not part of the abstract interface. |
| AC-006 | `InputSystemSDL3::on_sdl_event()` processes `SDL_EVENT_KEY_DOWN` / `SDL_EVENT_KEY_UP` events and converts SDL scancodes to `KeyCode` via `static_cast<KeyCode>(scancode)` with a bounds check. Recognised scancodes produce the correct `KeyCode`; scancodes with no matching `KeyCode` (value >= `KeyCode::_Count`) map to `KeyCode::Unknown`. | Integration test (SDL3 backend, requires `BUDDD_HAS_DISPLAY`): construct `Platform` via `Platform::create(Backend::SDL3)`. Push synthetic SDL key events onto the SDL event queue using `SDL_PushEvent()`. Call `Platform::poll_events()`, which internally calls `begin_frame()` then routes events to `InputSystemSDL3::on_sdl_event()`. Verify through the abstract `InputSystem&` (via `platform.input_system()`) that `is_down()` and `is_pressed()` return correct values for a representative set of keys (e.g., W, Space, Escape, ShiftLeft) and false for unrelated keys. |
| AC-007 | `InputSystemSDL3::on_sdl_event()` processes `SDL_EVENT_MOUSE_MOTION` events, updating `mouse_position()` and `mouse_delta()`. | Integration test (SDL3 backend, requires `BUDDD_HAS_DISPLAY`): construct a PlatformSDL3, push a synthetic `SDL_EVENT_MOUSE_MOTION` event via `SDL_PushEvent()` with position (100, 200) and rel (10, -5). Call `Platform::poll_events()`. Verify through `platform.input_system()` that `mouse_position()` returns (100, 200) and `mouse_delta()` returns (10, -5). Call `poll_events()` again (no new events) — `begin_frame()` resets accumulators, so `mouse_delta()` returns (0, 0). |
| AC-008 | `InputSystemSDL3::on_sdl_event()` processes `SDL_EVENT_MOUSE_BUTTON_DOWN` / `SDL_EVENT_MOUSE_BUTTON_UP` events for all five `MouseButton` values. | Integration test (SDL3 backend, requires `BUDDD_HAS_DISPLAY`): construct a PlatformSDL3. For each `MouseButton` value, push a synthetic button-down event via `SDL_PushEvent()`, call `Platform::poll_events()`, then verify through `platform.input_system()` that `is_mouse_pressed(button)` is true and `is_mouse_down(button)` is true. Push a button-up event, call `poll_events()`, then verify `is_mouse_released(button)` is true and `is_mouse_down(button)` is false. |
| AC-009 | `InputSystemSDL3::on_sdl_event()` processes `SDL_EVENT_MOUSE_WHEEL` events, accumulating `mouse_wheel()` values. | Integration test (SDL3 backend, requires `BUDDD_HAS_DISPLAY`): construct a PlatformSDL3. Push a synthetic `SDL_EVENT_MOUSE_WHEEL` event via `SDL_PushEvent()` with (x=1, y=2) and call `Platform::poll_events()`. Verify through `platform.input_system()` that `mouse_wheel()` returns (1, 2). Call `poll_events()` again (no new events) — `begin_frame()` resets accumulators, so `mouse_wheel()` returns (0, 0). Accumulation test: push two wheel events of (1, 2) before a single `poll_events()` call; verify `mouse_wheel()` returns (2, 4). |
| AC-010 | `InputSystemHeadless` (concrete class inheriting `InputSystem`) exists in a private header `src/engine/input/input_system_headless.h`. `begin_frame()` is a no-op. `is_down()`, `is_pressed()`, `is_released()`, `is_mouse_down()`, `is_mouse_pressed()`, `is_mouse_released()` all return `false`. `mouse_position()`, `mouse_delta()`, `mouse_wheel()` all return `(0.0f, 0.0f)`. | Unit test: create `InputSystemHeadless` (via factory with `Backend::Headless` or direct construction); call `begin_frame()` and all query methods; verify return values match specification. |
| AC-011 | `Platform` (abstract) gains `virtual auto input_system() -> InputSystem& = 0`. | File `platform.h` compiles; any attempt to instantiate a class derived from `Platform` without overriding `input_system()` fails to compile. |
| AC-012 | `PlatformSDL3` has an `auto input_system() -> InputSystem& override` that returns a reference to the embedded `InputSystemSDL3` member. `poll_events()` calls `input_system_.begin_frame()` at the start, then routes non-quit SDL events to `input_system_.on_sdl_event(event)`. | Unit test (SDL3 backend, `BUDDD_HAS_DISPLAY`): construct `PlatformSDL3` via factory; call `platform.input_system()`; verify it returns a non-null `InputSystem&`. Call `poll_events()`; verify that `begin_frame()` was called (e.g., via a test accessor or coverage). |
| AC-013 | `PlatformHeadless` has an `auto input_system() -> InputSystem& override` that returns a reference to the embedded `InputSystemHeadless` member. `poll_events()` calls `input_system_.begin_frame()` and returns `true`. | Unit test: construct headless platform; call `platform.input_system()`; verify it returns a valid `InputSystem&`; call `poll_events()`; verify it returns `true`. All input queries return defaults. |
| AC-014 | `InputSystem::create(Backend::SDL3)` returns a valid `unique_ptr<InputSystem>` pointing to an `InputSystemSDL3` instance. `InputSystem::create(Backend::Headless)` returns a valid `unique_ptr<InputSystem>` pointing to an `InputSystemHeadless` instance. Both succeed unconditionally (no error path in v1). | Unit test: both factory calls return engaged `Result` (no error); `dynamic_cast<...>` confirms the concrete type. |
| AC-015 | KeyCode values must match SDL_Scancode values for all supported keys. Every defined `KeyCode` (except `Unknown` and `_Count`) corresponds to an SDL_Scancode and the conversion via `static_cast<KeyCode>(scancode)` produces the correct value without a lookup table. | Compile-time assertion or unit test verifies that `static_cast<uint8_t>(KeyCode::A) == SDL_SCANCODE_A`, `static_cast<uint8_t>(KeyCode::Escape) == SDL_SCANCODE_ESCAPE`, etc. for all defined `KeyCode` values. |
| AC-016 | Double-buffered state correctly implements the pressed/released transition model: a key held across multiple frames produces `is_pressed=true` only on the first frame, `is_down=true` on all frames, and `is_released=true` on the frame after release. | Unit test: simulate key-down event; call `begin_frame()` and query — `is_pressed=true`, `is_down=true`, `is_released=false`. Simulate `begin_frame()` again without any new events — `is_pressed=false`, `is_down=true`, `is_released=false`. Simulate key-up event — `is_released=true`, `is_down=false`, `is_pressed=false`. |
| AC-017 | The `InputSystem` is correctly embedded in each Platform backend (not heap-allocated via `unique_ptr`). | Code review: `PlatformSDL3` has a member of type `InputSystemSDL3`, `PlatformHeadless` has a member of type `InputSystemHeadless`. The `input_system()` override returns `input_system_` (member reference, not dereferenced pointer). |
| AC-018 | No SDL3 types appear in the public headers (`key_code.h`, `input_system.h`). | `grep -E '(SDL_|SDL3)' src/engine/input/key_code.h src/engine/input/input_system.h` returns no matches. |
| AC-019 | All new files compile without warnings (with `-Wall -Wextra` or equivalent) on the reference compiler. | Build with `cmake --build` produces zero warnings related to input system source files. |
| AC-020 | Both SDL3 and headless backends are compiled into `buddd_engine` via the existing `GLOB_RECURSE` pattern — no CMakeLists.txt changes required. | Build succeeds; symbols for both `InputSystemSDL3` and `InputSystemHeadless` are present in the library (verify via `nm` or similar). |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An engine or application developer can read keyboard and mouse state using only abstract `InputSystem` methods — no SDL types or scancodes appear in public API. | A minimal program that creates a platform, calls `poll_events()`, and reads `input_system().is_down()`, `mouse_position()`, etc. compiles and links without SDL3 includes outside `src/engine/`. |
| SC-002 | All acceptance criteria tests pass in headless CI (no GPU, no display). | `cmake --build --preset debug && ctest --preset debug` — all input-system tests pass. |
| SC-003 | SDL3 input events (keyboard, mouse) are correctly processed and queryable through the abstract `InputSystem` interface in interactive mode. | Manual verification: run `buddd run` (or a test demo) with key press logging; pressing keys or moving the mouse produces correct `is_down`/`is_pressed`/`mouse_position` values (verified via logging to `std::cerr`). |
| SC-004 | The input system adds zero measurable overhead to frames with no input events (headless tests show no regression in test execution time). | Before/after benchmark: run scene-rendering test suite (100 frames) — execution time does not increase by more than 1% with the input system active. |
| SC-005 | Full keyboard and mouse state isolation: input events from one frame do not leak into the next frame (for delta/wheel) or produce stale pressed/released states. | Unit test: simulate event, call `begin_frame()` and verify transitions as described in AC-016. Run a multi-frame test across all buttons and a sample of keys. |

## Edge cases

| Case | Expected behaviour |
|---|---|
| Rapid key press and release between two `begin_frame()` calls (down and up in same frame) | Both events are processed within the same frame: `current[key]` is set to `true` by the down event, then set to `false` by the up event. Result: `is_down = false`, `is_pressed = false`, `is_released = false` (no net change from previous frame). The key was never held at a frame boundary. |
| Mouse moved but no `begin_frame()` called before next `poll_events()` | The application must call `poll_events()` exactly once per frame. `begin_frame()` is called at the start of `poll_events()`, so the pattern is always `poll_events()` → queries → (update + render). Multiple `poll_events()` calls in one frame would reset delta/wheel mid-frame — this is discouraged but not undefined (delta/wheel accumulate per-segment between resets). |
| Key held down when the window loses focus | SDL sends a key-up event for all held keys when the window loses focus. The input system correctly receives these events and clears the down state. No phantom "held" state persists across a focus loss. |
| Mouse position queried before any motion event has occurred | Returns `(0.0f, 0.0f)` (initial default). This is the expected behaviour — the first motion event sets the position. |
| Scroll wheel with horizontal axis (SDL_MOUSEWHEEL event with non-zero x) | `mouse_wheel()` returns `(x, y)` where both axes are accumulated. Horizontal scroll (e.g., trackpad two-finger horizontal swipe) is supported. |
| Unknown or unmapped SDL scancode received | Mapped to `KeyCode::Unknown`. Queries for `KeyCode::Unknown` are valid (they return state for that key slot), but application code should not rely on `KeyCode::Unknown` for any meaningful purpose. |
| More than 256 distinct key values needed in the future | `KeyCode` is `uint8_t`, limiting the enum to 256 values. The current definition uses ~65 values. If more are needed, the underlying type can be changed to `uint16_t` — this is a non-breaking change (no ABI concerns in a static library). |
| Repeated calls to `is_pressed` within the same frame | All return the same value within a frame (`current` and `previous` do not change mid-frame). This is safe and expected. |
| `MouseButton` cast from integer out of range | Behaviour is undefined (standard C++ enum class out-of-range cast). Application code should not cast integer values to `MouseButton` without validation. |
| Platform destructor runs while `InputSystem&` is still held by the caller | This is a use-after-free bug. The caller must ensure that `InputSystem&` is not accessed after the owning `Platform` is destroyed. This is consistent with the existing pattern (`RenderDevice` must outlive `RenderSystem`, etc.). |

## Error cases

| Case | Expected behaviour |
|---|---|
| `InputSystem::create()` called with a future unknown `Backend` value | Returns `make_error(Error::Category::Unsupported, "Unknown backend")`. |
| `InputSystem::create()` called with `Backend::SDL3` and SDL3 is unavailable at runtime | Returns `make_error(Error::Category::InitFailed, "InputSystemSDL3 construction failed")`. However, in v1 the `InputSystemSDL3` constructor has no fallible operations (it just initialises arrays to zero). This error path exists for forward compatibility. |
| `poll_events()` called before `input_system()` is first accessed | Safe: `begin_frame()` is called inside `poll_events()` before any event processing. The InputSystem is fully initialised at Platform construction time. |
| `begin_frame()` called twice without event processing in between | Safe but produces an empty frame (delta/wheel are zeroed twice). The double `begin_frame()` means the intervening events are lost — this is a programming error (caller should process events between begin_frame calls). Behaviour is well-defined but the intermediate events are discarded. |
| `is_down()` or other queries called before the first `begin_frame()` | Safe: `current` and `previous` are both initialised to zero in the constructor. All queries return `false`/`(0,0)` before any events are processed. |
| Keyboard event arrives with an unknown `SDL_Scancode` | Mapped to `KeyCode::Unknown`. The event is otherwise processed normally (the `Unknown` key slot tracks state just like any other key). |
| Mouse motion event with negative coordinates (e.g., cursor above/left of window during drag) | Both `position` and `delta` can be negative. The spec does not constrain values to the window rectangle — SDL may report negative coordinates. The application should clamp if needed. |

## Permissions and security

- No elevated privileges are required to receive input events.
- The InputSystem is a passive state tracker — it does not capture or record input beyond the current frame's state arrays.
- No secrets, credentials, or environment variables are consumed by the input abstraction layer.
- The headless backend never processes real input events, making it safe for CI environments without display or input hardware.
- No network or filesystem access is involved.
- The architecture boundary (CONST-001) is maintained: `InputSystem` public headers expose zero SDL3 types. Only the concrete `InputSystemSDL3` implementation includes `<SDL3/SDL.h>`.

## Observability

All observability uses `std::cerr` consistent with the project pattern.

| Signal | Source |
|---|---|
| `InputSystem::create()` backend selection | `std::cerr << "InputSystem backend: SDL3\n"` or `"InputSystem backend: Headless\n"` |
| `InputSystemSDL3` unrecognised scancode | `std::cerr << "InputSystemSDL3: unrecognised scancode " << scancode << "\n"` (debug builds only, to avoid spamming on unknown keys) |
| `Platform::poll_events()` event count per frame | `std::cerr << "poll_events: processed " << count << " events\n"` (debug builds only) |
| New `KeyCode` or `MouseButton` values added after v1 | Add corresponding enum entries and update observability if needed |

## Out of scope

- Gamepad / controller support (future feature).
- Touch input (future feature).
- Text input / IME (SDL text events — future feature).
- Input remapping / rebinding (application-level concern).
- Input action system (e.g., "Jump" → `KeyCode::Space` mapping — application-level).
- Joystick / hotplug detection (future gamepad feature).
- Raw input / relative mouse mode (SDL relative mouse mode is not used in v1).
- Clipboard events (`SDL_EVENT_CLIPBOARD_UPDATE`).
- Drop events (`SDL_EVENT_DROP_FILE`, `SDL_EVENT_DROP_TEXT`).
- The demo / application code that uses the InputSystem (e.g., camera controls in the cube demo) — the spec covers only the engine-level input system.
- Keyboard layout detection — `KeyCode` maps to physical key positions (SDL scancodes), not language-dependent keycodes.
- `Platform::input_system() const` (const overload) — not needed in v1 since `begin_frame()` mutates state.
- `InputSystem::create()` with no arguments — the factory always requires a `Backend` parameter for explicitness.
- GPU-side or network input — all input is CPU-based local device input only.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The InputSystem state arrays are sized to cover the maximum `KeyCode` value (currently ~65 defined values, max 256 with `uint8_t`). Storage is a `std::array<bool, 256>` (or equivalent bitset) for keys and a `std::array<bool, 5>` for mouse buttons. |
| A-02 | `InputSystemSDL3` converts SDL scancodes to `KeyCode` via `static_cast<KeyCode>(scancode)` with a bounds check in `input_system_sdl3.cpp`. Values corresponding to defined `KeyCode` entries are accepted; all other scancodes map to `KeyCode::Unknown`. |
| A-03 | `PlatformSDL3` owns the InputSystemSDL3 as an embedded member (not via `unique_ptr`), constructed at the point of member declaration. This means `InputSystemSDL3` must be default-constructible (all state initialised to zero). |
| A-04 | `PlatformHeadless` similarly owns `InputSystemHeadless` as an embedded member. |
| A-05 | The factory `InputSystem::create(Backend)` exists primarily for test and tool usage. In production, Platform backends construct their InputSystem directly. |
| A-06 | The `Error::Category` enum in `error.h` gains a new value `InputInitFailed` for future input initialisation errors. In v1, no InputSystem creation path actually fails, but the category is added for forward compatibility. |
| A-07 | Mouse wheel values are float accumulators. `SDL_EVENT_MOUSE_WHEEL` provides `float` values (precise scrolling). The input system passes these through without modification. |
| A-08 | No `Platform::input_system() const` overload is provided in v1 because `begin_frame()` is a mutating operation called inside `poll_events()`. If a const accessor is needed later, it can be added as a separate change. |
| A-09 | `begin_frame()` is idempotent in the sense that calling it multiple times between event processing is safe but causes loss of accumulated delta/wheel. The expected application pattern is one `poll_events()` call per frame, which internally calls `begin_frame()` once. |
| A-10 | Mouse position is stored as `float` pairs. SDL provides integer coordinates for mouse position and `float` for wheel. The input system converts mouse position to `float` for consistency. |
| A-11 | New files to be created:
- `src/engine/input/key_code.h` — public header, `KeyCode` enum
- `src/engine/input/input_system.h` — public header, abstract `InputSystem`, `MouseButton` enum
- `src/engine/input/input_system.cpp` — factory implementation
- `src/engine/input/input_system_sdl3.h` — private header, `InputSystemSDL3` concrete class
- `src/engine/input/input_system_sdl3.cpp` — SDL3 backend implementation (event processing, state management)
- `src/engine/input/input_system_headless.h` — private header, `InputSystemHeadless` concrete class
- `src/engine/input/input_system_headless.cpp` — headless backend implementation (stubs)
Modified files:
- `src/engine/platform/platform.h` — add `virtual auto input_system() -> InputSystem& = 0`
- `src/engine/platform/platform_sdl3.h` — add `InputSystemSDL3 input_system_` member, `input_system()` override declaration
- `src/engine/platform/platform_sdl3.cpp` — update `poll_events()` to call `input_system_.begin_frame()` and route events to `input_system_.on_sdl_event(event)`
- `src/engine/platform/platform_headless.h` — add `InputSystemHeadless input_system_` member, `input_system()` override declaration
- `src/engine/platform/platform_headless.cpp` — update `poll_events()` to call `input_system_.begin_frame()`
- `src/engine/error.h` — add `InputInitFailed` to `Error::Category` enum |
| A-12 | The `#pragma once` header guard convention is used for all new headers, consistent with the existing codebase. |
| A-13 | The project uses C++26 and supports `std::array`, `std::pair`, and `enum class` with specified underlying types. |
| A-14 | Tests for the input system live in a new file `tests/input_tests.cpp`, following the project convention of `*_tests.cpp` naming. |

## Open questions

All open questions have been resolved (see [coordination.md](./coordination.md) for the full answer log).
