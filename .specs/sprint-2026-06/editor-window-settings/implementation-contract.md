# IMPL-037 — Editor Window Geometry Persistence

## Source spec

`.specs/sprint-2026-06/editor-window-settings/spec.md` (SPEC-037)

## Goal

Persist editor window position (x, y), size (width, height), and window state (normal/maximized/minimized) to the `user_project_settings` tier on shutdown, and restore them on startup after validation (minimum 400×300 size, at least partially visible on a connected display, minimized→normal). This requires adding 5 new pure-virtual methods to `Window`, 2 to `Platform`, and implementing the save/load/validation logic in `Editor::setup()` and `Editor::shutdown()`.

## Non-goals

- No settings panel or UI for window geometry.
- No per-monitor profile or display-name-based position recovery.
- No mid-session save of window settings.
- No animated transitions for window repositioning.
- No support for multiple editor windows.
- No full-screen window state (only normal/maximized/minimized).
- No changes to ImGui docking layout.
- No changes to how the window is initially created (still created with defaults in `EngineService::create()`).
- No migration of old settings keys (none exist).
- No changes to `src/engine/` build system.

## Relevant ADRs

- **ADR-019**: No SDL3 headers may be included outside `src/engine/`. Editor code must use only the abstract `Window` and `Platform` interfaces.
- **ADR-009**: Test files follow `*_tests.cpp` naming convention.
- **ADR-020**: Custom logging system — use `BUDDD_LOG_TAG("Editor")`, `BUDDD_LOG_INFO`, `BUDDD_LOG_WARN`.
- **ADR-012**: Navigable object graph (`EngineService::platform()`, `EngineService::window()`) is unchanged.

## Files to inspect

| File | Reason |
|---|---|
| `src/engine/window/window.h` | Existing `Window` abstract class, `WindowConfig` struct |
| `src/engine/window/window_sdl3.h` | `WindowSDL3` class — add new method declarations |
| `src/engine/window/window_sdl3.cpp` | `WindowSDL3` implementations — add SDL3 calls |
| `src/engine/window/window_headless.h` | `WindowHeadless` class — add new method declarations |
| `src/engine/window/window_headless.cpp` | `WindowHeadless` stubs — add no-op implementations |
| `src/engine/platform/platform.h` | Existing `Platform` abstract class |
| `src/engine/platform/platform_sdl3.h` | `PlatformSDL3` class — add new method declarations |
| `src/engine/platform/platform_sdl3.cpp` | `PlatformSDL3` implementations — add SDL3 display calls |
| `src/engine/platform/platform_headless.h` | `PlatformHeadless` class — add new method declarations |
| `src/engine/platform/platform_headless.cpp` | `PlatformHeadless` stubs — add no-op implementations |
| `src/editor/editor.h` | `Editor` class — check existing members, no new members needed |
| `src/editor/editor.cpp` | `Editor::setup()` and `Editor::shutdown()` — add save/load/validation logic |
| `src/engine/settings/settings_store.h` | `SettingsStore::get<T>()`, `set<T>()` API signatures |
| `src/engine/settings/settings_manager.h` | `SettingsManager::user_project_settings()`, `save_all()`, `load_all()` |
| `src/engine/engine_service.h` | `EngineService::platform()` access |
| `tests/editor/settings_integration_tests.cpp` | Existing integration test patterns for settings |
| `tests/engine/settings_store_tests.cpp` | `temp_dir()` and `write_yaml()` helper patterns |
| `tests/engine/window_resize_tests.cpp` | Existing headless window test patterns |
| `tests/engine/platform_abstraction_tests.cpp` | Existing headless platform test patterns |
| `tests/CMakeLists.txt` | Test auto-discovery — `*_tests.cpp` glob pattern |

## Files allowed to change

Exact file paths (no glob patterns):

1. `src/engine/window/window.h`
2. `src/engine/window/window_sdl3.h`
3. `src/engine/window/window_sdl3.cpp`
4. `src/engine/window/window_headless.h`
5. `src/engine/window/window_headless.cpp`
6. `src/engine/platform/platform.h`
7. `src/engine/platform/platform_sdl3.h`
8. `src/engine/platform/platform_sdl3.cpp`
9. `src/engine/platform/platform_headless.h`
10. `src/engine/platform/platform_headless.cpp`
11. `src/editor/editor.h`
12. `src/editor/editor.cpp`
13. `tests/editor/settings_integration_tests.cpp`

