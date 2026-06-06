# IMPL-2026-006 - Governance Refactor: Remove the Constitution

## Source spec

`.specs/sprint-2026-06/governance-refactor-remove-constitution/spec.md`

## Goal

Remove the entire constitution governance layer (`docs/constitution/`, constitution-agent, constitution templates, and all cross-references) from the project. Migrate meaningful content (CONST-001 architecture boundaries → ADR-019; principles → wiki). Delete redundant or empty content (CONST-002, CONST-003, CONST-004, charter). Make the ADR agent a human-invoked on-demand tool rather than a mandatory workflow step. Update all 11 agent prompts, 5 templates, root-level documentation (`AGENTS.md`, `README.md`, `SpecKit.md`), `opencode.json`, and affected wiki pages to remove constitution references. Update the document authority order to `docs/adr/**` > current spec > `docs/wiki/**` > code conventions.

## Non-goals

- Do NOT modify any source code files (anything under `src/`, `tests/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/`).
- Do NOT modify `.specs/` archive files (historical snapshots remain read-only).
- Do NOT modify existing ADR files (001–018) — they may reference CONST-001 by file path; those historical references remain as-is.
- Do NOT modify `experiments-spec-driven-dev.md` — it is intentionally left as a historical journal with constitution references.
- Do NOT introduce new features, dependencies, or functionality.
- Do NOT delete the ADR agent entirely — only remove it from the mandatory workflow loop.
- Do NOT add new templates or documentation files beyond the migrated principles page.

## Relevant constitution rules

None. The constitution is being removed by this implementation. No existing constitution rules constrain this work.

## Relevant ADRs

- ADR-001 through ADR-018: No changes to these files. Some ADRs contain historical references to `docs/constitution/rules/CONST-001-architecture-boundaries.md` — these are left as-is for historical accuracy.
- A new ADR-019 will be created to replace CONST-001 with condensed content in ADR format.

## Files to inspect

Before making changes, the implementer MUST read the current state of the following files to understand existing content and structure:

- `docs/constitution/charter.md` — content to be deleted
- `docs/constitution/principles.md` — content to be migrated to wiki
- `docs/constitution/rules/CONST-001-architecture-boundaries.md` — content to be migrated to ADR-019
- `docs/constitution/rules/CONST-002-testing-policy.md` — content to be deleted
- `docs/constitution/rules/CONST-003-documentation-policy.md` — content to be deleted
- `docs/constitution/rules/CONST-004-security-policy.md` — content to be deleted
- `docs/templates/coordination-template.md` — sections to remove (constitution-agent, adr-agent)
- `docs/templates/governance-review-template.md` — section to remove (Constitution violations)
- `docs/templates/implementation-contract-template.md` — sections to remove (Relevant constitution rules, Constitution impact)
- `docs/templates/wiki-page-template.md` — section to remove (Related constitution rules)
- `docs/templates/adr-template.md` — section to remove (Does this imply a constitutional rule?)
- `.opencode/agents/orchestrator.md` — multiple constitution references to remove
- `.opencode/agents/scout.md` — constitution from description, mission, output format
- `.opencode/agents/spec-author.md` — self-validation rule 7
- `.opencode/agents/spec-critic.md` — check for constitution contradictions
- `.opencode/agents/implementation-contract-author.md` — contract sections table, self-validation rule 7
- `.opencode/agents/implementation-contract-critic.md` — check for constitution impact, contradictions
- `.opencode/agents/code-implementer.md` — before-editing steps, forbidden section
- `.opencode/agents/code-reviewer.md` — check against list, review questions
- `.opencode/agents/governance-reviewer.md` — check for, review process, rules
- `.opencode/agents/wiki-agent.md` — detect step, rules
- `.opencode/agents/adr-agent.md` — rules line 42
- `opencode.json` — constitution-agent entry, scout/spec-critic/code-reviewer descriptions
- `AGENTS.md` — authority order, non-negotiable rules, document roles, escalation
- `README.md` — agent table, authority order, project structure tree
- `SpecKit.md` — agent table, workflow diagram, documents section, installation, authority order
- `docs/adr/README.md` — line 3
- `docs/wiki/README.md` — current content contrasting with constitution
- `docs/wiki/decisions/adr-index.md` — constitution-related references
- `docs/wiki/engineering/testing.md` — lines 75 (AMEND-2026-001 ref), 223-226 (CONST-002 ref)
- `docs/wiki/architecture/overview.md` — directory tree line 33
- `docs/templates/adr-template.md` — current template structure for ADR-019 creation

