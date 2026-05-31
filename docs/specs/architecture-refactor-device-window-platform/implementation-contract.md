# IMPL-016 — Architecture Refactor: Navigable Object Graph (RenderDevice → Window → Platform → InputSystem)

## Source spec

`docs/specs/architecture-refactor-device-window-platform/spec.md` — SPEC-016.

## Goal

Implement the navigable object graph (`RenderDevice → Window → Platform → InputSystem`) by:
1. Adding `Platform&` (non-owning, protected) storage to the `Window` base class with `platform()` accessor.
2. Adding `set_mouse_capture(bool)` / `is_mouse_captured()` to the `Window` abstract interface, with SDL3 and headless backend implementations.
3. Adding `window() -> Window&` pure virtual to `RenderDevice`, with both backends implementing it.
4. Removing the `Platform&` parameter from all four demo function signatures — demos access `Platform` via `device.window().platform()`.
5. Creating a new `EngineService` class that owns the `Platform → Window → RenderDevice` chain via `unique_ptr`, usable in both tests and production.
6. Adding mouse-capture camera control to `free_camera_demo.cpp`: right-click to capture/release mouse, camera movement only while captured.
7. Updating all three affected test files (`render_device_tests.cpp`, `scene_rendering_tests.cpp`, `model_tests.cpp`) to use `EngineService` instead of direct `RenderDeviceHeadless` construction.
8. Removing `[cli][demo]` subprocess tests from `demo_tests.cpp`.

## Non-goals

- No changes to `InputSystem` interface or ownership.
- No changes to `Platform` interface (besides passing `*this` to `Window` constructors).
- No changes to `RenderSystem`, `MeshRenderer`, `Model`, or any scene-graph render types.
- No changes to `demo_helpers.h/.cpp` (they already accept only `RenderDevice&`).
- No changes to `CaptureCommand` or capture scenarios.
- No multi-window support (the graph supports it structurally, but only one active window remains).
- No behavioral changes in `triangle`, `cube`, or `cube-scene` demos.
- No error logging for `SDL_SetWindowRelativeMouseMode` failures.
- No backend selection changes.
- No changes to `demo_command.cpp`'s `platform` variable lifetime management (kept as-is).
- No changes to `tests/platform_abstraction_tests.cpp` (they already use `RenderDevice::create(*window)` which passes `Window&` — no signature change at the factory level).

## Relevant constitution rules

- **CONST-001** — Architecture Boundaries: No `#include` of SDL3/OpenGL headers outside `src/engine/`. The implementation must preserve this boundary; all SDL3 code stays within the SDL3 backend implementation files. The `Window` base class in `window.h` uses only a forward declaration for `Platform`, no SDL3 headers.
- **CONST-002** (principles): All references in the navigable graph are non-owning (`T&`). No ownership transfer occurs. Lifecycle invariants (`Platform` outlives `Window` outlives `RenderDevice`) must be maintained.

## Relevant ADRs

- **ADR-010** — Raw pointers prohibited in public API signatures. This contract is compliant: all cross-references use `T&` (guaranteed non-null references), not `T*`. The `Window base` stores `Platform& platform_` (non-owning reference), `RenderDeviceOpenGL`/`RenderDeviceHeadless` store `Window& window_`.
- **ADR-011** — (empty, no impact).
- **ADR-005** — (precedent for `std::optional<T&>`, not directly relevant here since the object graph uses guaranteed-non-null references).

## Files to inspect

The Code Agent MUST read these files before editing to understand current conventions:

- `src/engine/window/window.h` — Current `Window` base class (no `Platform&`, no mouse capture).
- `src/engine/window/window_sdl3.h` and `src/engine/window/window_sdl3.cpp` — SDL3 backend constructor and members.
- `src/engine/window/window_headless.h` and `src/engine/window/window_headless.cpp` — Headless backend constructor and members.
- `src/engine/render/render_device.h` — Current `RenderDevice` base class (no `window()` pure virtual).
- `src/engine/render/render_device_opengl.h` and `src/engine/render/render_device_opengl.cpp` — OpenGL backend constructor, `size()` impl, member layout.
- `src/engine/render/render_device_headless.h` and `src/engine/render/render_device_headless.cpp` — Headless backend constructor, `size()` impl, diagnostic methods.
- `src/engine/render/render_device.cpp` — Factory `RenderDevice::create(Window&)` — passes dimensions to `RenderDeviceHeadless`.
- `src/engine/platform/platform.h` — Interface that `create_window` belongs to.
- `src/engine/platform/platform_sdl3.cpp` — `create_window` currently constructs `WindowSDL3(sdl_window, w, h)`.
- `src/engine/platform/platform_headless.cpp` — `create_window` currently constructs `WindowHeadless(w, h)`.
- `src/cmd/demo/triangle_demo.h` and `.cpp` — Signature `(Platform&, RenderDevice&, ...)`.
- `src/cmd/demo/cube_demo.h` and `.cpp` — Same.
- `src/cmd/demo/cube_scene_demo.h` and `.cpp` — Same.
- `src/cmd/demo/free_camera_demo.h` and `.cpp` — Same; uses `platform.input_system()`, `platform.delta_time()`.
- `src/cmd/commands/demo_command.cpp` — Dispatch calls passing `**platform` and `**device`.
- `tests/render_device_tests.cpp` — Direct `RenderDeviceHeadless device(800, 600)` construction.
- `tests/scene_rendering_tests.cpp` — Multiple direct `RenderDeviceHeadless device(800, 600)` constructions, uses diagnostic methods.
- `tests/model_tests.cpp` — Has `create_headless_device()` helper, uses `device->begin_frame()` etc.
- `tests/demo_tests.cpp` — Three `[cli][demo]` subprocess tests that spawn `buddd demo <name>`.
- `tests/test_helpers.h` — Contains `buddd_binary_path()`, `temp_filename()`, `run_buddd()` helpers used by `demo_tests.cpp`.
- `tests/platform_abstraction_tests.cpp` — Already uses `RenderDevice::create(*window)`, no change needed.

