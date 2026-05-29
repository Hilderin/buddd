# SPEC-003 — SDL3 Backend Tests

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|
| Approved by | Guillaume |
| Date | 2026-05-29 |
| Time | 17:52 UTC |

## Problem

The SDL3 backend (`PlatformSDL3`, `WindowSDL3`, `RenderDeviceOpenGL`) has only minimal test coverage:

- **T-12** (`"Backend enum values exist"`) verifies that `Backend::SDL3` and `Backend::Headless` compile — it tests nothing at runtime.
- **T-13** (`"Platform::create(SDL3) success"`) is marked `[!mayfail]` because it requires a physical display. In practice it is never run in CI and is treated as a deferred item. This spec moves T-13 into the new conditional test file with the dummy driver.

This means the entire real backend — window creation, OpenGL context creation, frame lifecycle — is untested. Defects such as incorrect `SDL_GL_SetAttribute` calls, missing `SDL_WINDOW_OPENGL` flags, or broken `begin_frame`/`end_frame` cycles would go undetected until manual testing on a graphical workstation.

Additionally, the implementation contract (IMPL-002, "Required tests" section) explicitly notes that T-13 and other SDL3-specific tests "should be conditionally compiled (e.g., guarded by `#ifdef BUDDD_HAS_DISPLAY`)." No such guard exists today, and no real SDL3 tests have been written.

## Goals

- Create a new test file `tests/sdl3_backend_test.cpp` with real runtime tests for the SDL3 backend.
- Use SDL3's offscreen video driver (`SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`) before initialising the SDL3 backend so tests can run without a physical display or GPU.
- Verify the full lifecycle: `Platform::create(SDL3)` → `create_window()` → `RenderDevice::create()` → `begin_frame()` → `end_frame()` → destruction.
- Add a `BUDDD_HAS_DISPLAY` CMake option (default `ON`) that controls whether SDL3 backend tests are compiled.
- When `BUDDD_HAS_DISPLAY=OFF`, SDL3 tests are excluded from the `buddd_tests` executable at compile time and produce zero build artifacts.
- All new tests pass when `BUDDD_HAS_DISPLAY=ON` on a machine with SDL3's offscreen video driver (including headless CI runners that have the SDL3 library available).
- Keep all existing headless tests (T-01 through T-11) unchanged. Move T-13 (`Platform::create(SDL3) success`) into the conditional test file; T-12 remains unchanged.
- Add a GitHub Actions CI workflow that builds and tests with `BUDDD_HAS_DISPLAY=OFF` to validate CI compatibility.

## Non-goals

- No changes to the headless tests in `tests/platform_abstraction_test.cpp`.
- No changes to any source code in `src/engine/` (headers, implementation, or `CMakeLists.txt`).
- No changes to `CMakePresets.json` or other build infrastructure beyond `CMakeLists.txt` files.
- The root `CMakeLists.txt` is modified only to add the `option(BUDDD_HAS_DISPLAY ...)` definition. No other changes.
- A GitHub Actions CI workflow is added with `BUDDD_HAS_DISPLAY=OFF` to validate that tests compile and pass without SDL3 backend tests. No other CI pipeline changes.
- No addition of new engine APIs, factory overloads, or configuration structs to support the offscreen driver. Tests set the SDL3 hint directly.
- T-13 is removed from `tests/platform_abstraction_test.cpp` and replaced by the equivalent test in `tests/sdl3_backend_test.cpp` (with offscreen driver, no `[!mayfail]`).
- No integration tests, visual regression tests, or performance benchmarks.
- No test for OpenGL version string verification or debug context flag (those belong in a render-device-specific spec if needed later).

### GitHub Actions CI workflow

A GitHub Actions CI workflow `.github/workflows/ci.yml` is added with the following job:

- **Build and test with `BUDDD_HAS_DISPLAY=OFF`**: Configures with `cmake -DBUDDD_HAS_DISPLAY=OFF --preset debug`, builds, and runs `ctest --preset debug`.
- Runs on `ubuntu-latest` with standard dependencies (C++26 compiler, CMake, Ninja, SDL3 via `FetchContent`).
- The job verifies that:
  - The project builds without SDL3 backend tests.
  - All headless tests (T-01 through T-11) pass.
  - No SDL3 test code is compiled (the `sdl3_backend_test.cpp` file is excluded).
  - T-13 is no longer present in the test binary.