New files to create:

14. `src/engine/window/window_utils.h`
15. `src/engine/window/window_utils.cpp`
16. `tests/engine/window_state_tests.cpp`
17. `tests/engine/platform_display_tests.cpp`
18. `tests/editor/window_settings_tests.cpp`

## Files forbidden to change

- `src/engine/settings/settings_store.h`
- `src/engine/settings/settings_store.cpp`
- `src/engine/settings/settings_manager.h`
- `src/engine/settings/settings_manager.cpp`
- `src/engine/engine_service.h`
- `src/engine/engine_service.cpp`
- `tests/CMakeLists.txt` (no change needed — auto-discovery picks up `*_tests.cpp`)
- Any file under `src/cmd/`
- Any file under `docs/`
- Any `.specs/` file other than the current feature's coordination.md

## Existing conventions to follow

1. **`noexcept` rules**: Getters (`position()`, `state()`, `display_count()`, `display_bounds()`) are `noexcept` (matching `width()`, `height()`, `delta_time()`). Setters (`set_position()`, `set_state()`, `resize()`) are NOT `noexcept` (matching `set_title()`, `set_mouse_capture()`, `on_resize()`).
2. **`[[nodiscard]]`**: Apply to all getter-style pure virtual methods (matching existing convention: `width()`, `height()`, `native_handle()`, `is_mouse_captured()`, `delta_time()`).
3. **Namespace alias**: In `editor.cpp`, use `namespace be = buddd::engine;` (already present at line 34).
4. **Logging**: Use `BUDDD_LOG_INFO()` for normal operations, `BUDDD_LOG_WARN()` for validation fallbacks, `BUDDD_LOG_TAG("Editor")` at file scope (already present in `editor.cpp` at line 32).
5. **C++ style**: Trailing return types (`auto foo() -> int`), brace-init lists where appropriate.
6. **Include ordering**: In `editor.cpp`, existing includes use angle brackets for external libs (`<imgui.h>`), quotes for internal headers. Place new includes (`window/window_utils.h`) in alphabetical section with other `window/` includes.
7. **Comment style**: Use `// ── Section name ──` for section markers (as existing in `editor.cpp`).
8. **Test cases**: Use `TEST_CASE("AC-NNN: Description", "[tag1][tag2]")` naming with AC traceability.
9. **Temp dirs in tests**: Use the `temp_dir()` + `write_yaml()` helper pattern from `tests/engine/settings_store_tests.cpp`.

## Required implementation behavior

### 1. Add `WindowState` enum and `WindowPosition` struct to `window.h`

Insert BEFORE the `WindowConfig` struct definition in `src/engine/window/window.h`:

```cpp
enum class WindowState { Normal, Maximized, Minimized };
struct WindowPosition { int x; int y; };
```

### 2. Add pure virtual methods to `Window` class in `window.h`

Insert AFTER `auto set_mouse_capture(bool captured) -> void = 0;` and BEFORE the `Window(const Window&) = delete;` block:

```cpp
[[nodiscard]] virtual auto position() const noexcept -> WindowPosition = 0;
virtual auto set_position(WindowPosition pos) -> void = 0;
[[nodiscard]] virtual auto state() const noexcept -> WindowState = 0;
virtual auto set_state(WindowState state) -> void = 0;
virtual auto resize(int width, int height) -> void = 0;
```

**Order matters**: `position()` and `state()` are getters with `[[nodiscard]]` and `noexcept`. `set_position()`, `set_state()`, and `resize()` are setters without `noexcept`.

### 3. Add `WindowSDL3` implementations in `window_sdl3.h` and `window_sdl3.cpp`

**`window_sdl3.h`** — add these declarations in the `public:` section AFTER `auto is_mouse_captured() const noexcept -> bool override;`:

