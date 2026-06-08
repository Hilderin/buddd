# IMPL-2026-06-EDITOR — Editor Scaffolding

## Source spec

- `.specs/sprint-2026-06/editor-scaffolding/spec.md`

## Goal

Create the editor scaffolding: convert `buddd_editor` from an INTERFACE placeholder to a STATIC library with an `Editor` class skeleton, create an `EditorApp` subclass of `App` that drives the Editor via the existing App lifecycle, add a `buddd edit` CLI command that opens a 1280×800 ImGui-docked window, make ImGui init failure fatal in the display path of `RenderDevice::create()`, and add a headless unit test. Preserve all architecture boundaries (no SDL3/OpenGL/GLM outside `src/engine/`).

## Non-goals

- No editor panels, tools, scene hierarchy, inspector, or asset browser.
- No scene viewport rendering inside ImGui.
- No CLI flags for `buddd edit` as editor features. `--frame` and `--capture` work incidentally through `run_app()` but are not documented editor flags — they exist for debugging/testing only.
- No changes to existing `App` subclasses, `App` base class, or `run_app()` signature.
- No `#include` of SDL3, OpenGL, or GLM headers outside `src/engine/`.
- No changes to the root `CMakeLists.txt` (existing subdirectory order `engine → editor → cmd` is correct).
- No multi-viewport ImGui support.

## Relevant ADRs

| ADR | Relevance |
|---|---|---|
| ADR-027 (Editor Architecture) | Captures all editor architecture decisions: separate library, App lifecycle reuse, namespace, PIMPL, ImGui fatal init, CLI command, architecture boundary. Newly written for this feature. |
| ADR-019 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers outside `src/engine/`. The `src/editor/` and `src/cmd/apps/` directories must include zero such headers. |
| ADR-026 (Dear ImGui Integration) | Decision 2 ("Init failure is non-fatal") is amended by this spec: ImGui init failure in the display path is now **fatal** (error propagation instead of warning). The amendment must be reflected in ADR-026. `engine_imgui::is_initialized()` is the API the Editor calls to verify ImGui availability. |
| ADR-014 (CLI App System) | `EditorApp` follows the same `App` lifecycle pattern. `run_app()` is unchanged. |
| ADR-012 (Navigable Object Graph) | Editor accesses engine services through `EngineContext` only — no direct platform/window/device ownership. |
| ADR-001 (Result/Error Pattern) | All fallible APIs return `Result<void>` (`std::expected<void, Error>`). |

## Files to inspect

| File | Reason |
|---|---|
| `src/cmd/app.h` | Understand `App` base class virtual methods and `run_app()` signature — `EditorApp` must override identically. |
| `src/cmd/apps/imgui_demo_app.h` | Reference for an `App` subclass that uses ImGui in `on_render()`. |
| `src/cmd/apps/imgui_demo_app.cpp` | Reference for ImGui `#include` and `ImGui::*` usage pattern in an App subclass. |
| `src/cmd/apps/run_app.h` | Reference for minimal App subclass pattern (inline config/setup). |
| `src/cmd/main.cpp` | Existing command dispatch — `"edit"` must be inserted before the `if (cmd != "run")` unknown-command guard. |
| `src/cmd/CMakeLists.txt` | Add `buddd_editor` to `target_link_libraries`. `file(GLOB_RECURSE ... *.cpp)` already picks up `apps/editor_app.cpp` automatically. |
| `src/editor/CMakeLists.txt` | Currently an INTERFACE placeholder. Replace with a STATIC library definition. |
| `src/engine/render/render_device.cpp` | The ImGui init failure must be changed from non-fatal warning to error propagation. |
| `src/engine/imgui/engine_imgui.h` | The `is_initialized()` API that `Editor::setup()` calls to verify ImGui readiness. |
| `src/engine/engine_context.h` | `EngineContext` struct — Editor receives const ref to it in `setup()`. |
| `src/engine/error.h` | `Result<T>` / `Error` types used by all fallible APIs. |
| `tests/CMakeLists.txt` | Add `buddd_editor` link. GLOB already picks up `editor_tests.cpp`. |
| `tests/version_tests.cpp` | Reference for minimal Catch2 unit test pattern. |
| `docs/adr/ADR-026-imgui-integration.md` | Must be amended (Decision 2): change "Init failure is non-fatal" to fatal in display mode. |
| `docs/adr/ADR-027-editor-architecture.md` | New ADR — already written. Spec and contract reference it. |
| `docs/wiki/architecture/overview.md` | Update CMake targets table and directory layout for `buddd_editor` as STATIC lib. |
| `docs/wiki/architecture/module-map.md` | Add new `buddd_editor` module section. |
| `docs/wiki/architecture/dependency-map.md` | Add `buddd_editor → buddd_engine` dependency edge. |

