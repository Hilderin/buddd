# Spec Review — CI Docker Image Pre-Publishing on ghcr.io (Re-review #3)

## Summary

The Observability section contradiction identified in Re-review #2 has been fixed. Line 146 now correctly references `docker pull` output instead of `docker/build-push-action`. All three remaining references to `docker/build-push-action` in the spec are correctly scoped to the publish workflow (`publish.yml`), not the CI workflow (`ci.yml`). The spec is now technically correct, internally consistent, and all acceptance criteria are testable.

**Verdict**: accepted — no blocking issues remain.

---

## Blocking issues

Items that must be resolved before the artifact can be accepted:

*(None — all previously identified issues are resolved.)*

### Previously resolved issues (carried forward for history)

- [x] ~~**Observability section contradicts AC-005 — references `docker/build-push-action` for CI pull step**~~ — **RESOLVED.**
  Line 146 now reads: `| **CI pull step** | The pull step logs which image digest was pulled (visible in the \`docker pull\` output). |`
  This correctly describes `docker pull` output, not `docker/build-push-action`. No contradiction remains.

- [x] ~~**AC-005 contradicts AC-006 on use of `docker/build-push-action` in `ci.yml`**~~ — **RESOLVED.**
  AC-005 (line 95) requires zero occurrences of `docker/build-push-action` in `ci.yml`. AC-006 (line 96) specifies `docker pull`. The Permissions section (lines 135–136) correctly requires `docker/login-action@v3` for both workflows.

---

## Warnings

Non-blocking concerns for awareness:

### Previously noted (still applicable)

- [ ] **Wiki update not addressed**
  `docs/wiki/engineering/setup.md` describes the current CI workflow (building the image from the Dockerfile) and has a "Publishing the image (future)" section. After implementation this will be stale. The spec does not task anyone with updating the wiki.

- [ ] **No AC for Story 4 (CI resilience on publish failure)**
  Story 4 describes desirable behavior — CI pulls the last successfully published `:latest` if the publish workflow fails. However, there is no AC that explicitly verifies this. While this is an emergent property of the design, an explicit AC or design note would improve regression-testability.

- [ ] **Deployment ordering risk (Assumption A3)**
  The spec correctly documents that the initial manual publish must complete before the CI workflow change is merged (edge case table, row 1). If merged in the wrong order, CI on `main` fails immediately. No hard guard against this is specified.

- [ ] **AC-009 verification command omits `--user` flag**
  The existing CI workflow uses `--user "$(id -u):$(id -g)"` to match runner file ownership. AC-009's verification (`docker run --rm ghcr.io/hilderin/buddd-ci:latest bash -c ...`) does not include this flag. Fine for a read-only version check, but could be misleading as a pattern.

### New warnings

- [ ] **Implementation contract is stale**
  The existing `docs/specs/ci-docker-publish/implementation-contract.md` was rejected during loop #2. It still uses `docker/build-push-action` for the CI pull step and states "No `docker/login-action` needed." With the spec now accepted, the implementation contract must be rewritten to match the corrected spec.

---

## Required changes

- None — the spec is technically correct and consistent.

---

## Suggested improvements

Optional ideas (not required):

- **Add a `--user` note to AC-009** — Include `--user "$(id -u):$(id -g)"` in the verification command to match the actual workflow usage, even though it's not required for the version check.
- **Consider a deployment ordering guard** — An explicit workflow-level check in ci.yml that fails with a helpful message if the image does not exist (e.g., `docker pull` failure) rather than silently proceeding would improve the initial deployment experience.
- **Add explicit branching note to `workflow_dispatch`** — Story 3 says `workflow_dispatch` builds from "the current `main` tip". The YAML trigger does not enforce this — a note about using `ref: main` in the workflow dispatch configuration would improve clarity.
