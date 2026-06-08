# SPEC-NNNN — Editor Scaffolding

## Problem

The `buddd_editor` CMake target is currently an INTERFACE library placeholder with zero source files. There is no reusable editor library, no `Editor` class, no way to launch an editor from the CLI, and no skeleton for future editor panels. Although the engine now has ImGui integration (SPEC-026 / ADR-026), every editor feature would need to start from scratch — there is no foundation to build upon. Without this scaffold, any editor development would require each contributor to duplicate the same boilerplate (window creation, ImGui dockspace setup, editor lifecycle management). See ADR-027 for the full architecture rationale (separate library, App lifecycle reuse, ImGui fatal init, etc.).

Additionally, ImGui init failure in the SDL3/display path is currently non-fatal (logged warning, engine continues). For an editor that depends on ImGui, a non-fatal init failure produces a broken editor with no visible UI. Making ImGui init failure fatal in display mode ensures all ImGui-dependent features fail early rather than silently producing a broken experience.

## Goals

- **G-01**: Convert `buddd_editor` from an INTERFACE library to a STATIC library with its own CMake target and source files, linking `buddd_engine`.
- **G-02**: Create an `Editor` class skeleton in `namespace buddd::editor` with a minimal lifecycle: constructor, `setup(EngineContext const&)`, `draw_ui()`, `shutdown()`.
- **G-03**: Create an `EditorApp` subclass of `App` in `src/cmd/apps/` that creates and drives an `Editor` instance via the `App` lifecycle (`config()` → `setup()` → `on_render()` → `shutdown()`).
- **G-04**: Add a `buddd edit` CLI command that opens a 1280×800 "Buddd Editor" window with an ImGui dockspace.
- **G-05**: Render an ImGui dockspace placeholder (empty, ready for future panels) in the editor window.
- **G-06**: Change ImGui init failure from non-fatal to fatal: `RenderDevice::create()` returns an error if `engine_imgui::init()` fails in the display path.
- **G-07**: Preserve the architecture boundary (ADR-019): no SDL3, OpenGL, or GLM headers outside `src/engine/`.
- **G-08**: Update wiki documentation: `overview.md`, `module-map.md`, `dependency-map.md`.

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | No editor panels, tools, scene hierarchy, inspector, or asset browser. |
| NG-02 | No scene viewport rendering inside ImGui. |
| NG-03 | No editor-specific file formats or project management. |
| NG-04 | No gizmos, selection, undo/redo. |
| NG-05 | `--frame` and `--capture` are NOT editor features in v1. They happen to work because `EditorApp` uses `run_app()` which parses them, but they exist only for debugging/testing, not as documented editor flags. |
| NG-06 | No changes to existing `App` subclasses (triangle, cube, phong, etc.). |
| NG-07 | No dynamic discovery or plugin loading. |
| NG-08 | No changes to existing `run_app()` or `App` base class signature. |

This spec records the detailed functional decisions. The architectural rationale is captured in **ADR-027** (Editor Architecture). This spec amends **ADR-026** (Dear ImGui Integration) — specifically Decision 2's "Init failure is non-fatal" clause, which is changed to fatal in display mode.

## Actors

| Actor | Description |
|---|---|
| **Developer** | A human running the `buddd` CLI binary. Uses `buddd edit` to open the editor window. |
| **Editor developer** | A developer who adds editor features by creating new ImGui panels inside `Editor::draw_ui()`, extending the `Editor` class, or adding new editor subdirectories. |

## User-visible behavior

### `buddd edit`

Opens a 1280×800 window titled "Buddd Editor" with an interactive ImGui dockspace. Runs until the user closes the window. The editor does not respond to Escape — only window close exits. `--frame` and `--capture` flags happen to work through `run_app()` but are not editor features (they exist for debugging/testing only).

**Behavior details**:
- Platform is created with `BUDDD_HAS_DISPLAY` compile-time define. When `OFF` (headless), `buddd edit` prints an error and exits with code 1 — the editor requires a display.
- Window dimensions: 1280×800.
- Window title: "Buddd Editor".
- The editor window renders an ImGui dockspace node (`ImGui::DockSpaceOverViewport`) as the root UI element. No panels, tools, or widgets are visible — just the empty dockspace.
- The editor runs interactively until window close (no Escape handler — the editor is not a demo scene).
- Exit code: 0 on clean exit (window closed by user, no errors). Non-zero on error.