## Files allowed to change

### Files to DELETE:
1. `docs/constitution/charter.md`
2. `docs/constitution/principles.md`
3. `docs/constitution/rules/CONST-001-architecture-boundaries.md`
4. `docs/constitution/rules/CONST-002-testing-policy.md`
5. `docs/constitution/rules/CONST-003-documentation-policy.md`
6. `docs/constitution/rules/CONST-004-security-policy.md`
7. `docs/wiki/decisions/constitution-index.md`
8. `docs/templates/constitution-rule-template.md`
9. `docs/templates/amendment-template.md`
10. `.opencode/agents/constitution-agent.md`

### Files to CREATE:
11. `docs/adr/ADR-019-architecture-boundaries.md`
12. `docs/wiki/engineering/principles.md`

### Files to MODIFY:
13. `docs/templates/coordination-template.md`
14. `docs/templates/governance-review-template.md`
15. `docs/templates/implementation-contract-template.md`
16. `docs/templates/wiki-page-template.md`
17. `docs/templates/adr-template.md`
18. `.opencode/agents/orchestrator.md`
19. `.opencode/agents/scout.md`
20. `.opencode/agents/spec-critic.md`
21. `.opencode/agents/spec-author.md`
22. `.opencode/agents/implementation-contract-author.md`
23. `.opencode/agents/implementation-contract-critic.md`
24. `.opencode/agents/code-implementer.md`
25. `.opencode/agents/code-reviewer.md`
26. `.opencode/agents/governance-reviewer.md`
27. `.opencode/agents/wiki-agent.md`
28. `.opencode/agents/adr-agent.md`
29. `opencode.json`
30. `AGENTS.md`
31. `README.md`
32. `SpecKit.md`
33. `docs/adr/README.md`
34. `docs/wiki/README.md`
35. `docs/wiki/decisions/adr-index.md`
36. `docs/wiki/engineering/testing.md`
37. `docs/wiki/architecture/overview.md`

## Files forbidden to change

- Any `.specs/` archive files (except `coordination.md` in the current feature directory)
- Any files under `src/`, `tests/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/`
- `experiments-spec-driven-dev.md`
- `docs/adr/` files other than `README.md` and the newly created `ADR-019-architecture-boundaries.md` (existing ADR-001 through ADR-018 must NOT be modified)
- Any `.opencode/agents/` file not explicitly listed in "Files allowed to change"
- Any `docs/wiki/` file not explicitly listed in "Files allowed to change"

## Existing conventions to follow

- **ADR file naming**: Use the `ADR-NNN-title-with-dashes.md` convention (e.g., `ADR-019-architecture-boundaries.md`) as used by ADR-017.
- **ADR template**: Follow `docs/templates/adr-template.md` structure (after removing the constitutional rule section) — sections: Status, Context, Decision, Alternatives considered, Consequences, Related documents.
- **Wiki page format**: Follow existing wiki page conventions — markdown files with clear headings and tables where appropriate.
- **Agent prompt format**: Each agent prompt starts with a YAML frontmatter block (`---`), followed by `# Agent Name`, then instructions. Use `edit` for targeted changes rather than rewrites.
- **opencode.json**: Valid JSON. When removing the constitution-agent entry, ensure no trailing comma creates invalid JSON.
- **Coordination.md template**: Sections follow the exact heading names and field names as defined. Sub-agent sections appear in the exact order listed.
- **AGENTS.md**: Uses markdown with bullet lists and "##" headings.
- **README.md and SpecKit.md**: Use markdown tables for agent lists and code-fenced directory trees.

