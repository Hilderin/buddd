# IMPL-003 — SDL3 Backend Tests

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

## Source spec

`.specs/sprint-2026-05/sdl3-backend-tests/spec.md` (SPEC-003), accepted with warnings (`.specs/sprint-2026-05/sdl3-backend-tests/spec-critic.md` verdict: `Accepted with warnings`, all blocking issues resolved).

The spec has been updated to resolve all previously identified inconsistencies. Two minor Out of scope items are noted:
- The Out of scope section initially stated "No changes to the root `CMakeLists.txt`" — this has been fixed in the current spec version, and the contract aligns with the updated non-goals.
- The Out of scope section also states "No changes to existing test files (`tests/platform_abstraction_test.cpp`)" — however, the non-goals specify removing T-13 from that file. The contract follows the non-goals (higher precedence within the spec). This contract modifies `tests/platform_abstraction_test.cpp` **only** to remove T-13; no other changes.

## Goal

Implement the SDL3 backend test file, CMake option, and build infrastructure:

1. A new test file `tests/sdl3_backend_test.cpp` with 6 runtime test cases for the SDL3 backend (`PlatformSDL3`, `WindowSDL3`, `RenderDeviceOpenGL`).
2. A `BUDDD_HAS_DISPLAY` CMake option (default `ON`) in the root `CMakeLists.txt` that controls whether SDL3 tests are compiled.
3. Conditional compilation logic in `tests/CMakeLists.txt` to include/exclude `sdl3_backend_test.cpp` and set `target_compile_definitions(buddd_tests PRIVATE BUDDD_HAS_DISPLAY)` when the option is `ON`.
4. All new tests pass using SDL3's offscreen video driver without requiring a physical display or GPU.

## Non-goals

- No changes to any file in `src/engine/` (headers, implementation, or `src/engine/CMakeLists.txt`).
- No changes to `tests/version_test.cpp`.
- No changes to `CMakePresets.json`, `.clang-format`, `.vscode/`, `opencode.json`, `AGENTS.md`, `SpecKit.md`.
- T-13 is removed from `tests/platform_abstraction_test.cpp` (the `[!mayfail]` test case is deleted). Headless tests T-01 through T-11 and T-12 remain unchanged.
- No addition of new engine APIs, factory overloads, or configuration structs to support the offscreen driver.
- No removal or modification of the existing `[!mayfail]` tag on T-13.
- No multiple-window testing, stress tests, leak tests, thread-safety tests, or performance benchmarks.
- No OpenGL version string testing or debug context verification.
- No modification of wiki pages, README, or other documentation files.

## Relevant constitution rules

- **CONST-001 (architecture-boundaries.md)**: No code outside `src/engine/` may include platform/graphics/windowing library headers. The amendment **AMEND-2026-001** (ratified 2026-05-29) adds a narrow exception for SDL3 test files conditionally compiled with `BUDDD_HAS_DISPLAY=ON`. These files may include `<SDL3/SDL.h>` only for setting video driver hints before `Platform::create()`. The file `tests/sdl3_backend_test.cpp` is covered by this exception: the name `sdl3_backend_test.cpp` is deemed "similar" to the `*_sdl3*.cpp` glob pattern per A-11 and the amendment's "or similar" clause.
- **CONST-002 (testing-policy.md)**: Requires unit tests for all testable code.

## Relevant ADRs

- **ADR-001** (`docs/adr/001-result-error-pattern.md`): Establishes `Result<T>` / `Error` as the project-wide error handling pattern. All new tests use this pattern when calling `Platform::create()`, `create_window()`, and `RenderDevice::create()`.

## Files to inspect

