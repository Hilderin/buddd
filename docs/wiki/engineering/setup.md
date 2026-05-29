# Setup

## Prerequisites

| Tool | Minimum version | Notes |
|---|---|---|
| CMake | >= 3.28 | Required for C++26 standard and preset support |
| Ninja | >= 1.11 | Build system generator |
| C++ compiler | C++26-capable | GCC 14+ (reference), Clang 19+ |
| clang-format | >= 18 | Optional — needed only for the `format` target |

## Quick start

```bash
# Clone (if not already cloned)
git clone <repository-url>
cd buddd2

# Configure in Debug mode
cmake --preset debug

# Build all targets
cmake --build --preset debug

# Run the CLI
./build/debug/src/cmd/buddd                   # prints "Buddd Engine v0.1.0"
./build/debug/src/cmd/buddd --version          # prints "buddd 0.1.0"

# Run tests
ctest --preset debug                           # 100% tests passed
```

## Build presets

Two presets are available:

| Preset | Build type | Binary directory |
|---|---|---|
| `debug` | Debug | `build/debug/` |
| `release` | Release | `build/release/` |

Configure + build in one step:

```bash
cmake --preset release && cmake --build --preset release
```

## Formatting

Apply `clang-format` to all C++ sources under `src/` and `tests/`:

```bash
cmake --build --preset debug --target format
```

Requires `clang-format >= 18` on `PATH`. If not found, the target prints a clear error and exits non-zero.

## VS Code integration

The repository includes `.vscode/` workspace configuration files:

- **`settings.json`** — IntelliSense configured for C++26, C23, and project include paths (`src/engine`, `src/`). Format on save enabled with the C/C++ extension as default formatter.
- **`tasks.json`** — Shell tasks for CMake configure, build (default), and CTest run, all targeting the Debug preset.
- **`launch.json`** — Debugger configurations for `buddd` and `buddd_tests` using `gdb` with pretty-printing. Both trigger a pre-launch build task.

These files require the [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) (ms-vscode.cpptools).

## Reference

- Spec: [SPEC-001 Project Setup Bootstrap](/docs/specs/project-setup/spec.md) — Acceptance criteria AC-001 through AC-004 (build), AC-011 through AC-015 (formatting and IDE)
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — Done criteria and verification commands
