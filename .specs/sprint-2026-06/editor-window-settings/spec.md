# SPEC-037 — Editor Window Geometry Persistence

## Problem

The Buddd Editor currently opens every session with a hardcoded window size (1280×800) and lets SDL3 centre the window on the primary display. Users who prefer a different window size or position must resize and reposition the editor window on every launch. The window state (normal/maximized/minimized) is also reset to normal each time.

The settings infrastructure (SPEC-036) now exists with three tiers (editor, project, user_project) and is integrated into the Editor lifecycle. This feature is the first concrete consumer of that infrastructure: it stores window geometry (x, y, width, height) and window state in the `user_project_settings` tier so that the editor window restores to its previous position, size, and maximise/minimise state across restarts.

## Goals

- Persist editor window position (x, y), size (width, height), and window state (normal/maximized/minimized) to the `user_project_settings` tier on shutdown.
- Load and validate these saved settings on editor startup, applying them to the window if valid.
- Validate size: reject saved values below 400×300, fall back to default 1280×800.
- Validate position: the window must be at least partially visible on at least one connected display; if not, fall back to default (SDL3-centred) position.
- Validate state: if the saved state is `"minimized"`, force to `"normal"` on startup (do NOT restore minimized), but still apply any valid saved position/size.
- Add the minimal abstract API surface to `Window` (position, state, resize) and `Platform` (display count, display bounds) to support these operations without exposing SDL3 or platform-specific types to editor code.
- Provide unit and integration tests for all new behaviour, including validation, edge cases, and headless stubs.

## Non-goals

- No settings panel or UI for window geometry — window settings are transparent to the user.
- No per-monitor profile (the feature does not remember which monitor the window was on).
- No mid-session save of window settings — window settings are only saved on `Editor::shutdown()`.
- No window position animated transitions — position and size are applied immediately when read from settings.
- No support for multiple windows (the editor has exactly one window).
- No migration of old settings keys — there are no old keys to migrate since this is the first consumer of the settings system.
- No changes to ImGui docking layout — that is handled separately by the `.ini` file.
- No changes to how the window is initially created (still created with defaults in `run_app()` → `EngineService::create()` → `Platform::create_window()`).

## Actors

| Actor | Role |
|---|---|
| **Editor startup** (`Editor::setup`) | After `SettingsManager::load_all()`, reads saved window settings from `user_project_settings`, validates them, and applies valid values to the window via `Window` API. |
| **Editor shutdown** (`Editor::shutdown`) | Reads current window position, size, and state via `Window` API, writes them to `user_project_settings`, then calls `save_all()` which persists all dirty stores. |
| **Window** (abstract + SDL3/Headless) | Provides `position()`, `set_position()`, `state()`, `set_state()`, `resize()` virtual methods. `WindowSDL3` implements via SDL3 API. `WindowHeadless` provides no‑op stubs returning sensible defaults. |
| **Platform** (abstract + SDL3/Headless) | Provides `display_count()` and `display_bounds(index)` virtual methods for position validation. `PlatformSDL3` implements via `SDL_GetDisplayBounds`. `PlatformHeadless` returns 0 displays. |
| **Tests** | Create an `Editor` with a display (offscreen SDL3) or headless, exercise load/save/validation round-trips. |

## User-visible behaviour

- On first launch after this feature: the editor opens at the default size (1280×800), centred by SDL3. No `editor.window.*` keys exist yet in `user_project_settings`.
- On subsequent launches after the user has resized, moved, or maximised the window: the editor restores to the saved position, size, and state (unless the saved values fail validation).
- If the user previously had the editor minimised when they quit: the editor opens in normal state (not minimised) at the saved/validated position and size.
- If the user quits with the editor maximised: the editor reopens maximised at the saved position/size.
- If a saved size is unrealistically small (below 400×300): the editor falls back to the default 1280×800 but preserves saved position (if still valid).
- If a saved position refers to a display that is no longer connected: the editor opens centred on the primary (or only) display.
- In headless mode: window settings are saved and loaded but all operations are no-ops; no observable behaviour change.
- The settings file `.buddd/user/settings.yaml` now contains window geometry keys after the first editor shutdown.

