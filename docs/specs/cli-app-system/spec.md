# SPEC-008 — CLI App System: Centralised Render Loop with Scene Dispatch

## Problem

The CLI binary currently has three separate render-loop implementations spread across `RunCommand`, `DemoCommand`, and `CaptureCommand`. Each creates its own `Platform`/`Window`/`RenderDevice`, runs its own `poll_events`/`begin_frame`/`end_frame` loop, and handles frame limiting differently. This duplication makes the codebase harder to maintain and extend:

1. **RunCommand** opens an empty window and runs `begin_frame`/`end_frame` with no draw calls.
2. **DemoCommand** creates resources, then dispatches to per-demo functions — each of which has its own render loop with `begin_frame`/`end_frame`.
3. **CaptureCommand** creates resources, dispatches to per-scenario functions — each of which has its own render loop with `begin_frame`/`end_frame` plus `read_pixels`.
4. The `phong_capture` module manually reimplements `RenderSystem::render()` (140+ lines) solely to inject a `read_pixels` call inside the frame.
5. The `demo` and `capture` subcommands are separate CLI entry points when they could be combined: a single `run` command with optional scene name and capture flags.
6. There is no reusable "App" lifecycle (`setup` → render loop → `shutdown`) that future tools or tests could use.

## Goals

- **G-01**: Introduce a single `App` base class with `setup()`, `render()`, and `shutdown()` lifecycle methods, an `AppConfig` struct for window metadata, and a `RunningArgs` struct for frame limits and capture specifications.
- **G-02**: Introduce a `run_app()` function that owns the central render loop, accepting a `RunningArgs` struct for frame limits and capture specifications.
- **G-03**: Create one `App` subclass per existing scene (empty window, triangle, cube, cube-scene, textured-cube, free-camera, phong), absorbing the render logic from the demo and capture files.
- **G-04**: Unify the CLI into a single `run` command: `buddd run [<scene>] [--frame N] [--capture [N:]path]...`
- **G-05**: Remove the `demo` and `capture` subcommands entirely (no backward compat).
- **G-06**: Add `RenderSystem::render_scene()` — the rendering logic of `render()` but without `begin_frame()`/`end_frame()`, so callers can inject `read_pixels` between them.
- **G-07**: Keep `RenderSystem::render()` as backward compat: `begin_frame()` → `render_scene()` → `end_frame()`.
- **G-08**: Delete `src/cmd/commands/demo_command.*`, `src/cmd/commands/capture_command.*`, `src/cmd/commands/run_command.*`, and the entire `src/cmd/capture/` directory.
- **G-09**: Preserve the architecture boundary (CONST-001): no SDL3, OpenGL, or GLM headers from `src/cmd/`.

## Supersedes

This spec supersedes the following parts of SPEC-006 (CLI Command System) and SPEC-007 (CLI Command Evolution):

| Element | Superseded by |
|---|---|
| **RunCommand** (class, files, behavior) | Replaced by the `RunApp` subclass of `App` and `run_app()` dispatch. `src/cmd/commands/run_command.*` is removed. |
| **DemoCommand** (class, files, behavior) | Removed. The `demo` subcommand no longer exists. Use `buddd run <scene>` instead. |
| **CaptureCommand** (class, files, behavior) | Removed. The `capture` subcommand no longer exists. Use `buddd run <scene> --capture [N:]path` instead. |
| **Per-demo functions** (`run_triangle_demo`, `run_cube_demo`, etc.) | Replaced by `App` subclasses (`TriangleApp`, `CubeApp`, etc.) in `src/cmd/apps/`. |
| **Per-capture functions** (`capture_cube_scene`, `capture_phong_scene`) | Replaced by `App` subclasses. Capture logic is handled by `run_app()` using `RunningArgs.captures`. |
| **Help text** | Updated: `demo` and `capture` removed from command list; `run` description now mentions scenes and capture flags. |
| **Unknown command** | `demo` and `capture` fall through to unknown-command handler. |
| **RenderSystem::render()** contract | `render()` still works as before. New `render_scene()` provides the rendering logic without begin/end framing. |

The following elements are **not** superseded:
- `VersionCommand` and its behavior (SPEC-006 AC-002, AC-010, AC-016).
- `HelpCommand` structure — only the text content changes.
- Unknown-command dispatch rule.
- No-args default behavior (defaults to `run` with no scene = empty window).
- The `setup_triangle()` and `setup_cube()` helpers in `demo_helpers.h/.cpp` remain unchanged (used by App subclasses).
- Architecture boundary CONST-001.

## Non-goals

- No changes to the engine library (`src/engine/`) beyond the `RenderSystem::render_scene()` addition.
- No changes to the RenderSystem rendering logic itself — only the begin/end framing is extracted.
- No changes to demo scene content (models, materials, lights, animation parameters, camera positions).
- No changes to existing tests (they use `EngineService`, not the CLI).
- No rewriting of `demo_helpers` — their `std::exit` error handling stays as-is.
- No new demo scenes beyond the existing set (triangle, cube, cube-scene, textured-cube, free-camera, phong).
- No common base class for VersionCommand/HelpCommand — those stay as standalone classes.
- No CLI framework or third-party argument parsing library.
- No dynamic app discovery, plugin system, or reflection.
- No tab-completion or shell integration.

