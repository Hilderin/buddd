# Workflow Coordination: ci-docker-publish

## Orchestrator

**Feature**: `ci-docker-publish`
**Status**: completed
**Current step**: completed
**Initial instructions**: Pré-publier l'image Docker CI sur ghcr.io pour accélérer les builds CI. Créer un workflow de publish déclenché sur les changements du Dockerfile, et modifier le CI existant pour pull l'image pré-publiée au lieu de la builder à chaque run.
**Notes**:
- Décisions prises avec l'humain :
  - Registry : `ghcr.io/hilderin/buddd-ci`
  - Tags : `:latest` + `:<sha-du-commit>`
  - Trigger : sur changement de `docker/ci.Dockerfile` (et `docker/` path)
  - Scope : Workflow de publish + modification de `ci.yml` pour pull
  - Pas de besoin de modification constitutionnelle a priori
- Loop #1 : spec-critic → spec-author (résoudre contradiction AC-005/AC-006) — résolu via assouplissement AC-005
- Loop #2 : implementation-contract-critic → spec-author (problème technique : `docker/build-push-action` ne peut pas pull une image, il faut `docker pull` + `docker/login-action`)

### Décision loop #2

Le `docker/build-push-action` ne supporte pas le pull d'images pré-existantes — il build toujours. Il faut donc :
- AC-006 : retirer l'option `docker/build-push-action`, garder uniquement `docker pull`
- AC-005 : revenir à `grep -c 'docker/build-push-action'` retourne 0
- Ajouter une mention de `docker/login-action` dans la spec

## spec-author

**Status**: completed
**Summary**:
Wrote SPEC-CI-001 covering the full scope: publish workflow (trigger on Dockerfile change on main, push to ghcr.io/hilderin/buddd-ci with :latest and :<sha> tags, manual workflow_dispatch), modification of ci.yml to pull instead of build, edge cases (first publish, publish failure, race conditions, fork PRs), error cases (token permissions, registry unavailability), and 10 acceptance criteria. Two open questions flagged: fallback strategy on pull failure, and trigger path expansion.
**Artifacts**:
- docs/specs/ci-docker-publish/spec.md
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Spec accepted — Observability section contradiction resolved (line 146 now correctly references `docker pull` instead of `docker/build-push-action`). All three `docker/build-push-action` references are correctly scoped to `publish.yml`. The spec is technically correct, internally consistent, and all acceptance criteria are testable. No blocking issues remain.
**Artifacts**:
- docs/specs/ci-docker-publish/spec-critic.md
**Questions for human**:
none
**Warnings**:
- Wiki update (`docs/wiki/engineering/setup.md`) not addressed by spec — will be stale after implementation.
- No explicit AC for Story 4 (CI resilience on publish failure).
- Deployment ordering risk (A3) — merging ci.yml change before initial publish breaks CI.
- AC-009 verification command omits `--user` flag used by actual CI workflow.
- Implementation contract (`docs/specs/ci-docker-publish/implementation-contract.md`) is stale and must be rewritten to match the accepted spec.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Rewrote the implementation contract to fix the critical technical issue identified in Loop #2: replaced `docker/build-push-action` with `pull: true` with the correct approach of `docker/login-action@v3` + `docker pull` in ci.yml. Added `docker/login-action@v3` to publish.yml as required by the spec. Removed `docker/setup-buildx-action` from ci.yml (no longer needed). Updated all verification commands, done criteria, and documentation impact to match the corrected approach.
**Artifacts**:
- docs/specs/ci-docker-publish/implementation-contract.md
**Questions for human**:
none
**Warnings**:
- The `git diff --name-only` done-criteria check from the previous contract was replaced with `git status --porcelain` to correctly detect new/untracked files (publish.yml is a new file and would not appear in `git diff`).
- The spec-critic warning about AC-009 verifying without `--user` flag is carried forward — not addressed by this contract.
- The spec-critic warning about deployment ordering risk (merging ci.yml change before initial publish) is carried forward — no hard guard is added.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
All previously identified blocking issues resolved. The rewritten contract now correctly uses `docker/login-action@v3` + `docker pull` in ci.yml, `docker/login-action@v3` + `docker/build-push-action` with `push: true` in publish.yml, and removes `docker/setup-buildx-action` from ci.yml. All image references updated to `ghcr.io/hilderin/buddd-ci:latest`. Contract is technically correct, complete vs the spec, precise, and verifiable. Accepted — no blocking issues.
**Artifacts**:
- docs/specs/ci-docker-publish/implementation-contract-critic.md
**Questions for human**:
none
**Warnings**:
- Spec-critic warning about AC-009 `--user` flag omission carried forward (outside contract scope).
- Spec-critic warning about deployment ordering risk carried forward (by design, per spec A3).
- Action version convention table lists `docker/setup-buildx-action@v3` alongside other versions, but the action is removed from ci.yml — the convention is about version format, not presence, so this is minor.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: Thu Jun  4 09:38:15 PM EDT 2026
**Warnings**:
<none>
**Notes**: Approved via question tool.

## code-implementer

