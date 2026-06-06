# ADR-014: CLI App System — Centralised Render Loop with App Lifecycle

## Status

`Accepted`

## Context

ADR-004 (Demo System Architecture) established the pattern of per-demo free functions in `src/cmd/demo/`, each owning its own render loop, dispatched via if/else-if chains in `DemoCommand`. This served the project well for six scenes (triangle, cube, cube-scene, textured-cube, free-camera, phong), but accumulated several design tensions:

1. **Duplicated render-loop boilerplate**: Every demo and capture function repeated the same `poll_events()` / `begin_frame()` / `end_frame()` pattern with frame counting and `sleep_for`. Any change to the render-loop structure (e.g., adding capture support, changing frame limiting) required modifying all six demo files plus the capture files.

2. **`CaptureCommand` duplicated `RenderSystem::render()`**: The `phong_capture` module manually reimplemented ~200 lines of `RenderSystem` rendering logic solely to inject a `read_pixels()` call between `begin_frame()` and `end_frame()`.

3. **Three separate CLI commands with overlapping semantics**: `run` (empty window), `demo <scene>`, and `capture <scenario>` all did essentially the same thing — open a window, render frames, exit. The distinction between "demo", "capture", and "run" was an accidental artifact of separate implementation histories, not a meaningful semantic difference.

4. **No reusable application lifecycle**: There was no `App` abstraction that future tools (CI scripts, headless batch processing, integration tests) could reuse. Each entry point constructed `Platform` → `Window` → `RenderDevice` inline.

5. **Per-demo free-function signature was rigid**: Every demo accepted `(Platform&, RenderDevice&, int, const char* const*)`. Adding a new parameter (e.g., capture specs, frame limit) required changing every demo's signature, even those that did not use the parameter.

The project had accumulated architectural precedent that influenced the solution:

- **ADR-003**: The render loop is an application-level concern, not an engine concern.
- **ADR-004**: Demo code lives in `src/cmd/demo/` as per-demo files; `DemoCommand` dispatches by name.
- **ADR-012**: EngineService provides navigable object graph; demo functions no longer need `Platform&`.
- **CONST-001**: No SDL3, OpenGL, or GLM headers from `src/cmd/`.

## Decision

We replace the per-demo free-function-with-individual-render-loop pattern with a centralised `App` lifecycle pattern:

### 1. `App` base class with lifecycle

A virtual `App` base class in `src/cmd/app.h` defines:

- `config()` → `AppConfig` — window title, width, height
- `setup(RenderDevice&)` → `Result<void>` — one-time initialisation
- `render(RenderDevice&, int frame)` → `void` — one frame's rendering only (no begin/end)
- `shutdown()` → `void` — cleanup

A protected `running_` boolean allows interactive scenes to stop the loop (e.g., on ESC).

### 2. Centralised render loop in `run_app()`

A single `run_app(App&, const RunningArgs&)` free function owns the entire render loop:

```
create Platform, Window, RenderDevice
app.setup()
loop:
    begin_frame()
    app.render(device, frame)
    (optionally) read_pixels() for capture
    end_frame()
    poll_events() → exit if closed
app.shutdown()
```

`RunningArgs` carries frame limits and capture specifications parsed from CLI flags.

### 3. `App` subclasses in `src/cmd/apps/`

Each scene becomes an `App` subclass in a new `src/cmd/apps/` directory. The `render()` method contains **only** the per-frame rendering logic (no `begin_frame()`/`end_frame()`). Seven subclasses are created: `RunApp` (empty window), `TriangleApp`, `CubeApp`, `CubeSceneApp`, `TexturedCubeApp`, `FreeCameraApp`, `PhongApp`.

### 4. `RenderSystem::render_scene()` extraction

`RenderSystem::render_scene()` extracts the rendering logic from `RenderSystem::render()` without `begin_frame()`/`end_frame()`. The existing `render()` delegates to `render_scene()`. This lets `run_app()` call `render_scene()` directly and inject `read_pixels()` between framing calls, eliminating the need for manual `RenderSystem` reimplementation.

### 5. Unified CLI

The `demo` and `capture` subcommands are removed. The single `run` command handles all cases:

```
buddd run [<scene>] [--frame N] [--capture [N:]path]...
```

- No scene → empty window (same as old `buddd run`)
- `triangle`, `cube`, etc. → run the named scene
- `--frame N` → limit frames (for CI/headless)
- `--capture N:path` → capture frame N to PNG (repeatable)

### 6. New module layout

