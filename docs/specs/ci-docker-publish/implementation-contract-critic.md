# Implementation Contract Review — CI Docker Image Pre-Publishing on ghcr.io (Re-review)

## Summary

All previously identified blocking issues have been resolved. The contract now correctly uses `docker/login-action@v3` + `docker pull` in `ci.yml` (instead of `docker/build-push-action`), and `docker/login-action@v3` + `docker/build-push-action` with `push: true` in `publish.yml`. The `docker/setup-buildx-action` has been correctly removed from `ci.yml`. All image references use `ghcr.io/hilderin/buddd-ci:latest`. The contract is technically correct, complete with respect to the spec, precise, and verifiable.

**Verdict**: accepted — no blocking issues remain.

---

## Blocking issues

Items that must be resolved before the artifact can be accepted:

*(None — all previously identified blocking issues are resolved.)*

### Previously resolved issues (carried forward for history)

- [x] **CI pull step used `docker/build-push-action` which builds instead of pulling** — **RESOLVED.**
  The contract now correctly uses `docker/login-action@v3` + `docker pull ghcr.io/hilderin/buddd-ci:latest` (Section 2b, lines 155–165). This actually pulls the pre-published image from the registry. The `docker pull` command is a `run:` step, not a `uses:` action.

- [x] **Missing `docker/login-action` for ghcr.io pull in `ci.yml`** — **RESOLVED.**
  The contract now includes `docker/login-action@v3` as the first step after checkout in `ci.yml` (Section 2b, lines 156–161), authenticating with `registry: ghcr.io`, `username: ${{ github.actor }}`, `password: ${{ secrets.GITHUB_TOKEN }}`.

- [x] **`git diff` done-criteria check wouldn't detect new `publish.yml`** — **RESOLVED.**
  The done criteria now uses `git status --porcelain` (item 3, line 374), which correctly detects untracked/new files.

---

## Warnings

Non-blocking concerns for awareness:

- [ ] **Spec-critic warning about AC-009 (`--user` flag) carried forward**
  AC-009's verification command (`docker run --rm ghcr.io/hilderin/buddd-ci:latest bash -c ...`) omits the `--user "$(id -u):$(id -g)"` flag used by the workflow. The contract does not include AC-009 in its scope (correctly — it's outside the Code Agent's domain), but the pattern could be misleading.

- [ ] **Spec-critic warning about deployment ordering risk carried forward**
  Merging `ci.yml` before the initial manual publish breaks CI. The contract documents this as an edge case (first row) and explicitly says "by design" — no hard guard is added. This is consistent with the spec.

- [ ] **No explicit AC for Story 4 (CI resilience on publish failure) carried forward**
  The spec describes but does not explicitly verify the behavior where CI pulls the last successful `:latest` if publish fails. The contract's edge cases (row: "Publish workflow fails mid-run") cover this adequately at the design level.

- [ ] **Action version convention lists `docker/setup-buildx-action@v3`**
  The "Existing conventions to follow" table lists `docker/setup-buildx-action@v3` alongside other pinned action versions, but the contract removes this action from `ci.yml`. The convention is about version format, not about which actions must be present — but a code agent could briefly wonder about the contradiction. Mitigated by explicit removal instructions in sections 2a and 2d.

---

## Required changes

None — the contract is technically correct, consistent with the accepted spec, and all verifications are actionable.

---

## Suggested improvements

Optional ideas (not required):

- **Add a `--user` note to the wiki-update task** — The wiki-agent could be advised to include `--user "$(id -u):$(id -g)"` in any `docker run` examples to match actual workflow usage.
- **Consider a pre-merge guard** — A GitHub Action workflow check (or a PR template note) warning that `ci.yml` must be merged only after an initial manual publish, reducing the deployment ordering risk.