No other CI changes (release builds, additional presets, or GPU-enabled runners) are added.

## Actors

| Actor | Description |
|---|---|
| Engine developer | Writes and runs SDL3 backend tests to validate platform code during development. Benefits from early detection of regressions in `PlatformSDL3`, `WindowSDL3`, and `RenderDeviceOpenGL`. |
| CI maintainer | Configures builds with `-DBUDDD_HAS_DISPLAY=OFF` to exclude SDL3 tests from CI runs that have no display server. |
| Code reviewer | Reviews the new test file and `tests/CMakeLists.txt` changes for correctness, constitution compliance, and test quality. |

## User-visible behavior

- A new CMake option `BUDDD_HAS_DISPLAY` (boolean, default `ON`) appears in the CMake configuration.
- When `BUDDD_HAS_DISPLAY=ON`:
  - `tests/sdl3_backend_test.cpp` is compiled and linked into `buddd_tests`.
  - Running `ctest` or `./build/debug/tests/buddd_tests` executes the SDL3 backend tests (alongside all existing tests).
  - No physical display is required: all tests use the `offscreen` video driver (if available) to attempt OpenGL context creation.
- When `BUDDD_HAS_DISPLAY=OFF`:
  - `tests/sdl3_backend_test.cpp` is **not** compiled. No SDL3 test symbols exist in `buddd_tests`.
  - Existing headless tests (T-01 through T-11) and T-12 still compile and run as before.
  - T-13 is removed from `platform_abstraction_test.cpp` entirely — it is replaced by the equivalent test in `sdl3_backend_test.cpp` (with offscreen driver, no `[!mayfail]`).
- Running `cmake -DBUDDD_HAS_DISPLAY=OFF` disables all SDL3 backend tests without any source-code changes.

## User stories

### Story 1 — Run SDL3 backend tests without a display (Priority: P1)

As an engine developer, I want to run real SDL3 backend tests on a headless machine (e.g., my CI runner or WSL without X11) so that I catch regressions in `PlatformSDL3`, `WindowSDL3`, and `RenderDeviceOpenGL` before merging.

**Given** a machine with SDL3 installed but no physical display (no X11, no Wayland, no GPU)

**When** I configure with `cmake -DBUDDD_HAS_DISPLAY=ON` (the default), build, and run the tests

**Then** the SDL3 backend tests execute using the offscreen video driver, and all pass without a visible window or GPU.

**Given** the same headless machine

**When** I configure with `cmake -DBUDDD_HAS_DISPLAY=OFF`, build, and run the tests

**Then** no SDL3 test code is compiled, and the test binary runs only the headless tests (T-01 through T-11) and the trivial T-12 test.

### Story 2 — Disable SDL3 tests in CI with a single flag (Priority: P1)

As a CI maintainer, I want to turn off SDL3 backend tests with a single CMake flag so that my CI pipeline does not need a display server or GPU.

**Given** a CI configuration file (e.g., `.github/workflows/ci.yml`)

**When** I pass `-DBUDDD_HAS_DISPLAY=OFF` to the CMake configure step

**Then** the build skips all SDL3 test compilation, completes faster, and does not fail due to missing display capabilities.

### Video driver strategy by test type

The SDL3 video driver is selected per test case based on the test's needs:

- **All tests** (platform, window, and render device): Use `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")` — the offscreen driver supports SDL window creation with `SDL_WINDOW_OPENGL` and `SDL_GL_CreateContext` with OpenGL 4.5 Core profile when available. (The engine hardcodes `SDL_WINDOW_OPENGL` in `platform_sdl3.cpp`, so the dummy driver — which does not support OpenGL — cannot create windows.)

No fallback or retry logic is needed. If the offscreen driver does not support the required operations, the test fails with a clear error message.

### Story 3 — Verify the full SDL3 backend lifecycle (Priority: P2)

As an engine developer, I want to test the complete create-use-destroy cycle of the SDL3 backend so that I am confident the backend works end-to-end.

**Given** the offscreen video driver hint is set

**When** I create a platform, create a window, create a render device, call `begin_frame()` and `end_frame()`, and then let all objects go out of scope

**Then** each step succeeds, no crash occurs, and SDL3 resources are cleaned up on destruction (verifiable by a subsequent successful `Platform::create(Backend::SDL3)` call in the same process).

