# IMPL-008 — CLI App System: Centralised Render Loop with Scene Dispatch

## Source spec

`docs/specs/cli-app-system/spec.md` (SPEC-008), accepted. All blocking issues from the spec-critic review (`docs/specs/cli-app-system/spec-critic.md`) have been resolved.

## Goal

Replace the three separate render-loop implementations (`RunCommand`, `DemoCommand`, `CaptureCommand`) with a single `App` base class lifecycle (`setup()` → render loop → `shutdown()`) and a free `run_app()` function that owns the central render loop. Create 7 `App` subclasses absorbing all existing demo and capture scene logic. Unify the CLI into `buddd run [<scene>] [--frame N] [--capture N:path]...` — removing `demo` and `capture` subcommands entirely. Add `RenderSystem::render_scene()` to allow `read_pixels` injection between `begin_frame()`/`end_frame()`.

## Non-goals

- No changes to the engine library (`src/engine/`) beyond the `RenderSystem::render_scene()` addition.
- No changes to `RenderSystem` rendering logic — only the begin/end framing is extracted into a public `render_scene()` method.
- No changes to demo scene content (models, materials, lights, animation parameters, camera positions).
- No changes to existing tests (they use `EngineService`, not the CLI).
- No rewriting of `demo_helpers` — their `std::exit` error handling stays as-is.
- No new scenes beyond the existing set (triangle, cube, cube-scene, textured-cube, free-camera, phong, empty window).
- No CLI framework or third-party argument parsing library.
- No dynamic app discovery, plugin system, or reflection.
- No tab-completion or shell integration.

## Relevant constitution rules

- **CONST-001** (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): No SDL3, OpenGL, or GLM headers outside `src/engine/`. All access to platform/graphics from `src/cmd/` must go through engine abstractions (`Platform`, `Window`, `RenderDevice`). Must be verified by `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` — zero matches.
- **CONST-002** (`docs/constitution/rules/CONST-002-testing-policy.md`): All testable code must have corresponding tests. App subclasses' rendering correctness is verified manually (as acknowledged in the spec), but `run_app()` parsing logic, `RenderSystem::render_scene()`, and the new `main.cpp` dispatch must be tested.

## Relevant ADRs

- **ADR-003** (`docs/adr/003-render-pipeline-architecture.md`): Draw methods return `void`; `Platform::poll_events()` was added to avoid CONST-001 exception; the render loop is an application-level concern.
- **ADR-004** (`docs/adr/004-demo-system-architecture.md`): Establishes the demo dispatch pattern (if/else-if chain), per-demo files in `src/cmd/demo/`, `demo_helpers` namespace. This implementation supersedes the old demo/capture architecture but preserves the `demo_helpers` and the if/else-if chain pattern.
- **ADR-001** (`docs/adr/001-result-error-pattern.md`): `Result<T>` pattern for fallible APIs. `app.setup()` returns `Result<void>` per this pattern.

## Files to inspect

Before editing, the Code Agent must read these files to understand existing code:

| File | What to look for |
|---|---|
| `src/cmd/main.cpp` | Current dispatch chain (run/demo/version/help/capture). Must be rewritten. |
| `src/cmd/CMakeLists.txt` | Current glob pattern — must be updated to remove `capture/*.cpp` and add `apps/*.cpp`. |
| `src/cmd/commands/run_command.cpp` | The empty-window render loop to absorb into `RunApp`. |
| `src/cmd/commands/demo_command.cpp` | Demo dispatch pattern, backend selection, window creation, extra-arg warning. |
| `src/cmd/commands/capture_command.cpp` | Capture dispatch, flag parsing (`--frame N`), `read_pixels` + `Image::save`. |
| `src/cmd/commands/help_command.h` | `k_usage_text` — must be updated. |
| `src/cmd/demo/triangle_demo.cpp` | Triangle render loop to absorb into `TriangleApp`. |
| `src/cmd/demo/cube_demo.cpp` | Cube render loop to absorb into `CubeApp`. |
| `src/cmd/demo/cube_scene_demo.cpp` | Cube scene render loop to absorb into `CubeSceneApp`. |
| `src/cmd/demo/textured_cube_demo.cpp` | Textured cube to absorb into `TexturedCubeApp`. |
| `src/cmd/demo/free_camera_demo.cpp` | Free camera to absorb into `FreeCameraApp`. |
| `src/cmd/demo/phong_demo.cpp` | Phong demo to absorb into `PhongApp`. |
| `src/cmd/capture/cube_capture.cpp` | Cube capture — camera at (0,0,3) when captures active, driver quirk (min 2 frames). |
| `src/cmd/capture/phong_capture.cpp` | Phong capture — the manual RenderSystem reimplementation (~200 lines) to be deleted. |
| `src/cmd/demo/demo_helpers.h` / `.cpp` | `setup_triangle()` and `setup_cube()` — kept unchanged, used by App subclasses. |
| `src/engine/render/render_system.h` | Current `render()` declaration — must add `render_scene()`. |
| `src/engine/render/render_system.cpp` | Current `render()` body — must extract lines 23–155 (everything between begin_frame/end_frame) into `render_scene()`. |
| `src/engine/image/image.h` | `Image::create()` and `Image::save()` signatures — used by `run_app()` for captures. |
| `src/engine/render/render_device.h` | `read_pixels()`, `begin_frame()`, `end_frame()` signatures. |
| `src/engine/platform/platform.h` | `Platform::create()`, `poll_events()` signatures. |
| `src/engine/window/window.h` | `WindowConfig` struct, `Window::width()`, `Window::height()`. |

## Files allowed to change

### Files to create (18 new files)

| # | File | Class/Content |
|---|---|---|
| 1 | `src/cmd/app_config.h` | `CaptureSpec`, `RunningArgs`, `parse_running_args()` declaration |
| 2 | `src/cmd/app_config.cpp` | `parse_running_args()` implementation |
| 3 | `src/cmd/app.h` | `App` base class, `AppConfig`, `run_app()` declaration (includes `app_config.h`) |
| 4 | `src/cmd/app.cpp` | `run_app()` implementation |
| 5 | `src/cmd/apps/run_app.h` | `RunApp` class declaration |
| 6 | `src/cmd/apps/run_app.cpp` | `RunApp` implementation |
| 7 | `src/cmd/apps/triangle_app.h` | `TriangleApp` class declaration |
| 8 | `src/cmd/apps/triangle_app.cpp` | `TriangleApp` implementation |
| 9 | `src/cmd/apps/cube_app.h` | `CubeApp` class declaration |
| 10 | `src/cmd/apps/cube_app.cpp` | `CubeApp` implementation |
| 11 | `src/cmd/apps/cube_scene_app.h` | `CubeSceneApp` class declaration |
| 12 | `src/cmd/apps/cube_scene_app.cpp` | `CubeSceneApp` implementation |
| 13 | `src/cmd/apps/textured_cube_app.h` | `TexturedCubeApp` class declaration |
| 14 | `src/cmd/apps/textured_cube_app.cpp` | `TexturedCubeApp` implementation |
| 15 | `src/cmd/apps/free_camera_app.h` | `FreeCameraApp` class declaration |
| 16 | `src/cmd/apps/free_camera_app.cpp` | `FreeCameraApp` implementation |
| 17 | `src/cmd/apps/phong_app.h` | `PhongApp` class declaration |
| 18 | `src/cmd/apps/phong_app.cpp` | `PhongApp` implementation |