## Files allowed to change

- `src/editor/CMakeLists.txt` — **modify** (INTERFACE → STATIC)
- `src/editor/editor.h` — **create**
- `src/editor/editor.cpp` — **create**
- `src/cmd/apps/editor_app.h` — **create**
- `src/cmd/apps/editor_app.cpp` — **create**
- `src/cmd/main.cpp` — **modify** (add `edit` dispatch branch + `#include "apps/editor_app.h"`)
- `src/cmd/CMakeLists.txt` — **modify** (add `buddd_editor` to `target_link_libraries`)
- `src/engine/render/render_device.cpp` — **modify** (change ImGui init failure from warning to error propagation)
- `tests/CMakeLists.txt` — **modify** (add `buddd_editor` to `target_link_libraries`)
- `tests/editor_tests.cpp` — **create**

## Files forbidden to change

- `src/cmd/app.h` — App interface must not change.
- `src/cmd/app.cpp` — `run_app()` must not change.
- `src/engine/engine_context.h` — Editor receives existing context.
- `src/engine/imgui/engine_imgui.h` — Public API unchanged.
- `src/engine/imgui/engine_imgui.cpp` — Implementation unchanged.
- `src/engine/CMakeLists.txt` — No build changes needed.
- Root `CMakeLists.txt` — Subdirectory order is already correct.
- Any existing `App` subclass file (triangle, cube, phong, etc.).
- `src/cmd/commands/help_command.h` — Help text update is optional, not required for AC.

## Existing conventions to follow

1. **Include style**: Use `#include "..."` for project headers, `<...>` for external/system headers. Paths are relative to `src/engine/` for engine headers, `src/cmd/` for cmd headers, `src/editor/` for editor headers.
2. **Namespace nesting**: `buddd::editor` for Editor, `buddd::cmd::app` for EditorApp. Use `namespace` blocks without indentation of content (project style).
3. **`#pragma once`**: All new headers must use `#pragma once` as the include guard.
4. **`[[nodiscard]]`**: All `Result<T>`-returning functions must be marked `[[nodiscard]]`.
5. **PIMPL pattern**: `EditorImpl` forward-declared inside `Editor` class as `struct EditorImpl;` with `std::unique_ptr<EditorImpl> impl_;` as sole private member. See existing PIMPL usage in `PhongMaterial` / `PbrMaterial`.
6. **App subclass pattern**: Header declares `class EditorApp final : public buddd::cmd::App`, implements all virtual overrides. See `run_app.h` and `imgui_demo_app.h` for pattern.
7. **CMake**: STATIC library definition uses `add_library(buddd_editor STATIC ...)` with PUBLIC include directory and PUBLIC link to `buddd_engine`.
8. **Test pattern**: Catch2 `TEST_CASE("name", "[tag]")` with `#include <catch2/catch_test_macros.hpp>`. See `tests/version_tests.cpp`.
9. **ImGui include**: Files that use `ImGui::*` functions include `<imgui.h>` directly (not through any engine wrapper). The `<imgui.h>` header is available because it's fetched via `FetchContent` and added to `buddd_engine`'s include path.
10. **Error construction**: Use `make_error(Error::Category::..., "message")` for returning `Result<void>` errors. See `src/engine/error.h`.
11. **Editor app file glob**: `src/cmd/CMakeLists.txt` already globs `apps/*.cpp` automatically. No explicit source file listing is needed for `editor_app.cpp`.

## Required implementation behavior

### Step 1: Engine change — Make ImGui init failure fatal (display path only)

