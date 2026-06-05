# SPEC-001 - Project Setup: Buddd Engine Bootstrap

## Status

`Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume (user) |
| Date | 2026-05-29 |
| Time | ~10:30 UTC |

## Problem

No project skeleton exists. A new C++26 game engine project ("Buddd Engine") requires a reproducible, modern build system and a minimal directory structure that separates concerns (engine library, CLI binary, editor placeholder, tests). Without this bootstrap, no engine features can be developed, tested, or verified.

## Goals

- Establish a reproducible CMake + Ninja build system with Debug and Release presets.
- Define a project directory structure that separates engine library (`src/engine/`), CLI binary (`src/cmd/`), editor placeholder (`src/editor/`), and unit tests (`tests/`).
- Provide a minimal engine library with a single queryable version function (e.g. `buddd::engine::version()`).
- Provide a CLI binary named `buddd` that links the engine library and prints the version when invoked with `--version`.
- Integrate Catch2 v3 via CMake FetchContent for unit testing.
- Provide a single sanity test that verifies the engine library version function returns a non-empty string.
- Provide a `.clang-format` configuration file at the repository root (based on LLVM style) and a CMake custom target `format` to enforce consistent C++ code formatting across all source files.
- Provide Visual Studio Code workspace configuration files (`.vscode/settings.json`, `.vscode/tasks.json`, `.vscode/launch.json`) for a reproducible IDE setup with IntelliSense, build tasks, and debugging configurations.
- Ensure all build configurations pass on a developer workstation (Linux assumed, but toolchain-agnostic where possible).

## Non-goals

- No game engine features (rendering, physics, audio, input, ECS, asset pipeline, etc.).
- No editor code beyond a minimal placeholder directory/file so the structure is visible.
- No packaging, installation, distribution, or deployment.
- No CI/CD pipeline configuration.
- No cross-platform testing (single platform at bootstrap stage).

## Conventions

### File and directory naming

| Convention | Rule | Examples |
|---|---|---|
| Source file names | `snake_case` (lowercase ASCII letters, digits, underscores) | `version.h`, `main.cpp`, `engine_version.h` |
| Directory names | `snake_case` (lowercase ASCII letters, digits, underscores) | `src/engine/core/`, `tests/unit/` |

### Code formatting

The project uses a `.clang-format` configuration file at the repository root based on the **LLVM** style. A CMake custom target `format` is available to apply formatting consistently across all C++ source files under `src/` and `tests/`. Developers are expected to run `cmake --build --preset debug --target format` before committing.

### IDE configuration

Editor-agnostic IDE configuration files are provided under `.vscode/` for developers using Visual Studio Code with the C/C++ extension (ms-vscode.cpptools). These files are version-controlled to ensure a consistent development experience and are described in detail below (see User-visible behavior and Acceptance criteria).

## Actors

| Actor | Description |
|---|---|
| Developer | A human who clones the repository and builds the project locally. |
| Build toolchain | CMake, Ninja, and a C++26-capable compiler invoked during configure/build/test. |
| Test runner | `ctest` (CMake's test driver) or direct invocation of the compiled test binary. |

## User-visible behavior

- Running `cmake --preset debug` configures the project in Debug mode using Ninja.
- Running `cmake --preset release` configures the project in Release mode using Ninja.
- Running `cmake --build --preset debug` (or `release`) compiles all targets.
- Running `ctest --preset debug` (or `release`) runs all unit tests and reports pass/fail.
- Invoking the compiled `buddd` binary with no arguments prints a greeting message: `"Buddd Engine v0.1.0"`.
- Invoking `buddd --version` prints a version string (e.g. `buddd 0.1.0`).
- Running `cmake --build --preset debug --target format` applies `clang-format` to all C++ source files under `src/` and `tests/` according to the project's `.clang-format` style.
- Opening the repository in Visual Studio Code automatically applies IntelliSense settings (C++26, include paths), displays available build/test tasks, and provides launch configurations for debugging the `buddd` binary and the test executable.

## User stories

### Story 1 — Clone and build (Priority: P1)

As a developer new to the project, I want to clone the repository and run a minimal sequence of CMake commands to configure, build, and run the binary, so that I can verify the toolchain works before contributing.

**Given** a fresh clone of the repository with CMake and Ninja installed
**When** I run:
```
cmake --preset debug
cmake --build --preset debug
```
**Then** the build succeeds and a binary named `buddd` is produced under the build directory.

### Story 2 — Version reporting (Priority: P1)

As a developer, I want to run the compiled binary with `--version` and see a meaningful version string, so that I can confirm the engine library is linked correctly.

**Given** the project has been built successfully in Debug mode
**When** I run `./build/debug/src/cmd/buddd --version`
**Then** the output matches the pattern `buddd <major>.<minor>.<patch>` (e.g. `buddd 0.1.0`).

### Story 3 — Tests pass (Priority: P1)

As a developer, I want to run the test suite and see all tests pass, so that I know the project is correctly set up and the toolchain works end-to-end.

**Given** the project has been configured and built in Debug mode
**When** I run `ctest --preset debug`
**Then** all tests pass and the output indicates 100% tests passed.

### Story 4 — Switch between Debug and Release (Priority: P2)

As a developer, I want to build the project in Release mode and confirm the binary is optimized (smaller/faster), so that I can prepare builds for performance evaluation.

**Given** the project has been configured with `--preset release`
**When** I run `cmake --build --preset release`
**Then** the build succeeds and the resulting binary is located under `build/release/`.

### Story 5 — Format code with a single command (Priority: P2)

As a developer, I want to run a single CMake command to format all C++ source files according to the project style, so that I do not have to worry about formatting inconsistencies before committing.

**Given** the project has been configured with `--preset debug`
**When** I run `cmake --build --preset debug --target format`
**Then** all `.cpp`, `.hpp`, and `.h` files under `src/` and `tests/` are formatted according to the rules defined in `.clang-format` at the repository root.

### Story 6 — Debug the project in VS Code (Priority: P3)

As a developer using Visual Studio Code, I want to use provided launch configurations to debug the `buddd` binary and the `buddd_tests` executable, so that I can set breakpoints and inspect program state without manual gdb invocation.

**Given** the repository is opened in VS Code with the C/C++ extension installed and the project has been built in Debug mode
**When** I select the "Debug buddd" launch configuration and press F5
**Then** the `buddd` binary starts under the debugger with breakpoints enabled.

**Given** the repository is opened in VS Code with the C/C++ extension installed and the project has been built in Debug mode
**When** I select the "Debug buddd_tests" launch configuration and press F5
**Then** the `buddd_tests` test executable starts under the debugger.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | A CMake preset `debug` exists and configures the project with Ninja in Debug mode. | Run `cmake --preset debug` (exits 0), then `cmake -L -N build/debug` — output contains `CMAKE_BUILD_TYPE:STRING=Debug`. |
| AC-002 | A CMake preset `release` exists and configures the project with Ninja in Release mode. | Run `cmake --preset release` (exits 0), then `cmake -L -N build/release` — output contains `CMAKE_BUILD_TYPE:STRING=Release`. |
| AC-003 | The `src/engine/` directory contains a CMake target (static library) named `buddd_engine`. | After configure, `cmake --build --preset debug --target buddd_engine` succeeds and produces a library file. |
| AC-004 | The `src/cmd/` directory contains a CMake executable target named `buddd` that links `buddd_engine`. | After build, `build/debug/src/cmd/buddd` exists and is an executable. |
| AC-005 | Running `buddd --version` prints a string matching `buddd <major>.<minor>.<patch>` (e.g. `buddd 0.1.0`). | Execute `./build/debug/src/cmd/buddd --version`; stdout matches the pattern `buddd <major>.<minor>.<patch>`. |
| AC-006 | Running `buddd` with no arguments prints the greeting message `"Buddd Engine v0.1.0"`. | Execute `./build/debug/src/cmd/buddd`; process exits with code 0, stdout contains `Buddd Engine v0.1.0`, and stderr is empty. |
| AC-007 | The `tests/` directory contains a Catch2 v3 test that verifies the engine library's version function returns a non-empty string. | Run the test binary directly or via `ctest --preset debug`; the specific test case passes. |
| AC-008 | Catch2 v3 is fetched automatically via FetchContent (no manual download). | A clean build on a machine without Catch2 pre-installed succeeds — FetchContent downloads it. |
| AC-009 | The `src/editor/` directory exists with a placeholder `CMakeLists.txt` that defines an INTERFACE library target named `buddd_editor` with no sources (no binary produced). | `cmake --build --preset debug` succeeds; no editor binary is produced (the `buddd_editor` target is an INTERFACE library with no compiled sources). |
| AC-010 | `ctest --preset debug` reports all tests passing with 0 failures. | Run `ctest --preset debug`; exit code 0 and output contains "100% tests passed". |
| AC-011 | A `.clang-format` file exists at the repository root, based on LLVM style. | `ls .clang-format` succeeds; `head -1 .clang-format` contains `BasedOnStyle: LLVM` (or the file is parseable by `clang-format -style=file -dump-config` without error). |
| AC-012 | A CMake custom target `format` exists and runs `clang-format` on all C++ source files under `src/` and `tests/`. | Run `cmake --build --preset debug --target format`; exits 0 and `git diff --stat` after running on unformatted files shows only formatting changes. |
| AC-013 | A `.vscode/settings.json` file exists with C/C++ IntelliSense configured: `intelliSenseEngine` set to `"default"`, `cStandard` set to `"c23"`, `cppStandard` set to `"c++26"`, `includePath` covering `${workspaceFolder}/src/engine`, `clang-format` as the default C++ formatter, and format on save enabled. | `ls .vscode/settings.json` succeeds; file contains `"C_Cpp.intelliSenseEngine": "default"`, `"C_Cpp.default.cStandard": "c23"`, `"C_Cpp.default.cppStandard": "c++26"`, `"C_Cpp.default.includePath"` with an entry matching `**/src/engine`, `"[cpp]"` formatter set to `"ms-vscode.cpptools"` or `"xaver.clang-format"`, and `"editor.formatOnSave": true`. |
| AC-014 | A `.vscode/tasks.json` file exists with tasks for CMake configure (debug preset), CMake build (debug preset), and running tests (ctest). | `ls .vscode/tasks.json` succeeds; file contains task definitions with `"type": "cmake"` or `"type": "shell"` covering configure, build, and test operations for the debug preset. |
| AC-015 | A `.vscode/launch.json` file exists with configurations to debug the `buddd` executable and the `buddd_tests` test executable. | `ls .vscode/launch.json` succeeds; file contains a configuration named `"Debug buddd"` with `"program"` pointing to `${workspaceFolder}/build/debug/src/cmd/buddd` and a configuration named `"Debug buddd_tests"` with `"program"` pointing to `${workspaceFolder}/build/debug/tests/buddd_tests`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A new developer can go from `git clone` to `ctest` in under 5 minutes (excluding download times for dependencies). | Measure from clean clone to test pass on a typical Linux workstation with cached FetchContent. |
| SC-002 | The build is reproducible: two successive `cmake --build` invocations without source changes produce identical binary hashes. | Run `sha256sum` on the `buddd` binary after two clean builds with the same preset. |
| SC-003 | Build configuration (Debug / Release) completes without manual intervention other than the preset name. | Execute both presets in sequence on a clean checkout; both succeed. |

## Edge cases

| Case | Expected behavior |
|---|---|
| CMake not installed | CMake itself produces a clear OS-level error before any build command runs. |
| Ninja not installed | CMake configuration fails with a message indicating the Ninja generator was not found. |
| Compiler does not support C++26 | CMake configuration fails with a fatal error about unsupported C++ standard. |
| Network unavailable (first build) | FetchContent fails with a network error; CMake configure fails with a clear message about failed download. |
| FetchContent cached from prior build | No network request is made; build uses cached Catch2. |
| Incremental build (no changes) | `cmake --build` reports "no work to do" and exits 0. |
| `buddd` binary deleted | Re-running `cmake --build` rebuilds only the missing target. |

## Error cases

| Case | Expected behavior |
|---|---|
| Invalid CMake preset name | `cmake --preset invalid` exits with non-zero and lists available presets. |
| Build failure due to compiler error | `cmake --build` exits non-zero and prints compiler diagnostics. |
| Test failure | `ctest` exits non-zero and prints the failing test name and its output. |
| FetchContent download failure (e.g. timeout) | `cmake` exits non-zero with a message like "Failed to download Catch2". |
| Missing `Catch2` tag in FetchContent declaration | `cmake` configure fails with an error indicating the repository or tag was not found. |
| `clang-format` not installed | `cmake --build --preset debug --target format` exits non-zero with a clear error message indicating `clang-format` was not found. |

## Permissions and security

- No network access is required after the initial FetchContent download (cached locally).
- No elevated privileges (root) are required for any build or test operation.
- No secrets, credentials, or environment variables are needed.
- The build system does not execute arbitrary downloaded code beyond the declared FetchContent dependencies.

## Observability

| Signal | Source |
|---|---|
| CMake configure output | `cmake --preset <name>` stdout/stderr |
| Build progress and errors | `cmake --build --preset <name>` stdout/stderr |
| Test results summary | `ctest --preset <name>` stdout |
| Individual test output | `ctest --preset <name> --output-on-failure` (developer opt-in) |
| FetchContent download status | CMake configure output during first configure |

## Out of scope

- Any game engine system (rendering, physics, audio, input, ECS, asset pipeline, scripting).
- Editor application logic, UI framework, or GUI code.
- Packaging (CPack), installation rules, or system-wide deployment.
- CI/CD configuration (GitHub Actions, GitLab CI, etc.).
- Cross-compilation or multi-platform builds.
- Compiler warnings-as-errors policy.
- Precompiled headers or build optimization (Unity/Jumbo builds, ccache integration).
- Fuzzing, benchmarks, or performance profiling targets.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The host development environment is Linux (x86-64). Windows and macOS are not primary targets at bootstrap. |
| A-02 | CMake version >= 3.28 is available. Older versions may not support `CMAKE_CXX_STANDARD` 26 or the preset features used. |
| A-03 | Ninja >= 1.11 is available on `PATH`. |
| A-04 | The reference compiler is GCC 14+ (a C++26-capable compiler). Clang 19+ or equivalent C++26 compilers may be used but GCC 14+ is the primary target at bootstrap. |
| A-05 | The engine library starts as a static library (`STATIC`), not header-only. It may be converted later. |
| A-06 | The initial version string is `0.1.0` (semantic versioning). This is defined once in the engine library's version header. |
| A-07 | Catch2 v3 is available at the URL and tag specified in the FetchContent declaration. The specific tag is v3.7.0 or later. |
| A-08 | The editor placeholder is a single `CMakeLists.txt` with an empty library target (e.g. `add_library(buddd_editor INTERFACE)` or no sources). No editor code is compiled. |
| A-09 | The test binary is a single executable named `buddd_tests` linked against `buddd_engine` and Catch2. |
| A-10 | FetchContent downloads are written to the build directory and persist across rebuilds (CMake's default behavior with `FETCHCONTENT_QUIET`). |
| A-11 | VS Code with the C/C++ extension (ms-vscode.cpptools) is installed for IntelliSense and debugging functionality when using the `.vscode/` configuration files. Other editors may ignore these files. |
| A-12 | `clang-format` >= 18 is available on `PATH` when using the CMake `format` target. If `clang-format` is not installed, the `format` target will produce a clear error message during build. |

## Open questions

| ID | Question | Impact |
|---|---|---|
| Q-01 | [RESOLVED] GCC 14+ is the reference compiler at bootstrap. See Assumption A-04. | **Scope**: Toolchain requirements. |
| Q-02 | [NEEDS CLARIFICATION] Should the engine library expose its version as a `constexpr` / `consteval` function, a runtime function, or a macro? Each has different testability implications. | **Scope**: API design and test verification approach. |
| Q-03 | [RESOLVED] The greeting message is `"Buddd Engine v0.1.0"`. See AC-006 and User-visible behavior. | **Scope**: User-visible output; trivially changed but affects AC-006. |