## User stories

### Story 1 — Window Geometry Persists Across Restarts (Priority: P1)

*As a user, I want the editor to remember its window size, position, and maximise state between sessions so I don't have to resize and reposition the window every time I launch the editor.*

**Given** the editor window is at a non-default size (e.g., 1024×768), a non-default position, and maximised
**When** I quit the editor and relaunch it
**Then** the editor window opens at 1024×768, at the saved position, and maximised

### Story 2 — Invalid Size Falls Back to Default (Priority: P1)

*As a user, if my saved window size is corrupt or too small, I want the editor to use a reasonable default size instead of an unusable tiny window.*

**Given** the saved `editor.window.width` is less than 400 or saved `editor.window.height` is less than 300
**When** the editor starts
**Then** the editor window opens at 1280×800

### Story 3 — Invalid Position Falls Back to Default (Priority: P1)

*As a user, if my saved window position refers to a disconnected monitor, I want the editor to appear on the current primary display rather than off-screen.*

**Given** the saved window position is on a display that is no longer connected
**When** the editor starts
**Then** the editor window opens centred on an available display

### Story 4 — Minimised State on Startup Is Forcefully Restored (Priority: P2)

*As a user who accidentally left the editor minimised, I want it to open in normal state so I can immediately use it.*

**Given** the saved window state was `"minimized"` when the editor was last shut down
**When** the editor starts
**Then** the editor window is in normal state (not minimised)
**And** the saved position and size are still applied if they validate

### Story 5 — Saved State Persists Correctly (Priority: P2)

*As a user, I want the editor to save whether I had it normal or maximised so that it restores to the same visual state.*

**Given** the editor is running in normal (or maximised) state
**When** I quit the editor
**Then** the window state is saved as `"normal"` (or `"maximized"`) in `user_project_settings`

### Story 6 — Position/Size/State Save on Every Shutdown (Priority: P1)

*As a developer, I want window settings to be written to disk during every clean shutdown so they survive restarts.*

