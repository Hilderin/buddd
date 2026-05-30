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
| `buddd help outputs usage text` | stdout contains `"demo"` as a listed command |
| `buddd help ignores extra arguments` | stdout contains updated usage text |
| `buddd version outputs correct version string` | stdout contains `"buddd 0.1.0"` |
| `buddd version ignores extra arguments` | stdout contains `"buddd 0.1.0"` (extra args ignored) |
| `buddd with no arguments defaults to run command` | stdout contains `"Window opened: 1024x768"` |
| `buddd unknowncommand exits with code 1` | stderr contains `"Unknown command: 'unknowncommand'"` + usage; exit code 1 |
| `buddd demo with no name prints usage and exits 1` | stderr contains `"Usage: buddd demo <demo>"`; exit code 1 |
| `buddd demo unknownname prints error and exits 1` | stderr contains `"Unknown demo: 'unknownname'"`; exit code 1 |
| `buddd test is unknown command` | stderr contains `"Unknown command: 'test'"`; exit code 1 |
| `buddd demo triangle runs and completes` (guarded by `BUDDD_HAS_DISPLAY`) | stderr contains `"Demo complete: triangle (120 frames rendered)"` or an engine init error |

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

The `<SDL3/SDL.h>` include in these test files is permitted by constitutional amendment [AMEND-2026-001](/docs/constitution/rules/CONST-001-architecture-boundaries.md#amendment-amend-2026-001--sdl3-test-file-exception) — a narrow exception to CONST-001 for setting video driver hints.

| Test case | Tags | Source file | Verification |
|---|---|---|---|
| `Platform::create(SDL3) succeeds with offscreen driver` | `[sdl3][platform]` | `tests/sdl3_backend_test.cpp` | Returns valid `Platform` |
| `SDL3 Platform creates Window with valid config` | `[sdl3][window]` | `tests/sdl3_backend_test.cpp` | `create_window()` returns valid window |
| `SDL3 Window::native_handle() returns non-null` | `[sdl3][window]` | `tests/sdl3_backend_test.cpp` | `native_handle()` is non-null |
| `SDL3 Window dimensions match config` | `[sdl3][window]` | `tests/sdl3_backend_test.cpp` | `width()`/`height()` match config |
| `SDL3 RenderDevice creation` | `[sdl3][render]` | `tests/sdl3_backend_test.cpp` | `RenderDevice::create()` succeeds, `size()` matches |
| `SDL3 frame cycle completes` | `[sdl3][render]` | `tests/sdl3_backend_test.cpp` | `begin_frame()`/`end_frame()` sequence completes |

The old `T-13` (formerly `Platform::create(SDL3) success` with `[!mayfail]`) has been removed from `platform_abstraction_test.cpp`. It is replaced by the offscreen-driver-based test above (first row), which runs reliably in any environment including headless CI.

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

## Test conventions

- All assertions use `REQUIRE`/`REQUIRE_FALSE` (not `CHECK`).
- Headless tests are tagged `[headless]` plus a subsystem tag (`[platform]`, `[window]`, `[render]`, `[error]`).
- Test files go in `tests/` and are registered in `tests/CMakeLists.txt`.

## Adding tests

1. Add a new `.cpp` file in `tests/`.
2. Include the appropriate Catch2 header (`<catch2/catch_test_macros.hpp>`).
3. Write `TEST_CASE` blocks with descriptive names and tags.
4. Register the new source file in `tests/CMakeLists.txt` (append to the `add_executable` call).
5. Tests are automatically discovered by `catch_discover_tests()`.

## Constitution reference

[CONST-002 (Testing Policy)](/docs/constitution/rules/CONST-002-testing-policy.md) requires that all testable code must have corresponding unit tests and those tests must pass.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — AC-007 (version sanity test), AC-008 (FetchContent), AC-010 (ctest passes)
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 9 and 10 (test structure)
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — Acceptance criteria, User stories (headless testing)
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — Required tests (T-01 through T-12)
- Spec: [SPEC-003](/docs/specs/sdl3-backend-tests/spec.md) — SDL3 backend test specification
- Implementation contract: [IMPL-003](/docs/specs/sdl3-backend-tests/implementation-contract.md) — SDL3 backend test implementation
- Spec: [SPEC-007](/docs/specs/cli-command-evolution/spec.md) — CLI Command Evolution: Test implications, new CLI test cases
- Implementation contract: [IMPL-007](/docs/specs/cli-command-evolution/implementation-contract.md) — Required tests (demo no name, demo unknownname, test unknown, demo triangle)
- Spec: [SPEC-008](/docs/specs/scene-graph/spec.md) — Scene Graph: Acceptance criteria (AC-001 through AC-032), Edge cases, Test coverage requirements
- Implementation contract: [IMPL-008](/docs/specs/scene-graph/implementation-contract.md) — Required tests (T-01 through T-49), test conventions, pending-destroy contract verification
