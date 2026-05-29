# IMPL-001 — Project Setup: Buddd Engine Bootstrap

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume (user) |
| Date | 2026-05-29 |
| Time | ~10:30 UTC |

## Source spec

`docs/specs/project-setup/spec.md` (SPEC-001), accepted (`docs/specs/project-setup/spec-critic.md` verdict: `Accepted with warnings`, all blocking issues resolved).

## Goal

Create a minimal, reproducible C++26 project skeleton for the Buddd Engine with:

- A CMake + Ninja build system with `debug` and `release` presets.
- Four CMake targets: `buddd_engine` (static library), `buddd` (CLI executable), `buddd_editor` (INTERFACE library placeholder), `buddd_tests` (test executable).
- A version API `buddd::engine::version() -> std::string_view` returning `"0.1.0"`.
- A CLI binary that prints `Buddd Engine v0.1.0` with no arguments and `buddd 0.1.0` with `--version`.
- A single Catch2 v3 sanity test verifying the version function returns a non-empty string.
- Catch2 v3 fetched automatically via `FetchContent` (no manual download).
- All build and test commands (configure, build, test) execute via CMake presets with zero manual flags.

## Non-goals

- No game engine features (rendering, physics, audio, input, ECS, asset pipeline, scripting).
- No editor code beyond the INTERFACE library placeholder — zero compiled source files in `src/editor/`.
- No packaging, installation, distribution, or deployment (no CPack, no install rules).
- No CI/CD pipeline configuration.
- No cross-platform testing.
- No linting (static analysis, clang-tidy), precompiled headers, or ccache configuration.

  > **Note**: Formatting (`.clang-format` + `format` CMake target) is now in scope — see sections 11 and Root CMakeLists.txt update.
- No compilation with `-Wall -Werror` flags or warning policies.
- No README or documentation files outside of `docs/`.
- No modification of any existing source code (there is none yet — this is bootstrap).

## Relevant constitution rules

All constitution rules (`CONST-001` through `CONST-004`) contain placeholder "TODO" text. No active rule applies to this contract.

## Relevant ADRs

No accepted ADRs exist. This contract does not require an ADR.

## Files to inspect

None. This is a fresh bootstrap — no existing source code to inspect.

## Files allowed to change

The following files **must be created** (they do not exist yet). All paths are relative to the repository root.

1. **`CMakeLists.txt`** — Root CMake configuration.
2. **`CMakePresets.json`** — CMake presets (configure, build, test) for `debug` and `release`.
3. **`src/engine/CMakeLists.txt`** — Static library target `buddd_engine`.
4. **`src/engine/version.h`** — Public header declaring `buddd::engine::version()`.
5. **`src/engine/version.cpp`** — Implementation returning `"0.1.0"`.
6. **`src/cmd/CMakeLists.txt`** — Executable target `buddd`.
7. **`src/cmd/main.cpp`** — CLI entry point.
8. **`src/editor/CMakeLists.txt`** — INTERFACE library target `buddd_editor` (placeholder, no sources).
9. **`tests/CMakeLists.txt`** — Test executable target `buddd_tests` and Catch2 discovery.
10. **`tests/version_test.cpp`** — Single Catch2 test case for version non-emptiness.
11. **`.clang-format`** — Clang-format configuration (repository root), based on LLVM style.
12. **`.vscode/settings.json`** — VS Code workspace IntelliSense and formatting settings.
13. **`.vscode/tasks.json`** — VS Code build/test tasks (shell commands via CMake presets).
14. **`.vscode/launch.json`** — VS Code launch configurations for debugging `buddd` and `buddd_tests`.

## Files forbidden to change

No files exist in the repository that are relevant to source code (only `docs/`, `.opencode/`, `.git/`, `AGENTS.md`, `opencode.json`, `SpecKit.md`). None of these shall be modified.

> **Note**: The `.vscode/` directory is version-controlled for the workspace configuration files (`settings.json`, `tasks.json`, `launch.json`). User-specific VS Code files (e.g., `.vscode/extensions.json`, `.vscode/*.code-snippets`) should NOT be added or tracked.

## Existing conventions to follow