### Files to modify (4 files)

| # | File | Change |
|---|---|---|
| 1 | `src/cmd/main.cpp` | Rewrite dispatch — remove old command classes, replace with `run_app()` + `App` subclasses + `VersionCommand`/`HelpCommand` dispatch |
| 2 | `src/cmd/commands/help_command.h` | Update `k_usage_text` — remove `demo` and `capture` from command list, update `run` description |
| 3 | `src/cmd/CMakeLists.txt` | Remove `capture/*.cpp` from glob, add `apps/*.cpp` |
| 4 | `src/engine/render/render_system.h` | Add `render_scene()` public member declaration |
| 5 | `src/engine/render/render_system.cpp` | Extract body of `render()` (between begin_frame/end_frame, lines 23–155) into `render_scene()`. `render()` becomes `begin_frame()` → `render_scene()` → `end_frame()`. |

### Files to delete (19 files)

| # | File | Reason |
|---|---|---|
| 1 | `src/cmd/commands/run_command.h` | Replaced by `RunApp` + `run_app()` |
| 2 | `src/cmd/commands/run_command.cpp` | Replaced by `RunApp` + `run_app()` |
| 3 | `src/cmd/commands/demo_command.h` | Removed (demo subcommand removed) |
| 4 | `src/cmd/commands/demo_command.cpp` | Removed (demo subcommand removed) |
| 5 | `src/cmd/commands/capture_command.h` | Removed (capture subcommand removed) |
| 6 | `src/cmd/commands/capture_command.cpp` | Removed (capture subcommand removed) |
| 7 | `src/cmd/capture/cube_capture.h` | Absorbed into `CubeApp` |
| 8 | `src/cmd/capture/cube_capture.cpp` | Absorbed into `CubeApp` |
| 9 | `src/cmd/capture/phong_capture.h` | Absorbed into `PhongApp` |
| 10 | `src/cmd/capture/phong_capture.cpp` | Absorbed into `PhongApp` (entire 520+ line file removed) |
| 11 | `src/cmd/demo/cube_demo.h` | Absorbed into `CubeApp` |
| 12 | `src/cmd/demo/cube_demo.cpp` | Absorbed into `CubeApp` |
| 13 | `src/cmd/demo/cube_scene_demo.h` | Absorbed into `CubeSceneApp` |
| 14 | `src/cmd/demo/cube_scene_demo.cpp` | Absorbed into `CubeSceneApp` |
| 15 | `src/cmd/demo/free_camera_demo.h` | Absorbed into `FreeCameraApp` |
| 16 | `src/cmd/demo/free_camera_demo.cpp` | Absorbed into `FreeCameraApp` |
| 17 | `src/cmd/demo/phong_demo.h` | Absorbed into `PhongApp` |
| 18 | `src/cmd/demo/phong_demo.cpp` | Absorbed into `PhongApp` |
| 19 | `src/cmd/demo/textured_cube_demo.h` | Absorbed into `TexturedCubeApp` |
| 20 | `src/cmd/demo/textured_cube_demo.cpp` | Absorbed into `TexturedCubeApp` |
| 21 | `src/cmd/demo/triangle_demo.h` | Absorbed into `TriangleApp` |
| 22 | `src/cmd/demo/triangle_demo.cpp` | Absorbed into `TriangleApp` |

Total: 18 create + 5 modify + 22 delete + 0 kept unchanged (demo_helpers and version/help commands remain).

### Files to keep unchanged

- `src/cmd/demo/demo_helpers.h` / `demo_helpers.cpp` — `setup_triangle()` and `setup_cube()` still used by App subclasses.
- `src/cmd/commands/version_command.h` / `version_command.cpp` — unchanged.
- `src/cmd/commands/help_command.cpp` — unchanged (reads from `k_usage_text` constant).
- `src/capture/` directory is removed entirely (not just files).

## Files forbidden to change

- Any file under `src/engine/` except `render_system.h` and `render_system.cpp`.
- `src/engine/error.h`, `src/engine/image/`, `src/engine/math/`, `src/engine/platform/`, `src/engine/window/`, `src/engine/scene/`, `src/engine/input/`, `src/engine/render/` files other than `render_system.h` and `render_system.cpp`.
- `src/cmd/commands/version_command.h`, `src/cmd/commands/version_command.cpp`.
- `src/cmd/commands/help_command.cpp`.
- `src/cmd/demo/demo_helpers.h`, `src/cmd/demo/demo_helpers.cpp`.
- Any `.cpp` or `.h` in `src/cmd/apps/` other than the 14 files listed in "Files to create".
- Any file under `tests/`.
- Root `CMakeLists.txt`, `CMakePresets.json`, `.clang-format`, `AGENTS.md`, `opencode.json`.
- Any file under `docs/` except those explicitly listed in "Files to modify".
- `src/engine/render/render_device.h` — no changes needed (already has `read_pixels()`).
- `src/engine/render/render_device_opengl.h` / `.cpp` — no changes needed.
- `src/engine/render/render_device_headless.h` / `.cpp` — no changes needed.

## Existing conventions to follow

1. **File naming**: `snake_case` for all new files (`app.h`, `triangle_app.h`, etc.).
2. **Namespace naming**: `buddd::cmd` for App, `buddd::cmd::app` for App subclasses (namespace `app` under `cmd`). Use namespace aliases `namespace be = buddd::engine;` and `namespace bc = buddd::cmd;` in `.cpp` files.
3. **Namespace closing comments**: `} // namespace buddd::cmd` after closing brace of namespace.
4. **Include guard**: `#pragma once` in all headers.
5. **Forward declarations**: Prefer forward-declaring engine types in headers to minimize includes.
6. **Error handling**: Use `be::to_string(error)` for engine error formatting. Use `std::fprintf(stderr, ...)` and `std::cerr` for error output consistently with existing code. Fatal errors (shader/material/vb/creation failure) call `std::exit(EXIT_FAILURE)` as in `demo_helpers`.
7. **Include paths**: Relative to `src/engine/` for engine headers (e.g., `#include "platform/platform.h"`). Relative to `src/cmd/` for cmd headers (e.g., `#include "commands/help_command.h"`, `#include "demo/demo_helpers.h"`).
8. **CMake style**: Follow the existing pattern in `src/cmd/CMakeLists.txt` for `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)`.
9. **Backend selection**: Use compile-time `BUDDD_HAS_DISPLAY` define to select SDL3 vs Headless backend (see existing `run_command.cpp` pattern).
10. **Window title**: Set by each App subclass via `config()` returning an `AppConfig` with the desired title. Used for both the titlebar display and observability messages.
11. **`[[nodiscard]]`**: `run_app()`, `app.setup()`, and all `Result<T>`-returning functions are `[[nodiscard]]`.
12. **Include order**: 1. Standard library headers (`<>`), 2. Engine headers (`""`), 3. Cmd headers (`""`). Empty line between groups.
13. **Trailing return type**: `auto foo() -> int` style for all new functions.
14. **No `sleep_for`**: The render loop does NOT use `std::this_thread::sleep_for`. Frame timing relies on VSync (`end_frame` swap ↔ display refresh).