**Given** the Editor is running with a valid window
**When** `Editor::shutdown()` is called
**Then** the current window position, size, and state are written to `user_project_settings` before `save_all()` is invoked

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `WindowState` enum is defined in `window.h` with values `Normal`, `Maximized`, `Minimized`. `WindowPosition` struct is defined with `int x, y`. | Code review: check `window.h` for `enum class WindowState` and `struct WindowPosition`. |
| AC-002 | `Window` class gains pure virtual `position() -> WindowPosition`, `set_position(WindowPosition)`, `state() -> WindowState`, `set_state(WindowState)`, and `resize(int w, int h)` methods. | Code review: verify the five new pure virtual methods in `window.h`. |
| AC-003 | `WindowSDL3` implements `position()` via `SDL_GetWindowPosition`, `set_position()` via `SDL_SetWindowPosition`, `state()` via `SDL_GetWindowFlags`, `set_state()` via `SDL_RestoreWindow`/`SDL_MaximizeWindow`/`SDL_MinimizeWindow`, and `resize()` via `SDL_SetWindowSize` (plus immediate cache update for width/height). | Code review: verify SDL3 API calls in `window_sdl3.cpp`. |
| AC-004 | `WindowHeadless` implements all five new methods as no-ops: `position()` returns `{0, 0}`, `state()` returns `WindowState::Normal`, `set_position()`/`set_state()`/`resize()` do nothing (width/height cached values unchanged). | Code review: verify stub implementations in `window_headless.cpp`. |
| AC-005 | `DisplayBounds` struct is defined in `platform.h` with `int x, y, width, height`. | Code review: check `platform.h` for `struct DisplayBounds`. |
| AC-006 | `Platform` class gains pure virtual `display_count() -> int` and `display_bounds(int index) -> DisplayBounds`. | Code review: verify two new pure virtual methods in `platform.h`. |
| AC-007 | `PlatformSDL3` implements `display_count()` via `SDL_GetNumVideoDisplays` and `display_bounds()` via `SDL_GetDisplayBounds`. | Code review: verify SDL3 API calls in `platform_sdl3.cpp`. |
| AC-008 | `PlatformHeadless` implements `display_count()` returning `0`; `display_bounds()` returns `DisplayBounds{0,0,0,0}` (behaviour is undefined if called when count is 0). | Code review: verify headless stubs in `platform_headless.cpp`. |
| AC-009 | On `Editor::setup()`, after `load_all()`, window settings are read from `user_project_settings`, validated, and applied to the window. | Integration test (see E2E Verification below). |
| AC-010 | Size validation: if saved `editor.window.width` < 400 or `editor.window.height` < 300, the editor uses default size (1280×800). Saved position is still validated independently. | Unit test: inject settings with w=399/h=299 and w=400/h=300 and w=1920/h=1080; verify fallback only for below-minimum. |
| AC-011 | Position validation: position is valid if the window rect (using the validated width/height) overlaps at least one display bounds rect by at least 1 pixel. If `display_count() == 0`, skip position validation (position is considered invalid → use default). | Unit test: mock `Platform` (or test with real PlatformSDL3) to inject display bounds; test overlap, no-overlap, zero-display cases. |
| AC-012 | State validation: if saved `editor.window.state` is `"minimized"`, the window state is forced to `WindowState::Normal` on startup. Other valid states (`"normal"`, `"maximized"`) are applied as-is. | Unit test: inject `editor.window.state = "minimized"` and verify window state is Normal after load. |
| AC-013 | State parsing: unknown state strings (e.g., `"fullscreen"`, `""`) are treated as `WindowState::Normal`. | Unit test: inject various bad strings, verify applied state is Normal. |
| AC-014 | On `Editor::shutdown()`, before `save_all()`, the current window position, size, and state are written to `user_project_settings` under keys `editor.window.x`, `editor.window.y`, `editor.window.width`, `editor.window.height`, `editor.window.state`. | Integration test: shutdown editor, read `.buddd/user/settings.yaml`, verify keys exist with expected types. |
| AC-015 | Values written on shutdown use the correct types: `x`, `y`, `width`, `height` are `int32_t`; `state` is `std::string` (`"normal"`, `"maximized"`, or `"minimized"`). | Integration test: after shutdown, load YAML file, verify types match. |
| AC-016 | When position validation fails (display disconnected), `set_position()` is not called — the window keeps its default (SDL3-centred) position. | Unit test: inject settings with position referring to disconnected display, verify `set_position` is not invoked (or invoked with a flag indicating no-op). |
| AC-017 | When size validation fails (w < 400 or h < 300), `resize(DEFAULT_WIDTH, DEFAULT_HEIGHT)` is called. | Unit test: inject tiny saved size, verify resize is called with 1280×800. |
| AC-018 | The window's `resize()` method updates the cached `width()`/`height()` immediately (not deferred to the next event loop iteration). | Unit test: call `resize(w, h)`, immediately check `width()`/`height()` return the new values. |
| AC-019 | State string conversion functions (`window_state_to_string`, `parse_window_state`) round-trip correctly: `parse_window_state(window_state_to_string(s)) == s` for all three valid states. | Unit test: verify round-trip for `Normal`, `Maximized`, `Minimized`. |
| AC-020 | In headless mode, `Editor::setup()` reads window settings from `user_project_settings` but `WindowHeadless` methods are no-ops — no crash, no error. | Headless test: instantiate Editor with headless backend, call setup+shutdown, verify no errors and no unexpected behaviour. |
| AC-021 | `Editor::shutdown()` without a prior `Editor::setup()` does not attempt to save window settings (window pointer is null → skip). | Shield test: construct Editor, call shutdown (no setup), verify no crash. |
| AC-022 | Window settings keys use the dot-path convention `editor.window.*` and are stored in the `user_project_settings` tier (path: `<cwd>/.buddd/user/settings.yaml`). | Review: verify that `user_project_settings()` store is used for all `editor.window.*` keys. |
| AC-023 | No SDL3 or platform-specific headers are included in `src/editor/` code that reads or writes window settings — all platform interaction goes through `Window` and `Platform` abstract classes. | Code review: verify `editor.cpp` does not include any SDL3 header directly. |

## E2E Verification