- No existing C++ code conventions (this is the bootstrap).
- Follow the directory layout in the spec: `src/engine/`, `src/cmd/`, `src/editor/`, `tests/`.
- Source file names: `snake_case` (e.g., `version.h`, `main.cpp`, `engine_version.h`).
- Directory names: `snake_case` (e.g., `src/engine/core/`, `tests/unit/`).
- Code formatting: enforced via `.clang-format` (LLVM style, 4-space indent, 100 column limit).
- Use `PascalCase` for Catch2 test case names (matching Catch2 convention).

  > **Note**: The contract-required test case name is `"engine version is non-empty"` (sentence case), which does not follow this PascalCase convention. The explicit code block takes precedence.
- Use `UPPER_SNAKE_CASE` for CMake cache variables (`CMAKE_BUILD_TYPE`, etc.).

## Required implementation behavior

### 1. Root `CMakeLists.txt`

Must contain, in this order:

1. `cmake_minimum_required(VERSION 3.28)`
2. `project(buddd VERSION 0.1.0 LANGUAGES CXX)`
3. `set(CMAKE_CXX_STANDARD 26)` — must be before any target is defined.
4. `set(CMAKE_CXX_STANDARD_REQUIRED ON)`
5. `set(CMAKE_CXX_EXTENSIONS OFF)` — ensures strict C++26, no compiler extensions.
6. `add_subdirectory(src/engine)` — before other targets so the library exists first.
7. `add_subdirectory(src/cmd)`
8. `add_subdirectory(src/editor)`
9. `include(FetchContent)` then `FetchContent_Declare(Catch2 ...)` and `FetchContent_MakeAvailable(Catch2)` — must be before `add_subdirectory(tests)`.
10. `add_subdirectory(tests)`

FetchContent block must be:

```cmake
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.0
)
FetchContent_MakeAvailable(Catch2)
```

The root `CMakeLists.txt` must NOT set `CMAKE_BUILD_TYPE` — it is provided by the preset. It must NOT define any compiled targets itself (all compiled targets are in subdirectories). However, it MAY define a non-compiled custom target (`format`) for clang-format.

After the `add_subdirectory(tests)` call (step 10), append the following block to define the `format` custom target:

```cmake
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        src/*.cpp src/*.h src/*.hpp
        tests/*.cpp tests/*.h tests/*.hpp
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting all source files with clang-format..."
    )
else()
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found. Install clang-format >= 18."
        COMMAND false
    )
endif()
```

- `GLOB_RECURSE` is acceptable here because it runs at CMake configure time and is not relied on for build dependencies — the `format` target is a developer convenience, not a build step.
- The fallback `else()` block prints a clear message **and exits non-zero** (via `COMMAND false`), satisfying the error-case requirement.

### 2. `CMakePresets.json`

Must be at the repository root and conform to the CMakePresets.json schema (version 6 or later). Must contain three preset arrays:

**configurePresets:**

| Field | `debug` | `release` |
|---|---|---|
| `name` | `"debug"` | `"release"` |
| `displayName` | `"Debug"` | `"Release"` |
| `generator` | `"Ninja"` | `"Ninja"` |
| `binaryDir` | `"${sourceDir}/build/debug"` | `"${sourceDir}/build/release"` |
| `cacheVariables.CMAKE_BUILD_TYPE` | `"Debug"` | `"Release"` |
| `cacheVariables.CMAKE_CXX_STANDARD` | `"26"` | `"26"` |
| `cacheVariables.CMAKE_CXX_STANDARD_REQUIRED` | `"ON"` | `"ON"` |

**buildPresets:**

| Field | `debug` | `release` |
|---|---|---|
| `name` | `"debug"` | `"release"` |
| `configurePreset` | `"debug"` | `"release"` |

**testPresets:**

| Field | `debug` | `release` |
|---|---|---|
| `name` | `"debug"` | `"release"` |
| `configurePreset` | `"debug"` | `"release"` |

The `"version"` field at the top level must be `6` or higher (compatible with CMake >= 3.28).

### 3. `src/engine/CMakeLists.txt`

```cmake
add_library(buddd_engine STATIC
    version.h
    version.cpp
)

target_include_directories(buddd_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

- Target name MUST be `buddd_engine`.
- Type MUST be `STATIC`.
- Header `version.h` MUST be listed in the `add_library` sources so it appears in IDE project files.
- The PUBLIC include directory `${CMAKE_CURRENT_SOURCE_DIR}` allows consumers to `#include "version.h"`.

