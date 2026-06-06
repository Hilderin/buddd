# SPEC-2026-006 - Governance Refactor: Remove the Constitution

## Problem

The project currently maintains a **constitution** (`docs/constitution/`) as the highest-authority governance document with mandatory rules. In practice, the constitution has proven redundant with Architecture Decision Records (ADRs) and the wiki. Key problems:

1. **Redundant governance layers** — CONST-001 (Architecture Boundaries) duplicates what could be an ADR; CONST-002 (Testing Policy) is already enforced by agent prompts; CONST-003 and CONST-004 are empty TODOs.
2. **Unclear authority boundaries** — The constitution sits above ADRs and the wiki, but there is no clear criterion for what belongs in a constitution rule vs an ADR vs a wiki convention. This causes confusion for both humans and agents.
3. **Excessive workflow overhead** — The `constitution-agent` runs at the end of every feature workflow, and the `adr-agent` runs as a mandatory workflow step. Both add delay without proportional value.
4. **Maintenance burden** — Keeping constitution, ADRs, and wiki synchronized across every feature change is costly and error-prone.
5. **Template and reference sprawl** — Constitution-related templates (`constitution-rule-template.md`, `amendment-template.md`), wiki pages (`constitution-index.md`), and cross-references in every agent prompt and documentation file must be maintained, even though most constitution content is stale or placeholder.

The goal is to **remove the constitution entirely**, migrate its meaningful content to ADRs or the wiki, delete empty or redundant content, and streamline the governance workflow by making the ADR agent an on-demand tool rather than a mandatory workflow step.

## Goals

- Remove the `docs/constitution/` directory entirely.
- Migrate CONST-001 (Architecture Boundaries) content to a new ADR-019.
- Delete CONST-002 (redundant — already covered by agent prompts), CONST-003/004 (empty TODOs), Charter, and Principles (move principles to wiki).
- Delete the constitution-agent (`constitution-agent.md`) and its registration in `opencode.json`.
- Make the adr-agent an on-demand tool available to the orchestrator at any time, removed from the automatic workflow loop.
- Update the document authority order: `docs/adr/**` > current spec > `docs/wiki/**` > code conventions.
- Update all 11 agent prompts that reference the constitution to remove those references.
- Update all affected templates (coordination, governance-review, implementation-contract, wiki-page, ADR) to remove constitution sections.
- Delete `constitution-rule-template.md` and `amendment-template.md`.
- Update all root-level documentation files (`AGENTS.md`, `README.md`, `SpecKit.md`) to reflect the new governance structure.
- Update wiki pages that reference the constitution (`docs/wiki/decisions/constitution-index.md` → delete; `docs/wiki/README.md`, `docs/wiki/decisions/adr-index.md` → update).
- Do NOT modify `.specs/` archives (historical snapshots are read-only).
- Do NOT modify source code files.

## Non-goals

- No new features or functionality.
- No changes to the spec-driven development workflow structure other than removing constitution-agent and demoting adr-agent.
- No changes to individual ADR content (ADR-019 is a new ADR for CONST-001 migration, but existing ADRs are not modified).
- No changes to `.specs/` archives.

## Actors

| Actor | Description |
|---|---|
| **Orchestrator** | Coordinates the workflow; delegates to sub-agents; triggers adr-agent on-demand when needed. |
| **Spec Author** | Writes functional specs; self-validation checks must be updated to remove constitution references. |
| **Spec Critic** | Validates specs; must stop checking against `docs/constitution/**`. |
| **Scout** | Reconnaissance agent; must stop searching `docs/constitution/**`. |
| **Implementation Contract Author** | Reads constitution rules; contract sections must be updated to remove constitution impact. |
| **Implementation Contract Critic** | Validates contracts; must stop checking against `docs/constitution/**`. |
| **Code Implementer** | Implements contracts; must stop referencing constitution rules. |
| **Code Reviewer** | Reviews code against spec and contract; must stop referencing constitution. |
| **ADR Agent** | On-demand tool; no longer runs as automatic workflow step; removes "do not create constitutional rules" language. |
| **Governance Reviewer** | Cross-document validation; must remove constitution violations check. |
| **Wiki Agent** | Wiki maintenance; must stop referencing constitution. |
| **Human** | Initiates the refactor; provides explicit approval for governance changes. |

## User-visible behavior

