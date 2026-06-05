# IMPL-006 — CI Docker Image Pre-Publishing on ghcr.io

## Source spec

`.specs/sprint-2026-06/ci-docker-publish/spec.md` (SPEC-CI-001), accepted with no blocking issues according to `.specs/sprint-2026-06/ci-docker-publish/spec-critic.md`.

## Goal

Create a GitHub Actions workflow (`.github/workflows/publish.yml`) to publish the CI Docker image to `ghcr.io/hilderin/buddd-ci` when `docker/ci.Dockerfile` changes on `main`, and modify the existing CI workflow (`.github/workflows/ci.yml`) to pull the pre-published image instead of building it on every run — reducing per-run CI image setup time from ~3min30s to ~15s. The CI workflow uses `docker/login-action@v3` for ghcr.io authentication and `docker pull` to fetch the pre-built image.

## Non-goals

- No modification to the contents of `docker/ci.Dockerfile`.
- No modification to the configure, build, or test commands in `ci.yml` — only the image tag and the Docker build step change.
- No multi-platform image publishing (Linux/amd64 only, matching the `ubuntu-latest` runner).
- No automated vulnerability scanning, image signing, attestation, or SBOM generation.
- No addition of new dependencies to the project.
- No modification to source code (`src/`), test files (`tests/`), CMake files, or any build system configuration.
- No changes to local developer workflows — `docker build -f docker/ci.Dockerfile -t buddd-ci:latest .` continues to work as before.
- No modification of any wiki or documentation files.
- No fallback build logic if the pull fails — CI fails hard if the image cannot be pulled.
- No automated cleanup of old `<commit-sha>` tags on ghcr.io.

## Relevant constitution rules

- **CONST-002 (testing-policy.md)**: Requires tests for "all testable code." Workflow YAML is configuration, not testable code — no unit tests are required. Correctness is verified by YAML validation and post-merge GitHub Actions execution.
- **CONST-003 (documentation-policy.md)**: No blocking constraints.
- **CONST-004 (security-policy.md)**: No blocking constraints.

## Relevant ADRs

- **ADR-008** (`docs/adr/008-ci-docker-infrastructure.md`): This implementation acts on the deferred intent stated in the "Why not a pre-published image on ghcr.io" section — to publish the CI image to ghcr.io and rebuild only when dependencies change.

## Files to inspect

| File | Purpose |
|---|---|
| `.github/workflows/ci.yml` | Existing CI workflow — understand current step structure, image tag, cache configuration, and `docker run` patterns. Must be modified. |
| `docker/ci.Dockerfile` | Dockerfile contents — confirm no changes needed and understand the build context. |
| `docs/adr/008-ci-docker-infrastructure.md` | ADR context documenting the deferred decision to publish to ghcr.io. |
| `.specs/sprint-2026-06/ci-docker-publish/spec.md` | Source spec with full acceptance criteria, edge cases, and permission requirements. |
| `.specs/sprint-2026-06/ci-docker-publish/spec-critic.md` | Spec review confirming no blocking issues remain. Lists warnings that may affect implementation choices. |
| `.specs/sprint-2026-06/ci-docker-publish/coordination.md` | Workflow coordination document — must be updated after contract writing. |
| `.specs/sprint-2026-06/ci-docker-publish/implementation-contract-critic.md` | Previous contract review that identified blocking issues (docker/build-push-action does not pull) — the current rewrite resolves these issues. |

## Files allowed to change

### Create (1 file)

1. `.github/workflows/publish.yml` — New publish CI Docker image workflow.

### Modify (1 file)

2. `.github/workflows/ci.yml` — Replace the "Build CI Docker image" step with a `docker/login-action` + `docker pull` sequence, remove `docker/setup-buildx-action`, remove GHA cache configuration, and update all `docker run` image references.

## Files forbidden to change

