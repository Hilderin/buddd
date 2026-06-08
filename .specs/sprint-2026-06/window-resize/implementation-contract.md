# IMPL-2026-06-001 — Window Resize for All Apps

## Source spec

`.specs/sprint-2026-06/window-resize/spec.md`

## Goal

Make all SDL3 windows resizable by dragging borders/corners. The `Window` base class gains a pure virtual `on_resize(w, h)` method that `WindowSDL3` and `WindowHeadless` override to update their cached `width_`/`height_`. `PlatformSDL3` sets `SDL_WINDOW_RESIZABLE`, enforces a 320×240 minimum via `SDL_SetWindowMinimumSize()`, and routes `SDL_EVENT_WINDOW_RESIZED`/`MAXIMIZED`/`RESTORED` events through a `SDL_WindowID → Window*` map to call `on_resize()`. Headless windows support programmatic resize for testability.

## Non-goals

- No `App::on_resize()` callback — apps poll `Window::width()/height()` each frame if needed.
- No `resizable` configuration flag — all windows unconditionally resizable.
- No changes to `RenderDeviceOpenGL::begin_frame()` or `RenderDeviceOpenGL::size()` (they already query `SDL_GetWindowSize()` directly).
- No HiDPI / DPI change handling.
- No programmatic `set_size()` API or fullscreen toggle.
- No changes to `WindowConfig`, `AppConfig`, `Platform` base class, `PlatformHeadless`, `EngineService`, `EngineContext`, `App`, or demo apps.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-012 (Navigable Object Graph Engine Service) | `EngineService` owns Platform→Window→RenderDevice chain; no changes needed to this ownership model. |
| ADR-020 (Custom Logging System) | All new logging uses `BUDDD_LOG_DEBUG`/`BUDDD_LOG_INFO`/`BUDDD_LOG_WARN` via `BUDDD_LOG_TAG`. |
| ADR-026 (ImGui Integration) | Resize events flow through existing `engine_imgui::on_sdl_event()` path — no new ImGui code. |

## Files to inspect

- `src/engine/window/window.h` — current `Window` base class (40 lines)
- `src/engine/window/window_sdl3.h` — `WindowSDL3` header (33 lines)
- `src/engine/window/window_sdl3.cpp` — current impl (38 lines)
- `src/engine/window/window_headless.h` — `WindowHeadless` header (29 lines)
- `src/engine/window/window_headless.cpp` — current impl (28 lines)
- `src/engine/platform/platform.h` — `Platform` base (48 lines)
- `src/engine/platform/platform_sdl3.h` — `PlatformSDL3` header (33 lines)
- `src/engine/platform/platform_sdl3.cpp` — current impl (74 lines)
- `src/engine/platform/platform_headless.h` — `PlatformHeadless` header (29 lines)
- `src/engine/platform/platform_headless.cpp` — current impl (32 lines)
- `src/engine/render/render_device_opengl.cpp` — `begin_frame()` and `size()` already use `SDL_GetWindowSize()` (lines 119-140)
- `src/engine/render/render_device_headless.cpp` — `size()` delegates to `window_.width()/height()` (line 141-143)
- `tests/CMakeLists.txt` — auto-discovers `*_tests.cpp` via `file(GLOB_RECURSE)`
- `tests/render_device_tests.cpp` — existing test pattern for headless tests

## Files allowed to change

- `src/engine/window/window.h`
- `src/engine/window/window_sdl3.h`
- `src/engine/window/window_sdl3.cpp`
- `src/engine/window/window_headless.h`
- `src/engine/window/window_headless.cpp`
- `src/engine/platform/platform_sdl3.h`
- `src/engine/platform/platform_sdl3.cpp`
- `tests/window_resize_tests.cpp` (new file)

## Files forbidden to change

- `src/engine/platform/platform.h`
- `src/engine/platform/platform_headless.h`
- `src/engine/platform/platform_headless.cpp`
- `src/engine/engine_service.h`
- `src/engine/engine_service.cpp`
- `src/engine/render/render_device_opengl.h`
- `src/engine/render/render_device_opengl.cpp`
- `src/engine/render/render_device_headless.h`
- `src/engine/render/render_device_headless.cpp`
- `src/engine/imgui/engine_imgui.h`
- `src/cmd/app.h`
- `src/cmd/app.cpp`
- `src/cmd/app_config.h`
- `src/cmd/app_config.cpp`
- `tests/CMakeLists.txt` (new `*_tests.cpp` files are auto-discovered)
- Any demo app or editor source files

