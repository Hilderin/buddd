# SPEC-013 — Workflow Coordination

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Introduction

This spec introduces a lightweight coordination layer into the feature workflow: a `coordination.md` file per feature that becomes the single source of truth for orchestrator decision-making. It replaces the orchestrator's current practice of reading full artifact files (spec.md, spec-critic.md, implementation-contract.md, etc.) to determine status and next steps.

## Problem

The orchestrator currently drives the workflow by reading full documents at every gate. This creates three problems:

1. **Context bloat** — Each full document (spec.md, spec-critic.md, implementation-contract.md, implementation-contract-critic.md, code-review.md, governance-review.md) must be re-read entirely at each gate, even though the orchestrator only needs two fields: `## Status` and `## Blocking issues`. As the number of review cycles grows (spec-critic rejects → spec-author fixes → spec-critic re-reviews), the orchestrator's accumulated context grows without bound.

2. **Fragile gate logic** — The orchestrator must parse document-specific formats to extract status and blocking issues. Different artifact types use different templates (spec-template.md vs. review-report-template.md), forcing the orchestrator to understand each format's quirks.

3. **No single coordination point** — Questions for the human, blocking issues across multiple artifacts, and the current workflow step are scattered across files. The orchestrator must piece together the workflow state from multiple sources.

## Goals

- Define a `coordination.md` file per feature at `docs/specs/<feature>/coordination.md`.
- Define a coordination template at `docs/templates/coordination-template.md`.
- Make `coordination.md` the **only** file the orchestrator reads for workflow decisions.
- Ensure every sub-agent updates their section in `coordination.md` after completing their work.
- Preserve all existing artifacts — sub-agents still write spec.md, spec-critic.md, etc. for record-keeping.
- Keep all existing workflow steps and gates; only the coordination mechanism changes.
- Ensure the orchestrator NEVER reads full artifact files for status or blocking-issue information.
- When a sub-agent has a question for the human, the orchestrator asks immediately using the question tool (no batching).

## Non-goals

- Do NOT remove or redesign any workflow step.
- Do NOT remove or change workflow gates.
- Do NOT change what artifacts sub-agents produce.
- Do NOT change the order of the workflow stages.
- Do NOT redesign the agent system architecture.
- Do NOT change authority order (constitution > specs > ADRs > wiki > code conventions).
- Do NOT change the structure of artifact templates beyond the necessary removal of the `## Status` field (see Detailed design).
- Do NOT introduce new agents or remove existing agents.
- Do NOT automate the creation of coordination.md content beyond the orchestrator's initial template.

## Actors

| Actor | Role |
|---|---|
| Orchestrator | Creates coordination.md at workflow start. Reads only coordination.md for decisions. Delegates to sub-agents. Routes questions to human. Records human responses in coordination.md. |
| spec-author | Writes spec.md. Updates `## spec-author` section in coordination.md. |
| spec-critic | Writes spec-critic.md. Updates `## spec-critic` section in coordination.md. |
| implementation-contract-author | Writes implementation-contract.md. Updates `## implementation-contract-author` section in coordination.md. |
| implementation-contract-critic | Writes implementation-contract-critic.md. Updates `## implementation-contract-critic` section in coordination.md. |
| code-implementer | Implements code per the accepted contract. Updates `## code-implementer` section in coordination.md. |
| code-reviewer | Writes code-review.md. Updates `## code-reviewer` section in coordination.md. |
| adr-agent | Proposes/create ADRs as needed. Updates `## adr-agent` section in coordination.md. |
| constitution-agent | Proposes constitution changes as needed. Updates `## constitution-agent` section in coordination.md. |
| wiki-agent | Updates wiki as needed. Updates `## wiki-agent` section in coordination.md. |
| governance-reviewer | Writes governance-review.md. Updates `## governance-reviewer` section in coordination.md. |
| Human | Validates spec + implementation contract via the Human Validation section. Answers questions raised by sub-agents. |