### Unknown command

`buddd edit` is a known command. `buddd edt` (typo) falls through to existing unknown-command handling (stderr: "Unknown command: 'edt'", exit code 1).

## Key entities

### `Editor` class (`src/editor/editor.h`)

```cpp
namespace buddd::editor {

/// Scaffold for the Buddd Editor.
/// Lifecycle: Editor() → setup() → draw_ui() x N → shutdown().
class Editor {
public:
    Editor();
    ~Editor();

    /// Store const engine context reference for UI drawing.
    /// Called from EditorApp::setup(). Returns error if ImGui is not initialized.
    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void>;

    /// Draw the ImGui dockspace and any active editor panels.
    /// Called every frame from EditorApp::on_render(). No-op if setup() was not called.
    auto draw_ui() -> void;

    /// Cleanup. Called from EditorApp::shutdown().
    auto shutdown() -> void;

private:
    struct EditorImpl;
    std::unique_ptr<EditorImpl> impl_;
};

} // namespace buddd::editor
```

- `Editor()` constructor creates a default-constructed PIMPL (`EditorImpl`).
- `setup(EngineContext const&)` stores the const engine context reference. Returns an error if `engine_imgui::is_initialized()` returns `false` (ImGui not available).
- `draw_ui()` creates a full-viewport ImGui dockspace node. No-op if `setup()` was not called or if `impl_` is null.
- `shutdown()` releases the PIMPL. Safe to call multiple times.
- `~Editor()` calls `shutdown()`.

### `EditorApp` class (`src/cmd/apps/editor_app.h`)

```cpp
namespace buddd::cmd::app {

/// App subclass that opens the editor window.
class EditorApp final : public buddd::cmd::App {
public:
    auto config() const -> buddd::cmd::AppConfig override;

    /// Creates the Editor and calls Editor::setup().
    /// Returns error if Editor::setup() fails.
    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    /// Calls Editor::draw_ui() each frame.
    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

    /// Calls Editor::shutdown().
    auto shutdown() -> void override;

private:
    std::unique_ptr<buddd::editor::Editor> editor_;
};

} // namespace buddd::cmd::app
```

- `config()` returns `AppConfig{.title = "Buddd Editor", .width = 1280, .height = 800}`.
- `setup()` creates the `Editor` via `std::make_unique<Editor>()`, then calls `editor_->setup(ctx)`. If setup fails, the error is propagated and the app exits (via `run_app()` error handling).
- `on_render()` calls `editor_->draw_ui()`.
- `shutdown()` calls `editor_->shutdown()`.

### CMake changes

**`src/editor/CMakeLists.txt`** — changed from INTERFACE to STATIC library:

```cmake
add_library(buddd_editor STATIC
    editor.cpp
)

target_link_libraries(buddd_editor
    PUBLIC
        buddd_engine
)

target_include_directories(buddd_editor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
```

**`src/cmd/CMakeLists.txt`** — add `buddd_editor` link:

```cmake
target_link_libraries(buddd
    PRIVATE
        buddd_engine
        buddd_editor        # NEW
)
```

**Root `CMakeLists.txt`** — no change needed. The existing order is:
```
add_subdirectory(src/engine)
add_subdirectory(src/editor)
add_subdirectory(src/cmd)
```
This already satisfies the build order (editor depends on engine, cmd depends on both).

### ImGui init change (engine-level)

**File**: `src/engine/render/render_device.cpp`

**Current behavior** (display branch, after creating `RenderDeviceOpenGL`):
```
if (auto result = engine_imgui::init(sdl_window, gl_context); !result) {
    BUDDD_LOG_WARN(...);  // non-fatal warning, continue
}
```

**New behavior**:
```
if (auto result = engine_imgui::init(sdl_window, gl_context); !result) {
    return make_error(result.error());  // fatal: propagate error up
}
```

- Headless path is unaffected (ImGui init is not called).
- This change affects all apps running in display mode, not just the editor.
- ADR-026 Decision 2 must be amended to reflect this change. See ADR-027 for full architecture context.

## File changes

### Created