## Actors

| Actor | Description |
|---|---|
| Developer | A human running the `buddd` CLI binary from a terminal. Uses `buddd run [<scene>]` with optional flags. |
| Scene author | A developer who adds new visual demonstrations by creating an `App` subclass in `src/cmd/apps/` and adding a dispatch entry in `run_app()`. |
| CLI maintainer | A developer who maintains the binary entry point, `run_app()`, and build system integration. |

## User-visible behavior

### Command dispatch rules

1. The first positional argument after `buddd` selects the command.
2. If no positional argument is given (`argc == 1`), the default command is `run` (equivalent to `buddd run`).
3. Each command receives the full `argc`/`argv` and is responsible for its own argument parsing beyond the command name.
4. Commands are case-sensitive lowercase.
5. The old `demo` and `capture` subcommands are **removed**. Passing them as the first positional argument produces an "unknown command" error.

### `buddd run [<scene>] [--frame N] [--capture [N:]path]...`

Opens a window (1024×768, title `"Buddd Engine"` or `"Buddd Engine — <scene>"` when a scene name is given), runs the named scene, and exits. When no scene is given, opens an empty window that clears the framebuffer each frame (identical to current `buddd run` behavior).

**Behavior details**:
- Scene name is `argv[2]`. If `argc < 3` (no scene name), defaults to `RunApp` (empty window).
- Scene name is validated **before** creating platform resources: if unknown, prints error and exits immediately.
- Platform created using the `BUDDD_HAS_DISPLAY` compile-time define: when `ON`, uses `be::Backend::SDL3`; when `OFF`, uses `be::Backend::Headless`.
- Window dimensions: 1024×768.
- Window title:
  - No scene: `"Buddd Engine"`
  - With scene: `"Buddd Engine — <scene>"` where `<scene>` is the scene name as typed (case-preserving).
- Global flags are parsed from `argv[3..]` (when scene given) or `argv[2..]` (when no scene):
  - `--frame N`: frame limit. When N > 0, the render loop runs exactly N frames then exits. When 0 or omitted, runs interactively (no limit) until window close or ESC.
   - `--capture N:path`: capture specification. Can be specified multiple times. Format: `N:path` where `N` is the 1-based frame number and `path` is the output PNG file path. `N` is always required — there is no default frame.
- The render loop does **not** use `sleep_for` for frame limiting. It relies on VSync (`end_frame` swap ↔ display refresh).
- If the user closes the window during the loop (SDL3 backend only), the loop exits early and exits with code 0.
- If `--frame N` is given, the loop runs for N frames and exits automatically (useful for CI/headless where polling never returns false).

**Observability messages**:
- `stdout` (always): `"Window opened: 1024x768"`
- `stderr` (scene running): `"Scene started: <scene> (<frame_limit> frames)"` or `"Scene started: <scene> (interactive)"`
- `stderr` (early abort): `"Scene aborted by user (frame N)"`
- `stderr` (successful end): `"Scene complete: <scene> (<frames> frames rendered)"`
- `stdout` (capture saved): `"Captured: <path>"` (one line per capture)
- `stdout` (shutdown): `"Window closed, shutting down."`

**Error messages** (`stderr`):
- Unknown scene: `"Unknown scene: '<name>'"` followed by scene usage text.

**Scene usage text** (printed for unknown scene or `buddd run` with no scene and `--help`-style error):
```
Usage: buddd run [<scene>] [--frame N] [--capture [N:]path]...

Available scenes:
  (empty)      Interactive empty window (no scene)
  triangle     Coloured triangle (120 frames)
  cube         Rotating cube demo (120 frames)
  cube-scene   Cube demo via scene graph (World + RenderSystem, 120 frames)
  textured-cube  Textured cube with UV-mapped brick texture (120 frames)
  free-camera  Interactive free camera (WASD + mouse look, ESC to exit)
  phong        Phong lighting demo (interactive, 5 cubes + 5 lights)

Flags:
  --frame N        Render exactly N frames, then exit (default: interactive)
   --capture N:path  Capture frame N to path; can be repeated (N is required)

Scene names are case-sensitive.
```

#### Scene behaviors

##### `buddd run` (no scene, empty window)
Same as current `RunCommand`: 1024×768, "Buddd Engine", clears framebuffer each frame, runs until window close. No triangle, no draw calls. Extra arguments beyond the scene name produce a warning on stderr: `"Warning: unexpected arguments: arg1 arg2"` but the run proceeds.

##### `buddd run triangle`
Runs for 120 frames (or `--frame N` if specified), renders a coloured triangle using vertex-colour interpolation. Output messages use "scene" language instead of "demo" language. Same rendering appearance as current `buddd demo triangle`.