## Required implementation behavior

### Phase 1 — Create new files (before deleting old content)

#### 1.1 Create ADR-019-architecture-boundaries.md
- Read `docs/constitution/rules/CONST-001-architecture-boundaries.md` for source content.
- Read `docs/templates/adr-template.md` (after its constitution section has been removed) for the ADR structure.
- Create `docs/adr/ADR-019-architecture-boundaries.md` containing:
  - **Status**: Accepted
  - **Context**: Explain the architecture boundary need (code outside `src/engine/` must not include platform/graphics/windowing headers).
  - **Decision**: The architecture boundary rule with exceptions (the condensed text from CONST-001).
  - **Alternatives considered**: (brief) Not having a boundary; compile-time enforcement.
  - **Consequences**: Positive (platform independence, testability); Negative (extra abstraction layer).
  - **Related documents**: Reference ADR-003 (poll_events), ADR-002, ADR-004, ADR-012, ADR-014 (which reference this rule by its old CONST-001 path).
  - Include the content from amendment AMEND-2026-001 (SDL3 Test File Exception, ratified) as a subsection, preserving full rationale and ratification details.
  - Include a reference/historical note about AMEND-2026-002 (superseded/superseded by ADR-003) with explanation that it was superseded by design.
  - Do NOT include the "Does this imply a constitutional rule?" section (the template has already been modified to remove it).
  - End with a historical note: "Derived from the now-removed CONST-001 (Architecture Boundaries) of the project constitution, ratified amendments AMEND-2026-001 and AMEND-2026-002."

#### 1.2 Create docs/wiki/engineering/principles.md
- Read `docs/constitution/principles.md` for source content (5 principles).
- Create `docs/wiki/engineering/principles.md` with the full principles content.

### Phase 2 — Modify templates

#### 2.1 coordination-template.md
- Remove the entire `## constitution-agent` section (lines 120-134 inclusive).
- Remove the entire `## adr-agent` section (lines 104-119 inclusive, i.e., from the `## adr-agent` heading to the next `##` heading).
- Remove the constraint on line 174 (`- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (...)`) entirely, since those sections no longer exist. The wiki-agent constraint on the following line already covers the remaining section.
- Remove the explicit mention of `## adr-agent` and `## constitution-agent` from the constraints.
- The remaining sections in order should be: orchestrator, spec-author, spec-critic, implementation-contract-author, implementation-contract-critic, Human Validation, code-implementer, code-reviewer, wiki-agent, governance-reviewer.

#### 2.2 governance-review-template.md
- Remove the entire `## Constitution violations` section (lines 12-16 inclusive).
- Update `## Required governance updates` description (line 36-40): Change "Constitution, ADRs, wiki" to "ADRs, wiki".

#### 2.3 implementation-contract-template.md
- Remove `## Relevant constitution rules` (line 9) entirely.
- Remove `## Constitution impact` (line 48) entirely. Re-number subsequent sections or leave blank lines as appropriate.

#### 2.4 wiki-page-template.md
- Remove the `## Related constitution rules` line (line 13) entirely.

#### 2.5 adr-template.md
- Remove the `## Does this imply a constitutional rule?` line plus its description line (lines 22-23: `## Does this imply a constitutional rule?` and `No / Maybe / Yes`).

### Phase 3 — Modify agent prompts