## Files allowed to change

1. `src/engine/window/window.h` — Add `Platform` forward decl, `Platform& platform_`, `platform()`, `set_mouse_capture`, `is_mouse_captured`.
2. `src/engine/window/window_sdl3.h` — Constructor change, add `captured_`, override mouse capture methods.
3. `src/engine/window/window_sdl3.cpp` — Constructor body change, implement `set_mouse_capture`/`is_mouse_captured`.
4. `src/engine/window/window_headless.h` — Constructor change, override mouse capture methods.
5. `src/engine/window/window_headless.cpp` — Constructor body change, implement no-op mouse capture.
6. `src/engine/render/render_device.h` — Add `virtual auto window() -> Window& = 0` and virtual diagnostic accessors for testability.
7. `src/engine/render/render_device_opengl.h` — Constructor change, add `Window& window_` member, implement `window()`.
8. `src/engine/render/render_device_opengl.cpp` — Constructor body change (add `window_(window)` to init list).
9. `src/engine/render/render_device_headless.h` — Constructor change, replace `width_`/`height_` with `Window& window_`, implement `window()`.
10. `src/engine/render/render_device_headless.cpp` — Constructor body change, `size()` delegates to `window_`.
11. `src/engine/render/render_device.cpp` — Factory: pass `window` reference to backend constructors.
12. `src/engine/platform/platform_sdl3.cpp` — `create_window`: pass `*this` to `WindowSDL3` constructor.
13. `src/engine/platform/platform_headless.cpp` — `create_window`: pass `*this` to `WindowHeadless` constructor.
14. `src/engine/engine_service.h` — **NEW FILE**: `EngineService` class.
15. `src/engine/engine_service.cpp` — **NEW FILE**: `EngineService::create` implementation.
16. `src/cmd/demo/triangle_demo.h` — Remove `Platform&` param, remove `class Platform` forward decl.
17. `src/cmd/demo/triangle_demo.cpp` — Remove `Platform&` param, use `device.window().platform().poll_events()`, remove `#include "platform/platform.h"`.
18. `src/cmd/demo/cube_demo.h` — Same.
19. `src/cmd/demo/cube_demo.cpp` — Same.
20. `src/cmd/demo/cube_scene_demo.h` — Same.
21. `src/cmd/demo/cube_scene_demo.cpp` — Same.
22. `src/cmd/demo/free_camera_demo.h` — Same.
23. `src/cmd/demo/free_camera_demo.cpp` — Same, plus mouse capture behavior.
24. `src/cmd/commands/demo_command.cpp` — Dispatch calls: remove `**platform, ` from each call.
25. `tests/render_device_tests.cpp` — Replace `RenderDeviceHeadless device(800, 600)` with `EngineService::create(...)`.
26. `tests/scene_rendering_tests.cpp` — Replace `RenderDeviceHeadless device(800, 600)` with `EngineService::create(...)`. Add `#include "engine_service.h"`.
27. `tests/model_tests.cpp` — Replace `create_headless_device()` helper and all usages with `EngineService::create(...)`.
28. `tests/demo_tests.cpp` — Remove all three `[cli][demo]` subprocess test cases. The file may be kept with a comment or removed entirely; if kept, it must contain zero active test cases that spawn `buddd demo` as a subprocess.

## Files forbidden to change

- `tests/platform_abstraction_tests.cpp` — Already compatible; no changes needed.
- `tests/test_helpers.h` — `buddd_binary_path()`, `temp_filename()`, `run_buddd()` are not needed by new tests but do not need removal.
- `src/engine/platform/platform.h` — No interface changes.
- `src/engine/platform/platform_sdl3.h` — No interface changes.
- `src/engine/platform/platform_headless.h` — No interface changes.
- `src/engine/input/input_system.h` — No changes.
- `src/cmd/demo/demo_helpers.h` and `demo_helpers.cpp` — Already accept only `RenderDevice&`.
- `src/engine/render/render_system.h/.cpp` — No changes.
- `src/engine/render/mesh_renderer.h/.cpp` — No changes.
- `src/engine/render/model.h` — No changes.
- `src/engine/render/shader.h`, `material.h`, `vertex_buffer.h`, `index_buffer.h` — No changes.
- `src/engine/scene/` — No changes.
- `src/engine/math/` — No changes.
- `src/cmd/commands/capture_command.h/.cpp` — No changes (already accepts only `RenderDevice&`).
- `src/engine/error.h` — No changes.
- Any build system files (`CMakeLists.txt`, `meson.build`, etc.) — No changes unless a new source file must be added to the build (see Required implementation behavior).

## Existing conventions to follow