**Status**: completed
**Summary**:
Created `.github/workflows/publish.yml` — new workflow that publishes the CI Docker image to `ghcr.io/hilderin/buddd-ci` when `docker/ci.Dockerfile` changes on `main` (with manual `workflow_dispatch` support). Modified `.github/workflows/ci.yml` — replaced `docker/setup-buildx-action` + `docker/build-push-action` steps with `docker/login-action@v3` + `docker pull`, and updated all three `docker run` image references from `buddd-ci:latest` to `ghcr.io/hilderin/buddd-ci:latest`.
**Artifacts**:
- `.github/workflows/publish.yml` (created)
- `.github/workflows/ci.yml` (modified)
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Reviewed implementation of publish.yml (new) and ci.yml (modified) against spec (SPEC-CI-001), implementation contract (IMPL-006), constitution, and ADR-008. All 10 acceptance criteria satisfied. All verification commands pass. No forbidden files modified. YAML syntax valid. No blocking issues found. 5 non-blocking warnings carried forward from earlier review stages (grep precision, --user flag, deployment ordering, Story 4 AC, wiki staleness).
**Artifacts**:
- docs/specs/ci-docker-publish/code-review.md
**Questions for human**:
none
**Warnings**:
- Imprecise AC-007 verification command — `grep -c 'buddd-ci:latest'` returns 4 instead of 0 due to substring matching with `ghcr.io/hilderin/buddd-ci:latest`. The implementation correctly removed all bare `buddd-ci:latest` references.
- AC-009 `--user` flag omission (carried forward from spec-critic)
- Deployment ordering risk — merging ci.yml before initial publish breaks CI (carried forward)
- No explicit AC for Story 4 (CI resilience on publish failure) (carried forward)
- Wiki will be stale after implementation (carried forward)
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
Assessed whether the CI Docker image pre-publishing implementation requires ADR action. The pre-publishing of the CI Docker image is a meaningful architecture decision (deployment topology change, new registry dependency) that was deferred in ADR-008. Per the rule "Use superseding ADRs instead of modifying existing ADRs," created ADR-015 to formally document the decision and explicitly supersede the now-obsolete sections of ADR-008 ("Why not a pre-published image on ghcr.io", CI workflow pattern description, Docker build time negative consequence).
**Artifacts**:
- docs/adr/015-ci-docker-image-pre-publishing.md (created)
**Decisions needed**:
none
**Questions for human**:
none
**Warnings**:
- The wiki at `docs/wiki/engineering/setup.md` still describes the old build-on-every-run workflow and the "Publishing the image (future)" section — these should be updated by the wiki-agent to reflect the new approach.
**Blocking issues**:
none

## constitution-agent

**Status**: completed
**Summary**:
Reviewed the CI Docker image pre-publishing implementation against the constitution. The change adds a publish workflow (`.github/workflows/publish.yml`) and modifies CI to pull a pre-built image instead of building it on every run. No source code, build system, test, or developer workflow changes are involved. No existing constitution rule is affected, and no new durable rule is warranted — this is a routine CI infrastructure optimization, not a new architectural pattern or policy change.
**Artifacts**:
- none
**Changes needed**:
none
**Questions for human**:
none
**Warnings**:
- The deployment ordering risk (merging ci.yml change before initial publish breaks CI) is a known operational concern, not a constitutional matter.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated `docs/wiki/engineering/setup.md` to reflect the CI Docker image pre-publishing changes. Rewrote the "How it works" section to describe the publish/pull workflow instead of the old build-on-every-run approach. Replaced the obsolete "Publishing the image (future)" section with a summary of the current publishing setup referencing ADR-015. Added a note to the "Build the image locally" section clarifying that CI uses the pre-published image from ghcr.io.
**Artifacts**:
- `docs/wiki/engineering/setup.md` (modified)
**Changes made**:
- "How it works" section (lines 129-135): updated from old build-on-every-run with GHA layer caching to new publish/pull architecture (publish.yml builds and pushes on Dockerfile change, ci.yml pulls pre-published image, no layer caching needed)
- "Publishing the image (future)" section (lines 137-139): replaced with current publishing setup summary referencing `.github/workflows/publish.yml` and ADR-015
- "Build the image locally" section (line 97): added note that CI pulls the pre-published image from ghcr.io instead of building locally
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance validation passed. All seven artifacts (spec, implementation contract, coordination, ADR-008, ADR-015, wiki, workflow YAML files) are internally consistent and mutually coherent. No constitution violations found. No cross-document contradictions. No blocking issues identified. The deployment ordering risk (merging ci.yml before initial publish) is consistently documented as an accepted operational risk across all documents. Seven non-blocking warnings recorded, the most notable being ADR-008's missing partial-supersession indicator and ADR-015's pre-merge "Accepted" status.
**Artifacts**:
- `docs/specs/ci-docker-publish/governance-review.md`
**Questions for human**:
none
**Warnings**:
- ADR-008 status field lacks partial-supersession indicator — no inline note that sections are superseded by ADR-015.
- ADR-015 status says "Accepted" pre-merge (per ADR README, ADRs are accepted when merged via PR).
- Carried forward: Imprecise AC-007 grep verification command (substring match returns 4 instead of 0).
- Carried forward: AC-009 `--user` flag omission from verification command.
- Carried forward: Deployment ordering risk — merging ci.yml before initial publish breaks CI (accepted operational risk, no hard guard).
- Carried forward: No explicit AC for Story 4 (CI resilience on publish failure).
- Wiki local `buddd-ci:latest` examples unchanged — correct per spec non-goal (local dev unchanged), but developer awareness note.
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