```cpp
[[nodiscard]] auto position() const noexcept -> WindowPosition override;
auto set_position(WindowPosition pos) -> void override;
[[nodiscard]] auto state() const noexcept -> WindowState override;
auto set_state(WindowState state) -> void override;
auto resize(int width, int height) -> void override;
```

**`window_sdl3.cpp`** — add these implementations:

```cpp
auto WindowSDL3::position() const noexcept -> WindowPosition {
    int x, y;
    SDL_GetWindowPosition(window_, &x, &y);
    return {x, y};
}

auto WindowSDL3::set_position(WindowPosition pos) -> void {
    SDL_SetWindowPosition(window_, pos.x, pos.y);
}

auto WindowSDL3::state() const noexcept -> WindowState {
    Uint64 flags = SDL_GetWindowFlags(window_);
    if (flags & SDL_WINDOW_MAXIMIZED) return WindowState::Maximized;
    if (flags & SDL_WINDOW_MINIMIZED) return WindowState::Minimized;
    return WindowState::Normal;
}

auto WindowSDL3::set_state(WindowState state) -> void {
    switch (state) {
        case WindowState::Normal:    SDL_RestoreWindow(window_);   break;
        case WindowState::Maximized: SDL_MaximizeWindow(window_);  break;
        case WindowState::Minimized: SDL_MinimizeWindow(window_);  break;
    }
}

auto WindowSDL3::resize(int width, int height) -> void {
    SDL_SetWindowSize(window_, width, height);
    width_  = width;
    height_ = height;
}
```

**Important**: `resize()` must update `width_` and `height_` immediately after `SDL_SetWindowSize()`, NOT relying on `on_resize()` which is only called asynchronously via event processing.

### 4. Add `WindowHeadless` implementations in `window_headless.h` and `window_headless.cpp`

**`window_headless.h`** — add these declarations in the `public:` section AFTER `auto is_mouse_captured() const noexcept -> bool override;`:

```cpp
[[nodiscard]] auto position() const noexcept -> WindowPosition override;
auto set_position(WindowPosition pos) -> void override;
[[nodiscard]] auto state() const noexcept -> WindowState override;
auto set_state(WindowState state) -> void override;
auto resize(int width, int height) -> void override;
```

**`window_headless.cpp`** — add these implementations:

```cpp
auto WindowHeadless::position() const noexcept -> WindowPosition {
    return {0, 0};
}

auto WindowHeadless::set_position(WindowPosition /*pos*/) -> void {
    // no-op
}

auto WindowHeadless::state() const noexcept -> WindowState {
    return WindowState::Normal;
}

auto WindowHeadless::set_state(WindowState /*state*/) -> void {
    // no-op
}

auto WindowHeadless::resize(int width, int height) -> void {
    width_  = width;
    height_ = height;
}
```

### 5. Add `DisplayBounds` struct to `platform.h`

Insert AFTER `using FileDialogCallback = ...` line and BEFORE the `class Platform` declaration:

```cpp
struct DisplayBounds { int x; int y; int width; int height; };
```

### 6. Add pure virtual methods to `Platform` class in `platform.h`

Insert AFTER `[[nodiscard]] virtual auto delta_time() const noexcept -> float = 0;` and BEFORE `virtual auto show_open_file_dialog(...)`:

```cpp
[[nodiscard]] virtual auto display_count() const noexcept -> int = 0;
[[nodiscard]] virtual auto display_bounds(int index) const noexcept -> DisplayBounds = 0;
```

### 7. Add `PlatformSDL3` implementations in `platform_sdl3.h` and `platform_sdl3.cpp`

**`platform_sdl3.h`** — add declarations in `public:` section AFTER `[[nodiscard]] auto delta_time() const noexcept -> float override;`:

```cpp
[[nodiscard]] auto display_count() const noexcept -> int override;
[[nodiscard]] auto display_bounds(int index) const noexcept -> DisplayBounds override;
```

**`platform_sdl3.cpp`** — add implementations:

```cpp
auto PlatformSDL3::display_count() const noexcept -> int {
    return SDL_GetNumVideoDisplays();
}

auto PlatformSDL3::display_bounds(int index) const noexcept -> DisplayBounds {
    int count = SDL_GetNumVideoDisplays();
    if (index < 0 || index >= count) {
        return {0, 0, 0, 0};
    }
    SDL_Rect rect;
    if (SDL_GetDisplayBounds(index, &rect)) {
        return {rect.x, rect.y, rect.w, rect.h};
    }
    return {0, 0, 0, 0};
}
```

Include `<SDL3/SDL_video.h>` or rely on the existing `#include <SDL3/SDL.h>` at the top of `platform_sdl3.cpp`. The existing include of `<SDL3/SDL.h>` is sufficient.

### 8. Add `PlatformHeadless` implementations in `platform_headless.h` and `platform_headless.cpp`

**`platform_headless.h`** — add declarations in `public:` section AFTER `[[nodiscard]] auto delta_time() const noexcept -> float override;`:

```cpp
[[nodiscard]] auto display_count() const noexcept -> int override;
[[nodiscard]] auto display_bounds(int index) const noexcept -> DisplayBounds override;
```

**`platform_headless.cpp`** — add implementations:

```cpp
auto PlatformHeadless::display_count() const noexcept -> int {
    return 0;
}

auto PlatformHeadless::display_bounds(int /*index*/) const noexcept -> DisplayBounds {
    return {0, 0, 0, 0};
}
```

### 9. Create `src/engine/window/window_utils.h`

```cpp
#pragma once

#include "window.h"

#include <string>

namespace buddd::engine {

/// Convert WindowState to its string representation.
/// Normal → "normal", Maximized → "maximized", Minimized → "minimized".
auto window_state_to_string(WindowState state) -> std::string;

/// Parse a string to WindowState.
/// "normal" → Normal, "maximized" → Maximized, "minimized" → Minimized.
/// Any other string → Normal (fallback).
auto parse_window_state(const std::string& str) -> WindowState;

} // namespace buddd::engine
```

### 10. Create `src/engine/window/window_utils.cpp`

```cpp
#include "window_utils.h"

namespace buddd::engine {

auto window_state_to_string(WindowState state) -> std::string {
    switch (state) {
        case WindowState::Normal:    return "normal";
        case WindowState::Maximized: return "maximized";
        case WindowState::Minimized: return "minimized";
    }
    return "normal";
}

auto parse_window_state(const std::string& str) -> WindowState {
    if (str == "normal")    return WindowState::Normal;
    if (str == "maximized") return WindowState::Maximized;
    if (str == "minimized") return WindowState::Minimized;
    return WindowState::Normal;  // unknown → fallback
}

} // namespace buddd::engine
```

### 11. Modify `Editor::setup()` in `editor.cpp`

**Insert a new section** after the existing settings block (which ends at line 91 with `}`) and BEFORE the `// ── Create menu bar ──` comment (line 93), with the following code:

```cpp
    // ── Editor window geometry: load and validate from settings ──
    {
        constexpr int MIN_W = 400;
        constexpr int MIN_H = 300;
        constexpr int DEFAULT_W = 1280;
        constexpr int DEFAULT_H = 800;

        auto& ups = settings_manager_->user_project_settings();

        // Read raw values (defaults used if keys missing)
        int raw_w = ups.get<int32_t>("editor.window.width",  DEFAULT_W);
        int raw_h = ups.get<int32_t>("editor.window.height", DEFAULT_H);
        int raw_x = ups.get<int32_t>("editor.window.x",      0);
        int raw_y = ups.get<int32_t>("editor.window.y",      0);
        auto raw_s = ups.get<std::string>("editor.window.state", "normal");

        // 1. Size validation
        int valid_w = raw_w;
        int valid_h = raw_h;
        if (raw_w < MIN_W || raw_h < MIN_H) {
            BUDDD_LOG_WARN("Editor: window size below minimum ({}x{}), using default ({}x{})",
                raw_w, raw_h, DEFAULT_W, DEFAULT_H);
            valid_w = DEFAULT_W;
            valid_h = DEFAULT_H;
        }

        // 2. Position validation
        bool position_valid = false;
        int display_count = engine_->platform().display_count();
        if (display_count > 0) {
            for (int i = 0; i < display_count; ++i) {
                auto bounds = engine_->platform().display_bounds(i);
                // Overlap test: at least 1 pixel of window rect must be inside display rect
                if (raw_x < bounds.x + bounds.width
                    && raw_x + valid_w > bounds.x
                    && raw_y < bounds.y + bounds.height
                    && raw_y + valid_h > bounds.y)
                {
                    position_valid = true;
                    break;
                }
            }
        }
        if (!position_valid) {
            BUDDD_LOG_WARN("Editor: window position invalid (no overlapping display), using default");
        }

        // 3. State validation
        auto state = parse_window_state(raw_s);
        if (state == WindowState::Minimized) {
            BUDDD_LOG_INFO("Editor: saved window state was 'minimized' — forcing normal on startup");
            state = WindowState::Normal;
        } else if (raw_s != "normal" && raw_s != "maximized" && raw_s != "minimized") {
            BUDDD_LOG_INFO("Editor: saved window state '{}' is unknown — using normal", raw_s);
        }

        // 4. Apply to window
        window_->resize(valid_w, valid_h);
        if (position_valid) {
            window_->set_position({raw_x, raw_y});
        }
        window_->set_state(state);

        BUDDD_LOG_INFO("Editor: restoring window geometry from user settings ({}x{} + {{{}, {}}}, {})",
            valid_w, valid_h, raw_x, raw_y, window_state_to_string(state));
    }
```