- **Pragma once**: All headers use `#pragma once`. New files must follow this.
- **Namespace**: All engine code is in `namespace buddd::engine`. Demo code is in `namespace buddd::cmd::demo`.
- **Style**:
  - `snake_case_` for class member variables (e.g., `platform_`, `window_`, `captured_`).
  - `snake_case` for function/method names (e.g., `platform()`, `set_mouse_capture()`, `create_window()`).
  - `[[nodiscard]]` on factory functions and `Result`-returning functions.
  - `noexcept` on trivial accessors, `const` where applicable.
  - `auto` return type with trailing return type syntax (e.g., `auto platform() noexcept -> Platform&`).
  - `virtual ... = 0` for pure virtuals (override with `override` in derived classes).
- **Deleted copy/move**: All engine base classes delete copy and move constructors/assignment operators. Concrete subclass headers repeat the pattern.
- **Protected default constructor**: `Window()` and `RenderDevice()` have `protected` default constructors.
- **Concrete subclass member ordering**: Private members listed at the bottom of the class.
- **Public methods**: Accessor methods declared before constructors or as inline in the header.
- **Include order**: The file's own header first, then project headers (alphabetically), then system/third-party headers.

## Required implementation behavior

### 1. `src/engine/window/window.h` — Changes

**ADD** (before `class Window`):
```cpp
class Platform;
struct WindowConfig;
```

**MODIFY** class `Window`:
- ADD protected constructor: `explicit Window(Platform& platform) : platform_(platform) {}`
- ADD protected member: `Platform& platform_`
- ADD inline accessor: `auto platform() noexcept -> Platform& { return platform_; }`
- ADD pure virtual: `virtual auto set_mouse_capture(bool captured) -> void = 0;`
- ADD pure virtual: `virtual auto is_mouse_captured() const noexcept -> bool = 0;`

**KEEP** everything else (`#pragma once`, `#include <memory>`, `#include <string>`, `WindowConfig` struct, `virtual ~Window()`, `width()`, `height()`, `native_handle()`, deleted copy/move).

### 2. `src/engine/window/window_sdl3.h` — Changes

**MODIFY** constructor signature:
```cpp
WindowSDL3(SDL_Window* window, int width, int height, Platform& platform);
```

**ADD** override declarations:
```cpp
auto set_mouse_capture(bool captured) -> void override;
auto is_mouse_captured() const noexcept -> bool override;
```

**ADD** private member:
```cpp
bool captured_{false};
```

**KEEP** SDL_Window* `window_`, `width_`, `height_`, all other inherited members and deleted copy/move.

### 3. `src/engine/window/window_sdl3.cpp` — Changes

**MODIFY** constructor:
```cpp
WindowSDL3::WindowSDL3(SDL_Window* window, int width, int height, Platform& platform)
    : Window(platform), window_(window), width_(width), height_(height) {}
```

**ADD** implementations:
```cpp
auto WindowSDL3::set_mouse_capture(bool captured) -> void {
    SDL_SetWindowRelativeMouseMode(window_, captured);
    captured_ = captured;
    // NOTE: cached `captured_` may desync from actual SDL relative mouse mode
    // on window focus loss (EC-012). SDL auto-releases relative mode on focus
    // loss, but `captured_` stays `true`. The demo recovers when user releases
    // and re-presses right-click. A future fix could listen for
    // SDL_EVENT_WINDOW_FOCUS_LOST to reset `captured_`.
}

auto WindowSDL3::is_mouse_captured() const noexcept -> bool {
    return captured_;
}
```

### 4. `src/engine/window/window_headless.h` — Changes

**MODIFY** constructor signature:
```cpp
WindowHeadless(int width, int height, Platform& platform);
```

**ADD** override declarations:
```cpp
auto set_mouse_capture(bool captured) -> void override;
auto is_mouse_captured() const noexcept -> bool override;
```

**KEEP** `width_` and `height_` private members.

### 5. `src/engine/window/window_headless.cpp` — Changes

**MODIFY** constructor:
```cpp
WindowHeadless::WindowHeadless(int width, int height, Platform& platform)
    : Window(platform), width_(width), height_(height) {}
```

**ADD** implementations:
```cpp
auto WindowHeadless::set_mouse_capture(bool /*captured*/) -> void {
    // no-op
}

auto WindowHeadless::is_mouse_captured() const noexcept -> bool {
    return false;
}
```

### 6. `src/engine/render/render_device.h` — Changes

**ADD** pure virtual:
```cpp
virtual auto window() noexcept -> Window& = 0;
```

**ADD** virtual diagnostic accessors (to enable `EngineService` migration without `dynamic_cast`):
```cpp
virtual auto frame_begin_count() const noexcept -> int { return 0; }
virtual auto frame_end_count() const noexcept -> int { return 0; }
virtual auto draw_call_count() const noexcept -> int { return 0; }
```

These return 0 by default and are overridden by `RenderDeviceHeadless`. This is a minimal backward-compatible addition for test support. No other virtual methods are added.

**KEEP** all existing methods, `#pragma once`, forward declarations, deleted copy/move, protected default constructor.

### 7. `src/engine/render/render_device_opengl.h` — Changes

**MODIFY** constructor signature:
```cpp
RenderDeviceOpenGL(Window& window, SDL_Window* sdl_window, SDL_GLContext context);
```

**ADD** private member: `Window& window_;`

**ADD** override:
```cpp
auto window() noexcept -> Window& override { return window_; }
```