- `docker/ci.Dockerfile` — No changes to contents.
- Any file under `src/` — No source code changes.
- Any file under `tests/` — No test file changes.
- Any `CMakeLists.txt` file (root or subdirectories) — No build system changes.
- `CMakePresets.json` — No preset changes.
- `.clang-format`, `.vscode/`, `opencode.json`, `AGENTS.md`, `SpecKit.md` — No tooling changes.
- Any other `.github/workflows/*.yml` file — only `ci.yml` may be modified.
- Any documentation files under `docs/` not explicitly listed in "Files allowed to change" — including wiki, ADR, and constitution files.

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Action versions | Use pinned major versions: `actions/checkout@v4`, `docker/login-action@v3`, `docker/setup-buildx-action@v3`, `docker/build-push-action@v6`. |
| YAML style | 2-space indentation, consistent with existing `ci.yml`. |
| Step naming | Short descriptive names matching the existing style: e.g., "Log in to ghcr.io", "Pull CI Docker image", "Configure (headless)", "Build", "Test". |
| Docker run pattern | All `docker run` commands use the exact same parameter template: `--rm`, `-v ${{ github.workspace }}:/workspace`, `-w /workspace`, `--user "$(id -u):$(id -g)"`. Commands that take multiple lines use `\|` block scalar style. |
| Image tag in docker run | Use the full `ghcr.io/hilderin/buddd-ci:latest` tag, matching the published registry path. |
| `workflow_dispatch` placement | Listed as a top-level `on:` trigger alongside `push`. |
| `docker/login-action` placement | Placed immediately after `actions/checkout` and before any step that interacts with the registry. |

## Required implementation behavior

### 1. Create `.github/workflows/publish.yml`

Create a new workflow file with the following content:

```yaml
name: Publish CI Docker image

on:
  push:
    branches: [ main ]
    paths:
      - docker/ci.Dockerfile
  workflow_dispatch:

permissions:
  contents: read
  packages: write

jobs:
  publish:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Log in to ghcr.io
        uses: docker/login-action@v3
        with:
          registry: ghcr.io
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}

      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3

      - name: Build and push CI Docker image
        uses: docker/build-push-action@v6
        with:
          context: .
          file: docker/ci.Dockerfile
          tags: |
            ghcr.io/hilderin/buddd-ci:latest
            ghcr.io/hilderin/buddd-ci:${{ github.sha }}
          push: true
```

Requirements:
- `on.push.branches` is `[ main ]` (exact string, list format).
- `on.push.paths` includes exactly `docker/ci.Dockerfile` as the path filter.
- `on.workflow_dispatch` is present as a top-level trigger (no branch restriction).
- `permissions` block MUST explicitly set `contents: read` and `packages: write` as shown.
- `docker/login-action@v3` step MUST be present before `docker/setup-buildx-action` and `docker/build-push-action`, with `registry: ghcr.io`, `username: ${{ github.actor }}`, `password: ${{ secrets.GITHUB_TOKEN }}`.
- No `cache-from` or `cache-to` entries — building fresh each time is acceptable.
- Tags use the YAML list format (`\|` with one tag per line), NOT the inline comma-separated format.
- `${{ github.sha }}` is used for the commit SHA tag (not `github.ref_name` or any other variable).

### 2. Modify `.github/workflows/ci.yml`

Perform three categories of changes:

#### 2a. Remove the "Set up Docker Buildx" step

Delete the entire step:
```yaml
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3
```

This step is removed because the CI workflow no longer runs any `docker buildx build` commands. The `docker pull` and `docker run` commands do not require Buildx.

#### 2b. Replace the "Build CI Docker image" step with login + pull

Replace the entire build step with two steps:

```yaml
      - name: Log in to ghcr.io
        uses: docker/login-action@v3
        with:
          registry: ghcr.io
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}

      - name: Pull CI Docker image
        run: docker pull ghcr.io/hilderin/buddd-ci:latest
```

Requirements:
- `docker/login-action@v3` MUST use `registry: ghcr.io`, `username: ${{ github.actor }}`, `password: ${{ secrets.GITHUB_TOKEN }}`.
- The pull step MUST use `docker pull ghcr.io/hilderin/buddd-ci:latest` — NOT `docker/build-push-action`.
- No `load: true`, no `pull: true`, no `cache-from`, no `cache-to` — these are `docker/build-push-action` parameters that are irrelevant to `docker pull`.
- The `GITHUB_TOKEN` secret is the default token provided by GitHub Actions — no additional secrets are needed.

#### 2c. Update all `docker run` image references

In all three steps ("Configure (headless)", "Build", "Test"), replace the local image tag `buddd-ci:latest` with the full registry path `ghcr.io/hilderin/buddd-ci:latest`.

The three `docker run` commands must become:

