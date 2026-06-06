# Implementation Contract Review — governance-refactor-remove-constitution

**Re-review (cycle 2):** All 6 previous blocking issues are RESOLVED. The contract now correctly removes line 174 entirely (no duplicate), extends adr-agent range to include line 119 with content-based matching, fixes scout.md line 213 to avoid duplicate "wiki", adds wiki-agent.md line 21 constitution removal, adds scout.md line 273 constitution removal, and replaces vague Phase 8 with explicit verification steps. No new blocking issues found. Contract is acceptable.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Phase 2.1 — coordination-template.md constraint replacement creates duplicate line**: RESOLVED. Contract now says to remove line 174 entirely instead of replacing it with text that duplicates line 175.

- [x] **Phase 2.1 — coordination-template.md section removal has line-number dependency**: RESOLVED. Contract now specifies lines 104-119 inclusive with content-based matching ("from the `## adr-agent` heading to the next `##` heading"), eliminating the orphaned line 119.

- [x] **Phase 3.2 — scout.md line 213 replacement produces incorrect duplicate "wiki"**: RESOLVED. Contract now provides the full corrected text `"When code, specs, ADRs, or wiki disagree, flag the contradiction."` which avoids the duplicate "wiki".

- [x] **Phase 3.10 — wiki-agent.md misses constitution reference at line 21**: RESOLVED. Contract now includes explicit instruction for line 21 to remove "constitution," from the discover step.

- [x] **Phase 3.2 — scout.md misses constitution reference at line 273 (example section)**: RESOLVED. Contract now includes explicit instruction for line 273 to change to `"- relevant specs, ADRs, and wiki rules"`.

- [x] **Phase 6 — Phase 8 is vague and references a missing section**: RESOLVED. Phase 8 now contains explicit final verification steps (`git status`, `git diff --stat`, `git grep -n constitution`) instead of referencing a non-existent "After writing" section.

## Warnings

Non-blocking concerns for awareness:

- **SpecKit.md authority order differs from canonical spec order**: The contract's Phase 5.3 sets SpecKit.md authority order to `docs/adr/**` > `docs/wiki/**` > `.specs/**` > code conventions. The spec says the canonical order is `docs/adr/**` > current spec > `docs/wiki/**` > code conventions (which places specs above wiki). SpecKit.md is a starter kit and may intentionally differ, but this inconsistency should be reviewed.

- ~~**coordination.md listed as file 38 in "Files to MODIFY" but Phase 8 only describes critic workflow**~~: **OBSOLETE in re-review.** The coordination.md is no longer listed in the contract's "Files to MODIFY" list (only file 37, ending with `docs/wiki/architecture/overview.md`). This warning is resolved.

- **Phase 3.2 — scout.md example line 273 alternative phrasing**: If the implementer fixes the line 273 reference to remove "constitution", consider rephrasing as `"- relevant specs, ADRs, and wiki conventions"` (using "conventions" or removing "rules") rather than `"- relevant specs, ADRs, and wiki rules"` to avoid implying wiki content is "rules."

- **wiki-agent.md line 65 retains "constitutional"**: Line 65 says "The wiki is descriptive, not constitutional." This uses "constitutional" in a general sense (not referring to the constitution document). It is acceptable to retain, but the contract should explicitly note this is intentional to avoid confusion during implementation.

## Required changes

All 6 required changes have been implemented in the updated contract:

1. ✅ Phase 2.1: Constraint line 174 is now removed entirely (not replaced with duplicate).
2. ✅ Phase 3.2: scout.md line 213 now has full corrected text avoiding duplicate "wiki".
3. ✅ Phase 3.10: wiki-agent.md line 21 now explicitly addressed.
4. ✅ Phase 3.2: scout.md line 273 now explicitly addressed.
5. ✅ Phase 8: Replaced with explicit final verification steps.
6. ✅ Phase 2.1: adr-agent removal range now includes line 119 with content-based matching.

## Suggested improvements

Optional ideas (not required):

- Consider aligning SpecKit.md authority order with the canonical spec order (ADRs > current spec > wiki > code conventions) for consistency.
- ~~The contract could add a general instruction at the end: "After all changes, run `git grep -n constitution` on each modified file to verify no unintended constitution references remain."~~ **ADDRESSED in re-review.** Phase 8 now includes `git grep -n constitution` as a final verification step.
- Add a note about wiki-agent.md line 65 to explain why "constitutional" is intentionally retained.
