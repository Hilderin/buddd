# Setup

## Prerequisites

| Tool | Minimum version | Notes |
|---|---|---|---|
| CMake | >= 3.28 | Required for C++26 standard and preset support. Install via pip for latest version. |
| Ninja | >= 1.11 | Build system generator |
| C++ compiler | C++26-capable | GCC 16+ (reference), Clang 22+ |
| Docker | Latest | Optional — required only to run the CI build environment locally (see below) |
| clang-format | >= 18 | Optional — needed only for the `format` target |
| OpenGL | 4.5 Core | System headers and library (e.g., `libgl-dev` on Debian/Ubuntu, `mesa-libGL-devel` on Fedora) |

### Auto-fetched dependencies

The following are downloaded automatically at configure time via CMake's `FetchContent` — no manual installation required:
- **Catch2 v3.7.0** (test framework)
- **SDL3 release-3.2.30** (windowing and graphics context management)
- **GLM 1.0.1** (header-only math library — provides the implementation backend for Vec2, Vec3, Vec4, Mat4, Quat wrappers)

> **Note on compiled dependencies**: Large compiled dependencies (especially SDL3) are declared with `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` in `FetchContent_Declare`. This ensures SDL3 is built in Release mode regardless of the parent project's build type, avoiding debugger startup slowness caused by SDL3's own debug symbols. Header-only dependencies (like GLM) are unaffected, as they have no compiled artifacts. See `src/engine/CMakeLists.txt` for the current implementation and [ADR-007](/docs/adr/007-release-dependency-build.md) for the full rationale.

## Quick start

```bash
# Clone (if not already cloned)
git clone <repository-url>
cd buddd2

# Configure in Debug mode (fetches Catch2 and SDL3 automatically)
cmake --preset debug

# Build all targets
cmake --build --preset debug

# Run the CLI
./build/debug/src/cmd/buddd                   # prints "Buddd Engine v0.1.0"
./build/debug/src/cmd/buddd --version          # prints "buddd 0.1.0"

# Run tests (all headless tests pass without a display)
ctest --preset debug                           # 100% tests passed

# Build without SDL3 backend tests (headless-only, e.g., for CI)
cmake -DBUDDD_HAS_DISPLAY=OFF --preset debug
cmake --build --preset debug
ctest --preset debug

# On a headless CI runner without X11/Wayland development headers, also pass SDL_UNIX_CONSOLE_BUILD=ON:
cmake -DBUDDD_HAS_DISPLAY=OFF -DSDL_UNIX_CONSOLE_BUILD=ON --preset debug
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
- **`launch.json`** — Debugger configurations for `buddd` and `buddd_tests` using both **GDB** (with pretty-printing) and **LLDB**. All configurations trigger a pre-launch build task.

The GDB configurations require the [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) (ms-vscode.cpptools). The LLDB configurations require the [CodeLLDB extension](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb) (vadimcn.vscode-lldb).

## Docker CI environment

The CI pipeline runs inside a custom Docker image defined in `docker/ci.Dockerfile`. This image includes GCC 16+, the latest CMake (from pip), Ninja, and all system dependencies for headless builds.

### Build the image locally

```bash
docker build -f docker/ci.Dockerfile -t buddd-ci:latest .
```

### Run the CI build locally

```bash
docker run --rm \
  -v $(pwd):/workspace \
  -w /workspace \
  --user "$(id -u):$(id -g)" \
  buddd-ci:latest \
  bash -c "cmake --preset debug -DBUDDD_HAS_DISPLAY=OFF -DSDL_UNIX_CONSOLE_BUILD=ON -DCMAKE_CXX_COMPILER=g++-16 && cmake --build --preset debug && ctest --preset debug --output-on-failure"
```

Or step by step (useful for debugging failures):

```bash
# Configure
docker run --rm -v $(pwd):/workspace -w /workspace --user "$(id -u):$(id -g)" \
  buddd-ci:latest \
  cmake --preset debug -DBUDDD_HAS_DISPLAY=OFF -DSDL_UNIX_CONSOLE_BUILD=ON -DCMAKE_CXX_COMPILER=g++-16

# Build
docker run --rm -v $(pwd):/workspace -w /workspace --user "$(id -u):$(id -g)" \
  buddd-ci:latest \
  cmake --build --preset debug

# Test
docker run --rm -v $(pwd):/workspace -w /workspace --user "$(id -u):$(id -g)" \
  buddd-ci:latest \
  ctest --preset debug --output-on-failure
```

### How it works

1. The CI workflow (`.github/workflows/ci.yml`) builds the image from `docker/ci.Dockerfile` using `docker/build-push-action` with GitHub Actions layer caching (`type=gha`).
2. Each step (configure, build, test) runs in a separate container with the workspace mounted at `/workspace`.
3. The `--user "$(id -u):$(id -g)"` flag ensures file ownership matches the CI runner user.
4. The image uses GCC 16 as the default compiler (`CC=gcc-16`, `CXX=g++-16`).

### Publishing the image (future)

For faster CI, the image should be published to `ghcr.io/<org>/buddd-ci:latest` and rebuilt only when the Dockerfile changes. See [ADR-008](/docs/adr/008-ci-docker-infrastructure.md) for the full rationale.

## Reference

- Spec: [SPEC-001 Project Setup Bootstrap](/docs/specs/project-setup/spec.md) — Acceptance criteria AC-001 through AC-004 (build), AC-011 through AC-015 (formatting and IDE)
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — Done criteria and verification commands
- Spec: [SPEC-002 Platform Abstraction](/docs/specs/platform-abstraction/spec.md) — Build system integration, SDL3/OpenGL dependencies
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — CMakeLists.txt modifications, FetchContent SDL3, find_package OpenGL
- Spec: [SPEC-003 SDL3 Backend Tests](/docs/specs/sdl3-backend-tests/spec.md) — BUDDD_HAS_DISPLAY option
- Implementation contract: [IMPL-003](/docs/specs/sdl3-backend-tests/implementation-contract.md) — Build system changes, CI integration
- Spec: [SPEC-004 Math Foundations](/docs/specs/math-foundations/spec.md) — GLM FetchContent dependency, math module build integration
- Implementation contract: [IMPL-004](/docs/specs/math-foundations/implementation-contract.md) — CMakeLists.txt GLM FetchContent block, link configuration
