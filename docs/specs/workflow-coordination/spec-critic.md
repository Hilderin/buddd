# Spec Review — SPEC-013 — Workflow Coordination (Warnings addition review)

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

This review examines the recent additions to the accepted SPEC-013 (Workflow Coordination) that introduce a `**Warnings**` field into the coordination.md structure. Specifically:

1. **`**Warnings**` field** added to all sub-agent sections in coordination.md template
2. **AC-003** updated to include Warnings in required fields
3. **Story 2** updated to include Warnings in the field list
4. **Sub-agent behavior** section updated to define Warnings as non-blocking concerns
5. **Gate mechanism** clarified that Warnings are non-blocking and do not affect gate decisions
6. **Delegation invariant** added — orchestrator must tell sub-agents to update coordination.md including warnings

**Verdict: The spec additions are internally consistent and correct. The Warnings field is properly integrated across all relevant sections. All previous resolutions (B-01 through B-05, C-01 through C-04) remain intact. However, the accepted implementation contract is now out of sync with the spec — it does not mention Warnings in the template definition or any sub-agent "After writing" sections, creating a contradiction that must be resolved before implementation.**

### Verification of the 6 additions

| # | Addition | Status | Evidence |
|---|---|---|---|
| 1 | Warnings field in all sub-agent sections | ✅ Complete | Present in all 11 sub-agent sections + Human Validation (lines 162, 176, 190, 204, 215, 228, 242, 258, 274, 290, 304) |
| 2 | AC-003 updated | ✅ Complete | Line 116 includes `**Warnings**` in the required field list |
| 3 | Story 2 updated | ✅ Complete | Line 92 includes `**Warnings**` in the field list |
| 4 | Sub-agent behavior updated | ✅ Complete | Lines 433-434 define Warnings as "non-blocking concerns, suggestions, or minor issues" |
| 5 | Gate mechanism clarifies Warnings | ✅ Complete | Line 446: "**Warnings** are non-blocking and do NOT affect gate decisions" |
| 6 | Delegation invariant added | ✅ Complete | Line 357 includes "warnings" in the sub-agent instruction template |

### Verification of all sections having Warnings

| Section | Has Warnings? | Location |
|---|---|---|
| spec-author | ✅ | Line 162 |
| spec-critic | ✅ | Line 176 |
| implementation-contract-author | ✅ | Line 190 |
| implementation-contract-critic | ✅ | Line 204 |
| Human Validation | ✅ | Line 215 |
| code-implementer | ✅ | Line 228 |
| code-reviewer | ✅ | Line 242 |
| adr-agent | ✅ | Line 258 |
| constitution-agent | ✅ | Line 274 |
| wiki-agent | ✅ | Line 290 |
| governance-reviewer | ✅ | Line 304 |
| Orchestrator | N/A (not a sub-agent) | Not applicable — Orchestrator section tracks workflow state, does not need warnings |

All 11 sub-agent sections plus Human Validation have the Warnings field. ✅

## Blocking issues

No blocking issues identified for the spec itself. The spec additions are internally consistent, well-defined, and properly integrated.

## Critical issues

- [ ] **CR-01: Implementation contract out of sync — Warnings field missing from template definition and all "After writing" sections.**

  The accepted implementation contract (`docs/specs/workflow-coordination/implementation-contract.md`) was written before the Warnings field was added to the spec. It is now contradictory to the spec in two significant ways:

  **a) Template definition (lines 33–44):** The implementation contract defines each section's fields for the coordination template. **None of the 13 section definitions include `**Warnings**`.** For example:

  > `3. \`## spec-author\` — with fields: **Status** (pending, in-progress, completed, blocked), **Summary**, **Artifacts**, **Questions for human**, **Blocking issues**`

  This should include `**Warnings**` after `**Questions for human**` and before `**Blocking issues**` to match the spec's detailed design (lines 153–165).

  **b) All 11 sub-agent "After writing" sections (Files 3–12):** Every sub-agent's coordination.md update instructions list the fields to update. **None include `**Warnings**`.** For example, spec-author.md (lines 337–342) lists:

  > `**Status**`, `**Summary**`, `**Artifacts**`, `**Questions for human**`, `**Blocking issues**`

  Missing: `**Warnings**`. This pattern is repeated verbatim across spec-critic.md, implementation-contract-author.md, implementation-contract-critic.md, code-implementer.md, code-reviewer.md, adr-agent.md, constitution-agent.md, wiki-agent.md, and governance-reviewer.md.

  **Impact:** If the implementation contract is followed literally:
  - The `coordination-template.md` will not include Warnings fields
  - Sub-agents will not be instructed to populate Warnings
  - Resulting coordination.md files will violate AC-003 (which requires Warnings in each sub-agent section)

  **The implementation contract must be updated to include Warnings in every section's field list and every sub-agent's "After writing" instructions before implementation proceeds.**