## User-visible behavior

This is a meta-spec about the workflow itself. The primary "user" is the orchestrator agent and the human operator.

- The orchestrator creates `docs/specs/<feature>/coordination.md` at the start of the workflow.
- After each sub-agent finishes its work, the sub-agent updates the coordination.md with a short summary, status, blocking issues, and questions.
- The orchestrator reads coordination.md after each sub-agent finishes and decides the next step based solely on its contents.
- The human sees a consolidated view of workflow progress through coordination.md.
- When a sub-agent has a question, the orchestrator asks the human immediately and records the answer in coordination.md.

## User stories

### Story 1 — Orchestrator reads coordination.md instead of full documents (Priority: P1)

**Given** the orchestrator has delegated to `spec-critic` and the critic has finished its work
**When** the orchestrator checks the workflow state
**Then** the orchestrator reads `docs/specs/<feature>/coordination.md` ONLY (not the full spec-critic.md or spec.md files)
**And** the orchestrator determines the next action from the `## spec-critic` section's `**Status**` and `**Blocking issues**` fields via the gate mechanism

### Story 2 — Sub-agent updates coordination.md after work (Priority: P1)

**Given** a sub-agent (e.g., spec-author) has completed writing its artifact (e.g., spec.md)
**When** the sub-agent finishes
**Then** the sub-agent reads `docs/specs/<feature>/coordination.md`
**And** updates its section (e.g., `## spec-author`) with:
- `**Status**`: completed / in-progress / blocked
- `**Summary**`: 2–5 lines of what was done
- `**Artifacts**`: list of files created or modified
- `**Questions for human**`: any questions (or "none")
- `**Warnings**`: non-blocking concerns (or "none")
- `**Blocking issues**`: checklist (or "none")

### Story 3 — Orchestrator loops back on rejection (Priority: P1)

**Given** `## spec-critic` section shows `**Status**`: rejected or has unchecked `**Blocking issues**`
**When** the orchestrator reads coordination.md
**Then** the orchestrator loops back to `spec-author` with the blocking issues
**And** the orchestrator updates the coordination.md `## Orchestrator` section to reflect the loop

### Story 4 — Human asked immediately when a question arises (Priority: P2)

