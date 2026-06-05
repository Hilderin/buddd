# SPEC-CI-001 — CI Docker Image Pre-Publishing on ghcr.io

## Problem

Every CI run on `main` and pull requests builds the Docker image from `docker/ci.Dockerfile`. This takes ~3 minutes 30 seconds (52s for apt/ppa/pip package installation + ~2min30 for Docker buildx overhead with layer caching). The Dockerfile contents rarely change (only when compiler versions or system dependencies need updating). This wasted build time delays developer feedback on every CI run.

As documented in ADR-008, the image should be published to `ghcr.io/<org>/buddd-ci:latest` and rebuilt only when dependencies change.

## Goals

- Publish the CI Docker image to `ghcr.io/hilderin/buddd-ci` with tags `:latest` and `:<commit-sha>` whenever `docker/ci.Dockerfile` changes on `main`.
- Modify the existing CI workflow (`.github/workflows/ci.yml`) to pull the pre-published image instead of rebuilding it on every run.
- Preserve identical CI behavior (configure, build, test steps remain unchanged).
- Reduce per-run CI Docker image setup time from ~3min30s to ~15s for the vast majority of runs (where the Dockerfile hasn't changed).

## Non-goals

- No modification to the contents of `docker/ci.Dockerfile`.
- No modification to the configure, build, or test steps in `ci.yml` (they continue to use `docker run --rm` with the same arguments).
- No multi-platform image publishing (Linux/amd64 only, matching the `ubuntu-latest` runner).
- No automated vulnerability scanning or image signing.
- No automatic credential provisioning — maintainers handle GitHub token permissions manually.
- No migration of other workflows or local developer workflows (local builds continue using `buddd-ci:latest` tag as before).

## Actors

| Actor | Description |
|---|---|
| CI Workflow (publish) | GitHub Actions workflow that runs on push to `main` when `docker/ci.Dockerfile` changes. Builds and pushes the image to ghcr.io. |
| CI Workflow (ci) | Existing CI workflow modified to pull `ghcr.io/hilderin/buddd-ci:latest` instead of building the image from the Dockerfile. |
| Project Maintainer | Human who manages GitHub repository settings, secrets, and token permissions. May trigger initial publish manually. |
| Developer | Human contributor — no visible behavioral change. CI results are identical; feedback is faster. |

## User-visible behavior

- **No behavioral change for developers**: All CI steps (configure, build, test) run with the same compiler, dependencies, and environment as before.
- **CI runs finish faster**: The image build step is replaced by a ~15s pull, saving ~3 minutes per run on the median case.
- **Local development unchanged**: Developers building the Docker image locally with `docker build -f docker/ci.Dockerfile -t buddd-ci:latest .` continue to work exactly as before.
- **Maintainer visible**: A new `Publish CI Docker image` workflow appears in the GitHub Actions UI.

## User stories

### Story 1 — CI runs pull pre-published image (Priority: P1)

As a developer pushing code to a PR or `main`,
I want CI to pull the pre-published Docker image instead of building it,
So that I get faster feedback on my changes.

**Given** the CI workflow is triggered (push or PR),
**When** the workflow reaches the image setup step,
**Then** it pulls `ghcr.io/hilderin/buddd-ci:latest` instead of building from `docker/ci.Dockerfile`,
**And** the configure/build/test steps run in a container from that image,
**And** all steps produce identical results to the previous local-build approach.

### Story 2 — Publish workflow runs on Dockerfile changes (Priority: P1)

As a maintainer updating CI dependencies in `docker/ci.Dockerfile`,
I want the image to be automatically rebuilt and published to ghcr.io,
So that subsequent CI runs use the updated environment.

**Given** a commit is pushed to `main` that changes `docker/ci.Dockerfile`,
**When** the publish workflow completes,
**Then** `ghcr.io/hilderin/buddd-ci:latest` and `ghcr.io/hilderin/buddd-ci:<commit-sha>` reflect the new Dockerfile,
**And** subsequent CI runs pull the updated image.

### Story 3 — Manual publish trigger (Priority: P2)

As a maintainer,
I want to trigger the publish workflow manually,
So that I can publish the initial image or force a rebuild without pushing a fake Dockerfile change.

**Given** the publish workflow supports `workflow_dispatch`,
**When** a maintainer triggers it manually from the GitHub Actions UI,
**Then** it builds and pushes the image with the same tags (`:latest` and the commit SHA of the current `main` tip).

### Story 4 — CI works even if publish fails (Priority: P2)

As a maintainer,
I want CI to still function if the publish workflow fails for a given commit,
So that development is not blocked by transient registry or build issues.

**Given** the publish workflow failed for the latest commit on `main`,
**When** CI runs for a PR or subsequent push,
**Then** CI pulls the last successfully published `:latest` image (which may lag behind the Dockerfile by one commit),
**And** configure/build/test continue to work with the previous image.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | A `.github/workflows/publish.yml` file exists in the repository. | `git ls-files .github/workflows/publish.yml` exits 0. |
| AC-002 | `publish.yml` triggers **only** when `docker/ci.Dockerfile` changes on push to `main`. | Check workflow `on.push.paths` includes `docker/ci.Dockerfile` and `on.push.branches` includes `main`. No other trigger (e.g. PR, schedule) is present except `workflow_dispatch`. |
| AC-003 | `publish.yml` builds the image from `docker/ci.Dockerfile` and pushes to `ghcr.io/hilderin/buddd-ci` with both `:latest` and `:<commit-sha>` tags. | Inspect workflow YAML for `tags: ghcr.io/hilderin/buddd-ci:latest` and `ghcr.io/hilderin/buddd-ci:${{ github.sha }}` (or equivalent) in the `docker/build-push-action` step. |
| AC-004 | `publish.yml` supports `workflow_dispatch` trigger for manual invocation. | Check that `workflow_dispatch` is listed in the `on:` triggers. |
| AC-005 | `ci.yml` no longer builds the Docker image (no `docker/build-push-action` step). | `grep -c 'docker/build-push-action' .github/workflows/ci.yml` returns 0. |
| AC-006 | `ci.yml` includes a step that pulls `ghcr.io/hilderin/buddd-ci:latest` (via `docker pull` or equivalent). | Inspect `ci.yml` for a step referencing `ghcr.io/hilderin/buddd-ci:latest` and performing a pull (not a build). |
| AC-007 | All `docker run` commands in `ci.yml` use the image `ghcr.io/hilderin/buddd-ci:latest` instead of the local tag `buddd-ci:latest`. | `grep -c 'buddd-ci:latest' .github/workflows/ci.yml` returns 0; `grep -c 'ghcr.io/hilderin/buddd-ci:latest' .github/workflows/ci.yml` matches the number of `docker run` invocations (3). |
| AC-008 | The configure, build, and test commands in `ci.yml` are unchanged from the original workflow. | Diff the `run:` blocks of those three steps between old and new `ci.yml` — they are identical except for the image tag in the `docker run` arguments. |
| AC-009 | A published image can be pulled and used locally with the same commands as before. | `docker pull ghcr.io/hilderin/buddd-ci:latest && docker run --rm ghcr.io/hilderin/buddd-ci:latest bash -c "g++-16 --version && cmake --version && ninja --version"` succeeds and shows expected versions. |
| AC-010 | The CI workflow completes successfully (all 3 docker run steps pass) after the modification. | Run the CI workflow on a PR; all checks pass green. |

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | Median CI Docker image setup time drops from ~3min30s to ~15s or less (measured across 10 consecutive CI runs with no Dockerfile change). |
| SC-002 | Total CI workflow duration decreases by at least 3 minutes in the median case (from ~8-10 min to ~5-7 min). |
| SC-003 | Zero regressions in CI test results across 20 consecutive runs compared to the pre-change baseline. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **No image published yet** (first run after setup) | The publish workflow must be triggered manually (via `workflow_dispatch`) before the modified CI workflow can run. If `ci.yml` is merged before any publish, it will fail on `docker pull`. Maintainers should trigger a publish first, then merge the CI change. |
| **Publish workflow fails mid-run** | The `:latest` tag is pushed only after the build succeeds. A failure leaves the previous `:latest` intact. The `<commit-sha>` tag may still refer to the failed build if the push fails after tagging — but this is harmless because CI only pulls `:latest`. |
| **Dockerfile changes between publish and CI run** (race condition) | The publish workflow runs on the commit that changed the Dockerfile. CI runs triggered by the same commit (e.g. on push) will run concurrently or after the publish. If CI runs before publish finishes, it pulls the previous `:latest` — this is safe and still passes. On subsequent runs, CI gets the new image. |
| **Multiple commits changing the Dockerfile in quick succession** | Each pushes a new `:latest`. The last one to complete wins. CI always gets the winner. If a publish is slow and gets overwritten, that's fine — the last successful publish defines `:latest`. |
| **PR from a fork** | PRs from forks do not have access to the repository's GITHUB_TOKEN with `packages: write`. This is not a concern because publish only triggers on `main`, and CI on fork PRs can either read the public ghcr.io package (if it's public) or fall back. The spec assumes the ghcr.io package is public (matching the public repo). |
| **Local development with the published image** | Developers can optionally `docker pull ghcr.io/hilderin/buddd-ci:latest` and tag it locally as `buddd-ci:latest` to stay in sync. The old `docker build` approach still works. Both paths are supported. |