- The orchestrator no longer invokes `constitution-agent` at any point in the workflow.
- The orchestrator may invoke `adr-agent` on demand (via `question` tool or direct delegation) but is not required to do so as a workflow step.
- The workflow diagram in the orchestrator prompt removes the constitution-agent step and makes adr-agent optional.
- All agent prompts, templates, and documentation files no longer reference the constitution.
- `docs/constitution/` directory is deleted.
- `docs/adr/ADR-019-multi-material-model.md` (or `ADR-019-architecture-boundaries.md`) is created from CONST-001 content.
- `docs/wiki/engineering/principles.md` is created from constitution principles.
- Two templates are deleted: `constitution-rule-template.md` and `amendment-template.md`.

## User stories

### Story 1 — Remove constitution infrastructure (P1)

As an **orchestrator**, I want the constitution directory, constitution-agent, and all constitution-related templates removed, so that the governance model is simplified to ADRs + wiki.

**Given** the project currently has a constitution with rules, agent, templates, and wiki index
**When** the governance refactor is complete
**Then** `docs/constitution/` is deleted
**And** `.opencode/agents/constitution-agent.md` is deleted
**And** `opencode.json` no longer has a `constitution-agent` entry
**And** `docs/templates/constitution-rule-template.md` is deleted
**And** `docs/templates/amendment-template.md` is deleted

### Story 2 — Migrate meaningful constitution content (P1)

As a **governance reviewer**, I want constitution content migrated to appropriate locations, so that no information is lost.

**Given** the constitution is being removed
**When** the migration is complete
**Then** CONST-001 (Architecture Boundaries) is migrated to a new ADR-019 in `docs/adr/ADR-019-architecture-boundaries.md` with condensed content and amendment history
**And** CONST-002 (Testing Policy) is deleted (already covered by agent prompts)
**And** CONST-003/004 (empty TODOs) are deleted
**And** Charter is deleted
**And** Principles are migrated to `docs/wiki/engineering/principles.md`
**And** `docs/wiki/decisions/constitution-index.md` is deleted

### Story 3 — Update agent prompts (P1)

As an **agent developer**, I want all agent prompts updated to remove constitution references, so that agents no longer search, validate, or reference constitution documents.

**Given** the constitution is being removed
**When** all agent prompts are updated
**Then** the following agents have constitution references removed:
- `orchestrator.md` — remove agent table entry, workflow step, hard rules about constitution
- `scout.md` — remove "Constitution" from search scope sources and output format
- `spec-author.md` — remove self-check against constitution rules
- `spec-critic.md` — remove constitution contradictions check
- `implementation-contract-author.md` — remove "Relevant constitution rules" and "Constitution impact" sections from process
- `implementation-contract-critic.md` — remove constitution contradictions check
- `code-implementer.md` — remove "Read relevant constitution rules" and constitution modification restriction
- `code-reviewer.md` — remove constitution from check-against list and review questions
- `governance-reviewer.md` — remove constitution violation checks
- `wiki-agent.md` — remove constitution from detect step and "do not contradict the constitution" rule
- `adr-agent.md` — remove "do not create constitutional rules" note

### Story 4 — Update templates (P1)

As a **template author**, I want all templates updated to remove constitution sections, so that new governance documents don't include obsolete constitution references.

**Given** the constitution is being removed
**When** templates are updated
**Then** `coordination-template.md` removes the `## constitution-agent` section and the `## adr-agent` section entirely (orchestrator adds `## adr-agent` back dynamically when invoking adr-agent on-demand)
**And** `governance-review-template.md` removes the "Constitution violations" section
**And** `implementation-contract-template.md` removes "Relevant constitution rules" and "Constitution impact" sections
**And** `wiki-page-template.md` removes "Related constitution rules" section
**And** `adr-template.md` removes "Does this imply a constitutional rule?" section

### Story 5 — Update documentation files (P2)

As a **project maintainer**, I want all root-level and wiki documentation updated to reflect the new governance model, so that documentation is consistent.

**Given** the constitution is being removed
**When** documentation updates are complete
**Then** `AGENTS.md` authority order is updated to: ADRs > current spec > wiki > code; non-negotiable rules remove constitution-related rules; document roles remove constitution role; escalation reasons remove constitution references
**And** `README.md` is updated: workflow diagram removes constitution-agent step, agent table removes constitution-agent, authority order removes constitution
**And** `SpecKit.md` is updated: agent table removes constitution-agent, authority order removes constitution, documents section removes constitution
**And** `docs/adr/README.md` removes "not automatically constitutional rules" language
**And** `docs/wiki/README.md` no longer contrasts with constitution
**And** `docs/wiki/decisions/adr-index.md` removes constitution-related references
**And** `docs/wiki/decisions/constitution-index.md` is deleted

### Story 6 — Make ADR agent on-demand (P1)

