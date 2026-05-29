# Module Map

## Overview

The project is composed of four CMake targets organized into four source directories. Each target has a specific role within the bootstrap architecture.

## `buddd_engine` — Static library (`src/engine/`)

The engine library is the core of the project. In the bootstrap stage it provides only a version API; future engine features (rendering, physics, audio, etc.) will be added as submodules within this directory.

| File | Role |
|---|---|
| `version.h` | Public header: declares `buddd::engine::version() -> std::string_view` |
| `version.cpp` | Implementation: returns `"0.1.0"` |

The library exposes a PUBLIC include directory of `${CMAKE_CURRENT_SOURCE_DIR}` (i.e., `src/engine/`), allowing consumers to `#include "version.h"`.

## `buddd` — CLI executable (`src/cmd/`)

The command-line binary. Links `buddd_engine` as PRIVATE.

| File | Role |
|---|---|
| `main.cpp` | Entry point: parses `argc`/`argv`, calls `buddd::engine::version()`, prints output |

Behavior:
- No arguments → prints `Buddd Engine v0.1.0`
- `--version` as sole argument → prints `buddd 0.1.0`
- Any other argument combination → falls through to the greeting branch

## `buddd_editor` — INTERFACE library placeholder (`src/editor/`)

A placeholder for the future editor application. Currently defines an INTERFACE library target with no sources, no dependencies, and no include directories. No binary is produced.

## `buddd_tests` — Test executable (`tests/`)

The unit test binary. Links `buddd_engine` (PRIVATE) and `Catch2::Catch2WithMain` (PRIVATE). Catch2 provides its own `main()` entry point.

| File | Role |
|---|---|
| `version_test.cpp` | Single Catch2 test: `"engine version is non-empty"` tagged `[sanity]` |

## Source naming conventions

- Source files: `snake_case` (e.g., `version.h`, `main.cpp`, `version_test.cpp`)
- Directories: `snake_case` (e.g., `src/engine/`, `src/cmd/`, `tests/`)
- CMake target names: `snake_case` (e.g., `buddd_engine`, `buddd_tests`)
- Test case names: sentence case (e.g., `"engine version is non-empty"`)

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Goals, Conventions, Directory structure
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 3-10 (individual target specifications)