**KEEP** `SDL_Window* window_` (used for internal SDL calls such as `SDL_GetWindowSize`, `SDL_GL_SwapWindow`, `SDL_GL_DestroyContext`), `SDL_GLContext context_`, all other members, overrides, and deleted copy/move.

### 8. `src/engine/render/render_device_opengl.cpp` — Changes

**MODIFY** constructor:
```cpp
RenderDeviceOpenGL::RenderDeviceOpenGL(Window& window, SDL_Window* sdl_window, SDL_GLContext context)
    : window_(window), window_(sdl_window), context_(context)
```

Wait — there's a naming collision: both the new `Window& window_` and the existing `SDL_Window* window_` would be named `window_`. To resolve this, rename the existing SDL_Window* member to `sdl_window_` in both the header and the .cpp file.

**In `render_device_opengl.h`**: rename `SDL_Window* window_;` → `SDL_Window* sdl_window_;`

**In `render_device_opengl.cpp`**: Replace all uses of `window_` (the SDL_Window* pointer) with `sdl_window_` in:
- Constructor: `window_(sdl_window)` init list
- `~RenderDeviceOpenGL()`: `SDL_GL_DestroyContext(context_)` (context_ only, no change needed)
- `begin_frame()`: `SDL_GetWindowSize(sdl_window_, &w, &h)`
- `end_frame()`: `SDL_GL_SwapWindow(sdl_window_)`
- `size()`: `SDL_GetWindowSize(sdl_window_, &w, &h)`

Constructor body after change:
```cpp
RenderDeviceOpenGL::RenderDeviceOpenGL(Window& window, SDL_Window* sdl_window, SDL_GLContext context)
    : window_(window), sdl_window_(sdl_window), context_(context)
{
    // ... existing body unchanged ...
}
```

### 9. `src/engine/render/render_device_headless.h` — Changes

**MODIFY** constructor signature:
```cpp
explicit RenderDeviceHeadless(Window& window);
```

**REMOVE** `int width_;` and `int height_;` private members.

**ADD** private member: `Window& window_;`

**ADD** override:
```cpp
auto window() noexcept -> Window& override { return window_; }
```

**MODIFY** `size()` declaration — it remains as an override but the implementation changes (see below).

**KEEP** all diagnostic members (`shader_count_`, `material_count_`, `vertex_buffer_count_`, `index_buffer_count_`, `draw_call_count_`, `frame_begin_count_`, `frame_end_count_`), and the `frame_begin_count()`, `frame_end_count()`, `draw_call_count()` accessors (which now also override the base class virtuals).

### 10. `src/engine/render/render_device_headless.cpp` — Changes

**MODIFY** constructor:
```cpp
RenderDeviceHeadless::RenderDeviceHeadless(Window& window)
    : window_(window) {}
```

**MODIFY** `size()`:
```cpp
auto RenderDeviceHeadless::size() const noexcept -> std::pair<int, int> {
    return {window_.width(), window_.height()};
}
```

### 11. `src/engine/render/render_device.cpp` — Changes

**MODIFY** the Headless branch:
```cpp
return std::unique_ptr<RenderDevice>(
    new RenderDeviceHeadless(window));
```

**MODIFY** the OpenGL branch:
```cpp
return std::unique_ptr<RenderDevice>(
    new RenderDeviceOpenGL(window, sdl_window, gl_context));
```

### 12. `src/engine/platform/platform_sdl3.cpp` — Changes

**MODIFY** `create_window`:
```cpp
return std::unique_ptr<Window>(new WindowSDL3(sdl_window, config.width, config.height, *this));
```

### 13. `src/engine/platform/platform_headless.cpp` — Changes

**MODIFY** `create_window`:
```cpp
return std::unique_ptr<Window>(new WindowHeadless(config.width, config.height, *this));
```

### 14. `src/engine/engine_service.h` — NEW FILE

```cpp
#pragma once

#include "error.h"

#include <memory>

namespace buddd::engine {

class Platform;
class Window;
class RenderDevice;
class InputSystem;
struct WindowConfig;
enum class Backend;

class EngineService {
public:
    [[nodiscard]] static auto create(Backend backend, const WindowConfig& config)
        -> Result<std::unique_ptr<EngineService>>;

    auto platform() noexcept -> Platform&;
    auto window() noexcept -> Window&;
    auto device() noexcept -> RenderDevice&;

    EngineService(const EngineService&) = delete;
    auto operator=(const EngineService&) -> EngineService& = delete;
    EngineService(EngineService&&) = default;
    auto operator=(EngineService&&) -> EngineService& = default;

private:
    EngineService(std::unique_ptr<Platform> platform,
                  std::unique_ptr<Window> window,
                  std::unique_ptr<RenderDevice> device);

    // Member declaration order MUST be: platform_, window_, device_
    // This ensures Platform outlives Window outlives RenderDevice on destruction.
    std::unique_ptr<Platform> platform_;
    std::unique_ptr<Window> window_;
    std::unique_ptr<RenderDevice> device_;
};

} // namespace buddd::engine
```

### 15. `src/engine/engine_service.cpp` — NEW FILE

