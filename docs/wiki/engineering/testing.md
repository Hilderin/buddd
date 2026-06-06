# Testing

## Test framework

The project uses **Catch2 v3** (v3.7.0) as its unit test framework. Catch2 is downloaded automatically via CMake's `FetchContent` — no manual installation is required.

The test binary is named **`buddd_tests`** and links against `buddd_engine` and `Catch2::Catch2WithMain`.

## Running tests

```bash
# Via ctest (preset-based)
ctest --preset debug
ctest --preset release

# Direct invocation
./build/debug/tests/buddd_tests
```

Both Debug and Release presets include a `testPreset` that runs all registered tests.

## Current tests

### Sanity test (bootstrap)

| Test case | Tags | Source | Verification |
|---|---|---|---|
| `engine version is non-empty` | `[sanity]` | `tests/version_test.cpp` | `buddd::engine::version()` returns a non-empty `std::string_view` |

### CLI integration tests

The project includes CLI integration tests tagged `[cli]` that invoke the `buddd` binary and verify its output. These tests run without a display:

| Test case | Verification |
|---|---|
| `buddd help outputs usage text` | stdout contains `"run"` as a listed command |
| `buddd help does not list old commands` | stdout does NOT contain `"demo"` or `"capture"` |
| `buddd help ignores extra arguments` | stdout contains usage text |
| `buddd version outputs correct version string` | stdout contains `"buddd 0.1.0"` |
| `buddd version ignores extra arguments` | stdout contains `"buddd 0.1.0"` (extra args ignored) |
| `buddd with no arguments defaults to run command` | stderr contains `"Window opened: 1024x768"` |
| `buddd unknowncommand exits with code 1` | stderr contains `"Unknown command: 'unknowncommand'"` + usage; exit code 1 |
| `buddd run with no scene defaults to empty window` | stderr contains `"Window opened: 1024x768"` |
| `buddd run unknownscene prints error and exits 1` | stderr contains `"Unknown scene: 'unknownscene'"`; exit code 1 |
| `buddd demo is unknown command` | stderr contains `"Unknown command: 'demo'"`; exit code 1 |
| `buddd capture is unknown command` | stderr contains `"Unknown command: 'capture'"`; exit code 1 |
| `buddd test is unknown command` | stderr contains `"Unknown command: 'test'"`; exit code 1 |
| `buddd run triangle runs and completes` | stderr contains `"Scene complete: triangle (120 frames rendered)"` or an engine init error |
| `buddd run cube runs and completes` | stderr contains `"Scene complete: cube (120 frames rendered)"` or an engine init error |
| `buddd run triangle --frame 10` | stderr contains `"Scene complete: triangle (10 frames rendered)"` or engine init error |
| `buddd run cube --capture 60:/tmp/test.png` | stderr contains capture messages, stdout contains `"Captured:"` |

### Headless platform abstraction tests

The platform abstraction layer introduces headless backend tests that run **without a display or GPU** — they are safe for CI:

| ID | Test case | Tags | Verification |
|---|---|---|---|
| T-01 | `Platform::create(Headless) succeeds` | `[headless]` `[platform]` | Returns valid `unique_ptr<Platform>` |
| T-02 | `Headless Platform creates Window with valid config` | `[headless]` `[window]` | `create_window()` returns valid window |
| T-03 | `Headless Window creates RenderDevice` | `[headless]` `[render]` | `RenderDevice::create()` returns valid device |
| T-04 | `Headless frame cycle completes` | `[headless]` `[render]` | `begin_frame()` / `end_frame()` sequence completes |
| T-05 | `Headless RenderDevice::size() returns correct dimensions` | `[headless]` `[render]` | `size()` matches window config |
| T-06 | `Headless Window::native_handle() returns nullptr` | `[headless]` `[window]` | `native_handle()` is `nullptr` |
| T-07 | `WindowConfig negative dimensions return error` | `[headless]` `[window]` | Returns `WindowCreationFailed` error |
| T-08 | `Error struct construction and to_string` | `[headless]` `[error]` | `to_string()` format is correct |
| T-09 | `make_error helper compiles and returns correct category` | `[headless]` `[error]` | Category and message are correct |
| T-10 | `make_error with explicit code` | `[headless]` `[error]` | `code` field is set correctly |
| T-11 | `Result<T> compiles with unique_ptr` | `[headless]` `[error]` | `Result<std::unique_ptr<int>>` compiles and works |

