# Architecture Overview

## Project

**Buddd Engine** is a C++26 game engine. At the bootstrap stage (v0.1.0), the project provides a minimal, reproducible CMake + Ninja build system and a directory structure that separates concerns across four targets.

## Directory layout

```
buddd2/
├── CMakeLists.txt          # Root CMake configuration
├── CMakePresets.json        # Build presets (debug, release)
├── .clang-format            # Code formatting rules (LLVM-based)
├── .vscode/                 # VS Code workspace configuration
│   ├── settings.json
│   ├── tasks.json
│   └── launch.json
├── src/
│   ├── engine/              # Engine library (static lib)
│   ├── cmd/                 # CLI binary (links engine)
│   └── editor/              # Editor placeholder (INTERFACE lib)
├── tests/                   # Unit tests (Catch2 v3)
└── docs/
    ├── constitution/        # Mandatory project rules
    ├── specs/               # Product specs and implementation contracts
    ├── adr/                 # Architecture decision records
    └── wiki/                # Operational documentation (this wiki)
```

## Build system

- **Generator**: Ninja
- **Presets**: `debug` (Debug build) and `release` (Release build)
- **Standard**: C++26 (`CMAKE_CXX_STANDARD 26`, `REQUIRED ON`, `EXTENSIONS OFF`)
- **Formatting**: `clang-format` via custom `format` CMake target

## CMake targets

| Target | Type | Directory | Description |
|---|---|---|---|
| `buddd_engine` | Static library | `src/engine/` | Core engine; exposes `buddd::engine::version()` |
| `buddd` | Executable | `src/cmd/` | CLI binary; links `buddd_engine` |
| `buddd_editor` | INTERFACE library | `src/editor/` | Placeholder — no compiled sources |
| `buddd_tests` | Executable | `tests/` | Catch2 test binary; links `buddd_engine` |

## Key behaviors

- `./build/debug/src/cmd/buddd` — prints `Buddd Engine v0.1.0`
- `./build/debug/src/cmd/buddd --version` — prints `buddd 0.1.0`
- `ctest --preset debug` — runs tests, all pass
- `cmake --build --preset debug --target format` — formats all C++ sources

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md)
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md)