**Given** a sub-agent's section in coordination.md contains a `**Questions for human**` entry
**When** the orchestrator reads coordination.md
**Then** the orchestrator uses the `question` tool immediately to ask the human
**And** records the human's answer in the same section under the question
**And** proceeds based on the answer

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | coordination.md is created by the orchestrator at `docs/specs/<feature>/coordination.md` at the start of the workflow. | Inspect file existence after orchestrator begins a new feature. |
| AC-002 | coordination.md follows the structure defined in `docs/templates/coordination-template.md`. | Compare any coordination.md against the template for compliance. |
| AC-003 | Each sub-agent section in coordination.md contains `**Status**`, `**Summary**`, `**Artifacts**`, `**Questions for human**`, `**Warnings**`, and `**Blocking issues**`. | Inspect each section in a real coordination.md. |
| AC-004 | After completing its artifact, every sub-agent updates its section in coordination.md before reporting completion. | A test script can simulate each sub-agent writing its section to coordination.md and verify the section is correctly populated. |
| AC-005 | The orchestrator NEVER reads spec.md, spec-critic.md, implementation-contract.md, implementation-contract-critic.md, code-review.md, or governance-review.md for status or blocking-issue information. | A code review of the orchestrator prompt confirms that read operations reference coordination.md sections, not full artifact file paths. |
| AC-006 | The orchestrator checks coordination.md after every sub-agent finishes to decide the next step. | Trace orchestrator decisions: each decision must follow a read of coordination.md. |
| AC-007 | When a sub-agent's section has `**Status**`: rejected, the orchestrator loops back to the appropriate previous agent. | Set up a spec-critic review that rejects. Verify orchestrator re-invokes spec-author. |
| AC-008 | When a sub-agent's section has unchecked `**Blocking issues**`, the orchestrator loops back to the appropriate previous agent. | Set up a spec-critic review with a blocking issue. Verify orchestrator re-invokes spec-author with that issue. |
| AC-009 | When a sub-agent's section contains a `**Questions for human**` entry, the orchestrator asks the human immediately using the question tool. | Add a question to a sub-agent's section. Verify orchestrator invokes the question tool. |
| AC-010 | The human's answer to a question is recorded in the coordination.md under the relevant sub-agent's section. | After the human answers, verify coordination.md contains the answer documented alongside the question. |
| AC-011 | The orchestrator updates `## Orchestrator` section in coordination.md to reflect loops, status changes, and next steps. | After a loop-back, inspect `## Orchestrator` section for an updated current step entry. |
| AC-012 | The `## Human Validation` section is updated by the orchestrator when the human validates the spec and contract. | After human validation, inspect coordination.md `## Human Validation` section for status, approver, date/time. |
| AC-013 | The coordination-template.md file exists at `docs/templates/coordination-template.md` and defines all required sections. | Verify file exists and contains sections matching the required participant list. |
| AC-014 | When looping back, the orchestrator re-populates the target sub-agent's section with updated instructions and sets its status to "in-progress" (or clears previous blocking issues that are being addressed). | After a loop, inspect the target agent's section and `## Orchestrator` for evidence of the loop. |
| AC-015 | The `## Status` field is removed from all artifact templates (spec-template.md, review-report-template.md, implementation-contract-template.md, governance-review-template.md). Status lives only in coordination.md. | Inspect each template file — the `## Status` section must be absent. |

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | Orchestrator reads at most 1 file (coordination.md) per workflow decision point, instead of 1–4 full artifact files. |
| SC-002 | After a sub-agent completes, the orchestrator can determine the next action within 3 reads (one to read coordination.md, plus any reads needed to inspect specific artifact content if the orchestrator chooses). |
| SC-003 | No `[NEEDS CLARIFICATION]` markers introduced for any coordination-related question — all design decisions are addressed in this spec. |

## Detailed design

### coordination.md structure

```
# Workflow Coordination: <feature-name>

## Orchestrator

**Feature**: <kebab-case feature name>
**Status**: in-progress | completed | blocked
**Current step**: <current workflow step name>
**Initial instructions**: <the original feature request / human intent>
**Notes**: <orchestrator notes, loop history, decisions>

## spec-author

**Status**: pending | in-progress | completed | blocked
**Summary**:
<2–5 lines describing what was done or what is needed>
**Artifacts**:
- `docs/specs/<feature>/spec.md`
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## spec-critic

**Status**: pending | in-progress | completed | rejected | blocked
**Summary**:
<2–5 lines describing review outcome>
**Artifacts**:
- `docs/specs/<feature>/spec-critic.md`
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## implementation-contract-author

**Status**: pending | in-progress | completed | blocked
**Summary**:
<2–5 lines>
**Artifacts**:
- `docs/specs/<feature>/implementation-contract.md`
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## implementation-contract-critic

**Status**: pending | in-progress | completed | rejected | blocked
**Summary**:
<2–5 lines>
**Artifacts**:
- `docs/specs/<feature>/implementation-contract-critic.md`
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## Human Validation

**Status**: pending | approved | rejected
**Approver**: <human identity>
**Date**: <date>
**Time**: <time>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Notes**: <any human feedback or conditions>

## code-implementer

**Status**: pending | in-progress | completed | blocked
**Summary**:
<2–5 lines describing what was implemented>
**Artifacts**:
- <list of files created or modified>
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## code-reviewer

**Status**: pending | in-progress | completed | rejected | blocked
**Summary**:
<2–5 lines describing review outcome>
**Artifacts**:
- `docs/specs/<feature>/code-review.md`
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## adr-agent

**Status**: pending | in-progress | completed | blocked
**Summary**:
<2–5 lines>
**Artifacts**:
- <list of ADR files created or modified, or "none">
**Decisions needed**:
<none, or a description of ADR decisions identified>
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## constitution-agent

**Status**: pending | in-progress | completed | blocked
**Summary**:
<2–5 lines>
**Artifacts**:
- <list of constitution files, or "none">
**Changes needed**:
<none, or a description of constitution changes proposed>
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## wiki-agent

**Status**: pending | in-progress | completed | blocked
**Summary**:
<2–5 lines>
**Artifacts**:
- <list of wiki files created or modified, or "none">
**Changes made**:
<none, or a description of wiki updates>
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## governance-reviewer

**Status**: pending | in-progress | completed | rejected | blocked
**Summary**:
<2–5 lines>
**Artifacts**:
- `docs/specs/<feature>/governance-review.md`
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>
```

