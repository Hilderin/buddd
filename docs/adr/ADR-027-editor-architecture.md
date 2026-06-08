# ADR-027: Editor Architecture — Separate Library with App Lifecycle

## Status

`Accepted`

## Context

The `buddd_editor` CMake target was an INTERFACE library placeholder with zero source files — a nameplate with no substance. There was no reusable editor library, no `Editor` class, no way to launch an editor from the CLI, and no skeleton for future editor panels. Although the engine gained ImGui integration (ADR-026), every editor feature would have needed to start from scratch: window creation, ImGui dockspace setup, editor lifecycle management.

The following constraints shaped the decisions:

- **ADR-019 (Architecture Boundaries)**: No SDL3, OpenGL, or GLM headers may appear outside `src/engine/`. Any editor code outside the engine must respect this boundary.
- **ADR-014 (CLI App System)**: The `App` base class and `run_app()` centralised render loop exist and are the single entry point for all display-mode applications. A new editor should reuse this infrastructure rather than duplicate it.
- **ADR-026 (Dear ImGui Integration)**: Decision 2 states "Init failure is non-fatal" — ImGui init failure logs a warning and continues. For the editor (and all display-mode apps that depend on ImGui), this produces a broken experience with no visible UI.
- **Precedent from `src/cmd/`**: The CLI command library (`src/cmd/`) is a separate library that links `buddd_engine` and respects ADR-019. The editor should follow the same pattern.

### Alternatives considered

| Alternative | Verdict |
|---|---|
| **Editor inside engine library** — Add Editor class and all editor infrastructure to `src/engine/`. | **Rejected.** Violates separation of concerns. The engine is a graphics/core library; the editor is a user-facing application. Embedding editor code in the engine would increase engine surface area, coupling unrelated systems, and make the editor harder to test independently. Every downstream consumer of `buddd_engine` would carry editor code. |
| **Editor as separate executable** — Create a standalone `buddd_edit` binary with its own `main()`, build system, and entry point. | **Rejected.** Would duplicate CLI infrastructure (argument parsing, platform initialisation, error handling). Developers would need to build and run a different binary. Build complexity increases (third target, additional CI steps). The existing `buddd` binary already has all the infrastructure; a new command is ~5 lines of dispatch code. |
| **Editor as part of `src/cmd/`** — Add Editor class directly into `src/cmd/` (the CLI binary library). | **Rejected.** Would couple editor implementation to CLI dispatch. `src/cmd/` is the command-line interface layer — it should dispatch to reusable libraries, not own application logic. Editor code would be mixed with argument parsing and command dispatch, making both harder to maintain. |
| **ImGui init non-fatal** (current ADR-026 behavior) — Keep the warning-and-continue pattern. | **Rejected for editor context.** A non-fatal init failure produces a running application with no ImGui UI. The user sees a blank window with no indication of why the UI is missing. For interactive applications like the editor, this is worse than a hard failure — the developer wastes time debugging "why is my UI not showing" instead of "why did ImGui init fail." |
| **Editor exits on Escape key** — Treat Escape as a window-close trigger. | **Rejected.** The editor is not a demo scene. Demo scenes use Escape for quick exit. The editor is a professional tool — window close (via title-bar X button or OS shortcut) is the standard exit mechanism. Escape may be needed for cancelling operations, dismissing modals, or other in-editor actions in the future. |

## Decision

### Decision 1: Editor as a separate static library (`buddd_editor`)

A new static library target `buddd_editor` in `src/editor/` provides the `Editor` class and all future editor infrastructure. It is **not** part of `buddd_engine` or `buddd` (the CLI binary). The library links `buddd_engine` as PUBLIC.

```
src/editor/
├── CMakeLists.txt    # add_library(buddd_editor STATIC ...)
├── editor.h          # Editor class declaration
└── editor.cpp        # Editor implementation
```

