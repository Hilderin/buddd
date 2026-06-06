# Workflow Coordination: <feature-name>

## Orchestrator

**Feature**: `<kebab-case feature name>`
**Status**: in-progress | completed | blocked
**Current step**: `<current workflow step name>`
**Initial instructions**: `<the original feature request / human intent>`
**Notes**: `<orchestrator notes, loop history, decisions>`

## spec-author

**Status**: pending | in-progress | completed | blocked
**Summary**:
<2–5 lines describing what was done or what is needed>
**Artifacts**:
- `.specs/{{SPRINT}}/<feature>/spec.md`
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
- `.specs/{{SPRINT}}/<feature>/spec-critic.md`
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
- `.specs/{{SPRINT}}/<feature>/implementation-contract.md`
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
- `.specs/{{SPRINT}}/<feature>/implementation-contract-critic.md`
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

## Human Validation

**Status**: pending | approved | rejected
**Approver**: <git user name>
**Date**: <date and time>
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
- `.specs/{{SPRINT}}/<feature>/code-review.md`
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
- `.specs/{{SPRINT}}/<feature>/governance-review.md`
**Questions for human**:
<none, or a bullet list of questions>
**Warnings**:
<none, or a bullet list of non-blocking concerns>
**Blocking issues**:
<none, or a checklist of `- [ ]` items>

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