| File | Purpose |
|---|---|
| `tests/platform_abstraction_test.cpp` | Style reference for test code: `REQUIRE`/`REQUIRE_FALSE` usage, `using namespace buddd::engine;`, tag conventions (`[headless][platform]` etc.), include order, declaration-ordering of smart pointers. Also the file that will be modified to remove T-13. |
| `tests/CMakeLists.txt` | Current test target definition — must be modified to add conditional compilation for the new file. |
| `CMakeLists.txt` (root) | Current structure — `option(BUDDD_HAS_DISPLAY ...)` must be inserted before `add_subdirectory(tests)`. |
| `.specs/sprint-2026-05/sdl3-backend-tests/spec.md` | Authoritative spec for behavior, acceptance criteria, and edge cases. |
| `.specs/sprint-2026-05/sdl3-backend-tests/spec-critic.md` | Spec review — confirms all blocking issues resolved, verdict `Accepted with warnings`. |
| `docs/constitution/rules/CONST-001-architecture-boundaries.md` | Constitution rule with AMEND-2026-001 — confirms the SDL3 include exception. |

## Files allowed to change

### New files to create (2 files)

1. `tests/sdl3_backend_test.cpp` — The SDL3 backend test file.
2. `.github/workflows/ci.yml` — GitHub Actions CI workflow.

### Files to modify (3 files)

3. `tests/CMakeLists.txt` — Add conditional compilation logic for `sdl3_backend_test.cpp`. No other changes.
4. `CMakeLists.txt` (root) — Add `option(BUDDD_HAS_DISPLAY ...)` before `add_subdirectory(tests)`. No other changes.
5. `tests/platform_abstraction_test.cpp` — Remove the T-13 test case (`"Platform::create(SDL3) success"` with `[!mayfail]` tag). No other changes.

## Files forbidden to change

- Any file under `src/engine/` (including `CMakeLists.txt`, all headers, all `.cpp` files).
- `tests/version_test.cpp`.
- `CMakePresets.json`.
- `.clang-format`, `.vscode/`, `opencode.json`, `AGENTS.md`, `SpecKit.md`.
- `tests/platform_abstraction_test.cpp` — except for the removal of T-13 (lines 146–150). No other changes.
- Any documentation files under `docs/` not listed in "Files allowed to change".
- Any file in `src/cmd/` or `src/editor/`.

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Test assertions | Use `REQUIRE` / `REQUIRE_FALSE` (not `CHECK`). |
| Test tags | Each `TEST_CASE` must carry at least the `[sdl3]` tag. Subsystem tags (`[platform]`, `[window]`, `[render]`) are used alongside `[sdl3]`. Tags use `[tag1][tag2]` format (no spaces between brackets). |
| `using namespace` | Tests use `using namespace buddd::engine;` at file scope (as in `platform_abstraction_test.cpp`). |
| `#pragma once` | Not used in `.cpp` files. |
| Trailing return type | Not required in `.cpp` test code (unlike engine headers). |
| Include order | Standard library headers last; engine headers first; Catch2 header between them. (Pattern from `platform_abstraction_test.cpp`: engine headers, blank line, Catch2, blank line, std libs.) |
| SDL3 includes | Use `#include <SDL3/SDL.h>` (single header for all SDL3 API). Only permitted per AMEND-2026-001. |
| Object lifecycle | `Platform` must outlive `Window` must outlive `RenderDevice`. Tests follow declaration-order destruction (variables declared first are destroyed last) or explicit scoping to ensure correct order. |
| Declaration style | Use `auto` consistently for variable declarations (e.g., `auto platform = Platform::create(...)`). |
| Error handling | Check `result.has_value()` for success; access error via `result.error()`. Do NOT use `result.value()` without checking. |
| File name | `snake_case` for test file name. |

## Required implementation behavior

### 1. Root `CMakeLists.txt` modification

Insert the following `option()` call **immediately before** `enable_testing()` (currently line 20):

```cmake
option(BUDDD_HAS_DISPLAY "Enable SDL3 backend tests (requires display or offscreen driver)" ON)
```

The option must be placed before `enable_testing()` and before `add_subdirectory(tests)` so it is visible when processing `tests/CMakeLists.txt`. The `option()` call is the **only** modification to the root `CMakeLists.txt`. No other lines, comments, or whitespace changes are permitted.