```
src/cmd/
├── app.h / app.cpp          ← App base class + run_app()
├── app_config.h / app.cpp   ← CaptureSpec, RunningArgs, parse_running_args()
├── apps/                    ← App subclasses (replaces most of src/cmd/demo/)
│   ├── run_app.h/.cpp
│   ├── triangle_app.h/.cpp
│   ├── cube_app.h/.cpp
│   └── ...
├── demo/                    ← kept: demo_helpers.h/.cpp (setup_triangle, setup_cube)
├── commands/                ← kept: version_command, help_command
└── main.cpp                 ← dispatch: run/version/help only
```

### Status of ADR-004

ADR-004's decisions are partially superseded:

| ADR-004 Decision | Status |
|---|---|
| Dedicated `src/cmd/demo/` directory | **Superseded**. Scene code now lives in `src/cmd/apps/`. `src/cmd/demo/` retains only `demo_helpers.*`. |
| Per-demo free functions in `buddd::cmd::demo` | **Superseded**. Replaced by `App` subclasses. |
| If/else-if dispatch in `DemoCommand` | **Superseded**. Dispatch moved to `main.cpp` under `run <scene>`. |
| Demo helpers co-located with demos | **Retained**. `demo_helpers.h/.cpp` stay in `src/cmd/demo/`. |
| CMake glob auto-discovers demo files | **Evolved**. Glob now covers `apps/*.cpp`. |
| Each demo owns its render loop | **Superseded**. Centralised in `run_app()`. |

## Alternatives considered

### Keep per-demo free functions, extract only the loop framework

Instead of an `App` base class, keep the free-function pattern but provide a `run_loop()` helper that accepts a per-frame callback. The demos would remain as free functions but no longer own the loop.

- **Pros**: Minimal refactoring. Free functions are simpler than virtual classes. No virtual dispatch overhead.
- **Cons**: Per-frame state (camera, animation timers, render system references) must be captured via lambdas or global variables — no natural place for per-scene state. The `setup`/`render`/`shutdown` lifecycle is implicit rather than explicit. Capturing frame-specific configuration (e.g., capture flags, camera overrides for capture mode) requires threading parameters through lambda captures, which grows unwieldy with more parameters.
- **Verdict**: Rejected. The `App` virtual interface provides a natural home for per-scene state (members), a clear lifecycle contract, and straightforward subclassing for scene authors.

### Template-based render loop (policy-based design)

Make `run_app()` a template that accepts any callable with `setup()`/`render()`/`shutdown()` methods via duck typing, without a virtual base class.

- **Pros**: No virtual dispatch overhead. No coupling to a base class. Easier to test (no mocking required).
- **Cons**: Template code must be in headers, increasing compilation time for `main.cpp`. Error messages on misuse are less clear (template substitution failures). The `App` base class provides documentation-by-contract (the virtual interface is explicit and documented in a single header). The duck-typing approach is more common in generic libraries than in application code.
- **Verdict**: Rejected. The virtual base class is simpler, more explicit, and better suited to application-level code where the number of subclasses is small and known at compile time.

### Keep three separate commands (`run`/`demo`/`capture`)

Preserve the existing command separation but refactor each command to use the centralised render loop.

- **Pros**: Backward compatibility for users of `buddd demo triangle` and `buddd capture cube`. Less CLI surface change.
- **Cons**: Three commands doing essentially the same thing is confusing. The `demo`/`capture` distinction was an implementation artifact, not a user-facing concept. Maintaining backward compatibility would require keeping an alias layer and deprecation warnings, increasing maintenance surface. The project explicitly chose no deprecation period (SPEC-008, A-07).
- **Verdict**: Rejected. A single `run` command is simpler and more consistent.

### Keep per-demo render loops, fix only the `phong_capture` duplication

The minimal fix: extract `RenderSystem::render_scene()`, update `phong_capture` to use it, and leave everything else unchanged.

- **Pros**: Minimal change. No CLI breakage. No new files or directories.
- **Cons**: Does not address any of the other four context problems (duplicated loop boilerplate, three overlapping CLI commands, no reusable lifecycle, rigid function signatures). Each new scene would still need to copy-paste the 15-line render-loop skeleton. The maintenance burden would grow linearly with each new scene.
- **Verdict**: Rejected. The minimal fix treats only the symptom, not the root cause.

### Runtime scene registry (std::map<std::string, std::unique_ptr<App>>)

Register all `App` subclasses in a global map at startup, and dispatch `run <scene>` by map lookup.

- **Pros**: Adding a new scene requires zero dispatch code changes — just register in the map. Centralised scene list. Extensible at runtime (in theory).
- **Cons**: Requires a registration mechanism (static initialisers or explicit population in a single file). The if/else-if chain in `main.cpp` has the same maintenance cost (one line per scene) and is more transparent — the full list is immediately visible. The map adds indirection with no benefit at the current scale (< 20 scenes).
- **Verdict**: Rejected for now. If/else-if is simpler at the current scale. A registry can be introduced later if the scene count grows significantly, as ADR-004 also noted when rejecting it for demos.