## Error cases

| Error | Consequence | Mitigation |
|---|---|---|
| `GITHUB_TOKEN` lacks `packages: write` scope | Publish workflow fails with a permissions error at the push step. | Document that the token needs `write: packages` in the workflow's `permissions` block. |
| `ghcr.io/hilderin/buddd-ci` package does not exist | `docker pull` in CI fails. | Initial publish must happen before CI workflow change is merged. Manual `workflow_dispatch` solves this. |
| Docker registry rate-limited or unavailable | CI pull fails. | CI run fails. This is an accepted risk (rare for public repos on ghcr.io). Manual re-run resolves transient failures. |
| `docker/ci.Dockerfile` is deleted | Publish workflow triggers (path filter catches deletion by default) but build may fail if the path is missing. | Workflow step checks file existence before building, or the standard build failure serves as a signal. |
| Published image is corrupted or incomplete | CI pull succeeds but container commands fail. | CI test failures indicate the issue. Maintainer triggers manual rebuild via `workflow_dispatch`. |

## Permissions and security

| Requirement | Detail |
|---|---|
| **Publish workflow `GITHUB_TOKEN`** | Must have `contents: read` (to checkout the repo) and `packages: write` (to push to ghcr.io). The `permissions` block in `publish.yml` should explicitly set these. A `docker/login-action@v3` step is required to authenticate with ghcr.io before pushing. |
| **CI workflow `GITHUB_TOKEN`** | Must have `contents: read` (to checkout the repo) and `packages: read` (to pull from ghcr.io). A `docker/login-action@v3` step is required before `docker pull` to authenticate with ghcr.io using the default `GITHUB_TOKEN`. |
| **ghcr.io package visibility** | The `ghcr.io/hilderin/buddd-ci` package should be **public** (matching the public repository). GitHub Actions pulling from a public package do not need authentication beyond the default token. |
| **No secrets needed** | The default `GITHUB_TOKEN` is sufficient for both workflows. No additional personal access tokens or secrets are required. |
| **Image contents** | The image contains only open-source tooling (Ubuntu packages, GCC, CMake, Ninja). No credentials, private keys, or proprietary code are embedded. |