### Story 4 — Verify window and render device properties (Priority: P2)

As an engine developer, I want to confirm that `Window::size()`, `Window::native_handle()`, and `RenderDevice::size()` return meaningful values for the SDL3 backend.

**Given** a running SDL3 backend with a window and render device

**When** I query `window.width()`, `window.height()`, `window.native_handle()`, and `device.size()`

**Then** the dimensions match the `WindowConfig` used at creation time, and `native_handle()` returns a non-null pointer.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | A file `tests/sdl3_backend_test.cpp` exists and contains SDL3 backend unit tests. | File exists at the expected path. |
| AC-002 | Tests in `tests/sdl3_backend_test.cpp` set the offscreen video driver hint before any `Platform::create(Backend::SDL3)` call. All test cases use `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`. No fallback/retry logic is used. | Inspection of test file confirms the hint is set at the start of each relevant `TEST_CASE` or in a global fixture. |
| AC-003 | A test verifies that `Platform::create(Backend::SDL3)` succeeds under the offscreen driver. | Test passes when `BUDDD_HAS_DISPLAY=ON` on a machine with SDL3. |
| AC-004 | A test verifies that `create_window()` on an SDL3 platform succeeds under the offscreen driver. | Test passes; window dimensions match the config (e.g., 800×600). |
| AC-005 | A test verifies that `RenderDevice::create(Window&)` succeeds under the offscreen driver with an SDL3 window. | Test passes; `device.size()` matches window dimensions. |
| AC-006 | A test verifies the full frame cycle (`begin_frame()` followed by `end_frame()`) completes without error under the offscreen driver. | Test completes without crash or exception. |
| AC-007 | A test verifies that `Window::native_handle()` returns a non-null pointer for the SDL3 backend. | `REQUIRE(window.native_handle() != nullptr)` passes. |
| AC-008 | A test verifies that `Window::width()` and `Window::height()` return the values passed to `WindowConfig`. | `REQUIRE(window.width() == 800)` and `REQUIRE(window.height() == 600)` pass for a window created with those dimensions. |
| AC-009 | A test verifies that `RenderDevice::size()` returns the same dimensions as the window. | `REQUIRE(device.size() == std::pair{800, 600})` passes. |
| AC-010 | SDL3 test code is guarded by `#ifdef BUDDD_HAS_DISPLAY` / `#endif` in the source file. | The test file has `#ifdef BUDDD_HAS_DISPLAY` around all `TEST_CASE` blocks. |
| AC-011 | A CMake option `BUDDD_HAS_DISPLAY` exists, defaults to `ON`, and is defined in the root `CMakeLists.txt` (via `option(BUDDD_HAS_DISPLAY ...)`). | `cmake -LA | grep BUDDD_HAS_DISPLAY` shows `BUDDD_HAS_DISPLAY:BOOL=ON`. |
| AC-012 | When `BUDDD_HAS_DISPLAY=OFF`, the file `tests/sdl3_backend_test.cpp` is not compiled into `buddd_tests`. | Building with `-DBUDDD_HAS_DISPLAY=OFF` succeeds; the test binary contains no symbols from `sdl3_backend_test.cpp`. |
| AC-013 | When `BUDDD_HAS_DISPLAY=ON`, the SDL3 tests are compiled and can be discovered by Catch2. | `ctest --preset debug --list-tests` (or `./build/debug/tests/buddd_tests --list-tests`) shows the new test cases. |
| AC-014 | Existing headless tests (T-01 through T-11) compile and pass unchanged with both `BUDDD_HAS_DISPLAY=ON` and `BUDDD_HAS_DISPLAY=OFF`. | `ctest --preset debug` passes all headless tests in both configurations. |
| AC-015 | T-13 is removed from `tests/platform_abstraction_test.cpp`. T-12 (enum values) remains unchanged. | `git diff tests/platform_abstraction_test.cpp` shows only the removal of T-13 (no other changes). |
| AC-016 | All assertions in the new test file use `REQUIRE` / `REQUIRE_FALSE` (not `CHECK`), consistent with project conventions. | Inspection of the test file confirms only `REQUIRE` / `REQUIRE_FALSE` macros are used. |
| AC-017 | When `BUDDD_HAS_DISPLAY=ON`, `tests/CMakeLists.txt` adds `target_compile_definitions(buddd_tests PRIVATE BUDDD_HAS_DISPLAY)` to activate the `#ifdef` guard. | Inspection of `tests/CMakeLists.txt` confirms the `target_compile_definitions` call is present and conditional. |
| AC-018 | All new test cases in `tests/sdl3_backend_test.cpp` carry at least the `[sdl3]` tag for consistent filtering. | Inspection of the test file confirms every `TEST_CASE` includes the `[sdl3]` tag. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An engine developer can run all SDL3 backend tests on a headless machine (no display, no GPU) using the offscreen driver, and all tests pass. | Run `cmake --preset debug && cmake --build --preset debug && ctest --preset debug` on a machine without a display — SDL3 tests pass. |
| SC-002 | CI can exclude SDL3 backend tests by adding a single CMake flag `-DBUDDD_HAS_DISPLAY=OFF`. | Configure with `-DBUDDD_HAS_DISPLAY=OFF`, build, run tests — SDL3 test source is not compiled, no SDL3 test runs. |
| SC-003 | The full SDL3 backend lifecycle (Platform → Window → RenderDevice → begin_frame → end_frame → destruction) is tested at least once in the test suite. | All six lifecycle phases appear across the test cases; no phase is untested. |
| SC-004 | The new test file compiles with zero warnings on the reference compiler (GCC 14+ or Clang 19+) with `-Wall -Wextra`. | Build output shows zero warnings from `tests/sdl3_backend_test.cpp`. |