## Required implementation behavior

### 1. `src/cmd/app_config.h` — CaptureSpec, RunningArgs, parse_running_args()

```cpp
#pragma once

#include "error.h"
#include <string>
#include <vector>

namespace buddd::cmd {

struct CaptureSpec {
    int frame;          // 1-based
    std::string path;
};

struct RunningArgs {
    int frame_limit = 0;
    std::vector<CaptureSpec> captures;
};

/// Parse --frame N and --capture N:path from argv starting at index `start`.
/// Returns RunningArgs on success, or an error on invalid input.
[[nodiscard]] auto parse_running_args(int argc, char* argv[], int start)
    -> engine::Result<RunningArgs>;

} // namespace buddd::cmd
```

**Requirements:**
- `RunningArgs` default-constructs with `frame_limit=0`, empty `captures`.
- `CaptureSpec::frame` is 1-based (matches the `--capture N:path` syntax).
- `parse_running_args()` is self-contained with no dependency on the `App` class.

### 1b. `src/cmd/app.h` — App interface, AppConfig, run_app()

```cpp
#pragma once

#include "app_config.h"
#include "error.h"

#include <string>

namespace buddd::engine { class RenderDevice; }

namespace buddd::cmd {

struct AppConfig {
    std::string title = "Buddd Engine";
    int width = 1024;
    int height = 768;
};

/// Base class for all renderable applications (scenes, empty window, etc.).
/// Lifecycle: setup() → render() [called per frame] → shutdown().
class App {
public:
    virtual ~App() = default;

    /// Window configuration for this App (title, dimensions).
    [[nodiscard]] virtual auto config() const -> AppConfig = 0;

    /// Called once before the render loop.
    /// Return error to abort the application (loop is skipped, shutdown() is
    /// still called).
    [[nodiscard]] virtual auto setup(buddd::engine::RenderDevice& device)
        -> buddd::engine::Result<void> = 0;

    /// Called once per frame, between begin_frame() and end_frame().
    /// The `frame` parameter is 0-based (0 = first frame rendered).
    virtual auto render(buddd::engine::RenderDevice& device, int frame) -> void = 0;

    /// Called once after the render loop ends (whether normally or due to error).
    /// Default implementation does nothing.
    virtual auto shutdown() -> void {}

protected:
    /// Set to false to stop the render loop early (for interactive scenes).
    bool running_ = true;
};

/// Central render loop.
/// @param app  The App to run.
/// @param args CLI-parsed running arguments (frame_limit, captures).
[[nodiscard]] auto run_app(App& app, const RunningArgs& args) -> int;

} // namespace buddd::cmd
```

**Requirements:**
- `App` uses `AppConfig`, `CaptureSpec`, and `RunningArgs` from `app_config.h`.
- `App::running_` is `protected` so subclasses can set it to `false`.
- `App::config()` is a pure virtual method returning `AppConfig` by value. Each subclass defines its own window metadata (title, width, height).
- There is no `config_` member or `friend` declaration — `config()` is subclass-defined.
- `run_app()` receives pre-parsed `RunningArgs` from the caller (typically `main.cpp`).

### 2. `src/cmd/app.cpp` — run_app()

**Includes:**
```cpp
#include "app.h"
#include "app_config.h"

#include "image/image.h"
#include "image/image_buffer.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>
```

**Backend selection** (compile-time, matching existing pattern):
```cpp
constexpr auto k_app_backend = [] {
#ifdef BUDDD_HAS_DISPLAY
    return be::Backend::SDL3;
#else
    return be::Backend::Headless;
#endif
}();
```

**`parse_running_args()`** is declared in `app_config.h` and implemented in `app_config.cpp`. It parses `--frame N` and `--capture N:path` from `argv`. See section 1 for the declaration. Behavior:

Input: `int argc, char* argv[], int start`.
Returns: `Result<RunningArgs>` on success. On error, returns an error result (caller handles printing).

Behavior:
- Scan `argv[start..argc-1]` for flags.
- `--frame N`: if `++i >= argc`, return error `"Error: --frame requires a number"`. Parse N with `std::strtol`. If non-numeric or `N < 1`, return error `"Error: --frame requires a positive integer, got '%s'"`. Set `args.frame_limit = N`.
- `--capture N:path`: parse the argument value. Find the first `:` character. If no `:` found, return error `"Error: --capture requires a frame number (format: N:path)"`. The portion before `:` is the frame number string. Parse with `std::strtol`. If non-numeric or `N < 1`, return error `"Error: --capture requires a positive integer frame number"`. The portion after `:` is the path. Add `CaptureSpec{N, path}` to `args.captures`.
- Unknown flags are silently ignored (forward-compat).
- Return the populated `RunningArgs`.

**`run_app()` — central render loop**:

```
1. Get AppConfig from app.config():
   auto cfg = app.config();

2. Determine backend via compile-time BUDDD_HAS_DISPLAY (same as existing code).

3. Create Platform:
   auto platform = be::Platform::create(k_app_backend);
   If error -> print to stderr via be::to_string(), return EXIT_FAILURE.

4. Create Window:
   auto window = (*platform)->create_window({
       .title = cfg.title,
       .width = cfg.width,
       .height = cfg.height
   });
   If error -> print to stderr via be::to_string(), return EXIT_FAILURE.

5. Print window dimensions to stdout via std::printf:
   "Window opened: <cfg.width>x<cfg.height>"

6. Create RenderDevice:
   auto device = be::RenderDevice::create(**window);
   If error -> print to stderr via be::to_string(), return EXIT_FAILURE.

7. Call app.setup(*device):
   If error -> print error to stderr, call app.shutdown(), return EXIT_FAILURE.

8. Print scene start message to stderr:
   If args.frame_limit > 0:
     "Scene started: <cfg.title> (<args.frame_limit> frames)"
   Else:
     "Scene started: <cfg.title> (interactive)"

9. Render loop:
    bool any_capture_success = false;
    bool any_capture_failure = false;
    int frame = 0;
    while (true) {
    // Frame limit check
    if (args.frame_limit > 0 && frame >= args.frame_limit)
            break;

        // Event polling
        if (!(*platform)->poll_events()) {
            // Window closed
            break;
        }

        // App requested stop
        if (!app.running_)
            break;

        // Begin frame
        (*device)->begin_frame();

        // Render
        app.render(*device, frame);

        // Capture: read_pixels BEFORE end_frame()
        // Only read_pixels once per frame, even if multiple captures match.
        bool did_read_pixels = false;
        be::Result<be::ImageBuffer> pixel_buffer = ...; // uninitialized
        for (const auto& spec : args.captures) {
            // The OpenGL driver quirk means frame 1 cannot be reliably captured
            // (returns clear color). The workaround captures frame 2 instead when
            // frame 1 is requested. This is done by computing an effective_frame
            // before the match condition.
            int effective_frame = (spec.frame < 2) ? 2 : spec.frame;
            if (effective_frame == frame + 1) {
                    if (!did_read_pixels) {
                        pixel_buffer = (*device)->read_pixels();
                        did_read_pixels = true;
                    }
                    if (pixel_buffer) {
                        auto image = be::Image::create(*pixel_buffer);
                        if (image) {
                            auto save_result = image->save(spec.path);
                            if (save_result) {
                                any_capture_success = true;
                                std::printf("Captured: %s\n", spec.path.c_str());
                            } else {
                                any_capture_failure = true;
                                std::cerr << be::to_string(save_result.error()) << "\n";
                            }
                        } else {
                            any_capture_failure = true;
                            std::cerr << be::to_string(image.error()) << "\n";
                        }
                    } else {
                        any_capture_failure = true;
                        std::cerr << be::to_string(pixel_buffer.error()) << "\n";
                    }
            }
        }

        // End frame
        (*device)->end_frame();

        ++frame;
    }

10. Print scene end message:
    If app.running_ == false (ESC pressed):
      "Scene aborted by user (frame N)" to stderr (frame is 1-based)
    Else if platform.poll_events() returned false (window closed):
      "Scene aborted by user" to stderr
    Else (normal completion):
      "Scene complete: <cfg.title> (<frame_count> frames rendered)" to stderr

11. Call app.shutdown().

12. Print "Window closed, shutting down." to stdout.

13. Return appropriate exit code:
    if (!args.captures.empty() && !any_capture_success && any_capture_failure)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
```

**Requirements for the render loop:**
- Frame counter `frame` is 0-based internally. `--capture N` uses 1-based frame numbers. The comparison is `effective_frame == frame + 1` where `effective_frame` is derived from `spec.frame`. Uses `args.captures` and `args.frame_limit` from the `RunningArgs` parameter.
- Driver quirk: The OpenGL driver quirk means frame 1 cannot be reliably captured (returns clear color). The workaround captures frame 2 instead when frame 1 is requested. This is done by computing `effective_frame = (spec.frame < 2) ? 2 : spec.frame` before the match condition. No warning printed — silent per design decision.
- Multiple captures on the same frame: `read_pixels()` is called once; each capture spec is saved to its own path.
- No `sleep_for` anywhere in the loop.
- `poll_events()` is called once per frame, before `begin_frame()`.
- If `setup()` returns an error, the render loop is skipped entirely, but `shutdown()` is called.
- Platform/Window/Device creation errors short-circuit without calling `setup()` or `shutdown()`.

### 3. `src/cmd/apps/` — App subclass implementations

All App subclasses follow this pattern:
- Header: `#pragma once`, forward-declare engine types, declare class in `buddd::cmd::app` namespace (or directly in `buddd::cmd`).
- Implementation: `.cpp` file with includes and the class methods.

**Namespace convention for App subclasses**: `buddd::cmd::app` sub-namespace (e.g., `namespace buddd::cmd::app { class TriangleApp : public App { ... }; }`).

#### 3a. RunApp (`run_app.h` / `run_app.cpp`)

```
class RunApp : public App {
public:
    auto config() const -> AppConfig override {
        return {};  // defaults: "Buddd Engine", 1024x768
    }
    auto setup(be::RenderDevice&) -> be::Result<void> override { return {}; }
    auto render(be::RenderDevice&, int) -> void override {}
};
```

**Requirements:**
- `setup()`: no-op, return success.
- `render()`: no-op. The empty framebuffer is cleared by `begin_frame()`.
- No `running_` override (always runs until window close).
- Window title uses default AppConfig values: `"Buddd Engine"`, 1024×768 (returned by `config()`).

#### 3b. TriangleApp (`triangle_app.h` / `triangle_app.cpp`)

```
class TriangleApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine — triangle", 1024, 768};
    }
    auto setup(be::RenderDevice& device) -> be::Result<void> override;
    auto render(be::RenderDevice& device, int frame) -> void override;
private:
    std::unique_ptr<be::Material> material_;
    std::unique_ptr<be::VertexBuffer> vb_;
};
```

**Requirements:**
- `setup()`: calls `demo::setup_triangle(device)` from `demo/demo_helpers.h`, stores the material and vertex buffer in member variables. Returns success.
- `render()`: calls `device.draw(be::PrimitiveTopology::Triangles, *vb_, *material_, 3)`. No `begin_frame()`/`end_frame()` — those are handled by `run_app()`.
- No `running_` override (frame_limit from `--frame` or default 120 stops it).
- Aspect ratio: uses 1024/768 (the new window size) — but `setup_triangle` doesn't use aspect ratio (it's just vertex positions), so this is a no-op change.
- No `sleep_for`, no frame timing — `run_app()` handles the loop.
- `config()` returns `{"Buddd Engine — triangle", 1024, 768}`.

#### 3c. CubeApp (`cube_app.h` / `cube_app.cpp`)

```
class CubeApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine — cube", 1024, 768};
    }
    auto setup(be::RenderDevice& device) -> be::Result<void> override;
    auto render(be::RenderDevice& device, int frame) -> void override;
private:
    be::math::Camera camera_;
    demo::CubeResources cube_;
    std::chrono::steady_clock::time_point start_time_;
};
```

**Requirements:**
- `setup()`: calls `demo::setup_cube(device)`, stores the cube resources. Creates a camera:
  - Camera at `(3, 2, 3)` looking at origin (interactive-like position). Capture-specific camera adjustment is handled by `run_app()`.
  - Perspective 60° FOV, aspect 1024/768 (using `static_cast<float>(cfg.width)/cfg.height`), near 0.1, far 100.
  - Records `start_time_` for animation.
  - Returns success.
- `render()`: computes elapsed time from `start_time_`, computes rotation angle = `elapsed_seconds * 0.5f` (0.5 rad/s around Y), computes model matrix = `Mat4::rotate(angle, Vec3::unit_y())`, computes MVP = `projection * view * model`, sets `u_mvp` on the cube's material, calls `cube.model.draw(device)`.
- No `running_` override.
- `config()` returns `{"Buddd Engine — cube", 1024, 768}`.

#### 3d. CubeSceneApp (`cube_scene_app.h` / `cube_scene_app.cpp`)

```
class CubeSceneApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine — cube-scene", 1024, 768};
    }
    auto setup(be::RenderDevice& device) -> be::Result<void> override;
    auto render(be::RenderDevice&, int frame) -> void override;
private:
    be::World world_;
    std::unique_ptr<be::RenderSystem> render_system_;
    be::Entity entity_;
    std::chrono::steady_clock::time_point start_time_;
};
```