CMake:
```cmake
add_library(buddd_editor STATIC editor.cpp)
target_link_libraries(buddd_editor PUBLIC buddd_engine)
target_include_directories(buddd_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

The CLI binary (`buddd` in `src/cmd/`) links `buddd_editor` as PRIVATE:
```cmake
target_link_libraries(buddd PRIVATE buddd_engine buddd_editor)
```

**Rationale**: Separation of concerns. The editor is an application library that can be reused across different entry points (CLI, GUI launcher, tests) without coupling to any specific dispatch mechanism. It follows the same pattern as `src/cmd/` — a separate library that depends on the engine.

### Decision 2: Editor reuses the existing `App` lifecycle

An `EditorApp` subclass of `App` (defined in `src/cmd/apps/editor_app.h/.cpp`) adapts the `Editor` to the `App` lifecycle:

- `config()` → returns `AppConfig{.title = "Buddd Editor", .width = 1280, .height = 800}`
- `setup(ctx)` → creates an `Editor`, calls `editor_->setup(ctx)`
- `on_render(ctx)` → calls `editor_->draw_ui()`
- `shutdown()` → calls `editor_->shutdown()`

The editor runs via the existing `run_app()` centralised render loop. No changes to `run_app()` or the `App` base class are needed.

**Rationale**: Avoids duplicating the render loop, window creation, event handling, platform initialisation, and frame timing. The existing `App` lifecycle is the single source of truth for display-mode applications. The `EditorApp` is just another `App` subclass, following the same contract as `TriangleApp`, `PhongApp`, etc.

### Decision 3: Editor namespace `buddd::editor`

All editor code resides in `namespace buddd::editor`. This is consistent with existing namespaces:
- `buddd::engine` — engine internals
- `buddd::cmd` — CLI command infrastructure
- `buddd::cmd::app` — App subclasses
- `buddd::editor` — editor library

### Decision 4: Editor class with PIMPL pattern

The `Editor` class uses the PIMPL (Pointer to Implementation) idiom:

```cpp
class Editor {
public:
    Editor();
    ~Editor();
    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void>;
    auto draw_ui() -> void;
    auto shutdown() -> void;
private:
    struct EditorImpl;
    std::unique_ptr<EditorImpl> impl_;
};
```

**Rationale**: PIMPL hides implementation details from consumers, reduces header dependencies (no `<imgui.h>` in the public header), and provides a stable ABI for the library boundary. This is consistent with existing PIMPL usage in the engine (e.g., `PhongMaterial`, `PbrMaterial`).

### Decision 5: ImGui init failure is fatal in display mode (amends ADR-026)

**This decision amends ADR-026, Decision 2 ("Init failure is non-fatal").**

In `RenderDevice::create()` (the display-available branch), if `engine_imgui::init()` fails, the error is now propagated instead of logged as a warning:

```cpp
if (auto imgui_result = engine_imgui::init(sdl_window, gl_context); !imgui_result) {
    return make_error(imgui_result.error());  // was: BUDDD_LOG_WARN + continue
}
```

- The headless path is **unaffected** — no ImGui init is called there.
- This change affects **all** display-mode apps, not just the editor.
- `Editor::setup()` additionally verifies `engine_imgui::is_initialized()` as a belt-and-suspenders check.

**Rationale**: The editor (and all ImGui-dependent applications) cannot function without ImGui. A silent failure produces a running application with no visible UI — the user has no indication of why. Hard failure (error propagation, exit code 1) ensures the problem is immediately visible and actionable. This is the correct trade-off for interactive applications.

### Decision 6: Architecture boundary — `src/editor/` has same constraints as `src/cmd/`

No SDL3, OpenGL, or GLM headers may be included from any file under `src/editor/` or `src/cmd/apps/editor_app.*`. All platform and graphics access must go through engine abstractions (`EngineContext`, `Window`, `RenderDevice`).

This is consistent with ADR-019 and the existing `src/cmd/` convention. The editor library is a consumer of engine APIs, not a direct user of platform/graphics libraries.

### Decision 7: `buddd edit` CLI command via existing binary

The editor is launched via the existing `buddd` CLI binary as a command — the same pattern as `run`, `version`, `help`:

```cpp
if (cmd == "edit") {
    bc::app::EditorApp editor_app;
    return bc::run_app(editor_app, bc::RunningArgs{});
}
```

No separate binary. No new CMake executable target. The existing single-entry-point pattern is preserved.

**Rationale**: Single entry point. Shared infrastructure (argument parsing, platform init, error handling). Developers build and run one binary. Build complexity does not increase.

### Decision 8: `--frame` and `--capture` are incidental, not editor features

`--frame` and `--capture` work through `run_app()` infrastructure when passed to `buddd edit`, but they are NOT documented editor features. They exist for debugging and testing only. The editor runs interactively until window close.

The editor does **not** respond to Escape — only window close (title-bar X, `Alt+F4`, Cmd+Q) exits the editor.

**Rationale**: The editor is a professional tool, not a demo scene. Window close is the standard exit mechanism. Escape may be needed for future in-editor interactions.

## Consequences

### Positive

- **Separation of concerns**: Editor code is isolated from engine internals and CLI dispatch. Each layer has a clear responsibility.
- **Reusable editor library**: `buddd_editor` can be used from the CLI, from tests, or from future entry points (e.g., a native desktop launcher) without modification.
- **No render loop duplication**: The editor uses the same battle-tested `run_app()` render loop as all other applications. No new platform initialisation, event handling, or frame timing code.
- **Architecture boundary preserved**: Zero SDL3/OpenGL/GLM headers outside `src/engine/`. Verified by `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/ src/cmd/apps/editor_app.*` — zero matches.
- **Stable library boundary**: PIMPL hides implementation details, reduces recompilation when editor internals change, and provides a clean public API.
- **Hard failure on ImGui init failure**: Developers catch configuration issues immediately rather than debugging a blank window. All display-mode apps benefit.
- **Single entry point**: One binary, one build target. No separate editor executable to build, package, or debug.
- **Consistent namespace convention**: `buddd::editor` follows the established pattern of `buddd::engine`, `buddd::cmd`, `buddd::cmd::app`.
- **Consistent CLI pattern**: `buddd edit` follows the same command pattern as `buddd version`, `buddd help`, `buddd run`.

### Negative

- **Build time increase**: `buddd_editor` adds a new static library target. One additional `.cpp` file to compile (`editor.cpp`) per configuration. Negligible in practice (~0.1s debug build).
- **ABI boundary**: The PIMPL pattern adds a level of indirection and requires the destructor to be defined in the `.cpp` file (where `EditorImpl` is complete). This is standard C++ and well-understood.
- **Breaking change for ImGui-based apps**: Any code relying on the old non-fatal ImGui init behavior will now receive a hard error. In practice, this is the desired behavior — no display-mode application should silently lack ImGui.
- **Version coupling**: `buddd_editor` links `buddd_engine` as PUBLIC, so any ABI-breaking change in the engine requires recompilation of the editor. This is expected and correct for a tightly coupled static library.
- **Headless mode requires explicit error handling**: `EditorApp::setup()` must check `BUDDD_HAS_DISPLAY` at compile time and return an error. This is a one-time cost in `editor_app.cpp`.

### Impact on existing ADRs

- **ADR-026**: Decision 2 ("Init failure is non-fatal") is amended to "Init failure is fatal in display mode." The headless path is unchanged.
- **ADR-019**: The architecture boundary is extended to `src/editor/` — same constraints as `src/cmd/`. No amendment needed; ADR-019 already applies to all code outside `src/engine/`.
- **ADR-014**: No change. The `App` base class and `run_app()` are unchanged. `EditorApp` is a new subclass following the existing contract.
- **ADR-012**: No change. Editor accesses engine services through `EngineContext` only.

## Related documents

- SPEC-NNNN (`.specs/sprint-2026-06/editor-scaffolding/spec.md`): Spec-level documentation of the editor scaffolding requirements and acceptance criteria.
- ADR-026 (`docs/adr/ADR-026-imgui-integration.md`): Amended by this ADR (Decision 2 — init failure behavior).
- ADR-019 (`docs/adr/ADR-019-architecture-boundaries.md`): Architecture boundary enforced in `src/editor/`.
- ADR-014 (`docs/adr/ADR-014-cli-app-system.md`): Centralised App lifecycle reused by EditorApp.
- ADR-012 (`docs/adr/ADR-012-navigable-object-graph-engine-service.md`): EngineContext access pattern used by Editor.
- ADR-001 (`docs/adr/ADR-001-result-error-pattern.md`): `Result<T>` error pattern used by Editor and EditorApp.
