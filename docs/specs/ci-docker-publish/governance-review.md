# Governance Review — CI Docker Image Pre-Publishing

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [ ] **No cross-document contradictions found.**
  All six documents (spec.md, implementation-contract.md, coordination.md, ADR-008, ADR-015, code) are internally consistent and agree with each other on:
  - Architecture: `publish.yml` builds/pushes → `ci.yml` pulls pre-published image.
  - Tags: `:latest` and `:<commit-sha>` on `ghcr.io/hilderin/buddd-ci`.
  - CI workflow changes: replace `docker/setup-buildx-action` + `docker/build-push-action` with `docker/login-action@v3` + `docker pull`.
  - Permissions: `contents: read` + `packages: write` for publish; default `GITHUB_TOKEN` for CI.
  - Fallback: hard failure on pull failure (no fallback build).
  - Wiki staleness identified and addressed by wiki-agent.

- [ ] **Spec open questions (Q1, Q2) resolved consistently across all documents.**
  Q1 (fallback on pull failure): all documents agree on "no fallback — hard fail."
  Q2 (trigger path expansion): all documents agree on strict `docker/ci.Dockerfile` trigger only.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

- [ ] **CONST-001 (architecture-boundaries.md):** Not implicated — no code in `src/` or `tests/` was modified. Only CI workflow YAML files changed. No violation.
- [ ] **CONST-002 (testing-policy.md):** Not implicated — YAML workflow configuration is not "testable code." The implementation contract correctly documents this reasoning. No violation.
- [ ] **CONST-003 (documentation-policy.md):** Rule is `TODO` — no enforceable constraint. No violation.
- [ ] **CONST-004 (security-policy.md):** Rule is `TODO` — no enforceable constraint. No violation.
- [ ] **Engineering principle "Governance documents must not contradict each other":** All governance documents (constitution, ADRs, spec, contract, wiki) are consistent. No contradictions found.
- [ ] **Engineering principle "Prefer explicit contracts over implicit assumptions":** The implementation contract is explicit about every change. No implicit assumptions. Pass.
- [ ] **Engineering principle "Prefer small scoped changes over broad rewrites":** Only 2 files touched (1 created, 1 modified). Pass.
- [ ] **Engineering principle "Prefer existing conventions over new patterns":** Uses pinned action versions, existing YAML style, existing `docker run` parameter template. Pass.
- [ ] **Engineering principle "Prefer testable requirements over vague intent":** All 10 ACs are testable/verifiable. Pass.

**No constitution violations.** All checks pass.

## ADR alignment

Required ADRs exist or are proposed:

- [ ] **ADR-015 exists and documents the decision.** Created by adr-agent, correctly captures the architecture, alternatives considered, and supersession of ADR-008 sections.
- [ ] **ADR-015 correctly supersedes relevant ADR-008 sections.** The supersession declaration (lines 79–87) explicitly lists three superseded sections: "Why not a pre-published image on ghcr.io" (now obsolete because pre-publication is implemented), "CI workflow" three-step pattern (build step moved to `publish.yml`), and "Docker build time" negative consequence (mitigated by this change).
- [ ] **ADR-008 core decision remains intact.** The fundamental decision (use a custom Docker image for CI, defined in `docker/ci.Dockerfile`) is unaffected. ADR-015 only supersedes the parts that are no longer current.
- [ ] **No new ADR required beyond ADR-015.** The change is well-scoped and fully documented.
- [ ] **ADR-008 status "Accepted" without visible partial-supersession note.** ADR-008 still states `## Status: Accepted` with no marker that sections are superseded by ADR-015. While ADR-015 documents the supersession, a reader at ADR-008 alone would not know parts are obsolete. This is a documentation completeness concern, not a contradiction (no two documents disagree).

## Wiki alignment

Wiki reflects current state and does not become law:

- [ ] **Wiki correctly reflects current state.** `docs/wiki/engineering/setup.md` has been updated by the wiki-agent:
  - "How it works" section (lines 129–136) now describes the publish/pull architecture instead of the old build-on-every-run approach.
  - "Publishing the image (future)" section (lines 137–139) replaced with current setup referencing `.github/workflows/publish.yml` and ADR-015.
  - "Build the image locally" section (line 97) includes note that CI pulls from ghcr.io.
  - No future-tense or speculative language remains.
- [ ] **Wiki does not become law.** All content is descriptive of the current operational state. No new rules or policies are introduced.
- [ ] **Cross-reference integrity.** Wiki references ADR-015 (exists), workflow files (exist), and Dockerfile instructions (correct).

## Warnings

Non-blocking concerns for awareness:

- **ADR-008 status field lacks partial-supersession indicator.** ADR-008 shows `## Status: Accepted` with no inline note that its "Why not a pre-published image on ghcr.io", "CI workflow", and "Docker build time negative" sections are superseded by ADR-015. While ADR-015 documents the supersession correctly, a reader viewing ADR-008 in isolation would not know. Consider adding a `Partially superseded by ADR-015` note to ADR-008's status or adding a `## Superseded by` section.

- **ADR-015 status says "Accepted" pre-merge.** Per the ADR README (`docs/adr/README.md`), "ADRs are accepted when merged via PR." Since this feature has not been merged, ADR-015's status should technically be `Proposed` until merge. This is consistent with the in-progress workflow convention but deviates from the stated ADR lifecycle policy.

- **Carried forward: Imprecise AC-007 grep verification.** `grep -c 'buddd-ci:latest' .github/workflows/ci.yml` returns 4 (not 0) because `ghcr.io/hilderin/buddd-ci:latest` contains `buddd-ci:latest` as a substring. The implementation is correct — no bare `buddd-ci:latest` references remain — but the verification command is imprecise. (Source: code-review.md)

- **Carried forward: AC-009 omits `--user` flag.** The verification command `docker run --rm ghcr.io/hilderin/buddd-ci:latest bash -c ...` does not include `--user "$(id -u):$(id -g)"` used by the actual workflow. Not required for version checking, but potentially misleading as a pattern. (Source: spec-critic → contract-critic → code-review)

- **Carried forward: Deployment ordering risk.** Merging `ci.yml` before the initial manual `workflow_dispatch` publish breaks CI. Documented in spec (Assumption A3, edge case row 1), contract, ADR-015 (negative consequence), and all review stages. Accepted risk — no hard guard added by design. (Source: all stages)

- **Carried forward: No explicit AC for Story 4 (CI resilience on publish failure).** The spec describes but does not explicitly verify the behavior where CI pulls the last successful `:latest` if publish fails. Adequately covered at the design level in edge cases. (Source: spec-critic → contract-critic → code-review)

- **Wiki carries local `buddd-ci:latest` examples unchanged.** The local development sections of `docs/wiki/engineering/setup.md` (lines 91–127) still use the `buddd-ci:latest` local tag in `docker run` examples. This is correct (local development was intentionally unchanged per spec Non-goal #6: "Local development unchanged"), but developers should be aware that CI uses `ghcr.io/hilderin/buddd-ci:latest` while local use retains the old tag.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **No changes required.** The governance document set is complete, consistent, and accurate. All changes introduced by this feature are properly documented:
  - ADR-015: new ADR for the pre-publishing decision.
  - Wiki: updated to reflect current state.
  - Coordination.md: documents the full workflow.
  - Constitution: no changes needed (routine CI infrastructure optimization, not a new architectural pattern or policy change).