### Template design

The template file at `docs/templates/coordination-template.md` mirrors the structure above with placeholder values. Each field shows the accepted values or example content. The template serves as:

- A structural reference for the orchestrator when creating coordination.md.
- A field reference for sub-agents when updating their section.
- A validation guide for the governance-reviewer when checking coordination.md compliance.

The template file `docs/templates/coordination-template.md` must be created as part of this spec's implementation.

### Removal of `## Status` from artifact templates

The `## Status` field is removed from all individual artifact templates. Status information now lives **exclusively** in `coordination.md`. This is a deliberate consolidation:

- **Before:** Each artifact (spec.md, spec-critic.md, implementation-contract.md, implementation-contract-critic.md, code-review.md, governance-review.md) carried its own `## Status` field, and the orchestrator read those fields to determine workflow state.
- **After:** `coordination.md` is the sole source of status information. Artifact files remain as record-keeping documents without status fields.

**Affected templates:**
- `docs/templates/spec-template.md` — remove `## Status` section (lines 3-7)
- `docs/templates/review-report-template.md` — remove `## Status` section (lines 3-9)
- `docs/templates/implementation-contract-template.md` — remove `## Status` section (lines 3-7)
- `docs/templates/governance-review-template.md` — remove `## Status` section (lines 3-9)

**Rationale:** Status is workflow metadata that crosses agent boundaries. It belongs in the coordination layer, not in individual artifacts. This eliminates the problem of stale or contradictory status values across multiple files and ensures the orchestrator has a single source of truth.

### Status ownership

Who updates which status fields in coordination.md:

- **Sub-agents** update the `**Status**` field in **their own section** of coordination.md. For example:
  - The spec-author updates `## spec-author` → `**Status**`
  - The spec-critic updates `## spec-critic` → `**Status**`
  - The code-implementer updates `## code-implementer` → `**Status**`
  - etc.

- **Orchestrator** updates the following status fields:
  - `## Orchestrator` → `**Status**` — the global/overall feature status (`in-progress | completed | blocked`). This reflects the high-level state of the entire feature workflow.
  - `## Human Validation` → `**Status**` — transitions from `pending` to `approved` or `rejected` based on human sign-off.

**Exception — Loop handling:** During loop-backs (when a critic rejects and the orchestrator re-invokes a previous agent), the orchestrator may reset the target sub-agent's `**Status**` to `in-progress` in coordination.md. This is a temporary reset to reflect that the agent's work needs to be redone. The sub-agent ultimately controls its own final completion status when it finishes its work. This exception is documented to avoid contradiction with the Loop handling section.

### Orchestrator behavior

The orchestrator's workflow loop changes from "read full artifact → extract status + blocking → decide" to "read coordination.md → check section → decide".

#### Delegation invariant

When delegating to any sub-agent, the orchestrator MUST include in its instructions: "After completing your work, read coordination.md, find your section (`## <agent-name>`), and update it with your status, summary, artifacts, questions, warnings, and blocking issues."