## Recommendations

- [ ] **R-01 (Implementation contract update):** Update `docs/specs/workflow-coordination/implementation-contract.md` to:
  1. Add `**Warnings**` to every section definition in the template structure (lines 33–44), positioned after `**Questions for human**` and before `**Blocking issues**`.
  2. Add `**Warnings**` to every sub-agent "After writing" section (Files 3–12), following the same ordering.
  3. Add `**Warnings**` to the Human Validation section's field list (line 38) — note it already appears in the spec's template but is missing from the implementation contract's definition.
  4. Update VC-01 or add a new VC to verify Warnings field presence in the generated template.
  5. Update Risk #1 (line 787) to note that Warnings was added as a new field and the pattern is now stable.

- [ ] **R-02 (consistency check):** Consider verifying that the `adr-agent`, `constitution-agent`, and `wiki-agent` sections in the implementation contract also include Warnings in their respective "After writing" sections. The spec template correctly includes Warnings for these agents (lines 258, 274, 290), and the implementation contract's "After writing" sections (lines 552, 577, 602) should be updated to match.

- [ ] **R-03 (previous recommendation, still valid):** The implementation contract critic review (line 48) flagged a missing closing parenthesis in the diagram: `constitution-agent (parallel →` should be `constitution-agent (parallel) →`. This cosmetic issue should be fixed alongside the Warnings updates.

- [ ] **R-04 (previous recommendation, still valid):** The implementation contract (line 19) says "Modifies all 11 sub-agent `.md` files" but only 10 sub-agents receive "After writing" append sections. Consider correcting to "10 sub-agent" (or "11 agent" if including the orchestrator).

## Previous resolutions — verification of continued integrity

All previously resolved issues remain intact after the Warnings additions:

| ID | Description | Status |
|---|---|---|
| B-01 | question tool undefined — resolved (lines 456–457, 628, 601) | ✅ Intact |
| B-02 | approved vs accepted inconsistency — resolved (line 393, unified to "approved") | ✅ Intact |
| B-03 | Loop-back gap — resolved (line 387, re-review rule) | ✅ Intact |
| B-04 | AC-004 not verifiable — resolved (line 117, test-script approach) | ✅ Intact |
| B-05 | AC-005 instrumentation undefined — resolved (line 118–119, code-review approach) | ✅ Intact |
| C-01 | Status consolidation — resolved (lines 320–333, 632) | ✅ Intact |
| C-02 | Next step removed — resolved (field absent from all templates) | ✅ Intact |
| C-03 | Section identification — resolved (lines 428–429, exact heading match) | ✅ Intact |
| C-04 | Two-stage acceptance — resolved (lines 383–384, technical vs human) | ✅ Intact |
| CR-01 | Status ownership vs Loop handling — resolved (line 349, explicit exception) | ✅ Intact |

## Open questions for human

None. The single critical issue (CR-01 — implementation contract out of sync) is a straightforward mechanical update that the implementation-contract-author can resolve without human input. The spec's Warnings additions are correct and should be accepted.

## Detailed findings

### Gate mechanism correctness

Line 446: "Note: `**Warnings**` are non-blocking and do NOT affect gate decisions — they are informational only."

The gate mechanism (lines 446–451) specifies two checks:
1. `**Status**` — if `rejected`, loop back
2. `**Blocking issues**` — if unchecked `- [ ]` items, loop back

Warnings are explicitly excluded from both checks. This is correct and unambiguous. ✅

### Delegation invariant correctness

Line 357: The orchestrator MUST instruct sub-agents with: "After completing your work, read coordination.md, find your section (`## <agent-name>`), and update it with your status, summary, artifacts, questions, warnings, and blocking issues."

This matches the field ordering in Story 2 (lines 87–93) and Sub-agent behavior (lines 429–436). The invariant ensures every sub-agent knows it must update the Warnings field. ✅

### Field ordering consistency

The field order across all placement points is consistent:
1. Status
2. Summary
3. Artifacts
4. Questions for human
5. Warnings (new)
6. Blocking issues

Verified in: Story 2 (lines 87–93), Sub-agent behavior (lines 429–436), and all 12 template sections (lines 153–307). ✅

### No scope creep

The Warnings field is a minimal, well-scoped addition. It does not:
- Change the workflow structure
- Add new agents or roles
- Alter existing gate logic
- Introduce new templates or files
- Create new automation requirements

It simply adds a field for non-blocking concerns, which was an acknowledged gap in the previous review cycle. ✅