**File**: `src/engine/render/render_device.cpp`

Change the `engine_imgui::init()` failure handling in the display branch (lines 50-53) from:

```cpp
auto imgui_result = engine_imgui::init(sdl_window, gl_context);
if (!imgui_result) {
    BUDDD_LOG_WARN("ImGui init failed (non-fatal): {}", to_string(imgui_result.error()));
}
```

To:

```cpp
if (auto imgui_result = engine_imgui::init(sdl_window, gl_context); !imgui_result) {
    return make_error(imgui_result.error());
}
```

- The `BUDDD_LOG_WARN` line is **removed** (the error will propagate and be logged by `run_app()` via its existing error-handling path).
- The headless path (the `if (native == nullptr)` block above) is **unchanged** — no ImGui init is called there.
- This change affects ALL apps running in display mode, not just the editor.

### Step 2: Editor library — `src/editor/CMakeLists.txt`

Replace the current INTERFACE definition with:

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

This makes `buddd_editor` a STATIC library that publicly links `buddd_engine` and exposes its source directory as a public include path.

### Step 3: Editor class — `src/editor/editor.h`

Declare in `namespace buddd::editor`:

```cpp
#pragma once

#include "error.h"

#include <memory>

namespace buddd::engine { struct EngineContext; }

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

- Forward-declare `EngineContext` — do NOT include `engine_context.h`. The `#include "error.h"` is needed for the `Result<void>` return type. `error.h` is provided by `buddd_engine` (via `target_include_directories` PUBLIC).
- Do NOT include `<imgui.h>` in this header.
- The PIMPL struct `EditorImpl` is forward-declared as private; the full definition lives in `editor.cpp`.

### Step 4: Editor implementation — `src/editor/editor.cpp`

Implement the four class members:

**`Editor::Editor()`**

```cpp
Editor::Editor() : impl_(std::make_unique<EditorImpl>()) {}
```

- `EditorImpl` is a minimal struct defined at file scope in `editor.cpp`:
  ```cpp
  struct Editor::EditorImpl {
      buddd::engine::EngineContext const* ctx = nullptr;
  };
  ```
- No heavy includes in the impl struct in v1. The ctx pointer is stored as a raw non-owning pointer.

**`Editor::setup(EngineContext const& ctx) -> Result<void>`**

1. Store `&ctx` in `impl_->ctx`.
2. Check `buddd::engine::engine_imgui::is_initialized()`. If `false`, return `make_error(Error::Category::InitFailed, "ImGui is not initialized. The editor requires a display with working ImGui.")`.
3. Return success (`return {};`).

**`Editor::draw_ui()`**

1. Guard: if `impl_` is null or `impl_->ctx` is null, return immediately (no-op).
2. Create an ImGui dockspace covering the full viewport:
   ```cpp
   ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
   ```
3. No panels, widgets, or UI elements are added in v1 — just the dockspace.

**`Editor::shutdown()`**

1. `impl_.reset();` — releases the `EditorImpl`. Safe to call multiple times (resetting a null unique_ptr is a no-op).
2. No ImGui shutdown — that is owned by `RenderDeviceOpenGL`.

**`Editor::~Editor()`**

- Default or explicitly `= default` in the `.cpp` file (where `EditorImpl` is complete). The destructor must be defined in the `.cpp` because `EditorImpl` is incomplete at the point of the header declaration.

**Includes needed in `editor.cpp`**:

```cpp
#include "editor/editor.h"

#include "engine_context.h"
#include "error.h"
#include "imgui/engine_imgui.h"

#include <imgui.h>
```

All these headers are provided by `buddd_engine` via its PUBLIC include directory. The include paths are relative to `src/engine/`.

### Step 5: EditorApp class — `src/cmd/apps/editor_app.h`

```cpp
#pragma once

#include "app.h"

#include <memory>

namespace buddd::editor { class Editor; }

namespace buddd::cmd::app {

/// App subclass that opens the editor window.
class EditorApp final : public buddd::cmd::App {
public:
    auto config() const -> buddd::cmd::AppConfig override;

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void> override;

    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

    auto shutdown() -> void override;

private:
    std::unique_ptr<buddd::editor::Editor> editor_;
};

} // namespace buddd::cmd::app
```