## Existing conventions to follow

1. **Include style**: project-relative paths, e.g., `"window/window_sdl3.h"`, `"platform/platform_sdl3.h"`.
2. **Log tag**: `BUDDD_LOG_TAG("Platform:SDL3")` already in `platform_sdl3.cpp`; reuse it.
3. **Log macros**: `BUDDD_LOG_DEBUG`, `BUDDD_LOG_INFO`, `BUDDD_LOG_WARN` with `std::format`-style formatting (`{}` placeholders).
4. **Function signatures**: `auto function_name() -> ReturnType` trailing return type style.
5. **Result type**: `Result<T>` = `std::expected<T, Error>` with `make_error(...)`.
6. **Window creation pattern**: `PlatformSDL3::create_window()` calls `new WindowSDL3(...)` wrapped in `unique_ptr<Window>`.
7. **Test framework**: Catch2, `TEST_CASE("Name", "[tag1][tag2]")`, `REQUIRE(...)` macros.
8. **Test file naming**: `*_tests.cpp` with `_` separator.
9. **No RTTI**: no `dynamic_cast`; use `static_cast` only when type is guaranteed by architecture.
10. **Order of includes**: own header first, then same-directory headers, then library headers, then system headers.

## Required implementation behavior

### 1. `window.h` — Add `on_resize` pure virtual

Insert after line 25 (`virtual auto native_handle() -> void* = 0;`) and before `set_mouse_capture`:

```cpp
virtual auto on_resize(int w, int h) -> void = 0;
```

No other changes to this file.

### 2. `window_sdl3.h` — Declare `on_resize` override

Add after line 15 (`auto height() -> int override;`) and before `set_mouse_capture`:

```cpp
auto on_resize(int w, int h) -> void override;
```

No other changes.

### 3. `window_sdl3.cpp` — Implement `on_resize` and auto-unregister

**Add include** at top:
```cpp
#include "platform/platform_sdl3.h"
```

**Replace destructor** (lines 8-10):
```cpp
WindowSDL3::~WindowSDL3() {
    SDL_WindowID id = SDL_GetWindowID(window_);
    if (id != 0) {
        // Safe static_cast: WindowSDL3 is only ever created by PlatformSDL3::create_window()
        static_cast<PlatformSDL3&>(platform_).unregister_window(id);
    }
    SDL_DestroyWindow(window_);
}
```

**Add `on_resize` implementation** after `height()` (after line 18, before `native_handle`):
```cpp
auto WindowSDL3::on_resize(int w, int h) -> void {
    width_  = w;
    height_ = h;
}
```

### 4. `window_headless.h` — Declare `on_resize` override

Add after line 14 (`auto height() -> int override;`) and before `set_mouse_capture`:

```cpp
auto on_resize(int w, int h) -> void override;
```

### 5. `window_headless.cpp` — Implement `on_resize`

Add after `height()` (after line 14, before `native_handle`):
```cpp
auto WindowHeadless::on_resize(int w, int h) -> void {
    width_  = w;
    height_ = h;
}
```

No new includes needed.

### 6. `platform_sdl3.h` — Add window map and registration methods

**Add includes** before `#include <cstdint>` (line 6), maintaining convention order (library before system headers):
```cpp
#include <SDL3/SDL.h>
```
And after `#include <cstdint>` (line 6):
```cpp
#include <unordered_map>
```

**Add private members** before closing `private:` block (after `uint64_t last_frame_ticks_{0};` at line 30):
```cpp
std::unordered_map<SDL_WindowID, Window*> window_map_;
```

**Add public methods** after `auto input_system() -> InputSystem& override;` (after line 15, before `delta_time`):
```cpp
auto register_window(SDL_WindowID id, Window* window) -> void;
auto unregister_window(SDL_WindowID id) -> void;
```

### 7. `platform_sdl3.cpp` — Implement resize support

**Modify `create_window()`** (lines 53-72):

Replace `SDL_WINDOW_OPENGL` with `SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE`:
```cpp
    auto* sdl_window = SDL_CreateWindow(
        config.title.c_str(),
        config.width,
        config.height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
```