#### 3.1 orchestrator.md
- Agent table (line 38): Change scout description from "Searches code, wiki, ADRs, constitution, specs, and disk" to "Searches code, wiki, ADRs, specs, and disk".
- Agent table (line 44): Change code-reviewer description from "Reviews implementation against accepted spec, contract, tests, and constitution" to "Reviews implementation against accepted spec, contract, and tests".
- Agent table (lines 46-47): Remove the entire `constitution-agent` row.
- Responsibilities (line 59): Change "Keep the workflow aligned with the constitution." to "Keep the workflow aligned with the project's governance documents."
- Scout usage example (line 91): Change "ADR/wiki/constitution constraints" to "ADR/wiki constraints".
- Workflow diagram (lines 236-237): Replace `adr-agent → updates coordination.md` and `constitution-agent (parallel) → updates coordination.md` with just `adr-agent (on-demand, if needed) → updates coordination.md`.
- Workflow details step 9 (lines 435-444): Rewrite to:
  ```
  ### 9. Governance update
  
  If the orchestrator decides an ADR is needed, invoke `adr-agent` on-demand by delegating to it.
  The `adr-agent` is an on-demand tool, not a mandatory workflow step.
  
  After `adr-agent` reports completion:
  - Read coordination.md `## adr-agent` section.
  - If **Questions for human** is non-empty, ask human using the `question` tool and record answer.
  - Check **Blocking issues**: if present, resolve.
  - Update `## Orchestrator` → **Current step**.
  ```
- Done section (line 480): Change "ADR/wiki/constitution updates" to "ADR/wiki updates".
- Hard rules (line 496): Remove "Never accept a constitutional change without explicit human approval".
- Hard rules (line 501): Remove "Never create or update constitution yourself, ask `constitution-agent`".
- Re-number remaining hard rules sequentially.

#### 3.2 scout.md
- Description (line 2): Remove "constitution," so it reads "Searches code, wiki, ADRs, specs, and disk".
- Mission (line 59): Remove item "4. **Constitution** — What project rules constrain the work?" and re-number items 5-7 to 4-6.
- Output format (line 228): Remove the entire "- Constitution: ..." line.
- Lines 143, 153, 169: Remove any mention of "constitution" in examples or search strategies (e.g., line 143 "constitution rules", line 153 "constitution section", line 169 "constitution rules").
- Line 213: Change "When code, wiki, specs, ADRs, or constitution disagree, flag the contradiction." to "When code, specs, ADRs, or wiki disagree, flag the contradiction." (removes duplicate "wiki" and constitution reference).
- Line 273: Change "- relevant specs, ADRs, wiki, and constitution rules" to "- relevant specs, ADRs, and wiki rules".

#### 3.3 spec-critic.md
- Check for (line 38): Remove "- Contradictions with `docs/constitution/**`".

#### 3.4 spec-author.md
- Self-validation rule 7 (line 90): Change "Does the spec contradict any accepted spec or constitution rule?" to "Does the spec contradict any accepted spec?".

#### 3.5 implementation-contract-author.md
- Contract sections table (line 50): Remove the row "| `Relevant constitution rules` | Constitution rules that directly constrain implementation (cite by rule ID). |"
- Contract sections table (line 64): Remove the row "| `Constitution impact` | Whether this implementation warrants a constitution amendment. |"
- Self-validation rule 7 (line 92): Change "Does the contract contradict any accepted spec, ADR, or constitution rule?" to "Does the contract contradict any accepted spec or ADR?".

#### 3.6 implementation-contract-critic.md
- Check for (line 41): Remove "- Missing constitution impact".
- Check for (line 43): Remove "- Contradictions with `docs/constitution/**`".

#### 3.7 code-implementer.md
- Before editing (line 27): Remove "- Read relevant constitution rules."
- You must not section (line 135): Remove "- Modify `docs/constitution/**`."

#### 3.8 code-reviewer.md
- Check against (line 34): Remove "- Constitution rules".
- Review questions (line 47): Remove "- Did it violate the constitution?".
- Review questions (line 48): Remove "- Did it require an ADR or constitution update?" and replace with "- Did it require an ADR?".

#### 3.9 governance-reviewer.md
- Check for (line 35): Remove "- Constitution is not violated.".
- Check for (line 37): Remove "- Required constitution updates exist or are proposed.".
- Review process (line 49): Remove "7. Read the constitution at `docs/constitution/**`." and re-number steps 8-11 to 7-10.
- Rules (line 58): Remove "- Be strict about constitution violations — they are always blocking."

#### 3.10 wiki-agent.md
- Line 21: Change "1. Discover what has changed (code, specs, ADRs, constitution, domain concepts)." to "1. Discover what has changed (code, specs, ADRs, domain concepts)."
- Detect (line 36): Remove "- Check the constitution in `docs/constitution/` for any fundamental rule changes.".
- Rules (line 66): Remove "- Do not contradict the constitution.".
- Rules (line 69): Change "- Reference source documents (ADRs, specs, constitution) when updating wiki content." to "- Reference source documents (ADRs, specs) when updating wiki content."

#### 3.11 adr-agent.md
- Rules (line 42): Remove "- ADRs explain decisions; they do not automatically create constitutional rules."

### Phase 4 — Modify opencode.json
- Remove the entire `constitution-agent` entry (lines 52-57 inclusive, plus any trailing comma from the previous entry).
- Ensure valid JSON after removal (remove trailing comma if needed; add comma after previous entry if needed).
- Update scout description (line 35): Change `"Searches code, wiki, ADRs, constitution, specs, and disk"` to `"Searches code, wiki, ADRs, specs, and disk"`.
- Update spec-critic description (line 17): Change `"Critiques and validates specs for ambiguity, testability, scope, and constitutional conflicts."` to `"Critiques and validates specs for ambiguity, testability, and scope."`.
- Update code-reviewer description (line 47): Change `"Reviews implementation against accepted spec, contract, tests, and constitution."` to `"Reviews implementation against accepted spec, contract, and tests."`.

### Phase 5 — Modify root documentation files

#### 5.1 AGENTS.md
- Authority order (lines 7-11): Replace with:
  ```
  1. `docs/adr/**`
  2. `.specs/<current-sprint>/<feature>/spec.md` (the current active spec)
  3. `docs/wiki/**`
  4. Existing code conventions
  ```
- Non-negotiable rules (line 19): Remove "- Do not violate constitution rules."
- Non-negotiable rules (line 21): Remove "- Do not directly modify accepted constitution rules."
- Non-negotiable rules (line 23): Change "Do not update the wiki in a way that contradicts the constitution or accepted ADRs." to "Do not update the wiki in a way that contradicts accepted ADRs."
- Document roles: Remove the "- Constitution: mandatory project rules." row.
- Escalation (line 37): Remove "- The requested work conflicts with the constitution."

#### 5.2 README.md
- Workflow diagram (line 44): Change `→ ADR / Constitution / Wiki updates` to `→ ADR / Wiki updates`.
- Agent table (line 54): Change scout description from "code, wiki, ADRs, constitution" to "code, wiki, ADRs".
- Agent table (line 60): Change code-reviewer description from "spec, contract, tests, constitution" to "spec, contract, tests".
- Agent table (line 62): Remove the constitution-agent row entirely.
- Authority order (lines 68-72): Replace with:
  ```
  1. `docs/adr/` — architectural decision records
  2. `.specs/` — historical feature specs (snapshots, not live docs)
  3. `docs/wiki/` — operational knowledge
  4. Existing code conventions
  ```
- Project structure tree (line 172): Remove `├── constitution/    # Mandatory project rules` line.

#### 5.3 SpecKit.md
- Agent table (line 10): Change scout description to remove "constitution," so it reads "Searches code, wiki, ADRs, specs, and disk".
- Agent table (line 16): Change code-reviewer description to remove "and constitution" so it reads "Reviews implementation against spec, contract, and tests".
- Agent table (line 17): Remove constitution-agent row entirely.
- Workflow diagram (line 53): Change `→ ADR / constitution / wiki updates` to `→ ADR / wiki updates`.
- Documents section (line 68): Remove `- constitution/` — Mandatory project rules.
- Installation (line 83): Remove `- docs/constitution/**`.
- Authority order (lines 59-63): Replace with:
  ```
  1. `docs/adr/**`
  2. `docs/wiki/**`
  3. `.specs/**`
  4. Existing code conventions
  ```

### Phase 6 — Modify wiki files

#### 6.1 docs/adr/README.md
- Line 3: Change "ADRs document meaningful architecture decisions. They are not automatically constitutional rules." to "ADRs document meaningful architecture decisions."

#### 6.2 docs/wiki/README.md
- Line 3: Remove "It is not a source of mandatory rules." — this implicitly contrasted with the now-deleted constitution. Replace with "The wiki describes the current operational understanding of the system."

#### 6.3 docs/wiki/decisions/adr-index.md
- Remove any constitution-related references. Specifically:
  - Lines 40: "Architecture boundary by convention (no automated guard yet)" — This is fine as-is (doesn't reference constitution).
  - The table at the top of the file should remain unchanged (it's ADR data).
  - Check for any mention of "constitution" and remove or replace with appropriate ADR references.

#### 6.4 docs/wiki/engineering/testing.md
- Line 75: Update the AMEND-2026-001 reference from `[...](/docs/constitution/rules/CONST-001-architecture-boundaries.md#amendment-amend-2026-001--sdl3-test-file-exception)` to `[...](/docs/adr/ADR-019-architecture-boundaries.md#amendment-amend-2026-001--sdl3-test-file-exception)`.
- Lines 223-226: Remove the entire `## Constitution reference` section heading and its content about CONST-002. Replace with a brief statement: `Testing policy is enforced by agent prompts in the SDD workflow, not by a standalone document.` OR remove the section entirely.

#### 6.5 docs/wiki/architecture/overview.md
- Line 33: Remove `├── constitution/        # Mandatory project rules` from the directory tree.

### Phase 7 — Delete files and directories
After all content has been migrated, modified, and saved:
1. Delete the entire `docs/constitution/` directory and all its contents.
2. Delete `docs/wiki/decisions/constitution-index.md`.
3. Delete `docs/templates/constitution-rule-template.md`.
4. Delete `docs/templates/amendment-template.md`.
5. Delete `.opencode/agents/constitution-agent.md`.

### Phase 8 — Final verification
- Run `git status` to confirm the working tree matches expected changes.
- Run `git diff --stat` to confirm no files outside the "Files allowed to change" list were modified.
- Run `git grep -n constitution` on each modified file to verify no unintended constitution references remain.

## Required tests

This is a documentation and governance refactoring. No unit tests or integration tests are required.

### Verification checklist (replaces traditional tests)

- **AC-001**: Verify `docs/constitution/` directory does not exist (`ls docs/constitution/` should fail).
- **AC-002**: Verify `.opencode/agents/constitution-agent.md` does not exist.
- **AC-003**: Verify `opencode.json` has no `constitution-agent` key in the `agent` object.
- **AC-006**: Verify `docs/adr/ADR-019-architecture-boundaries.md` exists and contains architecture boundary rule text and amendment history.
- **AC-009**: Verify `docs/wiki/engineering/principles.md` exists with the 5 principles.
- **AC-011**: Verify `AGENTS.md` authority order is: `docs/adr/**` > current spec > `docs/wiki/**` > code.
- **AC-015**: Verify `orchestrator.md` has no references to constitution-agent.
- **AC-037**: Verify `implementation-contract-template.md` has no "Relevant constitution rules" or "Constitution impact" sections.
- **AC-049/050**: Verify `git diff --stat` shows no changes to `.specs/` archives or `src/`/`tests/` files.
- **Full grep**: Run `git grep -n "constitution" -- docs/templates/` — should return zero results.
- **E2E grep**: Run `git grep -in "constitution" -- .opencode/agents/ AGENTS.md README.md SpecKit.md` — only expected references should remain (ADR-019 will reference "constitutional rule" or "constitution" in historical context; those are acceptable).

## Edge cases

| Edge case | Handling |
|---|---|
| ADR-019 number conflict (ADR-019 may already exist) | Check `docs/adr/` directory. If ADR-019 exists, use ADR-020 or next available number. |
| Cross-references to CONST-001 in existing ADRs (002, 003, 004, 012, 014) | Do NOT modify existing ADRs. ADR-019 should note at the bottom which ADRs historically referenced the old CONST-001 path. |
| References to "constitution" in experiments document | Leave as-is (`experiments-spec-driven-dev.md` is a historical journal). |
| `opencode.json` invalid JSON after constitution-agent removal | Must ensure valid JSON: remove the comma from the preceding `code-reviewer` entry's closing brace if the constitution-agent entry was between them. |
| Constitution directory deletion fails (git doesn't track empty dirs) | Git tracks files, not directories. Since the constitution directory contains files, `git rm -r docs/constitution/` will handle it. |
| Non-empty `docs/constitution/` after rules deletion | Delete each file individually first, then the directory. Use `git rm -r docs/constitution/` to stage the deletion. |
| `docs/wiki/decisions/constitution-index.md` might not exist | If the file does not exist (e.g., already deleted), skip the deletion step for that file. |
| Agent prompt has additional constitution references not listed in spec | Treat as a bug — find and fix ALL constitution references in each target file during the modification phase. Run `git grep -n "constitution"` on each modified file after editing to verify completeness. |

## Security impact

None. This is a documentation and governance refactoring with no code or runtime changes.

## Data and migration impact

None. No schema changes, data migrations, or seed data changes.

## API compatibility impact

None. No code API or public interface changes.

## Documentation impact

- README: Update agent table, authority order, workflow diagram, project structure tree.
- Wiki pages: Update `docs/wiki/README.md`, `docs/wiki/decisions/adr-index.md`, `docs/wiki/engineering/testing.md`, `docs/wiki/architecture/overview.md`; create `docs/wiki/engineering/principles.md`; delete `docs/wiki/decisions/constitution-index.md`.
- Other docs: Update `AGENTS.md`, `SpecKit.md`, `docs/adr/README.md`.
- Templates: Update `coordination-template.md`, `governance-review-template.md`, `implementation-contract-template.md`, `wiki-page-template.md`, `adr-template.md`; delete `constitution-rule-template.md`, `amendment-template.md`.
- Agent prompts: Update all 11 agent prompts under `.opencode/agents/`.
- Configuration: Update `opencode.json`.

## ADR impact

A new ADR (`ADR-019-architecture-boundaries.md`) is created to migrate the CONST-001 architecture boundary content from the constitution into ADR format. No existing ADRs are modified.

## Constitution impact

Not applicable. The constitution is being removed by this implementation.

## Done criteria

All of the following must be verifiable:

- [ ] `docs/constitution/` directory no longer exists (verify with `ls docs/constitution/` — should fail).
- [ ] `.opencode/agents/constitution-agent.md` no longer exists.
- [ ] `docs/templates/constitution-rule-template.md` no longer exists.
- [ ] `docs/templates/amendment-template.md` no longer exists.
- [ ] `docs/wiki/decisions/constitution-index.md` no longer exists.
- [ ] `docs/adr/ADR-019-architecture-boundaries.md` exists and contains CONST-001 content with amendment history.
- [ ] `docs/wiki/engineering/principles.md` exists with the 5 engineering principles.
- [ ] `opencode.json` parses as valid JSON with no `constitution-agent` entry.
- [ ] `opencode.json` scout/spec-critic/code-reviewer descriptions no longer reference constitution.
- [ ] `AGENTS.md` authority order is: `docs/adr/**` > `.specs/<current-sprint>/<feature>/spec.md` > `docs/wiki/**` > code conventions.
- [ ] `AGENTS.md` non-negotiable rules do not contain "constitution" or "constitutional".
- [ ] `AGENTS.md` document roles do not include constitution.
- [ ] `AGENTS.md` escalation reasons do not include constitution conflicts.
- [ ] `README.md` agent table has no constitution-agent row.
- [ ] `README.md` workflow diagram shows `→ ADR / Wiki updates` (not constitution).
- [ ] `README.md` authority order has no constitution.
- [ ] `README.md` project structure tree has no constitution directory.
- [ ] `SpecKit.md` agent table has no constitution-agent row.
- [ ] `SpecKit.md` workflow diagram shows `→ ADR / wiki updates`.
- [ ] `SpecKit.md` documents section has no constitution directory.
- [ ] `SpecKit.md` installation section has no `docs/constitution/**`.
- [ ] `SpecKit.md` authority order has no constitution.
- [ ] `docs/templates/coordination-template.md` has no `## constitution-agent` or `## adr-agent` sections.
- [ ] `docs/templates/governance-review-template.md` has no `## Constitution violations` section.
- [ ] `docs/templates/implementation-contract-template.md` has no `## Relevant constitution rules` or `## Constitution impact` sections.
- [ ] `docs/templates/wiki-page-template.md` has no `## Related constitution rules` section.
- [ ] `docs/templates/adr-template.md` has no `## Does this imply a constitutional rule?` section.
- [ ] `.opencode/agents/orchestrator.md` has no references to constitution-agent or constitutional rules.
- [ ] `.opencode/agents/scout.md` has no "Constitution" in search scope or output format.
- [ ] `.opencode/agents/spec-critic.md` has no `docs/constitution/**` check.
- [ ] `.opencode/agents/spec-author.md` self-validation rule 7 does not mention "constitution".
- [ ] `.opencode/agents/implementation-contract-author.md` contract sections table has no "Relevant constitution rules" or "Constitution impact" rows; self-validation rule 7 does not mention "constitution".
- [ ] `.opencode/agents/implementation-contract-critic.md` has no "Missing constitution impact" or "Contradictions with `docs/constitution/**`" checks.
- [ ] `.opencode/agents/code-implementer.md` has no "Read relevant constitution rules" or "Modify `docs/constitution/**`" instructions.
- [ ] `.opencode/agents/code-reviewer.md` has no "Constitution rules" in check-against list; review questions do not mention constitution.
- [ ] `.opencode/agents/governance-reviewer.md` has no constitution violation checks or constitution review steps.
- [ ] `.opencode/agents/wiki-agent.md` has no "Check the constitution" or "Do not contradict the constitution" rules.
- [ ] `.opencode/agents/adr-agent.md` has no "do not automatically create constitutional rules" language.
- [ ] `docs/adr/README.md` does not mention "constitutional rules".
- [ ] `docs/wiki/README.md` does not contain "It is not a source of mandatory rules" or constitution contrasts.
- [ ] `docs/wiki/decisions/adr-index.md` has no constitution references.
- [ ] `docs/wiki/engineering/testing.md` line 75 references `ADR-019-architecture-boundaries.md` instead of the old constitution path; lines 223-226 constitution reference section is removed/updated.
- [ ] `docs/wiki/architecture/overview.md` directory tree no longer lists `├── constitution/`.
- [ ] `git diff --stat` shows changes only in files listed in "Files allowed to change".
- [ ] `git diff --stat` shows NO changes in `src/`, `tests/`, `CMakeLists.txt`, `.specs/` (except current feature coordination.md), `experiments-spec-driven-dev.md`, or existing ADRs (001-018).
- [ ] `git grep -in "constitution" -- docs/templates/` returns zero results.
- [ ] The remaining "constitution" references in the repository are only in: ADR-019 (historical migration note), existing ADRs (001-018, historical cross-references — not modified), and `experiments-spec-driven-dev.md` (historical journal — not modified).