```cpp
#include "engine_service.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

namespace buddd::engine {

EngineService::EngineService(std::unique_ptr<Platform> platform,
                             std::unique_ptr<Window> window,
                             std::unique_ptr<RenderDevice> device)
    : platform_(std::move(platform))
    , window_(std::move(window))
    , device_(std::move(device)) {}

auto EngineService::create(Backend backend, const WindowConfig& config)
    -> Result<std::unique_ptr<EngineService>>
{
    auto platform = Platform::create(backend);
    if (!platform) {
        return std::unexpected(platform.error());
    }

    auto window = (*platform)->create_window(config);
    if (!window) {
        return std::unexpected(window.error());
    }

    auto device = RenderDevice::create(*window.value());
    if (!device) {
        return std::unexpected(device.error());
    }

    return std::unique_ptr<EngineService>(
        new EngineService(std::move(*platform), std::move(*window), std::move(*device)));
}

auto EngineService::platform() noexcept -> Platform& {
    return *platform_;
}

auto EngineService::window() noexcept -> Window& {
    return *window_;
}

auto EngineService::device() noexcept -> RenderDevice& {
    return *device_;
}

} // namespace buddd::engine
```

### 16–23. Demo headers and .cpp files — Changes

#### Each demo header (`triangle_demo.h`, `cube_demo.h`, `cube_scene_demo.h`, `free_camera_demo.h`):

**REMOVE** from the forward-declaration block:
```cpp
class Platform;
```

**MODIFY** function signature from:
```cpp
auto run_xxxx_demo(buddd::engine::Platform& platform,
                   buddd::engine::RenderDevice& device,
                   int argc, const char* const* argv) -> int;
```
to:
```cpp
auto run_xxxx_demo(buddd::engine::RenderDevice& device,
                   int argc, const char* const* argv) -> int;
```

**UPDATE** doc comment: replace `@param platform  ...` with explanation that `Platform` is accessed via `device.window().platform()`.

#### Each demo .cpp file (`triangle_demo.cpp`, `cube_demo.cpp`, `cube_scene_demo.cpp`, `free_camera_demo.cpp`):

**MODIFY** function signature: remove `be::Platform& platform, ` parameter.

**MODIFY** function body:
- Replace `platform.poll_events()` → `device.window().platform().poll_events()`
- Replace `platform.input_system()` → `device.window().platform().input_system()` (free_camera_demo only)
- Replace `platform.delta_time()` → `device.window().platform().delta_time()` (free_camera_demo only)

**REMOVE** `#include "platform/platform.h"`.

**KEEP** `#include "input/input_system.h"` in free_camera_demo.cpp — the code directly references `be::KeyCode::MouseRight`, `be::KeyCode::Escape`, `be::KeyCode::W`, `be::KeyCode::S`, `be::KeyCode::D`, `be::KeyCode::A`, `be::KeyCode::Space`, `be::KeyCode::ControlLeft`, `be::KeyCode::ControlRight`. `KeyCode` is defined in `input/key_code.h` which is included via `input/input_system.h`. The include must stay.

### Free camera demo specific behavior

**File**: `src/cmd/demo/free_camera_demo.cpp`

After removing `Platform& platform` from the signature, apply these additional changes to implement mouse capture:

**ADD** after camera state variables (near line 58):
```cpp
bool prev_right_click_{false};
```

**MODIFY** the event of `auto& input = ...`:
```cpp
auto& input = device.window().platform().input_system();
```

**MODIFY** the frame loop body (after `poll_events` and `input` usage) to add right-click edge detection:

```cpp
// ── Mouse capture (right-click) ──
bool curr_right_click = input.is_down(buddd::engine::KeyCode::MouseRight);
if (curr_right_click && !prev_right_click_) {
    // Right-click pressed — capture mouse
    device.window().set_mouse_capture(true);
    std::cerr << "Mouse captured (right-click)\n";
}
if (!curr_right_click && prev_right_click_) {
    // Right-click released — release mouse
    device.window().set_mouse_capture(false);
    std::cerr << "Mouse released (right-click)\n";
}
prev_right_click_ = curr_right_click;

bool mouse_captured = device.window().is_mouse_captured();
```

**WRAP** mouse-look code in the guard:
```cpp
// ── Mouse look (only while captured) ──
if (mouse_captured) {
    auto [dx, dy] = input.mouse_delta();
    yaw -= dx * k_mouse_sensitivity;
    pitch += -dy * k_mouse_sensitivity;
    pitch = std::clamp(pitch, be::math::radians(-k_pitch_clamp),
                                be::math::radians(k_pitch_clamp));
    cam.set_orientation(be::math::Quat::from_euler(pitch, yaw, 0.0f));
}
```

**WRAP** keyboard movement code in the guard:
```cpp
// ── Keyboard movement (only while captured) ──
if (mouse_captured) {
    be::math::Vec3 forward = cam.orientation() * be::math::Vec3{0.0f, 0.0f, -1.0f};
    forward.y = 0.0f;
    if (forward.length_squared() > be::math::epsilon) {
        forward.normalize();
    }

    be::math::Vec3 right = cam.orientation() * be::math::Vec3{1.0f, 0.0f, 0.0f};
    be::math::Vec3 movement{0.0f, 0.0f, 0.0f};

    if (input.is_down(be::KeyCode::W))          { movement += forward; }
    if (input.is_down(be::KeyCode::S))          { movement -= forward; }
    if (input.is_down(be::KeyCode::D))          { movement += right; }
    if (input.is_down(be::KeyCode::A))          { movement -= right; }
    if (input.is_down(be::KeyCode::Space))      { movement += be::math::Vec3::unit_y(); }
    if (input.is_down(be::KeyCode::ControlLeft))  { movement -= be::math::Vec3::unit_y(); }
    if (input.is_down(be::KeyCode::ControlRight)) { movement -= be::math::Vec3::unit_y(); }

    cam.set_position(cam.position() + movement * k_move_speed * dt);
}
```