| File | Purpose |
|---|---|
| `src/editor/editor.h` | `Editor` class declaration (`namespace buddd::editor`). |
| `src/editor/editor.cpp` | `Editor` implementation (PIMPL, dockspace UI). |
| `src/editor/CMakeLists.txt` | Converted to STATIC library, links `buddd_engine`. |
| `src/cmd/apps/editor_app.h` | `EditorApp` class declaration. |
| `src/cmd/apps/editor_app.cpp` | `EditorApp` implementation. |

### Modified

| File | Change |
|---|---|
| `src/cmd/main.cpp` | Add `"edit"` dispatch branch: create `EditorApp`, call `run_app()`. |
| `src/cmd/CMakeLists.txt` | Add `buddd_editor` to `target_link_libraries`. |
| `src/engine/render/render_device.cpp` | Make ImGui init failure fatal in display path (error propagation instead of warning). |
| `tests/CMakeLists.txt` | Link `buddd_editor` to `buddd_tests` for headless unit test. |
| `docs/adr/ADR-026-imgui-integration.md` | Amend Decision 2: change "Init failure is non-fatal" to fatal. |
| `docs/wiki/architecture/overview.md` | Update CMake targets table (buddd_editor is now STATIC), update directory layout. |
| `docs/wiki/architecture/module-map.md` | Add `buddd_editor` module section with file descriptions. |
| `docs/wiki/architecture/dependency-map.md` | Add `buddd_editor → buddd_engine` edge in dependency diagram and table. |

### Unchanged

| File | Reason |
|---|---|
| `src/cmd/app.h` | `App` interface unchanged. No new virtual methods. |
| `src/cmd/app.cpp` | `run_app()` unchanged. Editor uses existing lifecycle. |
| `src/engine/engine_context.h` | `EngineContext` unchanged. Editor receives full context. |
| `src/engine/imgui/engine_imgui.h/.cpp` | Public API unchanged. Only init behaviour in `render_device.cpp` changes. |
| All existing `App` subclasses | No changes to existing scenes. |
| `src/cmd/commands/help_command.h/.cpp` | May need help text update (optional for v1). |

## User stories

### Story 1 — Editor library compiles and links (Priority: P1)

As a developer, I want `buddd_editor` to compile as a static library that links `buddd_engine`, so that the editor foundation is buildable.

**Given** the project is configured with CMake
**When** I run `cmake --build --preset debug`
**Then** the build succeeds and produces `libbuddd_editor.a` (or `.lib`) linked into the `buddd` binary.

### Story 2 — Launch editor from CLI (Priority: P1)

As a developer, I want to run `buddd edit` and see an editor window, so that I can verify the editor scaffold works.

**Given** the `buddd` binary is compiled with display support (`BUDDD_HAS_DISPLAY=ON`)
**When** I run `buddd edit`
**Then** a window opens (1280×800, title "Buddd Editor") with an ImGui dockspace visible, and closes cleanly when I close the window, exiting with code 0.

### Story 3 — Headless mode rejection (Priority: P2)

As a developer running in headless CI, I want `buddd edit` to fail immediately with a clear error, so that I don't waste time debugging a display-dependent feature.

**Given** the `buddd` binary is compiled without display support (`BUDDD_HAS_DISPLAY=OFF`)
**When** I run `buddd edit`
**Then** an error message is printed to stderr (e.g., "Error: editor requires a display") and the process exits with code 1.

### Story 4 — ImGui init failure is fatal (Priority: P1)

As a developer, I want a broken ImGui init in display mode to cause a hard failure, so that I catch configuration issues early rather than debugging a blank window.

**Given** the `buddd` binary is compiled with display support
**When** ImGui initialization fails (e.g., missing OpenGL context)
**Then** `RenderDevice::create()` returns an error, the app prints the error to stderr, and exits with code 1 — for any app, not just the editor.

### Story 5 — Unknown command fallthrough (Priority: P2)

As a developer, I want `buddd edt` (typo) to produce an unknown command error, so that I know the command doesn't exist.

**Given** the `buddd` binary is compiled
**When** I run `buddd edt`
**Then** stderr contains "Unknown command: 'edt'" and the process exits with code 1.

### Story 6 — Headless unit test (Priority: P2)

As an editor developer, I want a unit test that constructs an `Editor`, calls `setup()` and `shutdown()` in headless mode, so that memory management is verified without a display.

