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

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## adr-agent` and `## constitution-agent` sections must include their extra fields (`**Decisions needed**` and `**Changes needed**` respectively).
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.