#### Step-by-step orchestrator behavior

1. **Clarification** — Normal clarification with the human. After clarification, the orchestrator:
   - Creates `docs/specs/<feature>/` directory (if not exists).
   - Creates `docs/specs/<feature>/coordination.md` using `docs/templates/coordination-template.md`.
   - Fills in `## Orchestrator` section with feature name, overall status ("in-progress"), current step ("clarification-complete"), and initial instructions.
   - Fills all other sections with status "pending" and empty content.

2. **Spec-author** — Orchestrator delegates with intent, constraints, and scout findings. After the sub-agent reports completion:
   - Reads coordination.md `## spec-author` section.
   - Checks `**Status**`: if "blocked", ask human or resolve; otherwise proceed.
   - Checks `**Questions for human**`: if present, ask human immediately and record answer.
   - Updates `## Orchestrator` with current step.
   - Proceeds to spec-critic.

3. **Spec-critic** — Orchestrator delegates. After completion:
   - Reads coordination.md `## spec-critic` section.
   - Checks `**Status**`: if "rejected" → loop back to spec-author (step 2).
   - Checks `**Blocking issues**`: if any unchecked `- [ ]` items → loop back to spec-author (step 2).
   - Checks `**Questions for human**`: if present, ask immediately.
    - (No artifact status update needed — status lives only in coordination.md.)
   - Update `## Orchestrator` current step.
   - Proceed to implementation-contract-author.

> **Two-stage acceptance (intentional):** The workflow has two acceptance stages, both tracked exclusively in coordination.md. The first stage is **technical acceptance** — reflected in `## spec-critic` section `**Status**` becoming `completed` (not rejected). The second stage is **human validation** — `## Human Validation` section `**Status**` transitions from `pending` to `approved` or `rejected`. This two-stage model allows the workflow to proceed to implementation-contract-author without blocking on human availability, while still requiring human sign-off before code implementation. No status fields exist in artifact files — all workflow status lives in coordination.md.

4. **Implementation-contract-author** — Same pattern as spec-author (step 2).

5. **Implementation-contract-critic** — Same pattern as spec-critic (step 3), but loops back to either implementation-contract-author or spec-author as appropriate. **Important re-review rule:** If the loop targets spec-author (i.e., the critic identified a spec-level issue requiring spec.md changes), then after spec-author completes the fix, **spec-critic MUST re-review the modified spec** before the workflow proceeds to implementation-contract-author again. The implementation-contract-critic does NOT handle spec-level re-review. This ensures spec-level issues are validated by the dedicated spec-critic agent.

6. **Human validation** — Orchestrator reads coordination.md:
   - `## Human Validation` section status.
   - Summaries from `## spec-author` and `## implementation-contract-author` sections of coordination.md (not from the full artifact files).
   - Presents to human for approval using the `question` tool with a summary of the spec and implementation contract drawn from coordination.md sections.
    - Records approval in `## Human Validation` section: status (`approved` or `rejected`), approver, date, time, notes.
   - Proceeds to code-implementer.

7. **Code-implementer** — Same pattern as spec-author.

8. **Code-reviewer** — Same pattern as spec-critic, but loops back to code-implementer.

9. **ADR-agent + Constitution-agent (parallel)** — Orchestrator delegates to both simultaneously (or sequentially if tool limitations require). After each reports:
   - Reads respective section.
   - Checks for questions, blocking issues.
   - Never accepts constitution change without explicit human approval.

10. **Wiki-agent** — Same pattern as spec-author.

11. **Governance-reviewer** — Same pattern as spec-critic, loops back to appropriate agent.

12. **Done** — Orchestrator:
    - Updates `## Orchestrator` section `**Status**` to `completed`.
    - Reports completion to human with a summary of what was implemented and links to all artifacts.

#### Loop handling