After `if (sdl_window == nullptr)` error handling block (after line 68) and before `BUDDD_LOG_INFO` (line 70), add:
```cpp
    if (!SDL_SetWindowMinimumSize(sdl_window, 320, 240)) {
        BUDDD_LOG_WARN("SDL_SetWindowMinimumSize failed: {}", SDL_GetError());
    }
```

Replace the `BUDDD_LOG_INFO` and `return` lines (lines 70-71) with:
```cpp
    auto win = std::unique_ptr<Window>(new WindowSDL3(sdl_window, config.width, config.height, *this));
    SDL_WindowID win_id = SDL_GetWindowID(sdl_window);
    register_window(win_id, win.get());
    BUDDD_LOG_INFO("Window created (resizable): {}x{} (windowID={})", config.width, config.height, +win_id);
    return win;
```

**Modify `poll_events()`** (lines 19-43):

After the existing `// Route non-quit events...` section (line 39) and before `return true` (line 42), insert the resize event handling block:

```cpp
        // Handle window resize / maximize / restore events
        if (event.type == SDL_EVENT_WINDOW_RESIZED
         || event.type == SDL_EVENT_WINDOW_MAXIMIZED
         || event.type == SDL_EVENT_WINDOW_RESTORED)
        {
            SDL_WindowID window_id = event.window.windowID;
            auto it = window_map_.find(window_id);
            if (it != window_map_.end()) {
                Window* win = it->second;
                if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                    int w = event.window.data1;
                    int h = event.window.data2;
                    BUDDD_LOG_DEBUG("Window resize: {}x{} (windowID={})", w, h, +window_id);
                    win->on_resize(w, h);
                } else {
                    // MAXIMIZED or RESTORED: query current size from SDL
                    SDL_Window* sdl_win = static_cast<SDL_Window*>(win->native_handle());
                    int w, h;
                    SDL_GetWindowSize(sdl_win, &w, &h);
                    BUDDD_LOG_DEBUG("Window resize (maximize/restore): {}x{} (windowID={})", w, h, +window_id);
                    win->on_resize(w, h);
                }
            }
        }
```

This block must be inserted **before** `input_system_.on_sdl_event(event)` and `engine_imgui::on_sdl_event(event)` so that the window cache updates before any downstream handler reads it. The edge case is that ImGui and input system receive the event **after** the window cache is updated. For `SDL_EVENT_WINDOW_MAXIMIZED` and `SDL_EVENT_WINDOW_RESTORED`, `event.window.data1`/`data2` may not carry the dimensions, so we query `SDL_GetWindowSize()` directly.

The event still passes through `input_system_.on_sdl_event(event)` and `engine_imgui::on_sdl_event(event)` afterward (the existing routing on lines 39-40 remains). This ensures ImGui's SDL3 backend receives the event and updates its internal display size.

**Add `register_window()` and `unregister_window()` implementations** (after `create_window()`, before the closing `}` of namespace):

```cpp
auto PlatformSDL3::register_window(SDL_WindowID id, Window* window) -> void {
    window_map_[id] = window;
}

auto PlatformSDL3::unregister_window(SDL_WindowID id) -> void {
    window_map_.erase(id);
}
```

### 8. `tests/window_resize_tests.cpp` — New file

Create with three test cases:

**Test 1** — Headless window `on_resize` updates dimensions:
```cpp
#include "window/window_headless.h"
#include "platform/platform_headless.h"
#include "engine_service.h"
#include "render/render_device.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("WindowHeadless::on_resize updates dimensions", "[window][headless][resize]") {
    auto platform = buddd::engine::Platform::create(buddd::engine::Backend::Headless);
    REQUIRE(platform.has_value());

    buddd::engine::WindowConfig cfg{"Test", 640, 480};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    REQUIRE(window.value()->width() == 640);
    REQUIRE(window.value()->height() == 480);

    window.value()->on_resize(800, 600);

    REQUIRE(window.value()->width() == 800);
    REQUIRE(window.value()->height() == 600);
}
```

**Test 2** — Headless window + render device: `device.size()` reflects `on_resize`:
```cpp
TEST_CASE("RenderDeviceHeadless::size reflects on_resize", "[window][headless][resize][device]") {
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{"Test", 640, 480});
    REQUIRE(engine.has_value());

    auto& eng = **engine;
    REQUIRE(eng.window().width() == 640);
    REQUIRE(eng.window().height() == 480);
    REQUIRE(eng.device().size() == std::pair{640, 480});

    eng.window().on_resize(800, 600);

    REQUIRE(eng.window().width() == 800);
    REQUIRE(eng.window().height() == 600);
    REQUIRE(eng.device().size() == std::pair{800, 600});
}
```