### 24. `src/cmd/commands/demo_command.cpp` — Changes

**MODIFY** each dispatch call: remove `**platform, ` from the argument list.

```cpp
if (demo_name == "triangle") {
    return buddd::cmd::demo::run_triangle_demo(**device, argc - 2, argv + 2);
} else if (demo_name == "cube-scene") {
    return buddd::cmd::demo::run_cube_scene_demo(**device, argc - 2, argv + 2);
} else if (demo_name == "free-camera") {
    return buddd::cmd::demo::run_free_camera_demo(**device, argc - 2, argv + 2);
} else {
    // demo_name == "cube" (validated above)
    return buddd::cmd::demo::run_cube_demo(**device, argc - 2, argv + 2);
}
```

**KEEP** all `#include` directives, the `platform` local variable (for Platform lifetime management), all other code unchanged.

### 25. `tests/render_device_tests.cpp` — Changes

Replace:
```cpp
#include "render/render_device_headless.h"
...
buddd::engine::RenderDeviceHeadless device(800, 600);
```
with:
```cpp
#include "engine_service.h"
...
auto engine = buddd::engine::EngineService::create(
    buddd::engine::Backend::Headless,
    buddd::engine::WindowConfig{.title = "Test", .width = 800, .height = 600});
REQUIRE(engine.has_value());
auto& device = engine.value()->device();
```

### 26. `tests/scene_rendering_tests.cpp` — Changes

Add `#include "engine_service.h"`.

For each test case that constructs `RenderDeviceHeadless device(800, 600);`, replace with the `EngineService::create` pattern and use `engine->device()`.

For tests that use `device.frame_begin_count()`, `device.frame_end_count()`, `device.draw_call_count()`:
- Since `engine->device()` returns `RenderDevice&` (base class) and the virtual diagnostics were added to `RenderDevice`, these calls will work via virtual dispatch to `RenderDeviceHeadless` overrides.
- Replace `device.frame_begin_count()` with `static_cast<const buddd::engine::RenderDeviceHeadless&>(*engine->device()).frame_begin_count()` OR use the base class virtual accessors if they were added (see `render_device.h` change above).

Recommended approach (since virtual accessors are added to `RenderDevice`):
- `engine->device().frame_begin_count()` — this works directly through virtual dispatch.

**KEEP** all test logic and assertions unchanged.

### 27. `tests/model_tests.cpp` — Changes

Replace the `create_headless_device()` helper:
```cpp
// OLD
auto create_headless_device() -> std::unique_ptr<be::RenderDevice> {
    return std::make_unique<be::RenderDeviceHeadless>(800, 600);
}
```
with a helper that creates an `EngineService` and returns a reference to the device (or just inline the EngineService creation in each test).

Recommended approach: Inline the EngineService creation. Replace each instance of:
```cpp
auto device = create_headless_device();
```
with:
```cpp
auto engine = be::EngineService::create(
    be::Backend::Headless,
    be::WindowConfig{.title = "Test", .width = 800, .height = 600});
REQUIRE(engine.has_value());
auto& device = engine.value()->device();  // engine stays alive for this scope
```

And where `device` is accessed via `device->method()`, change to `device.method()` since `device` is now a `RenderDevice&` not a `unique_ptr`.

**ADD** `#include "engine_service.h"` at the top.

### 28. `tests/demo_tests.cpp` — Changes

**REMOVE** all three `TEST_CASE("buddd demo triangle runs and completes", "[cli][demo]")` test cases.

**EITHER**:
- Option A: Replace the file content with:
```cpp
// No [cli][demo] subprocess tests — these were removed as part of SPEC-016
// (architecture refactor). Demo correctness is verified via compilation
// of demo functions and the EngineService creation tests.
```
- Option B: Keep the file empty with just the header comment.

Prefer Option A (keep a valid file with an explanatory comment). Remove leftover includes for `test_helpers.h`, `catch2`, `<cstdlib>`, `<fstream>`, `<string>` if no longer needed.

### Build system changes

Add `src/engine/engine_service.cpp` to the build system. Locate where `src/engine/render/render_device.cpp` is added and add `engine_service.cpp` in the same build target.

### Important design constraint — `RenderDeviceOpenGL` member rename

The existing `SDL_Window* window_` member in `RenderDeviceOpenGL` MUST be renamed to `sdl_window_` to avoid naming collision with the new `Window& window_` member. This rename applies to:
- `src/engine/render/render_device_opengl.h` — Change declaration
- `src/engine/render/render_device_opengl.cpp` — Change all references

The `SDL_GLContext context_` member name stays unchanged.

## Required tests

No new test files are created. Existing tests are updated as specified in "Files allowed to change" (items 25–28).

The existing test suite covers:

| Test file | Covers ACs |
|---|---|---|
| `tests/render_device_tests.cpp` (updated + new tests) | AC-013, AC-015 (via EngineService), plus AC-037 to AC-043 (EngineService tests) |
| `tests/scene_rendering_tests.cpp` (updated) | AC-013, AC-015, AC-030 (via EngineService) |
| `tests/model_tests.cpp` (updated) | AC-030 (via EngineService) |
| Compilation of all 4 demo headers | AC-018, AC-019, AC-020, AC-021, AC-023 |
| Visual inspection | AC-001, AC-002, AC-003, AC-011, AC-025, AC-026, AC-027, AC-028, AC-029, AC-031 |