- **Method**: Integration test (requires `BUDDD_HAS_DISPLAY=ON`) following the same pattern as `tests/editor/settings_integration_tests.cpp`.
  1. Create a temporary project directory.
  2. Construct an `EngineService` with an SDL3 offscreen display and an `Editor`.
  3. Pre-populate the `user_project_settings` store with a known window position/size/state (by writing YAML to `.buddd/user/settings.yaml` before setup).
  4. Call `Editor::setup()` — which loads settings and applies them to the window.
  5. Verify that the window's `width()`, `height()`, `position()`, and `state()` match the saved valid values.
  6. Resize/move the window programmatically and change its state.
  7. Call `Editor::shutdown()`.
  8. Read `.buddd/user/settings.yaml` and verify the five `editor.window.*` keys contain the expected final values.
  9. Run the same test with:
     - Minimum-size-boundary values (width=399, height=299 → fallback; width=400, height=300 → accepted).
     - Position that does not overlap any display → default position used.
     - State `"minimized"` on disk → normal on startup, saved position/size still applied.
  10. Repeat the same test with `Backend::Headless` to verify no-crash behaviour.

This test requires a display and runs only when `BUDDD_HAS_DISPLAY=ON`. Separate headless-compatible unit tests cover per-class behaviour for the new Window and Platform methods.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All five `Window` new virtual methods have test coverage on both `WindowSDL3` and `WindowHeadless`. |
| SC-002 | Both `Platform` display query methods have test coverage on `PlatformSDL3` and `PlatformHeadless`. |
| SC-003 | Size validation (minimum 400×300) is tested with boundary values (399, 400, 299, 300) and default fallback. |
| SC-004 | Position validation (overlap with at least one display) is tested with valid overlap, no overlap, and zero displays. |
| SC-005 | State validation round-trip is tested for all three states plus unknown-string fallback. |
| SC-006 | The full save/load round-trip passes: settings survive a simulated restart (setup → shutdown → setup). |

## Key entities

### `WindowState` enum (in `src/engine/window/window.h`)

```cpp
enum class WindowState { Normal, Maximized, Minimized };
```

### `WindowPosition` struct (in `src/engine/window/window.h`)

```cpp
struct WindowPosition { int x; int y; };
```

### `DisplayBounds` struct (in `src/engine/platform/platform.h`)

```cpp
struct DisplayBounds { int x; int y; int width; int height; };
```

### `Window` abstract class — new virtual methods

Added to the existing `Window` abstract class in `src/engine/window/window.h`:

```cpp
[[nodiscard]] virtual auto position() const -> WindowPosition = 0;
virtual auto set_position(WindowPosition pos) -> void = 0;
[[nodiscard]] virtual auto state() const -> WindowState = 0;
virtual auto set_state(WindowState state) -> void = 0;
virtual auto resize(int width, int height) -> void = 0;
```

- **`WindowSDL3`** maps to SDL3 API:
  - `position()` → `SDL_GetWindowPosition(window_, &x, &y)`
  - `set_position(p)` → `SDL_SetWindowPosition(window_, p.x, p.y)`
  - `state()` → `SDL_GetWindowFlags(window_)` & `SDL_WINDOW_MAXIMIZED` / `SDL_WINDOW_MINIMIZED`
  - `set_state(s)` → `SDL_MaximizeWindow` / `SDL_RestoreWindow` / `SDL_MinimizeWindow`
  - `resize(w, h)` → `SDL_SetWindowSize(window_, w, h)` followed by immediate `width_ = w; height_ = h;`
- **`WindowHeadless`** provides no-op stubs:
  - `position()` returns `{0, 0}`
  - `set_position(p)` is a no-op
  - `state()` returns `WindowState::Normal`
  - `set_state(s)` is a no-op
  - `resize(w, h)` sets `width_ = w; height_ = h;` (updates cached values only)

### `Platform` abstract class — new virtual methods

Added to the existing `Platform` abstract class in `src/engine/platform/platform.h`:

```cpp
[[nodiscard]] virtual auto display_count() const -> int = 0;
[[nodiscard]] virtual auto display_bounds(int index) const -> DisplayBounds = 0;
```