### 4. `src/engine/version.h`

```cpp
#pragma once

#include <string_view>

namespace buddd::engine {

auto version() -> std::string_view;

} // namespace buddd::engine
```

- MUST use `#pragma once` (no include guards).
- MUST declare `version()` with return type `auto version() -> std::string_view` (trailing return type syntax).
- MUST be in namespace `buddd::engine`.
- MUST NOT define the function body — it goes in `version.cpp`.

### 5. `src/engine/version.cpp`

```cpp
#include "version.h"

#include <string_view>

namespace buddd::engine {

auto version() -> std::string_view {
    return "0.1.0";
}

} // namespace buddd::engine
```

- The version string literal is `"0.1.0"` — exactly this string, hard-coded, no leading `v`.
- No other headers besides `"version.h"` and `<string_view>`.

### 6. `src/cmd/CMakeLists.txt`

```cmake
add_executable(buddd main.cpp)

target_link_libraries(buddd PRIVATE buddd_engine)
```

- Target name MUST be `buddd`.
- Must link `buddd_engine` as PRIVATE.
- No other dependencies.

### 7. `src/cmd/main.cpp`

```cpp
#include "version.h"

#include <cstdio>
#include <string_view>

auto main(int argc, char* argv[]) -> int {
    if (argc == 2 && std::string_view{argv[1]} == "--version") {
        std::printf("buddd %s\n", buddd::engine::version().data());
    } else {
        std::printf("Buddd Engine v%s\n", buddd::engine::version().data());
    }
    return 0;
}
```

- MUST NOT include `<iostream>` — only `<cstdio>` (no iostream to keep the binary minimal).
- MUST check `argc == 2` and `argv[1] == "--version"` as a single condition.
- Any other argument combination (no args, more args, `--help`, etc.) falls through to the `else` branch and prints the greeting.
- MUST print a trailing newline (`\n`) in both branches.
- `main` MUST return 0 in all cases (no error paths at this stage).

### 8. `src/editor/CMakeLists.txt`

```cmake
add_library(buddd_editor INTERFACE)
```

- Target name MUST be `buddd_editor`.
- Type MUST be `INTERFACE`.
- No sources, no dependencies, no include directories.
- No `add_executable` — the editor produces no binary.

### 9. `tests/CMakeLists.txt`

```cmake
add_executable(buddd_tests version_test.cpp)

target_link_libraries(buddd_tests PRIVATE
    buddd_engine
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(buddd_tests)
```

- Target name MUST be `buddd_tests`.
- MUST link both `buddd_engine` and `Catch2::Catch2WithMain` as PRIVATE.
- `Catch2::Catch2WithMain` provides the `main()` entry point — the test source file does NOT define one.
- `include(Catch)` and `catch_discover_tests(buddd_tests)` MUST be present for `ctest` integration.

### 10. `tests/version_test.cpp`

```cpp
#include "version.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("engine version is non-empty", "[sanity]") {
    REQUIRE_FALSE(buddd::engine::version().empty());
}
```

- Test name: `"engine version is non-empty"`.
- Tags: `[sanity]`.
- Must use `REQUIRE_FALSE` on `.empty()` — not `REQUIRE(!...)` (consistency with Catch2 style).
- Only one test case in this file.
- The header include path `"version.h"` resolves via the PUBLIC include directory added by `buddd_engine`.

### 11. `.clang-format` (repository root)