**Include additions** at the top of `editor.cpp`: add `#include "window/window_utils.h"` in the window includes section (line 21, after `#include "window/window.h"`).

### 12. Modify `Editor::shutdown()` in `editor.cpp`

**Insert a new section** BEFORE the existing `if (settings_manager_)` block (which starts at line 413 with `if (settings_manager_) {`) and BEFORE the save_all() call. The existing shutdown code is:

```cpp
auto Editor::shutdown() -> void {
    if (settings_manager_) {
        auto save_result = settings_manager_->save_all();
        ...
    }
    ...
}
```

The new code must be inserted between the `{` at line 412 and the `if (settings_manager_) {` at line 413, like this:

```cpp
auto Editor::shutdown() -> void {
    // ── Save window geometry before persisting settings ──
    if (window_ && settings_manager_) {
        auto& ups = settings_manager_->user_project_settings();
        auto pos = window_->position();
        ups.set<int32_t>("editor.window.x",       pos.x);
        ups.set<int32_t>("editor.window.y",       pos.y);
        ups.set<int32_t>("editor.window.width",   window_->width());
        ups.set<int32_t>("editor.window.height",  window_->height());
        ups.set<std::string>("editor.window.state", window_state_to_string(window_->state()));
        BUDDD_LOG_INFO("Editor: saving window geometry ({}x{} + {{{}, {}}}, {})",
            window_->width(), window_->height(),
            pos.x, pos.y,
            window_state_to_string(window_->state()));
    }

    if (settings_manager_) {
```

### 13. No changes to `editor.h`

No new private members or public methods are needed. The existing `window_` and `settings_manager_` members are sufficient.

### Settings keys summary

| Key | Type | Default | Set in |
|---|---|---|---|
| `editor.window.x` | `int32_t` | 0 (ignored if position invalid) | `shutdown()` |
| `editor.window.y` | `int32_t` | 0 | `shutdown()` |
| `editor.window.width` | `int32_t` | 1280 | `shutdown()` |
| `editor.window.height` | `int32_t` | 800 | `shutdown()` |
| `editor.window.state` | `std::string` | `"normal"` | `shutdown()` |

All keys are stored in `user_project_settings` tier (`.buddd/user/settings.yaml`).

### Validation algorithm (exact logic)

The validation algorithm in `Editor::setup()` MUST follow the spec's pseudocode exactly (lines 249-298 of spec.md), with the addition noted by the spec-critic: `save_all()` must NOT be called inside the save block in `shutdown()` — it is called by the existing code that follows the save block.

### Log messages

All editor window geometry log messages use `"Editor"` tag (already set at `editor.cpp` line 32). Specific messages required:

| Condition | Level | Message |
|---|---|---|
| Valid restore | INFO | `"Editor: restoring window geometry from user settings (1280x800 + {100, 50}, Normal)"` |
| Size fallback | WARN | `"Editor: window size below minimum (300x200), using default (1280x800)"` |
| Position fallback | WARN | `"Editor: window position invalid (no overlapping display), using default"` |
| Minimized→Normal | INFO | `"Editor: saved window state was 'minimized' — forcing normal on startup"` |
| Unknown state | INFO | `"Editor: saved window state 'fullscreen' is unknown — using normal"` |
| Save | INFO | `"Editor: saving window geometry (1024x768 + {200, 100}, Maximized)"` |

## Required tests

### Unit tests — window

Create `tests/engine/window_state_tests.cpp`:

| Test case | What it verifies | AC trace | SC trace |
|---|---|---|---|
| `AC-001: WindowState enum values exist` | Compile check: Normal, Maximized, Minimized exist | AC-001 | SC-001 |
| `AC-019: WindowHeadless::position() returns {0,0}` | Headless position stub returns origin | AC-004 | SC-001 |
| `AC-019: WindowHeadless::state() returns Normal` | Headless state stub returns Normal | AC-004 | SC-001 |
| `AC-019: WindowHeadless::set_position is no-op` | No crash, no state change | AC-004 | SC-001 |
| `AC-019: WindowHeadless::set_state is no-op` | No crash, state unchanged | AC-004 | SC-001 |
| `AC-019: WindowHeadless::resize updates dimensions` | resize changes width()/height() immediately (headless) | AC-018, AC-004 | SC-001 |
| `AC-019: WindowState string round-trip for Normal` | `window_state_to_string(Normal)` → `"normal"` → `parse_window_state` → Normal | AC-019 | SC-005 |
| `AC-019: WindowState string round-trip for Maximized` | Same for Maximized | AC-019 | SC-005 |
| `AC-019: WindowState string round-trip for Minimized` | Same for Minimized | AC-019 | SC-005 |
| `AC-013: parse_window_state with unknown string returns Normal` | `"fullscreen"`, `""`, `"garbage"` all return Normal | AC-013 | SC-005 |
| `AC-018: WindowHeadless::resize immediate cache update` | Call resize(800,600), immediately check width()==800 && height()==600 | AC-018 | SC-001 |

### Unit tests — platform

Create `tests/engine/platform_display_tests.cpp`:

| Test case | What it verifies | AC trace | SC trace |
|---|---|---|---|
| `AC-008: PlatformHeadless::display_count() returns 0` | Headless has no displays | AC-008 | SC-002 |
| `AC-008: PlatformHeadless::display_bounds(0) returns zero` | Call with 0 returns {0,0,0,0} | AC-008 | SC-002 |
| `AC-007: PlatformSDL3 display_count and display_bounds (offscreen)` | Offscreen SDL3 count is 1, bounds are valid (run within `#ifdef BUDDD_HAS_DISPLAY`) | AC-007 | SC-002 |
| `AC-007: PlatformSDL3::display_bounds(-1) returns zero bounds` | Out-of-range index -1 returns {0,0,0,0} (run within `#ifdef BUDDD_HAS_DISPLAY`) | AC-007 | SC-002 |
| `AC-007: PlatformSDL3::display_bounds(999) returns zero bounds` | Out-of-range index 999 returns {0,0,0,0} (run within `#ifdef BUDDD_HAS_DISPLAY`) | AC-007 | SC-002 |

Note: For the SDL3 offscreen display tests, use `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")` before creating the platform (following the pattern in `tests/editor/settings_integration_tests.cpp`).

### Unit tests — editor window settings

Create `tests/editor/window_settings_tests.cpp`:

| Test case | What it verifies | AC trace | SC trace |
|---|---|---|---|
| `AC-010: Size validation — width below minimum falls back` | Inject w=399, h=800; verify window is 1280x800 after setup | AC-010 | SC-003 |
| `AC-010: Size validation — height below minimum falls back` | Inject w=800, h=299; verify window is 1280x800 | AC-010 | SC-003 |
| `AC-010: Size validation — minimum boundary accepted` | Inject w=400, h=300; verify window is 400x300 | AC-010 | SC-003 |
| `AC-010: Size validation — normal size accepted` | Inject w=1920, h=1080; verify window is 1920x1080 | AC-010 | SC-003 |
| `AC-011: Position validation — valid overlap accepted` | Inject position overlapping the single offscreen display; verify set_position was effective | AC-011 | SC-004 |
| `AC-011: Position validation — no overlap uses default` | Inject position far off-screen; verify window position is not the injected one | AC-011, AC-016 | SC-004 |
| `AC-011: Position validation — zero displays skips` | Headless mode: inject position; verify no crash (position treated as invalid) | AC-011, AC-008 | SC-004 |
| `AC-012: State 'minimized' forced to Normal` | Inject state="minimized", verify window state is Normal | AC-012, AC-004 | SC-005 |
| `AC-013: State unknown string treated as Normal` | Inject state="fullscreen", verify window state is Normal | AC-013 | SC-005 |
| `AC-012: State 'maximized' applied` | Inject state="maximized", verify window state is Maximized | AC-012 | SC-005 |
| `AC-017: Size fallback calls resize with defaults` | Inject tiny size; verify `resize(DEFAULT_W, DEFAULT_H)` semantics (check width/height) | AC-017 | SC-003 |
| `AC-021: shutdown without setup is safe` | Construct Editor, call shutdown(), no crash | AC-021 | — |
| `AC-022: Settings keys use editor.window.* convention` | After shutdown, verify keys exist in user_project_settings store | AC-022 | SC-006 |

These tests should run in headless mode (no `BUDDD_HAS_DISPLAY` needed) EXCEPT for the SDL3-specific tests (position validation with offscreen SDL3). For headless tests, the `WindowHeadless` and `PlatformHeadless` stubs are used — position is always invalid (display_count=0), state is always Normal, resize updates dimensions immediately.

### Integration tests — full round-trip

Append to `tests/editor/settings_integration_tests.cpp` within `#ifdef BUDDD_HAS_DISPLAY`:

| Test case | What it verifies | AC trace | SC trace |
|---|---|---|---|
| `AC-009/AC-014: Window settings round-trip save/load` | setup with pre-populated settings → verify window matches → shutdown → verify YAML has keys | AC-009, AC-014 | SC-006 |
| `AC-015: Written types are correct` | After shutdown, load YAML, verify x/y/width/height are integers, state is string | AC-015 | SC-006 |
| `AC-018: WindowSDL3::resize() immediate cache update` | Create offscreen SDL3 window, call resize(800,600), immediately verify width()==800 && height()==600 (cache updated before any event processing) | AC-018 | SC-001 |
| `AC-020: Headless no-crash round-trip` | Repeat the same test with Backend::Headless (must not crash) | AC-020 | SC-001, SC-006 |
| `Edge: minimized state on disk → normal on startup` | Pre-populate with state=minimized, valid position/size; verify Normal state but position/size applied | AC-012 | SC-005, SC-006 |

## Edge cases

All edge cases from the spec (lines 330-350) MUST be handled by the implementation. The contract's validation algorithm covers:

1. **Missing keys (first launch)**: `get<T>(key, default)` returns the default → size is 1280×800, state is Normal, position is not applied (raw_x=0, raw_y=0 but overlap test may pass on offscreen display — in real scenario, SDL3 centres by default).
2. **Saved width=399**: Falls below MIN_W (400) → default 1280×800.
3. **Saved height=299**: Falls below MIN_H (300) → default 1280×800.
4. **Saved width=400, height=300**: At boundary → accepted.
5. **Saved width=0 or negative**: Below minimum → default.
6. **Partially visible window**: Overlap test (>=1 pixel inside display) → valid.
7. **Fully off-screen position**: Overlap test fails → default position.
8. **Minimized state**: Forced to Normal, position/size still applied if valid.
9. **Maximized state**: Applied as-is.
10. **Unknown state string**: Treated as Normal.
11. **Display disconnected between sessions**: Position fails overlap check → default position.
12. **Headless mode**: display_count()=0 → position considered invalid → no-op. Size validation still applies. State always Normal.
13. **Multiple setup() calls**: Idempotent — reads and re-applies settings each time.
14. **Shutdown without setup**: `window_` or `settings_manager_` is null → save block skipped.
15. **Valid saved position with fallback size**: Position validated using fallback size (valid_w × valid_h).