##### `buddd run cube`
Runs for 120 frames (or `--frame N` if specified), renders a rotating colour-coded cube. Same rendering as current `buddd demo cube`.

##### `buddd run cube-scene`
Runs for 120 frames (or `--frame N` if specified), renders a rotating cube via scene graph (World + RenderSystem). Same as current `buddd demo cube-scene`.

##### `buddd run textured-cube`
Runs for 120 frames (or `--frame N` if specified), renders a rotating cube with brick texture. Same as current `buddd demo textured-cube`.

##### `buddd run free-camera`
Interactive free-camera scene. Runs until ESC or window close. Right-click to capture mouse, WASD + mouse look, Space/Ctrl up/down. Same as current `buddd demo free-camera`.

##### `buddd run phong`
Interactive Phong lighting demo. Runs until ESC or window close. Right-click to capture mouse, WASD + mouse look. 5 cubes with varying material properties, 5 lights (1 directional, 3 point, 1 spot) with orbiting point lights. Same as current `buddd demo phong`.

#### Capture behavior

When `--capture` is specified:
1. The render loop proceeds normally.
2. After `app.render()` returns (which leaves the framebuffer in the rendered state) but **before** `end_frame()` swaps the buffer, `run_app()` checks if the current `frame` matches any `CaptureSpec`'s frame number.
3. If matched, it calls `device.read_pixels()` to get the framebuffer contents, converts via `Image::create()`, and stores the `Image` paired with the output path.
4. After the render loop finishes, each stored image is saved to its path via `Image::save()`.
5. On save success, prints `"Captured: <path>"` to stdout.
6. On save failure, prints error to stderr and continues to the next capture (no early exit).

The frame number in `--capture N:path` is 1-based (first rendered frame is frame 1). `N` is always required — there is no default frame. Capturing without an explicit frame number is an error (see Error cases).

Example:
- `--capture 120:/tmp/out.png` — captures frame 120
- `--capture 50:/tmp/a.png --capture 200:/tmp/b.png` — captures frames 50 and 200
- `--capture 1:/tmp/first.png --capture 2:/tmp/second.png` — captures frames 1 and 2

**Driver quirk workaround**: The minimum capture frame is 2 (frame 1 on some OpenGL drivers returns the clear color). If a `CaptureSpec` specifies frame 1, `run_app` forces the frame to 2 before reading pixels. No warning is emitted — this is a known driver quirk documented in the spec.

### `buddd version` (VersionCommand)
Unchanged from SPEC-006 / SPEC-007. Prints version string and exits 0.

### `buddd help` (HelpCommand)
Updated to show the new command set:
```
Usage: buddd <command> [<args>]

Commands:
  run       Run a scene or the interactive window (default)
  version   Print version information
  help      Show this help message

For scene usage: buddd run --help
```

### Unknown command
If the first positional argument does not match any known command (`run`, `version`, `help`), the CLI prints an error message to **stderr** and exits with code 1. The usage block replaces `demo` and `capture` with the updated text.

## Key entities

### `CaptureSpec` and `RunningArgs` (`src/cmd/app_config.h`)

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

### `App` interface (`src/cmd/app.h`)

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

### `RenderSystem::render_scene()` addition

```cpp
// In src/engine/render/render_system.h (public member of class RenderSystem):

/// Renders one frame's worth of the scene, WITHOUT begin_frame()/end_frame().
/// The caller is responsible for framing. Same rendering logic as render(),
/// but does not call begin_frame() or end_frame().
/// Behaviour is undefined if called outside a begin_frame()/end_frame() pair.
/// No arguments — uses the stored RenderDevice and World references
/// (same as render()).
auto render_scene() -> void;

// Existing render() stays unchanged: begin_frame() → render_scene() → end_frame()
// render() now delegates to render_scene().
```

### App subclasses (`src/cmd/apps/`)

Each file in `src/cmd/apps/` defines a single `App` subclass in the `buddd::cmd::app` namespace. The naming convention is `<name>_app.h/.cpp` with the class named `<Name>App`.

