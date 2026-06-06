# SPEC-023 — Console Timestamps, FreeCameraMovement Refactoring & Helmet Investigation

## Problem

Five distinct but related gaps exist:

1. **No console timestamps** — The `ConsoleSink` writes `[LEVEL] [Tag] message` without any temporal context. Developers debugging the ~10s helmet load time cannot correlate log output with wall-clock time without manual instrumentation.
2. **Duplicated camera movement code** — The free-camera movement logic (right-click capture, mouse look, WASD movement) is copy-pasted identically in `free_camera_app.cpp:render()` and `phong_app.cpp:render()`. Any bug fix or enhancement must be applied in two places.
3. **No reusable free-camera component** — There is no ECS `Component` that encapsulates camera movement, making it impossible to add a free camera to a new app without duplicating the ~60 lines of input handling.
4. **Manual Updatable boilerplate** — Each interactive app must manually iterate its Updatable components (e.g., `world_.each<FreeCameraMovement>(...)` in `render()`), duplicating the same iteration pattern in every app. Apps should not need to know about component iteration.
5. **DamagedHelmet: deformation + slow load** — The helmet model renders with visible geometry deformation and takes ~10s to load. Both issues must be investigated and fixed.

## Goals

1. Prepend `HH:MM:SS.fff` timestamps to console log output for temporal awareness.
2. Create a reusable `FreeCameraMovement` ECS component encapsulating all free-camera input logic.
3. Refactor `free_camera_app` and `phong_app` to use `FreeCameraMovement` instead of inline duplicated code.
4. Create a new `gltf-helmet` demo app loading the DamagedHelmet with a free camera for inspection.
5. Investigate and fix the DamagedHelmet deformation and slow-load issues.
6. Ensure the DamagedHelmet renders with correct geometry when viewed from the default camera position.
7. Eliminate boilerplate by introducing an `Updatable` interface that `World` auto-discovers, so `run_app` automatically calls `update()` on all Updatable components each frame without manual iteration in app `render()` methods.

## Non-goals

- Adding CLI flags to disable console timestamps (V1 simplicity).
- Changing the FileSink timestamp format (stays ISO 8601).
- Adding colour/ANSI escape codes to console output.
- Touching normal mapping V2, alpha modes (MASK/BLEND), or `KHR_materials_pbrSpecularGlossiness`.
- Implementing async model loading.
- Refactoring any other apps beyond `free_camera_app` and `phong_app`.
- Adding unit tests for FreeCameraMovement (manual verification via app demos).
- Modifying the PBR shader pipeline.
- Changing the `Component` base class or adding dependencies to it.
- Making `Updatable` a subclass of `Component` (they are orthogonal hierarchies — `Updatable` is a pure abstract class not tied to ECS).

## Key entities

### `Updatable` interface (`src/engine/scene/updatable.h`)

Pure abstract class in namespace `buddd::engine`, **not** inheriting from `Component`:

```cpp
class Updatable {
public:
    virtual ~Updatable() = default;
    virtual auto update(const InputSystem& input, Window& window, float dt) -> bool = 0;
};
```

- `update()` returns `false` to request early loop exit (e.g., ESC press), `true` to continue.
- A component can inherit from *both* `Component` and `Updatable` via multiple inheritance (valid C++).

### `World` changes

- New private member: `std::vector<Updatable*> updatables_`.
- `World::add_component<T>()` uses `if constexpr (std::is_base_of_v<Updatable, T>)` to auto-register matching components into `updatables_`.
- New method: `World::update_updatables(InputSystem&, Window&, float dt) -> bool` iterates all registered updatables and calls `update()` on each. Returns `false` on the first `false` return (short-circuit), `true` otherwise.
- Updatable cleanup in `flush_destroyed()`: before the `owned` unique_ptr goes out of scope (destroying the node and its components), iterate the node's `components_` and remove any `Updatable` raw pointers from `updatables_` via `dynamic_cast<Updatable*>` and `std::erase`. Applied in both the parent-linked and root-entity branches.
- Updatable cleanup in `remove_component<T>()`: before erasing a component from `node->components_`, check if it derives from `Updatable` via `dynamic_cast<Updatable*>` and remove it from `updatables_` via `std::erase`.