**Configure (headless):**
```yaml
      - name: Configure (headless)
        run: |
          docker run --rm \
            -v ${{ github.workspace }}:/workspace \
            -w /workspace \
            --user "$(id -u):$(id -g)" \
            ghcr.io/hilderin/buddd-ci:latest \
            cmake --preset debug \
              -DBUDDD_HAS_DISPLAY=OFF \
              -DSDL_UNIX_CONSOLE_BUILD=ON \
              -DCMAKE_CXX_COMPILER=g++-16
```

**Build:**
```yaml
      - name: Build
        run: |
          docker run --rm \
            -v ${{ github.workspace }}:/workspace \
            -w /workspace \
            --user "$(id -u):$(id -g)" \
            ghcr.io/hilderin/buddd-ci:latest \
            cmake --build --preset debug
```

**Test:**
```yaml
      - name: Test
        run: |
          docker run --rm \
            -v ${{ github.workspace }}:/workspace \
            -w /workspace \
            --user "$(id -u):$(id -g)" \
            ghcr.io/hilderin/buddd-ci:latest \
            ctest --preset debug --output-on-failure
```

Requirements:
- The **only** change in each `docker run` command is the image tag — `buddd-ci:latest` → `ghcr.io/hilderin/buddd-ci:latest`.
- All other parameters (`--rm`, `-v`, `-w`, `--user`, cmake/ctest flags) are **identical** to the original.
- The three step names ("Configure (headless)", "Build", "Test") remain unchanged.

#### 2d. Final ci.yml structure

After all changes, the complete `ci.yml` must have the following structure (no extra or missing steps):

```yaml
name: CI

on:
  push:
  pull_request:
    branches: [ main ]

jobs:
  build-and-test:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Log in to ghcr.io
        uses: docker/login-action@v3
        with:
          registry: ghcr.io
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}

      - name: Pull CI Docker image
        run: docker pull ghcr.io/hilderin/buddd-ci:latest

      - name: Configure (headless)
        run: |
          docker run --rm \
            -v ${{ github.workspace }}:/workspace \
            -w /workspace \
            --user "$(id -u):$(id -g)" \
            ghcr.io/hilderin/buddd-ci:latest \
            cmake --preset debug \
              -DBUDDD_HAS_DISPLAY=OFF \
              -DSDL_UNIX_CONSOLE_BUILD=ON \
              -DCMAKE_CXX_COMPILER=g++-16

      - name: Build
        run: |
          docker run --rm \
            -v ${{ github.workspace }}:/workspace \
            -w /workspace \
            --user "$(id -u):$(id -g)" \
            ghcr.io/hilderin/buddd-ci:latest \
            cmake --build --preset debug

      - name: Test
        run: |
          docker run --rm \
            -v ${{ github.workspace }}:/workspace \
            -w /workspace \
            --user "$(id -u):$(id -g)" \
            ghcr.io/hilderin/buddd-ci:latest \
            ctest --preset debug --output-on-failure
```

## Required tests

No unit tests apply to CI workflow YAML files. The following verification methods are used:

| Check | Method |
|---|---|
| YAML syntax validity | `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/publish.yml'))"` and `yaml.safe_load(open('.github/workflows/ci.yml'))` both succeed. |
| No `docker/build-push-action` in ci.yml | `grep 'docker/build-push-action' .github/workflows/ci.yml` returns no matches. |
| No `docker/setup-buildx-action` in ci.yml | `grep 'docker/setup-buildx-action' .github/workflows/ci.yml` returns no matches. |
| No `push: true` in ci.yml | `grep 'push: true' .github/workflows/ci.yml` returns no matches. |
| No `load: true` in ci.yml | `grep 'load: true' .github/workflows/ci.yml` returns no matches. |
| No `pull: true` in ci.yml | `grep 'pull: true' .github/workflows/ci.yml` returns no matches. |
| GHA cache removed from ci.yml | `grep -c 'type=gha' .github/workflows/ci.yml` returns 0. |
| `docker/login-action@v3` present in ci.yml | `grep 'docker/login-action@v3' .github/workflows/ci.yml` exits 0. |
| `docker pull` present in ci.yml | `grep 'docker pull' .github/workflows/ci.yml` exits 0. |
| Image tag migration complete | `grep -c 'buddd-ci:latest' .github/workflows/ci.yml` returns 0; `grep -c 'ghcr.io/hilderin/buddd-ci:latest' .github/workflows/ci.yml` returns exactly 4 (1 in `docker pull`, 3 in `docker run` commands). |
| docker run commands unchanged except tag | Diff the three `docker run` blocks between old and new ci.yml — they are identical except for the image tag. |
| publish.yml has `docker/login-action@v3` | `grep 'docker/login-action@v3' .github/workflows/publish.yml` exits 0. |
| publish.yml trigger correct | `grep -c 'docker/ci.Dockerfile' .github/workflows/publish.yml` returns 1. `grep 'workflow_dispatch' .github/workflows/publish.yml` exits 0. |
| publish.yml permissions | `grep 'packages: write' .github/workflows/publish.yml` exits 0. |
| publish.yml push | `grep 'push: true' .github/workflows/publish.yml` exits 0. |
| publish.yml tags include commit SHA | `grep '\\${{ github.sha }}' .github/workflows/publish.yml` exits 0. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **No image published yet** (first run after setup) | The publish workflow must be triggered manually (via `workflow_dispatch`) before the modified CI workflow can run. If `ci.yml` is merged before any publish, CI will fail on the pull step. This is by design — maintainers must trigger a publish first, then merge the CI change (per Assumption A3 in the spec). |
| **Publish workflow fails mid-run** | The `:latest` tag is pushed only after the build succeeds. A failure leaves the previous `:latest` intact. The `<commit-sha>` tag may still refer to the failed build if the push fails after tagging — but this is harmless because CI only pulls `:latest`. |
| **Dockerfile changes between publish and CI run** (race condition) | The publish workflow runs on the commit that changed the Dockerfile. CI runs triggered by the same commit may run concurrently or after the publish. If CI runs before publish finishes, it pulls the previous `:latest` — this is safe and still passes. On subsequent runs, CI gets the new image. |
| **Multiple commits changing the Dockerfile in quick succession** | Each pushes a new `:latest`. The last one to complete wins. CI always gets the winner. |
| **PR from a fork** | PRs from forks do not have access to the repository's GITHUB_TOKEN with `packages: write`. This is not a concern because publish only triggers on `main`. CI on fork PRs can read the public ghcr.io package using the `docker/login-action` with the default GITHUB_TOKEN. |
| **`docker/ci.Dockerfile` is deleted** | Publish workflow triggers (path filter catches deletion by default) but the build step will fail because the file does not exist. The workflow failure serves as a signal to the maintainer. |
| **Registry rate-limited or unavailable during CI pull** | CI pull step fails. This is an accepted risk (rare for public repos on ghcr.io). Manual re-run resolves transient failures. |
| **`GITHUB_TOKEN` lacks `packages: write` scope in publish workflow** | Publish workflow fails with a permissions error at the push step. The `permissions` block in publish.yml mitigates this — if it is removed or misconfigured, the workflow will fail. |
| **`GITHUB_TOKEN` lacks `packages: read` scope in CI workflow** | The `docker/login-action@v3` authentication fails and `docker pull` returns a permission denied error. This is mitigated by the default token permissions which include `contents: read` sufficient for pulling public packages; if the token scope changes in the future the CI will fail. |

## Security impact

- **GITHUB_TOKEN permissions**: The publish workflow explicitly sets `permissions: { contents: read, packages: write }`. The CI workflow uses the default GITHUB_TOKEN (sufficient for reading a public ghcr.io package). No additional secrets or personal access tokens are needed.
- **`docker/login-action` authentication**: Both workflows use `docker/login-action@v3` with the default `GITHUB_TOKEN`. This is the recommended approach for ghcr.io authentication in GitHub Actions. The credentials are ephemeral and scoped to the workflow run.
- **Image contents**: The image contains only open-source tooling (Ubuntu base, GCC 16, CMake, Ninja, OpenGL development headers). No credentials, private keys, or proprietary code are embedded.
- **No supply chain hardening**: Image signing, attestation, and SBOM generation are explicitly out of scope per the spec.

## Data and migration impact

None. No schema changes, data migrations, seed data, or persistent state modifications.

## API compatibility impact

None. No public API is changed.

## Documentation impact

- `docs/wiki/engineering/setup.md` — The "How it works" section (currently describing `type=gha` caching) and "Publishing the image (future)" section will become stale after implementation. Updating the wiki is explicitly assigned to the wiki-agent step and is NOT part of this implementation contract. The Code Agent must not modify wiki files.

## ADR impact

None. This implementation acts on the deferred intent in ADR-008. No new ADR is required.