### Centralised render loop with `sleep_for` frame limiting

Keep the existing `sleep_for`-based frame limiting from the old demo code to control frame rate.

- **Pros**: Predictable frame timing independent of display refresh rate.
- **Cons**: VSync already provides frame limiting when a display is available. `sleep_for` adds unnecessary latency and CPU wake-up overhead. Headless mode already uses `--frame` as the limiting mechanism. The project explicitly chose VSync-only (SPEC-008, A-10).
- **Verdict**: Rejected. The centralised loop relies on VSync for frame timing, matching existing `RunCommand` behavior.

## Consequences

### Positive

- **Single render loop**: Frame counting, capture injection, event polling, and shutdown all live in one function (`run_app()`). A change to the loop structure modifies exactly one file instead of six or more.
- **No more manual `RenderSystem` reimplementation**: `phong_capture`'s ~200-line manual rendering pipeline is eliminated. All scenes use `RenderSystem::render_scene()` uniformly.
- **Cleaner CLI surface**: One command (`run`) instead of three overlapping commands (`run`/`demo`/`capture`). Scene authors no longer need to decide whether their work is a "demo" or a "capture scenario".
- **Extensible lifecycle**: Adding a new scene requires (a) creating an `App` subclass in `src/cmd/apps/`, (b) adding one `else if` branch in `main.cpp`. No CMake changes, no render-loop boilerplate, no new command classes.
- **Reusable `App` abstraction**: Future tools (CI scripts, automated regression tests, headless batch processors) can instantiate `App` subclasses without going through the CLI.
- **Per-frame state naturally scoped**: Scene state (cameras, render systems, animation timers) lives as member variables of the `App` subclass. No globals, no lambda captures.
- **Architecture boundary preserved**: All `App` subclasses go through engine abstractions (`RenderDevice`). CONST-001 compliance continues.
- **Capture logic extracted from scene code**: Capture specs are parsed by `parse_running_args()` and handled by `run_app()`. App subclasses do not know about capture — they simply render one frame when asked.

### Negative

- **Virtual dispatch overhead**: Each frame calls `app.render()` through a virtual dispatch. The cost is negligible (< 5 ns per call) compared to the GPU work of rendering a frame (milliseconds).
- **Breaking change for existing `demo`/`capture` users**: Developers who used `buddd demo triangle` or `buddd capture cube` must update their commands to `buddd run triangle`. The error message tells them the command is unknown, but there is no deprecation period.
- **Concrete `AppConfig` at construction time**: An `App` subclass's window configuration (`config()` return value) is determined at construction time. There is no mechanism for CLI flags to override window dimensions post-construction. This is acceptable because the spec defines fixed 1024×768.
- **`demo_helpers` `std::exit()` avoids `shutdown()`**: `setup_triangle()` and `setup_cube()` call `std::exit(EXIT_FAILURE)` on allocation failure. This bypasses `App::shutdown()`. This is a pre-existing limitation inherited from the old code, not a new problem.
- **Frame numbering dualism**: `App::render()` receives a 0-based frame index, but `--capture` uses 1-based frame numbers. The conversion happens in `run_app()`. This is a persistent off-by-one risk that requires careful maintenance.

### Compliance

- All new scene code SHALL create an `App` subclass in `src/cmd/apps/`.
- Scene `render()` methods SHALL NOT call `begin_frame()` or `end_frame()`.
- `src/cmd/apps/` is the canonical location for scene implementations.
- `src/cmd/demo/demo_helpers.*` are the only remaining files in `src/cmd/demo/`.
- The `demo` and `capture` commands are permanently removed; they SHALL NOT be reintroduced.
- CONST-001 continues to apply: no SDL3, OpenGL, or GLM headers in `src/cmd/`.

## Related documents

- SPEC-008 (`.specs/sprint-2026-06/cli-app-system/spec.md`): Spec-level documentation of the CLI App System.
- IMPL-008 (`.specs/sprint-2026-06/cli-app-system/implementation-contract.md`): Implementation contract with detailed file-by-file changes.
- ADR-004 (`docs/adr/ADR-004-demo-system-architecture.md`): **Partially superseded** — per-demo free functions with individual render loops replaced by App subclasses with centralised loop.
- ADR-003 (`docs/adr/ADR-003-render-pipeline-architecture.md`): Render pipeline architecture — precedent that render loops are application-level concern, now concretised in `run_app()`.
- ADR-012 (`docs/adr/ADR-012-navigable-object-graph-engine-service.md`): EngineService pattern — enables `App` subclasses to access `Platform`/`Window`/`InputSystem` via `RenderDevice.window().platform()` without explicit references.
- CONST-001 (`docs/constitution/rules/CONST-001-architecture-boundaries.md`): Architecture boundary preserved.
