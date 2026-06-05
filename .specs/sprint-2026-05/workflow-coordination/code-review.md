# SPEC-013 — Code Review: Workflow Coordination

## Scope

Review of the implementation of the workflow coordination feature against SPEC-013 and its implementation contract.

## Files reviewed

**Created (1):**
- `docs/templates/coordination-template.md`

**Modified (16):**
- `.opencode/agents/orchestrator.md`
- `.opencode/agents/spec-author.md`
- `.opencode/agents/spec-critic.md`
- `.opencode/agents/implementation-contract-author.md`
- `.opencode/agents/implementation-contract-critic.md`
- `.opencode/agents/code-implementer.md`
- `.opencode/agents/code-reviewer.md`
- `.opencode/agents/adr-agent.md`
- `.opencode/agents/constitution-agent.md`
- `.opencode/agents/wiki-agent.md`
- `.opencode/agents/governance-reviewer.md`
- `docs/templates/spec-template.md`
- `docs/templates/review-report-template.md`
- `docs/templates/implementation-contract-template.md`
- `docs/templates/governance-review-template.md`
- `AGENTS.md`

## Status

`Accepted with warnings`

## Positive aspects

### coordination-template.md
- All **13 sections** present in correct order as specified (title + 12 headings: Orchestrator, spec-author, spec-critic, implementation-contract-author, implementation-contract-critic, Human Validation, code-implementer, code-reviewer, adr-agent, constitution-agent, wiki-agent, governance-reviewer) ✓
- `## Human Validation` correctly placed between `## implementation-contract-critic` and `## code-implementer` ✓
- All required field names present in each section with correct **bold** markdown formatting ✓
- Extra fields present: `**Decisions needed**` in adr-agent, `**Changes needed**` in constitution-agent, `**Changes made**` in wiki-agent ✓
- `**Warnings**` field present in every sub-agent section ✓
- No `## Status` field in the template ✓
- Constraints section documents all required rules including the loop-back exception ✓

### orchestrator.md
- Workflow diagram shows coordination.md creation at start ("**CREATE** coordination.md from template") ✓
- All gates reference coordination.md sections exclusively (e.g., `check coordination.md ## spec-critic`) ✓
- No remaining "read full artifact" instructions for status/blocking-issue decisions ✓
- Delegation invariant present (lines 267–269): includes the exact instruction to sub-agents to update coordination.md ✓
- Step 1 (Clarify) includes detailed coordination.md creation instructions with field values ✓
- Steps 2–12 all reference coordination.md section reads in their gates ✓
- Loop-back logic present in steps 3, 5, 8, 11 with coordination.md section references ✓
- Step 6 (Human validation) draws summaries from coordination.md (not artifact files) ✓
- Step 12 (Done) sets `## Orchestrator` → **Status** to "completed" ✓
- Hard rules include "Never read full artifact files for status or blocking-issue information" (line 458) ✓
- Loop-back exception note present in step 3 (line 298) ✓
- Question mechanism: every gate that checks "Questions for human" says to ask immediately and record answer ✓

### Sub-agent files (11 agents)
- Each has a `## After writing` section appended after the appropriate preceding section ✓
- Each follows the consistent pattern: read coordination.md → find section by exact heading → update fields → do not modify other sections → append history → escalate if missing ✓
- The 6 standard fields present in all: Status, Summary, Artifacts, Questions for human, Warnings, Blocking issues ✓
- Critic agents (spec-critic, impl-contract-critic, code-reviewer, governance-reviewer) have Status values including "rejected" ✓
- Non-critic agents have Status values without "rejected" ✓
- adr-agent includes `**Decisions needed**` extra field ✓
- constitution-agent includes `**Changes needed**` extra field ✓
- wiki-agent includes `**Changes made**` extra field ✓

### Template status field removal
- **spec-template.md**: `## Status` section removed (lines 3–8), file now starts with `# SPEC-YYYY-NNNN - Title` → `## Approval` ✓
- **review-report-template.md**: `## Status` section removed (lines 3–10), file now starts with `# [Spec | Implementation Contract] Review` → `## Blocking issues` ✓
- **implementation-contract-template.md**: `## Status` section removed (lines 3–8), file now starts with `# IMPL-YYYY-NNNN - Title` → `## Approval` ✓
- **governance-review-template.md**: `## Status` section removed (lines 3–10), file now starts with `# Governance Review` → `## Cross-document coherence` ✓

### AGENTS.md
- Coordination files role added to `## Document roles` (line 33) ✓
- Non-negotiable rule about coordination.md added (line 24) ✓