## Constitution impact

None.

## Done criteria

The implementation is complete when all of the following are true:

1. **`publish.yml` exists** at `.github/workflows/publish.yml` with:
   - `on.push.branches: [ main ]` and `on.push.paths: [ docker/ci.Dockerfile ]`.
   - `on.workflow_dispatch` present.
   - `permissions: { contents: read, packages: write }` present.
   - `docker/login-action@v3` step with `registry: ghcr.io`, `username: ${{ github.actor }}`, `password: ${{ secrets.GITHUB_TOKEN }}`.
   - `docker/build-push-action@v6` step with `push: true` and tags listing both `ghcr.io/hilderin/buddd-ci:latest` and `ghcr.io/hilderin/buddd-ci:${{ github.sha }}`.
   - No `cache-from` or `cache-to`.
   - Valid YAML syntax.

2. **`ci.yml` modified** — Verify by checking:
   - No step named "Set up Docker Buildx" exists.
   - No step named "Build CI Docker image" exists (replaced by "Log in to ghcr.io" + "Pull CI Docker image").
   - `docker/login-action@v3` present with `registry: ghcr.io`, `username: ${{ github.actor }}`, `password: ${{ secrets.GITHUB_TOKEN }}`.
   - `docker pull ghcr.io/hilderin/buddd-ci:latest` present as a `run:` step (not a `uses:` action).
   - No `docker/build-push-action` in `ci.yml`.
   - No `docker/setup-buildx-action` in `ci.yml`.
   - No `push: true`, `load: true`, or `pull: true` in `ci.yml`.
   - No `cache-from: type=gha` or `cache-to: type=gha,mode=max` in `ci.yml`.
   - All 4 references to the image tag in `ci.yml` use `ghcr.io/hilderin/buddd-ci:latest` (1 in `docker pull`, 3 in `docker run` commands).
   - No references to the old local tag `buddd-ci:latest` remain in `ci.yml`.
   - The three `docker run` commands are identical to the original except for the image tag change.
   - Valid YAML syntax.

3. **Forbidden files unchanged** — `git status --porcelain` or `git diff --name-only --diff-filter=ACMR` lists only `.github/workflows/publish.yml` and `.github/workflows/ci.yml` (publish.yml may be untracked/new).

4. **Verification commands pass**:
   ```bash
   # YAML syntax validation
   python3 -c "import yaml; yaml.safe_load(open('.github/workflows/publish.yml')); print('publish.yml OK')"
   python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml')); print('ci.yml OK')"

   # No buildx in ci.yml
   grep -n 'docker/setup-buildx-action' .github/workflows/ci.yml || echo "No buildx in ci.yml (correct)"

   # No build-push-action in ci.yml
   grep -n 'docker/build-push-action' .github/workflows/ci.yml || echo "No build-push-action in ci.yml (correct)"

   # No push/load/pull in ci.yml
   grep -n 'push: true' .github/workflows/ci.yml || echo "No push in ci.yml (correct)"
   grep -n 'load: true' .github/workflows/ci.yml || echo "No load in ci.yml (correct)"
   grep -n 'pull: true' .github/workflows/ci.yml || echo "No pull in ci.yml (correct)"

   # No gha cache in ci.yml
   grep -n 'type=gha' .github/workflows/ci.yml || echo "No gha cache in ci.yml (correct)"

   # Old tag eliminated from ci.yml
   grep -n 'buddd-ci:latest' .github/workflows/ci.yml || echo "No old tags in ci.yml (correct)"

   # New tag present in ci.yml (expect 4 occurrences)
   grep -c 'ghcr.io/hilderin/buddd-ci:latest' .github/workflows/ci.yml

   # Login action present in ci.yml
   grep -n 'docker/login-action@v3' .github/workflows/ci.yml

   # Login action present in publish.yml
   grep -n 'docker/login-action@v3' .github/workflows/publish.yml

   # Publish workflow triggers and permissions correct
   grep 'docker/ci.Dockerfile' .github/workflows/publish.yml
   grep 'workflow_dispatch' .github/workflows/publish.yml
   grep 'packages: write' .github/workflows/publish.yml
   grep 'push: true' .github/workflows/publish.yml
   grep '\${{ github.sha }}' .github/workflows/publish.yml

   # Verify only the two expected files changed (including untracked)
   git status --porcelain
   ```