When looping back (e.g., spec-critic rejects), the orchestrator:

1. Updates `## Orchestrator` section with the current step noting the loop.
2. Updates the target agent's section (e.g., `## spec-author`) — sets status to "in-progress" and provides context about what needs fixing (referencing blocking issues from the critic section).
3. Re-invokes the agent.
4. After the agent finishes, re-runs the critic that triggered the loop.

### Sub-agent behavior

Every sub-agent, at the end of their work, must:

1. **Write their normal artifact** (spec.md, spec-critic.md, etc.) as they do today.
2. **Read coordination.md** from `docs/specs/<feature>/coordination.md`.
3. **Identify their section** by locating the exact heading `## <agent-name>` (case-sensitive, exact match) in coordination.md. For example, the spec-author looks for `## spec-author`, the spec-critic looks for `## spec-critic`. Sub-agents must NOT use fuzzy matching, line numbers, or partial heading searches — the heading must match exactly.
4. **Update their section** with:
   - `**Status**`: one of: completed, in-progress, blocked, rejected (critics only), approved (human validation only).
   - `**Summary**`: 2–5 lines describing what was done.
   - `**Artifacts**`: list of files created or modified.
    - `**Questions for human**`: if any questions arose that cannot be answered from project evidence, list them here. If none, write "none".
    - `**Warnings**`: non-blocking concerns, suggestions, or minor issues that do NOT block the workflow. If none, write "none".
    - `**Blocking issues**`: if any issues prevent the workflow from proceeding, use `- [ ]` checklist format. If none, write "none".
5. **Write coordination.md** back to disk.

**Important rules**:
- Sub-agents must NOT change other agents' sections.
- Sub-agents must NOT remove or restructure the file.
- Sub-agents must NOT change the `## Orchestrator` section.
- Sub-agents should add new content at the end of their section, preserving any previous content or loop history.

### Gate mechanism

The orchestrator checks the following fields in the relevant sub-agent's section. Note: `**Warnings**` are non-blocking and do NOT affect gate decisions — they are informational only.

1. **`**Status**`** — If the status is `rejected`, loop back to the appropriate previous agent. If `blocked`, resolve the blocking issue (possibly by asking the human). If `pending`, check whether the step was already completed or needs to be started.

2. **`**Blocking issues**`** — If any unchecked `- [ ]` items exist under the `**Blocking issues**` heading, loop back. Only when all items are checked (`[x]`) does the gate pass. Note: only `- [ ]` items under the `**Blocking issues**` heading are treated as blocking — standard markdown task lists elsewhere in the section do not affect the gate.

This replaces the previous pattern of reading the full review/artifact file and parsing its `## Status` and `## Blocking issues` sections.

### Question mechanism

The `question` tool is an existing synchronous tool available to the orchestrator. It presents a question to the human and returns the human's response immediately. Its API is defined outside this spec; this spec treats it as an available capability.

When a sub-agent lists questions under `**Questions for human**`:

1. The orchestrator reads the question(s) from coordination.md.
2. The orchestrator uses the `question` tool immediately to ask the human (no batching, no waiting for multiple questions).
3. The human's answer is recorded in the same sub-agent's section in coordination.md, formatted as:
   ```
   **Questions for human**:
   - Q: <question>
     A: <human's answer>
   ```
4. If the answer resolves a blocking issue, the orchestrator may also update the `**Blocking issues**` section accordingly.
5. The orchestrator then proceeds based on the answer.

## Workflow diagram