## Security impact

None. Window position data is not sensitive. No new file I/O paths are introduced. No input validation beyond YAML type parsing (handled by existing `SettingsStore`).

## Data and migration impact

None. No schema changes. No existing keys to migrate. The `.buddd/user/settings.yaml` file will grow new `editor.window.*` keys after the first editor shutdown. This file already exists and is maintained by the settings system (SPEC-036).

## API compatibility impact

- **Window abstract class**: Five new pure virtual methods added. All existing `Window` subclasses (`WindowSDL3`, `WindowHeadless`) MUST implement them or they will fail to compile. No existing code calls these methods, so there is no runtime compatibility issue.
- **Platform abstract class**: Two new pure virtual methods added. All existing `Platform` subclasses (`PlatformSDL3`, `PlatformHeadless`) MUST implement them. No existing code calls these methods.
- **Settings keys**: Five new keys added under `editor.window.*`. No existing code reads or writes these keys.
- **No public API changes**: The new methods are on classes that are not exposed as public API to end users. Internal consumers (Editor, tests) are the only callers.

## Documentation impact

- **README**: No changes needed.
- **Wiki pages**: After implementation, the wiki should be updated to document the new `Window` (position, state, resize) and `Platform` (display_count, display_bounds) methods. The `docs/wiki/architecture/overview.md` page should be updated.
- **Other specs**: SPEC-036 remains valid. SPEC-037 references it but does not modify it.

## ADR impact

- **ADR-019**: The implementation stays within the boundary — no SDL3 headers in `src/editor/`. The abstract `Window` and `Platform` interfaces are in `src/engine/`, and SDL3 implementations are in `src/engine/window/` and `src/engine/platform/`. Editor code uses only abstract methods.
- No new ADR needed.

## Done criteria

- [ ] `WindowState` enum defined in `src/engine/window/window.h` with `Normal`, `Maximized`, `Minimized`.
- [ ] `WindowPosition` struct defined in `src/engine/window/window.h` with `int x, y`.
- [ ] `Window` class has five new pure virtual methods: `position()`, `set_position()`, `state()`, `set_state()`, `resize()`.
- [ ] `WindowSDL3` implements all five methods using the correct SDL3 API calls (verified by code review and CI tests).
- [ ] `WindowHeadless` implements all five methods as no-op stubs (position returns {0,0}, state returns Normal).
- [ ] `DisplayBounds` struct defined in `src/engine/platform/platform.h` with `int x, y, width, height`.
- [ ] `Platform` class has two new pure virtual methods: `display_count()`, `display_bounds()`.
- [ ] `PlatformSDL3` implements both methods using `SDL_GetNumVideoDisplays` and `SDL_GetDisplayBounds` with bounds checking.
- [ ] `PlatformHeadless` implements stubs (count=0, bounds={0,0,0,0}).
- [ ] `window_utils.h` and `window_utils.cpp` created with `window_state_to_string()` and `parse_window_state()`.
- [ ] `Editor::setup()` reads window settings from `user_project_settings`, validates size (MIN_W=400, MIN_H=300), validates position (overlap test), validates state (minimized→normal, unknown→normal), and applies to window.
- [ ] `Editor::shutdown()` writes current window position/size/state to `user_project_settings` before `save_all()`.
- [ ] All log messages use the format specified in the contract and spec (Info level for restore/save, Warn level for fallbacks).
- [ ] No SDL3 headers included in `src/editor/editor.cpp` (ADR-019 compliance).
- [ ] `tests/engine/window_state_tests.cpp` created with all required test cases.
- [ ] `tests/engine/platform_display_tests.cpp` created with all required test cases.
- [ ] `tests/editor/window_settings_tests.cpp` created with all required test cases.
- [ ] Integration tests in `tests/editor/settings_integration_tests.cpp` extended with window settings round-trip tests.
- [ ] All tests pass on CI (both `BUDDD_HAS_DISPLAY=ON` and `BUDDD_HAS_DISPLAY=OFF`).
- [ ] Build succeeds with no new warnings.
