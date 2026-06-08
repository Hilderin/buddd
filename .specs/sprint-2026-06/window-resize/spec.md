# SPEC-2026-06-001 — Window Resize for All Apps

## Problem

Currently all windows created by the buddd engine are fixed-size — the user cannot resize them by dragging borders or corners. The `Window` abstraction caches `width`/`height` only at construction time, so even if the underlying OS window is resized (e.g. by the window manager), the engine's cached dimensions and Dear ImGui's display size never update. This produces a poor user experience in the 14 demo apps and the editor, and prevents the editor from being used in a flexible workspace.

The OpenGL viewport already adapts automatically each frame (via `SDL_GetWindowSize()` + `glViewport()` in `RenderDeviceOpenGL::begin_frame()`), but the rest of the system — the `Window` cache, Dear ImGui sizing, and any app logic querying `Window::width()/height()` — does not.

## Goals

- All windows (SDL3-backed) are resizable by dragging borders/corners using standard OS window decorations.
- The `Window` abstraction's cached `width()`/`height()` reflects the actual window size at all times.
- Dear ImGui adapts its display size to the new window size automatically.
- The OpenGL viewport continues to fill the window (existing behavior, no change needed).
- Headless windows support explicit resize for consistency and testability.
- Minimum window size of 320×240 prevents degenerate unusable sizes.
- Camera aspect ratios are NOT automatically adjusted — apps opt in by querying `Window::width()/height()`.

## Non-goals

- No `App::on_resize()` callback is added. Apps that need to react to resize poll `Window::width()/height()` each frame.
- No `resizable` flag is introduced — all windows are unconditionally resizable.
- The existing `RenderDeviceOpenGL::begin_frame()` and `RenderDeviceOpenGL::size()` behaviors are not modified — they already query `SDL_GetWindowSize()` directly.
- No HiDPI / DPI change handling — this is separate from basic window resize.
- No programmatic window placement (centering, tiling) or fullscreen toggle — pure border-drag resize only.
- No `WindowConfig` schema changes — `width` and `height` remain the initial creation size only.

## Actors

| Actor | Role |
|---|---|
| **User** | Drags a window border or corner to resize it. |
| **PlatformSDL3** | SDL3 platform backend. Owns the event loop (`poll_events()`) and creates SDL3 windows. Routes SDL events to the input system and ImGui. |
| **PlatformHeadless** | Headless platform backend. Creates headless windows. No OS event loop. |
| **Window** | Abstract base class caching `width()` / `height()`. Provides `on_resize(w, h)` virtual method. |
| **WindowSDL3** | SDL3 window implementation. Maintains cached `width_` / `height_`. Updates them via `on_resize()`. |
| **WindowHeadless** | Headless window implementation. Maintains cached `width_` / `height_`. Updates them via `on_resize()`. |
| **engine_imgui** | ImGui integration module. `on_sdl_event()` forwards events to `ImGui_ImplSDL3_ProcessEvent()`. |
| **RenderDeviceOpenGL** | OpenGL render device. Queries `SDL_GetWindowSize()` each frame in `begin_frame()` → viewport already adapts. |
| **RenderDeviceHeadless** | Headless render device. `size()` delegates to `Window::width()/height()`. |
| **EngineService** | Creates and owns the Platform → Window → RenderDevice chain. |

## User-visible behavior

1. **Resizable window borders**: Every app window created via `PlatformSDL3::create_window()` has standard OS window decorations with draggable resize borders and corners.
2. **Viewport fills window**: When the user drags to resize, the OpenGL viewport automatically fills the new dimensions (already works — no visual gap or clipping).
3. **ImGui adapts**: Dear ImGui UI elements reflow to fit the new display size. Toolbars, menus, and overlays are not clipped after a resize.
4. **No automatic camera adjustment**: 3D scenes maintain their original aspect ratio and field of view unless the app explicitly queries `Window::width()/height()` and updates the camera projection.
5. **Minimum size enforcement**: The window cannot be resized below 320×240 pixels. Attempts to drag below this size are clamped by the OS.
6. **Headless resize testable**: A headless window can be resized programmatically via `on_resize(800, 600)` and `width()/height()` reflect the new values — no visual output, but verifiable via test assertions.

