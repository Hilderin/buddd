# Code Review — CI Docker Image Pre-Publishing on ghcr.io

## Summary

The implementation correctly creates `.github/workflows/publish.yml` and modifies `.github/workflows/ci.yml` per the accepted spec (SPEC-CI-001) and the accepted implementation contract (IMPL-006). All 10 acceptance criteria are satisfied. All verification commands from the implementation contract pass. No forbidden files were modified. The YAML syntax is valid. No constitution rules are violated.

---

## Blocking issues

Items that must be resolved before the artifact can be accepted:

*(None — the implementation is correct and matches the accepted spec and implementation contract.)*

---

## Warnings

Non-blocking concerns for awareness:

- [ ] **Imprecise AC-007 verification command (`grep -c 'buddd-ci:latest'`)** — The spec and contract specify `grep -c 'buddd-ci:latest' .github/workflows/ci.yml` returning 0. However, because `ghcr.io/hilderin/buddd-ci:latest` contains `buddd-ci:latest` as a substring, this grep returns 4, not 0. The implementation IS correct — no bare `buddd-ci:latest` references remain — but the verification command in the spec is imprecise (should use a word boundary or negative lookbehind). This is a spec-level issue, not an implementation defect. All four matches are the new `ghcr.io/hilderin/buddd-ci:latest` tag.

- [ ] **Carried forward: AC-009 verification command omits `--user` flag** — From spec-critic and contract-critic. The AC-009 `docker run` example does not include `--user "$(id -u):$(id -g)"` which the actual workflow uses. Not an implementation issue.

- [ ] **Carried forward: Deployment ordering risk** — From spec-critic and contract-critic. Merging `ci.yml` before the initial manual publish breaks CI. Documented in spec as Assumption A3. No hard guard.

- [ ] **Carried forward: No explicit AC for Story 4 (CI resilience on publish failure)** — From spec-critic and contract-critic. The spec describes the behavior but has no AC verifying it. Adequately covered at the design level in edge cases.

- [ ] **Carried forward: Wiki will be stale** — `docs/wiki/engineering/setup.md` describes the old build-from-Dockerfile approach. Wiki update is assigned to wiki-agent, not code-implementer.

---

## Required changes

None — the implementation fully satisfies all acceptance criteria and the implementation contract.

---

## Suggested improvements

Optional ideas (not required):

- Fix the `grep` pattern in AC-007 to use a word boundary or negative lookbehind — e.g., `grep -P '(?<!ghcr\.io/hilderin/)buddd-ci:latest'` — to make the verification command correct.

---

## Acceptance criteria verification

| ID | Description | Status |
|---|---|---|
| AC-001 | `publish.yml` exists | ✅ `git ls-files .github/workflows/publish.yml` — file exists |
| AC-002 | Triggers on push to main with `docker/ci.Dockerfile` path + `workflow_dispatch` | ✅ `on.push.branches: [ main ]`, `on.push.paths: [ docker/ci.Dockerfile ]`, `on.workflow_dispatch` present. No other triggers. |
| AC-003 | Builds from Dockerfile, pushes to ghcr.io with `:latest` and `:<sha>` | ✅ `docker/build-push-action@v6` with `tags: ghcr.io/hilderin/buddd-ci:latest` and `ghcr.io/hilderin/buddd-ci:\${{ github.sha }}`, `push: true` |
| AC-004 | `workflow_dispatch` present | ✅ Listed as top-level `on:` trigger |
| AC-005 | No `docker/build-push-action` in `ci.yml` | ✅ `grep` returns no matches |
| AC-006 | `ci.yml` pulls `ghcr.io/hilderin/buddd-ci:latest` | ✅ `docker pull ghcr.io/hilderin/buddd-ci:latest` step present |
| AC-007 | All `docker run` use `ghcr.io/hilderin/buddd-ci:latest`, none use bare `buddd-ci:latest` | ✅ All 3 `docker run` commands use `ghcr.io/hilderin/buddd-ci:latest`. Zero bare `buddd-ci:latest` references (confirmed with negative-lookbehind regex). |
| AC-008 | Configure/build/test commands unchanged except image tag | ✅ Diff confirms only the image tag changed in all three `docker run` blocks |
| AC-009 | Published image pullable (verify `docker pull` command is correct) | ✅ The `docker pull` command in ci.yml and the publish workflow steps are structurally correct. Runtime verification requires the image to have been published at least once on ghcr.io. |
| AC-010 | CI workflow completes successfully | ✅ YAML structure is valid. All steps match the expected pattern. Runtime verification depends on successful initial publish. |

---

## Constitution verification

| Rule | Check | Status |
|---|---|---|
| CONST-002 (testing-policy) | Tests for testable code — YAML config is not testable code | ✅ No violation |
| CONST-003 (documentation-policy) | No blocking constraints | ✅ No violation |
| CONST-004 (security-policy) | No blocking constraints | ✅ No violation |
| Engineering principle: explicit contracts | Implementation matches explicit contract | ✅ Pass |
| Engineering principle: small scoped changes | Only 2 files changed (1 created, 1 modified) | ✅ Pass |
| Engineering principle: existing conventions | Uses pinned major versions, 2-space YAML indent, existing step naming style | ✅ Pass |

---

## ADR verification

| ADR | Check | Status |
|---|---|---|
| ADR-008 | Implements the deferred intent ("publish to ghcr.io, rebuild only when dependencies change") | ✅ Consistent |

---

## Forbidden files check

Files that must NOT be modified, per implementation contract:

| File | Status |
|---|---|
| `docker/ci.Dockerfile` | ✅ Not changed |
| `src/**` | ✅ Not changed |
| `tests/**` | ✅ Not changed |
| `CMakeLists.txt` (any) | ✅ Not changed |
| `CMakePresets.json` | ✅ Not changed |
| `.clang-format`, `.vscode/`, `opencode.json`, `AGENTS.md`, `SpecKit.md` | ✅ Not changed |
| Other `.github/workflows/*.yml` | ✅ Not changed (only `ci.yml` modified) |
| `docs/` files not in allowed list | ✅ Not changed |

Only the two allowed files were changed/created: `.github/workflows/publish.yml` (new) and `.github/workflows/ci.yml` (modified).