Based on LLVM style. Must contain at minimum:

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
AccessModifierOffset: -4
AlignAfterOpenBracket: Align
ColumnLimit: 100
Standard: c++26
```

- The `BasedOnStyle: LLVM` line MUST be present (satisfies AC-011).
- `ColumnLimit: 100` overrides LLVM default 80.
- `IndentWidth: 4` overrides LLVM default 2.
- `AccessModifierOffset: -4` aligns access modifiers (`public:`, `private:`, `protected:`) at column 0.
- `AlignAfterOpenBracket: Align` aligns function arguments after the opening bracket.
- `Standard: c++26` tells clang-format to use C++26 syntax features. An acceptable alternative is `Standard: Auto`.

### 12. `.vscode/settings.json`

VS Code workspace settings for C/C++ IntelliSense and formatting:

```json
{
    "C_Cpp.intelliSenseEngine": "default",
    "C_Cpp.default.cStandard": "c23",
    "C_Cpp.default.cppStandard": "c++26",
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/src/engine",
        "${workspaceFolder}/src"
    ],
    "[cpp]": {
        "editor.defaultFormatter": "ms-vscode.cpptools",
        "editor.formatOnSave": true
    },
    "files.associations": {
        "*.h": "cpp"
    }
}
```

- `intelliSenseEngine` MUST be `"default"` (not `"disabled"` or `"Tag Parser"`).
- `cStandard` MUST be `"c23"` (latest C standard for any C header dependencies).
- `cppStandard` MUST be `"c++26"`.
- `includePath` MUST include both `${workspaceFolder}/src/engine` and `${workspaceFolder}/src`.
- The `[cpp]` language-specific section MUST set `editor.defaultFormatter` to `ms-vscode.cpptools` and `editor.formatOnSave` to `true`.
- `files.associations` MUST map `*.h` to `cpp` (so that C headers are treated as C++ headers).

### 13. `.vscode/tasks.json`

VS Code tasks for CMake operations:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "type": "shell",
            "label": "CMake: configure debug",
            "command": "cmake",
            "args": ["--preset", "debug"],
            "group": "build",
            "problemMatcher": []
        },
        {
            "type": "shell",
            "label": "CMake: build debug",
            "command": "cmake",
            "args": ["--build", "--preset", "debug"],
            "group": {"kind": "build", "isDefault": true},
            "problemMatcher": ["$gcc"]
        },
        {
            "type": "shell",
            "label": "CTest: run tests debug",
            "command": "ctest",
            "args": ["--preset", "debug"],
            "group": "test",
            "problemMatcher": []
        }
    ]
}
```

- Task `"CMake: configure debug"` runs `cmake --preset debug`.
- Task `"CMake: build debug"` runs `cmake --build --preset debug`. It uses `"$gcc"` problem matcher for compiler diagnostics. This task is the default build task.
- Task `"CTest: run tests debug"` runs `ctest --preset debug`.

### 14. `.vscode/launch.json`