### 2. `tests/CMakeLists.txt` modification

Replace the entire file content with the following:

```cmake
if(BUDDD_HAS_DISPLAY)
    message(STATUS "BUDDD_HAS_DISPLAY=ON: SDL3 backend tests enabled")

    add_executable(buddd_tests
        version_test.cpp
        platform_abstraction_test.cpp
        sdl3_backend_test.cpp
    )

    target_compile_definitions(buddd_tests PRIVATE BUDDD_HAS_DISPLAY)
else()
    message(STATUS "BUDDD_HAS_DISPLAY=OFF: SDL3 backend tests excluded")

    add_executable(buddd_tests
        version_test.cpp
        platform_abstraction_test.cpp
    )
endif()

target_link_libraries(buddd_tests PRIVATE
    buddd_engine
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(buddd_tests)
```

Requirements:
- `sdl3_backend_test.cpp` appears in the source list **only** inside the `if(BUDDD_HAS_DISPLAY)` block.
- `target_compile_definitions(buddd_tests PRIVATE BUDDD_HAS_DISPLAY)` is called **only** inside the `if(BUDDD_HAS_DISPLAY)` block.
- The `target_link_libraries`, `include(Catch)`, and `catch_discover_tests` lines are **outside** the conditional — they apply to both configurations.
- The status messages must use **exactly** the strings `"BUDDD_HAS_DISPLAY=ON: SDL3 backend tests enabled"` and `"BUDDD_HAS_DISPLAY=OFF: SDL3 backend tests excluded"`.

### 3. `.github/workflows/ci.yml` — GitHub Actions CI workflow

Create `.github/workflows/ci.yml` with the following content:

```yaml
name: CI

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build-and-test:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y --no-install-recommends \
            cmake \
            ninja-build \
            g++-14 \
            libgl1-mesa-dev

      - name: Configure with BUDDD_HAS_DISPLAY=OFF
        run: cmake -DBUDDD_HAS_DISPLAY=OFF -DSDL_UNIX_CONSOLE_BUILD=ON -DCMAKE_CXX_COMPILER=g++-14 --preset debug

      - name: Build
        run: cmake --build --preset debug

      - name: Test
        run: ctest --preset debug --output-on-failure
```