- Forward-declare `buddd::editor::Editor` — do NOT include `<editor/editor.h>` in the header. The include is in the `.cpp`.
- Do NOT include `<imgui.h>` in this header.
- No SDL3, OpenGL, or GLM headers.

### Step 6: EditorApp implementation — `src/cmd/apps/editor_app.cpp`

**`EditorApp::config()`**

```cpp
auto EditorApp::config() const -> AppConfig {
    return {"Buddd Editor", 1280, 800};
}
```

**`EditorApp::setup(EngineContext const& ctx) -> Result<void>`**

```cpp
auto EditorApp::setup(EngineContext const& ctx) -> Result<void> {
#ifndef BUDDD_HAS_DISPLAY
    return make_error(Error::Category::InitFailed,
        "editor requires a display (compiled with BUDDD_HAS_DISPLAY=OFF)");
#endif

    editor_ = std::make_unique<buddd::editor::Editor>();
    return editor_->setup(ctx);
}
```

- The `#ifndef BUDDD_HAS_DISPLAY` guard provides the compile-time headless check. This triggers before any engine init and provides a clear error message.
- `editor_->setup(ctx)` will also verify `engine_imgui::is_initialized()` as a belt-and-suspenders check. In headless mode, this code path is never reached (the `#ifndef` returns early). In display mode where ImGui init succeeded, it returns success. In the unlikely scenario where display mode compiles with `BUDDD_HAS_DISPLAY` but ImGui init was not called (should not happen after the fatal-init change), it catches that too.

**`EditorApp::on_render(EngineContext const& ctx)`**

```cpp
auto EditorApp::on_render(EngineContext const& ctx) -> void {
    if (editor_) {
        editor_->draw_ui();
    }
}
```

- No-op guard for safety (if setup was never called or failed).

**`EditorApp::shutdown()`**

```cpp
auto EditorApp::shutdown() -> void {
    if (editor_) {
        editor_->shutdown();
        editor_.reset();
    }
}
```

- Safe to call multiple times.

**Includes**:

```cpp
#include "apps/editor_app.h"

#include "editor/editor.h"
#include "error.h"

#ifdef BUDDD_HAS_DISPLAY
#include <imgui.h>
#endif
```

- `editor/editor.h` is the editor library header (included from `src/editor/` via the PUBLIC include directory of `buddd_editor`).
- `<imgui.h>` is only needed if the `on_render()` body uses ImGui symbols — in v1, `draw_ui()` is on the `Editor` object, not called directly from `EditorApp`, so the `#include <imgui.h>` is not strictly needed in this file. It is optional but harmless.
- No SDL3, OpenGL, or GLM includes.

### Step 7: CLI dispatch — `src/cmd/main.cpp`

1. Add `#include "apps/editor_app.h"` to the includes block (line 5–18 area, after existing app includes).

2. Add an `"edit"` command branch **before** the `if (cmd != "run")` guard (currently at line 68). Insert after the `version`/`help` blocks and before the `if (cmd != "run")` check:

```cpp
    if (cmd == "edit") {
        bc::app::EditorApp editor_app;
        return bc::run_app(editor_app, bc::RunningArgs{});
    }
```

- `RunningArgs{}` (zero-initialized) means interactive mode (no frame limit, no captures).
- No `parse_running_args()` call — `edit` takes no flags in v1.
- There is no `#ifdef BUDDD_HAS_DISPLAY` guard here in main.cpp — the headless rejection happens inside `EditorApp::setup()`.

### Step 8: `src/cmd/CMakeLists.txt`

Add `buddd_editor` to the `target_link_libraries` call:

```cmake
target_link_libraries(buddd PRIVATE buddd_engine buddd_editor)
```

### Step 9: `tests/CMakeLists.txt`

Add `buddd_editor` to `target_link_libraries` for `buddd_tests`:

```cmake
target_link_libraries(buddd_tests PRIVATE
    buddd_engine
    buddd_editor
    Catch2::Catch2WithMain
)
```

- The `file(GLOB_RECURSE ... *_tests.cpp)` will automatically pick up `editor_tests.cpp`.