As an **orchestrator**, I want the ADR agent available on demand rather than as a mandatory workflow step, so that I can decide when ADR documentation is needed without slowing down every feature.

**Given** the current workflow has `adr-agent` as a mandatory step between code review and wiki update
**When** the governance refactor is complete
**Then** `orchestrator.md` workflow no longer includes adr-agent as a required step
**And** adr-agent is described as an on-demand tool the orchestrator can invoke at any time
**And** `opencode.json` retains the adr-agent entry (it is not deleted)
**And** `coordination-template.md` removes the `## adr-agent` section entirely (orchestrator adds it back dynamically when invoking adr-agent on-demand)

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `docs/constitution/` directory is deleted entirely | Confirm `docs/constitution/` does not exist |
| AC-002 | `.opencode/agents/constitution-agent.md` is deleted | Confirm file does not exist |
| AC-003 | `opencode.json` no longer has a `constitution-agent` entry | Confirm `constitution-agent` is absent from the `agent` object |
| AC-004 | `docs/templates/constitution-rule-template.md` is deleted | Confirm file does not exist |
| AC-005 | `docs/templates/amendment-template.md` is deleted | Confirm file does not exist |
| AC-006 | ADR-019 exists at `docs/adr/ADR-019-architecture-boundaries.md` with CONST-001 rule content condensed and including amendment history | Confirm file exists and contains architecture boundary rule and amendments |
| AC-007 | CONST-002, CONST-003, CONST-004 files are deleted | Confirm files do not exist under `docs/constitution/rules/` |
| AC-008 | Charter (`docs/constitution/charter.md`) is deleted | Confirm file does not exist |
| AC-009 | Principles (`docs/constitution/principles.md`) are migrated to `docs/wiki/engineering/principles.md` | Confirm file exists with same principles content |
| AC-010 | `docs/wiki/decisions/constitution-index.md` is deleted | Confirm file does not exist |
| AC-011 | Author order in `AGENTS.md` is updated to: `docs/adr/**` > `.specs/<current-sprint>/<feature>/spec.md` > `docs/wiki/**` > code conventions | Confirm AGENTS.md reflects new order |
| AC-012 | `AGENTS.md` non-negotiable rules remove "Do not violate constitution rules" and "Do not directly modify accepted constitution rules" | Confirm these lines are removed |
| AC-013 | `AGENTS.md` document roles remove constitution role | Confirm "Constitution: mandatory project rules" is removed |
| AC-014 | `AGENTS.md` escalation reasons remove "conflicts with the constitution" | Confirm removed |
| AC-015 | `orchestrator.md` removes constitution-agent from agent table and workflow step | Confirm no references to constitution-agent |
| AC-016 | `orchestrator.md` workflow diagram removes constitution-agent and demotes adr-agent to optional | Confirm workflow updated |
| AC-017 | `orchestrator.md` hard rules remove "Never create or update constitution yourself, ask `constitution-agent`" and "Never accept a constitutional change without explicit human approval" | Confirm removed |
| AC-018 | `scout.md` removes "Constitution" from search scope sources | Confirm "Constitution" not listed in search sources |
| AC-019 | `scout.md` output format removes "Constitution:" line | Confirm removed |
| AC-020 | `spec-author.md` self-validation removes "Does the spec contradict any accepted spec or constitution rule?" | Confirm removed |
| AC-021 | `spec-critic.md` removes "Contradictions with `docs/constitution/**`" from checks | Confirm removed |
| AC-022 | `implementation-contract-author.md` removes "Relevant constitution rules" and "Constitution impact" from contract sections | Confirm removed |
| AC-023 | `implementation-contract-author.md` self-validation removes "Does the contract contradict any accepted spec, ADR, or constitution rule?" | Confirm constitution part removed |
| AC-024 | `implementation-contract-critic.md` removes "Contradictions with `docs/constitution/**`" and "Missing constitution impact" from checks | Confirm removed |
| AC-025 | `code-implementer.md` removes "Read relevant constitution rules" from before-editing steps | Confirm removed |
| AC-026 | `code-implementer.md` "You must not" section removes "Modify `docs/constitution/**`" | Confirm removed |
| AC-027 | `code-reviewer.md` removes constitution from "Check against" list | Confirm removed |
| AC-028 | `code-reviewer.md` review questions remove "Did it violate the constitution?" and "Did it require an ADR or constitution update?" | Confirm removed |
| AC-029 | `governance-reviewer.md` removes "Constitution is not violated" and "Required constitution updates exist or are proposed" from checks | Confirm removed |
| AC-030 | `governance-reviewer.md` rules remove "Be strict about constitution violations — they are always blocking" | Confirm removed |
| AC-031 | `wiki-agent.md` removes constitution from detect step | Confirm removed |
| AC-032 | `wiki-agent.md` removes "Do not contradict the constitution" rule | Confirm removed |
| AC-033 | `adr-agent.md` removes "they do not automatically create constitutional rules" | Confirm removed |
| AC-034 | `coordination-template.md` removes `## constitution-agent` section entirely | Confirm section removed |
| AC-035 | `coordination-template.md` removes the `## adr-agent` section entirely and removes related constraints note requirements for `## adr-agent` fields | Confirm `## adr-agent` section removed and constraints note updated |
| AC-036 | `governance-review-template.md` removes "Constitution violations" section | Confirm section removed |
| AC-037 | `implementation-contract-template.md` removes "Relevant constitution rules" and "Constitution impact" sections | Confirm sections removed |
| AC-038 | `wiki-page-template.md` removes "Related constitution rules" section | Confirm section removed |
| AC-039 | `adr-template.md` removes "Does this imply a constitutional rule?" section | Confirm section removed |
| AC-040 | `README.md` workflow diagram removes constitution-agent step | Confirm diagram updated |
| AC-041 | `README.md` agent table removes constitution-agent | Confirm constitution-agent not listed |
| AC-042 | `README.md` authority order removes constitution | Confirm authority order updated |
| AC-043 | `SpecKit.md` agent table removes constitution-agent | Confirm not listed |
| AC-044 | `SpecKit.md` authority order removes constitution | Confirm updated |
| AC-045 | `experiments-spec-driven-dev.md` is NOT modified (left as historical journal) | Confirm `git diff` shows no changes to this file |
| AC-046 | `docs/adr/README.md` removes "They are not automatically constitutional rules" | Confirm removed |
| AC-047 | `docs/wiki/README.md` removes "It is not a source of mandatory rules" (this implicitly contrasted with the now-deleted constitution) and any other constitution-related language | Confirm updated |
| AC-048 | `docs/wiki/decisions/adr-index.md` removes constitution-related references | Confirm no constitution references |
| AC-049 | No `.specs/` archive files are modified | Confirm git diff shows no changes in `.specs/` archives |
| AC-050 | No source code files are modified | Confirm git diff shows no changes in `src/`, `tests/`, `CMakeLists.txt`, etc. |
| AC-051 | `AGENTS.md` non-negotiable rule "Do not update the wiki in a way that contradicts the constitution or accepted ADRs" removes "the constitution or" | Confirm line reads "Do not update the wiki in a way that contradicts accepted ADRs" |
| AC-052 | `docs/wiki/architecture/overview.md` directory tree removes `├── constitution/  # Mandatory project rules` | Confirm line is removed from the tree diagram |
| AC-053 | `docs/wiki/engineering/testing.md` updates `AMEND-2026-001` reference from `CONST-001-architecture-boundaries.md` to `ADR-019-architecture-boundaries.md` | Confirm reference updated |
| AC-054 | `docs/wiki/engineering/testing.md` removes or replaces the `## Constitution reference` section that links to CONST-002 | Confirm section removed or replaced with appropriate ADR/wiki reference |
| AC-055 | `README.md` project structure directory tree removes `├── constitution/    # Mandatory project rules` | Confirm line is removed from the tree diagram |
| AC-056 | `SpecKit.md` workflow diagram removes `constitution /` from `→ ADR / constitution / wiki updates` | Confirm diagram shows `→ ADR / wiki updates` |
| AC-057 | `SpecKit.md` documents section removes `- constitution/` — Mandatory project rules | Confirm section no longer lists constitution |
| AC-058 | `SpecKit.md` installation adaptation section removes `- docs/constitution/**` | Confirm section no longer references constitution |
| AC-059 | `opencode.json` spec-critic description removes "and constitutional conflicts" | Confirm description ends with "scope" |
| AC-060 | `opencode.json` scout description removes "constitution," from search scope | Confirm "constitution," not in scout description |
| AC-061 | `opencode.json` code-reviewer description removes "and constitution" from review scope | Confirm review scope no longer includes constitution |