### SDL3 backend tests (offscreen driver)

SDL3 backend tests use SDL3's **offscreen video driver** (`SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`), so they do **not** require a physical display. They are conditionally compiled via the `BUDDD_HAS_DISPLAY` CMake option (default `ON`). Set `-DBUDDD_HAS_DISPLAY=OFF` to exclude them (e.g., in CI).

The `<SDL3/SDL.h>` include in these test files is permitted by amendment [AMEND-2026-001](/docs/adr/ADR-019-architecture-boundaries.md#amendment-amend-2026-001--sdl3-test-file-exception) — a narrow exception to the architecture boundary for testing SDL3-dependent engine functionality (hint-setting, synthetic event injection via `SDL_PushEvent()`, and SDL3 backend exercise).

| Test case | Tags | Source file | Verification |
|---|---|---|---|
| `Platform::create(SDL3) succeeds with offscreen driver` | `[sdl3][platform]` | `tests/sdl3_backend_test.cpp` | Returns valid `Platform` |
| `SDL3 Platform creates Window with valid config` | `[sdl3][window]` | `tests/sdl3_backend_test.cpp` | `create_window()` returns valid window |
| `SDL3 Window::native_handle() returns non-null` | `[sdl3][window]` | `tests/sdl3_backend_test.cpp` | `native_handle()` is non-null |
| `SDL3 Window dimensions match config` | `[sdl3][window]` | `tests/sdl3_backend_test.cpp` | `width()`/`height()` match config |
| `SDL3 RenderDevice creation` | `[sdl3][render]` | `tests/sdl3_backend_test.cpp` | `RenderDevice::create()` succeeds, `size()` matches |
| `SDL3 frame cycle completes` | `[sdl3][render]` | `tests/sdl3_backend_test.cpp` | `begin_frame()`/`end_frame()` sequence completes |

The old `T-13` (formerly `Platform::create(SDL3) success` with `[!mayfail]`) has been removed from `platform_abstraction_test.cpp`. It is replaced by the offscreen-driver-based test above (first row), which runs reliably in any environment including headless CI.

### Input system tests

The input system test suite (`tests/input_tests.cpp`) provides 17 test cases covering the `InputSystem` abstraction, `KeyCode` enum, double-buffered state model, factory, Platform integration, and SDL3 event processing. The suite is split into 9 headless (always-run) and 8 SDL3 (conditional on `BUDDD_HAS_DISPLAY`) tests.

Tags used: `[input]`, `[input_sdl3]`.

| ID | Test case | Tags | Verification |
|---|---|---|---|
| T-01 | `Factory creates headless InputSystem` | `[headless][input]` | `InputSystem::create(Backend::Headless)` returns valid `unique_ptr<InputSystem>`, `dynamic_cast` confirms `InputSystemHeadless` |
| T-02 | `Factory creates SDL3 InputSystem` | `[input_sdl3][input]` | `InputSystem::create(Backend::SDL3)` returns valid `unique_ptr<InputSystem>`, `dynamic_cast` confirms `InputSystemSDL3` |
| T-03 | `KeyCode enum values match SDL_Scancode` | `[input]` | `static_cast<uint8_t>(KeyCode::A) == 4`, `static_cast<uint8_t>(KeyCode::Escape) == 41`, compiler check via `static_assert` |
| T-04 | `KeyCode size is 1 byte` | `[input]` | `sizeof(KeyCode) == 1` |
| T-05 | `Headless InputSystem returns defaults for all queries` | `[headless][input]` | All `is_down/is_pressed/is_released` return false; `mouse_position/delta/wheel` return `(0,0)` |
| T-06 | `Double-buffered pressed/released transition` | `[headless][input]` | Simulate frame sequence: key down → `is_pressed=true`; next frame no events → `is_pressed=false`, `is_down=true`; key up → `is_released=true` |
| T-07 | `Delta and wheel reset after begin_frame` | `[headless][input]` | Simulate events, call `begin_frame()`, verify delta/wheel reset to zero |
| T-08 | `Unknown/out-of-range scancode maps to KeyCode::Unknown` | `[headless][input]` | Direct `InputSystemSDL3` test: invalid scancode produces `Unknown` key mapping |
| T-09 | `KeyCode bounds check rejects out-of-range values` | `[headless][input]` | Scancode >= `KeyCode::_Count` maps to `KeyCode::Unknown` |
| T-10 | `Platform::input_system() returns valid reference (SDL3)` | `[input_sdl3][input]` | `Platform::create(SDL3)` → `platform->input_system()` returns non-null `InputSystem&` |
| T-11 | `SDL3 platform processes keyboard events through InputSystem` | `[input_sdl3][input]` | Push synthetic `SDL_EVENT_KEY_DOWN` (W), run `poll_events()`, verify `is_down(KeyCode::W)=true` |
| T-12 | `SDL3 platform processes mouse motion events` | `[input_sdl3][input]` | Push synthetic `SDL_EVENT_MOUSE_MOTION` (100,200,rel 10,-5), verify position and delta |
| T-13 | `SDL3 platform processes mouse button events` | `[input_sdl3][input]` | Push button-down for each `MouseButton`, verify `is_mouse_down/is_mouse_pressed`; push button-up, verify `is_mouse_released` |
| T-14 | `SDL3 platform processes mouse wheel events` | `[input_sdl3][input]` | Push `SDL_EVENT_MOUSE_WHEEL` (x=1,y=2), verify `mouse_wheel()` returns (1,2) |
| T-15 | `Mouse wheel accumulates multiple events before begin_frame` | `[input_sdl3][input]` | Push two wheel events of (1,2), call `poll_events()` once, verify `mouse_wheel()` returns (2,4) |
| T-16 | `Mouse delta resets after frame` | `[input_sdl3][input]` | Push motion event, `poll_events()`, verify delta; second `poll_events()` with no events, verify delta is (0,0) |
| T-17 | `Mouse wheel resets after frame` | `[input_sdl3][input]` | Push wheel event, `poll_events()`, verify wheel; second `poll_events()` with no events, verify wheel is (0,0) |

### Scene graph tests

The scene graph test suite (`tests/scene_graph_tests.cpp`) provides 49 Catch2 v3 test cases covering all acceptance criteria from SPEC-008. All tests are **headless** (no display, no GPU required) and are compiled in **both** `BUDDD_HAS_DISPLAY` branches. The test file is registered in `tests/CMakeLists.txt`.

Tags used: `[scene]`, `[entity_id]`, `[transform]`, `[component]`, `[entity]`, `[world]`, `[hierarchy]`, `[destroy]`, `[null_entity]`, `[pending_destroy]`.

| Category | Test range | Coverage |
|---|---|---|
| EntityId | T-01 to T-04 | Default construction, `none()` sentinel, comparison, `static_assert` checks |
| Transform | T-05 to T-09 | Default values, `local_matrix()` TRS order, `world_matrix()` with parent/grandparent chains |
| Component | T-10 to T-16 | Base class, `add_component`/`get_component`/`remove_component`, unique per type, pending-destroy nullopt, const overload |
| Entity lifecycle | T-17 to T-22 | Create returns valid entity, `none()` null entity, comparison, transform modify persists, destroy/is_pending_destroy, idempotent destroy |
| World | T-23 to T-26 | `flush_destroyed` empty/when entities exist, reclaims entities, `destroy_entity` equivalence, flush idempotent |
| Hierarchy | T-27 to T-31 | `create_child` parent link, `child_count`/`get_child`, reparent to root/another parent, reparent no-op |
| Destroy cascade | T-32 to T-35 | Cascade to children, flush reclaims all, deep hierarchy (10,000) no stack overflow, no-op flush |
| world_matrix | T-36 to T-37 | Convenience method equivalence, chain with different transforms |
| Null entity safety | T-38 | Safe operations: `id()`, `is_pending_destroy()`, comparison |
| Pending-destroy | T-39 to T-40 | `get_component` returns nullopt, `transform()` accessible |
| UB contract | T-41 to T-42 | Null entity `get_component` nullopt, null entity `child_count` zero |
| Edge cases | T-43 to T-49 | Multiple flushes, World destructor with pending entities, destroyed visible in parent before flush, reverse depth order, component destructor called, World destruction with pending, stale EntityId after flush |

### Image / capture tests

The image test suite (`tests/image_tests.cpp`) provides unit tests for `ImageBuffer`, `Image`, PNG I/O, and error handling. All tests are **headless** (no display, no GPU required) — they use stb_image and stb_image_write which are CPU-only operations. The test file is registered in `tests/CMakeLists.txt` (auto-discovered via the `*_tests.cpp` glob).

Tags used: `[image]`.

| Category | Test case coverage |
|---|---|
| ImageBuffer | Default construction (zero-initialised aggregate) |
| Image::create validation | Zero width, zero height, zero channels, mismatched data size — each returns `InvalidArgument` error |
| Row-flipping | 4×2 greyscale buffer: bottom row = 0xFF, top row = 0x00; after create, image row 0 = 0x00, row 1 = 0xFF |
| Save/load round-trip | Create, save to temp PNG, verify magic bytes (`\x89PNG`), load back, compare pixel data |
| Load error cases | Non-existent file → `IoFailed`; corrupt file → `IoFailed` |
| Copy/move semantics | Static asserts for non-copyable; move construction transfers ownership; moved-from source has empty data |
| Accessors | width/height/channels/data() return stored values |
| Save error cases | Non-existent directory → `IoFailed`; path is a directory → `IoFailed` |

### Model / cube tests

The model test suite (`tests/model_tests.cpp`) provides 24 Catch2 v3 test cases covering the `Model` utility class and cube data verification. All tests are **headless** (no display, no GPU required) and are compiled in **both** `BUDDD_HAS_DISPLAY` branches. The test file is registered in `tests/CMakeLists.txt`.

Tags used: `[model]`, `[cube]`.

| Category | Test range | Coverage |
|---|---|---|
| Model factory | T-01 to T-08 | Default construction (null model), non-copyable/movable, `Model::create` and `create_indexed` with valid data, validation errors (empty data, zero stride, zero attributes) |
| Model accessors | T-09 to T-12 | `material()` (mutable and const), `vertices()`, `indices()` / `has_indices()` |
| Model draw | T-13 to T-16 | Draw on non-indexed, indexed, null, and moved-from models |
| Move semantics | T-17 to T-18 | Move constructor and move assignment transfer ownership |
| Cube data | T-19 to T-22 | 24 vertices/36 indices verification, `u_mvp` / `u_color` uniform checks, `set_uniform("u_mvp")` success |
| Demo run | T-23 | Simulated `run_cube_demo` loop (headless, 5 frames) completes without crash |
| Shared ownership | T-24 | Material stays alive via `shared_ptr` when original shared_ptr is reset |

### Phong lighting tests

The Phong lighting test suite (`tests/lighting_tests.cpp`) provides 32 Catch2 v3 test cases covering all acceptance criteria from SPEC-018. All tests are **headless** (no display, no GPU required) and use `EngineService::create(Backend::Headless, ...)`. The test file is auto-discovered via the `*_tests.cpp` glob.

Tags used: `[lighting]`, `[lighting][vertex]`, `[lighting][component]`, `[lighting][material]`, `[lighting][glsl]`, `[lighting][render]`, `[lighting][headless]`.

| AC ID | Test case | Verification |
|---|---|---|
| AC-001 | `"Vertex struct layout"` | `static_assert(sizeof(Vertex) == 72)`, `offsetof` for each field, `k_standard_vertex_format` has 6 attributes |
| AC-002 | `"DirectionalLightComponent construction and accessors"` | Create with params, verify/mutate via const/non-const accessors |
| AC-003 | `"PointLightComponent construction and accessors"` | Same, plus range (default 10.0) |
| AC-004 | `"SpotLightComponent construction and accessors"` | Same, plus inner_angle (0.785), outer_angle (1.047) |
| AC-005 | `"Light component on_attach no-op"` | Each component type added to entity — no crash, no world registration |
| AC-006 | `"PhongMaterial is a valid Material subclass"` | Create `PhongMaterial`, verify `has_uniform("u_model")` |
| AC-007 | `"PhongMaterial embedded shaders"` | Constructor does not accept external shaders |
| AC-008 | `"PhongMaterial convenience setters"` | Header declares `set_camera_position`, `set_lights`, `set_transforms` |
| AC-009 | `"PhongMaterial known_uniform_names"` | All 17 standard uniforms recognized by `has_uniform()` |
| AC-010 | `"glsl_util extract_uniform_names"` | Various GLSL snippets parsed correctly |
| AC-011 | `"glsl_util normalize_uniform_name"` | Array suffix stripping correct |
| AC-012 | `"LightData struct"` | `k_max_lights = 8`, all 6 fields present |
| AC-013 | `"RenderSystem collects directional lights"` | 3 lights → `u_light_count = 3`, direction from rotation |
| AC-014 | `"RenderSystem collects point lights"` | Point at (5,3,1) → w=1.0, position matches |
| AC-015 | `"RenderSystem collects spot lights"` | Position, direction, cone cosines, type=2.0 |
| AC-016 | `"RenderSystem caps at 8 lights"` | 10 lights → u_light_count == 8 |
| AC-017 | `"Light colour * intensity premultiplied"` | (0.5, 0.5, 0.5) × 2.0 → (1.0, 1.0, 1.0) |
| AC-018 | `"Normal matrix computation"` | `u_normal_mat` ≈ `world_mat.inverse().transpose()` |
| AC-019 | `"Backward compat: unlit material"` | `has_uniform("u_model")` false, only u_mvp set |
| AC-020 | `"RenderSystem sets u_camera_pos"` | Matches camera position |
| AC-021 | `"RenderSystem sets material property defaults"` | Ambient, specular, shininess, tint defaults verified |
| AC-022 | `"Light component entity destruction"` | Destroy one → count decreases |
| AC-023 | `"Zero lights renders with ambient only"` | Lit mesh with 0 lights: u_light_count=0, no crash |
| AC-024 | `"phong_shaders.h exists and compiles"` | Both shader constants non-empty |
| AC-025 | `"RenderSystem sets u_model"` | Matches entity world_matrix |
| AC-026 | `"MaterialHeadless array subscript normalization"` | Bracket-syntax get/set/has works correctly |
| AC-027 | `"MaterialHeadless diagnostic accessors"` | Vec3/Vec4/float/int getters: set→get match, missing→nullopt, type mismatch→nullopt |
| AC-028 | `"Phong demo exists and compiles"` | `phong_demo.h` declares `run_phong_demo` |
| AC-029 | `"glsl_util used by both backends"` | Both `render_device_headless.cpp` and `render_device_opengl.cpp` include `glsl_util.h` |
| AC-030 | `"Demo helpers use Vertex struct"` | `setup_triangle()` and `setup_cube()` use `Vertex` with stride 72 |
| AC-031 | `"Spot light cone uniforms"` | `inner_cones[i]` ≈ cos(angle), `spot_directions[i]` matches direction |
| AC-032 | `"glsl_util handles layout qualifiers"` | `layout(location=0) uniform vec4 u_x;` → {"u_x"} |

### Assertion tests

The assertion system test suite (`tests/assertion_tests.cpp`) provides 12 test cases covering all acceptance criteria from SPEC-023. Tests exercise: `LogLevel::Fatal` enum ordering, `debug_break()` compilation, `format_assertion_failure_message()` formatting (with and without custom message), Fatal-level log capture via `ScopedMemoryLogger`, `BUDDD_VERIFY` expression evaluation in all builds, non-double-evaluation of `BUDDD_ASSERT` / `BUDDD_VERIFY`, release-build expression omission for `BUDDD_ASSERT`, `BUDDD_FAIL_MSG` formatting, and the fixed `"Assert"` tag convention.

Tags used: `[assertion]`.

## Test conventions

- All assertions use `REQUIRE`/`REQUIRE_FALSE` (not `CHECK`).
- Headless tests are tagged `[headless]` plus a subsystem tag (`[platform]`, `[window]`, `[render]`, `[error]`).
- Test files go in `tests/` with the plural suffix `_tests.cpp` (per ADR-009). They are automatically discovered via `file(GLOB_RECURSE ... *_tests.cpp CONFIGURE_DEPENDS)` in `tests/CMakeLists.txt`.

## Adding tests

1. Add a new `.cpp` file in `tests/`.
2. Include the appropriate Catch2 header (`<catch2/catch_test_macros.hpp>`).
3. Write `TEST_CASE` blocks with descriptive names and tags.
4. New `*_tests.cpp` files are **auto-discovered** by `file(GLOB_RECURSE ... *_tests.cpp CONFIGURE_DEPENDS)` in `tests/CMakeLists.txt` — no manual CMakeLists.txt edit is needed. If the file is not picked up, re-run CMake configure (`cmake --build build/debug` or `cmake --preset debug`).
5. Tests are automatically discovered by `catch_discover_tests()`.

## Reference

- Spec: [SPEC-001](/.specs/sprint-2026-05/project-setup/spec.md) — AC-007 (version sanity test), AC-008 (FetchContent), AC-010 (ctest passes)
- Implementation contract: [IMPL-001](/.specs/sprint-2026-05/project-setup/implementation-contract.md) — sections 9 and 10 (test structure)
- Spec: [SPEC-002](/.specs/sprint-2026-05/platform-abstraction/spec.md) — Acceptance criteria, User stories (headless testing)
- Implementation contract: [IMPL-002](/.specs/sprint-2026-05/platform-abstraction/implementation-contract.md) — Required tests (T-01 through T-12)
- Spec: [SPEC-003](/.specs/sprint-2026-05/sdl3-backend-tests/spec.md) — SDL3 backend test specification
- Implementation contract: [IMPL-003](/.specs/sprint-2026-05/sdl3-backend-tests/implementation-contract.md) — SDL3 backend test implementation
- Spec: [SPEC-007](/.specs/sprint-2026-05/cli-command-evolution/spec.md) — CLI Command Evolution: Test implications, new CLI test cases
- Implementation contract: [IMPL-007](/.specs/sprint-2026-05/cli-command-evolution/implementation-contract.md) — Required tests (demo no name, demo unknownname, test unknown, demo triangle)
- Spec: [SPEC-008](/.specs/sprint-2026-05/scene-graph/spec.md) — Scene Graph: Acceptance criteria (AC-001 through AC-032), Edge cases, Test coverage requirements
- Implementation contract: [IMPL-008](/.specs/sprint-2026-05/scene-graph/implementation-contract.md) — Required tests (T-01 through T-49), test conventions, pending-destroy contract verification
- Spec: [SPEC-009](/.specs/sprint-2026-05/3d-cube-demo/spec.md) — Model Utility & 3D Cube Demo: Acceptance criteria (AC-001 through AC-027), test coverage requirements
- Implementation contract: [IMPL-009](/.specs/sprint-2026-05/3d-cube-demo/implementation-contract.md) — Required tests (T-01 through T-24), headless test conventions
- Spec: [SPEC-010](/.specs/sprint-2026-05/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario)
- Implementation contract: [IMPL-010](/.specs/sprint-2026-05/capture/implementation-contract.md)
- Spec: [SPEC-018](/.specs/sprint-2026-05/lighting/spec.md) — Phong Lighting System (Standard Vertex, Light Components, Phong module, RenderSystem extension, Phong demo)
- Implementation contract: [IMPL-018-002](/.specs/sprint-2026-05/lighting/implementation-contract.md) — Phong Lighting System implementation contract