The following ACs require new tests that the implementer MUST add:

All new tests go into `tests/render_device_tests.cpp` (the EngineService test file). SDL3-conditional tests use `#ifdef BUDDD_HAS_DISPLAY` guards.

| AC | What to test |
|---|---|
| AC-004 | Create `WindowSDL3` (via `PlatformSDL3::create_window`), call `window->platform()`, verify reference matches. Conditional (`#ifdef BUDDD_HAS_DISPLAY`). |
| AC-005, AC-006, AC-007 | `set_mouse_capture(true/false)`, verify `is_mouse_captured()` returns matching value. Conditional. |
| AC-008 | Create `WindowHeadless` via `PlatformHeadless::create_window`, call `window->platform()`, verify address matches. |
| AC-009, AC-010 | Call `set_mouse_capture(true)` on headless, verify `is_mouse_captured()` returns `false`. |
| AC-012 | Create `EngineService(Headless)`, call `engine->device().window()`, verify reference is valid. |
| AC-013, AC-014, AC-015 | Via `EngineService::create(Headless, ...)`, verify `engine->device().window()` returns valid reference; verify `size()` matches config. |
| AC-037 | `EngineService::create(Headless, valid_config)` succeeds and returns non-null. |
| AC-038 | `EngineService::create(Headless, {.width=-1, .height=600})` returns error. |
| AC-039 | `engine->platform()` returns valid reference (call `delta_time()` or `poll_events()` on it). |
| AC-040 | `engine->window()` returns valid reference; `width()`/`height()` match config. |
| AC-041 | `engine->device()` returns valid reference; `size()` matches window config. |
| AC-042 | `&engine->device().window().platform() == &engine->platform()` (address comparison). |
| AC-043 | `engine->device().window().platform().input_system()` compiles and returns a reference (call a method on it like `input_system().begin_frame()`). |
| AC-044 | `tests/demo_tests.cpp` — verify no `[cli][demo]` subprocess tests remain. Inspect file, confirm subprocess-spawning code is removed. |

## Edge cases

All edge cases from SPEC-016 must be addressed:

| EC | How the implementation handles it |
|---|---|
| EC-001 (Window destroyed before RenderDevice) | Undefined behavior — existing lifecycle rule. Non-owning references assume `Window` outlives `RenderDevice`. No runtime guard. |
| EC-002 (Platform destroyed before Window) | Undefined behavior — existing lifecycle rule. `EngineService` member declaration order prevents this in normal usage. |
| EC-003 (set_mouse_capture on headless) | `WindowHeadless::set_mouse_capture` is a no-op; `is_mouse_captured()` returns `false`. |
| EC-004 (redundant capture on SDL3) | `SDL_SetWindowRelativeMouseMode(window_, true)` called again — SDL3 handles gracefully. `captured_` stays `true`. |
| EC-005 (redundant release on SDL3) | Same — SDL3 handles gracefully. `captured_` stays `false`. |
| EC-006 (window() with destroyed Window) | Undefined behavior — existing lifecycle rule. No runtime guard. |
| EC-007 (platform() with destroyed Platform) | Undefined behavior — existing lifecycle rule. No runtime guard. |
| EC-008 (Headless device from zero/negative window) | `WindowHeadless` stores whatever dimensions are passed. `size()` delegates to `window_.width()/height()`. No validation at this level. |
| EC-009 (multiple Windows from same Platform) | Each `Window` stores its own `Platform&` pointing to the creating `Platform`. Correct even with multiple windows. |
| EC-010 (SDL3 error in set_mouse_capture) | Current implementation ignores SDL3 return value (`void`). No crash. |
| EC-011 (rapid right-click) | Each press/release pair calls `set_mouse_capture(true/false)` sequentially. Edge detection (`prev_right_click_`) prevents double-trigger. |
| EC-012 (focus loss while captured) | SDL3 auto-releases relative mouse mode. `captured_` may be stale until next user interaction. Demo recovers when user releases and re-presses right-click. Add a code comment in `WindowSDL3::set_mouse_capture` noting this desync possibility. |

## Security impact

- No authentication, authorization, or access control concerns.
- `set_mouse_capture` calls standard SDL3 API (`SDL_SetWindowRelativeMouseMode`) that operates within the application window — no system-level privilege required.
- No new data is persisted or transmitted.
- All cross-references are `T&` (non-null, non-owning). No raw pointers introduced (compliant with ADR-010).

## Data and migration impact

None.

## API compatibility impact

Backward-incompatible changes:
1. `WindowSDL3` constructor: `(SDL_Window*, int, int)` → `(SDL_Window*, int, int, Platform&)` — third-party code calling this constructor directly must update. Only `PlatformSDL3::create_window` calls it, which is updated.
2. `WindowHeadless` constructor: `(int, int)` → `(int, int, Platform&)` — only `PlatformHeadless::create_window` calls it, which is updated.
3. `RenderDeviceOpenGL` constructor: `(SDL_Window*, SDL_GLContext)` → `(Window&, SDL_Window*, SDL_GLContext)` — only `RenderDevice::create` calls it, which is updated.
4. `RenderDeviceHeadless` constructor: `(int, int)` → `(Window&)` — tests that construct it directly must update to use `EngineService`.
5. All four demo function signatures: removed `Platform&` parameter — any external caller (only `demo_command.cpp`) must update.
6. `RenderDevice` base class adds `window()` pure virtual — all existing subclasses must implement it (both `RenderDeviceOpenGL` and `RenderDeviceHeadless` are updated). No third-party subclasses exist.
7. `RenderDevice` base class adds virtual diagnostic accessors (`frame_begin_count()`, etc.) with default implementations — backward compatible.