## Edge cases

| Case | Expected behavior |
|---|---|
| Dummy/offscreen video driver not available (e.g., SDL3 compiled without the driver) | `SDL_SetHint` returns `true` (it only sets a hint, does not validate the driver exists). `SDL_Init(SDL_INIT_VIDEO)` will fail. `Platform::create(SDL3)` returns `Result` with `Category::InitFailed`. The test reports failure with `REQUIRE(platform.has_value())`. |
| Offscreen driver is available but OpenGL 4.5 Core is not | `SDL_GL_CreateContext` fails inside `RenderDevice::create()`, which returns `Result` with `Category::RenderDeviceCreationFailed`. The render device test reports this failure with a clear error message — no fallback is attempted. |
| Offscreen driver is not available for render device tests | Render device tests use `"offscreen"` directly. If `Platform::create(SDL3)` fails (no offscreen driver), the test reports failure. No fallback — the test requires an OpenGL-capable driver. This is expected and acceptable: on machines without offscreen support, OpenGL tests will not run. |
| `BUDDD_HAS_DISPLAY=OFF` on a machine that does have a display | SDL3 tests are excluded from compilation. The user must reconfigure with `-DBUDDD_HAS_DISPLAY=ON` to run them. This is by design — the flag allows explicit selection. |
| `BUDDD_HAS_DISPLAY=ON` on a machine with no display and no offscreen driver support | SDL3 tests are compiled. At runtime, `Platform::create(SDL3)` fails. Test failures are reported. The user should set `-DBUDDD_HAS_DISPLAY=OFF` on such machines. |
| `Platform::create(SDL3)` succeeds but `create_window()` fails under offscreen driver | This would indicate a problem with the offscreen driver's window creation. The test for window creation checks for failure and reports it. |
| Window created but `RenderDevice::create()` fails (e.g., offscreen driver has no GL support) | The test for render device creation checks for failure and reports it. The window is still valid and must be destroyed cleanly. |
| A previous test in the same process left SDL3 in an unclean state (e.g., didn't call `SDL_Quit`) | Each test starts with a fresh scope; `unique_ptr` destruction ensures `SDL_Quit` is called. If a prior test crashes, subsequent tests may fail. Catch2's test isolation is relied upon. This is acceptable for unit tests. |
| `SDL_SetHint` is called multiple times (once per test) | `SDL_SetHint` is idempotent for the same hint key and value. Calling it multiple times is safe. |

## Error cases

| Case | Expected behavior |
|---|---|
| SDL3 library not available at link time | `buddd_engine` already links SDL3::SDL3 publicly. If SDL3 is unavailable, the build fails at CMake configure time (via `FetchContent`). The SDL3 backend tests are never compiled because the engine itself cannot build. |
| `SDL_SetHint` returns `false` (for either driver) | `SDL_SetHint` rarely returns `false`. If it does, the hint key is unknown or value rejected. The test does not check the return value — `Platform::create(SDL3)` will likely fail afterward with a clear error. |
| `Platform::create(Backend::SDL3)` returns error (InitFailed) under offscreen driver | All tests fail with `REQUIRE(platform.has_value())`. This indicates the offscreen driver is not available on this platform. |
| `create_window()` returns error (WindowCreationFailed) | The test reports the failure. The platform is still valid. |
| `RenderDevice::create()` returns error (RenderDeviceCreationFailed) | The test reports the failure. The platform and window are still valid. |
| `begin_frame()` or `end_frame()` crashes | Crash would terminate the test process. This indicates a serious defect in the OpenGL backend. Catch2 cannot recover from a segfault — this is acceptable. |
| Object destruction order is violated (e.g., `unique_ptr<RenderDevice>` destroyed after `unique_ptr<Platform>`) | Since `Platform` must outlive `RenderDevice`, destruction order in the test must be explicit (e.g., `device.reset()` before `platform.reset()`, or rely on declaration-order destruction). The spec requires tests to follow correct lifecycle order. |

## Permissions and security

- No elevated privileges are required to run the SDL3 backend tests.
- The offscreen video driver does not access the display server or GPU hardware — it is a software-only driver inside SDL3.
- No secrets, credentials, or environment variables are consumed.
- The `BUDDD_HAS_DISPLAY` option is a build-time switch with no runtime security implications.
- **CONST-001 amendment (resolved per Q-01)**: The new test file `tests/sdl3_backend_test.cpp` includes `<SDL3/SDL.h>` to call `SDL_SetHint()`. This requires amending CONST-001 to add a narrow exception for SDL3 test files. The amendment adds the following text to CONST-001 (see the constitution amendment process for details): "Exception: SDL3 test files (`tests/*_sdl3*.cpp`) conditionally compiled with `BUDDD_HAS_DISPLAY=ON` may include `<SDL3/SDL.h>` only for the purpose of setting video driver hints before `Platform::create()`." The exception is justified because:
  - The file exclusively tests the SDL3 backend implementation and is inherently SDL3-aware.
  - SDL3 headers are transitively available via `buddd_engine`'s PUBLIC link of `SDL3::SDL3`.
  - The file is conditionally compiled only when `BUDDD_HAS_DISPLAY=ON`.
  - The exception is narrow: it applies only to SDL3 test files and only for setting video driver hints. No other code outside `src/engine/` includes SDL3 headers.
- **SPEC-002 AC-015 note**: SPEC-002 AC-015 enforces the same architecture boundary. This spec (SPEC-003) narrows that rule for SDL3 test files as described above. SPEC-002 remains authoritative for all non-test code and for test files not conditionally compiled with `BUDDD_HAS_DISPLAY`.

## Observability

The tests use Catch2's built-in reporting. No additional logging is required.

| Signal | Source |
|---|---|
| Test pass/fail summary | Catch2 console reporter (stdout) |
| Individual assertion failures | Catch2 reports the exact `REQUIRE` line and values |
| Build-time exclusion of SDL3 tests | CMake status message: `-- BUDDD_HAS_DISPLAY=OFF: SDL3 backend tests excluded` |
| Build-time inclusion of SDL3 tests | CMake status message: `-- BUDDD_HAS_DISPLAY=ON: SDL3 backend tests enabled` |

## Out of scope

- No changes to `src/engine/` source code.
- No changes to existing test files (`tests/platform_abstraction_test.cpp`, `tests/version_test.cpp`).
- No changes to `CMakePresets.json`.
- No new engine APIs, configuration structs, or factory overloads.
- No OpenGL version string testing or debug context verification.
- No multiple-window testing (single window only).
- No stress tests, leak tests, or thread-safety tests.
- No test for `Platform::create()` with an invalid `Backend` enum value (covered by existing tests).
- T-13 is replaced by the equivalent test in `tests/sdl3_backend_test.cpp`.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")` is the SDL3 API to select the offscreen video driver. The constant name `SDL_HINT_VIDEO_DRIVER` was confirmed against the SDL3 headers. |

| A-02 | The offscreen video driver supports `SDL_CreateWindow` with the `SDL_WINDOW_OPENGL` flag and supports `SDL_GL_CreateContext` with OpenGL 4.5 Core profile. This has been verified (tests 18 and 19 pass with the offscreen driver). |

| A-03 | The dummy video driver does **not** support `SDL_CreateWindow` with the `SDL_WINDOW_OPENGL` flag (hardcoded in `platform_sdl3.cpp`). All tests use the offscreen driver instead. |
| A-04 | `buddd_engine` links SDL3::SDL3 as `PUBLIC`, making SDL3 headers and library available to test code that links `buddd_engine`. This is true based on `src/engine/CMakeLists.txt` line 25–27 (`target_link_libraries(buddd_engine PUBLIC SDL3::SDL3 OpenGL::GL)`). |
| A-05 | The `BUDDD_HAS_DISPLAY` CMake option is defined in the **root** `CMakeLists.txt` (not in `tests/CMakeLists.txt`), making it visible globally and accessible to CI scripts. |
| A-06 | When `BUDDD_HAS_DISPLAY=OFF`, the test file is excluded from the source list of `buddd_tests`. The `target_compile_definitions` for `BUDDD_HAS_DISPLAY` is NOT added, so the source-level `#ifdef` guard is not active on the excluded file (which is never compiled). |
| A-07 | All new tests follow the existing convention: `REQUIRE`/`REQUIRE_FALSE` (not `CHECK`), tag names use `[sdl3]` + subsystem tag (`[platform]`, `[window]`, `[render]`), and the file uses `<catch2/catch_test_macros.hpp>`. |
| A-08 | T-12 remains in `tests/platform_abstraction_test.cpp` unchanged. T-13 is removed from `platform_abstraction_test.cpp` and replaced by an equivalent test in `tests/sdl3_backend_test.cpp` (with offscreen driver, no `[!mayfail]`). |
| A-09 | The `Window` must outlive the `RenderDevice`, and `Platform` must outlive both. Tests follow declaration-order destruction or explicit `reset()` calls to ensure correct lifecycle order. |
| A-10 | No test in the new file modifies global SDL3 state (beyond the offscreen driver hint) that would leak to other tests. Each `TEST_CASE` creates and destroys its own `Platform`, `Window`, and `RenderDevice`. |
| A-11 | The file name `tests/sdl3_backend_test.cpp` is covered by AMEND-2026-001's "or similar" clause in the glob pattern `tests/*_sdl3*.cpp`, since it clearly targets the SDL3 backend. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | [RESOLVED] **CONST-001 violation**: The new test file must `#include <SDL3/SDL.h>` to call `SDL_SetHint()`. This directly violates CONST-001 ("No code outside `src/engine/` may include platform, graphics, or windowing library headers"). **Decision**: Amend CONST-001 to add a narrow exception for SDL3 test files. The amendment adds the following: "Exception: SDL3 test files (`tests/*_sdl3*.cpp`) conditionally compiled with `BUDDD_HAS_DISPLAY=ON` may include `<SDL3/SDL.h>` only for the purpose of setting video driver hints before `Platform::create()`." The exception is justified because: (1) the file exclusively tests the SDL3 backend; (2) SDL3 headers are transitively available via `buddd_engine`'s PUBLIC link of `SDL3::SDL3`; (3) the file is conditionally compiled only when `BUDDD_HAS_DISPLAY=ON`; (4) no other code outside `src/engine/` includes SDL3 headers. | **Scope**: Constitution amendment required before spec can be accepted. |
| Q-02 | [RESOLVED] **Video driver strategy**: The engine hardcodes `SDL_WINDOW_OPENGL` in `platform_sdl3.cpp`, so the dummy driver (which does not support OpenGL) cannot create windows. The `offscreen` driver supports both window and OpenGL context creation when available. **Decision**: All tests use `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`. If the offscreen driver is unavailable or doesn't support the required operations, the test fails gracefully — no retry or fallback. | **Scope**: Test coverage — all SDL3 tests use the offscreen driver. |
| Q-03 | [RESOLVED] **Where to define `BUDDD_HAS_DISPLAY`**: Defined in the **root `CMakeLists.txt** alongside `enable_testing()` and `add_subdirectory(tests)`, making it visible globally and accessible to CI scripts. | **Scope**: CMake organisation — option(`BUDDD_HAS_DISPLAY` ...) in root `CMakeLists.txt`, consumed in `tests/CMakeLists.txt`. |