## User stories

### Story 1 — Resize demo app window (Priority: P1)

A user runs any demo app (e.g., `budd-run-demo-spinning-cube`). The window appears with standard OS title bar and resize handles.

**Given** the demo app is running with an SDL3 window  
**When** the user clicks and drags the right edge of the window to the right  
**Then** the window width increases and the 3D viewport fills the expanded area  
**And** any ImGui overlay adjusts to the new display size  
**And** `eng.window().width()` returns the new width when queried by the app

**Given** the demo app is running  
**When** the user drags a corner handle to shrink the window below 320×240  
**Then** the window is clamped to a minimum of 320×240 by the OS

### Story 2 — Minimize and restore window (Priority: P2)

A user minimizes a running demo app and then restores it.

**Given** the demo app is running  
**When** the user minimizes the window and then restores it  
**Then** the window returns to its previous size  
**And** the viewport and ImGui continue to render correctly

### Story 3 — Headless window resize (Priority: P2)

A developer writes a headless integration test that exercises the resize path.

**Given** a headless window of size 640×480  
**When** the developer calls `window.on_resize(800, 600)`  
**Then** `window.width()` returns 800  
**And** `window.height()` returns 600

### Story 4 — Editor window resize (Priority: P1)

The editor app (which is also a buddd app) runs with a resizable window.

**Given** the editor is running  
**When** the user resizes the editor window  
**Then** the editor viewport and UI panels adapt to the new window size  
**And** no editor functionality breaks

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `Window` base class declares `virtual auto on_resize(int w, int h) -> void = 0;` (pure virtual). | Compile check — inspect `window.h` for `on_resize` declaration with `= 0`. |
| AC-002 | `WindowSDL3` overrides `on_resize()` to update `width_` and `height_`. | Unit test: construct `WindowSDL3`, call `on_resize(1024, 768)`, verify `width() == 1024` and `height() == 768`. |
| AC-003 | `WindowHeadless` overrides `on_resize()` to update `width_` and `height_`. | Headless integration test: construct `WindowHeadless`, call `on_resize(800, 600)`, verify `width() == 800` and `height() == 600`. |
| AC-004 | `PlatformSDL3::create_window()` passes `SDL_WINDOW_RESIZABLE` flag to `SDL_CreateWindow()`. | Compile check — inspect `platform_sdl3.cpp` for `SDL_WINDOW_RESIZABLE` in the flags bitmask. |
| AC-005 | `PlatformSDL3::create_window()` calls `SDL_SetWindowMinimumSize(window, 320, 240)` after window creation. | Compile check — inspect `platform_sdl3.cpp` for `SDL_SetWindowMinimumSize` call. |
| AC-006 | `PlatformSDL3::poll_events()` handles `SDL_EVENT_WINDOW_RESIZED` and calls `window_resize_callback()` with the new dimensions. | Automated test: use a test helper that injects an `SDL_EVENT_WINDOW_RESIZED` event and verifies the window cache updates. |
| AC-007 | When `SDL_EVENT_WINDOW_RESIZED` fires, the event is forwarded to `engine_imgui::on_sdl_event()` (already routed in the event loop). | Manual code inspection — verify the resize event is passed through the existing `engine_imgui::on_sdl_event()` path. |
| AC-008 | The window cannot be resized below 320×240. | Manual test: attempt to drag window border below minimum; observe clamping. |
| AC-009 | Existing `RenderDeviceOpenGL::begin_frame()` and `RenderDeviceOpenGL::size()` continue to work unchanged. | Full existing test suite passes. |
| AC-010 | `RenderDeviceHeadless::size()` reflects updated dimensions after `on_resize()`. | Headless integration test: create headless window + render device, call `window.on_resize(800, 600)`, verify `device.size() == {800, 600}`. |
| AC-011 | Existing test suite (headless + any SDL3 conditional tests) passes with no regressions. | Run all tests: `ctest` or equivalent. |