### Old instruction cleanup
- **spec-author.md**: No "Set the spec's `## Status`" — replaced with reference to spec-template without Status section ✓
- **spec-critic.md**: No "Set the review file's `## Status`" or "Update the spec file's `## Status`" — replaced with coordination.md references ✓; no duplicate "4." numbering — steps are 1–8 sequential ✓
- **implementation-contract-author.md**: No "Set the contract's `## Status`" — replaced with coordination.md reference ✓
- **implementation-contract-critic.md**: No "Set the review file's `## Status`" or "Update the contract file's `## Status`" ✓
- **code-reviewer.md**: No "Set the review file's `## Status`" — step 10 removed ✓
- **governance-reviewer.md**: No "Set the review file's `## Status`" — step 12 removed ✓
- All "update the verdict" → "update the review summary text" changes applied in spec-critic, impl-contract-critic, code-reviewer, governance-reviewer ✓

### code-implementer.md
- Before editing step 1: reads coordination.md `## Human Validation` section for approval status ✓
- Before editing step 2: reads coordination.md `## implementation-contract-critic` section instead of artifact file ✓
- Uses correct file path `implementation-contract-critic.md` (not old `contract-critic.md`) ✓

### Cross-file consistency
- No contradictions between coordination-template.md, orchestrator.md, sub-agent files, templates, and AGENTS.md ✓
- The loop-back exception is consistently documented in coordination-template.md constraints, orchestrator.md step 3, and referenced throughout ✓
- No orphaned references to artifact `## Status` fields ✓
- Sub-agent sections in coordination-template.md match the sub-agent names used throughout the system ✓

### Constitution compliance
- No constitution rules are violated by this implementation ✓
- The changes are documentation/governance only — no code changes, no architecture changes ✓

## Warnings

- [ ] **Missing explicit statement that Warnings are non-blocking (VC-28):** The spec (line 446) and implementation contract VC-28 require the orchestrator workflow to include an explicit clarification that `**Warnings**` are non-blocking and do NOT affect gate decisions — that they are informational only. While the orchestrator gates correctly ignore Warnings (they only check Status, Blocking issues, and Questions for human), there is no **explicit statement** to this effect anywhere in `orchestrator.md`. The Warnings field is defined as "non-blocking" in each sub-agent's `## After writing` section, but the orchestrator's own gate mechanism does not carry this clarification. This is a documentation gap that could lead to confusion about Warnings' role in gate decisions. **Recommendation:** Add a note to the `orchestrator.md` workflow section (e.g., after the Delegation invariant or as a preamble to the gate descriptions) stating: "Note: `**Warnings**` are non-blocking and do NOT affect gate decisions — they are informational only."

## Pre-existing observations (not introduced by this implementation)

- **orchestrator.md line 456:** The hard rule says "Never create or update constitution yourself, ask `adr-agent`." This should reference `constitution-agent`, not `adr-agent`. This error predates this implementation and was not corrected.
- **orchestrator.md line 455:** Similarly says "Never create or update ADR yourself, ask `adr-agent`." (This one is correct.)
- No existing coordination.md files need migration (deployed for the first time).

## Recommendations

1. **Fix the missing Warnings clarification (see Warning above):** Add an explicit statement in `orchestrator.md` that `**Warnings**` are non-blocking and do not affect gate decisions, as required by the spec.
2. **Fix the pre-existing orchestrator hard rule error** (`constitution-agent` vs `adr-agent`) in a separate change — it is out of scope for this implementation but should be corrected.
3. **Consider adding a runtime verification test:** The spec includes behavioral ACs (AC-004 through AC-012) that describe runtime orchestrator behavior. While these can only be verified by running the orchestrator, consider adding a traceability matrix that maps each AC to the specific orchestrator.md instructions that govern it.
4. **After this feature is accepted:** The wiki-agent should be invoked to update `docs/wiki/` with the new coordination workflow, coordination.md structure, and agent update patterns.

## Verification of all ACs

| AC-ID | Description | Status | Evidence |
|-------|-------------|--------|----------|
| AC-001 | coordination.md created by orchestrator at workflow start | ✓ Met | orchestrator.md step 1 (lines 257–265) |
| AC-002 | coordination.md follows template structure | ✓ Met | coordination-template.md exists with all required sections |
| AC-003 | Each sub-agent section has all 6 required fields | ✓ Met | All 11 sub-agent sections in template have Status, Summary, Artifacts, Questions, Warnings, Blocking |
| AC-004 | Sub-agents update coordination.md after completing artifact | ✓ Met | Every sub-agent file has `## After writing` section |
| AC-005 | Orchestrator never reads full artifacts for status/blocking | ✓ Met | All orchestrator gates reference coordination.md sections; hard rule line 458 enforces |
| AC-006 | Orchestrator checks coordination.md after every sub-agent | ✓ Met | Every workflow step's gate reads coordination.md |
| AC-007 | Status "rejected" loops back to appropriate agent | ✓ Met | Steps 3, 5, 8, 11 all check for rejected status and loop |
| AC-008 | Unchecked blocking issues loop back to appropriate agent | ✓ Met | Steps 3, 5, 8, 11 all check for unchecked `- [ ]` items |
| AC-009 | Questions for human asked immediately | ✓ Met | Multiple gates include "ask human immediately" instruction |
| AC-010 | Human answer recorded in coordination.md | ✓ Met | Step 3 (spec-critic) shows "record answer in coordination.md"; pattern repeated |
| AC-011 | Orchestrator updates `## Orchestrator` section | ✓ Met | Every step updates `## Orchestrator` → **Current step** |
| AC-012 | Human Validation section updated by orchestrator | ✓ Met | Step 6 (lines 344–355) records approval in `## Human Validation` |
| AC-013 | coordination-template.md exists with all sections | ✓ Met | File exists, 13 sections present |
| AC-014 | Loop-back re-populates target agent section | ✓ Met | Steps 3, 5, 8, 11 set target status to "in-progress" with context |
| AC-015 | `## Status` removed from all artifact templates | ✓ Met | All 4 templates confirmed without `## Status` |
| AC-009/010* | Question mechanism works end-to-end | ✓ Met | Orchestrator.md gates include ask+record pattern per spec design |