## E2E Verification

- **Method:** Manual file inspection and `git diff --stat` review. After all changes:
  1. Run `git diff --stat` to verify only expected files are modified (agent prompts, templates, docs, etc.) and no source code or archive files are touched.
  2. Run `ls docs/constitution/` to confirm directory deletion.
  3. Run `ls .opencode/agents/constitution-agent.md` to confirm deletion (should fail).
   4. Search for "constitution" across `docs/`, `.opencode/agents/`, `AGENTS.md`, `README.md`, `SpecKit.md` — all results should be expected (new ADR-019 may reference "constitution" as historical context; `experiments-spec-driven-dev.md` is excluded as it is intentionally left as a historical journal).
  5. Run `git grep -in "constitution" -- docs/templates/` — should return zero results.
  6. Verify ADR-019 exists and contains CONST-001 content.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All 61 acceptance criteria pass verification |
| SC-002 | Zero unexpected references to "constitution" remain in agent prompts, templates, and root docs (excluding experiments doc historical context and ADR-019 migration note) |
| SC-003 | `docs/constitution/` is absent from the filesystem |
| SC-004 | `opencode.json` parses as valid JSON with no constitution-agent entry |
| SC-005 | git diff shows zero changes to `.specs/` archives and `src/` or `tests/` source code |