## E2E Verification

- **Method 1 (automated, headless)**: Headless integration test (`tests/window_resize_tests.cpp`) that:
  1. Creates a headless platform, window, and render device.
  2. Verifies initial dimensions.
  3. Calls `window.on_resize(800, 600)`.
  4. Asserts `window.width() == 800`, `window.height() == 600`.
  5. Asserts `device.size() == {800, 600}`.
  6. Verifies `on_resize(320, 240)` minimum and `on_resize(319, 240)` → width is clamped to at least 320 (in headless, clamping is in the caller's hands; headless accepts any value — verification confirms the method updates correctly).

- **Method 2 (manual, SDL3)**: Run any demo app (`budd-run-demo-spinning-cube` for example), drag window borders/corners, visually confirm:
  - Viewport fills the resized area.
  - ImGui overlays reflow (e.g., FPS counter repositioned correctly).
  - Window cannot be shrunk below 320×240.
  - Window can be minimized and restored without issues.

- **Method 3 (automated, CI)**: Full test suite must pass: `ctest --output-on-failure`.

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | All windows are draggably resizable. | Manual test with each of the 14 demo apps + editor. |
| SC-002 | Viewport fills window after resize. | Visual inspection — no black bars or clipped rendering. |
| SC-003 | ImGui display size matches new window size. | ImGui debug widget shows correct display size after resize. |
| SC-004 | Headless `on_resize()` updates dimensions. | Headless integration test passes. |
| SC-005 | Existing test suite has zero regressions. | `ctest` passes. |

## Edge cases

| # | Edge case | Expected behavior |
|---|---|---|
| EC-01 | **Resize below minimum (320×240)**: User drags border to below 320 or 240 pixels. | OS/clamp enforces minimum — window stops at 320×240. The `SDL_EVENT_WINDOW_RESIZED` event fires with the clamped dimensions. |
| EC-02 | **Minimize then restore**: User minimizes the app window and later restores it. | The `SDL_EVENT_WINDOW_RESTORED` or `SDL_EVENT_WINDOW_RESIZED` event fires. The window cache and ImGui update to the restored dimensions. If the window manager sends `SDL_EVENT_WINDOW_RESIZED` on restore, the existing handler covers it. |
| EC-03 | **Rapid resize**: User drags resize handle very fast. | `SDL_EVENT_WINDOW_RESIZED` fires for each intermediate size. Each event is processed in `poll_events()` and the cache is updated. Performance is not an issue — update is O(1) integer assignment. |
| EC-04 | **Headless resize to zero or negative dimensions**: Test calls `on_resize(-1, -1)` or `on_resize(0, 0)`. | Headless accept any value (no clamping) — but `WindowConfig` validation (`width > 0 && height > 0`) already prevents creation with invalid dimensions. The `on_resize()` method does NOT validate or clamp in headless (by design, for testing flexibility). The spec makes no guarantee about behavior with negative values — the test framework should not exercise these. **Implementation note**: downstream users of `width()`/`height()` may not expect negative values; the headless implementation should at minimum store the values as `int` members (no crash), but callers are responsible for bounds-checking before use. |
| EC-05 | **Resize event before window is fully created**: Very early resize event (race condition). | In practice, `SDL_CreateWindow()` returns after the window is created but before the first event loop iteration. Any resize event sent before the first `poll_events()` call is queued and processed on the first iteration — harmless. |
| EC-06 | **Window resized by window manager (snap/tile)**: User uses OS snap/tile shortcut to resize. | Same as drag resize — `SDL_EVENT_WINDOW_RESIZED` fires, handler updates cache and ImGui. |
| EC-07 | **Maximize**: User clicks the maximize button. | `PlatformSDL3` handles both `SDL_EVENT_WINDOW_MAXIMIZED` and `SDL_EVENT_WINDOW_RESTORED` events proactively. On maximize, `SDL_GetWindowSize()` is queried and `on_resize()` is called to update the cache. On restore, the same is done. This ensures the cache is never stale regardless of which events SDL3 fires. |
| EC-08 | **Multiple windows** (if supported in future): One window resized, others unaffected. | Each `WindowSDL3` instance has its own cache. `PlatformSDL3` maintains an `SDL_WindowID → Window*` map. `WindowSDL3` registers itself with `PlatformSDL3` during construction. Incoming resize events use `event.window.windowID` to route to the correct `Window` instance via the map. This approach is already future-proof for multi-window. |

## Error cases

| # | Error case | Expected behavior |
|---|---|---|
| ER-01 | `SDL_SetWindowMinimumSize()` fails (e.g., window handle invalid). | Log a warning via `BUDDD_LOG_WARN` and continue. The window will still be created without a minimum size constraint. |
| ER-02 | `SDL_EVENT_WINDOW_RESIZED` contains zero or negative dimensions. | This is extremely unlikely from SDL3. If it occurs, the window cache is updated to the (invalid) value. No crash — downstream systems may render incorrectly. A defensive `BUDDD_LOG_WARN` on zero/negative dimensions could be added. |
| ER-03 | ImGui not initialised when `SDL_EVENT_WINDOW_RESIZED` is forwarded. | `engine_imgui::on_sdl_event()` already handles this — returns `false` and is a no-op if not initialised. No crash. |

## Permissions and security

- No changes to permissions or security model.
- Window resize is a standard OS-provided capability with no additional attack surface.
- On platforms where SDL3 is used, the user already granted window management permissions implicitly (display access).

## Observability

- Every resize event logged at `BUDDD_LOG_DEBUG` level in the event handler (both `SDL_EVENT_WINDOW_RESIZED` and `SDL_EVENT_WINDOW_MAXIMIZED`/`RESTORED`): `BUDDD_LOG_DEBUG("Window resize: {}x{} (windowID={})", w, h, windowID)`.
- Final size logged at `BUDDD_LOG_INFO` level when resize events settle (e.g., when the mouse button is released after a drag, or when no resize event has been received for a short period): `BUDDD_LOG_INFO("Window resized to final size: {}x{}", w, h)`.
- `BUDDD_LOG_WARN("SDL_SetWindowMinimumSize failed: {}")` if the minimum size constraint cannot be set.
- Existing `BUDDD_LOG_INFO("Window created: {}x{}", ...)` remains unchanged.
- Headless resize is not logged (no event loop) — tests use assertions for verification.

## Documentation updates

The following existing documentation files must be updated to reflect the new resize feature:

| Document | Required changes |
|---|---|
| `docs/wiki/architecture/module-map.md` | Add `on_resize()` virtual method to the `Window` entry, the `SDL_WindowID → Window*` map in `PlatformSDL3`, `SDL_WINDOW_RESIZABLE` flag, and `SDL_SetWindowMinimumSize()` call. |
| `docs/wiki/architecture/data-flow.md` | May need updating for the new resize event flow (SDL resize event → PlatformSDL3 routing via windowID map → `Window::on_resize()` → cache update → ImGui reflow). |
| Any other wiki files referencing the `Window` or `Platform` APIs | Audit at implementation time — if a page describes `Window` virtual methods, `PlatformSDL3::create_window()` flags, or the event loop, it must be updated to reflect the new resize behavior. |

## Out of scope

- Programmatic resize API (e.g., `Window::set_size(800, 600)` that triggers an OS window resize) — not needed for this feature.
- Fullscreen toggle (`SDL_SetWindowFullscreen()`) — separate feature.
- HiDPI / DPI change notification — the existing viewport and ImGui adaptation may or may not handle this correctly; it is not tested or guaranteed.
- Custom window decorations / client-side title bar — we use standard OS decorations.
- Per-monitor resize behavior or multi-monitor awareness.
- Resize handles on the headless backend (headless has no visual window).
- Camera / projection auto-update — apps must opt in.

## Assumptions

1. **SDL3** handles minimum size enforcement via `SDL_SetWindowMinimumSize()` — the window manager will clamp the window size from below.
2. **`SDL_EVENT_WINDOW_RESIZED`** is the correct event to handle (as opposed to `SDL_EVENT_WINDOW_SIZE_CHANGED`). `RESIZED` fires after the size has been applied; `CHANGED` fires on any size change including programmatic. Using `RESIZED` is sufficient for our use case.
3. **ImGui SDL3 backend** (`ImGui_ImplSDL3_ProcessEvent`) correctly handles `SDL_EVENT_WINDOW_RESIZED` and updates its internal display size. This is confirmed by ImGui's source — the backend calls `ImGui::GetIO().DisplaySize` from the window dimensions stored in the SDL event.
4. **Window handle routing via windowID map** — `PlatformSDL3` maintains a `std::unordered_map<SDL_WindowID, Window*>` to route SDL events to the correct `Window` instance. `WindowSDL3` registers itself during construction by calling `PlatformSDL3::register_window()`. This approach is future-proof for multi-window support.
5. **Thread safety** — All resize handling happens on the main thread (the event loop). No synchronization is needed.
6. **Headless accepts any dimensions** — `WindowHeadless::on_resize()` performs no clamping or validation. This is intentional for test flexibility.
7. **Window map in PlatformSDL3** — `PlatformSDL3` stores a `std::unordered_map<SDL_WindowID, Window*>` that maps `SDL_WindowID` → `Window*`. `WindowSDL3` registers itself with `PlatformSDL3` during construction (e.g., via `PlatformSDL3::register_window(this)`). When `SDL_EVENT_WINDOW_RESIZED` (or MAXIMIZED/RESTORED) fires, `PlatformSDL3` looks up the window ID from `event.window.windowID` in the map and calls `on_resize()` on the correct instance.

## Open questions

*None. All previously identified open questions have been resolved during spec review:*
- *Maximize/restore: Handle both `SDL_EVENT_WINDOW_MAXIMIZED` and `SDL_EVENT_WINDOW_RESTORED` proactively.*
- *Window routing: `PlatformSDL3` maintains a `SDL_WindowID → Window*` map with registration during construction.*
- *Multi-window routing: The same windowID→Window map is future-proof for multi-window.*
- *Logging verbosity: `BUDDD_LOG_DEBUG` per event, `BUDDD_LOG_INFO` on final size.*

## Key entities

### `Window` (abstract base)
- Current: `width()`, `height()`, `native_handle()`, `set_mouse_capture()`, `is_mouse_captured()`
- **New**: `virtual auto on_resize(int w, int h) -> void = 0;` (pure virtual, following the existing pattern)

### `WindowSDL3` (inherits `Window`)
- Current: stores `width_`, `height_`, `window_` (SDL handle)
- **New**: `on_resize()` override updates `width_` and `height_`

### `WindowHeadless` (inherits `Window`)
- Current: stores `width_`, `height_`
- **New**: `on_resize()` override updates `width_` and `height_`

### `PlatformSDL3`
- Current: `poll_events()` loops over SDL events, routes quit to return, sends events to `input_system_` and `engine_imgui::on_sdl_event()`
- **New**: Add `SDL_WINDOW_RESIZABLE` flag to window creation, call `SDL_SetWindowMinimumSize()`, handle `SDL_EVENT_WINDOW_RESIZED`, `SDL_EVENT_WINDOW_MAXIMIZED`, and `SDL_EVENT_WINDOW_RESTORED` in event loop to call `window.on_resize(w, h)`.
- Maintains a `std::unordered_map<SDL_WindowID, Window*>` for routing events to the correct `Window` instance.
- `WindowSDL3` registers itself during construction (`PlatformSDL3::register_window()`) and **un-registers itself during destruction** (`PlatformSDL3::unregister_window()`) to prevent dangling pointers in the map.

### `EngineService`
- No changes needed — the `Window&` returned by `engine.window()` now reflects the current size at all times.