- **`PlatformSDL3`** maps to SDL3 API:
  - `display_count()` → `SDL_GetNumVideoDisplays()`
  - `display_bounds(i)` → `SDL_GetDisplayBounds(i, &rect)` → convert `SDL_Rect` to `DisplayBounds`
  - If `index` is out of bounds, returns `DisplayBounds{0,0,0,0}` (caller should bounds-check)
- **`PlatformHeadless`** stubs:
  - `display_count()` returns `0`
  - `display_bounds(i)` returns `DisplayBounds{0,0,0,0}` (undefined behaviour if called when count is 0)

### State string conversion functions

Two free helper functions (location TBD, possibly in `window.h` or a new `window_utils.h`):

```cpp
auto window_state_to_string(WindowState state) -> std::string;
auto parse_window_state(const std::string& str) -> WindowState;
```

- `window_state_to_string(Normal)` → `"normal"`, `Maximized` → `"maximized"`, `Minimized` → `"minimized"`
- `parse_window_state("normal")` → `Normal`, `"maximized"` → `Maximized`, `"minimized"` → `Minimized`
- `parse_window_state(any_other_string)` → `Normal` (fallback)

### Settings keys

All stored in `user_project_settings` (the `.buddd/user/settings.yaml` file):

| Key | Type | Default | Description |
|---|---|---|---|
| `editor.window.x` | `int32_t` | (unset) | Window left edge position in screen coordinates |
| `editor.window.y` | `int32_t` | (unset) | Window top edge position in screen coordinates |
| `editor.window.width` | `int32_t` | `1280` | Window inner width in pixels |
| `editor.window.height` | `int32_t` | `800` | Window inner height in pixels |
| `editor.window.state` | `std::string` | `"normal"` | Window state: `"normal"`, `"maximized"`, `"minimized"` |

Default values are not written to the store until the first shutdown.

### Validation algorithm (executed in `Editor::setup()` after `load_all()`)

```
Inputs:
  raw_w   = user_project_settings.get<int32_t>("editor.window.width",  1280)
  raw_h   = user_project_settings.get<int32_t>("editor.window.height", 800)
  raw_x   = user_project_settings.get<int32_t>("editor.window.x",      0)
  raw_y   = user_project_settings.get<int32_t>("editor.window.y",      0)
  raw_s   = user_project_settings.get<std::string>("editor.window.state", "normal")

Constants:
  MIN_W     = 400
  MIN_H     = 300
  DEFAULT_W = 1280
  DEFAULT_H = 800

Procedure:
  1. Size validation
     if raw_w < MIN_W or raw_h < MIN_H:
         valid_w = DEFAULT_W
         valid_h = DEFAULT_H
     else:
         valid_w = raw_w
         valid_h = raw_h

  2. Position validation
     position_valid = false
     display_count = platform.display_count()
     if display_count > 0:
         for i = 0 .. display_count-1:
             bounds = platform.display_bounds(i)
             // Overlap test: at least 1 pixel of window rect must be inside display rect
             if (raw_x < bounds.x + bounds.width
                 and raw_x + valid_w > bounds.x
                 and raw_y < bounds.y + bounds.height
                 and raw_y + valid_h > bounds.y):
                 position_valid = true
                 break
     // If display_count == 0 (headless), position_valid stays false → use default position

  3. State validation
     state = parse_window_state(raw_s)
     if state == WindowState::Minimized:
         state = WindowState::Normal   // force normal on startup

  4. Application
     window.resize(valid_w, valid_h)
     if position_valid:
         window.set_position({raw_x, raw_y})
     window.set_state(state)
```

### Save algorithm (executed in `Editor::shutdown()` before `save_all()`)

```
if window_ != nullptr and settings_manager_ != nullptr:
    auto& ups = settings_manager_->user_project_settings()
    auto pos = window_->position()
    ups.set<int32_t>("editor.window.x",       pos.x)
    ups.set<int32_t>("editor.window.y",       pos.y)
    ups.set<int32_t>("editor.window.width",   window_->width())
    ups.set<int32_t>("editor.window.height",  window_->height())
    ups.set<std::string>("editor.window.state", window_state_to_string(window_->state()))
    // user_project_settings is now dirty → save_all() will persist it

    settings_manager_->save_all()
```