### `App` changes

- New virtual method: `[[nodiscard]] virtual auto world() noexcept -> World* { return nullptr; }` — default returns `nullptr`. Apps with a World override this.
- New public method: `void set_running(bool v) { running_ = v; }` so `run_app` can stop the loop when an updatable returns `false`.

### `run_app` render loop change

Before `app.render(...)`, the loop calls:
```cpp
if (auto* world = app.world()) {
    auto& input = (*device)->window().platform().input_system();
    auto& window = (*device)->window();
    float dt = (*device)->window().platform().delta_time();
    if (!world->update_updatables(input, window, dt)) {
        app.set_running(false);
    }
}
```

### `FreeCameraMovement`

Now inherits from **both** `Component` and `Updatable`. Its `update()` method contains the full camera movement logic (mouse capture toggle, mouse look, WASD movement, ESC to exit). Compatible with `dynamic_cast` in `World::each<T>()`.

## Actors

| Actor | Description |
|---|---|
| Developer | Reads console log output; inspects the Helmet via the `gltf-helmet` scene; runs existing demos to verify no regression. |
| End user | Launches `buddd run gltf-helmet`, `buddd run free-camera`, or `buddd run phong`. Interacts via keyboard and mouse. |

## User-visible behaviour

1. Every console log line now begins with `[HH:MM:SS.fff]` e.g. `[14:32:05.123] [INFO] [Engine] hello world`.
2. `FreeCameraMovement` is not directly user-visible, but the camera behaviour in `free-camera`, `phong`, and `gltf-helmet` scenes is identical to the pre-refactoring behaviour of `free-camera`.
   - **Exit mechanism**: The render loop (`run_app()`) calls `World::update_updatables()` before each `app.render()`. This iterates all `Updatable` components (including `FreeCameraMovement`) and calls `update()` on each. If any `update()` returns `false` (ESC pressed), `run_app()` calls `app.set_running(false)` to stop the render loop. Apps no longer need to manually iterate components in their `render()` method.
3. A new `gltf-helmet` scene is available: `buddd run gltf-helmet` opens a 1280×720 window displaying the DamagedHelmet with free-camera controls.
4. The DamagedHelmet renders with correct geometry (helmet shape, not deformed).
5. Model loading completes in under 3 seconds instead of ~10s.

## User stories

### Story 1 — Console timestamps (Priority: P1)

As a developer debugging the Helmet loading time,
I want every console log line to show wall-clock time with millisecond precision,
so that I can correlate log output with performance measurements.

**Given** the engine is started with any scene
**When** a log message is written to the console sink
**Then** the output line begins with `[HH:MM:SS.fff]` where `HH` is hours (00–23), `MM` is minutes (00–59), `SS` is seconds (00–59), and `fff` is milliseconds (000–999).

**Given** the engine is started with `--log-file=/tmp/test.log`
**When** log messages are written
**Then** the file sink output uses the existing ISO 8601 format `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message` (unchanged).

### Story 2 — Reusable camera component (Priority: P1)

As a developer writing a new interactive scene,
I want to add a `FreeCameraMovement` component to my camera entity and have it auto-updated by the engine,
so that I get free-camera controls (WASD + mouse look + ESC to exit) without duplicating code or manual iteration.

