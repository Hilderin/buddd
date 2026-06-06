# ADR-015: CI Docker Image Pre-Publishing

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

ADR-008 adopted a Docker-based CI infrastructure using a custom image defined in `docker/ci.Dockerfile`. The initial implementation built the image on every CI run using `docker/build-push-action` with GitHub Actions layer caching (`type=gha`). ADR-008 explicitly deferred pre-publication to a registry:

> In the future, the image should be published to `ghcr.io/<org>/buddd-ci:latest` and rebuilt only when dependencies change. For now, building from the Dockerfile on each run with layer caching is sufficient.

### Problems with build-on-every-run

1. **Slow CI feedback**: Every CI run — regardless of whether the Dockerfile changed — spent ~3m30s building the image (52s for apt/ppa/pip package installation + ~2m30s for Docker buildx overhead with layer caching). Most runs have no Dockerfile change, so this time was wasted.

2. **Dockerfile changes are rare**: The image contents (GCC version, CMake, Ninja, system deps) change infrequently — weeks or months between updates. Rebuilding every time is disproportionate to the rate of actual change.

### Alternatives considered

1. **Status quo (build on every run)** — Simple but wastes ~3m30s per CI run on average. Layer caching mitigates but does not eliminate the overhead. Rejected.

2. **Pre-publish to ghcr.io (selected)** — Build and publish the image only when `docker/ci.Dockerfile` changes. CI pulls the pre-published image in ~15s. The approach documented in ADR-008 as future work.

3. **Use `docker/build-push-action` with `pull: true`** — The `docker/build-push-action` action does not support a pull-only mode; it always builds. Requires using raw `docker pull` instead. Rejected for technical infeasibility.

4. **Fallback to local build on pull failure** — If the pull fails (registry unavailable, image not found), build locally from the Dockerfile. Adds complexity (~5 lines of conditional logic) and masks infrastructure issues. Rejected — a failed pull is an infrastructure failure that should be investigated, not silently worked around.

5. **Multi-registry publishing** — Publishing to Docker Hub or other registries in addition to ghcr.io. Adds complexity with no current benefit. Out of scope.

## Decision

We pre-publish the CI Docker image to `ghcr.io/hilderin/buddd-ci` and modify the CI workflow to pull this pre-published image instead of building it on every run.

### Architecture

A new publish workflow (`.github/workflows/publish.yml`) handles image publishing. The existing CI workflow (`.github/workflows/ci.yml`) is modified to pull instead of build.

```
docker/ci.Dockerfile change on main
          │
          ▼
  publish.yml ──build──► ghcr.io/hilderin/buddd-ci:latest
                        ghcr.io/hilderin/buddd-ci:<sha>
          ▲
          │
  ci.yml ──pull──► (configure / build / test in container)
```

### Publish workflow details

| Aspect | Decision |
|---|---|
| **Trigger** | Push to `main` modifying `docker/ci.Dockerfile` + manual `workflow_dispatch` |
| **Registry** | `ghcr.io/hilderin/buddd-ci` (user namespace, not org) |
| **Tags** | `:latest` (rolling) + `:<commit-sha>` (permanent traceability) |
| **Push mechanism** | `docker/login-action@v3` + `docker/build-push-action@v6` with `push: true` |
| **Permissions** | `contents: read`, `packages: write` |

### CI workflow changes

| Aspect | Before | After |
|---|---|---|
| **Image setup** | `docker/setup-buildx-action` + `docker/build-push-action` with GHA cache | `docker/login-action@v3` + `docker pull ghcr.io/hilderin/buddd-ci:latest` |
| **Image reference** | `buddd-ci:latest` (local tag) | `ghcr.io/hilderin/buddd-ci:latest` (registry reference) |
| **Build cache** | `type=gha` (Docker layer caching) | None needed (image is pulled, not built) |
| **Fallback** | N/A (always builds successfully or fails) | No fallback — hard failure on pull failure |

### Non-decisions (explicitly out of scope)

- Image signing, attestation, or SBOM generation.
- Automated cleanup of old `<commit-sha>` tags on ghcr.io.
- Multi-platform image publishing (Linux/amd64 only).
- Scheduled rebuilds or Dependabot for base image updates.
- Pre-caching `FetchContent` dependencies (SDL3, GLM, Catch2) in the image.

### Supersedes

This ADR supersedes the following sections of ADR-008:

- **"Why not a pre-published image on ghcr.io"** (lines 51–53) — This section is now obsolete. The pre-publication is implemented.
- **"CI workflow"** (lines 41–49) — The described three-step pattern (build, configure, build & test) is replaced. The build step no longer exists in `ci.yml`; it has moved to `publish.yml`.
- **"Negative" consequences** (line 67 — "Docker build time") — This negative is now mitigated. CI no longer spends 2-3 minutes on Docker build overhead.

The core decision of ADR-008 (use a custom Docker image for CI) remains in full effect.

## Consequences

### Positive

- **Faster CI feedback**: Median Docker image setup time drops from ~3m30s to ~15s for the vast majority of runs (where the Dockerfile hasn't changed).
- **Separation of concerns**: Image building is decoupled from CI execution. The publish workflow is the single place where image creation happens.
- **Traceability**: Every published image has a `<commit-sha>` tag linking it to the exact Dockerfile that produced it.
- **Same local development**: Developers can still `docker build -f docker/ci.Dockerfile -t buddd-ci:latest .` locally. No workflow changes for local use.
- **Reduced CI complexity**: No GHA cache configuration needed in `ci.yml`; no `docker/setup-buildx-action` step.

### Negative

- **Registry dependency**: CI now depends on `ghcr.io` availability. If the registry is unreachable, CI fails. Mitigation: `ghcr.io` is highly available for public repositories; transient failures are handled by re-running the workflow.
- **Deployment ordering**: The modified `ci.yml` must not be merged before the initial image is published. If merged too early, CI fails on `docker pull`. Mitigation: manual `workflow_dispatch` trigger for the initial publish.
- **Stale image on publish failure**: If the publish workflow fails, CI continues to use the last successfully published `:latest` image, which may lag behind the Dockerfile by one commit. This is an accepted risk — the image rarely changes and the lag is temporary.
- **Two workflows to maintain**: The publish workflow adds a new CI surface that needs monitoring and maintenance.

### Migration

- The initial publish MUST be triggered manually via `workflow_dispatch` before or simultaneously with the merge of the modified `ci.yml`.
- After migration, maintainers should monitor the first few CI runs to confirm the pull succeeds and the image works correctly.
- No changes to developer local workflows are required.

## Related

- ADR-008: Docker-based CI Infrastructure — established the Docker CI pattern that this ADR extends.
- `.github/workflows/publish.yml` — New workflow for publishing the CI Docker image.
- `.github/workflows/ci.yml` — Modified CI workflow that pulls the pre-published image.
- `docker/ci.Dockerfile` — Canonical CI image definition.
- `.specs/sprint-2026-06/ci-docker-publish/spec.md` — Product specification for this feature.
