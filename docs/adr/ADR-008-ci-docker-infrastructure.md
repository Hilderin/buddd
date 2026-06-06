# ADR-008: Docker-based CI Infrastructure

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The Buddd Engine CI pipeline (`.github/workflows/ci.yml`) needs a reliable, reproducible build environment. The project targets C++26 with specific compiler requirements (GCC 16+, Clang 22+ per ADR-001/ADR-005), and uses `FetchContent` to pull dependencies (SDL3, GLM, Catch2) at configure time.

The initial CI setup used the GitHub Actions `ubuntu-latest` runner directly, installing `g++-14` via `apt`. Two problems emerged:

1. **Compiler version mismatch**: The `ubuntu-latest` runner only provides GCC 12/13/14 out of the box. The project needs GCC 16+ for `std::optional<T&>` support (ADR-005). Installing newer GCC versions via PPA on every CI run adds latency and is fragile.

2. **Non-reproducible environment**: The runner image changes over time (Ubuntu 22.04 → 24.04 → ...), and `apt` package versions drift. CI failures caused by environment changes are hard to diagnose.

### Alternatives considered

1. **PPA installation in workflow** — Add `sudo add-apt-repository ppa:ubuntu-toolchain-r/test` and `apt-get install g++-16` as a workflow step. Simple but adds ~30-60s per run and doesn't solve reproducibility.

2. **GitHub-hosted GCC 16** — Not available in the default runner images.

3. **Docker container with pre-installed toolchain** — Build an image once (or pull cached layers) containing GCC 16, CMake (latest), Ninja, and system deps. The CI runs inside this container.

4. **Self-hosted runner** — Full control but adds maintenance burden and is not portable.

## Decision

We adopt **option 3: Docker-based CI** using a custom image defined in `docker/ci.Dockerfile`.

### Image contents

- **Base**: Ubuntu 24.04
- **Compiler**: GCC 16+ from PPA `ubuntu-toolchain-r/test`
- **Build system**: CMake (latest via pip), Ninja
- **System deps**: `libgl1-mesa-dev`, `libglu1-mesa-dev`, `git`, `build-essential`
- **Default env**: `CC=gcc-16`, `CXX=g++-16`, headless mode

### CI workflow

The workflow (`ci.yml`) follows a three-step pattern:

1. **Build Docker image** using `docker/build-push-action` with GitHub Actions cache (`type=gha`). Subsequent runs reuse cached layers.
2. **Configure** — `cmake --preset debug` with `BUDDD_HAS_DISPLAY=OFF` inside the container.
3. **Build & Test** — `cmake --build` and `ctest` in separate container runs with the workspace mounted.

Each step runs in a fresh container using `--user "$(id -u):$(id -g)"` to match the runner's file ownership.

### Why not a pre-published image on ghcr.io

In the future, the image should be published to `ghcr.io/<org>/buddd-ci:latest` and rebuilt only when dependencies change. For now, building from the Dockerfile on each run with layer caching is sufficient.

## Consequences

### Positive

- **Reproducible**: The same `docker/ci.Dockerfile` produces the same environment regardless of runner image changes.
- **Consistent local development**: Developers can run the exact CI environment locally: `docker run --rm -v $(pwd):/workspace -w /workspace buddd-ci:latest ...`
- **Fast caching**: Docker layer caching (`type=gha`) means only changed layers are rebuilt on subsequent runs.
- **Compiler flexibility**: Changing the GCC version is a one-line edit in the Dockerfile + rebuild.
- **Headless-friendly**: The image works on any runner without display hardware.

### Negative

- **Docker build time**: The initial image build takes ~2-3 minutes even with caching (installing GCC from PPA, pip cmake).
- **Docker complexity**: Team members need basic Docker knowledge to debug CI issues locally.
- **Dependency downloads**: SDL3, GLM, and Catch2 are still fetched via `FetchContent` on every fresh configure (not pre-cached in the image).

### Migration

- Existing workflow was replaced: from native `apt-get install g++-14` to Docker-based GCC 16.
- The old Ubuntu 22.04 runner would have been deprecated anyway; Docker insulates from runner version changes.

## Related

- ADR-001: Project-wide `Result<T>` / `Error` Pattern — established C++26 compiler baseline.
- ADR-005: `std::optional<T&>` for Component Lookup API — requires GCC 16+.
- `docker/ci.Dockerfile` — Canonical CI image definition.
- `.github/workflows/ci.yml` — CI workflow using Docker.
- `docs/wiki/engineering/setup.md` — Local development setup, including Docker usage.
