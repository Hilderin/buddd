# Spec Review — SPEC-2026-006: Governance Refactor: Remove the Constitution

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] ~~Missing AGENTS.md non-negotiable rule update (line 23)~~ — **RESOLVED** by AC-051.
- [x] ~~Missing wiki page scope — `docs/wiki/architecture/overview.md`~~ — **RESOLVED** by AC-052.
- [x] ~~Missing wiki page scope — `docs/wiki/engineering/testing.md`~~ — **RESOLVED** by AC-053 and AC-054.
- [x] ~~Missing README.md project structure tree update~~ — **RESOLVED** by AC-055.
- [x] ~~Missing SpecKit.md references~~ — **RESOLVED** by AC-056, AC-057, and AC-058.
- [x] ~~Missing opencode.json agent description updates~~ — **RESOLVED** by AC-059, AC-060, and AC-061.
- [x] ~~Agent prompt count inconsistency (10 vs 11)~~ — **RESOLVED** by fixing Goals to say "11".

- [x] ~~Out of scope contradicts AC-059/060/061~~ — **RESOLVED**: Line 275 now reads "Changing `opencode.json` agent descriptions beyond the scope of removing constitution references" — explicitly allows constitution-reference description updates (AC-059/060/061) while excluding other unrelated changes.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- ~~**ADR-019 edge case wording**: The Edge cases table said "ADR-019 already exists (ADR-017-multi-material-model.md is the highest)" but ADR-018 also exists.~~ — **RESOLVED**: Updated to "(ADR-018 is the highest current ADR number)."

- ~~**Error cases — CI scope contradiction**: The Error cases section said CI config is outside scope but should be updated if found, yet Out of scope listed `.github/` as out of scope.~~ — **RESOLVED**: Error cases now explicitly state: "this overrides the `.github/` out-of-scope exclusion for this necessary side effect."

- ~~**`docs/wiki/README.md` — vague AC**: AC-047 was vague about "no longer contrasts with constitution".~~ — **RESOLVED**: AC-047 now explicitly mentions removing "'It is not a source of mandatory rules'" and any other constitution-related language.

- **Review of existing ADRs for CONST-001 references**: While the spec correctly says "No changes to individual ADR content," multiple ADRs (002, 003, 004, 012, 014) reference CONST-001 by file path. After migration to ADR-019 and deletion of constitution, these links will break. Consider whether ADR-019 should contain a note that previous ADRs referenced the old location, or whether the implementer should warn about this during implementation. *(Still relevant — spec has not added handling for this.)*

- **E2E verification search scope — ADR exclusion**: The E2E verification section searches for "constitution" across `docs/`, `.opencode/agents/`, `AGENTS.md`, `README.md`, `SpecKit.md`. The `docs/` glob covers `docs/adr/` where existing ADRs (002, 003, 004, 012, 014) contain historical constitution references that are intentionally not modified. The E2E section says "all results should be expected" but does not explicitly state that existing ADR constitution references are allowed. Consider adding an explicit exclusion for ADRs or noting them as expected false positives. *(Still relevant — not explicitly addressed in spec.)*

## Required changes

Concrete, actionable changes requested:

~~1. Add AC for AGENTS.md line 23~~ — **RESOLVED** (AC-051).
~~2. Add `docs/wiki/architecture/overview.md`~~ — **RESOLVED** (AC-052).
~~3. Add `docs/wiki/engineering/testing.md`~~ — **RESOLVED** (AC-053, AC-054).
~~4. Add AC for README.md directory tree~~ — **RESOLVED** (AC-055).
~~5. Add three SpecKit.md ACs~~ — **RESOLVED** (AC-056, AC-057, AC-058).
~~6. Add opencode.json description ACs~~ — **RESOLVED** (AC-059, AC-060, AC-061).
~~7. Fix "10" → "11" agent prompts~~ — **RESOLVED** (Goals now says "11").
~~8. Fix ADR-019 edge case~~ — **RESOLVED** (parenthetical now references ADR-018).

~~1. **NEW** — Resolve contradiction between Out of scope and AC-059/060/061.~~ — **RESOLVED**: Line 275 reworded to "Changing `opencode.json` agent descriptions beyond the scope of removing constitution references."

## Suggested improvements

Optional ideas (not required):

- **ADR-019 file naming consistency**: Existing ADRs use two naming conventions: `NNN-title-with-dashes.md` (e.g., `016-yaml-cpp-dependency.md`) and `ADR-NNN-title-with-dashes.md` (e.g., `ADR-017-multi-material-model.md`). The spec uses `ADR-019-architecture-boundaries.md`. This is fine but the implementer should be aware of the inconsistency and may want to standardize.

- **E2E verification scope — ADR exclusion**: The E2E verification section mentions `experiments-spec-driven-dev.md` is excluded from the constitution grep, but does not explicitly exclude existing ADR files that will still contain historical constitution references. Consider adding an explicit note that `docs/adr/` constitution references (in ADRs 001, 002, 003, 004, 012, 014) are expected and allowed.
