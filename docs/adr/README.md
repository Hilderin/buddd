# Architecture Decision Records

ADRs document meaningful architecture decisions.

All ADRs live in this directory as flat files. Their `## Status` field (Proposed, Accepted, Superseded, Rejected) reflects the current state. ADRs are accepted when merged via PR.

## File naming convention

ADR files follow the pattern `ADR-NNN-title-with-dashes.md` where:

- `NNN` is a zero-padded, sequentially-assigned number (e.g., `001`, `019`)
- `title-with-dashes` is a short, kebab-case summary of the decision

Examples: `ADR-001-result-error-pattern.md`, `ADR-019-architecture-boundaries.md`

Use `docs/templates/adr-template.md` as the starting structure for new ADRs.