```
Human intent
    │
    ▼
┌─────────────────────────────────────────────────┐
│ Orchestrator                                    │
│ 1. Clarify with human                           │
│ 2. CREATE coordination.md (all sections filled) │
│    → reads/writes ONLY coordination.md          │
└─────────────────────────────────────────────────┘
    │
    ▼
┌──────────────────────┐
│ spec-author          │
│ → writes spec.md     │
│ → updates coor.md    │
└────────┬─────────────┘
    │
    ▼
┌──────────────────────┐
│ spec-critic          │◄──── loop: rejected or
│ → writes spec-cr.md  │      blocking issues
│ → updates coor.md    │
└────────┬─────────────┘
    │ gate: check coordination.md
    │   Status != rejected
    │   Blocking issues == none
    ▼
┌──────────────────────────────────┐
│ implementation-contract-author   │
│ → writes impl-contract.md        │
│ → updates coor.md                │
└────────┬─────────────────────────┘
    │
    ▼
┌──────────────────────────────────┐
│ implementation-contract-critic   │◄── loop: to impl-contract or spec
│ → writes impl-contract-cr.md     │
│ → updates coor.md                │
└────────┬─────────────────────────┘
    │ gate: check coordination.md
    ▼
┌──────────────────────┐
│ Human Validation     │
│ → presented via      │
│   coordination.md    │
│ → records approval   │
│   in coordination.md │
└────────┬─────────────┘
    │
    ▼
┌──────────────────────┐
│ code-implementer     │
│ → implements code    │
│ → updates coor.md    │
└────────┬─────────────┘
    │
    ▼
┌──────────────────────┐
│ code-reviewer        │◄──── loop: rejected or
│ → writes code-rev.md │      blocking issues
│ → updates coor.md    │
└────────┬─────────────┘
    │ gate: check coordination.md
    ▼
┌──────────────────────┐   ┌──────────────────────┐
│ adr-agent            │   │ constitution-agent   │
│ → updates coor.md    │   │ → updates coor.md    │
└────────┬─────────────┘   └────────┬─────────────┘
    │ (parallel)                    │
    └───────────────┬───────────────┘
                    ▼
┌──────────────────────┐
│ wiki-agent           │
│ → updates coor.md    │
└────────┬─────────────┘
    │
    ▼
┌──────────────────────────┐
│ governance-reviewer      │◄──── loop: to appropriate agent
│ → writes gov-review.md   │
│ → updates coor.md        │
└────────┬─────────────────┘
    │ gate: check coordination.md
    ▼
┌──────────────────────────────────┐
│ Orchestrator                     │
│ → updates ## Orchestrator Status │
│   to "completed"                 │
│ → reports to human               │
└──────────────────────────────────┘
    │
    ▼
(Done)
```

## Edge cases

| Case | Handling |
|---|---|
| A sub-agent cannot find coordination.md | The sub-agent escalates to the orchestrator. The orchestrator recreates it from the template. |
| Two sub-agents try to update coordination.md simultaneously | Not applicable — sub-agents run sequentially per the workflow. The orchestrator waits for each sub-agent to complete before delegating to the next. The adr-agent and constitution-agent run in parallel conceptually but the orchestrator must coordinate the writes sequentially (or use a merge strategy). |
| A sub-agent is re-invoked (loop-back) | The orchestrator updates the sub-agent's section before re-invoking: sets status to "in-progress", preserves or updates instructions, may carry over context from the critic's blocking issues. The sub-agent appends to their section rather than overwriting. |
| The orchestrator needs to inspect artifact details beyond status/blocking | The orchestrator MAY read an artifact file for detail (e.g., specific AC wording, code snippet) but MUST NOT read it for status or blocking-issue information. This is an explicit exception for when the orchestrator needs detail to answer a human question or to provide context when delegating. |
| coordination.md grows very large over many loops | Acceptable — growth is linear in the number of loops, and the orchestrator only reads the latest status/blocking per section. Older loop history serves as audit trail. |
| Human validation rejects the feature | The orchestrator records the rejection in `## Human Validation` section with notes. The workflow terminates and the orchestrator reports to the human. |
| The human answers a question that changes the scope | The orchestrator records the answer and determines whether to restart the workflow from clarification or continue from the current step. |

## Error cases