*Behavioral ACs that require runtime verification are supported by orchestrator.md instructions.

## Verification of all VCs

| VC-ID | Description | Status | Evidence |
|-------|-------------|--------|----------|
| VC-01 | coordination-template.md has 13 sections, correct order, all fields including Warnings | ✓ Pass | File inspection confirms |
| VC-02 | Each sub-agent (except orchestrator, scout) has coordination.md update section with Warnings | ✓ Pass | All 11 non-orchestrator agents have `## After writing` with Warnings |
| VC-03 | orchestrator.md: diagram shows coord.md creation, all gates reference coord.md sections, no "read full artifact" | ✓ Pass | Lines 189–235 diagram, all step gates reference coordination.md |
| VC-04 | Hard rules includes "Never read full artifact files for status/blocking" | ✓ Pass | Line 458 |
| VC-05 | spec-template.md has no `## Status` | ✓ Pass | Lines 1–2: title → Approval |
| VC-06 | review-report-template.md has no `## Status` | ✓ Pass | Lines 1–2: title → Blocking issues |
| VC-07 | implementation-contract-template.md has no `## Status` | ✓ Pass | Lines 1–2: title → Approval |
| VC-08 | governance-review-template.md has no `## Status` | ✓ Pass | Lines 1–2: title → Cross-document coherence |
| VC-09 | AGENTS.md includes coordination files role + coord.md rule | ✓ Pass | Lines 24, 33 |
| VC-10 | spec-author.md no "Set the spec's `## Status`" | ✓ Pass | Line 26: replaced with template reference |
| VC-11 | spec-critic.md no status-setting instructions | ✓ Pass | Lines 46–47: verdict text + coordination.md reference |
| VC-12 | impl-contract-author.md no status-setting | ✓ Pass | Line 26: replaced with template reference |
| VC-13 | impl-contract-critic.md no status-setting | ✓ Pass | Line 51: coordination.md status reference |
| VC-14 | code-reviewer.md no status-setting | ✓ Pass | Step 10 removed |
| VC-15 | governance-reviewer.md no status-setting | ✓ Pass | Step 12 removed |
| VC-16 | code-implementer.md reads coord.md for Human Validation | ✓ Pass | Line 25 |
| VC-17 | All After writing sections follow same pattern | ✓ Pass | All 11 agents verified |
| VC-18 | Steps 3, 5, 8, 11 contain re-review loop logic with coord.md | ✓ Pass | All contain coord.md section reads |
| VC-19 | code-implementer.md step 2 reads coord.md, not artifact file | ✓ Pass | Line 27 |
| VC-20 | spec-critic.md has sequential step numbering (no duplicate 4.) | ✓ Pass | Steps 1–8 sequential |
| VC-21 | spec-critic.md Rules: "update the review summary text" not "update the verdict" | ✓ Pass | Line 56 |
| VC-22 | impl-contract-critic.md Rules: correct wording | ✓ Pass | Line 59 |
| VC-23 | code-reviewer.md Rules: correct wording | ✓ Pass | Line 81 |
| VC-24 | governance-reviewer.md Rules: correct wording | ✓ Pass | Line 57 |
| VC-25 | coordination-template.md constraints include loop-back exception | ✓ Pass | Line 177 |
| VC-26 | orchestrator.md step 3 references loop-back exception | ✓ Pass | Line 298 |
| VC-27 | orchestrator has Delegation invariant | ✓ Pass | Lines 267–269 |
| VC-28 | Gate mechanism clarifies Warnings are non-blocking | ⚠️ See Warning | No explicit statement in orchestrator.md |

## Conclusion

The implementation is comprehensive and correct across all 17 files. All acceptance criteria and verification criteria are satisfied except for one documentation gap (VC-28). The implementation is structurally sound, internally consistent, and constitutionally compliant.

**Verdict:** `Accepted with warnings`