**Requirements:**
- `setup()`: creates World, creates Entity with CameraComponent (camera at (3,2,3) looking at origin, 60° FOV, aspect 1024/768, near 0.1 far 100), creates MeshRenderer with cube from `setup_cube()`, creates RenderSystem(device, world). Returns success.
- `render()`: computes elapsed time, rotates entity via `entity_.transform().rotation = Quat::angle_axis(angle, Vec3::unit_y())`, calls `render_system_->render_scene()`.
- Uses `render_scene()` (not `render()`), because `run_app()` handles begin_frame/end_frame.
- `config()` returns `{"Buddd Engine — cube-scene", 1024, 768}`.

#### 3e. TexturedCubeApp (`textured_cube_app.h` / `textured_cube_app.cpp`)

Same structure as CubeSceneApp but with textured material. `config()` returns `{"Buddd Engine — textured-cube", 1024, 768}`.

**Requirements:**
- `setup()`: loads `"assets/brick.png"` via `Image::load()`, creates Texture via `device.create_texture()`, creates World with CameraComponent, creates vertex/index buffers with texture coordinates (same data as current `textured_cube_demo.cpp`), creates Material with vertex/fragment shaders (embedded GLSL strings from current demo), sets texture on material, creates Model via `Model::create_indexed()`, attaches MeshRenderer, creates RenderSystem. Returns success on all steps; on any failure, prints to stderr and returns the error.
- `render()`: rotates entity, calls `render_system_->render_scene()`.

#### 3f. FreeCameraApp (`free_camera_app.h` / `free_camera_app.cpp`)

```
class FreeCameraApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine — free-camera", 1024, 768};
    }
    auto setup(be::RenderDevice& device) -> be::Result<void> override;
    auto render(be::RenderDevice& device, int frame) -> void override;
    // ... members
};
```

**Requirements:**
- `setup()`: creates World, camera entity (position (0,2,5), look at origin, 60° FOV, aspect 1024/768), cube from `setup_cube()` with MeshRenderer, RenderSystem(device, world). Stores input references for frame-by-frame use. Returns success.
- `render()`: handles input (WASD movement, mouse look, right-click to capture/release mouse, ESC to exit). Updates camera position and orientation. Calls `render_system_->render_scene()`. If ESC is pressed, sets `running_ = false`.
- Must NOT create its own `poll_events`/`begin_frame`/`end_frame`/`sleep_for` loop — those are in `run_app()`.
- Mouse capture toggle: on right-click down, call `device.window().set_mouse_capture(true)`. On right-click up, `set_mouse_capture(false)`.
- Movement: WASD for horizontal (relative to camera forward, Y=0), Space/Ctrl for up/down. Speed: 5.0 units/s, using `device.window().platform().delta_time()` for frame-rate-independent movement.
- `config()` returns `{"Buddd Engine — free-camera", 1024, 768}`.

#### 3g. PhongApp (`phong_app.h` / `phong_app.cpp`)

**IMPORTANT**: This replaces BOTH `phong_demo.cpp` AND `phong_capture.cpp`. The phong_capture manual reimplementation of `RenderSystem::render()` (~200 lines) must be deleted entirely. `PhongApp` uses `RenderSystem::render_scene()` and `read_pixels()` is handled by `run_app()`.

```
class PhongApp : public App {
public:
    auto config() const -> AppConfig override {
        return {"Buddd Engine — phong", 1024, 768};
    }
    auto setup(be::RenderDevice& device) -> be::Result<void> override;
    auto render(be::RenderDevice& device, int frame) -> void override;
    // ... members
};
```

**Requirements:**
- `setup()`: creates World with 5 cubes (same CubeSpec as current phong_demo/phong_capture), creates textures (checkerboard + solid white via procedural helpers), creates PhongMaterial per cube with appropriate material properties (specular, shininess, diffuse_tint, texture), creates models, attaches MeshRenderers. Creates 5 lights: 1 directional fill, 3 point lights (A orbitting warm orange, B orbitting cool blue out of phase, C static purple), 1 spot light (warm from above). Creates RenderSystem(device, world). Stores entity references for light animation and camera entity for input. Uses fixed camera for capture scenarios and interactive camera otherwise. Returns success.
- `render()`:
  - If interactive: handles WASD/mouse/ESC input (same as current phong_demo).
  - Animates orbiting lights using elapsed time (same formulas as current code).
  - Calls `render_system_->render_scene()` — NOT a manual reimplementation.
  - If ESC pressed: `running_ = false`.
- The `render()` method must NOT contain any of the light collection, MeshRenderer iteration, or uniform setting logic that was duplicated in `phong_capture.cpp`. That is all handled by `RenderSystem::render_scene()`.

### 4. `src/engine/render/render_system.h` — Add render_scene()

Add public method after the existing `render()` declaration:

```cpp
/// Renders one frame's worth of the scene, WITHOUT begin_frame()/end_frame().
/// The caller is responsible for framing. Same rendering logic as render(),
/// but does not call begin_frame() or end_frame().
/// Behaviour is undefined if called outside a begin_frame()/end_frame() pair.
auto render_scene() -> void;
```

**Requirements:**
- Public member function, no arguments, returns `void`.
- Same preconditions as `render()` (not re-entrant, not from within a `World::each()` callback).

### 5. `src/engine/render/render_system.cpp` — Extract render_scene()

**Current `render()`** (lines 22–156):
```
auto RenderSystem::render() -> void {
    device_->begin_frame();          // line 23
    ...
    auto vp = ...;                   // middle logic
    ...
    device_->end_frame();            // line 155
}
```

**New structure:**

```cpp
auto RenderSystem::render_scene() -> void {
    // Lines 25–153: everything between begin_frame and end_frame
    // (camera check, light collection, MeshRenderer iteration, uniform setting, draw calls)
    // Do NOT include begin_frame() or end_frame().
}

auto RenderSystem::render() -> void {
    device_->begin_frame();
    render_scene();
    device_->end_frame();
}
```

**Requirements:**
- The extracted `render_scene()` contains lines 25–153 from the current `render()`. This includes:
  - Camera lookup logic (lines 25–33)
  - Light collection (lines 35–98)
  - Debug light count print (lines 100–104)
  - MeshRenderer iteration, MVP computation, uniform setting, and draw calls (lines 106–153)