## Documentation impact

- `docs/specs/architecture-refactor-device-window-platform/coordination.md` — Must be updated (see after-writing instructions).

## ADR impact

- **ADR-010** is reinforced: this contract uses `T&` (non-owning references) throughout the object graph, consistent with the "no raw pointers" rule.
- No new ADR needed. The refactoring is constrained by existing ADRs.

## Constitution impact

None. The constitution is not affected by this implementation.

## Done criteria

- [ ] **C-001**: `window.h` contains `class Platform;` forward declaration, protected `Platform& platform_` member, `platform()` accessor, and `set_mouse_capture`/`is_mouse_captured` pure virtuals. *(Inspect `window.h`)*
- [ ] **C-002**: `WindowSDL3` constructor accepts `(SDL_Window*, int, int, Platform&)`, passes `Platform&` to `Window` base, implements `set_mouse_capture` via `SDL_SetWindowRelativeMouseMode`, caches `captured_` bool. *(Inspect `window_sdl3.h` and `window_sdl3.cpp`)*
- [ ] **C-003**: `WindowHeadless` constructor accepts `(int, int, Platform&)`, passes `Platform&` to `Window` base, `set_mouse_capture` is no-op, `is_mouse_captured()` returns `false`. *(Inspect `window_headless.h` and `window_headless.cpp`)*
- [ ] **C-004**: `RenderDevice` base declares `virtual auto window() noexcept -> Window& = 0` and virtual diagnostic accessors `frame_begin_count()`, `frame_end_count()`, `draw_call_count()`. *(Inspect `render_device.h`)*
- [ ] **C-005**: `RenderDeviceOpenGL` stores `Window& window_` and `SDL_Window* sdl_window_` (renamed from `window_`), constructor accepts `(Window&, SDL_Window*, SDL_GLContext)`, implements `window()`. *(Inspect `render_device_opengl.h` and `.cpp`)*
- [ ] **C-006**: `RenderDeviceHeadless` stores `Window& window_` (replaces `width_`/`height_`), constructor accepts `(Window&)`, `size()` delegates to `window_.width()/height()`. *(Inspect `render_device_headless.h` and `.cpp`)*
- [ ] **C-007**: `RenderDevice::create` passes `Window&` to both backend constructors: `RenderDeviceHeadless(window)` and `RenderDeviceOpenGL(window, sdl_window, gl_context)`. *(Inspect `render_device.cpp`)*
- [ ] **C-008**: `PlatformSDL3::create_window` passes `*this` to `WindowSDL3` constructor. *(Inspect `platform_sdl3.cpp`)*
- [ ] **C-009**: `PlatformHeadless::create_window` passes `*this` to `WindowHeadless` constructor. *(Inspect `platform_headless.cpp`)*
- [ ] **C-010**: `EngineService` exists at `src/engine/engine_service.h` and `.cpp` with `create()` factory, `platform()`, `window()`, `device()` accessors, and correct member declaration order (`platform_`, `window_`, `device_`). *(Inspect file content)*
- [ ] **C-011**: All four demo headers remove `Platform&` parameter and forward declaration of `class Platform`. *(Inspect each header)*
- [ ] **C-012**: All four demo `.cpp` files use `device.window().platform().poll_events()`, no `#include "platform/platform.h"`. Free camera demo additionally uses `device.window().platform().input_system()` and `device.window().platform().delta_time()`. *(Inspect each .cpp)*
- [ ] **C-013**: Free camera demo has right-click edge detection (`prev_right_click_`), calls `device.window().set_mouse_capture(true/false)` on press/release, guards mouse-look and keyboard movement behind `is_mouse_captured()` check. *(Inspect `free_camera_demo.cpp`)*
- [ ] **C-014**: `demo_command.cpp` dispatch calls pass only `(**device, argc-2, argv+2)` — no `**platform` argument. *(Inspect `demo_command.cpp`)*
- [ ] **C-015**: `tests/render_device_tests.cpp` uses `EngineService::create(Headless, ...)` instead of `RenderDeviceHeadless(800, 600)`. *(Inspect file)*
- [ ] **C-016**: `tests/scene_rendering_tests.cpp` uses `EngineService::create(Headless, ...)` instead of `RenderDeviceHeadless(800, 600)` and accesses diagnostic methods via virtual dispatch or `dynamic_cast`. *(Inspect file)*
- [ ] **C-017**: `tests/model_tests.cpp` replaces `create_headless_device()` with `EngineService::create(Headless, ...)` pattern. *(Inspect file)*
- [ ] **C-018**: `tests/demo_tests.cpp` has no `[cli][demo]` subprocess tests that spawn `buddd demo`. *(Inspect file)*
- [ ] **C-019**: All tests compile and pass: run `ctest --preset debug` — zero failures.
- [ ] **C-020**: `src/engine/engine_service.cpp` is added to the build system (same CMake build target as `render_device.cpp`). *(Inspect CMakeLists.txt or equivalent)*