### Step 10: Headless unit test — `tests/editor_tests.cpp`

```cpp
#include "editor/editor.h"
#include "engine_context.h"
#include "error.h"

#include <catch2/catch_test_macros.hpp>

// Forward declarations needed to construct a headless EngineContext
#include "engine_service.h"    // or relevant headers
#include "window/window.h"
#include "render/render_device.h"
// ... additional includes as needed for creating test context

TEST_CASE("Editor can be constructed, set up, and shut down headlessly", "[editor]") {
    // The editor test MUST run in headless mode. It constructs a minimal
    // EngineContext without a display.
    //
    // Strategy A (preferred if feasible): Create an EngineService in headless
    //   mode and extract a valid EngineContext from it. This matches how
    //   run_app() works and provides a real (headless) context.
    //
    // Strategy B (fallback): Build a minimal EngineContext struct on the
    //   stack with dummy references if headless EngineService creation is
    //   too heavy for this test.
    //
    // NOTE: The editor must NOT crash during setup or shutdown even when
    //       no ImGui is available (headless mode). Editor::setup() will
    //       return an error because engine_imgui::is_initialized() is false.
    //       That is acceptable — the test verifies crash-free lifecycle,
    //       not successful setup.
    //
    // [NEEDS CLARIFICATION] The exact test implementation depends on whether
    // a headless EngineService can be cheaply created. Two approaches:
    //
    // Approach A (preferred):
    //   auto engine = buddd::engine::EngineService::create(
    //       buddd::engine::Backend::Headless,
    //       buddd::engine::WindowConfig{"Editor Test", 128, 128});
    //   REQUIRE(engine);
    //   auto world = std::make_unique<buddd::engine::World>();
    //   auto render_system = std::make_unique<buddd::engine::RenderSystem>(
    //       engine->device(), *world);
    //   buddd::engine::EngineContext ctx{
    //       *engine, engine->window(), engine->device(),
    //       *world, *render_system, 0.016f, 0};
    //
    // Approach B (simpler if EngineService::create doesn't work in test):
    //   Create a struct with dangling references for the EngineContext — but
    //   this is risky and not recommended.
    //
    // The implementer should choose Approach A if EngineService::create()
    // works in headless mode without a display. If not, they must create
    // platform/window/device individually.

    buddd::editor::Editor editor;

    // Setup will likely fail (no ImGui in headless), but must not crash.
    // Both success and failure paths are valid outcomes for this test.
    auto result = editor.setup(ctx);  // ctx constructed per Approach A

    // Verify no crash: draw_ui is no-op (impl_ is null or ctx is null after failed setup)
    editor.draw_ui();

    // Shutdown: must be safe and idempotent
    editor.shutdown();
    editor.shutdown();  // second call must be a no-op
}
```

**Test requirements**:
- Tag the test case `[editor]`.
- The test must compile and link in both `BUDDD_HAS_DISPLAY=ON` and `BUDDD_HAS_DISPLAY=OFF` modes.
- The test must pass in headless mode (no display required).
- Verify crash-free constructor, `setup()`, `draw_ui()`, and `shutdown()` (including double `shutdown()`).
- The test must link `buddd_editor` (handled by `tests/CMakeLists.txt` change).

## Required tests

### Unit tests

| Test | File | What it verifies |
|---|---|---|
| Editor lifecycle | `tests/editor_tests.cpp` | `Editor` constructor, `setup()` with headless context, `draw_ui()` (no-op after failed setup), `shutdown()` (including double call). Tag: `[editor]`. Run unconditionally (no `#ifdef` guard). |
| CLI unknown command | `tests/cli_app_tests.cpp` | Existing tests already verify `buddd <unknown>` produces "Unknown command:" error. No new test needed — the `edit` branch does not change the unknown-command path. |

### E2E / Integration verification

| Method | What it verifies |
|---|---|
| **Manual (display)** | Run `buddd edit` → verify window appears with title "Buddd Editor", dimensions 1280×800, empty ImGui dockspace visible. Close window → verify exit code 0. |
| **Manual (headless)** | Build with `-DBUDDD_HAS_DISPLAY=OFF`, run `buddd edit` → verify stderr error message includes "editor requires a display" and exit code 1. |
| **Build verification** | `cmake --build --preset debug` succeeds (no compile/link errors). |
| **Existing scenes** | `buddd run triangle` and other scenes continue to work without regression. |