**Given** a headless engine context (compiled with `BUDDD_HAS_DISPLAY=OFF`)
**When** I construct an `Editor`, call `setup()`, and call `shutdown()`
**Then** no crash, no memory error occurs (verified by sanitizer in CI).

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `src/editor/editor.h` exists and declares `class Editor` in `namespace buddd::editor` with `setup(EngineContext const&) -> Result<void>`, `draw_ui() -> void`, and `shutdown() -> void`. | File exists; inspect declaration. |
| AC-002 | `src/editor/editor.h` uses PIMPL: private `struct EditorImpl` and `std::unique_ptr<EditorImpl>`. | Inspect file for forward-declared implementation struct and unique_ptr member. |
| AC-003 | `src/editor/CMakeLists.txt` defines `buddd_editor` as a STATIC library and links `buddd_engine` as PUBLIC. | Inspect file; verify `add_library(buddd_editor STATIC ...)` and `target_link_libraries(buddd_editor PUBLIC buddd_engine)`. |
| AC-004 | `src/editor/CMakeLists.txt` sets public include directory to `${CMAKE_CURRENT_SOURCE_DIR}`. | Inspect file; verify `target_include_directories(buddd_editor PUBLIC ...)`. |
| AC-005 | `src/cmd/apps/editor_app.h` exists and declares `class EditorApp` inheriting `buddd::cmd::App`. | File exists; inspect class declaration. |
| AC-006 | `EditorApp::config()` returns `AppConfig` with title "Buddd Editor", width 1280, height 800. | Inspect implementation; verify returned AppConfig fields. |
| AC-007 | `EditorApp::setup()` creates an `Editor` and calls `editor_->setup(ctx)`. | Inspect implementation; verify Editor construction and setup call. |
| AC-008 | `EditorApp::on_render()` calls `editor_->draw_ui()`. | Inspect implementation; verify draw_ui() is called. |
| AC-009 | `EditorApp::shutdown()` calls `editor_->shutdown()`. | Inspect implementation; verify Editor shutdown is called. |
| AC-010 | `src/cmd/main.cpp` has a dispatch branch for the `"edit"` command that creates an `EditorApp` and calls `run_app()`. | Inspect main.cpp; verify `else if (command == "edit")` or equivalent branch. |
| AC-011 | `src/cmd/CMakeLists.txt` links `buddd_editor` to `buddd`. | Inspect file; verify `target_link_libraries(buddd PRIVATE ... buddd_editor)`. |
| AC-012 | `src/engine/render/render_device.cpp` propagates `engine_imgui::init()` error instead of logging a warning in the display path. | Inspect render_device.cpp; verify the init call returns the error instead of falling through. |
| AC-013 | `buddd edit` opens a 1280×800 window titled "Buddd Editor" with ImGui dockspace visible. | Manual: run `buddd edit`, verify window size and title, verify ImGui dockspace (empty) is rendered. |
| AC-014 | `buddd edit` exits cleanly with code 0 when the window is closed. | Run `buddd edit`, close window, verify `echo $?` returns 0. |
| AC-015 | `buddd edit` fails with error message when `BUDDD_HAS_DISPLAY=OFF` (headless). | Build with `-DBUDDD_HAS_DISPLAY=OFF`, run `buddd edit`, verify stderr error message and exit code 1. |
| AC-016 | No SDL3, OpenGL, or GLM headers are included from any file under `src/editor/`. | Run `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/` — zero matches. |
| AC-017 | No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/apps/editor_app.*`. | Run `grep -rnE '#include.*(SDL3|GL/|glm/)' src/cmd/apps/` — zero matches. |
| AC-018 | `Editor::setup()` returns an error if `engine_imgui::is_initialized()` returns `false`. | Inspect `editor.cpp`; verify the check and error return for missing ImGui. |
| AC-019 | `Editor::draw_ui()` creates an ImGui dockspace via `ImGui::DockSpaceOverViewport()`. | Inspect `editor.cpp`; verify dockspace creation call. |
| AC-020 | `Editor::draw_ui()` is a no-op if `setup()` was never called (null impl_). | Inspect `editor.cpp`; verify guard clause. |
| AC-021 | `Editor::shutdown()` is safe to call multiple times. | Inspect `editor.cpp`; verify idempotency (null check, reset). |
| AC-022 | Build succeeds with `cmake --build --preset debug` after all changes. | Run `cmake --build --preset debug`; verify no compile or link errors. |
| AC-023 | `buddd edt` (typo) produces "Unknown command: 'edt'" error and exits with code 1. | Run `buddd edt`; verify stderr message and exit code. |

## E2E Verification

| Method | Description |
|---|---|
| **Manual (display)** | Run `buddd edit`, verify window title ("Buddd Editor") and dimensions (1280×800), verify ImGui dockspace is visible (empty docking grid), close window, verify clean exit (code 0). |
| **Headless unit test** | Build with `BUDDD_HAS_DISPLAY=OFF`. Link `buddd_editor` to `buddd_tests`. Write a test guarded by `#ifndef BUDDD_HAS_DISPLAY` (or the appropriate compile-time check): create `Editor`, call `setup()` with a headless `EngineContext`, call `shutdown()`. Verify no crash, no memory error (ASan/UBSan in CI). `draw_ui()` is not called in this test. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A new editor panel can be added by adding an `ImGui::Begin()`/`End()` block inside `Editor::draw_ui()`, without modifying engine code, CMake files, or the `EditorApp`. | Developer creates a minimal "Hello World" panel in `draw_ui()`, rebuilds, runs `buddd edit`, and sees the panel rendered in the dockspace. |
| SC-002 | No SDL3, OpenGL, or GLM headers are included outside `src/engine/` after all changes. | `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/ src/cmd/` — zero matches. |
| SC-003 | The editor window opens in under 3 seconds on a development machine with display. | Measure wall-clock time from `buddd edit` invocation to window appearing. |
| SC-004 | All existing `App` subclasses (triangle, cube, phong, etc.) continue to build and run unchanged. | Build `--preset debug` succeeds; `buddd run triangle` renders correctly. |