| File | Class | Description |
|---|---|---|
| `apps/run_app.h/.cpp` | `RunApp` | Empty window, framebuffer clear only. `render()` does nothing (the central loop's `begin_frame()` already clears). |
| `apps/triangle_app.h/.cpp` | `TriangleApp` | Manual rendering: `setup()` calls `setup_triangle()`, `render()` sets uniforms and draws (no begin_frame/end_frame — handled by run_app()). |
| `apps/cube_app.h/.cpp` | `CubeApp` | Manual rendering: `setup()` calls `setup_cube()`, `render()` computes rotation, sets MVP, draws (no begin_frame/end_frame). |
| `apps/cube_scene_app.h/.cpp` | `CubeSceneApp` | RenderSystem-based: `setup()` creates World/Entity/CameraComponent/MeshRenderer/RenderSystem. `render()` rotates entity, calls `render_system.render_scene()`. |
| `apps/textured_cube_app.h/.cpp` | `TexturedCubeApp` | RenderSystem-based: `setup()` loads texture, creates World/RenderSystem. `render()` rotates entity, calls `render_system.render_scene()`. |
| `apps/free_camera_app.h/.cpp` | `FreeCameraApp` | RenderSystem-based: `setup()` creates World/RenderSystem/input handlers. `render()` handles input, updates camera, calls `render_system.render_scene()`. Sets `running_ = false` on ESC. |
| `apps/phong_app.h/.cpp` | `PhongApp` | RenderSystem-based: `setup()` creates World with cubes and lights. `render()` animates orbiting lights, calls `render_system.render_scene()`. |

**Capture variant behavior**: For scenes that have a capture scenario (cube, phong), the App subclasses adjust their camera position when captures are active:
- `CubeApp`: uses front-facing camera `(0, 0, 3)` for captures instead of `(3, 2, 3)`.
- `PhongApp`: the camera is always the same for both interactive and capture modes in the current codebase; no camera adjustment is needed.

The `render()` method in each App subclass is responsible for **one frame's worth of rendering**. The outer `run_app()` handles `begin_frame()`/`end_frame()` and `read_pixels()`.

### File changes summary

**Create**:
- `src/cmd/app_config.h` — CaptureSpec, RunningArgs, parse_running_args() declaration
- `src/cmd/app_config.cpp` — parse_running_args() implementation
- `src/cmd/app.h` — App interface, AppConfig, run_app() declaration
- `src/cmd/app.cpp` — run_app() implementation
- `src/cmd/apps/run_app.h/.cpp` — RunApp (empty window)
- `src/cmd/apps/triangle_app.h/.cpp` — TriangleApp
- `src/cmd/apps/cube_app.h/.cpp` — CubeApp (absorbs cube_demo + cube_capture)
- `src/cmd/apps/cube_scene_app.h/.cpp` — CubeSceneApp (absorbs cube_scene_demo)
- `src/cmd/apps/textured_cube_app.h/.cpp` — TexturedCubeApp
- `src/cmd/apps/free_camera_app.h/.cpp` — FreeCameraApp
- `src/cmd/apps/phong_app.h/.cpp` — PhongApp (absorbs phong_demo + phong_capture)

**Modify**:
- `src/cmd/main.cpp` — single `run_app()` dispatch for `run`; `demo`/`capture` removed; VersionCommand and HelpCommand remain
- `src/cmd/commands/help_command.h` — updated usage text (remove `demo`/`capture`)
- `src/cmd/CMakeLists.txt` — remove `capture/*.cpp` from glob, add `apps/*.cpp`
- `src/engine/render/render_system.h` — add `render_scene()` declaration
- `src/engine/render/render_system.cpp` — extract rendering logic into `render_scene()`, `render()` calls `begin_frame()` → `render_scene()` → `end_frame()`

**Remove**:
- `src/cmd/commands/run_command.h` — replaced by RunApp + run_app()
- `src/cmd/commands/run_command.cpp`
- `src/cmd/commands/demo_command.h` — removed (demo subcommand removed)
- `src/cmd/commands/demo_command.cpp`
- `src/cmd/commands/capture_command.h` — removed (capture subcommand removed)
- `src/cmd/commands/capture_command.cpp`
- `src/cmd/capture/cube_capture.h` — absorbed into CubeApp
- `src/cmd/capture/cube_capture.cpp`
- `src/cmd/capture/phong_capture.h` — absorbed into PhongApp
- `src/cmd/capture/phong_capture.cpp`
- `src/cmd/demo/cube_demo.h` — absorbed into CubeApp
- `src/cmd/demo/cube_demo.cpp`
- `src/cmd/demo/cube_scene_demo.h` — absorbed into CubeSceneApp
- `src/cmd/demo/cube_scene_demo.cpp`
- `src/cmd/demo/free_camera_demo.h` — absorbed into FreeCameraApp
- `src/cmd/demo/free_camera_demo.cpp`
- `src/cmd/demo/phong_demo.h` — absorbed into PhongApp
- `src/cmd/demo/phong_demo.cpp`
- `src/cmd/demo/textured_cube_demo.h` — absorbed into TexturedCubeApp
- `src/cmd/demo/textured_cube_demo.cpp`
- `src/cmd/demo/triangle_demo.h` — absorbed into TriangleApp
- `src/cmd/demo/triangle_demo.cpp`

**Unchanged**:
- `src/cmd/demo/demo_helpers.h` — setup_triangle() and setup_cube() still used by App subclasses
- `src/cmd/demo/demo_helpers.cpp`
- `src/cmd/commands/version_command.h/.cpp` — unchanged
- `src/cmd/commands/help_command.cpp` — unchanged (reads from `k_usage_text` constant)
- All engine files except `render_system.h/.cpp`

## User stories

### Story 1 — Run interactive empty window (Priority: P1)

As a developer, I want to run `buddd` with no arguments and see an empty interactive window, so that the engine remains immediately launchable.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd` with no arguments
**Then** a window opens (1024×768, title "Buddd Engine") with a cleared framebuffer, shows nothing, and closes cleanly when I close the window.

### Story 2 — Run a named scene (Priority: P1)

As a developer, I want to run `buddd run triangle` and see the triangle scene for 120 frames, so that I can verify the render pipeline.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd run triangle`
**Then** a window opens (1024×768, title "Buddd Engine — triangle") and renders a coloured triangle for 120 frames, prints completion to stderr, and exits with code 0.

### Story 3 — Capture a specific frame (Priority: P1)

As a developer, I want to run `buddd run cube --frame 120 --capture 120:/tmp/out.png` and get a PNG of frame 120, so that I can capture rendered output.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd run cube --frame 120 --capture 120:/tmp/out.png`
**Then** the cube runs for 120 frames, frame 120 is saved to `/tmp/out.png`, and stdout contains `"Captured: /tmp/out.png"`.

### Story 4 — Multi-capture (Priority: P1)

As a developer, I want to capture multiple frames in a single run, so that I can compare different animation states.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd run cube --capture 50:/tmp/a.png --capture 200:/tmp/b.png` with `--frame 200`
**Then** frames 50 and 200 are captured and saved to the respective paths.

### Story 5 — Unknown scene name (Priority: P1)

As a developer, I want to see a clear error when I mistype a scene name, so that I know what scenes are available.

**Given** the `buddd` binary is compiled
**When** I run `buddd run unknownscene`
**Then** stderr contains `"Unknown scene: 'unknownscene'"` followed by the scene usage text, and the process exits with code 1.

### Story 6 — Old `demo` and `capture` commands rejected (Priority: P1)

As a developer who used `buddd demo triangle` or `buddd capture cube`, I want the CLI to tell me those commands no longer exist.

**Given** the `buddd` binary is compiled
**When** I run `buddd demo triangle` or `buddd capture cube`
**Then** stderr contains `"Unknown command: 'demo'"` (or `"capture"`) followed by the usage block, and the process exits with code 1.

### Story 7 — Frame-limited run (Priority: P2)

As a developer, I want to run a scene for a specific number of frames, so that CI can run predictable-length tests.

**Given** the `buddd` binary is compiled with headless or display backend
**When** I run `buddd run triangle --frame 60`
**Then** the triangle renders exactly 60 frames and exits with code 0, regardless of window close.

### Story 8 — Interactive scene with Escape exit (Priority: P2)

As a developer, I want to press Escape in interactive scenes and have them exit cleanly, so that I don't have to force-close the window.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd run free-camera` and press Escape
**Then** the scene exits cleanly with code 0.

### Story 9 — Capture always requires an explicit frame (Priority: P2)

As a developer, I want to always specify the frame number when capturing, so that the behavior is unambiguous and there is no risk of capturing the wrong frame.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd run cube --capture 60:/tmp/out.png --frame 60`
**Then** frame 60 is captured and saved.

**When** I run `buddd run cube --capture /tmp/out.png` (without frame number)
**Then** an error is printed and the process exits with code 1.

### Story 10 — Print version (Priority: P1)

Unchanged. `buddd version` prints the version string and exits 0.

### Story 11 — Show help (Priority: P1)

Updated. `buddd help` prints the new command list without `demo` or `capture`.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `src/cmd/app.h` exists and declares `class App` with virtual `setup()`, `render()`, `shutdown()`, protected `running_`, and pure virtual `config()`. | File exists and compiles without error. |
| AC-002 | `src/cmd/app.h` declares `struct AppConfig` with `title` (string), `width` (int), and `height` (int). | Inspect file; the struct has the specified fields. |
| AC-003 | `src/cmd/app_config.h` declares `struct CaptureSpec` with `frame` (int) and `path` (string), and `struct RunningArgs` with `frame_limit` (int) and `captures` (vector of CaptureSpec). | Inspect file; the structs have the specified fields. |
| AC-004 | `run_app(App&, const RunningArgs&) -> int` is declared in `app.h` and defined in `app.cpp`. | Files exist; symbol resolves at link time. |
| AC-005 | `parse_running_args()` parses `--frame N` from argv and sets `RunningArgs::frame_limit`. | Call `parse_running_args()` with `--frame 60` and verify `frame_limit == 60`. |
| AC-006 | `parse_running_args()` parses `--capture N:path` (multiple allowed) and populates `RunningArgs::captures`. | Call `parse_running_args()` with `--capture 50:/tmp/a.png --capture 200:/tmp/b.png`; verify two CaptureSpec entries with correct frame/path. |
| AC-007 | `run_app()` creates a Platform, Window (1024×768), and RenderDevice before calling `app.setup()`. | Console contains `"Window opened: 1024x768"` on stdout. |
| AC-008 | `run_app()` calls `app.setup()`; if setup returns an error, the loop is skipped and `shutdown()` is called. | Write a mock App whose `setup()` returns an error; verify that `render()` is never called, `shutdown()` is called, and exit code is non-zero. |
| AC-009 | `run_app()` calls `app.render(device, frame)` once per frame, inside a `begin_frame()`/`end_frame()` pair. | Write a mock App whose `render()` increments a counter; run with `--frame 10` and verify the counter reaches 10. |
| AC-010 | `run_app()` calls `device.read_pixels()` after `app.render()` when the current frame matches a CaptureSpec frame, and saves the result as PNG after the loop. | Run `buddd run cube --frame 3 --capture 3:/tmp/test_ac10.png`; verify `/tmp/test_ac10.png` exists and is a valid PNG. |
| AC-011 | `run_app()` calls `app.shutdown()` after the render loop exits (both normal and early exit). | Write a mock App whose `shutdown()` sets a flag; verify the flag is set after `run_app()` returns. |
| AC-012 | `RenderSystem::render_scene()` exists and does not call `begin_frame()` or `end_frame()`. | Inspect `render_system.cpp` — `render_scene()` contains the rendering logic (light collection, MeshRenderer iteration) but no `begin_frame()` or `end_frame()` calls. |
| AC-013 | `RenderSystem::render()` calls `begin_frame()` → `render_scene()` → `end_frame()` and produces identical rendering to the old implementation. | Run `buddd run cube-scene --frame 10` and verify the window renders correctly (manual visual inspection). |
| AC-014 | `src/cmd/commands/demo_command.h` and `src/cmd/commands/demo_command.cpp` no longer exist. | Check that the files are removed. |
| AC-015 | `src/cmd/commands/capture_command.h` and `src/cmd/commands/capture_command.cpp` no longer exist. | Check that the files are removed. |
| AC-016 | `src/cmd/commands/run_command.h` and `src/cmd/commands/run_command.cpp` no longer exist. | Check that the files are removed. |
| AC-017 | `src/cmd/capture/` directory no longer exists (all its files removed). | Check that the directory is removed. |
| AC-018 | `src/cmd/demo/cube_demo.*`, `cube_scene_demo.*`, `free_camera_demo.*`, `phong_demo.*`, `textured_cube_demo.*`, `triangle_demo.*` no longer exist. | Check that all six `.h`/`.cpp` pairs are removed. |
| AC-019 | `src/cmd/apps/` directory exists with App subclass files for all six scenes plus RunApp. | `ls src/cmd/apps/` shows `run_app`, `triangle_app`, `cube_app`, `cube_scene_app`, `textured_cube_app`, `free_camera_app`, `phong_app` (both `.h` and `.cpp`). |
| AC-020 | `src/cmd/demo/demo_helpers.h` and `src/cmd/demo/demo_helpers.cpp` still exist and are unchanged. | Files exist; `setup_triangle()` and `setup_cube()` are present. |
| AC-021 | Running `buddd run` (no scene) opens an empty 1024×768 window, clears framebuffer, draws nothing. | Manual visual: black empty window. |
| AC-022 | Running `buddd run triangle` opens window titled "Buddd Engine — triangle" and renders the coloured triangle identically to the old `buddd demo triangle`. | Manual visual: triangle appearance matches. |
| AC-023 | Running `buddd run cube` renders the rotating cube identically to the old `buddd demo cube`. | Manual visual: cube appearance matches. |
| AC-024 | Running `buddd run cube --frame 120 --capture 120:/tmp/out.png` captures frame 120 successfully. | File `/tmp/out.png` exists; is a valid PNG with cube content. |
| AC-025 | Running `buddd run cube --capture 50:/tmp/a.png --capture 200:/tmp/b.png` captures both frames. | Both `/tmp/a.png` and `/tmp/b.png` exist and are valid PNGs. |
| AC-026 | Running `buddd run cube --capture /tmp/out.png` (without frame number) produces an error because `--capture` always requires `N:path`. | stderr contains `"Error: --capture requires a frame number (format: N:path)"`; exit code is 1; no output file is created. |
| AC-027 | Running `buddd run unknownscene` prints `"Unknown scene: 'unknownscene'"` to stderr and exits with code 1. | Run the command; verify stderr message and exit code. |
| AC-028 | Running `buddd demo triangle` prints `"Unknown command: 'demo'"` to stderr and exits with code 1. | Run `buddd demo triangle`; verify stderr and exit code. |
| AC-029 | Running `buddd capture cube` prints `"Unknown command: 'capture'"` to stderr and exits with code 1. | Run `buddd capture cube`; verify stderr and exit code. |
| AC-030 | Running `buddd version` prints the version string and exits 0 (unchanged). | Same as SPEC-006 AC-010 / SPEC-007 AC-013. |
| AC-031 | Running `buddd help` prints the updated usage message (without `demo`/`capture`, with scene description) to stdout and exits 0. | Run `buddd help`; stdout does not mention `demo` or `capture` as commands. |
| AC-032 | The `CMakeLists.txt` glob includes `apps/*.cpp` and does not include `capture/*.cpp`. Build succeeds. | `cmake --build --preset debug` succeeds; `build/debug/src/cmd/buddd` is produced. |
| AC-033 | No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/`. | Run `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` — zero matches. |
| AC-034 | The `phong_capture` manual reimplementation of RenderSystem is removed; PhongApp uses `RenderSystem::render_scene()` and `read_pixels()` is handled by `run_app()`. | Inspect `phong_app.cpp` — no manual light collection or MeshRenderer iteration; simply calls `render_system.render_scene()`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A new scene can be added by creating one `.h`/`.cpp` pair in `src/cmd/apps/` (defining an `App` subclass) and adding one dispatch entry in `main.cpp`, without modifying any other scene file or `CMakeLists.txt` (glob picks up new files automatically). | Create a skeleton `spin_app` that renders an animated spinning cube. Build succeeds. |
| SC-002 | The duplicated render loop logic (poll_events, begin_frame, end_frame, sleep_for, frame counting) is removed from all six per-scene files. The only render loop exists in `run_app()`. | Inspect all files under `src/cmd/apps/` — none contain a `while` loop with `poll_events`, `begin_frame()`, or `end_frame()`. |
| SC-003 | Every existing CLI test for `buddd version`, `buddd help`, and unknown commands still passes after the refactor. | Run the test suite; version and help tests pass unchanged. |
| SC-004 | `buddd run triangle` completes in under 3 seconds (120 frames at ~60 FPS without sleep_for, relying on VSync). | Measure wall-clock time from invocation to exit. |

## Edge cases

| Case | Expected behavior |
|---|---|
| `buddd run` with no scene and no flags | Opens empty window, runs interactively until window close. |
| `buddd run` + trailing whitespace (no extra args) | Treated as no arguments; defaults to empty window. |
| `buddd run ''` (empty string as scene name) | Empty string is not a valid scene name; treated as unknown scene. |
| `buddd run TRIANGLE` (uppercase) | Case-sensitive comparison fails; treated as unknown scene. |
| `buddd run triangle extra_arg` | Warning printed to stderr about unexpected arguments; scene proceeds. |
| `--frame 0` | Treated as no limit (interactive mode). |
| `--frame -1` | Invalid: error printed to stderr; exits with code 1. |
| `--frame abc` | Invalid: error printed to stderr; exits with code 1. |
| `--capture abc:path` (non-numeric frame) | Invalid: error printed to stderr; exits with code 1. |
| `--capture 0:path` | Frame 0 is invalid (1-based): error printed; exits with code 1. |
| `--capture :path` (empty frame number) | Invalid: frame number must be a positive integer; error printed to stderr; exits 1. |
| `--capture /tmp/out.png` without frame number | Invalid: `--capture` always requires `N:path` format with an explicit frame number; error printed to stderr; exits 1. |
| Multiple `--capture` with same frame number | Both are stored and processed; the second overwrites the first file path silently. |
| `--capture` path is a directory | `Image::save()` fails; error printed to stderr; continues to next capture. |
| Window closed during interactive scene before `app.setup()` | Window never created; `run_app()` returns error. |
| Window closed during frame-limited run | `poll_events()` returns false; loop exits early; remaining captures are not processed; `shutdown()` called. |
| `BUDDD_HAS_DISPLAY=OFF` (headless) | Uses headless backend. `poll_events()` always returns `true` — use `--frame` to limit. Without `--frame`, runs until killed. |
| `run_app()` called twice on the same `App` | Undefined (not a supported use case). |
| `app.config()` called at any time | Returns the App's default `AppConfig` (title, width, height) — always safe to call. |
| Frame-limited run with `--capture` where frame > frame_limit | Capture spec is stored but never matched; no output file created (no error). |
| `--capture 1:path` (frame 1) | Driver quirk: `run_app()` forces minimum frame to 2; frame 1 is silently skipped and frame 2 is captured instead. No warning is printed — this is expected behaviour documented in the spec. |
| No scene given but extra unknown flags | Extra arguments are warned about but the run proceeds. |

## Error cases

| Case | Expected behavior |
|---|---|
| Unknown scene name | Print to stderr: `"Unknown scene: '<name>'"` followed by scene usage. Exit code 1. |
| Unknown command (`demo`, `capture`, etc.) | Print to stderr: `"Unknown command: '<cmd>'"` followed by usage block. Exit code 1. |
| `--frame N` with N < 1 or non-numeric | Print error to stderr; exit code 1. |
| `--capture` with invalid frame number | Print error to stderr; exit code 1. |
| `--capture /path` without frame number | Print error to stderr: `"Error: --capture requires a frame number (format: N:path)"`. Exit code 1. |
| `--capture path` where path is a directory | Print error to stderr from `Image::save()`; continue to next capture; exit with code 0 if at least one capture succeeded, else 1. |
| `app.setup()` returns error | Print error to stderr; call `app.shutdown()`; exit with `EXIT_FAILURE`. |
| Platform creation failure | Print error to stderr; exit with `EXIT_FAILURE`. |
| Window creation failure | Print error to stderr; exit with `EXIT_FAILURE`. |
| Render device creation failure | Print error to stderr; exit with `EXIT_FAILURE`. |
| `Image::save()` fails for all captures | Print errors to stderr; exit with `EXIT_FAILURE`. |
| `Image::save()` fails for some captures | Print errors for failed captures; print success for successful ones; exit with `EXIT_SUCCESS` if at least one succeeded. |
| `argv[1]` is `nullptr` (defensive) | Treated as no arguments; defaults to `run` with no scene. |
| `argc == 0` (defensive, impossible on hosted) | No `argv[0]` available; defaults to `run`. |

## Permissions and security

- The CLI binary requires no elevated privileges (root/admin) to run.
- No network access is required at runtime.
- No secrets, credentials, or environment variables are consumed.
- The architecture boundary CONST-001 is preserved: no SDL3, OpenGL, or GLM headers are included from `src/cmd/`. All platform and graphics access goes through engine abstractions (`Platform`, `Window`, `RenderDevice`).
- The existing exception AMEND-2026-001 (SDL3 test files) is unaffected.
- Output PNG files are written to paths specified by the user via `--capture`. The binary does not create files outside those paths.

## Observability

| Signal | Source |
|---|---|
| Window opened | stdout: `"Window opened: 1024x768"` |
| Scene started | stderr: `"Scene started: <scene> (N frames)"` |
| Interactive scene started | stderr: `"Scene started: <scene> (interactive)"` |
| Scene aborted early | stderr: `"Scene aborted by user (frame N)"` |
| Scene completed normally | stderr: `"Scene complete: <scene> (N frames rendered)"` |
| Capture saved | stdout: `"Captured: <path>"` |
| Window shutdown | stdout: `"Window closed, shutting down."` |
| app.setup() error | stderr: error description from `Result<>` |
| Unknown scene | stderr: `"Unknown scene: '<name>'"` + usage |
| Unknown command | stderr: `"Unknown command: '<cmd>'"` + usage |
| --frame parse error | stderr: descriptive error |
| --capture parse error | stderr: descriptive error |
| Exit code | Shell variable `$?` after process exits |

## Out of scope

- Unit tests for individual App subclasses (tested via CLI invocation integration tests).
- Integration tests that automate window/rendering verification.
- A project/module loading system for `RunApp`.
- Additional scenes beyond the existing set.
- Tab-completion scripts or shell integration.
- Subcommand aliases or short forms.
- CLI argument parsing library or framework.
- Dynamic app discovery, plugin loading, or reflection.
- Colourised or styled terminal output.
- Cross-platform testing of CLI argument parsing.
- Changes to triangle/cube rendering (vertex data, shaders, appearance).
- Rewriting `demo_helpers` error handling (std::exit stays).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The engine's public API (`Platform::create()`, `Platform::poll_events()`, `Window`, `RenderDevice`, `read_pixels()`, `Image::create()`, `Image::save()`) is unchanged by this spec except for the `RenderSystem::render_scene()` addition. |
| A-02 | The `RenderDevice::read_pixels()` method must be called AFTER `app.render()` and BEFORE `device.end_frame()` (within the begin/end pair) to read from the back buffer before the swap. |
| A-03 | C++26 standard library features (`std::string_view`, `std::span`, `std::expected` via `Result`) are available. |
| A-04 | The existing `src/cmd/CMakeLists.txt` glob pattern is updated to include `apps/*.cpp` and exclude `capture/*.cpp`. `demo/*.cpp` stays for `demo_helpers.*`. |
| A-05 | The `setup_triangle()` and `setup_cube()` functions in `demo_helpers` are unchanged and used by App subclasses via `#include "demo/demo_helpers.h"`. |
| A-06 | At compile time, `BUDDD_HAS_DISPLAY=OFF` selects the headless backend, which succeeds at platform creation but never returns `false` from `poll_events()`. |
| A-07 | The old `demo` and `capture` subcommands are dropped with no deprecation period. Developers who used them will see the unknown-command error and can switch to `buddd run <scene>`. |
| A-08 | The `run` command defaults to `RunApp` (empty window) when no scene name is given, matching the current `buddd run` behavior. |
| A-09 | The help text is an exact string embedded in `HelpCommand` (no i18n, no templating). |
| A-10 | The render loop does NOT use `sleep_for` (relies on VSync). |
| A-11 | The minimum capture frame workaround (frame 1 driver quirk → use frame 2) matches the existing behavior in `cube_capture.cpp` and `phong_capture.cpp`. |

## Open questions

| ID | Question | Resolution |
|---|---|---|---|
| Q-01 | Should `--capture` support a default frame (no number) or always require `N:path`? | **Always require N:path**. No default frame. `--capture /path` is an error. |
| Q-02 | Hardcoded scene list or runtime registry? | **Hardcoded string** in `main.cpp`/`app.cpp`, consistent with existing `k_usage_text` pattern. |
| Q-03 | `run_app()` as free function or static method? | **Free function** declared in `app.h`, defined in `app.cpp`. |
| Q-04 | Should frame 1 be skipped due to driver quirk? | **Skip frame 1**: force minimum capture frame to 2. If user requests `--capture 1:path`, silently capture frame 2 instead. Matches existing behavior. |