## Edge cases

| Case | Expected behavior | Handled by |
|---|---|---|
| Headless mode (`BUDDD_HAS_DISPLAY=OFF`) | `buddd edit` prints error to stderr and exits code 1. | `EditorApp::setup()` `#ifndef BUDDD_HAS_DISPLAY` guard |
| ImGui init fails (display but broken GPU) | `RenderDevice::create()` returns error → `run_app()` exits code 1 with error message. | Changed `render_device.cpp` — propagates error |
| `buddd edit` run multiple times | Each invocation is independent. No state leakage. | No global mutable state in Editor/EditorApp |
| Window closed immediately | `poll_events()` returns false → `run_app()` exits code 0. | Existing `run_app()` behavior |
| `buddd edit` with extra args | Extra args silently ignored. | `RunningArgs{}` default; no arg parsing |
| No display server (SSH, no X11) | SDL3 init fails → platform creation fails → `run_app()` prints error and exits code 1. | Existing platform error path |
| `Editor::setup()` called twice | Undefined behaviour (not a supported use case). No guard. | Implicit: caller responsibility |
| `Editor::draw_ui()` before `setup()` | No-op (impl_ is valid but ctx is null). | Guard in `draw_ui()` |
| `Editor::draw_ui()` after `shutdown()` | No-op (impl_ is null). | Guard in `draw_ui()` |
| `Editor::shutdown()` before `setup()` | No-op (impl_ is null reset). | Safe: `reset()` on null is no-op |
| `Editor::shutdown()` called twice | No-op on second call. | Safe: `reset()` on null is no-op |
| Editor window resized | ImGui dockspace fills viewport automatically. | `ImGui::DockSpaceOverViewport()` handles this |
| `buddd edt` (typo) | Falls through to unknown-command handler. | Existing `if (cmd != "run")` guard |

## Security impact

None. The editor makes no network calls, creates no files, and reads no user data in v1. No elevated privileges required.

## Data and migration impact

None. No schema changes, data files, or persistent state in v1.

## API compatibility impact

- `EDITOR_API`: `buddd_editor` is a new STATIC library. Public API is `Editor` class in `namespace buddd::editor`. No existing API is modified.
- `CMDLINE_API`: New CLI command `buddd edit`. No existing command semantics change.
- `ENGINE_API`: `RenderDevice::create()` now returns an error on ImGui init failure instead of a warning. This is a behavioral change in the display path. All existing callers that assume non-fatal ImGui init will now see a hard failure. However, `run_app()` already handles `Result` return from `RenderDevice::create()` by printing the error and exiting with code 1, so no `run_app()` change is needed.
- `IMGUI_API`: `engine_imgui::is_initialized()` public API is unchanged.
- `APP_API`: `App` base class is unchanged. `EditorApp` is a new subclass.

## Documentation impact

- **Wiki pages to update**: The wiki-agent handles these, but the implementer must know what changes are expected:
  - `docs/wiki/architecture/overview.md`: Update CMake targets table (`buddd_editor` → STATIC, add source files column), update directory layout (`src/editor/` description → "Editor library (static lib)"). Add `buddd edit` to Key behaviors.
  - `docs/wiki/architecture/module-map.md`: Add new section `## buddd_editor` with file descriptions (`editor.h`, `editor.cpp`).
  - `docs/wiki/architecture/dependency-map.md`: Change `buddd_editor` from "(standalone, no dependencies)" to `buddd_editor ──PUBLIC──► buddd_engine`, update table row.
- **ADR impact**: `docs/adr/ADR-026-imgui-integration.md` — Decision 2 clause "Init failure is non-fatal" must be amended to "Init failure is fatal in display mode." The adr-agent handles this; the implementer must NOT modify ADR files.
- **Help command** (`help_command.h`): Optionally add `edit` to the commands list. Not required for AC but recommended for polish.

## ADR impact