VS Code launch configurations for debugging:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug buddd",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/debug/src/cmd/buddd",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "CMake: build debug"
        },
        {
            "name": "Debug buddd_tests",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/debug/tests/buddd_tests",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "CMake: build debug"
        }
    ]
}
```

- Configuration `"Debug buddd"` launches `build/debug/src/cmd/buddd` with `MIMode: gdb` and pretty-printing enabled. It runs the `"CMake: build debug"` task first (`preLaunchTask`).
- Configuration `"Debug buddd_tests"` launches `build/debug/tests/buddd_tests` with the same debugger settings and pre-build task.
- Both configurations use `"type": "cppdbg"` (the C/C++ extension's debugger), not `"lldb"` or `"cppvsdbg"`.
- `stopAtEntry` is `false` (do not break at `main()` by default).

## Required tests

The following test cases are contractually required in the `buddd_tests` binary:

| Test case name | Tags | Source file | Verification |
|---|---|---|---|
| `engine version is non-empty` | `[sanity]` | `tests/version_test.cpp` | `buddd::engine::version()` returns a `std::string_view` whose `.empty()` is `false`. |

No additional tests are required for this bootstrap phase. The test-author must NOT add tests for CLI output, build system, or presets — those are verified manually via acceptance criteria.

## Edge cases

| Case | Expected behavior |
|---|---|
| CLI invoked with `--version` | Prints `buddd 0.1.0\n` and exits 0. |
| CLI invoked with no arguments | Prints `Buddd Engine v0.1.0\n` and exits 0. |
| CLI invoked with `--help` or unknown flags | Falls through to the `else` branch — prints `Buddd Engine v0.1.0\n` and exits 0. There is no `--help` handling yet. |
| CLI invoked with multiple arguments (e.g. `buddd foo bar`) | Falls through to the `else` branch — prints greeting and exits 0. |
| `FetchContent` network failure on first build | CMake configure fails with a network error message. |
| Catch2 repository tag `v3.7.0` is deleted or moved | CMake configure fails. The tag MUST be pinned to `v3.7.0`. |
| Compiler does not support `-std=c++26` | CMake configuration fails because `CMAKE_CXX_STANDARD_REQUIRED` is ON and the compiler cannot satisfy the request. |
| `clang-format` not installed | `cmake --build --preset debug --target format` exits non-zero with a clear message: "clang-format not found. Install clang-format >= 18." The `format` target fallback (`else()` block) prints the message and exits with error. |
| `build/debug/` directory does not exist | CMake creates it automatically during configure. |
| `buddd` binary deleted after build | Re-running `cmake --build --preset debug` rebuilds only the missing target. |
| No source changes between builds | `cmake --build --preset debug` reports "ninja: no work to do." |
| Test binary deleted after build | Re-running `cmake --build --preset debug` rebuilds it; `ctest --preset debug` re-discovers tests. |

## Security impact

None. The build system downloads Catch2 over HTTPS from GitHub (TLS-protected). No secret, credential, or environment variable is used. The resulting binary does not open network sockets, read files, or execute user input beyond standard CLI argument parsing.

## Data and migration impact

None. No persistent state, database, or file format is introduced.

## API compatibility impact

The public API surface is a single function:

```cpp
namespace buddd::engine {
auto version() -> std::string_view;
}
```

This is the first version of this API. Once released, changing the namespace, function name, return type, or semantic meaning of the returned string constitutes a breaking change.

The version string `"0.1.0"` is defined once in `src/engine/version.cpp`. The version in `CMakeLists.txt` (`project(buddd VERSION 0.1.0 ...)`) and in `version.cpp` MUST be kept in sync manually — no automation is introduced at this stage.

## Documentation impact

- This contract is the authoritative reference for the project skeleton.
- No README, wiki page, or other documentation is created or modified by this contract.
- The `SpecKit.md` and `AGENTS.md` remain untouched.

## ADR impact

None. No architectural decision requires an ADR — the choices made here (static library, FetchContent for Catch2, INTERFACE editor library, CLI output format) are all straightforward bootstrap decisions that do not constrain future architecture irreversibly.

## Constitution impact

None. No constitution rules need to be added or amended.

## Done criteria

The implementation is complete when all of the following are true:

1. **Files exist**: All 14 files listed in "Files allowed to change" exist with correct content.
2. **Configure succeeds**: `cmake --preset debug` exits 0 and creates `build/debug/` with `CMakeCache.txt`.
3. **Build succeeds**: `cmake --build --preset debug` exits 0 and produces `build/debug/src/cmd/buddd` and `build/debug/tests/buddd_tests`.
4. **Release preset works**: `cmake --preset release` and `cmake --build --preset release` succeed, producing `build/release/src/cmd/buddd`.
5. **CLI greeting**: `./build/debug/src/cmd/buddd` prints `Buddd Engine v0.1.0` followed by a newline, exits 0, and stderr is empty.
6. **CLI --version**: `./build/debug/src/cmd/buddd --version` prints `buddd 0.1.0` followed by a newline, exits 0, and stderr is empty.
7. **Tests pass**: `ctest --preset debug` exits 0 and reports "100% tests passed".
8. **No Catch2 manual install**: A build from a clean checkout (no system-installed Catch2) downloads and uses Catch2 via FetchContent successfully.
9. **Editor target exists**: `cmake --build --preset debug --target buddd_editor` succeeds (it is a no-op INTERFACE library; no binary produced).
10. **`build/debug/tests/buddd_tests` exists** and can be invoked directly: `./build/debug/tests/buddd_tests` runs the test suite and passes.
11. **`.clang-format` exists** at repository root with `BasedOnStyle: LLVM` and the required overrides.
12. **`.vscode/` files exist**: `settings.json`, `tasks.json`, and `launch.json` all present and contain the required configurations.
13. **`clang-format` target works**: `cmake --build --preset debug --target format` runs without error (if `clang-format` is installed) or exits non-zero with a clear error message about missing `clang-format` (if not installed).

## Verification commands (copy-paste ready)

```bash
# Configure and build in debug mode
cmake --preset debug
cmake --build --preset debug

# Verify CLI outputs
./build/debug/src/cmd/buddd                    # expected: "Buddd Engine v0.1.0"
./build/debug/src/cmd/buddd --version           # expected: "buddd 0.1.0"

# Run tests
ctest --preset debug                            # expected: 100% tests passed
./build/debug/tests/buddd_tests                 # alternative direct invocation

# Format all source files (requires clang-format >= 18)
cmake --build --preset debug --target format

# Release preset (optional, P2)
cmake --preset release
cmake --build --preset release
```
