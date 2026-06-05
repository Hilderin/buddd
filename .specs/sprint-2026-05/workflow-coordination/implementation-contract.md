# SPEC-013 — Implementation Contract: Workflow Coordination

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Source spec

[SPEC-013 Workflow Coordination](spec.md) (Accepted, CR-01 resolved per spec-critic review)

## Implementation overview

Introduce a `coordination.md` file per feature as the single source of truth for orchestrator decision-making. The orchestrator creates coordination.md at workflow start; each sub-agent updates their own section after completing their work. The orchestrator reads ONLY coordination.md for workflow decisions, replacing the current practice of reading full artifact files.

This implementation:
- Creates `docs/templates/coordination-template.md` as the structural template
- Modifies all 11 sub-agent `.md` files to add a coordination.md update step after artifact creation
- Rewrites the orchestrator's workflow section to use coordination.md gates instead of "read full artifact" gates
- Removes `## Status` sections from 4 artifact templates (spec, review-report, impl-contract, gov-review)
- Updates `AGENTS.md` workflow gates to reference coordination.md

No new dependencies, no code changes, no schema changes.

## Implementation plan

### File 1: `docs/templates/coordination-template.md` (CREATE)

Create the new template file at `docs/templates/coordination-template.md`. It must contain the following sections in this order with placeholder values:

1. `# Workflow Coordination: <feature-name>`
2. `## Orchestrator` — with fields: **Feature**, **Status** (allowed: in-progress, completed, blocked), **Current step**, **Initial instructions**, **Notes**
3. `## spec-author` — with fields: **Status** (pending, in-progress, completed, blocked), **Summary**, **Artifacts**, **Questions for human**, **Warnings**, **Blocking issues**
4. `## spec-critic` — with fields: **Status** (pending, in-progress, completed, rejected, blocked), **Summary**, **Artifacts**, **Questions for human**, **Warnings**, **Blocking issues**
5. `## implementation-contract-author` — with fields: **Status** (pending, in-progress, completed, blocked), **Summary**, **Artifacts**, **Questions for human**, **Warnings**, **Blocking issues**
6. `## implementation-contract-critic` — with fields: **Status** (pending, in-progress, completed, rejected, blocked), **Summary**, **Artifacts**, **Questions for human**, **Warnings**, **Blocking issues**
7. `## Human Validation` — with fields: **Status** (pending, approved, rejected), **Approver**, **Date**, **Time**, **Warnings**, **Notes**
8. `## code-implementer` — with fields: **Status** (pending, in-progress, completed, blocked), **Summary**, **Artifacts**, **Questions for human**, **Warnings**, **Blocking issues**
9. `## code-reviewer` — with fields: **Status** (pending, in-progress, completed, rejected, blocked), **Summary**, **Artifacts**, **Questions for human**, **Warnings**, **Blocking issues**
10. `## adr-agent` — with fields: **Status** (pending, in-progress, completed, blocked), **Summary**, **Artifacts**, **Decisions needed**, **Questions for human**, **Warnings**, **Blocking issues**
11. `## constitution-agent` — with fields: **Status** (pending, in-progress, completed, blocked), **Summary**, **Artifacts**, **Changes needed**, **Questions for human**, **Warnings**, **Blocking issues**
12. `## wiki-agent` — with fields: **Status** (pending, in-progress, completed, blocked), **Summary**, **Artifacts**, **Changes made**, **Questions for human**, **Warnings**, **Blocking issues**
13. `## governance-reviewer` — with fields: **Status** (pending, in-progress, completed, rejected, blocked), **Summary**, **Artifacts**, **Questions for human**, **Warnings**, **Blocking issues**

**Constraints:**
- Use exact heading names as listed above (case-sensitive)
- Use exact field names as listed above (bold markdown `**Field**`)
- Sub-agent sections must appear in the exact order listed above
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively)
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.

---

### File 2: `.opencode/agents/orchestrator.md` (MODIFY — Major rewrite of workflow section)

The existing `## Required workflow` section (lines 188–231) and `## Workflow details` section (lines 233–394) must be replaced. Specifically:

#### 2a. Replace the ASCII workflow diagram (lines 188–231)

Replace the old workflow diagram (lines 188–231) with a new diagram that shows the orchestrator creating coordination.md at the start and reading ONLY coordination.md at each gate. The new diagram must follow this structure:

```
Human
  ↓
orchestrator
  ↓
clarification with human, challenges request, decisions and assumptions
  ↓
**CREATE** coordination.md from template (all sections filled with "pending")
  ↓
spec-author → writes spec.md → updates coordination.md
  ↓
spec-critic → writes spec-critic.md → updates coordination.md
  ↓  (gate: check coordination.md ## spec-critic)
  ↓  Status == rejected? → loop to spec-author
  ↓  Blocking issues unchecked? → loop to spec-author
  ↓
implementation-contract-author → writes implementation-contract.md → updates coordination.md
  ↓
implementation-contract-critic → writes implementation-contract-critic.md → updates coordination.md
  ↓  (gate: check coordination.md ## implementation-contract-critic)
  ↓  Status == rejected? → loop to impl-contract-author (or spec-author for spec-level issues)
  ↓  Blocking issues unchecked? → loop to appropriate agent
  ↓  [Spec-level loop → spec-critic MUST re-review]
  ↓
**human validation gate** — present summaries from coordination.md, get explicit approval, record in ## Human Validation
  ↓
code-implementer → implements code → updates coordination.md
  ↓
code-reviewer → writes code-review.md → updates coordination.md
  ↓  (gate: check coordination.md ## code-reviewer)
  ↓  Status == rejected? → loop to code-implementer
  ↓  Blocking issues unchecked? → loop to code-implementer
  ↓
adr-agent → updates coordination.md
constitution-agent (parallel → updates coordination.md
  ↓
wiki-agent → updates coordination.md
  ↓
governance-reviewer → writes governance-review.md → updates coordination.md
  ↓  (gate: check coordination.md ## governance-reviewer)
  ↓  Status == rejected? → loop to appropriate agent
  ↓  Blocking issues unchecked? → loop to appropriate agent
  ↓
orchestrator sets ## Orchestrator Status to "completed" → reports to human
  ↓
(done)
```

#### 2b. Replace the "Workflow details" section (lines 233–394)

Replace the entire "## Workflow details" section (starting at line 233 through line 394) with the updated workflow below. The new workflow details section must contain the following **exact content**:

```
## Workflow details

### 1. Clarify

Understand and validate the human intent.

Clarify:
- the user goal
- expected behavior
- non-goals
- constraints
- impacted modules, if known
- acceptance criteria, if known

If project context is needed, call the scout with a bounded request.

Do not let the workflow continue if the feature is too vague to specify.

After clarification, create coordination.md:

1. Ensure `.specs/{{SPRINT}}/<feature>/` directory exists (create if not).
2. Create `.specs/{{SPRINT}}/<feature>/coordination.md` using `docs/templates/coordination-template.md` as the structure.
3. Fill `## Orchestrator` section with:
   - **Feature**: `<kebab-case feature name>`
   - **Status**: in-progress
   - **Current step**: clarification-complete
   - **Initial instructions**: the human's feature request / intent
   - **Notes**: (orchestrator's initial notes)
4. Fill all sub-agent sections with **Status**: pending, empty summaries/artifacts, and "none" for questions, warnings, and blocking issues.

### Delegation invariant

When delegating to any sub-agent, the orchestrator MUST include in its instructions: "After completing your work, read coordination.md, find your section (`## <agent-name>`), and update it with your status, summary, artifacts, questions, warnings, and blocking issues."

### 2. Spec author

Ask `spec-author` to draft a spec into `.specs/{{SPRINT}}/<feature>/spec.md`.

Give the spec-author:
- human intent
- relevant scout findings
- constraints
- open questions already answered by the human

Gate (after spec-author reports completion):
- Read coordination.md `## spec-author` section ONLY.
- If **Status** is "blocked", resolve the blocker (ask human if needed).
- If **Questions for human** is non-empty, ask human immediately using the `question` tool and record answer in coordination.md.
- If **Status** is "completed", proceed to spec-critic.
- Update `## Orchestrator` → **Current step** to "spec-author-complete".

### 3. Spec critic

Ask `spec-critic` to review and write `.specs/{{SPRINT}}/<feature>/spec-critic.md`.

Gate (after spec-critic reports completion):
- Read coordination.md `## spec-critic` section ONLY.
- If **Status** is "rejected" → loop back to spec-author (step 2).
- If any unchecked `- [ ]` items under **Blocking issues** → loop back to spec-author (step 2).
- If **Questions for human** is non-empty, ask human immediately using the `question` tool and record answer in coordination.md.
- When looping back: update `## Orchestrator` with loop note, set target agent's status to "in-progress" with context from blocking issues, re-invoke spec-author.
  - **Note:** This is the one exception to the rule that sub-agents self-manage their own status. The orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" during loop-backs. This is documented in the coordination.md template constraints.
- If gate passes, update `## Orchestrator` → **Current step** to "spec-critic-approved".
- Proceed to implementation-contract-author.

### 4. Implementation contract author

Ask `implementation-contract-author` to create `.specs/{{SPRINT}}/<feature>/implementation-contract.md`.

Give the contract author:
- accepted spec
- relevant scout findings
- relevant critic notes
- implementation constraints

Gate (same pattern as spec-author):
- Read coordination.md `## implementation-contract-author` section ONLY.
- Check **Status**, **Questions for human**, **Blocking issues**.
- Update `## Orchestrator` → **Current step**.

### 5. Implementation contract critic

Ask `implementation-contract-critic` to review and write `.specs/{{SPRINT}}/<feature>/implementation-contract-critic.md`.

Gate:
- Read coordination.md `## implementation-contract-critic` section ONLY.
- If **Status** is "rejected" → loop back to implementation-contract-author (step 4).
- If any unchecked `- [ ]` items under **Blocking issues**:
  - If the issue is a spec-level problem requiring spec.md changes → loop to spec-author (step 2). After spec-author fixes, orchestrator MUST invoke spec-critic to re-review the modified spec. If spec-critic accepts (coordination.md `## spec-critic` **Status** is `completed`), then proceed to implementation-contract-author (step 4). If spec-critic rejects, continue looping.
  - Otherwise → loop to implementation-contract-author (step 4).
- If **Questions for human** is non-empty, ask human immediately.
- When looping: update `## Orchestrator` with loop note, set target agent's status to "in-progress" with context.
- If gate passes, update `## Orchestrator` → **Current step**.

### 6. Human validation

Present the accepted spec and accepted implementation contract to the human.

Use the `question` tool with a summary drawn from coordination.md sections (NOT from full artifact files):
- Summary from `## spec-author` section
- Summary from `## implementation-contract-author` section
- Key ACs and done criteria

Ask for explicit approval to proceed with implementation.

Do not proceed until the human explicitly approves.

Record approval in coordination.md `## Human Validation` section:
- **Status**: approved (or rejected)
- **Approver**: human identity
- **Date**: <date>
- **Time**: <time>
- **Notes**: any human feedback or conditions

Also record approval metadata in `## Orchestrator` → **Notes**.

If rejected, terminate workflow and report to human.

If approved, update `## Orchestrator` → **Current step** to "human-approved".

### 7. Implement

Delegate to `code-implementer`.

Implementation is allowed only from an accepted and human-approved implementation contract. The contract's `## Approval` section must be filled.

Do not implement directly.
Do not allow implementation from a raw user request.
Do not allow implementation from a spec alone.

Gate (after code-implementer reports completion):
- Read coordination.md `## code-implementer` section ONLY.
- Check **Status**, **Questions for human**, **Blocking issues**.
- Update `## Orchestrator` → **Current step**.

### 8. Code review

Ask `code-reviewer` to review and write `.specs/{{SPRINT}}/<feature>/code-review.md`.

Gate:
- Read coordination.md `## code-reviewer` section ONLY.
- If **Status** is "rejected" → loop back to code-implementer (step 7).
- If any unchecked `- [ ]` items under **Blocking issues** → loop back to code-implementer (step 7).
- If **Questions for human** is non-empty, ask human immediately.
- Always rerun code-reviewer after implementation modifications.
- Be sure to analyze rendering and confirm functional display using `buddd capture` and vision analyze tool.
- When looping: update `## Orchestrator` with loop note, set code-implementer status to "in-progress" with context.
- If gate passes, update `## Orchestrator` → **Current step**.

### 9. Governance update

Ask `adr-agent` and `constitution-agent` in parallel whether any governance artifact is needed.

After each reports completion:
- Read coordination.md `## adr-agent` or `## constitution-agent` section.
- Check **Questions for human**: if present, ask human immediately.
- Check **Blocking issues**: if present, resolve.
- Never accept a constitutional change without explicit human approval.
- Update `## Orchestrator` → **Current step**.

### 10. Wiki update

Ask `wiki-agent` to update the wiki content.

Gate:
- Read coordination.md `## wiki-agent` section ONLY.
- Check **Status**, **Questions for human**, **Blocking issues**.
- Update `## Orchestrator` → **Current step**.

### 11. Final governance validation

Ask `governance-reviewer` for cross-document validation.

Gate:
- Read coordination.md `## governance-reviewer` section ONLY.
- If **Status** is "rejected" → resolve issues through the appropriate agent.
- If any unchecked `- [ ]` items under **Blocking issues** → resolve issues through the appropriate agent.
- If **Questions for human** is non-empty, ask human immediately.
- Always rerun governance-reviewer after modifications.
- When looping: update `## Orchestrator` with loop note, set target agent's status to "in-progress" with context.
- If gate passes, update `## Orchestrator` → **Current step**.

### 12. Done

Set `## Orchestrator` → **Status** to "completed".

Report completion to the human.

Include:
- implemented feature summary
- spec path
- contract path
- review path
- ADR/wiki/constitution updates, if any
- remaining non-blocking notes, if any

All artifacts stay in:
```
.specs/{{SPRINT}}/<feature>/
```

No file moving is needed.
```

#### 2c. Update the "Hard rules" section

In the existing `## Hard rules` section (starting at line 396), the following changes must be made:

- Add this new rule: "Never read full artifact files (spec.md, spec-critic.md, implementation-contract.md, etc.) for status or blocking-issue information — read only coordination.md sections."
- No existing hard rules are to be removed.

---

### File 3: `.opencode/agents/spec-author.md` (MODIFY — Add coordination.md update step)

After the existing "## Self-validation before submitting" section (ending at line 92), append:

```
## After writing

After completing the spec and passing self-validation:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## spec-author` section (exact heading match).
2. Update the following fields in `## spec-author`:
   - `**Status**`: `completed`
   - `**Summary**`: 2–5 lines describing what was done (scope, key sections, key decisions).
   - `**Artifacts**`: `- .specs/{{SPRINT}}/<feature>/spec.md`
   - `**Questions for human**`: if any questions arose that cannot be answered from project evidence, list them. If none, write "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: if any issues prevent the workflow from proceeding, use `- [ ]` checklist format. If none, write "none".
3. Do NOT modify any other section.
4. Do NOT modify the `## Orchestrator` section.
5. Do NOT remove or restructure the coordination.md file.
6. Append new content rather than overwriting previous loop history.

If coordination.md does not exist, escalate to the orchestrator.
```

Also, remove line 26 where `spec-author.md` currently says `Set the spec's `## Status` to `Draft` (the allowed values are `Draft`, `In Review`, `Accepted`).` — the spec-template no longer has a `## Status` field, so this instruction is obsolete. Replace it with: "The spec follows the template at `docs/templates/spec-template.md` which no longer contains a `## Status` section (status is tracked in coordination.md)."

---

### File 4: `.opencode/agents/spec-critic.md` (MODIFY — Add coordination.md update step)

Append after the last rule in the `## Rules` section (after line 57):

```
## After writing

After writing the review artifact and before reporting completion:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## spec-critic` section (exact heading match).
2. Update the following fields in `## spec-critic`:
   - `**Status**`: `completed` if accepted, `rejected` if rejected, `blocked` if blocked.
   - `**Summary**`: 2–5 lines describing review outcome.
   - `**Artifacts**`: `- .specs/{{SPRINT}}/<feature>/spec-critic.md`
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: copy the blocking issues checklist from spec-critic.md `## Blocking issues` section (the `- [ ]` items). If none, write "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.

Note: Do NOT update the spec file's `## Status` (the spec no longer carries a status field). The review verdict is expressed exclusively through the coordination.md `## spec-critic` section.
```

Also, remove or update step 6 in `## Review process` (line 47): "Update the spec file's `## Status` to `In Review`." — Replace with: "The spec's status is tracked in coordination.md. Do NOT modify the spec file's status field."

Also, replace step 5 (line 46): "Set the review file's `## Status` to one of: `Accepted`, `Accepted with warnings`, `Rejected`." — Since the review-report-template no longer has `## Status`, replace with: "Write the review verdict into the file's content (the verdict is expressed in the summary and blocking issues, not in a separate status field)."

Also, fix the duplicate step "4." numbering in `## Review process` (lines 44-45): The current file has two consecutive "4." steps. Change the second "4." to "5.":
```
4. Perform the review checks.
5. Write the review to `.specs/{{SPRINT}}/<feature>/spec-critic.md` using the template.
```

Also, update the last rule in `## Rules` (line 56): Change "update the verdict" to "update the review summary text to reflect the new verdict (verdict is expressed in coordination.md, not as a separate status field in the review file)."

After all the above changes, the final `## Review process` steps must be numbered sequentially as follows:
```
1. Load the template at `docs/templates/review-report-template.md`.
2. Read the spec file at `.specs/{{SPRINT}}/<feature>/spec.md`.
3. Search the wiki for relevant context using wiki search tools.
4. Perform the review checks.
5. Write the review to `.specs/{{SPRINT}}/<feature>/spec-critic.md` using the template.
6. Write the review verdict into the file's content (the verdict is expressed in the summary and blocking issues, not in a separate status field).
7. The spec's status is tracked in coordination.md. Do NOT modify the spec file's status field.
8. List every issue as a `- [ ]` checklist item under the appropriate section.
```

---

### File 5: `.opencode/agents/implementation-contract-author.md` (MODIFY — Add coordination.md update step)

Append after the `## Self-validation before submitting` section (after line 93):

```
## After writing

After completing the implementation contract and passing self-validation:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## implementation-contract-author` section (exact heading match).
2. Update the following fields in `## implementation-contract-author`:
   - `**Status**`: `completed`
   - `**Summary**`: 2–5 lines describing what was done.
   - `**Artifacts**`: `- .specs/{{SPRINT}}/<feature>/implementation-contract.md`
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: list any blockers, or "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.
```

Also, update line 26: `Set the contract's `## Status` to `Draft` (allowed values: `Draft`, `In Review`, `Accepted`). It will be updated to `Accepted` after review. No separate `proposed/` directory exists — the status is tracked in the file.`

Replace with: "The implementation contract follows the template at `docs/templates/implementation-contract-template.md` which no longer contains a `## Status` section (status is tracked in coordination.md)."

---

### File 6: `.opencode/agents/implementation-contract-critic.md` (MODIFY — Add coordination.md update step)

Append after the `## Rules` section (after line 61):

```
## After writing

After writing the review artifact and before reporting completion:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## implementation-contract-critic` section (exact heading match).
2. Update the following fields in `## implementation-contract-critic`:
   - `**Status**`: `completed` if accepted, `rejected` if rejected, `blocked` if blocked.
   - `**Summary**`: 2–5 lines describing review outcome.
   - `**Artifacts**`: `- .specs/{{SPRINT}}/<feature>/implementation-contract-critic.md`
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: copy the blocking issues checklist from the review artifact's `## Blocking issues` section. If none, write "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.

Note: Do NOT update the contract file's `## Status`. The review verdict is expressed exclusively in coordination.md.
```

Also, update steps 6 and 7 in `## Review process` (lines 51-52):
- Step 6 (line 51): `Set the review file's `## Status` to one of: `Accepted`, `Accepted with warnings`, `Rejected`.` — Remove this step.
- Step 7 (line 52): `Update the contract file's `## Status` to `In Review`.` — Replace with: "The contract's status is tracked in coordination.md. Do NOT modify the contract file's status field."

Also, update the last rule in `## Rules` (line 60): Change "update the verdict" to "update the review summary text to reflect the new verdict (verdict is expressed in coordination.md, not as a separate status field in the review file)."

---

### File 7: `.opencode/agents/code-implementer.md` (MODIFY — Add coordination.md update step)

Append after the `## If blocked` section (after line 153):

```
## After writing

After all implementation steps are complete and done criteria are verified, before reporting completion:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## code-implementer` section (exact heading match).
2. Update the following fields in `## code-implementer`:
   - `**Status**`: `completed`
   - `**Summary**`: 2–5 lines describing what was implemented.
   - `**Artifacts**`: list of files created or modified.
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: list any blockers, or "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.
```

Also, update `## Before editing` step 1 (line 25): Currently says:

`Read the accepted implementation contract at `.specs/{{SPRINT}}/<feature>/implementation-contract.md` — confirm `## Status` is `Accepted` and `## Approval` is filled (human-approved).`

Replace with:

`Read the accepted implementation contract at `.specs/{{SPRINT}}/<feature>/implementation-contract.md` — confirm the `## Approval` section is filled (human-approved). Also read `.specs/{{SPRINT}}/<feature>/coordination.md` `## Human Validation` section to confirm **Status** is `approved`.`

Also, update `## Before editing` step 2 (line 27): Currently says:

`Read the implementation-contract-critic review at `.specs/{{SPRINT}}/<feature>/contract-critic.md` — confirm the verdict is `Accepted` or `Accepted with warnings` and no unchecked blocking issues remain.`

Replace with:

`Read the `## implementation-contract-critic` section from `.specs/{{SPRINT}}/<feature>/coordination.md` — confirm **Status** is `completed` (not `rejected`) and no unchecked `- [ ]` items remain under **Blocking issues**. Do NOT read the full artifact file (implementation-contract-critic.md) for status or blocking-issue information.`

Also fix the inconsistent file path in the old step 2: `contract-critic.md` must become `implementation-contract-critic.md` (the standard naming convention).

---

### File 8: `.opencode/agents/code-reviewer.md` (MODIFY — Add coordination.md update step)

Append after the `## Rules` section (after line 83):

```
## After writing

After writing the review artifact and before reporting completion:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## code-reviewer` section (exact heading match).
2. Update the following fields in `## code-reviewer`:
   - `**Status**`: `completed` if accepted, `rejected` if rejected, `blocked` if blocked.
   - `**Summary**`: 2–5 lines describing review outcome.
   - `**Artifacts**`: `- .specs/{{SPRINT}}/<feature>/code-review.md`
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: copy the blocking issues checklist from code-review.md `## Blocking issues`. If none, write "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.

Note: Do NOT set the review file's `## Status`. The verdict is expressed exclusively in coordination.md.
```

Also, update step 10 in `## Review process` (line 75): `Set the review file's `## Status` to one of: `Accepted`, `Accepted with warnings`, `Rejected`.` — Remove this step.

Also, update the last rule in `## Rules` (line 82): Change "update the verdict" to "update the review summary text to reflect the new verdict (verdict is expressed in coordination.md, not as a separate status field in the review file)."

---

### File 9: `.opencode/agents/adr-agent.md` (MODIFY — Add coordination.md update step)

Append after the `## Rules` section (after line 45):

```
## After writing

After completing ADR work and before reporting completion:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## adr-agent` section (exact heading match).
2. Update the following fields in `## adr-agent`:
   - `**Status**`: `completed` (or `blocked` if blocked).
   - `**Summary**`: 2–5 lines describing what was done.
   - `**Artifacts**`: list of ADR files created or modified, or "none".
   - `**Decisions needed**`: description of ADR decisions identified, or "none".
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: list any blockers, or "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.
```

---

### File 10: `.opencode/agents/constitution-agent.md` (MODIFY — Add coordination.md update step)

Append after the `## Rules` section (after line 39):

```
## After writing

After completing constitution work and before reporting completion:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## constitution-agent` section (exact heading match).
2. Update the following fields in `## constitution-agent`:
   - `**Status**`: `completed` (or `blocked` if blocked).
   - `**Summary**`: 2–5 lines describing what was done.
   - `**Artifacts**`: list of constitution files created or modified, or "none".
   - `**Changes needed**`: description of constitution changes proposed, or "none".
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: list any blockers, or "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.
```

---

### File 11: `.opencode/agents/wiki-agent.md` (MODIFY — Add coordination.md update step)

Append after the `## Rules` section (after line 71):

```
## After writing

After completing wiki updates and before reporting completion:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## wiki-agent` section (exact heading match).
2. Update the following fields in `## wiki-agent`:
   - `**Status**`: `completed` (or `blocked` if blocked).
   - `**Summary**`: 2–5 lines describing what was done.
   - `**Artifacts**`: list of wiki files created or modified, or "none".
   - `**Changes made**`: description of wiki updates, or "none".
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: list any blockers, or "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.
```

---

### File 12: `.opencode/agents/governance-reviewer.md` (MODIFY — Add coordination.md update step)

Append after the `## Rules` section (after line 59):

```
## After writing

After writing the governance review and before reporting completion:

1. **Write coordination.md update** — Open `.specs/{{SPRINT}}/<feature>/coordination.md` and locate the `## governance-reviewer` section (exact heading match).
2. Update the following fields in `## governance-reviewer`:
   - `**Status**`: `completed` if accepted, `rejected` if rejected, `blocked` if blocked.
   - `**Summary**`: 2–5 lines describing review outcome.
   - `**Artifacts**`: `- .specs/{{SPRINT}}/<feature>/governance-review.md`
   - `**Questions for human**`: list any questions, or "none".
   - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
   - `**Blocking issues**`: copy the blocking issues checklist from governance-review.md `## Blocking issues`. If none, write "none".
3. Do NOT modify other sections.
4. Do NOT modify the `## Orchestrator` section.
5. Append rather than overwrite previous loop history.
6. If coordination.md does not exist, escalate.

Note: Do NOT set the review file's `## Status`. The verdict is expressed exclusively in coordination.md.
```

Also, update step 12 in `## Review process` (line 51): `Set the review file's `## Status` to one of: `Accepted`, `Accepted with warnings`, `Rejected`.` — Remove this step.

Also, update the last rule in `## Rules` (line 58): Change "update the verdict" to "update the review summary text to reflect the new verdict (verdict is expressed in coordination.md, not as a separate status field in the review file)."

---

### File 13: `docs/templates/spec-template.md` (MODIFY — Remove `## Status` section)

Remove lines 3–8 (the `## Status` section including blank line after):

```
## Status

`Draft`

Allowed values: `Draft`, `In Review`, `Accepted`
```

After removal, the file must start with:

```
# SPEC-YYYY-NNNN - Title

## Approval
```

---

### File 14: `docs/templates/review-report-template.md` (MODIFY — Remove `## Status` section)

Remove lines 3–10 (the `## Status` section including the blank line and quote line after):

```
## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.
```

After removal, the file must start with:

```
# [Spec | Implementation Contract] Review — <feature-name>

## Blocking issues
```

---

### File 15: `docs/templates/implementation-contract-template.md` (MODIFY — Remove `## Status` section)

Remove lines 3–8 (the `## Status` section including blank line after):

```
## Status

`Draft`

Allowed values: `Draft`, `In Review`, `Accepted`
```

After removal, the file must start with:

```
# IMPL-YYYY-NNNN - Title

## Approval
```

---

### File 16: `docs/templates/governance-review-template.md` (MODIFY — Remove `## Status` section)

Remove lines 3–10 (the `## Status` section including the blank line and quote line after):

```
## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.
```

After removal, the file must start with:

```
# Governance Review — <feature-name>

## Cross-document coherence
```

---

### File 17: `AGENTS.md` (MODIFY — Update workflow gates reference)

Add a new bullet under `## Document roles` (after line 32, before `## Escalation`):

```
- Coordination files: orchestrator decision-making state for a feature workflow.
```

Add a new paragraph at the end of the `## Non-negotiable rules` section (after line 23):

```
- The orchestrator MUST read only coordination.md for workflow decisions (not full artifact files for status or blocking-issue information).
```

## Dependencies

1. **The `question` tool** — Must be available to the orchestrator as a synchronous tool for asking the human questions. The spec treats this as an existing capability; no changes are needed to the tool itself.
2. **No new dependencies** — This implementation uses only existing agent file read/write capabilities. No new npm packages, libraries, or system dependencies.
3. **No code changes** — This is purely a meta-workflow change (agent prompts, templates, governance docs). No source code, build files, or test files are modified.

## Verification criteria

| ID | Criteria | Verification method |
|---|---|---|
| VC-01 | `docs/templates/coordination-template.md` exists and defines all 11 required sub-agent sections plus Orchestrator and Human Validation sections, in the correct order, with all required fields including `**Warnings**` in every sub-agent section | Inspect file |
| VC-02 | Each `*.md` file in `.opencode/agents/` (except `orchestrator.md` and `scout.md`) has a coordination.md update section appended, and each "After writing" section includes `**Warnings**` in the field list | Grep each agent file for "coordination.md" and "After writing" (or equivalent) |
| VC-03 | `orchestrator.md` workflow section: diagram shows coordination.md creation at start, all gates reference coordination.md sections, no "read full artifact" instructions remain | Read the full `## Required workflow` and `## Workflow details` sections |
| VC-04 | `orchestrator.md` hard rules includes "Never read full artifact files for status or blocking-issue information" | Read `## Hard rules` section |
| VC-05 | `spec-template.md` has no `## Status` section | Inspect file — lines 3-8 removed |
| VC-06 | `review-report-template.md` has no `## Status` section | Inspect file — lines 3-10 removed |
| VC-07 | `implementation-contract-template.md` has no `## Status` section | Inspect file — lines 3-8 removed |
| VC-08 | `governance-review-template.md` has no `## Status` section | Inspect file — lines 3-10 removed |
| VC-09 | `AGENTS.md` includes coordination files role and non-negotiable rule about coordination.md | Inspect file |
| VC-10 | `spec-author.md` no longer instructs setting `## Status` in spec file | Grep for "Set the spec's \`## Status\`" — must not exist |
| VC-11 | `spec-critic.md` no longer instructs setting `## Status` in review or spec files | Grep for "Set the review file's \`## Status\`" and "Update the spec file's \`## Status\`" — must not exist |
| VC-12 | `implementation-contract-author.md` no longer instructs setting `## Status` in contract | Grep for "Set the contract's \`## Status\`" — must not exist |
| VC-13 | `implementation-contract-critic.md` no longer instructs setting `## Status` in contract or review | Grep for "Set the review file's \`## Status\`" and "Update the contract file's \`## Status\`" — must not exist |
| VC-14 | `code-reviewer.md` no longer instructs setting `## Status` in review | Grep for "Set the review file's \`## Status\`" — must not exist |
| VC-15 | `governance-reviewer.md` no longer instructs setting `## Status` in review | Grep for "Set the review file's \`## Status\`" — must not exist |
| VC-16 | `code-implementer.md` reads coordination.md for Human Validation status instead of checking `## Status` in the contract file | Verify step 1 in `## Before editing` |
| VC-17 | Every sub-agent's "After writing" section follows the same pattern: read coordination.md, find section by exact heading, update fields, do not modify other sections, append history, escalate if missing | Read each agent's appended section |
| VC-18 | Orchestrator's workflow details for steps 3, 5, 8, 11 contain re-review and loop-back logic that references coordination.md sections, not full artifact files | Read each gate description in orchestrator.md |
| VC-19 | `code-implementer.md` step 2 in `## Before editing` reads coordination.md for the implementation-contract-critic verdict instead of the old artifact file | Verify step 2 in `## Before editing` — must reference `coordination.md` and `implementation-contract-critic` section, not `contract-critic.md` |
| VC-20 | `spec-critic.md` has no duplicate "4." numbering in `## Review process` steps | Inspect spec-critic.md `## Review process` — steps are numbered 1-8 sequentially |
| VC-21 | `spec-critic.md` `## Rules` last rule says "update the review summary text" not "update the verdict" | Grep spec-critic.md for "update the verdict" — must not exist |
| VC-22 | `implementation-contract-critic.md` `## Rules` last rule says "update the review summary text" not "update the verdict" | Grep implementation-contract-critic.md for "update the verdict" — must not exist |
| VC-23 | `code-reviewer.md` `## Rules` last rule says "update the review summary text" not "update the verdict" | Grep code-reviewer.md for "update the verdict" — must not exist |
| VC-24 | `governance-reviewer.md` `## Rules` last rule says "update the review summary text" not "update the verdict" | Grep governance-reviewer.md for "update the verdict" — must not exist |
| VC-25 | `coordination-template.md` constraints include the loop-back exception about orchestrator setting sub-agent status | Read the constraints section in coordination-template.md |
| VC-26 | `orchestrator.md` workflow step 3 references the loop-back status exception note | Read the spec-critic gate section in orchestrator.md |
| VC-27 | Orchestrator workflow includes the "Delegation invariant" instruction that tells sub-agents to update coordination.md after completing their work | Read the orchestrator workflow section in orchestrator.md — must contain "After completing your work, read coordination.md, find your section" or equivalent |
| VC-28 | Gate mechanism in orchestrator workflow clarifies that `**Warnings**` are non-blocking and do NOT affect gate decisions | Read the gate mechanism section or orchestrator workflow section for explicit statement that Warnings are informational only |

## Risks and unknowns

1. **Sub-agent guidance consistency risk** — Each sub-agent's `## After writing` section describes the same coordination.md update pattern independently. If a pattern change is needed later (e.g., a new field is added), all 11 sub-agent files must be updated. **Mitigation:** The spec defines a stable, minimal set of fields. The template serves as the structural reference.
2. **spec-critic.md step 5/6 rewrite ambiguity** — The spec-critic agent's review process currently tells it to set `## Status` on both the review file and the spec file. Since the review-report-template's `## Status` is removed, the critic must express its verdict differently. The contract specifies replacing these steps with "write verdict in summary/blocking issues" and "status tracked in coordination.md." The replacement text is unambiguous: step 5 is replaced, not removed. The final step numbering (1-8 after duplicate-4 fix) is explicitly specified.
3. **Risk of lingering `## Status` references** — There may be existing artifacts (spec files in other feature directories) that still use `## Status`. Those are NOT modified by this implementation (they are pre-existing documents). The change is only to the templates. Existing files retain their status fields until they are naturally updated by future workflow runs.
4. **Risk of the orchestrator falling back to old patterns** — The orchestrator's existing behavior of reading full artifacts is deeply embedded. The rewritten workflow must be comprehensive enough that the orchestrator has no reason to fall back. The hard rules addition provides a safety net.
5. **`scout.md` agent** — The scout is not listed among the agents that need coordination.md updates. The scout is a reconnaissance agent that does not produce workflow artifacts in `.specs/{{SPRINT}}/<feature>/`. No coordination.md update is needed for the scout. This is by design.
6. **`[NEEDS CLARIFICATION]` — Merge conflict strategy for parallel agents** — The spec notes that adr-agent and constitution-agent run in parallel "conceptually" but says the orchestrator "must coordinate the writes sequentially (or use a merge strategy)." This contract assumes sequential writes (orchestrator waits for one before starting the other). If this assumption is wrong, the coordination.md update pattern would need a merge step. The spec-critic's R-03 recommends sequential execution, which this contract adopts.
7. **Orchestrator/sub-agent status ownership exception** — The general principle is "sub-agents self-manage their own status," but loop-backs require the orchestrator to reset a sub-agent's status to "in-progress." This exception is explicitly documented in the coordination-template.md constraints, in the orchestrator workflow loop-back instructions, and in the edge case table. The Code Agent must ensure the orchestrator workflow does not use this exception to bypass the general rule in non-loop scenarios.
8. **No existing `.specs/{{SPRINT}}/<feature>/coordination.md` files to inspect** — This implementation creates the first coordination-template.md. Coordination.md files will only exist after this implementation is deployed and the orchestrator creates them. No pre-existing coordination.md files need migration.
9. **spec-critic.md's re-review step ambiguity** — The spec-critic currently says "On re-review, update the same review file: mark resolved items with `[x]`, add new issues as `[ ]`, and update the verdict." Since the verdict was previously expressed via `## Status`, and that field is removed, the critic must now express the verdict in the summary and blocking issues. This contract addresses this by modifying the verdict wording in all four critic agents' Rules sections (spec-critic, implementation-contract-critic, code-reviewer, governance-reviewer) to read "update the review summary text."

## Edge cases from spec carried forward

| Case | Expected handling |
|---|---|
| Sub-agent cannot find coordination.md | Escalate to orchestrator. Orchestrator recreates from template. |
| Two sub-agents update simultaneously | Not applicable — sub-agents run sequentially. Orchestrator waits for each. |
| Sub-agent re-invoked (loop-back) | Orchestrator updates sub-agent's section before re-invoking: sets status to "in-progress" (this is the one exception to the sub-agent self-management principle — documented in coordination-template.md constraints), preserves or updates instructions. Sub-agent appends, does not overwrite. |
| Orchestrator needs artifact detail | MAY read artifact for detail (e.g., specific AC wording) but MUST NOT read for status/blocking info. |
| coordination.md grows large over loops | Acceptable — linear growth. Older history serves as audit trail. |
| Human validation rejects | Orchestrator records in `## Human Validation` section. Workflow terminates. |
| Human answer changes scope | Orchestrator records answer, determines whether to restart from clarification or continue. |
| coordination.md missing at read time | Orchestrator recreates from template from memory. |
| coordination.md section invalid/missing for completed agent | Orchestrator treats agent as not yet completed, may re-invoke. |
| Sub-agent fails to update coordination.md | Orchestrator checks coordination.md after completion; finds stale status; asks sub-agent to update before proceeding. |
| coordination.md format corrupted | Orchestrator treats section as error state; asks sub-agent to fix; last resort: fall back to reading artifact file `## Status` (one-time exception, logged). |

## Human Approval

- **Approved by**: Guillaume (human)
- **Date**: 2026-05-30
- **Time**: (session time)
- **Notes**: Approved for implementation. All review issues resolved.