| Case | Handling |
|---|---|
| coordination.md is missing when the orchestrator tries to read it | The orchestrator recreates it from the template using the information in the `## Orchestrator` section from memory (or from the human if the orchestrator has reset). |
| coordination.md has an invalid or missing section for a completed agent | The orchestrator treats the agent as not yet completed and may re-invoke it. |
| A sub-agent fails to update coordination.md | The sub-agent reports completion. The orchestrator checks coordination.md and finds the section still shows "pending" or previous status. The orchestrator asks the sub-agent to update coordination.md before proceeding. |
| coordination.md format is corrupted (e.g., missing `**Status**` field) | The orchestrator treats the section as error state and asks the sub-agent to fix it, or falls back to reading the artifact file's `## Status` as a last resort (one-time exception, logged). |
| The human's answer to a question conflicts with an accepted spec | The orchestrator records the answer and evaluates whether a spec amendment is needed. If so, the orchestrator may restart from the spec-author step. |

## Permissions and security

- All agents have read/write access to coordination.md (same as other files in `docs/specs/<feature>/`).
- Sub-agents must NOT modify sections other than their own.
- The orchestrator is the only agent that modifies the `## Orchestrator` section.
- No special permissions beyond standard file access are needed.
- The coordination.md file is a plain Markdown file with no executable content.

## Observability

- The orchestrator should log each time it reads coordination.md and the decision it makes based on that read.
- The orchestrator should log each time a sub-agent updates coordination.md.
- The coordination.md itself serves as an audit trail — every loop, every status change, every human question is recorded.
- If the orchestrator ever falls back to reading a full artifact file (error recovery), it must log a warning.
- The `## Orchestrator` section's `**Current step**` field tracks the workflow position and can be monitored for stalls.

## Out of scope

- Changes to any agent's core capabilities or responsibilities beyond updating coordination.md.
- Changes to the content or structure of spec.md, spec-critic.md, implementation-contract.md, implementation-contract-critic.md, code-review.md, or governance-review.md.
- Automation or CI checks for coordination.md compliance (deferred to future).
- Integration with external project management tools.
- Any changes to the human-facing approval process.

## Assumptions

- The orchestrator runs sub-agents sequentially (parallel execution is an orchestrator implementation detail that does not change the coordination.md update pattern).
- Markdown is the appropriate format for coordination.md — it is human-readable, diffable, and consistent with other project documents.
- Sub-agents can read and write coordination.md using the same file tools they already use.
- The coordination.md file will be small enough (under 100KB in practice) that reading it is cheap.
- The `question` tool exists and is available to the orchestrator as a synchronous tool (this spec references it as an existing capability; its API is defined elsewhere).
- All existing sub-agents use file read/write tools that can target arbitrary paths under `docs/specs/<feature>/`.

## Open questions

None — all design decisions are resolved in this spec. Key resolutions:

- **question tool (B-01):** The `question` tool is an existing synchronous tool available to the orchestrator. Its API is defined outside this spec; this spec references it as an existing capability.
- **Human Validation status (B-02):** Unified to `approved` throughout.
- **Re-review chain (B-03):** When implementation-contract-critic loops to spec-author for spec-level changes, spec-critic MUST re-review the modified spec before proceeding.
- **Two-stage acceptance (C-04):** `Accepted` at the spec-critic gate is intentional — it represents technical acceptance. Human validation provides final approval via the `## Human Validation` section.
- **Status consolidation (C-01):** Status fields removed from all artifact templates. Status lives exclusively in coordination.md. Orchestrator no longer modifies artifact status fields.

The coordination.md structure, orchestrator behavior, sub-agent behavior, gate mechanism, question mechanism, and all edge/error cases are fully specified.

## Human Approval

- **Approved by**: Guillaume (human)
- **Date**: 2026-05-30
- **Time**: (session time)
- **Notes**: Approved for implementation. All review issues resolved.
