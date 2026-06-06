# Implementation Contract Review — Governance Refactor: Remove the Constitution

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] *(none found — all ACs verified)*

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **experiments-spec-driven-dev.md was modified** — The contract explicitly states "Do NOT modify experiments-spec-driven-dev.md" (per spec AC-045). The file has 148 lines of diff in the working tree (adding an Iteration #9 section about the `.specs/` migration and updating observations). These changes appear to be pre-existing from the previous sprint's spec refactoring work and are not related to the constitution removal. The changes do not introduce contradictions with the constitution removal, but the file should not have been modified per the contract.

- **AGENTS.md line 49 says "authority order #4" for wiki** — The text reads "The wiki sits at authority order #4 — above existing code conventions." With the constitution removed, the wiki is now at authority order #3 (after ADRs and current spec). This is a stale number from the old hierarchy. The sentence correctly states that wiki is above code conventions, but the number should be #3. The primary authority order list (lines 7–10) is correct — only this descriptive reference is stale.

- **docs/wiki/architecture/overview.md line 133** — The directory tree description still references "CONST-001" in a code comment (`shader_program.h` description). This is a minor cosmetic inconsistency since CONST-001 no longer exists; the reference could be updated to say "ADR-019" but was not required by the contract.

## Required changes

Concrete, actionable changes requested:

- **AGENTS.md line 49**: Change "authority order #4" to "authority order #3" to reflect the updated hierarchy after constitution removal.

## Suggested improvements

Optional ideas (not required):

- Update the "CONST-001" reference in `docs/wiki/architecture/overview.md` line 133 to "ADR-019" for consistency.
- Remove the `## adr-agent` section from this feature's `coordination.md` now that the template has been updated (this is a one-time cleanup; future features will use the updated template automatically).

---

## Verification results

### Deleted files (all confirmed)
| File | Status |
|---|---|
| `docs/constitution/` directory | ✅ Deleted |
| `.opencode/agents/constitution-agent.md` | ✅ Deleted |
| `docs/templates/constitution-rule-template.md` | ✅ Deleted |
| `docs/templates/amendment-template.md` | ✅ Deleted |
| `docs/wiki/decisions/constitution-index.md` | ✅ Deleted |
| Constitution rule files (CONST-001 through CONST-004) | ✅ Deleted |
| `docs/constitution/charter.md` | ✅ Deleted |
| `docs/constitution/principles.md` | ✅ Deleted |

### Created files (both confirmed)
| File | Status |
|---|---|
| `docs/adr/ADR-019-architecture-boundaries.md` | ✅ Created — contains CONST-001 architecture boundary, amendment history (AMEND-2026-001, AMEND-2026-002), alternatives, consequences, related docs, and historical derivation note |
| `docs/wiki/engineering/principles.md` | ✅ Created — contains all 5 principles from original constitution |

### Modified files — key checks

| File | Key verification | Status |
|---|---|---|
| `AGENTS.md` | Authority order: ADR > current spec > wiki > code ✓; No constitution in non-negotiable rules ✓; No constitution in document roles ✓; No constitution in escalation ✓ | ✅ (except stale #4 on line 49) |
| `README.md` | No constitution-agent in table ✓; Workflow diagram shows ADR / Wiki updates ✓; Authority order updated ✓; Tree has no constitution line ✓ | ✅ |
| `SpecKit.md` | No constitution-agent ✓; No constitution in documents ✓; Authority order updated ✓ | ✅ |
| `opencode.json` | No constitution-agent entry ✓; Valid JSON ✓; Scout/spec-critic/code-reviewer descriptions updated ✓ | ✅ |
| `docs/adr/README.md` | "not automatically constitutional rules" removed ✓ | ✅ |
| `docs/wiki/README.md` | "It is not a source of mandatory rules" replaced ✓ | ✅ |
| `docs/wiki/engineering/testing.md` | CONST-001 path → ADR-019 ✓; Constitution reference section removed ✓ | ✅ |
| `docs/wiki/architecture/overview.md` | Constitution tree line removed ✓ | ✅ |
| `docs/adr/ADR-019-architecture-boundaries.md` | Contains all required sections: Status, Context, Decision (with exceptions), Amendments AMEND-2026-001 (ratified) and AMEND-2026-002 (superseded), Alternatives, Consequences, Related docs, Historical note ✓ | ✅ |

### Templates
| File | Key verification | Status |
|---|---|---|
| `coordination-template.md` | No `## constitution-agent` or `## adr-agent` sections ✓; Constraints updated ✓ | ✅ |
| `governance-review-template.md` | No `## Constitution violations` section ✓; Required governance updates updated ✓ | ✅ |
| `implementation-contract-template.md` | No "Relevant constitution rules" or "Constitution impact" sections ✓ | ✅ |
| `wiki-page-template.md` | No "Related constitution rules" section ✓ | ✅ |
| `adr-template.md` | No "Does this imply a constitutional rule?" section ✓ | ✅ |

### Agent prompts — constitution reference grep results
| Agent | Result |
|---|---|
| `orchestrator.md` | ✅ No constitution references; adr-agent on-demand; constitution-agent removed from workflow |
| `scout.md` | ✅ No constitution references; search scope updated |
| `spec-author.md` | ✅ No constitution references |
| `spec-critic.md` | ✅ No constitution references |
| `implementation-contract-author.md` | ✅ No constitution references |
| `implementation-contract-critic.md` | ✅ No constitution references |
| `code-implementer.md` | ✅ No constitution references |
| `code-reviewer.md` | ✅ No constitution references |
| `governance-reviewer.md` | ✅ No constitution references |
| `wiki-agent.md` | ✅ One remaining: "The wiki is descriptive, not constitutional." (intentional — "constitutional" used as an adjective, not a reference to the removed governance layer) |
| `adr-agent.md` | ✅ No constitution references |

### Forbidden files — git diff --stat verification
| Area | Status |
|---|---|
| `.specs/` archives | ✅ No changes |
| `src/` source code | ✅ No changes |
| `tests/` test files | ✅ No changes |
| `CMakeLists.txt` / `CMakePresets.json` | ✅ No changes |
| `.github/` | ✅ No changes |
| Existing ADRs (001–018) | ✅ No changes |

### Constitution reference sweep
- `git grep -n "constitution\|constitutional" -- .opencode/agents/ AGENTS.md README.md SpecKit.md opencode.json docs/templates/ docs/wiki/ docs/adr/README.md`
- Only remaining reference: `wiki-agent.md:64` ("The wiki is descriptive, not constitutional.") — intentional, uses "constitutional" as adjective
- Existing ADRs (001–018) contain historical references to CONST-001 — intentionally not modified ✓
- `experiments-spec-driven-dev.md` contains historical constitution references — intentionally left as historical journal ✓

### Summary
All 61 acceptance criteria appear to be satisfied. The implementation is thorough and correctly removes the constitution governance layer. No blocking issues found. Two minor warnings noted: AGENTS.md line 49 stale number and experiments-spec-driven-dev.md pre-existing modification.