### Execution order in `Editor::setup()`

1. Existing code runs (construct SettingsManager, set layout_ini_path, call load_all()) [as in SPEC-036].
2. **New**: Read window settings from `user_project_settings`, validate, apply to window.
3. Existing code continues (create menu bar, register panels, bind shortcuts, set window title).

### Execution order in `Editor::shutdown()`

1. Existing code: `ImGui::GetIO().IniFilename = nullptr;`
2. **New**: If `window_` and `settings_manager_` are non-null, write current window geometry/state to `user_project_settings`.
3. Existing code: `settings_manager_->save_all()` (persists all dirty stores, including the now-dirty `user_project_settings`).
4. Existing code: `initialized_ = false; engine_ = nullptr; window_ = nullptr;`

## Edge cases

| Case | Expected behaviour |
|---|---|
| Settings file contains no `editor.window.*` keys (first launch). | Default values are used: size 1280×800, default position (SDL3-centred), normal state. |
| Saved width=399, height=800. | Width < 400 → fallback to 1280×800. |
| Saved width=400, height=299. | Height < 300 → fallback to 1280×800. |
| Saved width=400, height=300. | Both at minimum boundary → accepted. |
| Saved width=0 or negative. | Treated as below minimum → fallback to defaults. |
| Saved position is (-500, -500) but window at that position partially overlaps display at (0,0,1920,1080). | Partially visible → position is valid (window rect: x=-500, w=1280 → visible portion: x=0..780 which overlaps display 0..1920). |
| Saved position is (-1500, -1500) with window size 1280×800 on a 1920×1080 display at (0,0). | No overlap → position invalid, use default (centred). |
| Saved position is (0, 0) but display was moved/changed and current display bounds are (1920, 0, 1920, 1080) [second monitor became primary]. | Overlap test: window rect (0,0,1280,800) vs display (1920,0,1920,1080) → 0 < 1920+1920 && 0+1280 > 1920 → 1280 > 1920 is false → no overlap. Position invalid → use default. |
| Saved state is `"minimized"` with valid position/size. | State forced to Normal; position and size still applied. |
| Saved state is `"maximized"` with valid position/size. | State applied as Maximized; position/size are still saved and will be restored if the user un-maximises (or if on next startup the state is Normal but position/size from last shutdown are applied). |
| Saved state is an unknown string (e.g., `"fullscreen"`, empty string). | Treated as Normal. |
| Display count changes between sessions (e.g., external monitor disconnected). | Position validated against current displays; if no overlap, use default position. |
| Headless mode (no display at all). | `display_count()` returns 0 → position validation skipped → position considered invalid → default position used (no-op). Size validation still applies. State is always Normal in headless; setting state is no-op. |
| `Editor::setup()` is called multiple times. | Window settings are re-read and re-applied on each call (idempotent). |
| `Editor::shutdown()` is called without preceding `setup()`. | No crash: `window_` and `settings_manager_` are null → window settings save is skipped. |
| Window is closed by OS close button (triggers shutdown path). | Window settings are saved during the subsequent `Editor::shutdown()` call. |
| Saved position is valid but the validated size differs from the saved size (due to fallback). | Position is validated using the validated size (valid_w × valid_h). If the window at valid size and saved position is still at least partially visible, the position is kept. |

## Error cases

| Error | Scenario | Behaviour |
|---|---|---|
| `display_bounds(i)` called with an out-of-range index. | The implementation returns `{0,0,0,0}`. The validation loop iterates `0..display_count()-1`, so out-of-range access should never occur in correct code. Defensive: bounds-check in PlatformSDL3. |
| User_project_settings YAML file is corrupt. | `load_all()` returns an error (handled by existing SPEC-036 error handling). The editor logs a warning and continues with defaults. Window settings get their default values. |
| Window resize fails (e.g., SDL_SetWindowSize returns false). | The implementation logs a warning. The window remains at its previous size. The saved settings are still written on shutdown (best-effort). |
| Platform display query fails (unlikely with SDL3). | `display_count()` returns 0 or `display_bounds()` returns an empty rect. Position validation falls through to default position gracefully. |