**Test 3** — Minimum size boundary:
```cpp
TEST_CASE("WindowHeadless::on_resize with boundary values", "[window][headless][resize][boundary]") {
    auto platform = buddd::engine::Platform::create(buddd::engine::Backend::Headless);
    REQUIRE(platform.has_value());

    buddd::engine::WindowConfig cfg{"Test", 320, 240};
    auto window = (*platform)->create_window(cfg);
    REQUIRE(window.has_value());

    // Headless accepts any values (no clamping by design)
    window.value()->on_resize(320, 240);
    REQUIRE(window.value()->width() == 320);
    REQUIRE(window.value()->height() == 240);
}
```

No changes to `tests/CMakeLists.txt` needed — new `*_tests.cpp` files are auto-discovered via `file(GLOB_RECURSE ...)`.

## Notable deviations from spec

- **Log message in `create_window()`**: The spec states `BUDDD_LOG_INFO("Window created: {}x{}", ...)` remains unchanged. The contract changes this to `BUDDD_LOG_INFO("Window created (resizable): {}x{} (windowID={})", ...)` for additional context (resizable flag and windowID). This is a deliberate improvement; no behavior depends on the log message format.
- **Registration responsibility**: The spec's Key Entities states "`WindowSDL3` registers itself during construction (`PlatformSDL3::register_window()`)", implying self-registration inside `WindowSDL3`'s constructor. The contract registers in `PlatformSDL3::create_window()` after construction. Behavior is equivalent (the map is populated before any event can arrive) and avoids circular include dependencies.

## Required tests

### Unit tests (in `tests/window_resize_tests.cpp`)

| Test | What it verifies | Traces to |
|---|---|---|
| `WindowHeadless::on_resize updates dimensions` | `WindowHeadless::on_resize(800, 600)` sets `width()==800`, `height()==600` | AC-003 |
| `RenderDeviceHeadless::size reflects on_resize` | After `window.on_resize(800, 600)`, `device.size() == {800, 600}` | AC-010 |
| `WindowHeadless::on_resize with boundary values` | `on_resize(320, 240)` works correctly (minimum boundary) | AC-008 (via headless) |

### E2E / Integration verification

- **Manual (SDL3)**: Run any demo app (e.g., `buddd-run-demo-spinning-cube`), drag window borders/corners, visually confirm viewport fills resized area, ImGui reflows, minimum 320×240 enforced, minimize/restore works.
- **Automated (CI)**: Full `ctest --output-on-failure` suite must pass with zero regressions.

## Edge cases

| # | Edge case | Required handling |
|---|---|---|
| EC-01 | **Resize below 320×240** | `SDL_SetWindowMinimumSize(320, 240)` in `create_window()`. OS clamps; `SDL_EVENT_WINDOW_RESIZED` fires with clamped size. |
| EC-02 | **Minimize then restore** | `SDL_EVENT_WINDOW_RESTORED` handler in `poll_events()` queries `SDL_GetWindowSize()` and calls `on_resize()`. |
| EC-03 | **Rapid resize** | Each `SDL_EVENT_WINDOW_RESIZED` processed in the loop; `on_resize` is O(1) integer assignment. No performance concern. |
| EC-04 | **Headless resize to zero/negative** | `WindowHeadless::on_resize()` stores values as-is (no clamping). No crash. |
| EC-05 | **Resize event before first poll** | SDL queues events; processed on first `poll_events()` call. Harmless. |
| EC-06 | **Window manager snap/tile** | Same as drag resize — `SDL_EVENT_WINDOW_RESIZED` fires, handler updates cache. |
| EC-07 | **Maximize** | `SDL_EVENT_WINDOW_MAXIMIZED` handler queries `SDL_GetWindowSize()` and calls `on_resize()`. |
| EC-08 | **Multiple windows (future)** | Each window has its own `width_`/`height_` cache. `window_map_` routes events by `SDL_WindowID`. |

## Security impact

None. Window resize is a standard OS capability. No new input handling, no new network/disk I/O, no new data exposure.

## Data and migration impact

None. No schema changes, no persistent data, no migrations.