## Edge cases

| Case | Expected behavior |
|---|---|
| Headless mode (`BUDDD_HAS_DISPLAY=OFF`) | `buddd edit` prints error to stderr and exits with code 1. |
| ImGui init fails (display mode, e.g., broken GPU driver) | `RenderDevice::create()` returns an error, `run_app()` exits with code 1, error message printed to stderr. This applies to ALL apps, not just editor. |
| `buddd edit` run multiple times sequentially | Each invocation opens a fresh editor window, runs independently, closes cleanly. No state leakage between invocations. |
| Editor window closed immediately after opening | `poll_events()` returns false on first frame, `run_app()` exits cleanly with code 0. |
| `buddd edit --frame 5 --capture 5:out.png` | Works through `run_app()` infrastructure but is not a documented editor feature. Exists for debugging/testing. |
| `buddd edit --foo` (unknown flag) | `run_app()` keeps strict argument parsing — this will likely produce a parse error or warning, consistent with how other apps handle unknown flags. |
| `buddd edit` without display server (e.g., SSH session without X11) | SDL3 init fails → platform creation failure → `run_app()` prints error and exits with code 1. |
| `Editor::setup()` called twice | Undefined behaviour (not a supported use case). Each Editor instance should be set up once. |
| `Editor::draw_ui()` called before `setup()` | No-op (guarded by null impl_ check). |
| `Editor::draw_ui()` called after `shutdown()` | No-op (impl_ is null after shutdown). |
| `Editor::shutdown()` called before `setup()` | No-op (impl_ is null). Safe to call. |
| Editor window resized | ImGui dockspace fills the viewport automatically (DockSpaceOverViewport tracks window size). |

## Error cases

| Case | Expected behavior |
|---|---|
| Headless mode (BUDDD_HAS_DISPLAY=OFF) | Print to stderr: "Error: editor requires a display (compiled with BUDDD_HAS_DISPLAY=OFF)". Exit code 1. |
| ImGui init failure (display mode) | `RenderDevice::create()` returns error with category `InitFailed`. `run_app()` prints error to stderr. Exit code 1. |
| Platform creation failure | `run_app()` handles this: error printed to stderr. Exit code 1. |
| Window creation failure | `run_app()` handles this: error printed to stderr. Exit code 1. |
| `Editor::setup()` returns error | `EditorApp::setup()` propagates the error to `run_app()`. Render loop is skipped, `shutdown()` is called. Exit code 1. |

## Permissions and security