## Permissions and security

- Window settings are stored in `user_project_settings` at `.buddd/user/settings.yaml` — the same per-project per-user non-version-controlled location used by the rest of the settings system.
- No new file paths or I/O concerns beyond what SPEC-036 already defines.
- No sensitive data is stored in window settings.
- Window position could theoretically be used to infer display layout; this is no different from any application that restores its window position.

## Observability

- Window settings load: logged at `Info` level, e.g.:
  - `"Editor: restoring window geometry from user settings (1280x800 + {100, 50}, Normal)"`
  - `"Editor: window position invalid (no overlapping display), using default"`
  - `"Editor: window size below minimum (300x200), using default (1280x800)"`
- Window settings save: logged at `Info` level, e.g.:
  - `"Editor: saving window geometry (1024x768 + {200, 100}, Maximized)"`
- Validation failures are logged at `Warn` level:
  - `"Editor: saved window state was 'minimized' — forcing normal on startup"`
  - `"Editor: saved window state 'fullscreen' is unknown — using normal"`
- All log messages use the tag `"Editor"` (consistent with existing editor logging).
- No new log tag is introduced.

## Out of scope

- Settings panel for window geometry — no UI.
- Per-monitor profile or display-name-based position recovery.
- Mid-session save of window settings.
- Animated transitions for window repositioning.
- Support for multiple editor windows.
- Full-screen window state (only normal/maximized/minimized).
- Remembering which window state was active when restoring maximized (the feature saves the current state at shutdown; if maximized, next session opens maximized).

## Assumptions

| Assumption | Rationale |
|---|---|
| The editor has exactly one window, accessible via `Editor::window_` which is set in `setup()`. | Consistent with existing architecture — `Editor` stores a `Window*` pointer. |
| The window exists (non-null) for the entire lifetime between `setup()` and `shutdown()` when a display is present. | The window is created in `EngineService::create()` and destroyed after `Editor::shutdown()`. |
| The SDL3 offscreen video driver supports `SDL_GetWindowPosition`, `SDL_SetWindowSize`, `SDL_GetWindowFlags`, and `SDL_GetDisplayBounds`. | The offscreen driver implements a minimal subset of SDL3; these calls return sensible values or zero. Verified by integration tests. |
| `display_count()` returns 0 in headless mode. | Headless has no displays — consistent with the abstract `Platform` design. |
| Position (0,0) is the top-left of the primary display. | Standard SDL3 coordinate convention. All display bounds are relative to this origin. |
| Saved position/size values may be stale if the user changed display configuration between sessions. | The validation algorithm handles this by checking overlap with current displays. |
| The `SettingsStore` `set<T>()` call for an unchanged value does not mark the store dirty (as per SPEC-036 AC-023). | If window geometry hasn't changed between shutdowns, the store is not dirtied and `save_all()` is a no-op for that store. |
| No concurrent writes to user_project_settings from other features during shutdown. | The editor is single-threaded during shutdown. |

## Existing documentation that must be updated

- `docs/adr/ADR-019-architecture-boundaries.md` — No update needed. The new `Window` and `Platform` virtual methods stay within the existing architecture boundary (abstract interfaces in `src/engine/`, concrete backends in `src/engine/window/` and `src/engine/platform/`). Editor code (`src/editor/`) still uses only the abstract interfaces.
- `docs/adr/ADR-012-navigable-object-graph-engine-service.md` — No update needed. The navigable object graph (`RenderDevice → Window → Platform → InputSystem`) is unchanged.
- `.specs/sprint-2026-06/settings-system/spec.md` — No changes needed to SPEC-036 itself, but this SPEC-037 is the first concrete consumer of the settings infrastructure. The existing ACs in SPEC-036 remain valid and are relied upon by this feature.
- `docs/wiki/architecture/overview.md` — After implementation, the wiki-agent should update the overview page to document the new `Window` and `Platform` methods.
- No other wiki pages currently document window geometry or platform display query — after implementation, the wiki-agent should create or update relevant wiki pages.

## Open questions

No open questions — all design decisions were resolved during the grill-me step.