- **ADR-027**: Newly written — captures editor architecture. Already created.
- **ADR-026**: Decision 2 changes from "Init failure is non-fatal" to "Init failure is fatal in display mode." This is noted here; the adr-agent will produce the actual amendment. The implementer must NOT modify ADR-026 directly.

## Done criteria

The implementation is complete when ALL of the following are verifiable:

- [ ] **AC-001**: `src/editor/editor.h` exists and declares `class Editor` in `namespace buddd::editor` with `setup(EngineContext const&) -> Result<void>`, `draw_ui() -> void`, and `shutdown() -> void`.
- [ ] **AC-002**: `src/editor/editor.h` uses PIMPL: private `struct EditorImpl` and `std::unique_ptr<EditorImpl>`.
- [ ] **AC-003**: `src/editor/CMakeLists.txt` defines `buddd_editor` as a STATIC library and links `buddd_engine` as PUBLIC.
- [ ] **AC-004**: `src/editor/CMakeLists.txt` sets public include directory to `${CMAKE_CURRENT_SOURCE_DIR}`.
- [ ] **AC-005**: `src/cmd/apps/editor_app.h` exists and declares `class EditorApp` inheriting `buddd::cmd::App`.
- [ ] **AC-006**: `EditorApp::config()` returns `AppConfig` with title "Buddd Editor", width 1280, height 800.
- [ ] **AC-007**: `EditorApp::setup()` creates an `Editor` and calls `editor_->setup(ctx)`.
- [ ] **AC-008**: `EditorApp::on_render()` calls `editor_->draw_ui()`.
- [ ] **AC-009**: `EditorApp::shutdown()` calls `editor_->shutdown()`.
- [ ] **AC-010**: `src/cmd/main.cpp` has a dispatch branch for the `"edit"` command that creates an `EditorApp` and calls `run_app()`.
- [ ] **AC-011**: `src/cmd/CMakeLists.txt` links `buddd_editor` to `buddd`.
- [ ] **AC-012**: `src/engine/render/render_device.cpp` propagates `engine_imgui::init()` error instead of logging a warning in the display path.
- [ ] **AC-013**: `buddd edit` opens a 1280×800 window titled "Buddd Editor" with ImGui dockspace visible (manual verification).
- [ ] **AC-014**: `buddd edit` exits cleanly with code 0 when the window is closed (manual verification).
- [ ] **AC-015**: `buddd edit` fails with error message when `BUDDD_HAS_DISPLAY=OFF` (manual verification).
- [ ] **AC-016**: No SDL3, OpenGL, or GLM headers are included from any file under `src/editor/`. Run: `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/` — zero matches.
- [ ] **AC-017**: No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/apps/editor_app.*`. Run: `grep -rnE '#include.*(SDL3|GL/|glm/)' src/cmd/apps/editor_app.*` — zero matches.
- [ ] **AC-018**: `Editor::setup()` returns an error if `engine_imgui::is_initialized()` returns `false`.
- [ ] **AC-019**: `Editor::draw_ui()` creates an ImGui dockspace via `ImGui::DockSpaceOverViewport()`.
- [ ] **AC-020**: `Editor::draw_ui()` is a no-op if `setup()` was never called (null impl_ or null ctx pointer).
- [ ] **AC-021**: `Editor::shutdown()` is safe to call multiple times (check for null reset guard).
- [ ] **AC-022**: Build succeeds with `cmake --build --preset debug` after all changes.
- [ ] **AC-023**: `buddd edt` (typo) produces "Unknown command: 'edt'" error and exits with code 1 (existing behavior unchanged).
- [ ] **TEST-001**: `tests/editor_tests.cpp` exists, compiles, and links in both display and headless modes.
- [ ] **TEST-002**: Editor test passes without a display (headless CI) — no crash on `Editor` construction, `setup()`, `draw_ui()`, or `shutdown()`.
- [ ] **TEST-003**: All existing tests continue to pass: `ctest --preset debug`.
- [ ] **TEST-004**: All existing `App` subclasses continue to build and run: `buddd run triangle --frame 2` works correctly.
- [ ] **SC-002**: No SDL3, OpenGL, or GLM headers outside `src/engine/` after all changes: `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/ src/cmd/` — zero matches.