## API compatibility impact

- `Window` base class adds a new pure virtual method `on_resize(int, int) -> void`. Any existing subclass that inherits `Window` directly (none exist beyond `WindowSDL3` and `WindowHeadless`) would fail to compile. This is acceptable — only two implementations exist in the codebase, both updated in this contract.
- `PlatformSDL3` gains two new public methods (`register_window`, `unregister_window`) and one new public member (the map is private). No existing callers use these; they are for internal window lifecycles only.
- No existing public API is removed or changed in signature.

## Documentation impact

- `docs/wiki/architecture/module-map.md`: Update `Window` entry to include `on_resize()` virtual method, `PlatformSDL3` entry to include window map and registration methods, and resize event handling in the event loop description. (Carried forward from spec "Documentation updates" section.)
- `docs/wiki/architecture/data-flow.md`: Add resize event flow: SDL resize event → `PlatformSDL3::poll_events()` → windowID map lookup → `Window::on_resize()` → cache update → ImGui reflow via existing `engine_imgui::on_sdl_event()` path.
- Other wiki files: Audit at implementation time for references to `Window` virtual methods or `PlatformSDL3` event loop.

## ADR impact

No new ADR needed. Existing ADRs (ADR-012, ADR-020, ADR-026) adequately cover the architecture. No ADR is deprecated.

## Done criteria

- [ ] `Window` base class in `window.h` declares `virtual auto on_resize(int w, int h) -> void = 0;` (line 25-26 area)
- [ ] `WindowSDL3` in `window_sdl3.h` declares `auto on_resize(int w, int h) -> void override;`
- [ ] `WindowSDL3::on_resize()` in `window_sdl3.cpp` assigns `width_ = w; height_ = h;`
- [ ] `WindowSDL3::~WindowSDL3()` in `window_sdl3.cpp` guards `unregister_window()` with `if (id != 0)` check and calls `unregister_window(SDL_GetWindowID(window_))` before `SDL_DestroyWindow`
- [ ] `window_sdl3.cpp` includes `"platform/platform_sdl3.h"`
- [ ] `WindowHeadless` in `window_headless.h` declares `auto on_resize(int w, int h) -> void override;`
- [ ] `WindowHeadless::on_resize()` in `window_headless.cpp` assigns `width_ = w; height_ = h;`
- [ ] `PlatformSDL3` in `platform_sdl3.h` declares `register_window(SDL_WindowID, Window*)` and `unregister_window(SDL_WindowID)` as public methods
- [ ] `PlatformSDL3` in `platform_sdl3.h` has private member `std::unordered_map<SDL_WindowID, Window*> window_map_`
- [ ] `platform_sdl3.h` includes `<unordered_map>`
- [ ] `PlatformSDL3::create_window()` in `platform_sdl3.cpp` passes `SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE`
- [ ] `PlatformSDL3::create_window()` calls `SDL_SetWindowMinimumSize(window, 320, 240)` and logs `BUDDD_LOG_WARN` on failure
- [ ] `PlatformSDL3::create_window()` calls `register_window(SDL_GetWindowID(sdl_window), win.get())`
- [ ] `PlatformSDL3::create_window()` logs `BUDDD_LOG_INFO("Window created (resizable): {}x{} (windowID={})", ...)`
- [ ] `PlatformSDL3::poll_events()` handles `SDL_EVENT_WINDOW_RESIZED` — looks up window in `window_map_`, calls `win->on_resize(data1, data2)`, logs `BUDDD_LOG_DEBUG("Window resize: {}x{} (windowID={})", ...)`
- [ ] `PlatformSDL3::poll_events()` handles `SDL_EVENT_WINDOW_MAXIMIZED` and `SDL_EVENT_WINDOW_RESTORED` — queries `SDL_GetWindowSize()`, calls `win->on_resize(w, h)`, logs `BUDDD_LOG_DEBUG(...)`
- [ ] Resize/maximize/restore events still pass through `input_system_.on_sdl_event(event)` and `engine_imgui::on_sdl_event(event)` (existing routing unchanged)
- [ ] `register_window()` and `unregister_window()` implementations exist
- [ ] `tests/window_resize_tests.cpp` exists with three test cases as specified
- [ ] Full test suite passes: `ctest --output-on-failure` (headless only)
- [ ] Manual verification steps documented in spec pass for any SDL3 demo app
