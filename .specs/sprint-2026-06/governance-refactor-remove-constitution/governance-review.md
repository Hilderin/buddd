# Governance Review — Governance Refactor: Remove the Constitution

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec matches human intent** — SPEC-2026-006 correctly captures the human goal: remove constitution, migrate CONST-001 to ADR-019, move principles to wiki, delete redundant content, make adr-agent on-demand, update all agent prompts/templates/docs. All 61 ACs are verified.
- [x] **Contract matches accepted spec** — IMPL-2026-006 covers all 61 ACs with explicit phases (create → modify → delete → verify). The contract survived two critic cycles with all 6 blocking issues resolved.
- [x] **Code (documentation changes) matches accepted contract** — All 38 files handled as specified: 10 deleted, 2 created, 26 modified. All verified by code-review (full table at SPEC_DIR/code-review.md).
- [x] **No source code or archive files modified** — git diff --stat confirms zero changes to `src/`, `tests/`, `.specs/` archives, `.github/`, existing ADRs (001-018), or `CMakeLists.txt`. The only allowed exception is `experiments-spec-driven-dev.md` (pre-existing changes from previous sprint, unrelated to this work).
- [x] **AGENTS.md authority order** — Correct: ADRs > current spec > Wiki > Code. Line 49 now correctly says "authority order #3" (was #4 in the old hierarchy — fixed by implementer).
- [x] **SpecKit.md authority order differs from canonical order** — SpecKit.md has `ADRs > Wiki > Specs > Code` vs the canonical `ADRs > current spec > Wiki > Code`. This is acceptable since SpecKit.md is a starter kit for other projects, but noted for awareness.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-019-architecture-boundaries.md exists** — Created at `docs/adr/ADR-019-architecture-boundaries.md` with: Status (Accepted), Context, Decision (architecture boundary with exceptions), Alternatives considered, Consequences, Related documents (002, 003, 004, 012, 014).
- [x] **Amendments migrated correctly** — AMEND-2026-001 (SDL3 Test File Exception) included with full ratification details. AMEND-2026-002 (superseded) included with note about being superseded by ADR-003.
- [x] **Historical note present** — Document ends with: "Derived from the now-removed CONST-001 (Architecture Boundaries) of the project constitution, ratified amendments AMEND-2026-001 and AMEND-2026-002."
- [x] **No existing ADRs modified** — ADR-001 through ADR-018 are untouched (verified by git diff).
- [x] **ADR-019 does not include constitutional rule section** — The "Does this imply a constitutional rule?" section is absent, as required.
- [x] **Related documents reference ADRs by number** — ADR-002, 003, 004, 012, 014 listed as related documents.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/engineering/principles.md` created** — Contains the 5 engineering principles migrated from the constitution.
- [x] **`docs/wiki/decisions/constitution-index.md` deleted** — Confirmed absent.
- [x] **`docs/wiki/README.md` updated** — "It is not a source of mandatory rules" replaced with "The wiki describes the current operational understanding of the system." No constitution contrasts remain.
- [x] **`docs/wiki/engineering/testing.md` updated** — Line 75 references ADR-019 instead of CONST-001. The `## Constitution reference` section (old lines 223-226) replaced by a `## Reference` section with appropriate spec/contract links.
- [x] **`docs/wiki/architecture/overview.md` updated** — Directory tree no longer lists `├── constitution/`. Line 133 "CONST-001" reference updated to "ADR-019" by wiki-agent.
- [x] **`docs/wiki/architecture/module-map.md` updated** — Two remaining CONST-001 references (shader_program.h line 177, IMPL-006 line 344) updated to ADR-019 by wiki-agent.
- [x] **`docs/wiki/domain/glossary.md` updated** — CONST-001 reference in ShaderProgram term updated to ADR-019 by wiki-agent.
- [x] **No remaining constitution references in wiki** — `git grep -n "constitution\|CONST-001" -- docs/wiki/` returns zero results.

## Warnings

Non-blocking concerns for awareness:

- **SpecKit.md authority order differs from canonical order**: SpecKit.md has `ADRs > Wiki > Specs > Code` while the canonical project order is `ADRs > current spec > Wiki > Code`. SpecKit.md is a starter kit for other projects, so this difference is acceptable, but the two orders are not identical.
- **experiments-spec-driven-dev.md has pre-existing modifications**: 103 lines of diff from adding Iteration #9 (specs refactoring). These changes are from a previous sprint and are unrelated to the constitution removal. The contract explicitly allows this file to be left as-is.
- **coordination.md still has `## adr-agent` constraint**: Line 184 requires the adr-agent section to include `**Decisions needed**`. This is correct for this specific coordination.md (adr-agent was invoked on-demand), but the template has removed this section. Future features using the updated template will not have this constraint. This is a one-time transitional state.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None required. All ADR and wiki updates have been applied correctly. The constitution has been fully removed, ADR-019 is created, and all wiki pages reflect the new governance structure.