## Edge cases

| Edge case | Handling |
|---|---|
| ADR-019 already exists (ADR-018 is the highest current ADR number) | The new ADR is ADR-019 since ADR-017 and ADR-018 exist. If ADR-019 is taken, use the next available number. |
| Cross-references to constitution in old spec archives | Not modified — `.specs/` archives are historical snapshots and are read-only. |
| References to "constitution" in experiments document that describe historical context | Kept as-is — the experiments doc is a personal log and historical accuracy is preserved. |
| Agent prompts that reference constitution in example orchestrator delegation patterns | All references must be updated, including example delegation text. |
| `opencode.json` constitution-agent removal causes JSON validity issues | Must ensure valid JSON after removal (remove trailing comma if needed). |
| Coordination.md template constraints order (adr-agent after constitution-agent) | After removing constitution-agent section, the constraints note must be updated to reflect the new expected ordering. |

## Error cases

| Error case | Handling |
|---|---|
| `opencode.json` is invalid after removing constitution-agent | Fix JSON syntax (trailing comma, missing comma). Must remain valid JSON. |
| Some agent prompt has a constitution reference not identified in the spec | Treat as a bug — implementation must find and fix ALL constitution references in the target files. |
| ADR-019 number conflicts with an existing or in-progress ADR | Use the next available ADR number. |
| Deleting \`docs/constitution/\` breaks git or CI | Ensure the deletion is committed properly; update CI configuration if it references the constitution (this overrides the \`.github/\` out-of-scope exclusion for this necessary side effect). |

## Permissions and security

- No authentication or authorization changes.
- The governance structure change is administrative; no user-facing security impact.
- The implementer must have write access to `.opencode/agents/`, `docs/`, `opencode.json`, `AGENTS.md`, `README.md`, `SpecKit.md`, `experiments-spec-driven-dev.md`.

## Observability

- **Validation:** All AC items can be verified by file inspection (ls, grep, git diff).
- **Audit trail:** The git diff and commit history provide the audit trail for what was deleted and modified.
- **No logging or metrics changes** — this is a documentation and configuration refactor with no runtime impact.

## Out of scope

- Modifying `.specs/` archive files.
- Modifying source code (anything under `src/`, `tests/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/`).
- Adding new ADRs beyond ADR-019 for CONST-001.
- Changing the content of ADR-017 (multi-material model) or any existing ADR other than ADR-019 creation.
- Changing the behavior or implementation of any OpenAI agent beyond removing constitution references from their prompts.
- Deleting the ADR agent entirely (only removing it from the mandatory workflow loop).
- Changing `opencode.json` agent descriptions beyond the scope of removing constitution references.
- Adding new templates or documentation files beyond the migrated principles page.
- Modifying `experiments-spec-driven-dev.md` (left as-is; constitution references may remain for historical accuracy).

## Assumptions

| Assumption | Rationale |
|---|---|
| ADR-019 is the next available ADR number after ADR-018 and ADR-017. | Files in `docs/adr/` show 001-018 and 017 (multi-material), making 019 available. |
| The experiments document (`experiments-spec-driven-dev.md`) may retain historical constitution references for context. | It is a personal experiment log, not a governance document. Historical accuracy is valued over strict consistency. |
| `opencode.json` agent order does not matter for functionality. | Reordering or removing an agent entry only affects the agent registry, not execution flow. |
| The orchestrator will use the `question` tool to ask the human before invoking adr-agent on-demand. | This matches the existing pattern for human-in-the-loop decisions. |
| All agent prompts are plain markdown files with no external dependencies on the constitution beyond text references. | No code logic depends on constitution file paths — only text instructions mention them. |

## Open questions

*(All previously identified questions have been resolved by human decision.)*