- The editor binary requires no elevated privileges (root/admin) to run.
- No network access is required at runtime.
- No secrets, credentials, or environment variables are consumed.
- The architecture boundary CONST-001 (ADR-019) is preserved: no SDL3, OpenGL, or GLM headers are included from `src/editor/` or `src/cmd/`. All platform and graphics access goes through engine abstractions (`EngineContext`, `EngineService`, `Window`, `RenderDevice`).
- ImGui uses file-scope static state in `engine_imgui.cpp` (already documented in ADR-026 as an accepted exception).
- The editor does not create files on disk or access user data in v1.

## Observability

| Signal | Source |
|---|---|
| Editor window opened | stdout: `"Window opened: 1280x800"` (from `run_app()`) |
| Editor running | stderr: `"Scene started: (interactive)"` (from `run_app()` — scene is the EditorApp) |
| Editor shutdown | stdout: `"Window closed, shutting down."` (from `run_app()`) |
| Editor setup error | stderr: error description from `Result<>` (e.g., "ImGui not initialized") |
| Headless mode error | stderr: `"Error: editor requires a display (compiled with BUDDD_HAS_DISPLAY=OFF)"` |
| Exit code | Shell variable `$?` after process exits |

## Out of scope

- Editor panels, tools, scene hierarchy view, inspector, asset browser.
- Scene viewport rendering inside ImGui.
- Editor-specific file formats or project management (`.buddd` project files, etc.).
- Gizmos, selection, undo/redo.
- CLI flags for `buddd edit` (`--frame`, `--capture`, `--scene`, etc.).
- Changes to existing `App` subclasses or demo scenes.
- Dynamic discovery or plugin loading for editor panels.
- Tab-completion or shell integration.
- Multi-viewport ImGui support (`ImGuiConfigFlags_ViewportsEnable`).
- Theme customisation or editor colour scheme.
- Mouse/keyboard input filtering for ImGui capture (deferred to future work).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `App` base class and `run_app()` function are unchanged by this spec. The editor uses the existing lifecycle via `EditorApp`. |
| A-02 | `engine_imgui::is_initialized()` returns `true` after successful init and `false` otherwise. The Editor checks this in `setup()`. |
| A-03 | C++26 standard library features (`std::unique_ptr`, `std::expected` via `Result`) are available. |
| A-04 | The root `CMakeLists.txt` already adds subdirectories in the correct order (`engine` → `editor` → `cmd`). No change needed. |
| A-05 | The existing `src/cmd/CMakeLists.txt` glob pattern for `apps/*.cpp` picks up `editor_app.cpp` automatically. |
| A-06 | At compile time, `BUDDD_HAS_DISPLAY=OFF` selects the headless backend. The editor checks `#ifdef BUDDD_HAS_DISPLAY` or the equivalent to determine if display is available. |
| A-07 | The help text in `help_command.cpp` may optionally include `edit` in the command list. This is a minor cosmetic update, not required for AC. |
| A-08 | ImGui docking branch features (`DockSpaceOverViewport`) are available. The engine already fetches the docking branch of Dear ImGui (`v1.91.8-docking`). |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | Should `buddd edit` support `--scene` or other flags in v1? | **No flags in v1.** Interactive-only. Future flags are deferred. |
| Q-02 | How should the headless-mode error be detected? | Compile-time `#ifdef BUDDD_HAS_DISPLAY` (or equivalent CMake define). If display is off, `EditorApp::setup()` returns an error. |
| Q-03 | Should `Editor::setup()` use `EngineContext&` (non-const) or `EngineContext const&`? | **`EngineContext const&`** (const ref). `request_exit()` is already const (mutable flag), so const ref is sufficient and avoids the override signature mismatch with `App::setup(EngineContext const&)`. |
| Q-04 | Is the ImGui dockspace initially empty or should it show a "Welcome" panel? | **Empty.** No welcome panel, no placeholder text. Just the bare dockspace. A "[Welcome / Getting Started]" panel could be added later as a feature. |
| Q-05 | Should `Editor::setup()` fail if ImGui is not initialized, or silently succeed with draw_ui() as no-op? | **Fail.** If ImGui is not initialized (which should only happen in headless mode now that init failure is fatal), the Editor cannot function. `setup()` returns an error. |

All questions are resolved. No `[NEEDS CLARIFICATION]` markers remain.