**Given** an entity with both `CameraComponent` and `FreeCameraMovement` components
**When** the render loop calls `World::update_updatables()` each frame
**Then** the `FreeCameraMovement::update()` is invoked automatically (camera position and orientation update based on user input using the component's configurable parameters), without the app needing to manually call `each<FreeCameraMovement>()` in its `render()` method.

### Story 3 — Refactored free_camera_app (Priority: P1)

As a maintainer,
I want `free_camera_app` to use the new `FreeCameraMovement` component with automatic Updatable dispatch,
so that the duplicated camera movement code and manual component iteration are removed from `render()`.

**Given** the `free_camera_app` project
**When** I build and run `buddd run free-camera`
**Then** the app behaves identically to the pre-refactoring version (right-click capture, mouse look, WASD movement, ESC to exit).

### Story 4 — Refactored phong_app (Priority: P1)

As a maintainer,
I want `phong_app` to use the new `FreeCameraMovement` component with automatic Updatable dispatch,
so that the duplicated camera movement code, `yaw_`/`pitch_`/`prev_right_click_` member variables, and manual `each<>()` iteration are eliminated.

**Given** the `phong_app` project
**When** I build and run `buddd run phong`
**Then** the app behaves identically to the pre-refactoring version (camera controls, orbiting lights, same initial camera position).

### Story 5 — gltf-helmet app (Priority: P1)

As a developer,
I want a dedicated `gltf-helmet` scene that loads and displays the DamagedHelmet model,
so that I can inspect it interactively with free-camera controls.

**Given** the engine is built with the `gltf-helmet` scene
**When** I run `buddd run gltf-helmet`
**Then** the window opens with title `"Buddd Engine — glTF Helmet"` at 1280×720 resolution showing the DamagedHelmet with a directional light (white, intensity 1.5, pitch=-45°, yaw=45°), default camera at position (0, 1.5, 3) looking at origin, and the camera is controllable via right-click + mouse look + WASD.

### Story 6 — Helmet deformation fix (Priority: P1)

As a developer,
I want the DamagedHelmet to render with correct geometry,
so that it looks like a helmet and not a deformed mesh.

**Given** the DamagedHelmet is loaded and rendered
**When** viewed from the default camera position
**Then** the geometry matches the expected helmet shape (no shearing, no incorrect rotation, no flipped normals).

### Story 7 — Helmet load performance (Priority: P2)

As a developer,
I want the DamagedHelmet to load faster than the current ~10s,
so that iteration time is reduced.

**Given** the engine is started with `buddd run gltf-helmet`
**When** the model loading completes
**Then** the elapsed time between the first load log and the "render system ready" log is under 3 seconds.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|---|
| AC-001 | `ConsoleSink::write()` prepends `[HH:MM:SS.fff]` before the level. Format: `[14:32:05.123] [LEVEL] [Tag] message`. | Run `buddd run free-camera` and observe console output. Timestamp appears on every line. |
| AC-002 | `FileSink::write()` output is unchanged (still ISO 8601 `YYYY-MM-DDTHH:MM:SS`). | Run with `--log-file=/tmp/test.log`, inspect file — format is `2026-06-06T14:32:05 [LEVEL] [Tag] message`. |
| AC-003 | No CLI flag to disable console timestamps. | Check `--help` output and source: no `--no-timestamp` or equivalent flag. |
| AC-004 | `Updatable` interface exists in `src/engine/scene/updatable.h` as a pure abstract class with `virtual auto update(const InputSystem& input, Window& window, float dt) -> bool = 0`. | File exists and compiles. |
| AC-005 | `World::add_component<T>()` auto-registers components that inherit from `Updatable` into an internal `updatables_` vector using `if constexpr (std::is_base_of_v<Updatable, T>)`. | Code review of `world.h`. |
| AC-006 | `World::update_updatables(InputSystem&, Window&, float dt)` iterates all registered updatables; returns `false` if any returns `false` (short-circuits), `true` if all return `true`. | Wire a component that returns `false` and verify the app exits on that frame. |
| AC-007 | `App` class has a public `set_running(bool)` method and a virtual `world()` accessor returning `World*` (default `nullptr`). | Code review of `app.h`. |
| AC-008 | `run_app()` calls `app.world()->update_updatables(...)` before `app.render()` each frame when a world is available. | Code review of `app.cpp`. |
| AC-009 | `FreeCameraMovement` class exists in `src/engine/scene/free_camera_movement.h` and `.cpp`, inherits from **both** `Component` and `Updatable`. | File exists and compiles. |
| AC-010 | `FreeCameraMovement` has public fields: `move_speed` (default 5.0f), `mouse_sensitivity` (default 0.002f), `pitch_clamp_degrees` (default 89.0f), `invert_yaw` (default false), `invert_pitch` (default false). | Inspect header. |
| AC-011 | `FreeCameraMovement` has a `update(InputSystem& input, Window& window, float dt) -> bool` method. | Compiles and signature matches. |
| AC-012 | `FreeCameraMovement::update()` returns `false` when ESC is pressed (request exit), `true` otherwise. | Wire component into app; pressing ESC exits the app. |
| AC-013 | `FreeCameraMovement::update()` implements right-click mouse capture toggle. | Click right mouse button → mouse captured; release → mouse released. |
| AC-014 | `FreeCameraMovement::update()` implements mouse look (yaw/pitch from mouse_delta) when mouse is captured. | Move mouse while right-click held → camera rotates. |
| AC-015 | `FreeCameraMovement::update()` implements WASD (+ Space / Ctrl for up/down) movement when mouse is captured. | Press W/A/S/D/Space/Ctrl while mouse captured → camera moves. |
| AC-016 | `FreeCameraMovement` automatically finds `CameraComponent` on the same entity via `entity().get_component<CameraComponent>()`. | Component compiles and works with the camera entity. |
| AC-017 | `FreeCameraMovement` stores private state: `yaw`, `pitch`, `prev_right_click`. | Inspect header (private section). |
| AC-018 | `free_camera_app` overrides `world()` to return its `World*`. Manual `each<FreeCameraMovement>()` call is removed from `render()`. | Code review: `free_camera_app.h` has `world()` override; `free_camera_app.cpp::render()` has no `each<FreeCameraMovement>` call. |
| AC-019 | `free_camera_app.h` removes `yaw_`, `pitch_`, `prev_right_click_` member variables. | Code review. |
| AC-020 | `free_camera_app` behaviour is identical to before refactoring. | Run `buddd run free-camera` — same camera controls, same exit behaviour. |
| AC-021 | `phong_app` overrides `world()` to return its `World*`. Manual `each<FreeCameraMovement>()` call is removed from `render()`. | Code review: `phong_app.h` has `world()` override; `phong_app.cpp::render()` has no `each<FreeCameraMovement>` call. |
| AC-022 | `phong_app.h` removes `yaw_`, `pitch_`, `prev_right_click_` member variables. | Code review. |
| AC-023 | `phong_app` behaviour is identical to before refactoring (camera controls, orbiting lights, initial camera position unchanged). | Run `buddd run phong` — same camera start position (6.0, 3.5, 8.0), same controls, orbiting lights work. |
| AC-024 | `gltf_helmet_app.h` and `gltf_helmet_app.cpp` exist in `src/cmd/apps/`. | Files exist. |
| AC-025 | `gltf-helmet` scene dispatches in `main.cpp`. | Running `buddd run gltf-helmet` launches the correct app. |
| AC-026 | `gltf-helmet` app window title is `"Buddd Engine — glTF Helmet"`, resolution is 1280×720. | Check `config()` return value. |
| AC-027 | `gltf-helmet` app loads DamagedHelmet from `"models/damaged-helmet/DamagedHelmet"` (via `AssetManager::create<ModelAsset>` using YAML metadata), uses `FreeCameraMovement` component (auto-updated via `Updatable`), and has a directional light (white, intensity 1.5, pitch=-45°, yaw=45°). | Code review. |
| AC-028 | DamagedHelmet loads in under 3 seconds (measured from start of model loading to completion). | Add temporary `BUDDD_LOG_INFO` before/after loading or use `--log-level=debug` to observe timestamps on load completion messages. |
| AC-029a | Loaded vertex count matches glTF accessor: 14556 vertices. | Code review of `build_model_from_mesh` — verify vertex count read from accessor matches expected. |
| AC-029b | Loaded index count matches glTF accessor: 46356 indices. | Code review — verify index count read from accessor matches expected. |
| AC-029c | Vertex position min/max values match glTF accessor bounds (±0.001 tolerance). | Code review — compare min/max of loaded vertex positions against `accessor.min`/`accessor.max` arrays. |
| AC-029d | Node rotation `[0.7071, 0, -0, 0.7071]` (glTF x,y,z,w) correctly converted to engine Quat(w=0.7071, x=0.7071, y=0, z=0) producing a 90° rotation around X axis. | Code review of `build_node()` quaternion conversion. |
| AC-029e | The rendered helmet has no visual artifacts (missing triangles, inverted faces, or clearly wrong geometry) when viewed from the default camera position at (0, 1.5, 3) looking at origin. | Launch `buddd run gltf-helmet`, view from default camera — no missing triangles, inverted faces, or clearly wrong geometry. |
| AC-030 | DamagedHelmet quaternion conversion (glTF x,y,z,w → engine w,x,y,z) is verified correct in `build_node()`. | Code review of `src/engine/asset/model_loader.cpp` line 795–800. |
| AC-031 | DamagedHelmet TRS application in `build_node()` is verified correct. | Code review: no double-application or missing TRS. |
| AC-032 | DamagedHelmet vertex/index buffer generation consumes the model hierarchy only once (no double traversal). | Code review: `add_model_to_world` consumes `Model` from node (move semantics) — verify hierarchy is traversed exactly once. |
| AC-033 | The timestamps use `std::chrono::system_clock::now()` for wall-clock time. | Code review of `console_sink.cpp`. |
| AC-034 | Destroyed entities and removed components correctly unregister their `Updatable` components from the internal `updatables_` vector (no dangling pointers, no crash on subsequent `update_updatables()` call). | Code review of `flush_destroyed()` and `remove_component<T>()` in `world.cpp`/`world.h`. Run an app that destroys entities with `Updatable` components — no crash. |

> **Investigation risk note**: If the deformation root cause proves to be in the PBR shader pipeline or requires changes explicitly excluded from this spec's scope (see [Non-goals](#non-goals)), the finding will be reported to the orchestrator for scope re-evaluation.

## E2E Verification

- **Method**: Launch the following commands and visually inspect the results:
  1. `buddd run free-camera` — verify identical camera behaviour to before refactoring.
  2. `buddd run phong` — verify identical behaviour (camera start pos, orbiting lights).
  3. `buddd run gltf-helmet` — verify helmet renders correctly, load time under 3s, camera controls work.
  4. Inspect console output of any scene — verify `[HH:MM:SS.fff] [LEVEL] [Tag] message` format.
  5. Inspect a log file produced with `--log-file=/tmp/test.log` — verify ISO 8601 format unchanged.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | Console log lines include a correct wall-clock timestamp with millisecond precision. |
| SC-002 | FreeCameraMovement can be added to any camera entity and provides full free-camera controls. |
| SC-003 | free_camera_app and phong_app compile and run identically after refactoring. |
| SC-004 | gltf-helmet app launches and displays the DamagedHelmet with correct geometry. |
| SC-005 | Helmet model load time is under 3 seconds. |
| SC-006 | Interactive apps (`free_camera_app`, `phong_app`) no longer manually iterate `FreeCameraMovement` in `render()`. The render loop auto-discovers and updates `Updatable` components via `World::update_updatables()`. |

## Edge cases

| ID | Edge case | Expected behaviour |
|---|---|---|
| EC-001 | `FreeCameraMovement::update()` called with `dt = 0` (first frame or paused). | No movement occurs. No division by zero. |
| EC-002 | `FreeCameraMovement` on an entity without `CameraComponent`. | `entity().get_component<CameraComponent>()` returns `nullopt` — component gracefully handles this: logs a warning once and returns `true` (no-op). No assert/crash. |
| EC-003 | Window not focused / no input events for an extended period. | `mouse_delta()` returns `(0,0)`, no movement. No drift. |
| EC-004 | Rapid right-click toggle. | State machine handles correctly: `prev_right_click_` tracks edge, not level. |
| EC-005 | `system_clock::now()` throwing an exception (extremely rare). | `ConsoleSink::write()` does NOT guard against clock exceptions; a `system_clock` failure is treated as a fatal program error. The function should NOT be `noexcept` (matching `FileSink::write()` behaviour) and should NOT have a try/catch around the chrono call. If `system_clock::now()` throws, the exception propagates naturally and terminates the program. |
| EC-006 | Helmet has 5 external textures, some may fail to load. | Fallback magenta texture used for missing textures. Log warning per texture. |
| EC-007 | Helmet has a single node with rotation `[0.7071, 0, -0, 0.7071]` (glTF quaternion). | Correctly converted to engine Quat(w,x,y,z) = (0.7071, 0.7071, 0, -0). Since -0 == 0, this should be (0.7071, 0.7071, 0, 0). |
| EC-008 | Index type is `Uint16` with 46356 indices and 14556 vertices. | Index buffer correctly read and vertex offset adjustments work. |
| EC-009 | Multiple root nodes in glTF scene. | All root nodes are traversed and added to the world. |
| EC-010 | An entity with an `Updatable` component is destroyed (via `destroy_entity()` + `flush_destroyed()`) or its `Updatable` component is removed (via `remove_component`). | The `Updatable*` pointer is removed from `updatables_` before the component destructor runs. No dangling pointer. The next `update_updatables()` call does not crash or exhibit undefined behaviour. |

## Error cases

| ID | Error case | Expected behaviour |
|---|---|---|
| ER-001 | `model_loader.cpp` fails to parse glTF file. | Error logged, app setup returns error, engine exits gracefully. |
| ER-002 | `load_gltf_texture` fails for one of the 5 external textures. | Warning logged, magenta fallback used. Rendering continues. |
| ER-003 | Window creation fails at 1280×720. | Engine falls back to 1024×768 (or whatever the default is). App continues. |
| ER-004 | `FreeCameraMovement::update` called before component is attached to an entity. | `entity()` returns handle with `world_ == nullptr`. `get_component<CameraComponent>()` returns `nullopt`. Component logs a warning once and returns `true` (no-op) — no crash. |
| ER-005 | The glTF file references non-existent textures or images. | Loading logs a warning, magenta fallback used, no crash. |

## Permissions and security

- No new permissions or security considerations apply.
- Console log output is plain text; no sensitive data is expected in logs.
- `system_clock::now()` has no security implications.

## Observability

| Aspect | Detail |
|---|---|
| Log output format | Console: `[HH:MM:SS.fff] [LEVEL] [Tag] message` — all timestamps reference `system_clock`. |
| File log format | Unchanged: `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message` |
| FreeCameraMovement | No internal logging (component is silent). Camera state changes are observable via camera position/orientation values logged externally if desired. |
| Helmet load time | Developers can use the new timestamps in console log output to measure load durations precisely. |

## Out of scope

- Normal mapping V2 (tangent space computation improvements).
- Alpha modes MASK/BLEND support in PBR.
- `KHR_materials_pbrSpecularGlossiness` extension support.
- Async/background model loading.
- Colourised/ANSI console output.
- Adding a CLI flag `--no-timestamps` (V1 intentionally simple).
- Unit tests for `FreeCameraMovement` (tested via integration in the three apps).
- Refactoring of `cube_app`, `cube_scene_app`, `asset_demo_app`, `hot_reload_app`, `gltf_demo_app`, `multi_material_app`, or any other app beyond `free_camera_app` and `phong_app`.
- Changing the PBR shader or material pipeline.
- Adding any new external dependencies.

## Assumptions

| ID | Assumption |
|---|---|
| A-001 | `std::chrono::system_clock::now()` provides sufficient precision (milliseconds) and is not prohibitively expensive for a once-per-log-message call. |
| A-002 | The engine process runs in a locale where `std::chrono::system_clock` returns reasonable values (no embedded systems with no RTC). |
| A-003 | The initial camera position in `gltf-helmet` is (0, 1.5, 3), looking at origin. |
| A-004 | The DamagedHelmet is loaded via `AssetManager::create<ModelAsset>` using a YAML metadata entry, which is the standard public loading path (matching `gltf_demo_app`). |
| A-005 | The DamagedHelmet load time can be improved by identifying redundant work (e.g., double texture loading, unnecessary copies) in the current `build_model_from_mesh` / `load_gltf_texture` pipeline — no fundamental algorithmic change is required. |
| A-006 | The free-camera default camera position for `gltf-helmet` app is (0, 1.5, 3) looking at origin, with perspective 55° FOV and 0.1/100 near/far. |
| A-007 | The `Window` class provides `set_mouse_capture(bool)` and `is_mouse_captured()` methods (as used in the existing code). |
| A-008 | The `Updatable` interface uses multiple inheritance orthogonal to `Component` — a component can inherit from both without changes to the `Component` base class. RTTI (`dynamic_cast`) still works correctly for `World::each<T>()` on the `Component` hierarchy. |
| A-009 | `World::add_component<T>()` template can use `if constexpr (std::is_base_of_v<Updatable, T>)` to conditionally register Updatable components, requiring `<type_traits>` already included (it is). |

## Documentation impact

### ADR-020 acknowledgement

This spec changes the ConsoleSink output format from `[LEVEL] [Tag] message` to `[HH:MM:SS.fff] [LEVEL] [Tag] message`. ADR-020 (accepted) explicitly states for ConsoleSink: *"No timestamp, no color."*

**Rationale for overriding**: The original ADR deferred timestamps for V1 simplicity. This feature adds millisecond timestamps to the console sink to aid real-time debugging of rendering and performance issues (specifically the DamagedHelmet investigation), responding to direct user feedback.

**ADR update required**: ADR-020 will need to be updated to reflect this change (or a new ADR created if the architect decides). See the list below.

### Documents requiring updates

| Document | Reason for update |
|---|---|
| `docs/adr/ADR-020-custom-logging-system.md` | ConsoleSink format changed from `[LEVEL] [Tag] message` to `[HH:MM:SS.fff] [LEVEL] [Tag] message`. Section about ConsoleSink design must be updated. |
| `docs/wiki/domain/logging.md` | Record new console log format, document new source tag `GltfHelmet`. |
| `docs/wiki/architecture/module-map.md` | Document new `FreeCameraMovement` component, `Updatable` interface, and `gltf-helmet` app. |
| `docs/wiki/architecture/data-flow.md` | Document `Updatable` auto-update step in the `run_app` render loop diagram. |

### New source tag

The `gltf-helmet` app introduces a new source tag: `GltfHelmet`. This should be added to the wiki's source tag registry in `docs/wiki/domain/logging.md`.

## Resolved questions

*All open questions from the initial draft have been answered:*

1. **Missing CameraComponent in FreeCameraMovement** — Log a warning once and return `true` (no-op). Do not assert/crash.
2. **Loading path for gltf-helmet** — Use `AssetManager::create<ModelAsset>` via YAML metadata (the standard public path, matching `gltf_demo_app`).
3. **Directional light setup** — White light, intensity 1.5, pitch=-45°, yaw=45° (same as `gltf_demo_app`).
4. **Default camera position** — Position (0, 1.5, 3), looking at origin.
5. **Infinite recursion risk in sink write** — No risk. `ConsoleSink::write()` uses `std::fprintf` to stderr; `FileSink::write()` uses `std::ofstream`. Neither calls `BUDDD_LOG_*`. No recursion possible.