## Observability

| Aspect | Approach |
|---|---|
| **Publish workflow logs** | Standard GitHub Actions log output. The `docker/build-push-action` step outputs the pushed digest. |
| **CI pull step** | The pull step logs which image digest was pulled (visible in the `docker pull` output). |
| **Failure alerts** | Standard GitHub Actions notifications for workflow failures. No additional alerting configured in this spec. |
| **Image version traceability** | The `<commit-sha>` tag allows tracing any published image back to the exact Dockerfile commit that produced it. |

## Out of scope

- Migrating the CI workflow to use a matrix strategy or multiple runners.
- Pre-caching CMake `FetchContent` dependencies (SDL3, GLM, Catch2) in the image.
- Publishing to multiple registries (Docker Hub, quay.io, etc.).
- Image signing, attestation, or SBOM generation.
- Automated cleanup of old `<commit-sha>` tags on ghcr.io.
- Changing the image build strategy (e.g. scheduled rebuilds, Dependabot for base image updates).

## Assumptions

| # | Assumption |
|---|---|
| A1 | The repository is **public** on GitHub. ghcr.io offers free, unlimited storage for public packages. Authentication for pull is implicit via the `GITHUB_TOKEN`. |
| A2 | The GitHub username is `Hilderin` and the image will be published to `ghcr.io/hilderin/buddd-ci`. |
| A3 | The initial publish will be triggered manually via `workflow_dispatch` before merging the CI workflow change. This ensures an image exists at the registry when CI first tries to pull it. |
| A4 | The `docker/ci.Dockerfile` contents change infrequently (weeks or months between updates). This makes the pre-publish strategy worthwhile — most CI runs have no Dockerfile change. |
| A5 | The `ubuntu-latest` GitHub Actions runner has Docker installed and authenticated with ghcr.io via the default `GITHUB_TOKEN`. |
| A6 | The GHA cache (`type=gha`) is no longer needed in `ci.yml` (since the image is pulled, not built). However, it may remain relevant for other optimizations — this is left to the implementer to decide. |

## Open questions

| # | Question | Impact |
|---|---|---|
| Q1 | [NEEDS CLARIFICATION] Should the CI workflow include a fallback: attempt to pull the image, and if it fails (e.g. image doesn't exist or registry unreachable), build it locally from the Dockerfile? A fallback would make the system more robust but adds complexity. Without it, CI fails hard if the pull fails. | **Scope**: adds ~5 lines of conditional logic. **Recommended default**: no fallback — a failed pull is an infrastructure issue that should be investigated. However, the initial transition requires a manual publish step. |
| Q2 | [NEEDS CLARIFICATION] Should the publish workflow also trigger on changes to files that are part of the Docker build context other than `docker/ci.Dockerfile` (e.g. scripts copied into the image)? Currently no such files exist, so this is a future-proofing concern. | **Scope**: if new context files are added, they won't trigger a publish. **Recommended default**: keep the trigger strictly on `docker/ci.Dockerfile` as documented in the human intent. Extend later if needed. |