Requirements:
- Runs on push/PR to `main`.
- Installs C++ compiler (g++-14), CMake, Ninja, and Mesa OpenGL headers.
- Configures with `BUDDD_HAS_DISPLAY=OFF`, `SDL_UNIX_CONSOLE_BUILD=ON` (to skip SDL3's X11/Wayland check), and `CMAKE_CXX_COMPILER=g++-14` to select the C++26-capable compiler.
- Uses `--output-on-failure` for detailed test results.

### 4. `tests/platform_abstraction_test.cpp` — remove T-13

Delete the following block from `tests/platform_abstraction_test.cpp`:

```cpp
// T-13 — Requires a display; marked mayfail so CI does not fail
TEST_CASE("Platform::create(SDL3) success", "[sdl3][platform][!mayfail]") {
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());
}
```

No other changes to this file. The remaining tests (T-01 through T-12) are untouched.

### 5. `tests/sdl3_backend_test.cpp` — exact content

The file must contain the following content, with all test cases exactly as specified below.

#### 3a. Include guard and includes

```cpp
#ifdef BUDDD_HAS_DISPLAY

#include <SDL3/SDL.h>     // For SDL_SetHint (CONST-001 exception, per AMEND-2026-001)
#include "error.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>

using namespace buddd::engine;
```

Requirements:
- The `#ifdef BUDDD_HAS_DISPLAY` guard wraps **all** content after it, including includes. The file ends with `#endif` (see 3h).
- The SDL3 include is present only for `SDL_SetHint` calls, per AMEND-2026-001. No other SDL3 APIs are called directly in test code.
- Include order: engine headers (`error.h`, `platform/platform.h`, etc.), blank line, Catch2, blank line, standard library headers.

#### 3b. Platform::create(SDL3) succeeds with offscreen driver

```cpp
// Corresponds to AC-003
TEST_CASE("Platform::create(SDL3) succeeds with offscreen driver", "[sdl3][platform]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());
}
```

Requirements:
- `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")` is called **before** `Platform::create(Backend::SDL3)`.
- Uses `REQUIRE(platform.has_value())`.

#### 3c. SDL3 Platform creates Window with valid config

```cpp
// Corresponds to AC-004
TEST_CASE("SDL3 Platform creates Window with valid config", "[sdl3][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());
}
```

Requirements:
- Window config uses designated initializers: `{.title = "SDL3 Test", .width = 800, .height = 600}`.
- Verifies `create_window()` succeeds.

#### 3d. SDL3 Window::native_handle() returns non-null

```cpp
// Corresponds to AC-007
TEST_CASE("SDL3 Window::native_handle() returns non-null", "[sdl3][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    REQUIRE(window.value()->native_handle() != nullptr);
}
```

Requirements:
- `native_handle()` must return a non-null pointer (unlike the headless backend which returns `nullptr`).
- Assertion uses `REQUIRE(window.value()->native_handle() != nullptr)`.

#### 3e. SDL3 Window dimensions match config

```cpp
// Corresponds to AC-008
TEST_CASE("SDL3 Window dimensions match config", "[sdl3][window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    REQUIRE(window.value()->width() == 800);
    REQUIRE(window.value()->height() == 600);
}
```

Requirements:
- Verifies both `width()` and `height()` match the config values using two separate `REQUIRE` calls.

#### 3f. SDL3 RenderDevice creation

```cpp
// Corresponds to AC-005, AC-009
TEST_CASE("SDL3 RenderDevice creation", "[sdl3][render]") {
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    auto [w, h] = device.value()->size();
    REQUIRE(w == 800);
    REQUIRE(h == 600);
}
```

Requirements:
- Uses `"offscreen"` driver directly. If the offscreen driver is not available, `Platform::create()` fails and the test reports failure.
- Verifies `RenderDevice::size()` matches window dimensions.

#### 3g. SDL3 frame cycle completes

```cpp
// Corresponds to AC-006
TEST_CASE("SDL3 frame cycle completes", "[sdl3][render]") {
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
    auto platform = Platform::create(Backend::SDL3);
    REQUIRE(platform.has_value());

    auto window = platform.value()->create_window(
        {.title = "SDL3 Test", .width = 800, .height = 600});
    REQUIRE(window.has_value());

    auto device = RenderDevice::create(*window.value());
    REQUIRE(device.has_value());

    device.value()->begin_frame();
    device.value()->end_frame();
    REQUIRE(true);
}
```

Requirements:
- Uses `"offscreen"` driver directly.
- If the offscreen driver is not available or does not support OpenGL, `Platform::create()` or `RenderDevice::create()` will fail and the test reports failure.
- After a successful `begin_frame()` + `end_frame()` cycle, `REQUIRE(true)` confirms no crash occurred.

#### 3h. File closing

```cpp
#endif // BUDDD_HAS_DISPLAY
```

The file must end with a newline after `#endif`.

## Required tests

The test file `tests/sdl3_backend_test.cpp` **is** the implementation of the required tests. No separate test file is needed. The following table maps the test cases to the spec's acceptance criteria:

| Section | Test case | Tags | AC mapping |
|---|---|---|---|
| 3b | `Platform::create(SDL3) succeeds with offscreen driver` | `[sdl3][platform]` | AC-003 |
| 3c | `SDL3 Platform creates Window with valid config` | `[sdl3][window]` | AC-004 |
| 3d | `SDL3 Window::native_handle() returns non-null` | `[sdl3][window]` | AC-007 |
| 3e | `SDL3 Window dimensions match config` | `[sdl3][window]` | AC-008 |
| 3f | `SDL3 RenderDevice creation` | `[sdl3][render]` | AC-005, AC-009 |
| 3g | `SDL3 frame cycle completes` | `[sdl3][render]` | AC-006 |

## Edge cases

| Case | Expected behavior |
|---|---|
| Offscreen video driver not available (SDL3 compiled without offscreen driver support) | All tests use `"offscreen"`. `SDL_Init(SDL_INIT_VIDEO)` will fail, `Platform::create(SDL3)` returns `InitFailed`. Tests fail at `REQUIRE(platform.has_value())`. |
| Offscreen driver not available for render device tests | Tests 3f and 3g use `"offscreen"`. `Platform::create(SDL3)` fails, `REQUIRE(platform.has_value())` fails. These tests require an OpenGL-capable driver. |
| Offscreen driver available but OpenGL 4.5 Core is not | `SDL_GL_CreateContext` fails inside `RenderDevice::create()`. Tests 3f and 3g fail at `REQUIRE(device.has_value())`. This documents actual capabilities. |
| `Platform::create(SDL3)` succeeds but `create_window()` fails under offscreen driver | Tests 3c–3g fail at `REQUIRE(window.has_value())`. Indicates a problem with the selected driver. |
| Window created but `RenderDevice::create()` fails (offscreen has no GL support) | Tests 3f and 3g fail at `REQUIRE(device.has_value())`. This documents that the offscreen driver does not support OpenGL on this platform. |
| A previous test left SDL3 in unclean state | Each test starts in its own scope; `unique_ptr` destruction ensures `SDL_Quit` is called. If a prior test crashes, subsequent tests may fail. Catch2 test isolation is relied upon. |
| `SDL_SetHint` is called multiple times (once per test) | `SDL_SetHint` is idempotent for the same hint key and value. Calling it multiple times is safe. |
| `BUDDD_HAS_DISPLAY=OFF` on a machine that does have a display | SDL3 tests are excluded from compilation. Must reconfigure with `-DBUDDD_HAS_DISPLAY=ON` to run them. |
| `BUDDD_HAS_DISPLAY=ON` on a machine with no display and no offscreen driver support | SDL3 tests are compiled. At runtime, `Platform::create(SDL3)` fails. Test failures are reported. User should set `-DBUDDD_HAS_DISPLAY=OFF`. |
| Object destruction order violation | Not possible given declaration-order destruction. `platform` (first declared) is destroyed last; `window` (second) is destroyed after `device` (last declared). This is guaranteed by C++ stack unwinding. |
| `SDL_SetHint` returns `false` | The hint key is unknown or value rejected. The tests do not check the return value of `SDL_SetHint`. If it fails, `Platform::create(SDL3)` will likely fail afterward. |

## Security impact

None. The SDL3 offscreen video driver is software-only and does not access the display server, GPU, or hardware. No elevated privileges are required. No secrets, credentials, or environment variables are consumed. The `BUDDD_HAS_DISPLAY` option is a build-time switch with no runtime security implications.

The `#include <SDL3/SDL.h>` in the test file is the only SDL3 include outside `src/engine/`. It is tightly scoped per AMEND-2026-001: used only for `SDL_SetHint()`, guarded by `#ifdef BUDDD_HAS_DISPLAY`, and present only in a test file that tests the SDL3 backend.

## Data and migration impact

None. No persistent state, database, or file format is introduced. The test file exists only at build time and is not shipped.

## API compatibility impact

No new public API is introduced. No existing API is modified. The test file exercises the existing `Platform`, `Window`, and `RenderDevice` API surface as defined in IMPL-002.

## Documentation impact

None. No README, wiki page, or other documentation files are created or modified. The wiki (`docs/wiki/engineering/testing.md`) may be updated separately to document the new tests, but that is out of scope for this contract.

## ADR impact

None. No architectural decision requires an ADR. The patterns (conditional compilation, offscreen driver) are design decisions documented in the spec.

## Constitution impact

None. The constitution amendment AMEND-2026-001 has already been ratified. No further constitutional changes are required.

## Done criteria

The implementation is complete when all of the following are true:

1. **File `tests/sdl3_backend_test.cpp` exists** with content matching Required implementation behavior section 5a–5h.
2. **File `.github/workflows/ci.yml` exists** with content matching Required implementation behavior section 3.
3. **Root `CMakeLists.txt` modified** — Contains `option(BUDDD_HAS_DISPLAY "Enable SDL3 backend tests (requires display or offscreen driver)" ON)` before `enable_testing()`.
4. **`tests/CMakeLists.txt` modified** — Contains the conditional compilation logic matching Required implementation behavior section 2, including status messages.
5. **`tests/platform_abstraction_test.cpp` modified** — T-13 test case removed. T-01 through T-12 unchanged.
6. **Build succeeds with `BUDDD_HAS_DISPLAY=ON`** — `cmake --preset debug && cmake --build --preset debug` exits 0 with zero warnings from `tests/sdl3_backend_test.cpp`.
7. **Build succeeds with `BUDDD_HAS_DISPLAY=OFF`** — `cmake -DBUDDD_HAS_DISPLAY=OFF --preset debug && cmake --build --preset debug` exits 0; test binary does not contain SDL3 test symbols.
8. **Existing tests still pass** — `ctest --preset debug` passes all headless tests (T-01 through T-11) and T-12.
9. **CMake option visible** — `cmake -LA | grep BUDDD_HAS_DISPLAY` shows `BUDDD_HAS_DISPLAY:BOOL=ON`.
10. **Tests are discoverable when ON** — `cmake --build --preset debug && ./build/debug/tests/buddd_tests --list-tests` shows the 6 new test cases with `[sdl3]` tag.
11. **Forbidden files unchanged** — `git diff --name-only` does not include any file under `src/engine/`, `tests/version_test.cpp`, or `CMakePresets.json`.
12. **All assertions use `REQUIRE` / `REQUIRE_FALSE`** — Grep for `CHECK(` in `tests/sdl3_backend_test.cpp` returns zero matches.
13. **Include guard present** — `tests/sdl3_backend_test.cpp` has `#ifdef BUDDD_HAS_DISPLAY` around all content.
14. **SDL3 include is narrow** — `tests/sdl3_backend_test.cpp` includes `<SDL3/SDL.h>` only for `SDL_SetHint()`, verified by code review.

## Verification commands (copy-paste ready)

```bash
# 1. Verify the CMake option is defined
cmake -LA 2>/dev/null | grep BUDDD_HAS_DISPLAY

# 2. Build and test with BUDDD_HAS_DISPLAY=ON (default)
cmake --preset debug && cmake --build --preset debug && ctest --preset debug

# 3. Build and test with BUDDD_HAS_DISPLAY=OFF
cmake -DBUDDD_HAS_DISPLAY=OFF --preset debug && cmake --build --preset debug && ctest --preset debug

# 4. Re-enable ON for normal development
cmake --preset debug

# 5. Verify changed files are correct
git diff --name-only
# Expected: tests/sdl3_backend_test.cpp, .github/workflows/ci.yml,
#           tests/CMakeLists.txt, CMakeLists.txt, tests/platform_abstraction_test.cpp

# 6. List SDL3 tests (discoverable when ON)
./build/debug/tests/buddd_tests --list-tests 2>/dev/null | grep sdl3

# 7. Verify no CHECK macros used
grep -n 'CHECK(' tests/sdl3_backend_test.cpp || echo "No CHECK macros found (good)"

# 8. Verify include guard present
grep -n 'BUDDD_HAS_DISPLAY' tests/sdl3_backend_test.cpp

# 9. Verify T-13 is removed from platform_abstraction_test.cpp
grep -n 'T-13\|mayfail\|Platform::create(SDL3) success' tests/platform_abstraction_test.cpp || echo "T-13 removed successfully"

# 10. Check CI workflow file exists
test -f .github/workflows/ci.yml && echo "CI workflow exists"
```