- Does NOT include `begin_frame()` (line 23) or `end_frame()` (line 155).
- The early-return path when no camera is active (lines 26–30) must be handled: in `render_scene()`, if no active camera, just return early (no `end_frame()` call since we didn't begin one).
- `render()` calls `begin_frame()` → `render_scene()` → `end_frame()`.
- No other changes to the rendering logic.

### 6. `src/cmd/main.cpp` — New dispatch

```cpp
#include "app.h"
#include "app_config.h"
#include "apps/cube_app.h"
#include "apps/cube_scene_app.h"
#include "apps/free_camera_app.h"
#include "apps/phong_app.h"
#include "apps/run_app.h"
#include "apps/textured_cube_app.h"
#include "apps/triangle_app.h"

#include "commands/help_command.h"
#include "commands/version_command.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>

namespace bc = buddd::cmd;
namespace be = buddd::engine;

auto main(int argc, char* argv[]) -> int {
    if (argc <= 0) return EXIT_FAILURE;  // defensive
    // No positional argument -> default to run with no scene
    if (argc < 2 || argv[1] == nullptr) {
        bc::app::RunApp run_app_instance;
        auto args = bc::parse_running_args(argc, argv, 1);
        if (!args) {
            std::fprintf(stderr, "Error: %s\n", be::to_string(args.error()).c_str());
            return EXIT_FAILURE;
        }
        return bc::run_app(run_app_instance, *args);
    }

    std::string_view cmd{argv[1]};

    if (cmd == "version")
        return bc::VersionCommand::run(argc, argv);
    if (cmd == "help")
        return bc::HelpCommand::run(argc, argv);

    if (cmd != "run") {
        std::fprintf(stderr, "Unknown command: '%s'\n\n", argv[1]);
        std::fwrite(bc::k_usage_text.data(), 1, bc::k_usage_text.size(), stderr);
        return EXIT_FAILURE;
    }

    // "run" command
    std::unique_ptr<bc::App> app;
    int flags_start;
    if (argc < 3 || argv[2] == nullptr) {
        app = std::make_unique<bc::app::RunApp>();
        flags_start = 2;
    } else {
        std::string_view scene{argv[2]};
        flags_start = 3;

        if (scene == "triangle")
            app = std::make_unique<bc::app::TriangleApp>();
        else if (scene == "cube")
            app = std::make_unique<bc::app::CubeApp>();
        else if (scene == "cube-scene")
            app = std::make_unique<bc::app::CubeSceneApp>();
        else if (scene == "textured-cube")
            app = std::make_unique<bc::app::TexturedCubeApp>();
        else if (scene == "free-camera")
            app = std::make_unique<bc::app::FreeCameraApp>();
        else if (scene == "phong")
            app = std::make_unique<bc::app::PhongApp>();
        else {
            std::fprintf(stderr, "Unknown scene: '%s'\n\n", argv[2]);
            // Print scene usage text (hardcoded here)
            std::fprintf(stderr,
                "Usage: buddd run [<scene>] [--frame N] [--capture N:path]...\n"
                "\n"
                "Available scenes:\n"
                "  (empty)      Interactive empty window (no scene)\n"
                "  triangle     Coloured triangle (120 frames)\n"
                "  cube         Rotating cube demo (120 frames)\n"
                "  cube-scene   Cube demo via scene graph (World + RenderSystem, 120 frames)\n"
                "  textured-cube  Textured cube with UV-mapped brick texture (120 frames)\n"
                "  free-camera  Interactive free camera (WASD + mouse look, ESC to exit)\n"
                "  phong        Phong lighting demo (interactive, 5 cubes + 5 lights)\n"
                "\n"
                "Flags:\n"
                "  --frame N        Render exactly N frames, then exit (default: interactive)\n"
                "  --capture N:path  Capture frame N to path; can be repeated\n"
                "\n"
                "Scene names are case-sensitive.\n");
            return EXIT_FAILURE;
        }
    }

    // Parse running arguments (--frame, --capture)
    auto args = bc::parse_running_args(argc, argv, flags_start);
    if (!args) {
        std::fprintf(stderr, "Error: %s\n", be::to_string(args.error()).c_str());
        return EXIT_FAILURE;
    }

    // Warning for extra arguments after scene name
    if (argc > flags_start) {
        std::fprintf(stderr, "Warning: unexpected arguments after '%s':", argv[flags_start]);
        for (int i = flags_start; i < argc; ++i)
            std::fprintf(stderr, " %s", argv[i]);
        std::fprintf(stderr, "\n");
    }

    return bc::run_app(*app, *args);
}
```

**Requirements:**
- If `cmd != "run"` and not version/help: "Unknown command" error + usage to stderr, exit 1.
- Scene name validation happens BEFORE platform resources are created (fails fast).
- Scene usage text is hardcoded in `main.cpp` (consistent with existing `k_usage_text` pattern).
- `RunApp` is default when no scene given (matches current `buddd run` behavior).
- `parse_running_args()` is called before `run_app()`. The start index is 2 when no scene name is given (flags start after "run"), and 3 when a scene name is given (flags start after "run <scene>").
- `run_app()` receives the pre-parsed `RunningArgs` from `parse_running_args()`, not raw `argc`/`argv`.
- For no-scene run: extra arguments produce a warning on stderr but proceed.

### 7. `src/cmd/CMakeLists.txt` — Updated glob

```cmake
file(GLOB_RECURSE CMD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/demo/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cpp
)

add_executable(buddd ${CMD_SOURCES})

target_include_directories(buddd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(buddd PRIVATE buddd_engine)

if(BUDDD_HAS_DISPLAY)
    target_compile_definitions(buddd PRIVATE BUDDD_HAS_DISPLAY)
    message(STATUS "buddd: BUDDD_HAS_DISPLAY=ON (SDL3 backend)")
else()
    message(STATUS "buddd: BUDDD_HAS_DISPLAY=OFF (headless backend)")
endif()
```

**Changes from current:**
- Remove `${CMAKE_CURRENT_SOURCE_DIR}/capture/*.cpp` (entire capture directory deleted).
- Add `${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cpp` (new App subclasses).
- Keep `${CMAKE_CURRENT_SOURCE_DIR}/demo/*.cpp` (for `demo_helpers.*`).
- Keep `${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp` (for `version_command.*`, `help_command.*`).

### 8. `src/cmd/commands/help_command.h` — Updated k_usage_text

Replace the current `k_usage_text` content with:

```cpp
inline constexpr std::string_view k_usage_text =
    "Usage: buddd <command> [<args>]\n"
    "\n"
    "Commands:\n"
    "  run       Run a scene or the interactive window (default)\n"
    "  version   Print version information\n"
    "  help      Show this help message\n"
    "\n"
    "For scene usage: buddd run --help\n";
```

### 9. Deletion specification

Files to delete (after ensuring their logic is absorbed into App subclasses):
- `src/cmd/commands/run_command.h` and `run_command.cpp`
- `src/cmd/commands/demo_command.h` and `demo_command.cpp`
- `src/cmd/commands/capture_command.h` and `capture_command.cpp`
- `src/cmd/capture/` (entire directory — `cube_capture.*` and `phong_capture.*`)
- `src/cmd/demo/triangle_demo.*`
- `src/cmd/demo/cube_demo.*`
- `src/cmd/demo/cube_scene_demo.*`
- `src/cmd/demo/free_camera_demo.*`
- `src/cmd/demo/phong_demo.*`
- `src/cmd/demo/textured_cube_demo.*`

The directory `src/cmd/capture/` is removed entirely. The files `src/cmd/demo/demo_helpers.*` remain.

## Required tests

### Unit tests for `parse_running_args()` flag parsing (if testable without display)

These tests verify `parse_running_args()` logic. They run without a display because they test only argument parsing, not rendering.

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| AP-01 | `"--frame 60 sets frame_limit to 60"` | `[cli][app]` | Call `parse_running_args()` with `--frame 60` in argv. Verify `frame_limit == 60`. |
| AP-02 | `"--frame 0 is treated as no limit"` | `[cli][app]` | Verify that `--frame 0` produces an error (`N < 1` is invalid). |
| AP-03 | `"--frame -1 produces error"` | `[cli][app]` | Invalid frame: error returned. |
| AP-04 | `"--frame abc produces error"` | `[cli][app]` | Non-numeric: error returned. |
| AP-05 | `"--capture 50:/tmp/a.png adds one CaptureSpec"` | `[cli][app]` | Verify `captures` has size 1, frame=50, path="/tmp/a.png". |
| AP-06 | `"--capture /tmp/out.png (no N:) produces error"` | `[cli][app]` | Invalid: error returned. |
| AP-07 | `"--capture 0:path produces error"` | `[cli][app]` | Frame 0 is invalid (1-based): error returned. |
| AP-08 | `"--capture 50:/tmp/a.png --capture 200:/tmp/b.png adds two specs"` | `[cli][app]` | Verify two CaptureSpec entries with correct frame/path. |
| AP-09 | `"--frame 60 --capture 50:/tmp/a.png --capture 200:/tmp/b.png combined"` | `[cli][app]` | Both frame_limit and captures are set correctly. |
| AP-10 | `"No flags produces default config"` | `[cli][app]` | frame_limit=0, captures empty. |

**Implementation note:** These tests call `parse_running_args()` directly from `app_config.h`, which is self-contained and has no dependency on the `App` class or display backend.

### CLI integration tests (subprocess, require built binary)

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| CT-01 | `"buddd --help prints usage"` | `[cli]` | Run `buddd help` — stdout contains new command list (no `demo`/`capture`). |
| CT-02 | `"buddd demo triangle prints unknown command"` | `[cli]` | Run `buddd demo triangle` — stderr contains `"Unknown command: 'demo'"`, exit code 1. |
| CT-03 | `"buddd capture cube prints unknown command"` | `[cli]` | Run `buddd capture cube` — stderr contains `"Unknown command: 'capture'"`, exit code 1. |
| CT-04 | `"buddd run unknownscene prints error"` | `[cli]` | Run `buddd run unknownscene` — stderr contains `"Unknown scene: 'unknownscene'"`, exit code 1. |
| CT-05 | `"buddd version prints version"` | `[cli]` | Unchanged: stdout contains `"buddd 0.1.0"`, exit code 0. |
| CT-06 | `"buddd --frame 60 runs and exits"` | `[cli][headless]` | Run with `--frame 60` on headless — exits with code 0. |

### render_scene() test

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| RS-01 | `"render_scene() does not call begin_frame or end_frame"` | `[render][headless]` | Use headless backend with diagnostic counters. Create a World with a CameraComponent and RenderSystem. Call `render_scene()` inside a begin_frame/end_frame pair. Verify `frame_begin_count()` and `frame_end_count()` are unchanged by `render_scene()` itself (only the outer begin_frame/end_frame increment them). |

### Test linkage to acceptance criteria

| AC ID | Test(s) |
|---|---|
| AC-001 (app.h exists with App class) | File exists, compiles |
| AC-002 (AppConfig fields) | Code review |
| AC-003 (CaptureSpec fields) | Code review |
| AC-004 (run_app declared/defined) | Build succeeds |
| AC-005 (--frame N parsing) | AP-01, AP-03, AP-04 |
| AC-006 (--capture parsing) | AP-05, AP-06, AP-07, AP-08, AP-09 |
| AC-007 (Window opened message) | Manual (requires display) |
| AC-008 (setup error → shutdown called) | Manual (mock App) |
| AC-009 (render called per frame) | Manual (mock App or headless) |
| AC-010 (capture read_pixels + save) | Manual via `--capture` |
| AC-011 (shutdown called) | Manual (mock App) |
| AC-012 (render_scene exists, no begin/end) | RS-01, code review |
| AC-013 (render() delegates to render_scene) | RS-01, code review |
| AC-014–AC-018 (old files removed) | Files do not exist |
| AC-019 (app files created) | Files exist |
| AC-020 (demo_helpers unchanged) | Files exist, verified |
| AC-021–AC-023 (scene runs) | Manual visual |
| AC-024 (capture frame 120) | Manual |
| AC-025 (multi-capture) | Manual |
| AC-026 (capture without N: error) | AP-06 |
| AC-027 (unknown scene) | CT-04 |
| AC-028 (old demo command rejected) | CT-02 |
| AC-029 (old capture command rejected) | CT-03 |
| AC-030 (version command unchanged) | CT-05 |
| AC-031 (help text updated) | CT-01 |
| AC-032 (CMakeLists.txt glob updated) | Code review + build |
| AC-033 (CONST-001 compliance) | grep check |

## Edge cases

| Case | Expected behavior |
|---|---|
| `buddd run` with no scene and no flags | Opens empty window, runs interactively until window close. |
| `buddd run ''` (empty string as scene name) | Empty string not a valid scene name → unknown scene error. |
| `buddd run TRIANGLE` (uppercase) | Case-sensitive → unknown scene error. |
| `buddd run triangle extra_arg` | Warning printed to stderr: `"Warning: unexpected arguments after 'run triangle': extra_arg"`. Scene proceeds. |
| `buddd run triangle --frame 60` | Triangle runs for 60 frames (overrides default 120). |
| `--frame 0` | Treated as no limit (interactive mode) — error per spec (N < 1 invalid). |
| `--frame -1` | Invalid: error printed to stderr; exits with code 1. |
| `--frame abc` | Invalid: error printed to stderr; exits with code 1. |
| `--capture abc:path` (non-numeric frame) | Invalid: error printed to stderr; exits with code 1. |
| `--capture 0:path` | Frame 0 is invalid (1-based): error printed; exits with code 1. |
| `--capture :path` (empty frame number) | Invalid: error printed to stderr; exits 1. |
| `--capture /tmp/out.png` without frame number | Invalid: error printed to stderr; exits 1. |
| Multiple `--capture` with same frame number | Both are stored and processed; the second overwrites the first file silently. |
| `--capture` path is a directory | `Image::save()` fails; error printed to stderr; continues to next capture. |
| Window closed during frame-limited run | `poll_events()` returns false; loop exits early; remaining captures not processed; `shutdown()` called. |
| `BUDDD_HAS_DISPLAY=OFF` (headless) | Uses headless backend. `poll_events()` always returns `true` — use `--frame` to limit. Without `--frame`, runs until killed. |
| Frame-limited run with `--capture` where frame > frame_limit | Capture spec stored but never matched; no output file created (no error). |
| `--capture 1:path` (frame 1) | Driver quirk: `run_app()` forces minimum frame to 2; frame 1 is silently skipped and frame 2 is captured instead. No warning. |
| No scene given but extra unknown flags | Extra arguments are warned about but the run proceeds. |

## Security impact

- No elevated privileges required.
- No network access required at runtime.
- No secrets, credentials, or environment variables consumed.
- Architecture boundary CONST-001 is preserved: no SDL3, OpenGL, or GLM headers included from `src/cmd/`.
- Output PNG files are written to paths specified by the user via `--capture`. The binary does not create files outside those paths.
- The existing exception AMEND-2026-001 (SDL3 test files) is unaffected.

## Data and migration impact

None. No schema changes, data migrations, seed data, or persistent state. All changes are code restructuring.

## API compatibility impact

- **Breaking**: `buddd demo <name>` no longer works. Use `buddd run <scene>` instead.
- **Breaking**: `buddd capture <scenario>` no longer works. Use `buddd run <scene> --capture N:path` instead.
- **Breaking**: `buddd run <scene>` now uses 1024×768 window (was 800×600 for demo mode). This changes the rendered output dimensions for captures.
- **New**: `buddd run [<scene>] [--frame N] [--capture N:path]...` unified command.
- **New**: `--capture N:path` replaces the old capture subcommand. N is always required.
- **Backward-compatible**: `buddd` with no arguments still opens an empty interactive window (same as `buddd run`).
- **Backward-compatible**: `buddd version` and `buddd help` are unchanged (except help text content).
- `RenderSystem` gains a new public method `render_scene()`. The existing `render()` still works identically.

## Documentation impact

- `src/cmd/commands/help_command.h` is updated with new `k_usage_text`.
- The wiki module map (`docs/wiki/architecture/module-map.md`) already documents the old structure. The wiki-agent should update it to reflect the new architecture.
- No README changes required as part of this contract.

## ADR impact

- **ADR-004** (Demo System Architecture): The per-demo-file architecture inherited from SPEC-007 is superseded by the App pattern. However, `demo_helpers` remain and the if/else-if dispatch pattern is preserved. No ADR change is required — the new architecture is documented in SPEC-008.
- No new ADR is required. All design decisions were resolved during the spec phase.

## Constitution impact

No constitution changes are required. CONST-001 is preserved (no SDL3/OpenGL/GLM headers in `src/cmd/`). CONST-002 (testing policy) is satisfied by the test requirements above.

## Done criteria

The contract is done when ALL of the following are satisfied:

### Files created
- [ ] `src/cmd/app_config.h` — declares `CaptureSpec`, `RunningArgs`, `parse_running_args()`.
- [ ] `src/cmd/app_config.cpp` — implements `parse_running_args()`.
- [ ] `src/cmd/app.h` — declares `AppConfig`, `App` base class, `run_app()` (includes `app_config.h`).
- [ ] `src/cmd/app.cpp` — implements `run_app()`.
- [ ] `src/cmd/apps/run_app.h/.cpp` — `RunApp` (empty window).
- [ ] `src/cmd/apps/triangle_app.h/.cpp` — `TriangleApp`.
- [ ] `src/cmd/apps/cube_app.h/.cpp` — `CubeApp`.
- [ ] `src/cmd/apps/cube_scene_app.h/.cpp` — `CubeSceneApp`.
- [ ] `src/cmd/apps/textured_cube_app.h/.cpp` — `TexturedCubeApp`.
- [ ] `src/cmd/apps/free_camera_app.h/.cpp` — `FreeCameraApp`.
- [ ] `src/cmd/apps/phong_app.h/.cpp` — `PhongApp`.
- [ ] All 18 new files compile without errors.

### Files modified
- [ ] `src/cmd/main.cpp` — new dispatch (no `demo`/`capture` branches, `run` dispatch with scene names).
- [ ] `src/cmd/commands/help_command.h` — `k_usage_text` updated (no `demo`/`capture`, updated `run` description).
- [ ] `src/cmd/CMakeLists.txt` — glob updated (remove `capture/*.cpp`, add `apps/*.cpp`).
- [ ] `src/engine/render/render_system.h` — `render_scene()` declared as public member.
- [ ] `src/engine/render/render_system.cpp` — `render_scene()` extracted; `render()` delegates to it.

### Files deleted
- [ ] `src/cmd/commands/run_command.h` and `run_command.cpp` — removed.
- [ ] `src/cmd/commands/demo_command.h` and `demo_command.cpp` — removed.
- [ ] `src/cmd/commands/capture_command.h` and `capture_command.cpp` — removed.
- [ ] `src/cmd/capture/` directory — removed (all files).
- [ ] `src/cmd/demo/triangle_demo.*` — removed.
- [ ] All 6 other demo file pairs removed (cube, cube_scene, free_camera, phong, textured_cube, triangle).
- [ ] `src/cmd/demo/demo_helpers.h/.cpp` — unchanged and still present.

### Build
- [ ] `cmake --build --preset debug` succeeds with no errors related to new code.
- [ ] CONST-001 compliance: `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` returns zero matches.

### RenderSystem
- [ ] `render_scene()` contains the rendering logic (light collection, MeshRenderer iteration) but no `begin_frame()` or `end_frame()` calls.
- [ ] `render()` calls `begin_frame()` → `render_scene()` → `end_frame()`.

### Tests
- [ ] Flag parsing tests pass (AP-01 through AP-10, via direct calls to `parse_running_args()` from `app_config.h`).
- [ ] CLI integration tests pass (CT-01 through CT-06).
- [ ] `render_scene()` headless test passes (RS-01).
- [ ] All existing tests still pass.

### Manual verification (requires display — gated by `BUDDD_HAS_DISPLAY`)
- [ ] `buddd` (no args) opens empty 1024×768 window, clears framebuffer, closes cleanly.
- [ ] `buddd run triangle` renders triangle for 120 frames, exits with code 0.
- [ ] `buddd run cube` renders rotating cube, exits with code 0.
- [ ] `buddd run cube-scene` renders rotating cube via RenderSystem, exits with code 0.
- [ ] `buddd run textured-cube` renders textured cube, exits with code 0.
- [ ] `buddd run free-camera` interactive, ESC exits cleanly.
- [ ] `buddd run phong` interactive, ESC exits cleanly.
- [ ] `buddd run cube --frame 3 --capture 3:/tmp/test.png` captures frame 3 as valid PNG.
- [ ] `buddd run cube --capture 1:/tmp/frame1.png` silently captures frame 2 (driver quirk).
- [ ] `buddd demo triangle` → `"Unknown command: 'demo'"` + exit 1.
- [ ] `buddd capture cube` → `"Unknown command: 'capture'"` + exit 1.
- [ ] `buddd version` → `"buddd 0.1.0"` + exit 0.
- [ ] `buddd help` → updated usage text (no `demo`/`capture`).
- [ ] `buddd run unknownscene` → `"Unknown scene: 'unknownscene'"` + exit 1.
